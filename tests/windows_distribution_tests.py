import importlib.util
import io
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import zipfile
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools/distribution'))
import prepared
import windows_archive as archive
import pe_audit
import manage_cache
import windows_smoke
driver_spec=importlib.util.spec_from_file_location('windows_release',
    Path(__file__).resolve().parents[1]/'tools/windows_release.py')
driver=importlib.util.module_from_spec(driver_spec)
driver_spec.loader.exec_module(driver)


class Distribution(unittest.TestCase):
    def test_context_failure_classification(self):
        self.assertTrue(windows_smoke.context_unavailable(
            'Blobber: Graphics unavailable: cannot open an OpenGL window'))
        self.assertFalse(windows_smoke.context_unavailable('GLFW: WGL: warning\nBlobber: missing content'))
        self.assertFalse(windows_smoke.context_unavailable('Blobber: shader compilation failed'))

    def test_owned_cache_bound(self):
        key='wd01-prepared-v1-'+'a'*64
        records=[{'id':i,'key':'wd01-prepared-v1-'+str(i)*64,'size_in_bytes':500*1024**2,
                  'created_at':str(i)} for i in range(1,5)]
        records.append({'id':99,'key':'unrelated-cache','size_in_bytes':3*1024**3,'created_at':'9'})
        self.assertEqual(set(manage_cache.removals(records,key)),{1,2,3})
        records.append({'id':5,'key':key,'size_in_bytes':500*1024**2,'created_at':'5'})
        self.assertEqual(set(manage_cache.removals(records,key)),{1,2,3})
        records[-1]['size_in_bytes']=2*1024**3
        self.assertIn(5,manage_cache.removals(records,key))
        self.assertNotIn(99,manage_cache.removals(records,key))

    def test_path_rejection(self):
        for n in ['../escape','C:/file','a\\b','a/CON.txt','a/x:ads','a/file.','a/file ','/absolute','a/NUL']:
            with self.subTest(n=n), self.assertRaises(ValueError): archive.windows_path(n)

    def test_zip_roundtrip_and_determinism(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp); game=root/'game'; game.write_bytes(b'game')
            files={'blobber.exe':game}; extra={'BUILD.json':b'{}','README.txt':b'read'}
            archive.write_zip(root/'one.zip','package',files,extra,0)
            archive.write_zip(root/'two.zip','package',files,extra,0)
            self.assertEqual((root/'one.zip').read_bytes(),(root/'two.zip').read_bytes())
            result=archive.inspect_zip(root/'one.zip',root/'extract')
            self.assertEqual(result['files'],4)
            self.assertEqual((root/'extract/package/blobber.exe').read_bytes(),b'game')

    def test_malformed_zip(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            for members in [['package/../escape'],['package/CON'],['a/file','b/file'],['a/X','a/x']]:
                with zipfile.ZipFile(root/'bad.zip','w') as z:
                    for n in members:z.writestr(n,b'x')
                with self.assertRaises(ValueError):archive.inspect_zip(root/'bad.zip',root/'extract')
                self.assertFalse((root/'extract').exists())

    def test_pe_imports(self):
        with tempfile.TemporaryDirectory() as temp:
            p=Path(temp)/'game.exe'
            def pe(dll, machine=0x8664):
                b=bytearray(1024); b[:2]=b'MZ';struct.pack_into('<I',b,60,64);b[64:68]=b'PE\0\0'
                struct.pack_into('<HH',b,68,machine,1);struct.pack_into('<H',b,84,240)
                struct.pack_into('<H',b,88,0x20b);struct.pack_into('<I',b,196,16)
                struct.pack_into('<II',b,208,0x1000,40)
                struct.pack_into('<IIII',b,336,512,0x1000,512,512)
                struct.pack_into('<IIIII',b,512,0,0,0,0x1050,0)
                b[592:592+len(dll)+1]=dll.encode()+b'\0';p.write_bytes(b)
            pe('KERNEL32.dll');self.assertEqual(pe_audit.inspect(p)['imports'],['kernel32.dll'])
            for dll in ['VCRUNTIME140.dll','MSVCP140.dll','ucrtbase.dll','mystery.dll']:
                pe(dll)
                with self.assertRaises(ValueError):pe_audit.inspect(p)
            pe('KERNEL32.dll',0x14c)
            with self.assertRaises(ValueError):pe_audit.inspect(p)

    def test_prepared_validation_and_ownership(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);source=root/'source';source.mkdir()
            subprocess.run(['git','init','-q',str(source)],check=True)
            (source/'assets').mkdir();(source/'assets/file').write_bytes(b'input')
            subprocess.run(['git','-C',str(source),'add','.'],check=True)
            # A producer revision is required, independent of the user's Git identity.
            subprocess.run(['git','-C',str(source),'-c','user.name=Fixture','-c','user.email=fixture@example.invalid','commit','-qm','fixture'],check=True)
            pub=root/'pub'
            for n in ['creator.qoi','portraits-small.qoi','portraits-large.qoi','world/metadata.json','world/sources.json','world/textures.bin']:
                p=pub/n;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(b'fixture')
            bundle=root/'bundle';prepared.export(pub,bundle,source)
            output=root/'build/prepared';prepared.import_prepared(bundle,output,source)
            prepared.import_prepared(bundle,output,source)
            (output/'creator.qoi').write_bytes(b'broken');prepared.import_prepared(bundle,output,source)
            self.assertEqual((output/'creator.qoi').read_bytes(),b'fixture')
            (bundle/'prepared/creator.qoi').write_bytes(b'corrupt')
            with self.assertRaises(ValueError):prepared.import_prepared(bundle,output,source)
            self.assertEqual((output/'creator.qoi').read_bytes(),b'fixture')
            (source/'assets/file').write_bytes(b'changed')
            with self.assertRaises(ValueError):prepared.validate(bundle,source)
            (source/'assets/file').write_bytes(b'input')
            added=source/'assets/new';added.write_bytes(b'new untracked input')
            with self.assertRaises(ValueError):prepared.validate(bundle,source)
            first=prepared.source_files(source)
            first_identity=driver.source_identity(source)
            added.write_bytes(b'changed untracked input')
            self.assertNotEqual(first,prepared.source_files(source))
            second_identity=driver.source_identity(source)
            self.assertEqual(first_identity['source_status'],second_identity['source_status'])
            self.assertNotEqual(first_identity['source_fingerprint'],second_identity['source_fingerprint'])
            added.unlink();(source/'assets/file').unlink()
            self.assertIsNone(prepared.inputs(source)['assets/file'])
            with self.assertRaises(ValueError):prepared.validate(bundle,source)


if __name__=='__main__':unittest.main()
