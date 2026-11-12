#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "platform/file_io.hpp"
#include <algorithm>
#include <cstdlib>
#include <system_error>
#include <utility>
#include <vector>
#include <windows.h>

// These SDK headers require the Win32 declarations above.
#include <bcrypt.h>
#include <shlobj.h>
namespace blobber::disk {
namespace {
HANDLE h(std::intptr_t value) {
    return reinterpret_cast<HANDLE>(value);
}
[[noreturn]] void error(const char* op) {
    throw std::system_error(int(GetLastError()), std::system_category(), op);
}
void regular(HANDLE f) {
    FILE_ATTRIBUTE_TAG_INFO info{};
    if (GetFileType(f) != FILE_TYPE_DISK ||
        !GetFileInformationByHandleEx(f, FileAttributeTagInfo, &info, sizeof(info)))
        throw std::runtime_error("Expected readable disk file");
    if (info.FileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT))
        throw std::runtime_error("Expected regular file without reparse point");
}
} // namespace
File::~File() {
    if (handle_ != -1)
        CloseHandle(h(handle_));
}
File::File(File&& f) noexcept : handle_(std::exchange(f.handle_, -1)) {}
File& File::operator=(File&& f) noexcept {
    if (this != &f) {
        if (handle_ != -1)
            CloseHandle(h(handle_));
        handle_ = std::exchange(f.handle_, -1);
    }
    return *this;
}
File File::create(const std::filesystem::path& path) {
    File f(reinterpret_cast<std::intptr_t>(CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                                                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT,
                                                       nullptr)));
    if (f.handle_ == -1) {
        if (GetLastError() == ERROR_FILE_EXISTS || GetLastError() == ERROR_ALREADY_EXISTS)
            throw std::system_error(std::make_error_code(std::errc::file_exists), "Create exclusive file");
        error("Create exclusive file");
    }
    regular(h(f.handle_));
    return f;
}
File File::lock(const std::filesystem::path& path, bool wait, bool& busy) {
    busy = false;
    File f(reinterpret_cast<std::intptr_t>(
        CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                    OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr)));
    if (f.handle_ == -1)
        error("Open lock");
    regular(h(f.handle_));
    OVERLAPPED overlap{};
    if (!LockFileEx(h(f.handle_), LOCKFILE_EXCLUSIVE_LOCK | (wait ? 0 : LOCKFILE_FAIL_IMMEDIATELY), 0, 1, 0,
                    &overlap)) {
        if (!wait && GetLastError() == ERROR_LOCK_VIOLATION) {
            busy = true;
            return {};
        }
        error("Lock file");
    }
    return f;
}
Read read(const std::filesystem::path& path, std::size_t limit, bool missingOkay) {
    File f(reinterpret_cast<std::intptr_t>(
        CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr)));
    if (f.handle_ == -1) {
        if (missingOkay && (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND))
            return {};
        error("Open file");
    }
    regular(h(f.handle_));
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(h(f.handle_), &size))
        error("Inspect file size");
    if (size.QuadPart < 0 || std::uint64_t(size.QuadPart) > limit)
        throw std::length_error("File exceeds byte limit");
    Read out{true, std::string(std::size_t(size.QuadPart), '\0')};
    std::size_t at = 0;
    while (at < out.bytes.size()) {
        DWORD n = 0;
        if (!ReadFile(h(f.handle_), out.bytes.data() + at,
                      DWORD(std::min<std::size_t>(out.bytes.size() - at, 1u << 20)), &n, nullptr))
            error("Read file");
        if (!n)
            throw std::runtime_error("File changed during read");
        at += n;
    }
    char extra;
    DWORD n = 0;
    if (!ReadFile(h(f.handle_), &extra, 1, &n, nullptr))
        error("Read file tail");
    if (n)
        throw std::runtime_error("File changed during read");
    return out;
}
void File::write(const std::string& bytes) {
    std::size_t at = 0;
    while (at < bytes.size()) {
        DWORD n = 0;
        if (!WriteFile(h(handle_), bytes.data() + at,
                       DWORD(std::min<std::size_t>(bytes.size() - at, 1u << 20)), &n, nullptr))
            error("Write file");
        if (!n)
            throw std::runtime_error("Short file write");
        at += n;
    }
}
void File::flush() {
    if (!FlushFileBuffers(h(handle_)))
        error("Flush file");
}
void File::close() {
    auto value = std::exchange(handle_, -1);
    if (!CloseHandle(h(value)))
        error("Close file");
}
void replace(const std::filesystem::path& from, const std::filesystem::path& to, bool overwrite) {
    // Refuse replacing links/devices, including a dangling reparse point.
    const auto status = std::filesystem::symlink_status(to);
    if (std::filesystem::exists(status)) {
        File checked(reinterpret_cast<std::intptr_t>(
            CreateFileW(to.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr)));
        if (checked.handle_ == -1)
            error("Inspect replacement target");
        regular(h(checked.handle_));
    }
    if (!MoveFileExW(from.c_str(), to.c_str(),
                     MOVEFILE_WRITE_THROUGH | (overwrite ? MOVEFILE_REPLACE_EXISTING : 0)))
        error("Publish file");
}
void confirm(const std::filesystem::path& path) {
    File f(reinterpret_cast<std::intptr_t>(
        CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr)));
    if (f.handle_ == -1)
        error("Open published file for confirmation");
    regular(h(f.handle_));
    f.flush();
    f.close();
}
bool entropy(std::array<std::uint8_t, 16>& bytes) noexcept {
    return BCryptGenRandom(nullptr, bytes.data(), ULONG(bytes.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0;
}
std::filesystem::path dataDirectory() {
    if (const auto* p = _wgetenv(L"LOCALAPPDATA"); p && *p && std::filesystem::path(p).is_absolute())
        return std::filesystem::path(p) / L"blobber";
    PWSTR value = nullptr;
    const auto result = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &value);
    std::filesystem::path path;
    try {
        if (SUCCEEDED(result) && value)
            path = std::filesystem::path(value) / L"blobber";
    } catch (...) {
        CoTaskMemFree(value);
        throw;
    }
    CoTaskMemFree(value);
    if (path.empty() || !path.is_absolute())
        throw std::runtime_error("No absolute LocalAppData directory");
    return path;
}
std::filesystem::path configDirectory() {
    return dataDirectory();
}
std::filesystem::path applicationDirectory() {
    std::vector<wchar_t> buffer(512);
    while (buffer.size() <= 32768) {
        const DWORD n = GetModuleFileNameW(nullptr, buffer.data(), DWORD(buffer.size()));
        if (!n)
            error("Find executable");
        if (n < buffer.size())
            return std::filesystem::path(std::wstring(buffer.data(), n)).parent_path();
        buffer.resize(buffer.size() * 2);
    }
    throw std::runtime_error("Executable path exceeds Windows limit");
}
std::filesystem::path loadedModulePath(const char* name) {
    auto module = GetModuleHandleW(std::filesystem::u8path(name).c_str());
    if (!module)
        return {};
    std::vector<wchar_t> buffer(32768);
    const auto n = GetModuleFileNameW(module, buffer.data(), DWORD(buffer.size()));
    if (!n || n >= buffer.size())
        error("Inspect loaded module path");
    return std::filesystem::path(std::wstring(buffer.data(), n));
}
bool reparsePoint(const std::filesystem::path& path) {
    const auto attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND)
            return false;
        error("Inspect reparse point");
    }
    return (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}
} // namespace blobber::disk
