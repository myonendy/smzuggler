# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Tests for the M1 V8 attribution rules (docs/perf/buckets-v8.toml). They pin
# that attribute.py can load the file and that JIT, interpreter, GC and
# builtins stay separate, which is what Gate G1 (spec section 6) needs.
import importlib.util
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOL = ROOT / 'tools/perf/attribute.py'
RULES = ROOT / 'docs/perf/buckets-v8.toml'
SPEC = importlib.util.spec_from_file_location('attribute', TOOL)
ATTR = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ATTR)


def bucket_for(symbol, dso):
    """Attribute one synthetic frame under the shipped V8 rules."""
    buckets, rules, fallback = ATTR.load_rules(RULES)
    del buckets, fallback
    rule = ATTR.classify((0x1000, symbol, dso), rules)
    return rule['bucket'] if rule else ''


class V8BucketTests(unittest.TestCase):
    """Keep the V8 split stable and loadable by the attribution tool."""

    def test_v8_buckets_are_declared(self):
        """The four Gate G1 buckets exist with their report labels."""
        buckets, _rules, _fallback = ATTR.load_rules(RULES)
        labels = dict(buckets)
        self.assertEqual(labels['v8_jit'], 'V8 JIT code')
        self.assertEqual(labels['v8_interp'], 'V8 interpreter')
        self.assertEqual(labels['gc'], 'GC')
        self.assertEqual(labels['v8_builtin'], 'V8 builtins')

    def test_jit_frames_keep_their_script_origin(self):
        """A JIT frame is shim or game JS by its script URL."""
        self.assertEqual(bucket_for('JS:*render file:///x/shims/a.js:1:1',
                                    'perf-1.map'), 'shim_js')
        self.assertEqual(bucket_for('JS:*render file:///x/js/a.js:1:1',
                                    'perf-1.map'), 'game_js')

    def test_interpreter_gc_and_builtins_are_separate(self):
        """Ignition, GC and builtin frames never collapse into each other."""
        cases = [
            ('BytecodeHandler:LdaConstant', 'perf-1.map', 'v8_interp'),
            ('Builtin:ArrayPush', 'perf-1.map', 'v8_builtin'),
            ('v8::internal::Interpreter::Run', 'smzuggler', 'v8_interp'),
            ('v8::internal::compiler::PipelineImpl::Run', 'smzuggler',
             'v8_jit'),
            ('v8::internal::Runtime_ArrayPush', 'smzuggler', 'v8_builtin'),
            ('v8::internal::Heap::CollectGarbage', 'smzuggler', 'gc'),
        ]
        for symbol, dso, expected in cases:
            with self.subTest(symbol=symbol):
                self.assertEqual(bucket_for(symbol, dso), expected)


if __name__ == '__main__':
    unittest.main()
