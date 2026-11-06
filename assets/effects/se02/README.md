# SE02 authored spell and portrait animation

Generated with the built-in ImageGen tool on 2026-11-05. Original and cleaned masters
are durable source inputs, with initial prompts in `source/*-prompt.txt` and cleanup
instructions in `source/cleanup-prompts.md`. No fallback API or production game art
is used by correctness tests.

Regenerate runtime inputs with `python3 tools/prepare_se02_effects.py`. This offline
packer registers eight frames per row in 128² cells, with 112² content and eight-pixel
transparent gutters. It uses nearest-neighbor resampling and the same explicit
low-alpha residue threshold (96) as SE01. Emission is a deterministic luminance gate
on authored pixels: alpha × max(0, min(R,G,B) − 128) / 127. It cannot extend outside
the body silhouette. The pressure-front row alone is selected from
`source/contact-pressure-master.png`; other contact rows retain the cleaned master.
World publication creates premultiplied mip chains; sampling
is capped at mip 3 for every typed effect texture. The portrait atlas uses straight
RGBA and the native UI's ordinary texture pipeline.

| Sheet | Rows, top to bottom |
| --- | --- |
| arcane | Force dart, force impact, fire ray, ray impact, radiant bolt, radiant impact, Sacred Flame, guiding coating |
| contact | Electricity, inward necrotic pulse, acid splash, poison cloud, pressure front, Shatter fracture, arcane/dust particles, acid flecks |
| portraits | Green healing, blue protection, neutral tintable damage pulse, violet harmful wisps |

`arcane-body/emission.png` and `contact-body/emission.png` are 1024²; `portraits.png`
is 1024×512. World sprites run at 80 ms/frame; short contacts compress/fade within
the existing combat deadline and cosmetic tail. Portrait animation uses its own
bounded supportive/hostile duration. Only runtime sheets are loaded in play.

SE01 source/runtime sheets are unchanged. SE02 visual acceptance is pending; see
[design and verification](../../../docs/se02_design.md).
