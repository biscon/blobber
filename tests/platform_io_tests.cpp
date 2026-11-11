#include "platform/file_io.hpp"
#include "support/check.hpp"
#include "support/temp_directory.hpp"
#include <fstream>
#include <iostream>
using namespace blobber;
int main(int argc, char** argv) {
    try {
        if (argc == 3) {
            bool busy = false;
            auto lock = disk::File::lock(std::filesystem::u8path(argv[2]), false, busy);
            if (std::string(argv[1]) == "probe")
                return busy ? 3 : 0;
            check(!busy, "holder acquires lock");
            std::cout << "locked" << std::endl;
            std::cin.get();
            return 0;
        }
        const auto root = testDirectory();
        struct Cleanup {
            std::filesystem::path p;
            ~Cleanup() {
                std::error_code e;
                std::filesystem::remove_all(p, e);
            }
        } cleanup{root};
        const auto path = root / std::filesystem::u8path(u8"save æøå 日本.json");
        check(!disk::read(path, 16, true).exists, "missing file");
        disk::writeDocument(path, "first\n");
        disk::writeDocument(path, "second\n");
        check(disk::read(path, 16).bytes == "second\n", "Unicode path and repeat replacement");
        rejects([&] { disk::read(path, 1); }, "bounded read rejects oversize");
        rejects([&] { disk::read(root, 16); }, "directory rejects");
        {
            disk::Temporary t(root / "temporary");
            t.file.write("new");
            t.file.flush();
            t.file.close();
            rejects([&] { t.publish(path, false); }, "no-clobber refuses existing file");
        }
        check(!std::filesystem::exists(root / "temporary") && disk::read(path, 16).bytes == "second\n",
              "failure cleans owned temporary and preserves destination");
        disk::writeDocument(root / "existing", "retain");
        rejects([&] { disk::Temporary t(root / "existing"); }, "exclusive creation");
        check(disk::read(root / "existing", 16).bytes == "retain", "unowned existing file retained");
        {
            disk::Temporary t(root / "closed");
            t.file.close();
            rejects([&] { t.file.flush(); }, "closed handle failure");
        }
        std::array<std::uint8_t, 16> bytes{};
        check(disk::entropy(bytes), "system entropy available");
        bool busy = false;
        {
            auto first = disk::File::lock(root / "lock", false, busy);
            check(!busy, "first lock");
            auto second = disk::File::lock(root / "lock", false, busy);
            check(busy, "conflicting lock");
        }
        {
            auto released = disk::File::lock(root / "lock", false, busy);
            check(!busy, "lock released");
        }
        std::error_code ec;
        std::filesystem::create_symlink(path, root / "link", ec);
        if (!ec)
            rejects([&] { disk::read(root / "link", 16); }, "symlink rejects");
        else
            std::cout << "Symlink fixture unavailable: " << ec.message() << '\n';
        std::cout << "Native I/O tests passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
