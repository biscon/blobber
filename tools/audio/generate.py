#!/usr/bin/env python3
"""Offline Csound authoring. Only generated/ and authoring reports are writable outputs."""
import argparse, hashlib, html, json, os, pathlib, subprocess, tempfile, wave
import numpy as np
from scipy.io import wavfile
ROOT=pathlib.Path(__file__).resolve().parents[2]
AUDIO=ROOT/'assets/audio'
def run(args):
    p=subprocess.run(args,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    if p.returncode: raise RuntimeError(' '.join(map(str,args))+'\n'+p.stdout[-5000:])
    return p.stdout
def metrics(path,loop=False):
    with wave.open(str(path)) as w:
        assert w.getframerate()==48000 and w.getsampwidth()==2
        channels=w.getnchannels();data=np.frombuffer(w.readframes(w.getnframes()),dtype='<i2').reshape(-1,channels)/32768.
    peak=float(np.max(np.abs(data)));rms=float(np.sqrt(np.mean(data*data)));dc=float(np.max(np.abs(np.mean(data,axis=0))))
    assert len(data)>100 and 1e-5<peak<.708 and rms>1e-6 and dc<.01,(path,peak,rms,dc)
    seam=float(np.max(np.abs(data[0]-data[-1])))
    if loop: assert seam<.04,(path,seam)
    else: assert max(np.max(np.abs(data[:1])),np.max(np.abs(data[-1:])))<.02,(path,'endpoints')
    return dict(seconds=len(data)/48000,channels=channels,peak_dbfs=20*np.log10(peak),rms_dbfs=20*np.log10(rms),dc=dc,seam=seam,sha256=hashlib.sha256(path.read_bytes()).hexdigest())
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--cue',action='append');ap.add_argument('--variant');ap.add_argument('--dry-run',action='store_true');ap.add_argument('--validate',action='store_true');args=ap.parse_args()
    bank=json.loads((AUDIO/'bank.json').read_text());recipes=json.loads((ROOT/'tools/audio/recipes.json').read_text())['cues']
    template=(ROOT/'tools/audio/voices.csd').read_text()
    cues=bank['cues'];selected=args.cue or list(cues)
    if any(x not in cues for x in selected):raise ValueError('Unknown cue')
    if args.variant and not any(v['id']==args.variant for name in selected for v in cues[name]['variants']):ap.error('Variant does not exist in selected cues')
    checked=0
    reportpath=AUDIO/'generated/provenance.json';report=json.loads(reportpath.read_text()) if reportpath.exists() else {}
    versions={tool:next(line for line in run([tool,'--version']).splitlines() if ('Csound version' if tool=='csound' else 'SoX') in line) for tool in ['csound','sox']}
    with tempfile.TemporaryDirectory(prefix='blobber-audio-') as tmp:
        tmp=pathlib.Path(tmp)
        for name in selected:
            c=cues[name];r=recipes[name]
            for index,v in enumerate(c['variants']):
                if args.variant and v['id']!=args.variant:continue
                output=AUDIO/v['path']
                if output.parent!=AUDIO/'generated':raise ValueError('Refusing non-generated output')
                if args.dry_run:print(name,v['id'],output);continue
                if not args.validate:
                    seed=r['seed']+index*17;freq=r['frequency']*(.94+.04*index);duration=r['duration'];loop=c.get('loop',False)
                    length=duration+(.25 if loop else 0)
                    score=f'i {r["instrument"]} 0 {length} {freq} {r["tone"]} 1\n'
                    if name=='ambience':
                        score+=''.join(f'i 1 {t} .6 {620+(i%3)*213} .1 .055\n' for i,t in enumerate([7,18.2,19.7,36,51]))
                        score+='i 3 28 1.7 32 .3 .025\n'
                    # p6 is per-layer level; the sound design itself stays in Csound.
                    lines=[]
                    for line in template.splitlines():
                        lines.append('out p6*('+line[4:]+')' if line.startswith('out ') else line)
                    csd='\n'.join(lines).replace('$SEED',str(seed)).replace('$SCORE',score+'e')
                    source=tmp/'render.csd';source.write_text(csd)
                    raw=tmp/'raw.wav';run(['csound','-W','-f','-o',str(raw),str(source)])
                    # Keep float synthesis peaks inside SoX input range before filtering.
                    rate, data=wavfile.read(raw)
                    data=data.astype(np.float32); data*=.7/max(float(np.max(np.abs(data))),1e-10)
                    wavfile.write(raw,rate,data)
                    # Loop crossfade and stereo decorrelation are processing, not synthesis.
                    if loop:
                        rate,data=wavfile.read(raw);n=12000;blend=np.linspace(0,1,n,dtype=np.float32)
                        y=data[:-n].copy();y[:n]=data[-n:]*(1-blend)+data[:n]*blend
                        if name=='ambience':y=np.column_stack((y,np.roll(y,2387)*.92))
                        wavfile.write(raw,rate,y.astype(np.float32))
                    output.parent.mkdir(exist_ok=True)
                    final=tmp/'final.wav'
                    effects=['highpass','35','gain','-n','-6']
                    if not loop:effects+=['fade','t','.002',str(duration),'.015']
                    run(['sox','-R',str(raw),'-r','48000','-b','16',str(final)]+effects)
                    m=metrics(final,loop)
                    staged=output.with_suffix(".tmp");staged.write_bytes(final.read_bytes());os.replace(staged,output)
                    report[v['path']]=dict(cue=name,variant=v['id'],seed=seed,recipe=r,tools=versions,generated=True,review=r['review'],metrics=m,recipe_sha256=hashlib.sha256((csd+json.dumps(r,sort_keys=True)).encode()).hexdigest())
                else:metrics(output,c.get('loop',False))
                checked+=1
            print(name,flush=True)
    if args.dry_run:return
    reportpath.write_text(json.dumps(report,indent=2)+'\n')
    rows=['<!doctype html><meta charset="utf-8"><title>Blobber audio audition</title><style>body{background:#151d1d;color:#ddd;font:16px sans-serif;max-width:1000px;margin:30px auto}section{border-top:1px solid #789;padding:12px}audio{width:300px}small{color:#dcc58f}</style><h1>Underkeep sound bank</h1><p>Generated provisional sound design. Creature voices and creaks need listening review. Use game Settings to assess mix balance.</p>']
    playlist=['#EXTM3U'];review=[]
    for name,c in cues.items():
        rows.append('<section><h2>'+html.escape(name)+'</h2>')
        if recipes[name]['review']:review.append(name);rows.append('<small>Listening review: realism / timbre</small><br>')
        for v in c['variants']:
            rows.append(f'<label>{v["id"]} <audio controls preload="none" src="{html.escape(v["path"])}"></audio></label>');playlist.append(v['path'])
        rows.append('</section>')
    (AUDIO/'audition.html').write_text('\n'.join(rows));(AUDIO/'audition.m3u').write_text('\n'.join(playlist)+'\n')
    (AUDIO/'generated/review.txt').write_text('All sounds remain pending subjective acceptance. Priority listening review:\n'+'\n'.join(review)+'\n')
    print(f'{checked} exports mechanically checked ({len(report)} recorded in bank); listening acceptance pending.')
if __name__=='__main__':main()
