# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Unit tests for the §14.5 analyser and its report gate words."""
from __future__ import annotations
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools.perf import analyse  # noqa: E402
def frame(number: int, work: float, present: float = 16.0) -> dict:
    """One frame record in frame_stats.c's shape."""
    return {"type": "frame", "frame": number, "work_ms": work, "gpu_ms": None,
            "present_interval_ms": present, "missed_vblank": present > 24.0,
            "over_budget": work > 16.7}
def frames(count: int, work: float = 8.0,
           present: float = 16.0) -> list[dict]:
    """A contiguous run of ordinary frames."""
    return [frame(i, work, present) for i in range(count)]
def marker(number: int, kind: str, phase: str = "point") -> dict:
    """A scene point or save/load boundary."""
    return {"type": "marker", "frame": number, "kind": kind, "phase": phase}
def run(records: list[dict], profile: str = "dev-laptop",
        warmup: int = 0) -> dict:
    """Analyse the whole list as the scored run."""
    return analyse.analyse_records(records, profile, "S1", warmup,
                                   len(records) - warmup, 60.0)
class Tests(unittest.TestCase):
    """§14.7 gate words and §14.5 rules 1-4."""
    def test_gate_words(self) -> None:
        self.assertEqual(analyse.gate_name("dev-laptop", True), "pass")
        self.assertEqual(analyse.gate_name("dev-laptop", False), "fail")
        self.assertEqual(analyse.gate_name("steam-deck", True), "confirm-pass")
        self.assertEqual(analyse.gate_name("steam-deck", False), "confirm-fail")
        self.assertEqual(analyse.gate_name("deck-proxy", True), "proxy")
        self.assertEqual(analyse.gate_name("extra", False), "extra")
        with self.assertRaises(ValueError):
            analyse.gate_name("toaster", True)
    def test_pass_summary_and_gate(self) -> None:
        report = run(frames(100))
        self.assertEqual(report["gate"], "pass")
        self.assertEqual(report["summary"]["p99_work_ms"], 8.0)
        self.assertEqual(report["summary"]["p50_work_ms"], 8.0)
        self.assertTrue(all(item["ok"] for item in report["criteria"].values()))
    def test_rule1_p99_and_laptop_limit(self) -> None:
        records = frames(100)
        records[98]["work_ms"] = records[99]["work_ms"] = 18.0
        report = run(records)
        self.assertEqual(report["gate"], "fail")
        self.assertFalse(report["criteria"]["p99_work_ms"]["ok"])
        self.assertEqual(report["criteria"]["p99_work_ms"]["value"], 18.0)
        tight = frames(100, work=16.69)
        self.assertEqual(run(tight)["gate"], "fail")
        self.assertEqual(run(tight, "steam-deck")["gate"], "confirm-pass")
    def test_rule2_p99_9_fails(self) -> None:
        records = frames(1000)
        records[998]["work_ms"] = records[999]["work_ms"] = 30.0
        report = run(records)
        self.assertTrue(report["criteria"]["p99_work_ms"]["ok"])
        self.assertFalse(report["criteria"]["p99_9_work_ms"]["ok"])
        self.assertEqual(report["gate"], "fail")
    def test_rule3_missed_vblanks(self) -> None:
        records = frames(100, present=30.0)
        self.assertFalse(run(records)["criteria"]["missed_vblank_percent"]["ok"])
        for record in records[:99]:
            record["present_interval_ms"] = 16.0
        self.assertTrue(run(records)["criteria"]["missed_vblank_percent"]["ok"])
    def test_rule4_and_excluded_windows(self) -> None:
        records = frames(100)
        records[50]["work_ms"] = 60.0
        self.assertEqual(run(records)["gate"], "fail")
        records.insert(50, marker(50, "scene"))
        report = run(records)
        self.assertTrue(report["criteria"]["max_work_ms"]["ok"])
        self.assertGreaterEqual(report["frames_excluded"], 31)
        save = frames(100)
        save[45]["work_ms"] = 60.0
        save.insert(40, marker(40, "save", "begin"))
        save.insert(53, marker(52, "save", "end"))
        self.assertTrue(run(save)["criteria"]["max_work_ms"]["ok"])
    def test_warmup_and_worst_run(self) -> None:
        records = frames(80)
        for record in records[:20]:
            record["work_ms"] = 60.0
        self.assertEqual(run(records, warmup=20)["frames_scored"], 60)
        combined = analyse.combine_runs(
            [run(frames(100)), run(frames(100, work=20.0))], "dev-laptop", "S1")
        self.assertEqual(combined["gate"], "fail")
        self.assertEqual(len(combined["runs"]), 2)
    def test_cli_report_files(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / "frame-stats-dev-laptop-S1-run1.jsonl"
            source.write_text("\n".join(json.dumps(r) for r in frames(100)),
                              encoding="utf-8")
            out = root / "reports"
            result = subprocess.run(
                [sys.executable, str(ROOT / "tools" / "perf" / "analyse.py"),
                 "--input", str(source), "--warmup-frames", "0",
                 "--scored-frames", "100", "--out-dir", str(out)],
                capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads(
                (out / "perf-report-dev-laptop-S1.json").read_text("utf-8"))
            self.assertEqual(report["gate"], "pass")
            markdown = (out / "perf-report-dev-laptop-S1.md").read_text("utf-8")
            self.assertIn("Gate: **pass**", markdown)
            self.assertNotIn(str(root), markdown)
if __name__ == "__main__":
    unittest.main()
