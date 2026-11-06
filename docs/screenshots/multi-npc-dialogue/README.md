# Multi-NPC dialogue — runtime review

2026-11-06. Implemented and mechanically verified; **visual timing and listening
approval remain pending**. Existing resident-artwork approval is unchanged.
These six hidden application-owned captures cover the corrected mayor/guards exchange.
The user rejected indefinite Talk/Laugh and sub-200 ms pans; this correction ends
speech animation at reveal completion and uses 200 ms outgoing/return pans.
Its interactive feel remains pending review.
The [design and verification](../../dialogue_design.md#multi-npc-event-beats)
records automated results and limitations.

| Case | Capture |
| --- | --- |
| Mayor after click-to-complete: Idle, primary close-up and default speech color | [Mayor report](01-mayor-report.png) |
| Halfway through the 200 ms cutaway; speaker context already changed | [Boric whip-pan](02-boric-whip.png) |
| Guard close-up while text reveals: teal speech and authored Laugh clip | [Boric reaction](03-boric-laugh.png) |
| Natural reveal completes while camera stays on Boric; he returns to Idle | [Boric after reveal](04-boric-revealed-idle.png) |
| Click-to-complete immediately returns Balin to Idle; lavender speech remains | [Balin after reveal](05-balin-revealed-idle.png) |
| Dialogue closed and exploration camera restored | [Exploration](06-exploration-restored.png) |

All captures use the same stationary party pose at (10,8), facing west. Actor
placement and game time remain unchanged. The runner verifies host-owned running
gag history, repeat-response selection, exact camera forwarding, reveal-driven Idle
reset and 200 ms outgoing/return timings and zero retained scene texture/alpha leases. No OS input or focus control
was used. Static images establish composition and context, not subjective timing
or audio quality. Provisional synthetic reactions await listening review.

Reproduce with `blobber_review_sanctuary_city ASSETS NEW_OUTPUT dialogue-beats`,
using the normal owned-artifact workflow. Raw trial captures and logs are cleaned
after recording results. The separate six-image innkeeper dialogue gallery retains
distinct earlier eye-level review cases, including its explicitly rejected baseline;
these mayor/guard captures do not replace or imply acceptance of that review.
