#!/usr/bin/env python3
"""Small self-owned archive fixtures; no production assets or display."""
import importlib.util
import sys
import io
import json
from pathlib import Path
import tarfile
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('archive', Path(__file__).resolve().parents[1] / 'tools/distribution/archive.py')
a = importlib.util.module_from_spec(spec)
spec.loader.exec_module(a)


sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from distribution.verify_package import verification_record
from distribution import verify_package as verifier


class ArchiveTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.game = self.root / 'blobber'
        self.game.write_bytes(b'owned executable fixture')
        self.game.chmod(0o755)
        self.name = a.package_name('0.1.0', 'a' * 40, False, False)
        self.extra = {'BUILD.json': b'{}\n', 'README.txt': b'Run ./blobber\n'}

    def archive(self, name='fixture.tar.gz'):
        path = self.root / name
        a.write_archive(path, self.name, {'blobber': self.game}, self.extra, 123456)
        return path

    def test_determinism_hashes_and_permissions(self):
        one, two = self.archive('one.tar.gz'), self.archive('two.tar.gz')
        self.assertEqual(one.read_bytes(), two.read_bytes())
        target = self.root / 'extract'
        result = a.inspect_archive(one, target)
        self.assertEqual(result['files'], 4)
        game = target / self.name / 'blobber'
        self.assertEqual(game.read_bytes(), self.game.read_bytes())
        self.assertEqual(game.stat().st_mode & 0o777, 0o755)
        self.assertEqual((game.parent / 'README.txt').stat().st_mode & 0o777, 0o644)

    def test_naming_dirty_and_unsafe_version(self):
        self.assertEqual(self.name, 'blobber-0.1.0-linux-x86_64')
        self.assertEqual(self.name, a.package_name('0.1.0', 'b' * 40, False, False))
        self.assertIn('-dirty-', a.package_name('0.1.0', 'b' * 40, True, True))
        with self.assertRaisesRegex(ValueError, 'Dirty'):
            a.package_name('0.1.0', 'b' * 40, True, False)
        for version in ('../escape', '0.1.0/name', ''):
            with self.assertRaises(ValueError):
                a.package_name(version, 'b' * 40, False, False)

    def malformed(self, members):
        path = self.root / 'bad.tar.gz'
        with tarfile.open(path, 'w:gz') as out:
            for name, kind, mode in members:
                entry = tarfile.TarInfo(name)
                entry.type, entry.mode = kind, mode
                if kind == tarfile.SYMTYPE:
                    entry.linkname = '/outside'
                out.addfile(entry, io.BytesIO())
        target = self.root / 'rejected'
        with self.assertRaises((ValueError, tarfile.TarError)):
            a.inspect_archive(path, target)
        self.assertFalse(target.exists())

    def test_unsafe_members_and_duplicates(self):
        for member in ('../escape', '/absolute', 'game/../escape', 'game//file'):
            with self.subTest(member=member):
                self.malformed([(member, tarfile.REGTYPE, 0o644)])
        for kind in (tarfile.SYMTYPE, tarfile.LNKTYPE, tarfile.FIFOTYPE, tarfile.CHRTYPE):
            self.malformed([('game/special', kind, 0o644)])
        self.malformed([('game/a', tarfile.REGTYPE, 0o644)] * 2)
        self.malformed([('one/a', tarfile.REGTYPE, 0o644), ('two/a', tarfile.REGTYPE, 0o644)])
        self.malformed([('game/blobber', tarfile.REGTYPE, 0o644)])

    def test_corruption_and_manifest_mismatch(self):
        path = self.archive()
        # Rebuild a structurally valid tar with altered payload and the old manifest.
        bad = self.root / 'corrupt.tar.gz'
        with tarfile.open(path, 'r:gz') as source, tarfile.open(bad, 'w:gz') as out:
            for entry in source:
                data = source.extractfile(entry).read() if entry.isfile() else None
                if entry.name.endswith('/blobber'):
                    data = b'x' * entry.size
                out.addfile(entry, io.BytesIO(data) if data is not None else None)
        with self.assertRaisesRegex(ValueError, 'hash mismatch'):
            a.inspect_archive(bad)

    def test_stale_audit_unexpected_files_and_symlinks(self):
        stage = self.root / 'stage'
        stage.mkdir()
        game = stage / 'blobber'
        game.write_bytes(b'fixture')
        game.chmod(0o755)
        inventory = {'complete': True, 'configuration': 'Release', 'stage_id': 'current',
                     'files': [{'path': 'blobber', 'bytes': 7}]}
        audit = {'status': 'passed', 'stage_id': 'current', 'elfs': [{'path': 'blobber', 'sha256': a.digest(game)}]}
        self.assertEqual(set(a.stage_inputs(stage, inventory, audit)), {'blobber'})
        with self.assertRaisesRegex(ValueError, 'Stale'):
            a.stage_inputs(stage, inventory, {**audit, 'stage_id': 'old'})
        game.write_bytes(b'changed')
        with self.assertRaisesRegex(ValueError, 'Stale ELF'):
            a.stage_inputs(stage, inventory, audit)
        (stage / 'extra').write_text('unexpected')
        with self.assertRaisesRegex(ValueError, 'Unexpected'):
            a.stage_inputs(stage, inventory, audit)
        (stage / 'extra').unlink()
        (stage / 'link').symlink_to(game)
        with self.assertRaisesRegex(ValueError, 'Unsafe'):
            a.stage_inputs(stage, inventory, audit)

    def test_promotion_failure_restores_previous(self):
        final, candidate = self.root / 'bundle', self.root / 'candidate'
        final.mkdir()
        (final / 'old').write_text('last success')
        candidate.mkdir()
        original = Path.rename
        def fail(path, target):
            if path == candidate:
                raise OSError('injected promotion failure')
            return original(path, target)
        with patch.object(Path, 'rename', fail), self.assertRaises(OSError):
            a.promote(candidate, final)
        self.assertEqual((final / 'old').read_text(), 'last success')
        self.assertTrue(candidate.is_dir())

    def test_invalid_bundle_does_not_create_unowned_lock(self):
        with self.assertRaises(ValueError):
            verifier.verify_package(self.root, self.root / 'outside/dist/package')
        self.assertFalse((self.root / 'outside/.baseline.lock').exists())

    def test_bundle_symlink_cannot_remove_another_result(self):
        build = self.root / 'build-linux-baseline'
        dist = build / 'dist'
        dist.mkdir(parents=True)
        (dist / '.blobber-dist-owner.json').write_text('{"version":1,"build":"/build"}')
        (build / '.baseline-owner.json').write_text('{"baseline":"ubuntu22.04-amd64"}')
        unrelated = self.root / 'unrelated'
        unrelated.mkdir()
        (unrelated / 'verification.json').write_text('preserve')
        (dist / 'package').symlink_to(unrelated, target_is_directory=True)
        with patch.object(verifier, 'ROOT', self.root), self.assertRaises(ValueError):
            verifier.verify_package(self.root, dist / 'package')
        self.assertEqual((unrelated / 'verification.json').read_text(), 'preserve')
        self.assertFalse((build / '.baseline.lock').exists())

    def test_verification_requires_both_complete_runtimes(self):
        phases = {p: {'status': 'passed'} for p in ('create', 'continue')}
        result = {'status': 'passed', 'phases': phases}
        with self.assertRaises(ValueError):
            verification_record('a' * 64, {'ubuntu22': result})
        with self.assertRaises(ValueError):
            verification_record('a' * 64, {'ubuntu22': result, 'debian13': {'status': 'failed'}})
        record = verification_record('b' * 64, {'ubuntu22': result, 'debian13': result})
        self.assertEqual(record['archive_sha256'], 'b' * 64)

    def test_bundle_checksum_and_root_binding(self):
        bundle = self.root / self.name
        bundle.mkdir()
        path = self.archive()
        target = bundle / (self.name + '.tar.gz')
        path.rename(target)
        checksum = bundle / (target.name + '.sha256')
        checksum.write_text(a.digest(target) + '  ' + target.name + '\n')
        self.assertEqual(a.validate_bundle(bundle)[1], a.digest(target))
        checksum.write_text('0' * 64 + '  ' + target.name + '\n')
        with self.assertRaisesRegex(ValueError, 'checksum'):
            a.validate_bundle(bundle)


if __name__ == '__main__':
    unittest.main()
