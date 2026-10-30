#!/usr/bin/env python3
"""Small synchronous NDJSON client; no gameplay policy or external packages."""
import argparse
import json
from pathlib import Path
import subprocess
import sys


class Client:
    def __init__(self, executable, assets, mode="diagnostic", record=None, on_event=None):
        command = [str(executable), "--assets", str(assets), "--mode", mode]
        if record is not None:
            command += ["--record", str(record)]
        self.process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                        text=True)  # Runner diagnostics inherit stderr.
        self.sequence = 0
        self.identity = None
        self.on_event = on_event
        self.closed = False

    def request(self, op, **fields):
        request = dict(fields, version=1, id=str(self.sequence), op=op)
        self.sequence += 1
        if self.identity and op in ("start", "reset", "advance", "act", "preview"):
            request.update(self.identity)
        self.process.stdin.write(json.dumps(request) + "\n")
        self.process.stdin.flush()
        while True:
            line = self.process.stdout.readline()
            if not line:
                raise RuntimeError("Runner stopped before responding")
            response = json.loads(line)
            if response["type"] == "fatal":
                raise RuntimeError(response["error"])
            if response["type"] == "events":
                if self.on_event:
                    self.on_event(response)
                continue
            if response.get("id") != request["id"]:
                raise RuntimeError("Response ID mismatch")
            data = response.get("data", {})
            observation = data.get("observation", data)
            if "session" in observation and "revision" in observation:
                self.identity = {key: observation[key] for key in ("session", "revision")}
            if op == "close" and response["ok"]:
                self.closed = True
            return response

    def __enter__(self):
        return self

    def __exit__(self, kind, value, traceback):
        try:
            if kind is None and not self.closed:
                self.request("close")
        finally:
            self.process.stdin.close()
            self.process.stdout.close()
            try:
                code = self.process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()
                if kind is None:
                    raise RuntimeError("Runner did not shut down")
            else:
                if kind is None and code:
                    raise RuntimeError(f"Runner exited with status {code}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runner", default="cmake-build-headless-debug/blobber_headless")
    parser.add_argument("--assets", default="assets")
    parser.add_argument("--mode", choices=("diagnostic", "visual"), default="diagnostic")
    parser.add_argument("--scenario", required=True, type=Path)
    parser.add_argument("--record", type=Path)
    parser.add_argument("--script", type=Path, help="JSON lines with op/fields; default stdin")
    args = parser.parse_args()
    def show(message):
        print(json.dumps(message), flush=True)
    with Client(args.runner, args.assets, args.mode, args.record, show) as client:
        response = client.request("start", scenario=json.loads(args.scenario.read_text()))
        show(response)
        if not response["ok"]:
            raise RuntimeError("Scenario rejected")
        stream = args.script.open() if args.script else sys.stdin
        try:
            for line in stream:
                if not line.strip():
                    continue
                command = json.loads(line)
                show(client.request(command.pop("op"), **command))
                if client.closed:
                    break
        finally:
            if args.script:
                stream.close()


if __name__ == "__main__":
    main()
