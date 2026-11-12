#include "app/application.hpp"
#include <exception>
#include <iostream>
namespace {
int launch(int argc, char** argv) {
    try {
        return blobber::run(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "Blobber: " << error.what() << '\n';
        return 1;
    }
}
} // namespace

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <string>
#include <vector>
#include <windows.h>
int wmain(int argc, wchar_t** argv) {
    std::vector<std::string> text;
    std::vector<char*> args;
    text.reserve(argc);
    args.reserve(std::size_t(argc) + 1);
    for (int i = 0; i < argc; ++i) {
        const int n =
            WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, argv[i], -1, nullptr, 0, nullptr, nullptr);
        if (!n)
            return 1;
        text.emplace_back(std::size_t(n), '\0');
        if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, argv[i], -1, text.back().data(), n, nullptr,
                                 nullptr))
            return 1;
    }
    for (auto& value : text)
        args.push_back(value.data());
    args.push_back(nullptr);
    return launch(argc, args.data());
}
#else
int main(int argc, char** argv) {
    return launch(argc, argv);
}
#endif
