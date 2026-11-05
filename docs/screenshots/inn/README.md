# Sanctuary inn review

Hidden application-owned 1920×1080 captures, seed 5, supersampling and environment
AO enabled. No desktop mouse/keyboard/focus automation was used. Interactive review
remains user-owned.

Current captures include the human-scale correction, a 25% lower ceiling,
cleaned-up wood joints, single-cell seating groups and no persistent free-rest prompt.
[South wall and ceiling](single-cell-tables/south-wall.png). The previous
[entry](review-v2/entry.png) and [counter](review-v2/bar.png) are retained for comparison.

- [Entry and common room](single-cell-tables/entry.png)
- [Bar and shelves](single-cell-tables/bar.png), [staff aisle](single-cell-tables/staff-aisle.png)
- Adjacent table views: [north](single-cell-tables/table-north.png), [south](single-cell-tables/table-south.png), [east](single-cell-tables/table-east.png), [west](single-cell-tables/table-west.png)
- [Seating](single-cell-tables/seating.png), [plants](single-cell-tables/plant-close.png), [ale barrels](single-cell-tables/ale-barrels.png)
- [Hearth](single-cell-tables/hearth.png), animation [phase 0](single-cell-tables/fire-0.png) / [phase 1](single-cell-tables/fire-1.png)
- [Decorative staircase](single-cell-tables/stairs.png)
- [Free rest](single-cell-tables/free-rest.png), [inn map](single-cell-tables/inn-map.png)
- [City entry prompt](single-cell-tables/city-entry.png), [exit prompt](single-cell-tables/exit.png), [return outdoors](single-cell-tables/city-return.png)
- [Continue inside](single-cell-tables/continue-inn.png)
- [Resource and behavior report](single-cell-tables/report.txt)

Reproduce sequentially after a production build:

```sh
nice -n 15 ionice -c 3 cmake --build --preset debug --target blobber_review_inn -j 2
nice -n 15 ionice -c 3 cmake-build-debug/blobber_review_inn cmake-build-debug/assets NEW_OUTPUT
```

The runner checks active/staged cancellation, commit, asset release, paused fire,
unchanged picking revision during animation, save roundtrip and return position.
