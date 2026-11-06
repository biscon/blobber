"""Safety and lifecycle checks; no game assets, windows or builds required."""
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest

spec = importlib.util.spec_from_file_location(
    'agent_artifacts', Path(__file__).resolve().parents[1] / 'tools/agent_artifacts.py')
artifacts = importlib.util.module_from_spec(spec)
spec.loader.exec_module(artifacts)


class ArtifactTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.project = Path(self.temp.name)
        subprocess.run(['git', 'init', '-q', str(self.project)], check=True)

    def test_preview_delete_and_repeat_preserve_neighbor(self):
        run = artifacts.create(self.project)
        other = artifacts.create(self.project)
        (run / 'nested').mkdir()
        (run / 'nested' / 'capture.png').write_bytes(b'fixture')
        preview = artifacts.clean(self.project, run)
        self.assertEqual(preview['state'], 'preview')
        self.assertEqual(preview['files'], 2)
        self.assertTrue((run / 'nested' / 'capture.png').exists())
        self.assertEqual(artifacts.clean(self.project, run, True)['state'], 'removed')
        self.assertFalse(run.exists())
        self.assertTrue((other / artifacts.MARKER).exists())
        self.assertEqual(artifacts.clean(self.project, run, True)['state'], 'absent')

    def test_unmanaged_and_unmarked_directories_are_preserved(self):
        run = artifacts.create(self.project)
        unmarked = run.parent / 'unmarked'
        unmarked.mkdir()
        for path in (self.project, run.parent, unmarked, run / 'nested',
                     run / '..' / run.name):
            with self.subTest(path=path), self.assertRaises(ValueError):
                artifacts.clean(self.project, path, True)
        self.assertTrue(unmarked.exists())
        (run / artifacts.MARKER).write_text('{}')
        with self.assertRaises(ValueError):
            artifacts.clean(self.project, run, True)

    def test_tracked_content_is_preserved(self):
        run = artifacts.create(self.project)
        fixture = run / 'important.txt'
        fixture.write_text('keep')
        subprocess.run(['git', '-C', str(self.project), 'add', str(fixture)], check=True)
        with self.assertRaises(ValueError):
            artifacts.clean(self.project, run, True)
        self.assertEqual(fixture.read_text(), 'keep')

    def test_symlinks_are_rejected_before_any_removal(self):
        run = artifacts.create(self.project)
        target = self.project / 'important'
        target.mkdir()
        (target / 'save.json').write_text('keep')
        for name, destination in [('file', target / 'save.json'), ('dir', target)]:
            link = run / name
            link.symlink_to(destination)
            with self.assertRaises(ValueError):
                artifacts.clean(self.project, run, True)
            self.assertTrue((run / artifacts.MARKER).exists())
            link.unlink()
        marker = run / artifacts.MARKER
        marker.unlink()
        marker.symlink_to(target / 'save.json')
        with self.assertRaises(ValueError):
            artifacts.clean(self.project, run, True)
        alias = run.parent / 'alias'
        alias.symlink_to(run, target_is_directory=True)
        with self.assertRaises(ValueError):
            artifacts.clean(self.project, alias, True)
        self.assertEqual((target / 'save.json').read_text(), 'keep')

    def test_symlinked_managed_root_is_rejected(self):
        external = self.project / 'external'
        external.mkdir()
        (self.project / 'output').symlink_to(external, target_is_directory=True)
        with self.assertRaises(ValueError):
            artifacts.create(self.project)
        with self.assertRaises(ValueError):
            artifacts.clean(self.project, external / 'run', True)

    def test_git_failure_preserves_run(self):
        run = artifacts.create(self.project)
        (self.project / '.git').rename(self.project / 'hidden-git')
        with self.assertRaises(subprocess.CalledProcessError):
            artifacts.clean(self.project, run, True)
        self.assertTrue(run.exists())


if __name__ == '__main__':
    unittest.main()
