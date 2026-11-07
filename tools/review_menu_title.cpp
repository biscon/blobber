#include "assets/rules_loader.hpp"
#include "platform/presenter.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "ui/menu_screen.hpp"
#include <iostream>
using namespace blobber;
int main(int argc, char** argv) try {
    if (argc != 3)
        return 2;
    const std::filesystem::path assets(argv[1]), output(argv[2]);
    if (!std::filesystem::create_directory(output))
        throw std::runtime_error("Use a new capture directory");
    SetTraceLogLevel(LOG_WARNING);
    const auto catalog = loadRules(assets / "rules/catalog.json");
    Window window(1920, 1080, true);
    check(IsWindowState(FLAG_WINDOW_HIDDEN), "Capture window stays hidden");
    GpuResources resources;
    unsigned preparedTextures = 0;
    {
        Presenter presenter(resources, assets);
        const auto initial = resources.counts();
        // Failure on title upload preserves the background and permits retry.
        resources.failAfter(1);
        rejects([&] { presenter.prepareMenu(); }, "Title upload failure is explicit");
        resources.failAfter(-1);
        check(resources.counts().textures == initial.textures + 1,
              "Failed title upload retains only the prepared background");
        presenter.prepareMenu();
        const auto ready = resources.counts();
        const auto transfers = resources.transfers();
        preparedTextures = ready.textures;
        check(ready.textures == initial.textures + 2, "Menu prepares background and title textures");
        presenter.prepareMenu();
        check(resources.counts().textures == ready.textures &&
                  resources.transfers().uploads == transfers.uploads,
              "Repeated menu preparation reuses its textures");
        CharacterLibrary library(catalog, output / "unused-library");
        MenuView menu(catalog, library);
        for (const bool canContinue : {false, true}) {
            menu.canContinue = canContinue;
            presenter.presentMenu(drawMenu, &menu);
            const auto name = canContinue ? "main-menu-continue" : "main-menu";
            presenter.capture(output / (std::string(name) + ".png"));
            presenter.capture(output / (std::string(name) + "-1280.png"), 1280, 720);
        }
        allocations = 0;
        watching = true;
        for (unsigned frame = 0; frame < 6; ++frame) {
            menu.canContinue = frame % 2;
            presenter.presentMenu(drawMenu, &menu);
        }
        watching = false;
        check(!allocations, "Engine-owned C++ menu frames allocate no heap storage");
        const auto after = resources.counts();
        check(after.textures == ready.textures && after.targets == ready.targets &&
                  after.bytes == ready.bytes && resources.transfers().uploads == transfers.uploads,
              "Menu frames preserve GPU resource counts, bytes and uploads");
        menu.model.screen = MenuScreen::Library;
        presenter.presentMenu(drawMenu, &menu);
        presenter.capture(output / "character-library.png");
        std::cout << "Prepared menu GPU bytes=" << ready.bytes - initial.bytes << '\n';
    }
    check(resources.counts().textures + 1 == preparedTextures,
          "Presenter destruction releases the title texture lease");
    using GlError = unsigned (*)();
    const auto glError = reinterpret_cast<GlError>(rlGetProcAddress("glGetError"));
    check(glError && glError() == 0, "Menu preparation, drawing and release have no OpenGL errors");
    check(IsWindowState(FLAG_WINDOW_HIDDEN), "Capture window remains hidden");
    std::cout << "Five production menu captures; preparation retry, reuse, C++ allocations, GPU lifetime "
                 "and OpenGL checks passed. Driver/C allocations are outside this guard.\n";
    return 0;
} catch (const std::exception& error) {
    watching = false;
    std::cerr << error.what() << '\n';
    return 1;
}
