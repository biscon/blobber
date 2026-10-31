# R04 review evidence

[Gallery](index.html) · [Verification](../../r04_verification.md) ·
[Design](../../renderer_integration_design.md).

The gallery retains 27 native PNGs and three 960×540 VP9 motion previews. Door source
frames use 75 ms intervals over the 300 ms opening, followed by 40 ms movement
intervals over the normal 160 ms step. The video preserves those timestamps;
compression is a preview, and PNGs preserve exact pixels.

The campaign images come from the retained 1,070-request strict world trace:
ordinary commands, diagnostic route inspection, two guardian kills, actual loot
pickup, descent to floor two, paid recall, save and Continue. There is no terminal
victory reconstruction in these images. The same requests ran through legacy
visibility with zero admission or inspected gameplay-state differences. Comparison
includes Party resources/builds, enemy state (excluding visibility membership),
attack deadlines, AI clock, fixtures, ground effects and loot.

Spell specimens use independent legal builds; level-two spells prepare a level-three
caster through checked Party advancement before the diagnostic script. They are
separate from the campaign expedition. The save overwrite/error images and arena
result images are existing application-owned presentation scenarios; the save error
uses actual injected store failure. No OS input or foreign-window control was used.

At initial delivery (appearance revision 2), all 44 door PNGs and all 568 spell
PNGs matched exactly between Debug and Release on
the reference machine. Only selected frames are retained here. Native filtered door
captures were also inspected; quality-only picking invariants are checked separately.
No cross-GPU bit identity is claimed.

## Baseline effect review correction

Appearance revision 3 corrects Spiritual Weapon clearance, spell colors/fade,
target-plane impact clipping and overlapping multi-dart/ray glyphs. The hammer and
Scorching Ray gallery frames were replaced; Burning Hands phases were added. The
Release spell script exported 586 PNGs with new transient samples; Debug/Release
image parity was not rerun for this correction. Other captures and the retained
strict campaign trace remain initial revision-2 evidence.

Scorching Ray flight/impact/tail/expiry occur at 117/234/294/354 ms after casting in
this specimen. Burning Hands retains its existing 120 ms fiery impact pulses, plus
the separate white body hit flash; there is no flame cone. Its sparse specimen
makes the affected target easier to inspect. No new spell art was introduced.
[Exact times](effect-review-times.tsv) · [Capture metadata](effect-review.json).

The two spell previews preserve sampled phase times in real time with short holds
before casting/after expiry. They are sampled sequences, not continuous recordings.
The spell times are independent of the door preview. PNGs retain native exact pixels.

## Reproduce

```sh
./cmake-build-debug/blobber --renderer world
./cmake-build-debug/blobber --capture-doors /tmp/new-r04-doors
./cmake-build-debug/blobber --renderer world --capture-spells /tmp/new-r04-spells
./cmake-build-debug/blobber --renderer world --capture-projectiles /tmp/new-r04-shots
python3 tools/renderer_integration_review.py --runner cmake-build-debug/blobber_headless --assets cmake-build-debug/assets --scenario docs/screenshots/r04/campaign-scenario.json --output /tmp/new-r04-expedition
```

The review tool is an explicit, bounded single-expedition renderer check, not a
balance experiment or a seed sweep. Use `--backend legacy --compare-trace TRACE`
to execute identical recorded requests and compare simulation outcomes. The retained
trace belongs to its recorded binary/content identities; strict replay correctly
rejects another build. Regenerate it for a new executable, then select operations
from `review.json` for `--replay ... --capture-dir ... --capture-operation N`.
