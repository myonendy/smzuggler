#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Turn --frame-stats JSON Lines into spec §14.5 reports and §14.7 gate words.
Reads one or more runs, drops the 10 s warm-up and the rule-5 transition and
save windows, applies rules 1-4, and keeps the worst run. Reports carry only
profile and scene names. Exit: 0 written, 1 bad input, 2 usage.
"""
from __future__ import annotations
import argparse
import json
import math
from pathlib import Path
import re
import sys
PROFILES = ("dev-laptop", "steam-deck", "deck-proxy", "extra")
LIMITS = {"p99-laptop": 16.69, "p99-deck": 16.7, "p99.9": 25.0,
          "max": 50.0, "missed": 1.0}
SCENE_RE = re.compile(
    r"frame-stats-(?P<profile>[a-z-]+)-(?P<scene>S[0-9]+)-run[0-9]+\.jsonl$")
class InputError(Exception):
    """An input file is unreadable or has no scored frames."""
def gate_name(profile: str, passed: bool) -> str:
    """Map a result to the §14.7 word; only the laptop run is the gate."""
    if profile == "dev-laptop":
        return "pass" if passed else "fail"
    if profile == "steam-deck":
        return "confirm-pass" if passed else "confirm-fail"
    if profile == "deck-proxy":
        return "proxy"
    if profile == "extra":
        return "extra"
    raise ValueError(f"unknown profile: {profile}")
def p99_limit(profile: str) -> float:
    """One refresh period on the profile (§14.5 rule 1)."""
    return LIMITS["p99-laptop"] if profile in ("dev-laptop", "extra") \
        else LIMITS["p99-deck"]
def percentile(values: list[float], percent: float) -> float:
    """Nearest-rank percentile, matching frame_stats.c."""
    ordered = sorted(values)
    rank = math.ceil(percent * len(ordered) / 100.0)
    return ordered[min(max(rank, 1), len(ordered)) - 1]
def load_jsonl(path: Path) -> list[dict]:
    """Read records; malformed and blank lines are skipped with a warning."""
    try:
        text = path.read_text(encoding="utf-8")
    except OSError as exc:
        raise InputError(f"cannot read {path.name}: {exc.strerror}") from exc
    records = []
    for number, line in enumerate(text.splitlines(), 1):
        try:
            value = json.loads(line)
        except json.JSONDecodeError:
            if line.strip():
                print(f"analyse: {path.name}:{number}: skipped malformed line",
                      file=sys.stderr)
            continue
        if isinstance(value, dict):
            records.append(value)
    return records
def frame_number(record: dict) -> int:
    """Frame number, or -1 for a broken record."""
    try:
        return int(record.get("frame", -1))
    except (TypeError, ValueError):
        return -1
def exclusion_frames(records: list[dict], last_frame: int) -> set[int]:
    """Rule-5 windows: a scene point plus 30 frames, or a save/load pair."""
    excluded: set[int] = set()
    open_window: dict[str, int] = {}
    for record in records:
        if record.get("type") != "marker":
            continue
        kind, phase = record.get("kind"), record.get("phase")
        frame = frame_number(record)
        if frame < 0:
            continue
        if kind in ("scene", "transfer") and phase == "point":
            excluded.update(range(frame, frame + 31))
        elif kind in ("save", "load") and phase == "begin":
            open_window[kind] = frame
        elif kind in ("save", "load") and phase == "end":
            excluded.update(range(open_window.pop(kind, frame), frame + 1))
    for start in open_window.values():
        excluded.update(range(start, last_frame + 1))
    return excluded

def analyse_records(records: list[dict], profile: str, scene: str, warmup: int, scored: int, refresh_hz: float) -> dict:
    """Build one report; raises InputError when no scored frame remains."""
    gate_name(profile, True)  # reject an unknown profile before any work
    all_frames = sorted((r for r in records if r.get("type") == "frame"),
                        key=frame_number)
    end = warmup + scored if scored > 0 else None
    inside = [r for r in all_frames if frame_number(r) >= warmup and
              (end is None or frame_number(r) < end)]
    if not inside:
        raise InputError("no frames in the scored window")
    excluded = exclusion_frames(records, frame_number(inside[-1]))
    counted = [r for r in inside if frame_number(r) not in excluded]
    values = []
    for record in counted:
        try:
            value = float(record.get("work_ms"))
        except (TypeError, ValueError):
            continue
        if math.isfinite(value) and value >= 0:
            values.append(value)
    if not values:
        raise InputError("every scored frame was excluded")
    missed = sum(1 for record in counted
                 if float(record.get("present_interval_ms", 0.0)) >
                 1.5 * 1000.0 / refresh_hz)
    p99, p99_9, maximum = (percentile(values, 99.0),
                           percentile(values, 99.9), max(values))
    percent = 100.0 * missed / len(values)
    criteria = {
        "p99_work_ms": {"value": p99, "limit": p99_limit(profile),
                        "ok": p99 < p99_limit(profile)},
        "p99_9_work_ms": {"value": p99_9, "limit": LIMITS["p99.9"],
                          "ok": p99_9 < LIMITS["p99.9"]},
        "missed_vblank_percent": {"value": percent, "limit": LIMITS["missed"],
                                  "ok": percent <= LIMITS["missed"]},
        "max_work_ms": {"value": maximum, "limit": LIMITS["max"],
                        "ok": maximum <= LIMITS["max"]},
        "mean_work_ms": {"value": sum(values) / len(values), "limit": None,
                         "ok": True},
    }
    passed = all(item["ok"] for item in criteria.values())
    return {
        "schema": "smzuggler-perf-report/1", "profile": profile,
        "scene": scene, "gate": gate_name(profile, passed), "passed": passed,
        "warmup_frames": warmup, "scored_frames": scored,
        "refresh_hz": refresh_hz, "frames_total": len(all_frames),
        "frames_scored": len(inside),
        "frames_excluded": len(inside) - len(counted),
        "frames_counted": len(counted), "criteria": criteria,
        "summary": {
            "mean_work_ms": criteria["mean_work_ms"]["value"],
            "p50_work_ms": percentile(values, 50.0),
            "p95_work_ms": percentile(values, 95.0),
            "p99_work_ms": p99, "p99_9_work_ms": p99_9,
            "max_work_ms": maximum, "missed_vblanks": missed,
            "missed_vblank_percent": percent,
        },
    }
def combine_runs(reports: list[dict], profile: str, scene: str) -> dict:
    """Keep the worst value of each rule across runs (spec §14.4)."""
    combined = json.loads(json.dumps(reports[0]))
    for key in ("p99_work_ms", "p99_9_work_ms", "missed_vblank_percent",
                "max_work_ms", "mean_work_ms"):
        value = max(report["criteria"][key]["value"] for report in reports)
        item = combined["criteria"][key]
        item["value"] = value
        if item["limit"] is not None:
            item["ok"] = value < item["limit"] if key.startswith("p99") \
                else value <= item["limit"]
    combined["passed"] = all(i["ok"] for i in combined["criteria"].values())
    combined["gate"] = gate_name(profile, combined["passed"])
    combined["scene"] = scene
    combined["runs"] = reports
    return combined
def report_markdown(report: dict) -> str:
    """Short Markdown summary; profile names only."""
    lines = [f"# Perf report: {report['scene']} on {report['profile']}", "",
             f"Gate: **{report['gate']}**", "",
             "| Rule | Value | Limit | Result |", "| --- | ---: | ---: | --- |"]
    labels = {"p99_work_ms": "p99 work", "p99_9_work_ms": "p99.9 work",
              "missed_vblank_percent": "missed vblanks",
              "max_work_ms": "max frame over 50 ms"}
    for key, label in labels.items():
        item = report["criteria"][key]
        lines.append(f"| {label} | {item['value']:.3f} | "
                     f"{item['limit']:g} | {'ok' if item['ok'] else 'FAIL'} |")
    lines += ["", f"Scored frames counted: {report['frames_counted']} "
              f"(excluded {report['frames_excluded']}).", ""]
    return "\n".join(lines)
def infer_names(path: Path) -> tuple[str, str] | None:
    """Profile and scene from a harness filename."""
    match = SCENE_RE.search(path.name)
    return (match.group("profile"), match.group("scene")) if match else None

def group_inputs(paths: list[Path], scene: str | None, profile: str | None) -> dict[tuple[str, str], list[Path]]:
    """Group files by (profile, scene); the setup stream is dropped."""
    groups: dict[tuple[str, str], list[Path]] = {}
    for path in paths:
        if path.name == "setup.jsonl":
            continue
        names = infer_names(path)
        key = (profile or (names[0] if names else "dev-laptop"),
               scene or (names[1] if names else path.stem))
        groups.setdefault(key, []).append(path)
    return groups
def write_report(report: dict, out_dir: Path) -> None:
    """Write the JSON and Markdown report pair."""
    out_dir.mkdir(parents=True, exist_ok=True)
    stem = f"perf-report-{report['profile']}-{report['scene']}"
    (out_dir / f"{stem}.json").write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8")
    (out_dir / f"{stem}.md").write_text(report_markdown(report), encoding="utf-8")
def parse_args(argv: list[str] | None) -> argparse.Namespace:
    """CLI: JSON Lines files or a harness output directory."""
    parser = argparse.ArgumentParser(
        description="Analyse --frame-stats JSON Lines under spec §14.5.")
    parser.add_argument("--input", action="append", default=[],
                        help="frame-stats JSONL file (repeatable)")
    parser.add_argument("--dir", help="directory of frame-stats-*.jsonl files")
    parser.add_argument("--profile", choices=PROFILES)
    parser.add_argument("--scene")
    parser.add_argument("--out-dir", help="write perf-report-*.json/.md here")
    parser.add_argument("--warmup-frames", type=int, default=600)
    parser.add_argument("--scored-frames", type=int, default=7200,
                        help="0 means to the end")
    parser.add_argument("--refresh-hz", type=float, default=60.0)
    return parser.parse_args(argv)
def main(argv: list[str] | None = None) -> int:
    """Run the analyser; 1 on bad input, 2 on usage errors."""
    args = parse_args(argv)
    if args.refresh_hz <= 0:
        print("analyse: --refresh-hz must be positive", file=sys.stderr)
        return 2
    paths = [Path(item) for item in args.input]
    if args.dir:
        paths.extend(sorted(Path(args.dir).glob("frame-stats-*.jsonl")))
    if not paths:
        print("analyse: give --input FILE... or --dir DIR", file=sys.stderr)
        return 2
    try:
        for (profile, scene), files in sorted(
                group_inputs(paths, args.scene, args.profile).items()):
            reports = [analyse_records(load_jsonl(path), profile, scene,
                                      args.warmup_frames, args.scored_frames,
                                      args.refresh_hz) for path in files]
            report = reports[0] if len(reports) == 1 else combine_runs(
                reports, profile, scene)
            if args.out_dir:
                write_report(report, Path(args.out_dir))
            else:
                print(json.dumps(report, indent=2))
    except (InputError, ValueError) as exc:
        print(f"analyse: {exc}", file=sys.stderr)
        return 1
    return 0
if __name__ == "__main__":
    raise SystemExit(main())
