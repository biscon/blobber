#pragma once
#include "app/options.hpp"
#include "ui/menu_screen.hpp"
namespace blobber {
class Presenter;
bool creatorCaptureFailStage(LibraryStage);
void captureCreator(MenuView&, Presenter&, const Options&);
} // namespace blobber
