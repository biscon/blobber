# Sanctuary inn review

Hidden application-owned 1920×1080 captures, seed 5, supersampling and environment
AO enabled. No desktop mouse/keyboard/focus automation was used. Interactive review
remains user-owned.

Current captures include the human-scale correction: lower furnishings, smaller
loose props/doorway, normal stair risers and narrower floorboards. The previous
[entry](review-v2/entry.png) and [counter](review-v2/bar.png) are retained for comparison.

- [Entry and common room](human-scale/entry.png)
- [Bar and shelves](human-scale/bar.png), [staff aisle](human-scale/staff-aisle.png)
- [Seating](human-scale/seating.png), [plants](human-scale/plant-close.png), [ale barrels](human-scale/ale-barrels.png)
- [Hearth](human-scale/hearth.png), animation [phase 0](human-scale/fire-0.png) / [phase 1](human-scale/fire-1.png)
- [Decorative staircase](human-scale/stairs.png)
- [Free rest](human-scale/free-rest.png), [inn map](human-scale/inn-map.png)
- [City entry prompt](human-scale/city-entry.png), [exit prompt](human-scale/exit.png), [return outdoors](human-scale/city-return.png)
- [Continue inside](human-scale/continue-inn.png)
- [Resource and behavior report](human-scale/report.txt)

Reproduce sequentially after a production build:

```sh
nice -n 15 ionice -c 3 cmake --build --preset debug --target blobber_review_inn -j 2
nice -n 15 ionice -c 3 cmake-build-debug/blobber_review_inn cmake-build-debug/assets NEW_OUTPUT
```

The runner checks active/staged cancellation, commit, asset release, paused fire,
unchanged picking revision during animation, save roundtrip and return position.
