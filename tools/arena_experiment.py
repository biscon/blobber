#!/usr/bin/env python3
"""Opt-in arena playtesting; never run automatically for unrelated code changes."""
import argparse
import json
from pathlib import Path
import signal
import sys

from arena_experiments import config
from arena_experiments.batch import preflight, run_batch, rerun
from arena_experiments.report import write_reports


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    for name in ("validate", "run"):
        p = sub.add_parser(name)
        p.add_argument("manifest", type=Path)
        p.add_argument("--runner", default="cmake-build-headless-release/blobber_headless", type=Path)
        p.add_argument("--assets", default="assets", type=Path)
        if name == "run": p.add_argument("--output", required=True, type=Path)
        else: p.add_argument("--structure-only", action="store_true")
    p = sub.add_parser("report"); p.add_argument("directory", type=Path)
    p = sub.add_parser("rerun"); p.add_argument("directory", type=Path)
    p.add_argument("--trial", required=True); p.add_argument("--output", required=True, type=Path)
    p.add_argument("--runner", type=Path); p.add_argument("--assets", type=Path)
    args = parser.parse_args(argv)
    interrupted = False
    def cancel(_signum, _frame):
        nonlocal interrupted
        interrupted = True
    old = {sig: signal.signal(sig, cancel) for sig in (signal.SIGINT, signal.SIGTERM)}
    try:
        if args.command in ("validate", "run"):
            manifest = config.load(args.manifest)
            if args.command == "validate":
                result = dict(ok=True, scope="structure_only_gameplay_unchecked") if args.structure_only else preflight(
                    manifest, args.runner, args.assets, cancelled=lambda: interrupted)
                print(json.dumps(result, indent=2))
                return 0 if result["ok"] else 1
            result = run_batch(manifest, args.runner, args.assets, args.output, cancelled=lambda: interrupted)
        elif args.command == "report":
            result = write_reports(args.directory)
        else:
            result = rerun(args.directory, args.trial, args.output, runner=args.runner, assets=args.assets,
                           cancelled=lambda: interrupted)
        print(json.dumps({k:result[k] for k in ("planned", "started", "unstarted", "counts", "batch", "rerun") if k in result}, indent=2))
        return 0 if result["batch"]["status"] == "complete" and result.get("rerun", {}).get("matched", True) else 1
    except Exception as exc:
        print(f"Arena experiment: {type(exc).__name__}: {exc}", file=sys.stderr)
        return 1
    finally:
        for sig, handler in old.items(): signal.signal(sig, handler)


if __name__ == "__main__":
    raise SystemExit(main())
