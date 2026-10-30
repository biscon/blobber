"""Bounded local outputs and explicit artifact access; no database or service."""
import base64
import os
from pathlib import Path
import stat
import threading
import uuid

from arena_experiments.config import MIB, read_json


class AdapterError(RuntimeError):
    def __init__(self, code, message, **details):
        self.code, self.details = code, details
        super().__init__(message)

    def result(self):
        return dict(code=self.code, message=str(self), **self.details)


def integer(value, low, high, name):
    if type(value) is not int or not low <= value <= high:
        raise AdapterError("invalid_input", f"{name} must be an integer in {low}..{high}")
    return value


def input_file(path, maximum):
    p = Path(path).resolve(strict=True)
    if not p.is_file() or p.stat().st_size > maximum:
        raise AdapterError("invalid_input", f"Expected a regular file of at most {maximum} bytes")
    return p


def size(root):
    return sum(p.stat().st_size for p in root.rglob('*') if p.is_file() and not p.is_symlink()) if root.exists() else 0


class Storage:
    def __init__(self, root, capacity=8192*MIB):
        self.root = Path(root).resolve()
        self.capacity = capacity
        self.lock = threading.RLock()
        self.reservations = {}
        self.read_roots = set()

    def reserve(self, kind, maximum):
        with self.lock:
            self.root.mkdir(parents=True, exist_ok=True)
            used = size(self.root)
            reserved = sum(max(0, n-size(p)) for p, n in self.reservations.items())
            if used+reserved+maximum > self.capacity:
                raise AdapterError("output_capacity", "Output budget cannot reserve this operation",
                                   used=used, reserved=reserved, requested=maximum, capacity=self.capacity)
            import shutil
            if shutil.disk_usage(self.root).free < maximum:
                raise AdapterError("output_capacity", "Insufficient free disk space for reservation")
            path = self.root/(kind+'-'+uuid.uuid4().hex)
            path.mkdir()
            self.reservations[path] = maximum
            return path

    def release(self, path):
        with self.lock:
            self.reservations.pop(path, None)

    def register(self, root):
        p = Path(root).resolve(strict=True)
        if not p.is_dir():
            raise AdapterError("invalid_input", "Run directory required")
        with self.lock:
            if not p.is_relative_to(self.root) and p not in self.read_roots:
                if len(self.read_roots) >= 64:
                    raise AdapterError("artifact_capacity", "At most 64 external run roots per server")
                self.read_roots.add(p)
        return p

    def read(self, path, offset=0, limit=65536, image=False):
        integer(offset, 0, 2**63-1, 'offset')
        integer(limit, 1, MIB, 'limit')
        supplied = Path(path)
        if '..' in supplied.parts:
            raise AdapterError("artifact_path", "Parent traversal is not allowed")
        p = supplied.resolve(strict=True)
        with self.lock:
            roots = (self.root, *self.read_roots)
        if not any(p.is_relative_to(root) for root in roots):
            raise AdapterError("artifact_path", "Artifact is outside registered roots")
        # No FIFO/device reads; O_NOFOLLOW rejects a final-component symlink race.
        fd = os.open(p, os.O_RDONLY | os.O_NONBLOCK | os.O_NOFOLLOW)
        with os.fdopen(fd, 'rb') as f:
            info = os.fstat(f.fileno())
            if not stat.S_ISREG(info.st_mode):
                raise AdapterError("artifact_path", "Regular files only")
            if image:
                if offset or p.suffix.lower() != '.png' or info.st_size > 16*MIB:
                    raise AdapterError("artifact_limit", "Image reads require a PNG of at most 16 MiB and offset zero")
                data = f.read(16*MIB+1)
                if len(data) > 16*MIB or not data.startswith(b'\x89PNG\r\n\x1a\n'):
                    raise AdapterError("artifact_format", "Invalid or oversized PNG")
                return dict(path=str(p), mime_type='image/png', base64=base64.b64encode(data).decode())
            f.seek(offset)
            raw = f.read(limit)
        return dict(path=str(p), offset=offset, next_offset=offset+len(raw), size=info.st_size,
                    eof=offset+len(raw) >= info.st_size, encoding='utf-8', text=raw.decode('utf-8', errors='replace'))


def artifact_list(root, limit=128):
    # Expose the small top-level report set, not all trial files on every poll.
    return [dict(name=p.name, path=str(p), bytes=p.stat().st_size)
            for p in sorted(root.iterdir()) if p.is_file() and not p.is_symlink()][:limit] if root.exists() else []


def json_file(root, name, maximum=4*MIB):
    p = (root/name).resolve(strict=True)
    if not p.is_relative_to(root.resolve()):
        raise AdapterError('artifact_path', 'Artifact escapes run root')
    return read_json(input_file(p, maximum), maximum)
