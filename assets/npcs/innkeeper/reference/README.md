# Innkeeper visual reference

Status: **approved by the user (2026-11-06)**: “approved, continue implementing him”.

Review candidate: [turnaround-v1.png](turnaround-v1.png).

The first economy-related NPC is a stationary, non-hostile human innkeeper behind
the Sanctuary inn bar. The completed reference checkpoint produced a visual mockup:
four cardinal idle views and a larger front-facing speaking-expression study.
These are identity/wardrobe references, not registered runtime animation frames.

The proposed appearance is a burly man aged 55–60 with a warm smile, short greying
hair, a neat salt-and-pepper beard, rolled linen sleeves, a brown waistcoat and a
work apron. The existing goblin archer provides painted rendering style only;
the existing inn capture provides environmental context only.

Exact built-in image generation inputs: [turnaround-v1.prompt.json](turnaround-v1.prompt.json).
The selected source artwork remains here as a durable reference. No game
definition or runtime asset consumes it before approval.

The [implementation design](../../../../docs/innkeeper_design.md) covers the friendly
stationary actor and its integration: four directional idle animations and one
front-facing talk animation. Walking, attacking, dialogue, trading and other
economy behavior are outside this checkpoint. Rendering remains prepared-image
based with fixed 1920×1080 presentation and discrete movement.

## Review and verification

Review the generated mockup for age, warmth, anatomy, consistent identity and
wardrobe across directions, full silhouettes and genuine background transparency.
Visual inspection confirms four complete cardinal standing views and a welcoming
front-facing speaking-expression study, with consistent clothing and human anatomy.
The selected PNG is 1920×819 RGBA; alpha spans 0–255 and the background contains
fully transparent pixels. This is a concept reference; production animation
registration, edge cleanup, in-engine scale and bar occlusion remain unverified.
No gameplay build, test suite or desktop automation is needed for this artwork-only
checkpoint. The source mockup is approved; runtime animation/placement review is separate.
