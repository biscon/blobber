#include "platform/window.hpp"
#include "ui/music_banner.hpp"
#include <filesystem>
#include <iostream>
int main(int argc, char** argv) {
    if (argc != 4)
        return 2;
    blobber::Window window(1920, 1080, true);
    blobber::UiFonts fonts(std::filesystem::path(argv[1]) / "fonts");
    const auto background = LoadTexture(argv[2]);
    if (!IsTextureValid(background))
        return 3;
    const auto target = LoadRenderTexture(1920, 1080);
    const std::filesystem::path output(argv[3]);
    std::filesystem::create_directories(output);
    const float times[]{.12f, .65f, 2.f, 5.8f, 2.f, 2.f};
    const char* names[]{"entry", "reveal", "settled", "exit", "two-lines", "long-title"};
    for (unsigned i = 0; i < 6; ++i) {
        BeginTextureMode(target);
        ClearBackground(BLACK);
        DrawTexturePro(background, {0, 0, float(background.width), float(background.height)},
                       {0, 0, 1920, 1080}, {0, 0}, 0, WHITE);
        blobber::drawMusicBanner({i == 5   ? "WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW"
                                  : i == 4 ? "Epic Hair Metal Synth Rock Instrumental"
                                           : "SVT Speed Metal Trance With Chiptune Vibes",
                                  i == 5 ? "A deliberately overlong second line for the title clipping check"
                                  : i == 4 ? "High Energy Motivation"
                                           : "",
                                  "Nick Panek", 7, times[i], true},
                                 fonts);
        EndTextureMode();
        auto image = LoadImageFromTexture(target.texture);
        ImageFlipVertical(&image);
        ExportImage(image, (output / (std::string(names[i]) + ".png")).c_str());
        UnloadImage(image);
    }
    UnloadRenderTexture(target);
    UnloadTexture(background);
    std::cout << "Hidden native HUD announcement captures exported\n";
}
