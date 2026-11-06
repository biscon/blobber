# Friendly NPC dialogue — runtime review

2026-11-06. **Implemented and mechanically verified; awaiting user visual and
interactive review of the new eye-level framing.** The user accepted the interface
and fluid transition; the previous counter-height framing was rejected. The [generated mockup](../../mockups/dialogue-v1/README.md)
was approved as a direction. These actual engine captures do not imply acceptance.
See the [design and verification record](../../dialogue_design.md).

| Case | Capture |
| --- | --- |
| Rejected low counter-height view, matched completed topic | [Before correction](before-eye-height.png) |
| Camera halfway in, greeting revealing at 350 ms | [Transition](dialogue-transition.png) |
| Settled camera at 700 ms, introduction revealing with Talk | [Greeting](dialogue-greeting.png) |
| Completed topic response, innkeeper Idle, topics available | [Topic choice](dialogue-topic-idle.png) |
| Return visit, short greeting and immediately available topics | [Return](dialogue-return.png) |
| Conversation closed, original exploration camera restored | [Restored](dialogue-restored.png) |

All six use the same party pose, cell (6,5) facing north. The innkeeper remains
at (6,3), subcell (1,1), facing south. The fixed 500×282 world viewport and
1920×1080 application presentation are unchanged; panels cover the lower world
view instead of resizing it. The corrected innkeeper uses the generic defaults:
0.6-cell distance and automatic eye height from the visible standing body. Its
camera is at (6.75,0.543602,4.35), meeting the character's approximate eye line.
Furniture no longer determines framing. An explicit JSON eye_height can still
adjust unusual artwork. The rejected baseline uses .75 / .33.

Captured by the application-owned `blobber_review_inn ASSETS NEW_OUTPUT dialogue`
mode with a hidden OpenGL window and in-process commands/time. No OS input, focus
changes or desktop automation. The runner verifies paused gameplay, camera input
agreement, Talk/Idle, first-meeting history, repeat entry, checkpoint history and
world texture/alpha release. GPU CPU/ID parity and small generated future-NPC
fixtures are tested separately. Stills cannot establish transition feel or reveal
speed; those remain for interactive user review.

Exactly six selected PNGs are retained. Unselected trials and disposable logs are
cleaned with the owned agent run; source dialogue, reference artwork and mockup
remain durable project assets.
