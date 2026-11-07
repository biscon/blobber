#!/usr/bin/env python3
"""Measure resident masters and write registration/definitions; never alter pixels.

The existing C++ sheet extractor generates frames during world publication.
Run after visually checking all named masters. One uniform scale per sheet.
"""
import json
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
DURATIONS = {"idle": [1800, 140], "walk": [150] * 4,
             "talk": [180, 160, 180, 220], "laugh": [160, 120, 160, 240],
             "pull_document": [150] * 4, "explosive_rage": [120, 100, 120, 160]}


def register(name, body_height):
    root = ROOT / "assets/npcs" / name
    entries, audit, animations = [], {}, {}
    clips = (("idle", "talk") if name in ("blacksmith", "shadowtart") else
             ("idle", "walk", "talk", *(("laugh",) if name != "mayor" else ()),
              *(("pull_document",) if name == "boric" else ()),
              *(("explosive_rage",) if name == "mayor" else ())))
    for clip in clips:
        views = {}
        for side in (("front",) if clip not in ("idle", "walk") else ("front", "back", "left", "right")):
            key = f"{clip}-{side}"
            with Image.open(root / "animation/sheets" / (key + ".png")) as image:
                assert image.mode == "RGBA", key
                alpha = np.asarray(image.getchannel("A"))
            height, width = alpha.shape
            rows = 1 if clip == "idle" else 2
            split_x, split_y = width // 2, height // 2
            boxes, pivots, grounds = [], [], []
            for frame in range(2 * rows):
                x = split_x if frame % 2 else 0
                y = split_y if frame >= 2 else 0
                w = width - x if frame % 2 else split_x
                h = height if rows == 1 else height - y if frame >= 2 else split_y
                mask = alpha[y:y+h, x:x+w] > 15
                yy, xx = np.nonzero(mask)
                if not len(xx):
                    raise ValueError(f"Empty cell {key}/{frame}")
                left, top, right, bottom = int(xx.min()), int(yy.min()), int(xx.max())+1, int(yy.max())+1
                if left == 0 or right == w or bottom == h:
                    raise ValueError(f"Visible figure touches split/edge: {key}/{frame}")
                boxes.append([left, top, right, bottom])
                grounds.append(bottom)
                # Middle torso silhouette, excluding swinging hands and lower hems.
                centers = []
                for row in range(top + int((bottom-top)*.28), top + int((bottom-top)*.43)):
                    indices = np.flatnonzero(mask[row])
                    if len(indices):
                        centers.append((float(indices[0]) + float(indices[-1]) + 1) / 2)
                pivots.append(round(float(np.median(centers)), 3))
            scale = body_height / max(b[3]-b[1] for b in boxes)
            entry = dict(name=key, columns=2, rows=rows, split_x=split_x,
                         scale=scale, pivot_x=pivots, ground_y=grounds,
                         canvas_width=800, canvas_height=608, anchor_x=400, anchor_y=584)
            if rows == 2:
                entry["split_y"] = split_y
            entries.append(entry)
            audit[key] = dict(source_size=[width, height], alpha_bounds=boxes,
                              target_max_height=body_height, registration=entry)
            views[side] = [dict(image=f"animation/frames/{key}-{i}.png", anchor=[.5, 584/608])
                           for i in range(2*rows)]
        animations[clip] = dict(durations_ms=DURATIONS[clip], width=800/608*.75, views=views)
    for path, data in ((root / "animation/sheets.json", entries),
                       (root / "animation/registration_audit.json", audit),
                       (root / (name + ".json"), dict(id=name, role="resident", source_sheets="animation/sheets.json",
                        width=.45, height=.75, alpha_cutoff=15, animations=animations))):
        path.write_text(json.dumps(data, indent=2) + "\n")
    print(name, len(entries), "sheets registered")


if __name__ == "__main__":
    import argparse
    heights = {"mayor": 464, "boric": 500, "balin": 420, "blacksmith": 520, "shadowtart": 464}
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("characters", nargs="*", choices=tuple(heights))
    args = parser.parse_args()
    for character in args.characters or ("mayor", "boric", "balin"):
        register(character, heights[character])
