"""Offline authoring only. Pinned model; never publishes into the runtime bank."""
import argparse
import hashlib
import html
import json
import os
from pathlib import Path
import re
import time

MODEL = 'stabilityai/stable-audio-3-small-sfx'
REVISION = '05ca3857881c94a2b84026b1111c537d78f5f9f0'
SOURCE = '5f5e6ac21ae9ff78a6e8847eb7a1dcbe969ffb84'
CACHE = Path('/cache')
OUTPUT = CACHE / 'output'


def snapshot(offline=True):
    from huggingface_hub import snapshot_download
    return Path(snapshot_download(MODEL, revision=REVISION, local_files_only=offline,
                                  max_workers=2, ignore_patterns=['*.png', '.gitattributes']))


def load_model():
    import torch
    from stable_audio_3 import StableAudioModel
    from stable_audio_3.loading_utils import load_diffusion_cond
    root = snapshot()
    config = json.loads((root / 'model_config.json').read_text())
    # Use the text encoder bundled in the same pinned snapshot, not remote main.
    def localize(node):
        if isinstance(node, dict):
            if (node.get('model_name') == 'google/t5gemma-b-b-ul2' or
                    (node.get('repo_id') == MODEL and node.get('subfolder') == 't5gemma-b-b-ul2')):
                node['model_path'] = str(root / 't5gemma-b-b-ul2')
                node.pop('repo_id', None)
                node.pop('subfolder', None)
            for value in node.values():
                localize(value)
        elif isinstance(node, list):
            for value in node:
                localize(value)
    localize(config)
    if not torch.cuda.is_available():
        raise RuntimeError('CUDA unavailable: check NVIDIA CDI. No automatic slow CPU fallback.')
    torch.set_num_threads(2)
    torch.set_num_interop_threads(1)
    model = load_diffusion_cond(config, str(root / 'model.safetensors'), device='cuda', model_half=True)
    model.use_lora = False
    model.lora_names = []
    return StableAudioModel(model, config, 'cuda', True)


def generate(model, name, prompt, duration, seed):
    import numpy as np
    import soundfile as sf
    import torch
    if not re.fullmatch(r'[a-zA-Z0-9_-]+', name):
        raise ValueError('Name must contain only letters, digits, underscores and hyphens')
    if not 0.5 <= duration <= 30 or seed < 0:
        raise ValueError('Use duration 0.5..30 seconds and a nonnegative seed')
    path = OUTPUT / (name + '.wav')
    if path.exists() or path.with_suffix('.json').exists():
        raise ValueError(f'Output already exists: {path}; choose a new name')
    start = time.monotonic()
    audio = model.generate(prompt=prompt, duration=duration, steps=8, cfg_scale=1,
                           seed=seed, batch_size=1)
    data = audio[0].detach().float().cpu().numpy().T
    peak = float(np.max(np.abs(data)))
    if not np.isfinite(data).all() or peak < 1e-5:
        raise RuntimeError('Non-finite or silent model output')
    # Preserve raw float output; deliberate trimming/SoX export happens on selection.
    sf.write(path, data, model.model_config['sample_rate'], subtype='FLOAT')
    record = dict(model=MODEL, revision=REVISION, source_revision=SOURCE, prompt=prompt,
                  duration=duration, seed=seed, steps=8, cfg_scale=1, generated=True,
                  sample_rate=model.model_config['sample_rate'], peak=peak, seconds=time.monotonic()-start,
                  torch=torch.__version__, device=torch.cuda.get_device_name(0),
                  peak_vram_bytes=torch.cuda.max_memory_allocated(),
                  sha256=hashlib.sha256(path.read_bytes()).hexdigest(), listening_review='pending')
    path.with_suffix('.json').write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps(record), flush=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('command', choices=['status', 'download', 'generate', 'audition'])
    ap.add_argument('--prompt')
    ap.add_argument('--name', default='sample')
    ap.add_argument('--duration', type=float, default=4)
    ap.add_argument('--seed', type=int, default=42)
    args = ap.parse_args()
    if args.command == 'download':
        print(snapshot(offline=False))
        return
    os.environ['HF_HUB_OFFLINE'] = '1'
    os.environ['TRANSFORMERS_OFFLINE'] = '1'
    if args.command == 'status':
        import torch
        import stable_audio_3
        print('Source:', SOURCE, 'Torch:', torch.__version__, 'CUDA:', torch.cuda.is_available())
        if torch.cuda.is_available():
            print('GPU:', torch.cuda.get_device_name(0))
        root = snapshot()
        for relative in ['model_config.json', 'model.safetensors', 't5gemma-b-b-ul2/model.safetensors']:
            if not (root / relative).is_file():
                raise RuntimeError(f'Missing pinned model file: {relative}')
        print('Pinned weights ready:', root)
        return
    if args.command == 'generate' and not args.prompt:
        ap.error('--prompt is required for generate')
    OUTPUT.mkdir(exist_ok=True)
    model = load_model()
    if args.command == 'generate':
        generate(model, args.name, args.prompt, args.duration, args.seed)
    else:
        for cue in json.loads(Path('/recipes/audition.json').read_text()):
            generate(model, **cue)
    rows = ['<!doctype html><meta charset="utf-8"><title>Local model auditions</title>',
            '<h1>Stable Audio Small-SFX experiments — listening review pending</h1>']
    for path in sorted(OUTPUT.glob('*.wav')):
        rows.append(f'<p>{html.escape(path.stem)} <audio controls preload="none" src="{html.escape(path.name)}"></audio></p>')
    (OUTPUT / 'index.html').write_text('\n'.join(rows))


if __name__ == '__main__':
    main()
