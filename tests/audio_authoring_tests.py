"""Owned minimal authoring fixture: deterministic selective rebuild preserves replacements."""
import importlib.util,json,pathlib,sys,tempfile,shutil,hashlib
source=pathlib.Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('author',source/'tools/audio/generate.py');author=importlib.util.module_from_spec(spec);spec.loader.exec_module(author)
with tempfile.TemporaryDirectory(prefix='blobber-author-test-') as tmp:
    root=pathlib.Path(tmp);(root/'assets/audio/replacements').mkdir(parents=True);(root/'tools/audio').mkdir(parents=True)
    shutil.copy(source/'tools/audio/voices.csd',root/'tools/audio/voices.csd')
    bank={'cues':{'owned':{'variants':[{'id':'01','path':'generated/owned_01.wav'}]}}}
    recipe={'instrument':1,'frequency':180,'tone':.1,'duration':.2,'seed':198,'review':False}
    (root/'assets/audio/bank.json').write_text(json.dumps(bank));(root/'tools/audio/recipes.json').write_text(json.dumps({'cues':{'owned':recipe}}))
    replacement=root/'assets/audio/replacements/recorded.wav';replacement.write_bytes(b'user-owned-preserve')
    override=root/'assets/audio/overrides.json';override.write_text('{"cues":{}}');before=override.read_bytes()
    author.ROOT=root;author.AUDIO=root/'assets/audio';sys.argv=['generate','--cue','owned'];author.main()
    output=root/'assets/audio/generated/owned_01.wav';digest=hashlib.sha256(output.read_bytes()).digest()
    author.main();assert hashlib.sha256(output.read_bytes()).digest()==digest,'fixed recipe must reproduce output'
    assert replacement.read_bytes()==b'user-owned-preserve' and override.read_bytes()==before
    sys.argv=['generate','--validate'];author.main()
print('Selective authoring, reproducibility, validation and replacement preservation passed')
