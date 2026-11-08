# Sanctuary innkeeper — runtime review

2026-11-06. Initial runtime views **rejected by the user** for scale impression and
reversed side orientations. The correction below is **awaiting user re-review**:
only the counter and rear shelves are 15% smaller; the innkeeper and all other
model geometry keep their sizes. Side art now follows his south-facing direction.
The [character reference](../../../assets/npcs/innkeeper/reference/README.md) is
approved. These runtime captures do not imply acceptance of animation or placement.

| Case | Capture |
| --- | --- |
| Before furniture reduction, identical front pose/clock | [before](before-furniture-front.png) |
| Seen from the inn entrance | [entry](entry.png) |
| Idle behind the counter; lower body occluded | [front](innkeeper-front.png) |
| Close anatomical left side, looking west; nose points left | [left](innkeeper-left.png) |
| Close anatomical right side, looking east; nose points right | [right](innkeeper-right.png) |
| Front talk preview at 180 ms | [talk 1](innkeeper-talk-1.png) |

Captured by the application-owned `blobber_review_inn ... innkeeper` mode with a
hidden 1920×1080 OpenGL window and in-process pose/time controls. No OS input,
window focus control or interactive desktop automation was used. The runner
checks exclusion from offensive picking/combat and stages normal city/inn travel.
Rear artwork and all four directional talk fallbacks are covered by source
inspection and the generated CPU fixture; the shelves prevent a straight rear
approach in this inn layout. Normal gameplay selects idle; no talk prompt or
dialogue/trading UI exists yet. Final timing/feel review remains user-owned.
The resident remains in cell (6,3), subcell (1,1), with feet at (6.75,3.75):
the half-subcell nearest the counter. Current campaigns remain compatible.

Exactly six selected captures are retained. Disposable captures, extraction
derivatives and logs are removed with the owned agent run. Reproduction, results
and limits live in the [review correction](../../innkeeper_design.md#runtime-review-correction--2026-11-06).
