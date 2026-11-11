#include "platform/file_io.hpp"
#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <system_error>
#include <unistd.h>
#include <utility>
namespace blobber::disk {
namespace {
[[noreturn]] void error(const char* op) {
    throw std::system_error(errno, std::generic_category(), op);
}
void regular(int fd) {
    struct stat s{};
    if (::fstat(fd, &s))
        error("Inspect file");
    if (!S_ISREG(s.st_mode))
        throw std::runtime_error("Expected regular file");
}
} // namespace
File::~File() {
    if (handle_ >= 0)
        ::close(int(handle_));
}
File::File(File&& f) noexcept : handle_(std::exchange(f.handle_, -1)) {}
File& File::operator=(File&& f) noexcept {
    if (this != &f) {
        if (handle_ >= 0)
            ::close(int(handle_));
        handle_ = std::exchange(f.handle_, -1);
    }
    return *this;
}
File File::create(const std::filesystem::path& path) {
    File f(::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600));
    if (f.handle_ < 0)
        error("Create exclusive file");
    regular(int(f.handle_));
    return f;
}
File File::lock(const std::filesystem::path& path, bool wait, bool& busy) {
    busy = false;
    File f(::open(path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK, 0600));
    if (f.handle_ < 0)
        error("Open lock");
    regular(int(f.handle_));
    while (::flock(int(f.handle_), LOCK_EX | (wait ? 0 : LOCK_NB))) {
        if (errno == EINTR)
            continue;
        if (!wait && (errno == EWOULDBLOCK || errno == EAGAIN)) {
            busy = true;
            return {};
        }
        error("Lock file");
    }
    return f;
}
Read read(const std::filesystem::path& path, std::size_t limit, bool missingOkay) {
    File f(::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK));
    if (f.handle_ < 0) {
        if (missingOkay && errno == ENOENT)
            return {};
        error("Open file");
    }
    regular(int(f.handle_));
    struct stat s{};
    if (::fstat(int(f.handle_), &s))
        error("Inspect file size");
    if (s.st_size < 0 || std::uint64_t(s.st_size) > limit)
        throw std::length_error("File exceeds byte limit");
    Read out{true, std::string(std::size_t(s.st_size), '\0')};
    std::size_t at = 0;
    while (at < out.bytes.size()) {
        auto n = ::read(int(f.handle_), out.bytes.data() + at, out.bytes.size() - at);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            throw std::runtime_error("File changed during read");
        at += std::size_t(n);
    }
    char extra;
    for (;;) {
        auto n = ::read(int(f.handle_), &extra, 1);
        if (n < 0 && errno == EINTR)
            continue;
        if (n != 0)
            throw std::runtime_error("File changed during read");
        break;
    }
    return out;
}
void File::write(const std::string& bytes) {
    std::size_t at = 0;
    while (at < bytes.size()) {
        auto n = ::write(int(handle_), bytes.data() + at, bytes.size() - at);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            error("Write file");
        at += std::size_t(n);
    }
}
void File::flush() {
    if (::fsync(int(handle_)))
        error("Flush file");
}
void File::close() {
    const int fd = int(std::exchange(handle_, -1));
    if (::close(fd))
        error("Close file");
}
void replace(const std::filesystem::path& from, const std::filesystem::path& to, bool overwrite) {
    if (overwrite) {
        if (::rename(from.c_str(), to.c_str()))
            error("Replace file");
    } else {
        if (::link(from.c_str(), to.c_str()))
            error("Publish without overwrite");
        ::unlink(from.c_str());
    }
}
void confirm(const std::filesystem::path& path) {
    const int fd = ::open(path.parent_path().c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0)
        error("Open publication directory");
    const int r = ::fsync(fd), saved = errno;
    ::close(fd);
    errno = saved;
    if (r)
        error("Flush publication directory");
}
bool entropy(std::array<std::uint8_t, 16>& bytes) noexcept {
    const int fd = ::open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    if (fd < 0)
        return false;
    std::size_t at = 0;
    while (at < bytes.size()) {
        auto n = ::read(fd, bytes.data() + at, bytes.size() - at);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0) {
            ::close(fd);
            return false;
        }
        at += std::size_t(n);
    }
    ::close(fd);
    return true;
}
namespace {
std::filesystem::path directory(const char* key, const char* fallback) {
    if (const char* p = std::getenv(key); p && *p && std::filesystem::path(p).is_absolute())
        return std::filesystem::path(p) / "blobber";
    if (const char* p = std::getenv("HOME"); p && *p && std::filesystem::path(p).is_absolute())
        return std::filesystem::path(p) / fallback / "blobber";
    throw std::runtime_error("No absolute user directory");
}
} // namespace
std::filesystem::path dataDirectory() {
    return directory("XDG_DATA_HOME", ".local/share");
}
std::filesystem::path configDirectory() {
    return directory("XDG_CONFIG_HOME", ".config");
}
std::filesystem::path applicationDirectory() {
    return std::filesystem::read_symlink("/proc/self/exe").parent_path();
}
std::filesystem::path loadedModulePath(const char*) {
    return {};
}
bool reparsePoint(const std::filesystem::path& path) {
    return std::filesystem::is_symlink(std::filesystem::symlink_status(path));
}
} // namespace blobber::disk
