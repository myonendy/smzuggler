# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Local, isolated control scenarios for the QuickJS host; see docs/testing.md.

This entry point wraps the upstream driver without changing its NW.js workflow.
The host runs against a disposable copy under the run directory. Calls are serial and
all screenshots remain local; only the packager may create a shareable archive.
"""
import argparse
import csv
import json
import os
import re
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import zipfile

from .drivers import OutsiderDriver, DriverError
from .scenario import Runner, load_scenario
from .cli import game_resolution

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
SCENARIO = HERE / 'scenarios/smz-basics.json'
STORAGE_BEGIN = '/* --- Storage isolation'
STORAGE_END = '/* --- Stay windowed'


def prelude_text():
    """Return the upstream prelude without its StorageManager stubs.

    Upstream's prelude replaces saveObject, loadObject and exists so its own
    visual scenarios never touch player data; that would turn this scenario's
    save and reload into no-ops. Here the host only sees a scratch copy without
    owner saves, so real slot IO is safe. The section is cut between upstream's
    own section headers; raise ValueError if they move instead of shipping stubs.
    """
    text = (HERE / 'prelude.js').read_text(encoding='utf-8')
    begin, end = text.find(STORAGE_BEGIN), text.find(STORAGE_END)
    if begin < 0 or end < begin:
        raise ValueError('prelude.js storage section markers not found')
    return text[:begin] + text[end:]


def strings(value):
    """Yield borrowed string leaves of JSON so portability checks cover metadata."""
    if isinstance(value, str):
        yield value
    elif isinstance(value, dict):
        for item in value.values():
            yield from strings(item)
    elif isinstance(value, list):
        for item in value:
            yield from strings(item)


def paths(env, home):
    """Resolve input and output without writes; reject store paths and overlap.

    env is a string mapping, home a Path used only for the XDG default.
    Return owned Paths or raise ValueError before any launch or output creation.
    """
    raw = env.get('SMZUGGLER_GAME_DIR')
    if not raw:
        raise ValueError('BLOCKED: needs dev-laptop GUI run (SMZUGGLER_GAME_DIR unset)')
    game = Path(raw).expanduser()
    for candidate in (game.absolute(), game.resolve()):
        if {'steamapps', 'compatdata'} & {p.lower() for p in candidate.parts}:
            raise ValueError('refusing Steam or Proton game directory')
    game = game.resolve()
    state = Path(env.get('XDG_STATE_HOME') or home / '.local/state').expanduser().resolve()
    out = state / 'smzuggler-rt/evaluator'
    if out.is_relative_to(game) or game.is_relative_to(out):
        raise ValueError('game and output directories overlap')
    if not game.is_dir():
        raise ValueError('BLOCKED: needs dev-laptop GUI run (game copy unavailable)')
    return game, out


def stage_game(game, run):
    """Copy inputs into scratch storage so native writes cannot reach the source.

    Do not copy owner saves or logs. Symlinks are dereferenced into independent
    files; copy errors propagate before launch. The caller owns the resulting
    Path and must exclude the entire scratch tree from share archives.
    """
    stage = run / 'input'
    stage.mkdir()
    for item in game.iterdir():
        if item.name.lower() in {'save', 'saves', 'debug.log', 'console.log'}:
            continue
        if item.suffix.lower() in {'.rmmzsave', '.rpgsave'}:
            continue
        target = stage / item.name
        if item.is_dir():
            shutil.copytree(item, target, symlinks=False)
        else:
            shutil.copyfile(item, target)
    return stage


def quiet(message):
    """Discard driver step descriptions so game text cannot enter the summary."""
    return None


def play(exe, game, run, scenario):
    """Drive steps serially and request window close, requiring exit status zero.

    Inputs are Paths and a validated scenario. Outputs stay under run. Exceptions
    abort the run; finally always releases the child. Frame counts are logical
    evaluator frames, not a substitute for native presentation-time measurements.
    """
    stage = stage_game(game, run)
    results = [{'step': index, 'status': 'not-run'}
               for index in range(len(scenario['steps']))]
    results.append({'step': 'window-close', 'status': 'not-run'})
    driver = None
    try:
        width, height = game_resolution(str(stage))
        driver = OutsiderDriver(str(exe), str(stage), prelude_text(),
                            width, height, shims_dir=str(ROOT / 'src/shims'),
                            log_path=str(run / 'runtime.log'), cwd=str(run),
                            extra_args=['--perf', '1'])
        runner = Runner(driver, str(run / 'shots'), log=quiet)
        with (run / 'steps.csv').open('w') as stream:
            writer = csv.writer(stream)
            writer.writerow(['step', 'start_frame', 'end_frame', 'wall_ms', 'status'])
            for index, step in enumerate(scenario['steps']):
                before = driver.evaluate('__ev.frame')
                start = time.monotonic()
                result = runner.run([step])[-1]
                after = driver.evaluate('__ev.frame')
                results[index] = {'step': index, 'status': result.status,
                                  'start_frame': before, 'end_frame': after}
                writer.writerow([index, before, after, (time.monotonic()-start)*1000, result.status])
                if result.status != 'ok':
                    raise DriverError('scenario step failed: %d' % index)
        check_scratch_save(stage)
        # The same NW.js close API used by the game's window controls.
        driver._send('eval', json.dumps('nw.Window.get().close()'))
        code = driver.proc.wait(timeout=15)
        results[-1] = {'step': 'window-close', 'status': 'ok' if code == 0 else 'error'}
        if code != 0:
            raise DriverError('unclean window exit')
    finally:
        try:
            if driver is not None:
                driver.close()
                driver.reader.join(timeout=5)
        finally:
            (run / 'summary.json').write_text(json.dumps(results, indent=2) + '\n')
    timing_report(run)


def check_scratch_save(stage):
    """Prove slot 1 reached disk in the scratch copy, not only in memory.

    stage is the scratch game root the host was given; the host derives its
    save directory from it; local mode writes file1.rmmzsave and localforage
    mode a forage_ file whose key ends in file1. Raise DriverError when neither exists.
    """
    save = stage / 'save'
    if not (list(save.glob('file1.*')) or list(save.glob('forage_*file1'))):
        raise DriverError('slot 1 was not written to the scratch save directory')


def timing_report(run):
    """Extract native one-frame perf samples into CSV, preserving milliseconds.

    The host rounds its counters to 0.1 ms; do not use these debug-mode samples
    for the device acceptance gate. Missing samples fail rather than fabricate
    timing evidence. The caller owns every output and calls after driver close.
    """
    pattern = re.compile(r'perf: 1 frames avg ([\d.]+) ms \(events ([\d.]+), '
                         r'timers ([\d.]+), jobs ([\d.]+), rAF ([\d.]+), swap ([\d.]+)\)')
    samples = pattern.findall((run / 'runtime.log').read_text(errors='replace'))
    with (run / 'frame-times.csv').open('w') as stream:
        writer = csv.writer(stream)
        writer.writerow(['frame', 'total_ms', 'events_ms', 'timers_ms', 'jobs_ms', 'raf_ms', 'swap_ms'])
        for index, sample in enumerate(samples):
            writer.writerow([index + 1, *sample])
    (run / 'perf.json').write_text(json.dumps({'samples': len(samples),
        'precision_ms': 0.1, 'control_mode': True,
        'mean_total_ms': sum(float(s[0]) for s in samples) / len(samples) if samples else None}) + '\n')
    if not samples:
        raise DriverError('no native frame-time samples recorded')


def bundle(game, run, denylist=None):
    """Package only the artifact directory through the privacy-scanning tool.

    Screenshots are explicitly included in the local archive; saves are excluded.
    denylist is an optional deny-list path passed through to the packager,
    which otherwise uses its default lookup. The packager refuses privacy
    findings or missing configuration; on success the per-step summary is added
    to the archive's README.txt. Return the packager's process status.
    """
    artifacts = run / 'artifacts'
    artifacts.mkdir()
    for name in ('runtime.log', 'steps.csv', 'frame-times.csv', 'perf.json'):
        path = run / name
        if path.exists():
            archive_name = {'perf.json': 'perf-report-evaluator.json'}.get(name, name)
            shutil.copyfile(path, artifacts / archive_name)
    results = None
    if (run / 'summary.json').exists():
        # The packager reads perf-report JSON as an object, so wrap the list.
        results = json.loads((run / 'summary.json').read_text())
        (artifacts / 'perf-report-steps.json').write_text(json.dumps({'steps': results}) + '\n')
    if (run / 'shots').exists():
        shutil.copytree(run / 'shots', artifacts / 'shots')
    output = run / 'bundle.zip'
    command = [sys.executable, str(ROOT / 'tools/share/make_bundle.py'),
               str(artifacts), '--include-screenshots', '--game-dir', str(game),
               '--output', str(output)]
    if denylist:
        command += ['--denylist', str(denylist)]
    status = subprocess.run(command).returncode
    if status == 0 and results is not None:
        add_step_summary(output, results)
    return status


def step_lines(results):
    """Render summary.json entries as fixed-vocabulary pass/fail lines.

    The packager's README has no step section, so this one is appended after
    its scan. Only integer step indices, 'window-close' and known statuses are
    emitted; anything else raises ValueError, so no free text reaches the
    archive without having been scanned.
    """
    words = {'ok': 'pass', 'not-run': 'not run'}
    lines = ['', 'Steps (pass/fail):']
    for item in results:
        step = item['step']
        if not (isinstance(step, int) or step == 'window-close'):
            raise ValueError('unexpected step label in summary.json')
        status = item['status']
        if not isinstance(status, str) or not re.fullmatch(r'[a-z-]+', status):
            raise ValueError('unexpected step status in summary.json')
        label = 'step %d' % step if isinstance(step, int) else step
        lines.append('%s: %s' % (label, words.get(status, 'fail')))
    return '\n'.join(lines) + '\n'


def add_step_summary(archive, results):
    """Rewrite archive with step_lines appended to its README.txt.

    The new zip is written beside archive and replaces it only when complete,
    so a failure leaves the packager's bundle intact. Entries keep their order.
    """
    temporary = archive.with_name(archive.name + '.tmp')
    with zipfile.ZipFile(archive) as source, \
         zipfile.ZipFile(temporary, 'w', zipfile.ZIP_DEFLATED) as target:
        for info in source.infolist():
            data = source.read(info)
            if info.filename == 'README.txt':
                data += step_lines(results).encode('utf-8')
            target.writestr(info, data)
    os.replace(temporary, archive)


def main(argv=None):
    """Run local preflight or a single isolated session; return a CLI exit code.

    No game path is inferred. --dry-run and --print-prelude have no filesystem
    side effects; --print-prelude writes the scenario prelude to stdout. Real
    runs require the owner's explicit confirmation of the manual battery gate.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dry-run', action='store_true')
    parser.add_argument('--print-prelude', action='store_true',
                        help='print the storage-preserving prelude for NW.js references')
    parser.add_argument('--runtime', type=Path, default=ROOT / 'build/smzuggler-rt')
    parser.add_argument('--battery-gate-confirmed', action='store_true',
                        help='owner verified power-saver, unplugged >=2 min, battery >=50%%')
    args = parser.parse_args(argv)
    try:
        if args.print_prelude:
            sys.stdout.write(prelude_text())
            return 0
        game, out = paths(os.environ, Path.home())
        scenario = load_scenario(SCENARIO)
        # Paths are printed only to the local terminal, never to the summary.
        print('game:', game, '\noutput:', out, '\nscratch saves: <run>/input/save')
        print('runtime:', args.runtime.resolve(), '\nscenario:', SCENARIO)
        if args.dry_run:
            return 0
        if not args.battery_gate_confirmed or not args.runtime.is_file():
            raise ValueError('BLOCKED: needs dev-laptop GUI run (battery gate or runtime unavailable)')
        if not (os.environ.get('DISPLAY') or os.environ.get('WAYLAND_DISPLAY')):
            raise ValueError('BLOCKED: needs dev-laptop GUI run (display unavailable)')
        out.mkdir(parents=True, exist_ok=True)
        run = Path(tempfile.mkdtemp(prefix='run-', dir=out))
        try:
            play(args.runtime.resolve(), game, run, scenario)
        finally:
            status = bundle(game, run)
        return status
    except (ValueError, OSError, DriverError, subprocess.TimeoutExpired) as error:
        print(str(error), file=sys.stderr)
        return 2


if __name__ == '__main__':
    raise SystemExit(main())
