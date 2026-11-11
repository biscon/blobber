#!/usr/bin/env python3
"""Linux release ELF and build-command audit. Python standard library + readelf."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys


class AuditError(RuntimeError):
    pass


def require(condition, message):
    if not condition:
        raise AuditError(message)


def digest(path):
    result = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            result.update(block)
    return result.hexdigest()


def command(args):
    return subprocess.check_output(args, text=True, env={**os.environ, 'LC_ALL': 'C'},
                                   stderr=subprocess.PIPE)


def numeric_version(value):
    require(bool(re.fullmatch(r'\d+(?:\.\d+)*', value)), 'Unknown ABI version: ' + value)
    return tuple(int(part) for part in value.split('.'))


def inspect(path, shipped=True):
    path = Path(path)
    with path.open('rb') as stream:
        header = stream.read(64)
    require(len(header) == 64 and header[:4] == b'\x7fELF', 'Malformed ELF: ' + str(path))
    require(header[4:7] == b'\x02\x01\x01' and int.from_bytes(header[18:20], 'little') == 62,
            'ELF must be ELF64 little-endian x86-64: ' + str(path))
    require(int.from_bytes(header[16:18], 'little') in (2, 3), 'Not an executable/shared ELF: ' + str(path))
    dynamic = command(['readelf', '--wide', '--dynamic', str(path)])
    program = command(['readelf', '--wide', '--program-headers', str(path)])
    versions = command(['readelf', '--wide', '--version-info', str(path)])
    symbols = command(['readelf', '--wide', '--dyn-syms', str(path)])
    notes = command(['readelf', '--wide', '--notes', str(path)])
    search_paths = re.findall(r'\((?:RPATH|RUNPATH)\).*?\[([^]]+)\]', dynamic)
    if shipped:
        require(not search_paths, 'Prohibited ELF search path: ' + str(path))
    needed = re.findall(r'\(NEEDED\).*?\[([^]]+)\]', dynamic)
    require(all('/' not in name for name in needed), 'Absolute/path dependency: ' + str(path))
    interpreter = re.search(r'Requesting program interpreter: ([^]]+)', program)
    # Version definitions describe exports, not requirements; inspect version needs only.
    needs = versions.split('Version needs section', 1)[-1] if 'Version needs section' in versions else ''
    required_versions = sorted(set(re.findall(r'\bName: (\S+)', needs)))
    imports, exports = set(), set()
    for line in symbols.splitlines():
        fields = line.split()
        if len(fields) < 8 or not re.fullmatch(r'\d+:', fields[0]):
            continue
        name = fields[7].replace('@@', '@')
        if fields[6] == 'UND':
            if '@' in name:
                imports.add(name)
        else:
            exports.add(name)
    for line in notes.splitlines():
        if shipped and 'x86 ISA needed:' in line:
            require(not re.search(r'x86-64-v[234]', line), 'Nonbaseline CPU property: ' + str(path))
    return {'needed': needed, 'interpreter': interpreter.group(1) if interpreter else None,
            'versions': required_versions, 'imports': imports, 'exports': exports,
            'search_paths': search_paths}


def check_versions(info, policy):
    for version in info['versions']:
        if version.startswith('GLIBC_'):
            require(numeric_version(version[6:]) <= numeric_version(policy['glibc_max']),
                    'GLIBC baseline exceeded: ' + version)
        require(not version.startswith(('GLIBCXX_', 'CXXABI_', 'GCC_')),
                'Shared compiler-runtime requirement: ' + version)


def library_paths():
    entries = {}
    for line in command(['/sbin/ldconfig', '-p']).splitlines():
        match = re.match(r'\s*(\S+) \([^)]*x86-64[^)]*\) => (\S+)', line)
        if match:
            entries.setdefault(match.group(1), Path(match.group(2)).resolve())
    return entries


def check_flags(arguments):
    require('-march=x86-64' in arguments and '-mtune=generic' in arguments,
            'Missing generic x86-64 compilation flags')
    for arg in arguments:
        if arg.startswith(('-march=', '-mtune=')):
            require(arg in ('-march=x86-64', '-mtune=generic'), 'Host-specific CPU flag: ' + arg)
        if arg.startswith('-m') and arg not in ('-march=x86-64', '-mtune=generic', '-m64'):
            require(arg.startswith('-mno-'), 'Unapproved CPU flag: ' + arg)
        require(not any(root in arg for root in ('/home/', '/usr/local/', 'cmake-build-debug',
                                                'cmake-build-release')), 'Host build path: ' + arg)


def audit_build(build):
    metadata = json.loads((build / 'baseline-build.json').read_text())
    require(metadata['configuration'] == 'Release' and metadata['baseline'] is True,
            'Not a baseline Release build')
    require(metadata['compiler_id'] == 'GNU' and metadata['compiler_version'].startswith('11.'),
            'Baseline requires GCC 11')
    require(metadata['raylib_type'] == metadata['yaml_type'] == 'STATIC_LIBRARY',
            'Pinned dependencies must be static')
    commands = json.loads((build / 'compile_commands.json').read_text())
    require(bool(commands), 'Empty compile commands')
    for item in commands:
        check_flags(item.get('arguments') or shlex.split(item['command']))
    # Use CMake's actual configured Ninja, through CMake, to obtain effective link commands.
    cache = (build / 'CMakeCache.txt').read_text()
    match = re.search(r'^CMAKE_COMMAND:INTERNAL=(.+)$', cache, re.M)
    require(match is not None, 'No configured CMake')
    lines = command([match.group(1), '--build', str(build), '--target', 'blobber',
                     '--', '-t', 'commands']).splitlines()
    links = [shlex.split(line) for line in lines if ' -o blobber ' in line]
    require(len(links) == 1, 'Cannot identify game link command')
    require(all(flag in links[0] for flag in ('-static-libstdc++', '-static-libgcc')),
            'Missing static compiler-runtime linkage')
    require(not any('/home/' in arg or '/usr/local/' in arg for arg in links[0]),
            'Host path in link command')
    return metadata


def audit_graph(shipped, policy, providers, inspector=inspect, host_inspector=None, runtime_roots=()):
    """Audit DT_NEEDED closure; providers is explicit and never comes from the caller's cwd."""
    require(not any(re.match(r'lib(?:stdc\+\+|gcc_s|raylib|yaml-cpp)(?:[.-]|$)', name)
                    for name in shipped), 'Static dependency shipped as a shared library')
    require(not (set(shipped) & set(policy['host_needed'])),
            'Host system library bundled in stage')
    infos = {name: inspector(path) for name, path in shipped.items()}
    host = {}
    require('blobber' in infos, 'Stage has no game ELF')
    require(infos['blobber']['interpreter'] == policy['interpreter'], 'Wrong game interpreter')
    for name, info in infos.items():
        check_versions(info, policy)
        if info['interpreter']:
            require(info['interpreter'] == policy['interpreter'], 'Wrong ELF interpreter: ' + name)
        for dependency in info['needed']:
            require(dependency in shipped or dependency in policy['shipped_needed'],
                    'Unexpected shipped dependency: ' + dependency)
    host_inspector = host_inspector or inspector
    visiting = set()

    def closure(name):
        if name in visiting:
            return set()
        visiting.add(name)
        if name in infos:
            info = infos[name]
        else:
            require(name in policy['host_needed'], 'Unexpected host dependency: ' + name)
            require(name in providers, 'Missing dependency: ' + name)
            info = host_inspector(providers[name])
            # Host libraries may legitimately use their own shared compiler runtimes.
            for version in info['versions']:
                if version.startswith('GLIBC_') and version != 'GLIBC_PRIVATE':
                    require(numeric_version(version[6:]) <= numeric_version(policy['glibc_max']),
                            'Host provider exceeds baseline: ' + name + ': ' + version)
            host[name] = info
        exports = set(info['exports'])
        for dependency in info['needed']:
            exports.update(closure(dependency))
        return exports

    for name in infos:
        visiting.clear()
        exports = closure(name)
        require(infos[name]['imports'] <= exports,
                'Unresolved versioned imports in ' + name + ': ' +
                ', '.join(sorted(infos[name]['imports'] - exports)))
    for name in runtime_roots:
        visiting.clear()
        closure(name)
    # Also validate versioned imports of every visited host provider against its closure.
    for name in list(host):
        visiting.clear()
        exports = closure(name)
        require(host[name]['imports'] <= exports, 'Unresolved provider imports: ' + name)
    return infos, host


def check_build_paths(path):
    # Keep overlap so a forbidden path spanning read boundaries cannot be missed.
    tail = b''
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            data = tail + block
            require(not any(token in data for token in (b'/src/', b'/build/', b'/home/')),
                    'Build-path leakage in ' + str(path))
            tail = data[-8:]


def audit(stage, build, policy_path, output):
    # Remove first: neither a failed scan nor a malformed request leaves an old success.
    output.unlink(missing_ok=True)
    require(not stage.is_symlink() and stage.is_dir(), 'Invalid stage directory')
    policy = json.loads(policy_path.read_text())
    require(policy['version'] == 1, 'Unknown runtime policy')
    metadata = audit_build(build)
    shipped = {}
    records = []
    for path in sorted(stage.rglob('*')):
        require(not path.is_symlink(), 'Symlink in stage: ' + str(path))
        if path.is_dir():
            continue
        require(path.is_file(), 'Special file in stage')
        with path.open('rb') as stream:
            is_elf = stream.read(4) == b'\x7fELF'
        if is_elf:
            require(path.name not in shipped, 'Ambiguous bundled ELF name: ' + path.name)
            shipped[path.name] = path
            # All deployed compiled strings must use stable logical prefixes.
            check_build_paths(path)
            records.append({'path': path.relative_to(stage).as_posix(), 'sha256': digest(path)})
    providers = library_paths()
    infos, host = audit_graph(shipped, policy, providers,
                              host_inspector=lambda path: inspect(path, shipped=False),
                              runtime_roots=[entry['library'] for entry in policy['runtime_loads']
                                             if entry['required']])
    runtime = []
    for entry in policy['runtime_loads']:
        provider = providers.get(entry['library'])
        require(provider is not None or not entry['required'], 'Missing runtime load: ' + entry['library'])
        info = inspect(provider, shipped=False) if provider is not None else None
        runtime.append({**entry, 'resolved': str(provider) if provider else None,
                        'search_paths': info['search_paths'] if info else []})
    inventory = stage.parent / (stage.name + '.inventory.json')
    stage_report = json.loads(inventory.read_text())
    # Bind the audit to the exact inventory as well as its stage_id.
    result = {'version': 1, 'status': 'passed', 'stage_id': stage_report['stage_id'],
              'inventory_sha256': digest(inventory), 'policy_sha256': digest(policy_path),
              'build': metadata, 'elfs': records,
              'required_glibc': sorted({v for info in infos.values() for v in info['versions']
                                        if v.startswith('GLIBC_')}, key=lambda v: numeric_version(v[6:])),
              'host_dependencies': {name: str(providers[name]) for name in sorted(host)},
              'runtime_loads': runtime}
    environment = build / 'baseline-environment.json'
    require(environment.is_file(), 'Missing controlled build environment identity')
    result['environment'] = json.loads(environment.read_text())
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_suffix('.candidate')
    temporary.write_text(json.dumps(result, indent=2, sort_keys=True) + '\n')
    temporary.replace(output)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--stage', type=Path, required=True)
    parser.add_argument('--policy', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    try:
        result = audit(args.stage, args.build, args.policy, args.output)
        print('Baseline audit passed:', len(result['elfs']), 'shipped ELFs;',
              len(result['host_dependencies']), 'host providers')
    except (AuditError, OSError, ValueError, KeyError, subprocess.SubprocessError) as error:
        print('Baseline audit failed:', error, file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
