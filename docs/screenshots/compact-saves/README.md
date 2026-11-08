# Compact-save presentation evidence

Mechanically verified 2026-11-09. Interactive timing and user acceptance remain pending.
The two images use an owned simulation fixture and the production renderer/UI in a
hidden 1920×1080 window, with worker work deliberately delayed for 70 ms. No desktop
input or focus automation was used. They illustrate presentation during the wait;
CPU integration tests separately exercise actual campaign victory/defeat publication.

- [Saving](saving.png): existing paused Saving feedback continues to present.
- [Victory during terminal write](victory-during-write.png): the campaign result
  remains visible while the worker finishes; close is deferred without cancellation.

The hidden integration also checks repeated presentation, engine allocation/GPU
resource stability and OpenGL errors. Reproduce with the configured Debug build's
`blobber_world_integration_gpu_tests ASSETS NEW_OUTPUT --campaign-save-review`.
Only these two selected images are retained; disposable output was cleaned.
