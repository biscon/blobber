# Sanctuary inn review

Hidden application-owned 1920×1080 captures, seed 5, supersampling and environment
AO enabled. No desktop mouse/keyboard/focus automation was used. Interactive review
remains user-owned.

- [Entry and common room](review-v2/entry.png)
- [Bar and shelves](review-v2/bar.png), [staff aisle](review-v2/staff-aisle.png)
- [Seating](review-v2/seating.png), [plants](review-v2/plant-close.png), [ale barrels](review-v2/ale-barrels.png)
- [Hearth](review-v2/hearth.png), animation [phase 0](review-v2/fire-0.png) / [phase 1](review-v2/fire-1.png)
- [Decorative staircase](review-v2/stairs.png)
- [Free rest](review-v2/free-rest.png), [inn map](review-v2/inn-map.png)
- [City entry prompt](review-v2/city-entry.png), [exit prompt](review-v2/exit.png), [return outdoors](review-v2/city-return.png)
- [Continue inside](review-v2/continue-inn.png)
- [Resource and behavior report](review-v2/report.txt)

Reproduce sequentially after a production build:

```sh
nice -n 15 ionice -c 3 cmake --build --preset debug --target blobber_review_inn -j 2
nice -n 15 ionice -c 3 cmake-build-debug/blobber_review_inn cmake-build-debug/assets NEW_OUTPUT
```

The runner checks active/staged cancellation, commit, asset release, paused fire,
unchanged picking revision during animation, save roundtrip and return position.
