"""Managed incremental retirement preserves current world/UI and external assets."""
from pathlib import Path
import subprocess
import sys
import tempfile


def main(script):
    with tempfile.TemporaryDirectory(prefix="blobber retirement ") as temporary:
        base = Path(temporary)
        build = base / "managed build"
        build.mkdir()
        keep, retired = [], []
        for config in ("", "Debug", "Release", "RelWithDebInfo", "MinSizeRel", "Profile"):
            root = build / config / "assets"
            for name in ("preparation.json", "maze.txt", "rules/recovery.json", "prepared/scene.bin", "prepared/npcs.bin",
                         "prepared/loot.bin", "prepared/scene.bin.tmp-123",
                         "prepared/metadata.json", "prepared/metadata.json.tmp",
                         "prepared/derivatives/npcs/body.png"):
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b"obsolete")
                retired.append(path)
            for name in ("prepared/world/metadata.json", "prepared/world/textures.bin",
                         "prepared/creator.qoi", "prepared/portraits-small.qoi", "ui/menu.png"):
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b"current")
                keep.append((path, path.stat().st_mtime_ns))
        external = base / "external"
        external.mkdir()
        source = external / "scene.bin"
        source.write_bytes(b"external")
        command = ["cmake", f"-DMANAGED_BINARY_DIR={build}", "-DMANAGED_CONFIGS=Profile", "-P", str(script)]
        for _ in range(2):
            subprocess.run(command, check=True, capture_output=True, text=True)
            assert not any(path.exists() for path in retired)
            assert all(path.read_bytes() == b"current" and path.stat().st_mtime_ns == stamp
                       for path, stamp in keep)
            assert source.read_bytes() == b"external"
        # A link cannot turn a managed path into permission to clean another tree.
        linked = build / "Debug" / "assets"
        import shutil
        shutil.rmtree(linked)
        linked.symlink_to(external, target_is_directory=True)
        rejected = subprocess.run(command, capture_output=True, text=True)
        assert rejected.returncode and source.read_bytes() == b"external"
    print("Managed clean/incremental retirement, idempotence and ownership checks passed")


if __name__ == "__main__":
    main(Path(sys.argv[1]).resolve())
