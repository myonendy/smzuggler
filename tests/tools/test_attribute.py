# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Tests for tools/perf/attribute.py (spec section 6.1, docs/perf.md). The input
# is the hand-written tests/tools/fixtures/perf/mixed.perf.txt; outputs go to
# temp dirs. They pin exact bucket weights, kernel and [unknown] handling, and
# that reports never carry an absolute path.
import importlib.util
import json
import pathlib
import re
import subprocess
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL = ROOT / 'tools/perf/attribute.py'
FIXTURE = ROOT / 'tests/tools/fixtures/perf/mixed.perf.txt'
SPEC = importlib.util.spec_from_file_location('attribute', TOOL)
ATTR = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ATTR)
ABSOLUTE = re.compile(r'(?:^|[\s"(`|:=])/[A-Za-z0-9._-]+/|file:|[A-Za-z]:\\')

# Weights (summed periods) per bucket for the fixture, worked out by hand.
SELF = {'quickjs': 1000, 'gc': 2000, 'shim_js': 1000, 'game_js': 1000,
        'canvas2d': 2000, 'renderer': 1000, 'audio': 500, 'video': 500,
        'other': 2000}
TOTAL = {'quickjs': 4000, 'gc': 2000, 'shim_js': 1000, 'game_js': 2000,
         'canvas2d': 2000, 'renderer': 1000, 'audio': 500, 'video': 500,
         'other': 4500}


def run_tool(*args, stdin=None):
    """Run the tool as a subprocess so the CLI contract is what is tested."""
    return subprocess.run([sys.executable, str(TOOL), *args], input=stdin,
                          capture_output=True, text=True, check=False)


def fixture_report():
    """Attribute the fixture in-process with the shipped rules."""
    buckets, rules, fallback = ATTR.load_rules(ATTR.DEFAULT_RULES)
    with open(FIXTURE, encoding='utf-8') as handle:
        samples = ATTR.parse_samples(handle)
    return ATTR.summarise(samples, buckets, rules, fallback)


class AttributeTests(unittest.TestCase):
    """Pin the spec section 6.1 bucket split on a known synthetic profile."""

    def setUp(self):
        """Index the fixture's bucket rows by id."""
        self.report = fixture_report()
        self.rows = {row['id']: row for row in self.report['buckets']}

    def test_exact_bucket_weights(self):
        """Self and total weights match the hand-computed values exactly."""
        self.assertEqual(list(self.rows), list(SELF))
        self.assertEqual({k: r['self_weight'] for k, r in self.rows.items()}, SELF)
        self.assertEqual({k: r['total_weight'] for k, r in self.rows.items()}, TOTAL)
        self.assertEqual(sum(SELF.values()), self.report['diagnostics']['weight'])

    def test_sample_counts_and_percentages(self):
        """Counts and percentages derive from the same attribution."""
        self.assertEqual(self.rows['canvas2d']['self_samples'], 2)
        self.assertEqual(self.rows['other']['total_samples'], 5)
        self.assertEqual(self.rows['quickjs']['self_pct'], 9.09)
        self.assertEqual(self.rows['quickjs']['total_pct'], 36.36)
        self.assertEqual(self.rows['other']['total_pct'], 40.91)
        self.assertEqual(sum(r['self_samples'] for r in self.rows.values()), 11)

    def test_kernel_and_unknown_frames(self):
        """Kernel and [unknown] frames pass self time to their caller."""
        diag = self.report['diagnostics']
        self.assertEqual(diag['samples'], 11)
        self.assertEqual(diag['kernel_samples'], 2)
        self.assertEqual(diag['unknown_frames'], 3)
        self.assertEqual(diag['unclaimed_samples'], 2)
        buckets, rules, fallback = ATTR.load_rules(ATTR.DEFAULT_RULES)
        syscall = [(0xffffffff81234567, 'entry_SYSCALL_64', '[kernel.kallsyms]'),
                   (0x7f0000005000, '[unknown]', 'libnvidia-glcore.so.550.0'),
                   (0x55d000004000, 'gpu_submit', 'smzuggler')]
        self.assertEqual(ATTR.attribute_sample(syscall, rules, fallback)[0], 'renderer')
        unknown = [(0x55d000009000, '[unknown]', '[unknown]'),
                   (0x55d000003000, 'canvas2d_fill_text', 'smzuggler')]
        self.assertEqual(ATTR.attribute_sample(unknown, rules, fallback)[0], 'canvas2d')
        idle = [(0xffffffff81000100, '[unknown]', '[unknown]')]
        self.assertEqual(ATTR.attribute_sample(idle, rules, fallback),
                         ('other', {'other'}, False))

    def test_frame_parsing(self):
        """C++ parentheses, (deleted) DSOs and offsets parse correctly."""
        frame = ATTR.parse_frame('\t 7f00 std::function<void ()>::operator()() const+0x20 '
                                 '(/usr/lib/libstdc++.so.6)')
        self.assertEqual(frame, (0x7f00, 'std::function<void ()>::operator()() const',
                                 'libstdc++.so.6'))
        frame = ATTR.parse_frame('\t 10 vpx_codec_decode+0x4 (/opt/x/libvpx.so.9 (deleted))')
        self.assertEqual(frame, (0x10, 'vpx_codec_decode', 'libvpx.so.9'))
        self.assertIsNone(ATTR.parse_frame('\tnot a frame'))

    def test_missing_period_weighs_one(self):
        """Without a printed period every sample weighs 1."""
        text = ['app 1/1 [000] 1.000001: cycles:u:\n', '\t 10 JS_Eval (/opt/a/app)\n', '\n',
                'app 1/1 [000] 1.000002: cycles:u:\n', '\t 20 canvas2d_save (/opt/a/app)\n']
        samples = ATTR.parse_samples(text)
        self.assertEqual([s['weight'] for s in samples], [1, 1])
        with self.assertRaises(ValueError):
            ATTR.parse_samples(['\t 10 JS_Eval (/opt/a/app)\n'])

    def test_no_absolute_path_in_outputs(self):
        """JSON and Markdown carry basenames only, though the input has paths."""
        self.assertRegex(FIXTURE.read_text(encoding='utf-8'), ABSOLUTE)
        with tempfile.TemporaryDirectory() as tmp:
            out_json, out_md = pathlib.Path(tmp, 'r.json'), pathlib.Path(tmp, 'r.md')
            result = run_tool(str(FIXTURE), '--json', str(out_json), '--markdown', str(out_md))
            self.assertEqual(result.returncode, 0, result.stderr)
            data, table = out_json.read_text(), out_md.read_text()
        for text in (data, table):
            self.assertIsNone(ABSOLUTE.search(text), ABSOLUTE.search(text))
        report = json.loads(data)
        self.assertEqual(report['source'], 'mixed.perf.txt')
        symbols = {leaf['symbol'] for leaf in report['top_leaves']}
        self.assertIn('JS:*updateTransform pixi_shim.js:120:10', symbols)
        self.assertIn('| QuickJS interpreter | 9.09 | 36.36 | 1 | 3 |', table)

    def test_stdin_and_bad_rules(self):
        """stdin input prints Markdown; an unknown bucket fails loudly."""
        result = run_tool(stdin=FIXTURE.read_text(encoding='utf-8'))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('| GC | 18.18 | 18.18 | 1 | 1 |', result.stdout)
        with tempfile.TemporaryDirectory() as tmp:
            rules = pathlib.Path(tmp, 'b.toml')
            rules.write_text('fallback = "x"\n[[bucket]]\nid = "x"\nlabel = "X"\n'
                             '[[rule]]\nsymbol = "^a"\nbucket = "nope"\n')
            result = run_tool(str(FIXTURE), '--rules', str(rules))
        self.assertEqual(result.returncode, 2)
        self.assertIn("unknown bucket 'nope'", result.stderr)


if __name__ == '__main__':
    unittest.main()
