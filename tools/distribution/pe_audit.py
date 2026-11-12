#!/usr/bin/env python3
"""Bounded PE32+ AMD64 import audit, independent of developer PATH resolution."""
import struct
from pathlib import Path
from archive import digest, require

SYSTEM = {'kernel32.dll', 'user32.dll', 'gdi32.dll', 'shell32.dll', 'winmm.dll',
          'ole32.dll', 'oleaut32.dll', 'advapi32.dll', 'comdlg32.dll', 'shlwapi.dll',
          'bcrypt.dll', 'opengl32.dll', 'ntdll.dll', 'version.dll', 'imm32.dll',
          'setupapi.dll', 'dwmapi.dll', 'ws2_32.dll', 'cfgmgr32.dll'}


def inspect(path):
    data = Path(path).read_bytes()
    require(64 <= len(data) <= 128 * 1024**2 and data[:2] == b'MZ', 'Invalid/oversized PE')
    def unpack(fmt, offset):
        require(0 <= offset <= len(data) - struct.calcsize(fmt), 'Truncated PE')
        return struct.unpack_from(fmt, data, offset)
    pe, = unpack('<I', 60)
    require(data[pe:pe+4] == b'PE\0\0', 'Invalid PE signature')
    machine, count = unpack('<HH', pe+4)
    optional_size, = unpack('<H', pe+20)
    opt = pe+24
    require(machine == 0x8664 and unpack('<H', opt)[0] == 0x20b, 'PE must be AMD64 PE32+')
    require(0 < count <= 96 and optional_size >= 240, 'Invalid PE headers')
    subsystem, = unpack('<H', opt+68)
    require(subsystem == 2, 'Player PE must use the Windows GUI subsystem (no console)')
    image_base, = unpack('<Q', opt+24)
    directories, = unpack('<I', opt+108)
    sections = []
    for i in range(count):
        size, va, raw_size, raw = unpack('<IIII', opt+optional_size+40*i+8)
        sections.append((va, max(size, raw_size), raw, raw_size))
    def rva(value, size=1):
        for va, length, raw, raw_size in sections:
            if va <= value and value-va+size <= min(length, raw_size):
                offset = raw+value-va
                require(offset+size <= len(data), 'PE section outside file')
                return offset
        raise ValueError('Unmapped PE RVA')
    def dll(value):
        at = rva(value)
        end = data.find(b'\0', at, min(at+256, len(data)))
        require(end > at, 'Invalid PE DLL name')
        name = data[at:end].decode('ascii').lower()
        require('/' not in name and '\\' not in name and ':' not in name, 'Invalid import path')
        return name
    imports = set()
    for directory, size in [(1, 20), (13, 32)]:
        if directories <= directory: continue
        address, length = unpack('<II', opt+112+8*directory)
        if not address: continue
        require(size <= length <= 1024**2, 'Invalid import table size')
        terminated = False
        for i in range(min(length//size, 4096)):
            fields = unpack('<'+'I'*(size//4), rva(address+i*size, size))
            if not any(fields): terminated = True; break
            name = fields[3] if directory == 1 else fields[1]
            if directory == 13 and not fields[0] & 1: name -= image_base
            imports.add(dll(name))
        require(terminated, 'Unterminated import table')
    for name in imports:
        require(name in SYSTEM or name.startswith('api-ms-win-core-'), 'Undeclared Windows dependency: '+name)
    return {'path': Path(path).name, 'sha256': digest(path), 'machine': 'AMD64',
            'subsystem': 'Windows GUI', 'imports': sorted(imports)}


def audit(stage):
    paths = sorted(p for p in Path(stage).rglob('*') if p.suffix.lower() in ('.exe', '.dll'))
    require([p.relative_to(stage).as_posix() for p in paths] == ['blobber.exe'],
            'Unexpected executable/DLL in player payload')
    return [inspect(p) for p in paths]
