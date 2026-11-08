# Sanctuary mayor and guards

2026-11-06: implemented and mechanically verified; runtime appearance approved.
The requested dialogue idle correction awaits review. Approved v2 concepts remain
accepted. Hidden application-owned captures
use the production artwork and renderer at 1920×1080, without desktop input.

| Capture | Review purpose |
| --- | --- |
| [Town-square formation](formation.png) | Mayor in front, Balin on the left and Boric on the right, building behind. |
| [Mayor introduction](mayor-page.png) | Rescue premise, readable dialogue and conversation framing. |
| [Mayor talking](mayor-talk.png) | Authored gesture and tiny comic spit flecks during text reveal. |
| [Boric laughing](boric-laugh-0-2.png) | Third Laugh pose in an isolated stationary diagnostic. |
| [Balin laughing](balin-laugh-0-2.png) | Third Laugh pose in an isolated stationary diagnostic. |
| [Mayor walking](mayor-walk-2-1.png) | Side-facing Walk pose and foot registration in a stationary diagnostic. |
| [Boric idling during dialogue](boric-dialogue-idle.png) | Boric's second idle pose while the mayor conversation remains open. |
| [Balin idling during dialogue](balin-dialogue-idle.png) | Balin's second idle pose during the same frozen-simulation conversation. |

Isolated animation diagnostics omit the party HUD and display its default Depth I
label; they use Sanctuary geometry. They do not represent guard dialogue or roaming.
All Walk directions and Talk/Laugh frame boundaries were exercised by the review
runner. Eight images are retained while the idle correction awaits review: the six
original accepted appearance cases and two additional guard-idle cases. The focused
runner verifies that both guards render both idle poses during the conversation.

Start a new campaign, turn west at the arrival square, approach the mayor and
press F or click his body. Guards deliberately have no conversation. Walking and
laughter are prepared assets for later behavior/dialogue work.

See [design and verification](../../sanctuary_residents_design.md). Source sheets,
prompts and registration are durable under `assets/npcs/mayor`, `boric` and `balin`;
unselected captures and extraction copies are disposable.
