# Sanctuary inn review

Hidden application-owned 1920×1080 captures, seed 5, supersampling and environment
AO enabled. No desktop mouse/keyboard/focus automation was used. Interactive review
remains user-owned.

Current captures include the human-scale correction, a 25% lower ceiling,
cleaned-up wood joints and no persistent free-rest prompt.
[South wall and ceiling](ceiling-wood-final/south-wall.png). The previous
[entry](review-v2/entry.png) and [counter](review-v2/bar.png) are retained for comparison.

- [Entry and common room](ceiling-wood-final/entry.png)
- [Bar and shelves](ceiling-wood-final/bar.png), [staff aisle](ceiling-wood-final/staff-aisle.png)
- [Seating](ceiling-wood-final/seating.png), [plants](ceiling-wood-final/plant-close.png), [ale barrels](ceiling-wood-final/ale-barrels.png)
- [Hearth](ceiling-wood-final/hearth.png), animation [phase 0](ceiling-wood-final/fire-0.png) / [phase 1](ceiling-wood-final/fire-1.png)
- [Decorative staircase](ceiling-wood-final/stairs.png)
- [Free rest](ceiling-wood-final/free-rest.png), [inn map](ceiling-wood-final/inn-map.png)
- [City entry prompt](ceiling-wood-final/city-entry.png), [exit prompt](ceiling-wood-final/exit.png), [return outdoors](ceiling-wood-final/city-return.png)
- [Continue inside](ceiling-wood-final/continue-inn.png)
- [Resource and behavior report](ceiling-wood-final/report.txt)

Reproduce sequentially after a production build:

```sh
nice -n 15 ionice -c 3 cmake --build --preset debug --target blobber_review_inn -j 2
nice -n 15 ionice -c 3 cmake-build-debug/blobber_review_inn cmake-build-debug/assets NEW_OUTPUT
```

The runner checks active/staged cancellation, commit, asset release, paused fire,
unchanged picking revision during animation, save roundtrip and return position.
