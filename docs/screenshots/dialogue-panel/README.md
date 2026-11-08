# Dialogue panel cleanup

Implemented and mechanically verified, awaiting user review (2026-11-06).
Three application-owned 1920×1080 hidden-window captures, with in-process inputs
and no desktop input or focus automation:

1. [Introduction](panel-introduction.png): larger speech immediately below the
   speaker, left-aligned page count and enabled Next; subtitle/help/Skip removed.
2. [Completed multipage response](panel-final-page.png): Next stays visible but
   disabled after the final page has fully revealed.
3. [Single-page response](panel-single-page.png): no Next button. This capture is
   during the camera's return to the Mayor; it documents panel layout.

Focused dialogue/button tests and the production dialogue capture scenario pass.
Interactive feel remains for user review. See [design and verification](../../dialogue_design.md#dialogue-panel-cleanup).
Disposable output was cleaned after selecting these images.
