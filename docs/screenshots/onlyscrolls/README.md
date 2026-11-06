# OnlyScrolls portrait briefing

Status: timing correction implemented and mechanically verified, **awaiting user
review** (2026-11-06). User rejected the original immediate, persistent overlay.
The six replacement captures show the YAML-controlled delay and first-Next dismissal,
refreshed with the larger dialogue text and explicit Next control.
Existing character-reference approvals are unchanged; new derived artwork and
interactive timing remain pending.

Six application-owned 1920×1080 captures from a hidden window, using in-process
scripted inputs without desktop input/focus automation.

1. [Boric holds up the document](01-boric-holds-document.png): at 450 ms, the pan
   has settled and the hold-up animation is unobstructed.
2. [Scroll close-up](02-boric-scroll.png): shows at 700 ms, after all four frames.
3. [First Next dismisses](03-boric-dismissed.png): same beat, revealed text and
   idle guard; close-up gone immediately.
4. [Balin unobstructed](04-balin-unobstructed.png): portrait stays hidden through
   the remaining guard dialogue.
5. [Mayor rage](05-mayor-rage.png): dedicated reaction while the final line reveals.
6. [Exploration restored](06-exploration-restored.png): clean return to gameplay.

The runner reads authored timings (currently 300 ms for the document pan and
700 ms for the overlay), checking absence during the pan and just before the delay,
appearance at the delay,
dismissal without skipping the current beat, no delayed reappearance, early-Next
cancellation, replay, topic replacement, frozen simulation and resource release.
Physical listening and interactive feel remain unverified.

See [design and verification](../../dialogue_design.md#overlay-timing-correction)
and [artwork provenance](../../../assets/ui/dialogue/README.md). Superseded evidence
is replaced, and disposable capture/log output is cleaned.
