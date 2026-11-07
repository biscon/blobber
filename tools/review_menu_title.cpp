#include "assets/image.hpp"
#include "assets/rules_loader.hpp"
#include "platform/presenter.hpp"
#include "platform/window.hpp"
#include "platform/world_renderer.hpp"
#include "rlgl.h"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/world_visibility_fixtures.hpp"
#include "ui/menu_layout.hpp"
#include "ui/menu_screen.hpp"
#include <iostream>
#include <limits>
using namespace blobber;
namespace {
std::vector<unsigned char> pixels(const std::filesystem::path& file) {
    OwnedImage image(LoadImage(file.c_str()));
    check(image.value.data && image.value.width == 1920 && image.value.height == 1080,
          "Native capture is readable");
    ImageFormat(&image.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    const auto* data = static_cast<const unsigned char*>(image.value.data);
    return {data, data + 1920 * 1080 * 4};
}
} // namespace
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
    unsigned preparedTextures = 0, preparedShaders = 0;
    {
        Presenter presenter(resources, assets);
        const auto initial = resources.counts();
        // Failure on title upload preserves the background and permits retry.
        resources.failAfter(1);
        rejects([&] { presenter.prepareMenu(); }, "Title upload failure is explicit");
        resources.failAfter(-1);
        check(resources.counts().textures == initial.textures + 1,
              "Failed title upload retains only the prepared background");
        // Background is cached now: fail shader creation after acquiring the title texture.
        resources.failAfter(1);
        rejects([&] { presenter.prepareMenu(); }, "Shader acquisition failure is explicit");
        resources.failAfter(-1);
        check(resources.counts().textures == initial.textures + 1 &&
                  resources.counts().shaders == initial.shaders,
              "Shader failure releases the staged title texture");
        presenter.prepareMenu();
        const auto ready = resources.counts();
        const auto transfers = resources.transfers();
        preparedTextures = ready.textures;
        preparedShaders = ready.shaders;
        check(ready.textures == initial.textures + 2, "Menu prepares background and title textures");
        check(ready.shaders == initial.shaders + 1, "Menu prepares one title shader");
        presenter.prepareMenu();
        check(resources.counts().textures == ready.textures &&
                  resources.transfers().uploads == transfers.uploads &&
                  resources.counts().shaders == ready.shaders,
              "Repeated menu preparation reuses its textures and shader");
        CharacterLibrary library(catalog, output / "unused-library");
        MenuView menu(catalog, library);
        for (const bool canContinue : {false, true}) {
            menu.canContinue = canContinue;
            menu.titleSeconds = canContinue ? 2 : 0;
            presenter.presentMenu(drawMenu, &menu);
            const auto name = canContinue ? "main-menu-continue" : "main-menu";
            presenter.capture(output / (std::string(name) + ".png"));
            presenter.capture(output / (std::string(name) + "-1280.png"), 1280, 720);
        }
        const auto active = pixels(output / "main-menu-continue.png");
        const auto captureAt = [&](double seconds) {
            menu.titleSeconds = seconds;
            presenter.presentMenu(drawMenu, &menu);
            presenter.capture(output / "phase-check.png");
            return pixels(output / "phase-check.png");
        };
        const auto resting = captureAt(0);
        check(captureAt(14) == active, "Gleam repeats deterministically after twelve seconds");
        check(captureAt(-1) == resting && captureAt(std::numeric_limits<double>::quiet_NaN()) == resting &&
                  captureAt(std::numeric_limits<double>::infinity()) == resting,
              "Negative and nonfinite decorative times safely use phase zero");
        unsigned changed = 0;
        const auto titleRect = menuLayout::mainTitle(true);
        for (int y = 0; y < 1080; ++y)
            for (int x = 0; x < 1920; ++x) {
                const auto offset = std::size_t(y * 1920 + x) * 4;
                const bool different = !std::equal(active.begin() + offset, active.begin() + offset + 4,
                                                   resting.begin() + offset);
                if (!titleRect.contains({float(x), float(y)}))
                    check(!different, "Shader leaves background, controls and text unchanged");
                else
                    changed += different;
            }
        check(changed > 2000, "Active gleam visibly changes the logo");
        {
            test::VisibilityFixture fixture;
            const auto map = test::worldRoom(7, 7);
            MovementState state;
            state.player = {{3, 3}, Heading::North};
            WorldRenderer renderer(resources, fixture.publication());
            renderer.stage(world_render::FrameInput{map, state});
            check(renderer.commit(), "Tiny generated world commits alongside menu resources");
            check(resources.counts().shaders == 8, "Menu and seven world shaders fit existing capacity");
            renderer.render(world_render::FrameInput{map, state});
            check(captureAt(14) == active, "Menu gleam survives world drawing and shader coexistence");
        }
        check(resources.counts().shaders == ready.shaders, "World release retains only the menu shader");
        const auto frameTransfers = resources.transfers();
        allocations = 0;
        watching = true;
        for (unsigned frame = 0; frame < 6; ++frame) {
            menu.canContinue = frame % 2;
            menu.titleSeconds = frame * .7;
            presenter.presentMenu(drawMenu, &menu);
        }
        watching = false;
        check(!allocations, "Engine-owned C++ menu frames allocate no heap storage");
        const auto after = resources.counts();
        check(after.textures == ready.textures && after.targets == ready.targets &&
                  after.shaders == ready.shaders && after.bytes == ready.bytes &&
                  resources.transfers().uploads == frameTransfers.uploads,
              "Menu frames preserve GPU resource counts, bytes and uploads");
        menu.model.screen = MenuScreen::Library;
        presenter.presentMenu(drawMenu, &menu);
        presenter.capture(output / "character-library.png");
        menu.model.screen = MenuScreen::Main;
        menu.canContinue = true;
        std::filesystem::create_directory(output / "frames");
        for (unsigned frame = 0; frame < 120; ++frame) {
            menu.titleSeconds = frame / 10.0;
            presenter.presentMenu(drawMenu, &menu);
            char name[32];
            std::snprintf(name, sizeof(name), "frame-%03u.png", frame);
            presenter.capture(output / "frames" / name, 768, 432);
        }
        std::cout << "Prepared menu GPU bytes=" << ready.bytes - initial.bytes << '\n';
    }
    check(resources.counts().textures + 1 == preparedTextures,
          "Presenter destruction releases the title texture lease");
    check(resources.counts().shaders + 1 == preparedShaders,
          "Presenter destruction releases the title shader lease");
    using GlError = unsigned (*)();
    const auto glError = reinterpret_cast<GlError>(rlGetProcAddress("glGetError"));
    check(glError && glError() == 0, "Menu preparation, drawing and release have no OpenGL errors");
    check(IsWindowState(FLAG_WINDOW_HIDDEN), "Capture window remains hidden");
    std::cout << "Five production captures and 120 animation frames; preparation retry/reuse, phase and "
                 "off-logo invariants, eight-shader coexistence, C++ allocations, GPU lifetime and OpenGL "
                 "checks passed. Driver/C allocations are outside this guard.\n";
    return 0;
} catch (const std::exception& error) {
    watching = false;
    std::cerr << error.what() << '\n';
    return 1;
}
