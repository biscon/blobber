#include "app/audio_settings.hpp"
#include "platform/audio.hpp"
#include "platform/window.hpp"
int main(int argc, char** argv) {
    if (argc != 3)
        return 2;
    blobber::Window window(1920, 1080, true);
    blobber::GpuResources resources;
    blobber::AudioRuntime audio;
    blobber::Presenter presenter(resources, argv[1]);
    presenter.audio = &audio;
    blobber::captureAudioSettings(presenter, argv[2]);
}
