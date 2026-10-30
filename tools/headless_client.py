#!/usr/bin/env python3
"""Synchronous, bounded NDJSON transport. No gameplay policy or external packages."""
import argparse
import json
import os
from pathlib import Path
import selectors
import subprocess
import sys
import time


class ClientError(RuntimeError):
    """Transport failure, separate from a well-formed gameplay rejection."""


class DeadlineExceeded(ClientError):
    pass


class Cancelled(ClientError):
    pass


class ProtocolError(ClientError):
    pass


class RunnerError(ClientError):
    pass


class Client:
    MAX_LINE = 4 * 1024 * 1024
    MAX_REQUEST = 2 * 1024 * 1024
    STDERR_TAIL = 1024 * 1024

    def __init__(self, executable, assets, mode="diagnostic", record=None, on_event=None,
                 *, timeout=10, startup_timeout=30, deadline=None, cancelled=None, campaign_save=None):
        command = [str(executable), "--assets", str(assets), "--mode", mode]
        if campaign_save is not None:
            command += ["--campaign-save", str(campaign_save)]
        if record is not None:
            command += ["--record", str(record)]
        self.process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE, bufsize=0)
        self.selector = selectors.DefaultSelector()
        for stream, name in ((self.process.stdout, "stdout"), (self.process.stderr, "stderr")):
            os.set_blocking(stream.fileno(), False)
            self.selector.register(stream, selectors.EVENT_READ, name)
        os.set_blocking(self.process.stdin.fileno(), False)
        self.sequence = 0
        self.identity = None
        self.on_event = on_event
        self.closed = self.failed = self.disposed = False
        self.timeout, self.startup_timeout = timeout, startup_timeout
        self.deadline, self.cancelled = deadline, cancelled
        self.buffer, self.stderr = bytearray(), bytearray()
        self.stdout_eof = False
        self.request_seconds = 0.0
        self.bytes_read = self.bytes_written = 0
        self.peak_rss_kib = None

    def _check(self, end):
        if self.cancelled and self.cancelled():
            raise Cancelled("cancelled")
        if time.monotonic() >= end:
            raise DeadlineExceeded("request_or_trial_wall_limit")

    def _pump(self, end):
        self._check(end)
        for key, _ in self.selector.select(min(0.05, max(0, end - time.monotonic()))):
            if key.data == "stdin":
                continue
            data = os.read(key.fd, 65536)
            if not data:
                self.selector.unregister(key.fileobj)
                if key.data == "stdout":
                    self.stdout_eof = True
                continue
            if key.data == "stderr":
                self.stderr.extend(data)
                del self.stderr[:-self.STDERR_TAIL]
            else:
                self.bytes_read += len(data)
                self.buffer.extend(data)
                if len(self.buffer) > self.MAX_LINE + 65536:
                    raise ProtocolError("response_buffer_limit")
                # At most one read chunk beyond a line bound; no unbounded partial lines.
                first = self.buffer.find(b"\n")
                if (first < 0 and len(self.buffer) > self.MAX_LINE) or first > self.MAX_LINE:
                    raise ProtocolError("response_line_limit")

    def _line(self, end):
        while True:
            self._check(end)
            split = self.buffer.find(b"\n")
            if split >= 0:
                if split > self.MAX_LINE:
                    raise ProtocolError("response_line_limit")
                line = bytes(self.buffer[:split])
                del self.buffer[:split + 1]
                return line
            if self.stdout_eof:
                raise RunnerError("Runner stopped before responding: " + self.error_tail())
            self._pump(end)

    def error_tail(self):
        return self.stderr.decode("utf-8", errors="replace")

    def _memory(self):
        # Linux process-local high water mark, not cumulative RUSAGE_CHILDREN.
        try:
            for line in Path(f"/proc/{self.process.pid}/status").read_text().splitlines():
                if line.startswith("VmHWM:"):
                    value = int(line.split()[1])
                    self.peak_rss_kib = max(self.peak_rss_kib or 0, value)
        except (OSError, ValueError):
            pass

    def request(self, op, *, _timeout=None, **fields):
        if self.closed or self.disposed or self.failed:
            raise ClientError("Client is closed or failed")
        started = time.monotonic()
        duration = _timeout if _timeout is not None else (
            self.startup_timeout if self.sequence == 0 else self.timeout)
        end = min(started + duration, self.deadline if self.deadline is not None else float("inf"))
        request = dict(fields, version=1, id=str(self.sequence), op=op)
        self.sequence += 1
        if self.identity and op in ("start", "reset", "advance", "act", "preview", "preview_save", "save", "load", "retry_terminal"):
            request.update(self.identity)
        payload = (json.dumps(request, allow_nan=False, separators=(",", ":")) + "\n").encode()
        if len(payload) - 1 > self.MAX_REQUEST:
            raise ProtocolError("request_byte_limit")
        try:
            self.selector.register(self.process.stdin, selectors.EVENT_WRITE, "stdin")
            offset = 0
            try:
                while offset < len(payload):
                    self._check(end)
                    try:
                        count = os.write(self.process.stdin.fileno(), payload[offset:offset + 65536])
                        offset += count
                        self.bytes_written += count
                    except BlockingIOError:
                        self._pump(end)
            finally:
                self.selector.unregister(self.process.stdin)
            while True:
                try:
                    response = json.loads(self._line(end))
                except (ValueError, UnicodeError) as exc:
                    raise ProtocolError("Invalid response JSON") from exc
                if not isinstance(response, dict) or type(response.get("version")) is not int or response["version"] != 1:
                    raise ProtocolError("Response version/type mismatch")
                kind = response.get("type")
                if kind == "fatal":
                    raise RunnerError(str(response.get("error", "Runner fatal error")))
                if response.get("id") != request["id"]:
                    raise ProtocolError("Response ID mismatch")
                if kind == "events":
                    events = response.get("events")
                    if not isinstance(events, list) or len(events) > 256 or any(
                            not isinstance(e, dict) or not isinstance(e.get("event"), str) for e in events):
                        raise ProtocolError("Invalid event chunk")
                    if self.on_event:
                        self.on_event(response)
                    continue
                if kind != "response" or type(response.get("ok")) is not bool:
                    raise ProtocolError("Invalid response envelope")
                if response["ok"]:
                    data = response.get("data")
                    if not isinstance(data, dict):
                        raise ProtocolError("Invalid response data")
                    observation = data.get("observation", data)
                    if not isinstance(observation, dict):
                        raise ProtocolError("Invalid observation")
                    if "session" in observation or "revision" in observation:
                        tokens = {k: observation.get(k) for k in ("session", "revision")}
                        if any(not isinstance(v, str) or not v.isascii() or not v.isdecimal()
                               for v in tokens.values()):
                            raise ProtocolError("Invalid identity tokens")
                        self.identity = tokens
                    if op == "close":
                        self.closed = True
                elif not isinstance(response.get("error"), dict):
                    raise ProtocolError("Missing protocol error")
                self._memory()
                return response
        except (OSError, ClientError):
            self.failed = True
            raise
        finally:
            self.request_seconds += time.monotonic() - started

    def dispose(self):
        """Always reap our child; never signal another process/window."""
        if self.disposed:
            return
        self.disposed = True
        try:
            self.process.stdin.close()
            end = time.monotonic() + 0.5
            while self.process.poll() is None and time.monotonic() < end:
                try:
                    self._pump(end)
                except (ClientError, OSError):
                    break
            if self.process.poll() is None:
                self.process.terminate()
                try:
                    self.process.wait(timeout=0.5)
                except subprocess.TimeoutExpired:
                    self.process.kill()
                    self.process.wait(timeout=1)
        finally:
            self.selector.close()
            self.process.stdout.close()
            self.process.stderr.close()

    def __enter__(self):
        return self

    def __exit__(self, kind, value, traceback):
        try:
            if kind is None and not self.closed and not self.failed:
                self.request("close")
        finally:
            self.dispose()
        if kind is None and self.process.returncode:
            raise RunnerError(f"Runner exited with status {self.process.returncode}: {self.error_tail()}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runner", default="cmake-build-headless-debug/blobber_headless")
    parser.add_argument("--assets", default="assets")
    parser.add_argument("--mode", choices=("diagnostic", "visual"), default="diagnostic")
    parser.add_argument("--scenario", required=True, type=Path, help="Arena, generated_floor or campaign scenario JSON")
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
