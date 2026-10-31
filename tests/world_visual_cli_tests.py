"""World visibility transport/replay uses generated data and no display or source images."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from headless_cli_tests import fixture

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from headless_client import Client


def main(runner, writer):
    env = os.environ.copy()
    env.pop("DISPLAY", None)
    env.pop("WAYLAND_DISPLAY", None)
    caps = json.loads(subprocess.check_output([runner, "--capabilities"], env=env, text=True))
    assert "world" in caps["visibility_backends"] and caps["visual"]
    with tempfile.TemporaryDirectory(prefix="blobber-world-cli-") as tmp:
        root = Path(tmp)
        scenario = fixture(root)
        subprocess.run([writer, str(root)], env=env, check=True)
        trace = root / "run.jsonl"
        # Client forwarding is exercised against the actual runner, not a command mock.
        old_display = {key: os.environ.pop(key, None) for key in ("DISPLAY", "WAYLAND_DISPLAY")}
        try:
            with Client(runner, root, "visual", trace, visibility="world") as client:
                start = client.request("start", scenario=scenario)
                assert start["ok"], start
                observed = client.request("observe")
                assert observed["ok"], observed
                movement = client.request("act", action={"kind": "move", "direction": "forward"})
                assert movement["ok"] and movement["data"]["accepted"], movement
                advance = client.request("advance", ticks=17)
                assert advance["ok"], advance
                reset = client.request("reset")
                assert reset["ok"], reset
                assert client.request("close")["ok"]
        finally:
            for key, value in old_display.items():
                if value is not None:
                    os.environ[key] = value
        header = json.loads(trace.read_text().splitlines()[0])["data"]
        assert header["visibility_backend"] == "world"
        assert header["visibility_version"] == "world-visibility-v2"
        assert "world_publication" in header["content"]
        command = [runner, "--assets", str(root), "--mode", "visual", "--visibility", "world"]
        result = subprocess.run(command + ["--replay", str(trace)], env=env, capture_output=True, text=True)
        assert result.returncode == 0, (result.stdout, result.stderr)
        assert json.loads(result.stdout)["replay"] == "matched"
        modified = root / "modified.jsonl"
        records = trace.read_text().splitlines()
        first = json.loads(records[0])
        first["data"]["visibility_version"] = "incompatible-world-visibility"
        records[0] = json.dumps(first)
        modified.write_text("\n".join(records) + "\n")
        result = subprocess.run(command + ["--replay", str(modified)], env=env, capture_output=True, text=True)
        assert result.returncode and "header" in result.stderr, result.stderr
        if not caps["capture"]:
            result = subprocess.run(command + ["--replay", str(trace), "--capture-dir", str(root / "captures"), "--capture-operation", "1"], env=env, capture_output=True, text=True)
            assert result.returncode and "graphics_unavailable" in result.stderr
            assert not (root / "captures").exists()
        # Publication errors cannot fall back to permissive diagnostic visibility.
        payload = root / "prepared/world/textures.bin"
        data = bytearray(payload.read_bytes())
        data[-1] ^= 1
        payload.write_bytes(data)
        result = subprocess.run(command, input="", env=env, capture_output=True, text=True)
        assert result.returncode and "rebuild" in result.stderr, result.stderr
    print("World visibility: display-free CLI/client, strict replay, backend identity and publication failures passed")


if __name__ == "__main__":
    main(*sys.argv[1:])
