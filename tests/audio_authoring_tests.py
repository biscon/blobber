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
    # Two differently pitched chimes separated by silence: schedule survives export,
    # variation preserves the musical intervals, and selective rebuild touches one file.
    bank['cues']['layered']={'variants':[{'id':'01','path':'generated/layered_01.wav'},
                                        {'id':'02','path':'generated/layered_02.wav'}]}
    layered={'duration':.6,'seed':92,'review':False,'layers':[
        {'instrument':9,'frequency':660,'duration':.15,'tone':.2},
        {'instrument':9,'frequency':880,'start':.35,'duration':.2,'tone':.2}]}
    (root/'assets/audio/bank.json').write_text(json.dumps(bank))
    (root/'tools/audio/recipes.json').write_text(json.dumps({'cues':{'owned':recipe,'layered':layered}}))
    sys.argv=['generate','--cue','layered'];author.main()
    import numpy as np
    from scipy.io import wavfile
    paths=[root/f'assets/audio/generated/layered_{v}.wav' for v in ['01','02']]
    original=[p.read_bytes() for p in paths]
    for path in paths:
        rate,data=wavfile.read(path);data=data.astype(float)/32768
        assert abs(len(data)/rate-.6)<1/rate
        assert np.max(np.abs(data[int(.2*rate):int(.3*rate)]))<.001,'scheduled gap must remain quiet'
        for begin,end,target in [(0,.12,660),(.35,.52,880)]:
            section=data[int(begin*rate):int(end*rate)]
            frequencies=np.fft.rfftfreq(len(section),1/rate)
            dominant=frequencies[np.argmax(np.abs(np.fft.rfft(section*np.hanning(len(section)))))]
            assert abs(dominant-target)<12,'musical note preserved across variants'
    sys.argv=['generate','--cue','layered','--variant','01'];author.main()
    assert [p.read_bytes() for p in paths]==original,'selective layered recipe is deterministic'
    assert hashlib.sha256(output.read_bytes()).digest()==digest,'unselected cue untouched'
    assert replacement.read_bytes()==b'user-owned-preserve' and override.read_bytes()==before
    # An adopted sample is authoritative content, not a recipe to regenerate.
    imported=root/'assets/audio/replacements/imported.wav';imported.write_bytes(output.read_bytes())
    bank['cues']['adopted']={'variants':[{'id':'01','path':'replacements/imported.wav'}]}
    (root/'assets/audio/bank.json').write_text(json.dumps(bank))
    (root/'tools/audio/recipes.json').write_text(json.dumps({'cues':{
        'owned':recipe,'layered':layered,'adopted':{'source':'retained','review':False}}}))
    before_import=imported.read_bytes()
    sys.argv=['generate','--cue','adopted'];author.main()
    assert imported.read_bytes()==before_import,'retained sample survives regeneration byte-for-byte'
    assert not (root/'assets/audio/generated/adopted_01.wav').exists(),'no discarded synth recreated'
    imported.unlink()
    try:
        author.main();raise AssertionError('missing retained asset silently accepted')
    except FileNotFoundError:pass
print('Selective authoring, reproducibility, validation and replacement preservation passed')
