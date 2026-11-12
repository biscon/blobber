"""Owned miniature packaging/provenance/cache fixtures; no production assets or GPU."""
import copy
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import unittest
from unittest.mock import patch
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools/distribution'))
import archive
import hosted_evidence as e
import hosted_release as h
import manage_cache as cache
import prepared
import windows_archive

ENV = {'GITHUB_SHA':'a'*40, 'GITHUB_RUN_ID':'123', 'GITHUB_RUN_ATTEMPT':'1',
       'GITHUB_WORKFLOW_REF':'owner/repo/.github/workflows/windows-review.yml@refs/heads/review',
       'GITHUB_WORKFLOW_SHA':'a'*40, 'GITHUB_REPOSITORY':'owner/repo'}


class HostedTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        env = patch.dict(os.environ, ENV)
        env.start();self.addCleanup(env.stop)

    def make_player(self, platform):
        directory = self.root/platform;directory.mkdir()
        exe = directory/'exe';exe.write_bytes(b'owned executable');exe.chmod(0o755)
        name = 'blobber-1.2.3-'+platform+'-x86_64'
        info = {'revision':ENV['GITHUB_SHA'],'dirty':False,'project_version':'1.2.3',
                'prepared':{'input_digest':'b'*64,'producer_revision':'c'*40,'publication_sha256':'d'*64},'identities':{'fixture':7}}
        extra = {'BUILD.json':archive.encoded(info),'README.txt':b'Run the game'}
        if platform == 'linux':
            player = directory/(name+'.tar.gz')
            archive.write_archive(player,name,{'blobber':exe},extra,1)
        else:
            player = directory/(name+'.zip')
            windows_archive.write_zip(player,name,{'blobber.exe':exe},extra,1)
        sha = archive.digest(player)
        phases = {p:{'status':'passed'} for p in ('create','continue')}
        verification = {'status':'passed','archive_sha256':sha,'phases':phases,'depth_copy':'passed',
                        'runtimes':{n:{'status':'passed','phases':phases} for n in ('ubuntu22','debian13')}}
        (directory/'build.json').write_bytes(archive.encoded(info))
        (directory/'verification.json').write_bytes(archive.encoded(verification))
        checksum = directory/(player.name+'.sha256');checksum.write_text(sha+'  '+player.name+'\n')
        report = {'context':e.context(),'platform':platform,'status':'passed',
                  'prepared_import':{'status':'passed','publication_sha256':'d'*64},
                  'player':{'name':player.name,'bytes':player.stat().st_size,'sha256':sha},
                  'build_sha256':archive.digest(directory/'build.json'),
                  'verification_sha256':archive.digest(directory/'verification.json')}
        return directory,player,checksum,report

    def test_actual_downloaded_player_validation_and_wrong_bytes(self):
        # Windows chmod cannot express the Linux executable-mode contract; that
        # archive path is exercised on Linux, with the same ZIP path on both OSes.
        for platform in (('windows',) if os.name == 'nt' else ('linux','windows')):
            directory,player,checksum,report = self.make_player(platform)
            info,_ = h.verify_player(player,checksum,report,directory)
            self.assertEqual(info['revision'],ENV['GITHUB_SHA'])
            checksum.write_text('0'*64+'  '+player.name+'\n')
            with self.assertRaisesRegex(ValueError,'checksum'):
                h.verify_player(player,checksum,report,directory)
            checksum.write_text(report['player']['sha256']+'  '+player.name+'\n')
            data = bytearray(player.read_bytes());data[-2] ^= 1;player.write_bytes(data)
            with self.assertRaisesRegex(ValueError,'hash'):
                h.verify_player(player,checksum,report,directory)

    def test_unavailable_stale_and_incomplete_smoke_rejected(self):
        directory,player,checksum,report = self.make_player('windows')
        original = e.read_json(directory/'verification.json')
        for changes in ({'status':'unverified'},{'archive_sha256':'b'*64},{'depth_copy':None},{'phases':{'create':{'status':'passed'}}},
                        {'phases':{'create':{'status':'failed'},'continue':{'status':'passed'}}}):
            (directory/'verification.json').write_bytes(archive.encoded({**original,**changes}))
            changed = {**report,'verification_sha256':archive.digest(directory/'verification.json')}
            with self.subTest(changes=changes),self.assertRaises(ValueError):
                h.verify_player(player,checksum,changed,directory)

    @unittest.skipIf(os.name == 'nt', 'Linux archive/import proof')
    def test_linux_import_proof_bound_to_actual_publication(self):
        directory,player,checksum,report=self.make_player('linux')
        for proof in [{},{'status':'unverified','publication_sha256':'d'*64},
                      {'status':'passed','publication_sha256':'e'*64}]:
            with self.subTest(proof=proof),self.assertRaisesRegex(ValueError,'import proof'):
                h.verify_player(player,checksum,{**report,'prepared_import':proof},directory)

    def test_same_version_from_wrong_source_and_attempt(self):
        _,_,_,linux = self.make_player('linux')
        _,_,_,windows = self.make_player('windows')
        e.validate_pair(linux,windows)
        for key,value in [('sha','b'*40),('run_id','124'),('run_attempt','2'),('workflow_sha','d'*40)]:
            changed=copy.deepcopy(windows);changed['context'][key]=value
            with self.subTest(key=key),self.assertRaises(ValueError):e.validate_pair(linux,changed)
        with self.assertRaises(ValueError):e.validate_pair(linux,{**windows,'status':'unverified'})

    def test_transport_ledger_tamper_and_links(self):
        transport=self.root/'transport';transport.mkdir()
        (transport/'fixture.json').write_text('{}')
        e.seal(transport);e.validate_transfer(transport)
        (transport/'fixture.json').write_text('corrupt')
        with self.assertRaises(ValueError):e.validate_transfer(transport)
        if os.name != 'nt':
            (transport/'fixture.json').unlink();(transport/'fixture.json').symlink_to(self.root/'target')
            with self.assertRaises(ValueError):e.seal(transport)

    def test_combined_build_rejects_mixed_content_and_producer(self):
        first={'project_version':'1.2.3','identities':{'world':7},
               'prepared':{'producer_revision':'a'*40,'publication_sha256':'b'*64}}
        e.validate_builds(first,copy.deepcopy(first))
        for key,value in [('project_version','1.2.4'),('identities',{'world':8}),
                          ('prepared',{'producer_revision':'c'*40,'publication_sha256':'b'*64})]:
            with self.subTest(key=key),self.assertRaises(ValueError):e.validate_builds(first,{**first,key:value})

    def test_evidence_extraction_rejects_traversal_duplicates_and_capacity(self):
        for i,members in enumerate([['../escape'],['a','a'],['one']]):
            path=self.root/(str(i)+'.zip')
            with zipfile.ZipFile(path,'w') as z:
                for name in members:z.writestr(name,b'x'*(100 if i==2 else 1))
            with patch.object(e,'LIMIT',50),self.assertRaises(ValueError):
                e.extract_evidence(path,self.root/('extract'+str(i)))
        self.assertFalse((self.root/'escape').exists())

    def test_git_free_ledger_rehashes_inputs_and_unsafe_names(self):
        source=self.root/'source';(source/'assets').mkdir(parents=True)
        (source/'assets/a').write_bytes(b'original')
        ledger=self.root/'ledger.json';ledger.write_text(json.dumps({'assets/a':archive.digest(source/'assets/a')}))
        with patch.object(prepared,'source_files',side_effect=AssertionError('Git must not run')):
            self.assertEqual(prepared.inputs(source,ledger),json.loads(ledger.read_text()))
            (source/'assets/a').write_bytes(b'changed')
            with self.assertRaisesRegex(ValueError,'ledger differs'):prepared.inputs(source,ledger)
            ledger.write_text('{"../escape":null}')
            with self.assertRaises(ValueError):prepared.inputs(source,ledger)

    def test_git_free_prepared_import_preserves_prior_output_on_corruption(self):
        source=self.root/'source';(source/'assets').mkdir(parents=True)
        (source/'assets/a').write_bytes(b'input')
        inputs={'assets/a':archive.digest(source/'assets/a')}
        ledger=self.root/'source-ledger.json';ledger.write_bytes(archive.encoded(inputs))
        bundle=self.root/'bundle'
        for name in ('creator.qoi','portraits-small.qoi','portraits-large.qoi','world/metadata.json','world/sources.json','world/textures.bin'):
            path=bundle/'prepared'/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(b'owned publication fixture')
        manifest={'version':1,'inputs':inputs,'input_digest':__import__('hashlib').sha256(archive.encoded(inputs)).hexdigest(),
                  'files':prepared.files(bundle/'prepared')}
        (bundle/'prepared.json').write_bytes(archive.encoded(manifest))
        output=self.root/'build/prepared'
        with patch.object(prepared,'source_files',side_effect=AssertionError('Git unavailable')):
            prepared.import_prepared(bundle,output,source,ledger)
            prepared.import_prepared(bundle,output,source,ledger)
            (bundle/'prepared/creator.qoi').write_bytes(b'corrupt')
            with self.assertRaises(ValueError):prepared.import_prepared(bundle,output,source,ledger)
        self.assertEqual((output/'creator.qoi').read_bytes(),b'owned publication fixture')

    def test_cache_reserves_current_ref_version_and_actual_sizes(self):
        key='wd01-prepared-v1-'+'a'*64
        def r(id,key=key,ref='refs/heads/review',version=cache.CACHE_VERSION,size=500*1024**2):
            return {'id':id,'key':key,'ref':ref,'version':version,'size_in_bytes':size,'created_at':str(id)}
        records=[r(1,ref='refs/heads/sibling'),r(2,version='other'),r(3,key='unrelated')]
        self.assertEqual(cache.removals(records,key,'refs/heads/review','refs/heads/main',cache.CACHE_VERSION),[1])
        records.append(r(4))
        self.assertEqual(set(cache.removals(records,key,'refs/heads/review','refs/heads/main',cache.CACHE_VERSION)),{1})
        self.assertFalse(cache.usable(r(5,size=2*1024**3),key,'refs/heads/review','refs/heads/main',cache.CACHE_VERSION))
        self.assertFalse(cache.usable(r(5,ref='refs/heads/sibling'),key,'refs/heads/review','refs/heads/main',cache.CACHE_VERSION))
        self.assertTrue(cache.usable(r(5,ref='refs/heads/main'),key,'refs/heads/review','refs/heads/main',cache.CACHE_VERSION))

    def test_download_rejects_wrong_run_before_fetch(self):
        metadata={'workflow_run':{'id':124,'head_sha':'a'*40}}
        with patch.object(subprocess,'check_output',return_value=json.dumps(metadata)),patch.object(subprocess,'run') as fetch:
            with self.assertRaisesRegex(ValueError,'another run'):e.download_artifact(17,self.root/'download')
            fetch.assert_not_called()

    @unittest.skipIf(os.name == 'nt', 'Linux resource policy')
    def test_baseline_api_rejects_exhaustion_before_build_lock(self):
        sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
        import linux_release
        from types import SimpleNamespace
        for disk,memory in [(7*1024**3,8*1024**2),(10*1024**3,5*1024**2)]:
            with patch.object(linux_release.shutil,'disk_usage',return_value=SimpleNamespace(free=disk)), \
                 patch.object(Path,'read_text',return_value='MemAvailable: '+str(memory)+' kB\n'), \
                 patch.object(linux_release,'build_lock') as lock:
                with self.assertRaisesRegex(RuntimeError,'Baseline requires'):
                    linux_release.build_release(self.root,self.root/'unused-build')
                lock.assert_not_called()

    @unittest.skipIf(os.name == 'nt', 'Linux compiler image transport')
    def test_package_transport_hash_and_url_policy(self):
        sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
        import linux_release
        recipe=self.root/'recipe';recipe.mkdir()
        (recipe/'lock.json').write_text('{"base_image":"fixture-pinned-base"}')
        (recipe/'packages.tsv').write_text('owned-package\t1.0\tamd64\n')
        ledger=recipe/'build-ubuntu22.debs.tsv'
        lock=recipe/'build-ubuntu22-lock.json'
        def write(url='http://archive.ubuntu.com/ubuntu/pool/main/f/fixture_1_amd64.deb'):
            ledger.write_text('a'*64+'\t'+url+'\tfixture_1_amd64.deb\n')
            lock.write_bytes(archive.encoded({'version':1,'base_image':'fixture-pinned-base','packages':1,
                'ledger_sha256':archive.digest(ledger),'inventory_sha256':archive.digest(recipe/'packages.tsv')}))
        write();linux_release.validate_package_transport(recipe)
        ledger.write_text(ledger.read_text().replace('a'*64,'b'*64))
        with self.assertRaisesRegex(RuntimeError,'lock differs'):linux_release.validate_package_transport(recipe)
        for url in ['http://example.invalid/ubuntu/pool/a.deb','file:///tmp/a.deb','http://archive.ubuntu.com/other/a.deb']:
            write(url)
            with self.subTest(url=url),self.assertRaisesRegex(RuntimeError,'transport entry'):
                linux_release.validate_package_transport(recipe)

    @unittest.skipIf(os.name == 'nt', 'Linux baseline API preparation')
    def test_build_api_creates_container_home_and_tmp_before_invocation(self):
        sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
        import linux_release
        def invoke(*args):
            self.assertTrue((self.root/'home').is_dir())
            self.assertTrue((self.root/'tmp').is_dir())
            self.assertTrue((self.root/'build/.baseline.lock').is_file())
        with patch.object(linux_release,'check_resources'),patch.object(linux_release,'_build_release',side_effect=invoke) as command:
            linux_release.build_release(self.root,self.root/'build',dist=True)
            command.assert_called_once()


if __name__ == '__main__':unittest.main()
