#pragma once

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <objidl.h>
#include <filesystem>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace ordered_pdf {
namespace fs = std::filesystem;
using Progress = std::function<void(const std::wstring&)>;

struct Error : std::runtime_error {
    std::wstring message;
    explicit Error(std::wstring text);
};
struct ProcessResult {
    DWORD exitCode;
    std::string output;
};

std::wstring Lower(std::wstring text);
std::string Utf8(const std::wstring& text);
std::wstring Wide(const std::string& text);
fs::path ExecutableDirectory();
bool IsSupported(const fs::path& path);
fs::path FindQpdf();
fs::path FindLibreOffice();
ProcessResult RunProcess(const fs::path& exe, const std::vector<std::wstring>& args,
                         const fs::path& workdir, DWORD timeoutMs = 120000);
void ImageToPdf(const fs::path& input, const fs::path& output);
void Merge(const std::vector<fs::path>& inputs, const fs::path& output,
           const Progress& progress = {});
} // namespace ordered_pdf
