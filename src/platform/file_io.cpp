#include "platform/file_io.hpp"
#include <stdexcept>
#include <system_error>
#include <utility>
namespace blobber::disk {
std::filesystem::path suffix(std::filesystem::path path, const char* text) {
    path += text;
    return path;
}
Temporary::Temporary(std::filesystem::path path, bool unique) {
    for (unsigned attempt = 0; attempt < 64; ++attempt) {
        path_ = path;
        if (unique) {
            std::array<std::uint8_t, 16> bytes{};
            if (!entropy(bytes))
                throw std::runtime_error("Cannot obtain temporary file entropy");
            constexpr char hex[] = "0123456789abcdef";
            std::string text;
            for (auto b : bytes) {
                text += hex[b >> 4];
                text += hex[b & 15];
            }
            path_ += text;
        }
        try {
            file = File::create(path_);
            owned_ = true;
            return;
        } catch (const std::system_error& e) {
            if (!unique || e.code() != std::errc::file_exists)
                throw;
        }
    }
    throw std::runtime_error("Temporary filename attempts exhausted");
}
Temporary::~Temporary() {
    // Close before deleting on Windows; never remove an unowned pre-existing file.
    file = File{};
    if (owned_) {
        std::error_code ec;
        std::filesystem::remove(path_, ec);
    }
}
void Temporary::publish(const std::filesystem::path& target, bool overwrite) {
    replace(path_, target, overwrite);
    owned_ = false;
}
void writeDocument(const std::filesystem::path& path, const std::string& bytes) {
    std::filesystem::create_directories(path.parent_path());
    Temporary temp(path.parent_path() / ".document-", true);
    temp.file.write(bytes);
    temp.file.flush();
    temp.file.close();
    temp.publish(path);
    confirm(path);
}
} // namespace blobber::disk
