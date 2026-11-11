#!/usr/bin/env python3
"""Small owned fixtures for the release checker; no game assets or desktop needed."""
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


audit = module('elf_audit', ROOT / 'tools/distribution/elf_audit.py')
driver = module('linux_release', ROOT / 'tools/linux_release.py')


class PolicyTests(unittest.TestCase):
    def setUp(self):
        self.policy = json.loads((ROOT / 'packaging/linux/runtime-policy.json').read_text())
        self.info = {'needed': [], 'interpreter': self.policy['interpreter'],
                     'versions': ['GLIBC_2.9'], 'imports': set(), 'exports': set()}

    def graph(self, infos, providers=None):
        paths = {'blobber': 'blobber'}
        paths.update({name: name for name in infos if name.startswith('bundled')})
        return audit.audit_graph(paths, self.policy, providers or {}, lambda path: infos[str(path)])

    def test_version_order_and_boundary(self):
        for version in ('GLIBC_2.2.5', 'GLIBC_2.9', 'GLIBC_2.10', 'GLIBC_2.35'):
            audit.check_versions({**self.info, 'versions': [version]}, self.policy)
        for version in ('GLIBC_2.36', 'GLIBC_2.100', 'GLIBC_PRIVATE', 'GLIBC_ABI_DT_RELR',
                        'GLIBCXX_3.4.1', 'CXXABI_1.3', 'GCC_3.0'):
            with self.subTest(version=version), self.assertRaises(audit.AuditError):
                audit.check_versions({**self.info, 'versions': [version]}, self.policy)

    def test_valid_recursive_provider_resolution(self):
        game = {**self.info, 'needed': ['libm.so.6'], 'imports': {'provided@GLIBC_2.35'}}
        first = {**self.info, 'interpreter': None, 'needed': ['libc.so.6']}
        last = {**self.info, 'exports': {'provided@GLIBC_2.35'}}
        _, host = self.graph({'blobber': game, 'm': first, 'c': last},
                             {'libm.so.6': 'm', 'libc.so.6': 'c'})
        self.assertEqual(set(host), {'libm.so.6', 'libc.so.6'})

    def test_missing_wrong_version_and_unexpected_dependencies(self):
        for dependency in ('libm.so.6', 'libsurprise.so', 'libyaml-cpp.so.0.9',
                           'libstdc++.so.6', 'libgcc_s.so.1'):
            with self.subTest(dependency=dependency), self.assertRaises(audit.AuditError):
                self.graph({'blobber': {**self.info, 'needed': [dependency]}})
        game = {**self.info, 'needed': ['libc.so.6'], 'imports': {'foo@GLIBC_2.35'}}
        provider = {**self.info, 'exports': {'foo@GLIBC_2.34'}}
        with self.assertRaisesRegex(audit.AuditError, 'Unresolved versioned'):
            self.graph({'blobber': game, 'c': provider}, {'libc.so.6': 'c'})

    def test_bundled_transitive_failure(self):
        game = {**self.info, 'needed': ['bundled.so']}
        for bad in ({**self.info, 'needed': ['libsurprise.so']},
                    {**self.info, 'versions': ['GLIBC_2.36']}):
            with self.assertRaises(audit.AuditError):
                self.graph({'blobber': game, 'bundled.so': bad})

    def test_host_cxx_runtime_is_allowed_for_host_graphics(self):
        game = {**self.info, 'needed': ['libGLU.so.1']}
        provider = {**self.info, 'needed': ['libstdc++.so.6'], 'versions': ['GLIBCXX_3.4']}
        runtime = {**self.info, 'needed': []}
        self.graph({'blobber': game, 'gl': provider, 'cxx': runtime},
                   {'libGLU.so.1': 'gl', 'libstdc++.so.6': 'cxx'})

    def test_required_dlopen_graph_is_checked(self):
        infos = {'blobber': self.info, 'glx': {**self.info, 'needed': ['libsurprise.so']}}
        with self.assertRaisesRegex(audit.AuditError, 'Unexpected host dependency'):
            audit.audit_graph({'blobber': 'blobber'}, self.policy, {'libGLX.so.0': 'glx'},
                              inspector=lambda path: infos[path], runtime_roots=['libGLX.so.0'])

    def test_interpreter(self):
        with self.assertRaisesRegex(audit.AuditError, 'interpreter'):
            self.graph({'blobber': {**self.info, 'interpreter': '/host/loader'}})

    def test_codegen_policy(self):
        good = ['cc', '-march=x86-64', '-mtune=generic', '-O3', '/src/test.cpp']
        audit.check_flags(good)
        for extra in ('-march=native', '-mtune=native', '-mavx2', '-msse4.2',
                      '-I/home/author/include', '-L/usr/local/lib'):
            with self.subTest(extra=extra), self.assertRaises(audit.AuditError):
                audit.check_flags(good + [extra])
        with self.assertRaises(audit.AuditError):
            audit.check_flags(['cc', '-O2'])

    def test_host_libc_loader_private_interface_is_not_a_shipped_requirement(self):
        game = {**self.info, 'needed': ['libc.so.6']}
        provider = {**self.info, 'versions': ['GLIBC_PRIVATE', 'GLIBC_2.35']}
        self.graph({'blobber': game, 'c': provider}, {'libc.so.6': 'c'})

    def test_stream_boundary_path_leakage(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'file'
            path.write_bytes(b'x' * (1024 * 1024 - 2) + b'/src/private.cpp')
            with self.assertRaisesRegex(audit.AuditError, 'Build-path leakage'):
                audit.check_build_paths(path)

    def test_recipe_pins_agree(self):
        driver.validate_recipe()

    def test_owned_build_rejects_identity_change_and_unknown_content(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            with driver.build_lock(root):
                driver.check_build_identity(root, {'image': 'first'})
                driver.check_build_identity(root, {'image': 'first'})
                with self.assertRaises(RuntimeError):
                    driver.check_build_identity(root, {'image': 'second'})
                with self.assertRaises(RuntimeError):
                    with driver.build_lock(root):
                        pass
            other = root / 'unowned'
            other.mkdir()
            (other / 'important.txt').write_text('keep')
            with self.assertRaises(RuntimeError):
                driver.check_build_identity(other, {})
            self.assertEqual((other / 'important.txt').read_text(), 'keep')

    def test_failed_audit_removes_previous_success(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            output = root / 'audit.json'
            output.write_text('{"status":"passed"}')
            with self.assertRaises((audit.AuditError, OSError)):
                audit.audit(root / 'absent', root, ROOT / 'packaging/linux/runtime-policy.json', output)
            self.assertFalse(output.exists())


@unittest.skipUnless(shutil.which(os.environ.get('CC', 'cc')) and shutil.which('readelf'),
                     'C compiler/readelf unavailable')
class RealElfTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        (self.root / 'provider.c').write_text('int provided(void) { return 7; }\n')
        (self.root / 'main.c').write_text('extern int provided(void); int main(void) { return provided(); }\n')
        (self.root / 'versions').write_text('GLIBC_2.36 { global: provided; local: *; };\n')
        self.cc = os.environ.get('CC', 'cc')
        self.compile('-nostdlib', '-shared', '-fPIC', 'provider.c', '-Wl,-soname,libfixture.so',
                     '-Wl,--version-script=versions', '-o', 'libfixture.so')
        self.compile('-nostdlib', 'main.c', '-L.', '-lfixture', '-Wl,-e,main', '-o', 'game')

    def compile(self, *args):
        subprocess.run([self.cc, *args], cwd=self.root, check=True, capture_output=True)

    def test_real_imports_and_version_definitions(self):
        consumer = audit.inspect(self.root / 'game')
        provider = audit.inspect(self.root / 'libfixture.so')
        self.assertEqual(consumer['needed'], ['libfixture.so'])
        self.assertIn('GLIBC_2.36', consumer['versions'])
        self.assertIn('provided@GLIBC_2.36', consumer['imports'])
        self.assertIn('provided@GLIBC_2.36', provider['exports'])
        self.assertEqual(provider['versions'], [])
        with self.assertRaisesRegex(audit.AuditError, 'baseline exceeded'):
            audit.check_versions(consumer, {'glibc_max': '2.35'})

    def test_reject_rpath_and_runpath(self):
        for mode in ('--enable-new-dtags', '--disable-new-dtags'):
            self.compile('-nostdlib', 'main.c', '-L.', '-lfixture', '-Wl,-e,main',
                         '-Wl,' + mode + ',-rpath,/unwanted', '-o', 'bad')
            with self.assertRaisesRegex(audit.AuditError, 'search path'):
                audit.inspect(self.root / 'bad')

    def test_host_search_path_is_recorded_without_shipping_it(self):
        self.compile('-nostdlib', '-shared', '-fPIC', 'provider.c',
                     '-Wl,-rpath,/usr/lib/host-audio', '-o', 'host.so')
        info = audit.inspect(self.root / 'host.so', shipped=False)
        self.assertEqual(info['search_paths'], ['/usr/lib/host-audio'])

    def test_reject_malformed_and_wrong_machine(self):
        path = self.root / 'bad'
        path.write_bytes(b'\x7fELFbad')
        with self.assertRaisesRegex(audit.AuditError, 'Malformed'):
            audit.inspect(path)
        data = bytearray((self.root / 'game').read_bytes())
        data[18:20] = (183).to_bytes(2, 'little')
        path.write_bytes(data)
        with self.assertRaisesRegex(audit.AuditError, 'x86-64'):
            audit.inspect(path)


if __name__ == '__main__':
    unittest.main()
