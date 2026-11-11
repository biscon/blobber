#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>

// Native operations at explicit I/O/preparation boundaries; no graphics dependency.
namespace blobber::disk {
struct Read {
    bool exists = false;
    std::string bytes;
};
class File {
  public:
    File() = default;
    ~File();
    File(File&&) noexcept;
    File& operator=(File&&) noexcept;
    File(const File&) = delete;
    File& operator=(const File&) = delete;
    static File create(const std::filesystem::path&);
    static File lock(const std::filesystem::path&, bool wait, bool& busy);
    void write(const std::string&);
    void flush();
    void close(); // checked close; destructor is best effort
  private:
    explicit File(std::intptr_t handle) : handle_(handle) {}
    std::intptr_t handle_ = -1;
    friend Read read(const std::filesystem::path&, std::size_t, bool);
    friend void confirm(const std::filesystem::path&);
    friend void replace(const std::filesystem::path&, const std::filesystem::path&, bool);
};
Read read(const std::filesystem::path&, std::size_t limit, bool missingOkay = false);
void replace(const std::filesystem::path& source, const std::filesystem::path& target, bool overwrite = true);
void confirm(const std::filesystem::path& published);
bool entropy(std::array<std::uint8_t, 16>&) noexcept;
std::filesystem::path suffix(std::filesystem::path, const char*);
class Temporary {
  public:
    explicit Temporary(std::filesystem::path, bool unique = false);
    ~Temporary();
    Temporary(const Temporary&) = delete;
    Temporary& operator=(const Temporary&) = delete;
    File file;
    void publish(const std::filesystem::path&, bool overwrite = true);

  private:
    std::filesystem::path path_;
    bool owned_ = false;
};
void writeDocument(const std::filesystem::path&, const std::string&);
std::filesystem::path dataDirectory();
std::filesystem::path configDirectory();
std::filesystem::path applicationDirectory();
std::filesystem::path loadedModulePath(const char* name); // Borrowed OS module query, diagnostics only.
} // namespace blobber::disk
