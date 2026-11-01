# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Guard local evaluator scenarios; see docs/testing.md.

Game-free tests keep the driver recipes portable and private. No game is read.
"""
import json
import os
import re
from pathlib import Path
import sys
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools.evaluator.scenario import load_scenario
from tools.evaluator import smzuggler


class ScenarioTests(unittest.TestCase):
    """Exercise recipes and preflight without starting a game process."""

    def test_scenario_schema_and_portability(self):
        """Validate every new recipe with the evaluator's actual schema loader."""
        paths = sorted((ROOT / 'tools/evaluator/scenarios').glob('smz-*.json'))
        self.assertTrue(paths)
        # Reuse the public names gate's policy, never the private deny-list.
        policy = (ROOT / 'tools/forge/game_names.sh').read_text()
        forbidden = re.search(r"^pattern='([^']+)'", policy, re.MULTILINE).group(1)
        for path in paths:
            self.assertIsNone(re.search(forbidden, path.read_text(), re.IGNORECASE))
            data = load_scenario(path)
            self.assertTrue(data['steps'])
            self.assertIn('SPDX-License-Identifier: GPL-3.0-or-later', data['_header'])
            for value in smzuggler.strings(data):
                self.assertFalse(os.path.isabs(value))
                self.assertNotRegex(value, r'(?i)([a-z]:[\\/]|\.rmmzsave|\.rpgsave|steamapps|compatdata)')
            self.assertEqual(data['name'], 'title-map-menu-save')
            self.assertEqual([s['wait'] for s in data['steps'] if 'wait' in s], [60, 240, 300])
            self.assertIn('DataManager.saveGame(1)', path.read_text())
            self.assertIn('DataManager.loadGame(1)', path.read_text())

    def test_loaded_prelude_keeps_real_storage(self):
        """The prelude the host loads must not stub save, load or exists."""
        stub = re.compile(r'StorageManager\.(saveObject|loadObject|exists)\s*=')
        upstream = (smzuggler.HERE / 'prelude.js').read_text()
        # Upstream stubs all three, so an absent match below is meaningful.
        self.assertEqual(sorted(set(stub.findall(upstream))), ['exists', 'loadObject', 'saveObject'])
        loaded = smzuggler.prelude_text()
        self.assertIsNone(stub.search(loaded))
        self.assertNotIn('patchStorage', loaded)
        for kept in ('Math.random = function', 'ev.waitScene = function', 'ev.newGame = function'):
            self.assertIn(kept, loaded)
        with tempfile.TemporaryDirectory() as tmp:
            run = Path(tmp)
            with patch.object(smzuggler, 'stage_game', return_value=run), \
                 patch.object(smzuggler, 'game_resolution', return_value=(640, 480)), \
                 patch.object(smzuggler, 'OutsiderDriver', side_effect=smzuggler.DriverError('stop')) as driver:
                with self.assertRaises(smzuggler.DriverError):
                    smzuggler.play(run / 'host', run, run, {'steps': []})
            self.assertEqual(driver.call_args.args[2], loaded)
            self.assertEqual(driver.call_args.args[3:5], (640, 480))

    def test_prelude_cut_fails_loudly_without_markers(self):
        """A moved upstream section must stop the run rather than keep stubs."""
        with tempfile.TemporaryDirectory() as tmp:
            (Path(tmp) / 'prelude.js').write_text('StorageManager.saveObject = 0;')
            with patch.object(smzuggler, 'HERE', Path(tmp)):
                with self.assertRaisesRegex(ValueError, 'markers'):
                    smzuggler.prelude_text()

    def test_map_step_checks_map_92_directly(self):
        """Map readiness compares the live map ID to 92, not the start-map setting."""
        evals = [s['eval'] for s in load_scenario(smzuggler.SCENARIO)['steps'] if 'eval' in s]
        self.assertTrue(any('$gameMap.mapId() !== 92' in e for e in evals))
        self.assertFalse(any('startMapId' in e for e in evals))

    def test_scratch_save_check_requires_slot_file(self):
        """Save must reach the scratch save directory in either storage mode."""
        with tempfile.TemporaryDirectory() as tmp:
            stage = Path(tmp)
            (stage / 'save').mkdir()
            with self.assertRaisesRegex(smzuggler.DriverError, 'slot 1'):
                smzuggler.check_scratch_save(stage)
            (stage / 'save' / 'forage_rmmzsave.x.file1').write_text('{}')
            smzuggler.check_scratch_save(stage)

    def test_failed_launch_keeps_original_error_and_summary(self):
        """A constructor failure must survive cleanup without timing evidence."""
        with tempfile.TemporaryDirectory() as tmp:
            run = Path(tmp)
            with patch.object(smzuggler, 'stage_game', return_value=run), \
                 patch.object(smzuggler, 'OutsiderDriver', side_effect=smzuggler.DriverError('boot failed')), \
                 patch.object(smzuggler, 'timing_report') as timing:
                with self.assertRaisesRegex(smzuggler.DriverError, 'boot failed'):
                    smzuggler.play(run / 'host', run, run, {'steps': []})
            timing.assert_not_called()
            self.assertEqual(json.loads((run / 'summary.json').read_text()),
                             [{'step': 'window-close', 'status': 'not-run'}])

    def test_step_failure_survives_missing_perf_samples(self):
        """An early child failure must retain its diagnostic and partial summary."""
        with tempfile.TemporaryDirectory() as tmp:
            run = Path(tmp)
            with patch.object(smzuggler, 'stage_game', return_value=run), \
                 patch.object(smzuggler, 'OutsiderDriver') as driver, \
                 patch.object(smzuggler, 'Runner') as runner:
                driver.return_value.evaluate.return_value = 0
                runner.return_value.run.side_effect = smzuggler.DriverError('child died')
                with self.assertRaisesRegex(smzuggler.DriverError, 'child died'):
                    smzuggler.play(run / 'host', run, run, {'steps': [{'wait': 1}]})
            driver.return_value.close.assert_called_once()
            self.assertTrue((run / 'summary.json').is_file())

    def test_preflight_refuses_store_paths_and_symlinks(self):
        """Reject store components before resolving a symlink can hide them."""
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            game = root / 'copy'
            game.mkdir()
            for component in ('steamapps', 'COMPATDATA'):
                link = root / component
                link.symlink_to(game, target_is_directory=True)
                with self.assertRaisesRegex(ValueError, 'Steam or Proton'):
                    smzuggler.paths({'SMZUGGLER_GAME_DIR': str(link)}, root)

    def test_output_cannot_be_inside_game(self):
        """Prevent state configuration from placing scratch writes in the input."""
        with tempfile.TemporaryDirectory() as tmp:
            game = Path(tmp)
            with self.assertRaisesRegex(ValueError, 'overlap'):
                smzuggler.paths({'SMZUGGLER_GAME_DIR': tmp, 'XDG_STATE_HOME': tmp}, game)

    def test_dry_run_has_no_writes(self):
        """Preflight prints resolved destinations without creating the state tree."""
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            game = root / 'copy'
            game.mkdir()
            state = root / 'state'
            with patch('builtins.print'), patch.dict(os.environ, {'SMZUGGLER_GAME_DIR': str(game),
                                        'XDG_STATE_HOME': str(state)}):
                self.assertEqual(smzuggler.main(['--dry-run']), 0)
            self.assertFalse(state.exists())

    def test_scratch_root_excludes_saves_and_logs(self):
        """Ensure the host gets fresh writable storage rather than owner saves."""
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            game, run = root / 'copy', root / 'run'
            game.mkdir()
            run.mkdir()
            for name in ('data', 'save', 'saves'):
                (game / name).mkdir()
            (game / 'debug.log').write_text('private')
            stage = smzuggler.stage_game(game, run)
            self.assertEqual([p.name for p in stage.iterdir()], ['data'])
            self.assertFalse((stage / 'data').is_symlink())
            (stage / 'data' / 'scratch.txt').write_text('scratch')
            self.assertFalse((game / 'data' / 'scratch.txt').exists())
            self.assertEqual(list((game / 'save').iterdir()), [])

    def test_frame_csv_uses_native_samples(self):
        """Preserve phase counters without presenting step wall time as frame time."""
        with tempfile.TemporaryDirectory() as tmp:
            run = Path(tmp)
            (run / 'runtime.log').write_text(
                'perf: 1 frames avg 16.2 ms (events 0.1, timers 0.2, jobs 0.3, rAF 10.0, swap 5.6)\n')
            smzuggler.timing_report(run)
            self.assertIn('1,16.2,0.1,0.2,0.3,10.0,5.6', (run / 'frame-times.csv').read_text())
            self.assertEqual(json.loads((run / 'perf.json').read_text())['samples'], 1)

    def test_bundle_stages_only_allowed_reports(self):
        """Keep scratch inputs out and name JSON reports for the bundler allowlist."""
        with tempfile.TemporaryDirectory() as tmp:
            run = Path(tmp)
            (run / 'summary.json').write_text('[]')
            (run / 'perf.json').write_text('{"samples": 1}')
            (run / 'runtime.log').write_text('diagnostic')
            (run / 'input').mkdir()
            with patch.object(smzuggler.subprocess, 'run') as invoke, \
                 patch.object(smzuggler, 'add_step_summary') as summary:
                invoke.return_value.returncode = 0
                self.assertEqual(smzuggler.bundle(run / 'input', run), 0)
            summary.assert_called_once_with(run / 'bundle.zip', [])
            self.assertEqual({p.name for p in (run / 'artifacts').iterdir()},
                             {'perf-report-evaluator.json', 'perf-report-steps.json', 'runtime.log'})
            self.assertIn('--include-screenshots', invoke.call_args.args[0])
