#include "merger.h"
#include <gdiplus.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>

using namespace ordered_pdf;
namespace {
void Require(bool condition, const char* reason) {
    if (!condition) throw std::runtime_error(reason);
}
std::string Read(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
void Fixture(const fs::path& path, const std::vector<int>& widths) {
    std::ofstream out(path, std::ios::binary);
    std::vector<std::streamoff> offsets(3 + widths.size() * 2);
    auto object = [&](size_t id) { offsets.at(id) = out.tellp(); out << id << " 0 obj\n"; };
    out << "%PDF-1.4\n";
    object(1); out << "<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
    object(2); out << "<< /Type /Pages /Count " << widths.size() << " /Kids [";
    for (size_t i = 0; i < widths.size(); ++i) out << 3 + i * 2 << " 0 R ";
    out << "] >>\nendobj\n";
    for (size_t i = 0; i < widths.size(); ++i) {
        object(3 + i * 2);
        out << "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 " << widths[i] << " 600] /Contents "
            << 4 + i * 2 << " 0 R >>\nendobj\n";
        object(4 + i * 2); out << "<< /Length 0 >>\nstream\nendstream\nendobj\n";
    }
    auto xref = out.tellp();
    out << "xref\n0 " << offsets.size() << "\n0000000000 65535 f \n";
    for (size_t i = 1; i < offsets.size(); ++i) out << std::setw(10) << std::setfill('0') << offsets[i] << " 00000 n \n";
    out << "trailer\n<< /Size " << offsets.size() << " /Root 1 0 R >>\nstartxref\n" << xref << "\n%%EOF\n";
    Require(static_cast<bool>(out), "fixture write failed");
}
template<class Action> void ExpectError(Action action) {
    bool failed = false;
    try { action(); } catch (const Error&) { failed = true; }
    Require(failed, "expected operation to reject input");
}

void RunTests(const fs::path& root) {
    const auto qpdf = FindQpdf();
    const auto settings = ExecutableDirectory() / L"settings.ini";
    if (!fs::exists(settings) && GetEnvironmentVariableW(L"LIBREOFFICE_PATH", nullptr, 0) == 0) {
        struct SettingsCleanup {
            fs::path path;
            ~SettingsCleanup() { std::error_code ignored; fs::remove(path, ignored); }
        } cleanup{settings};
        const auto configured = root / L"自定义目录 with spaces/soffice.exe";
        fs::create_directories(configured.parent_path());
        { std::ofstream file(configured); file << "path discovery fixture only"; }
        // Windows INI APIs require a Unicode BOM for non-ASCII paths.
        { std::ofstream file(settings, std::ios::binary); file.put('\xff'); file.put('\xfe'); }
        Require(WritePrivateProfileStringW(L"Dependencies", L"LibreOffice", configured.c_str(), settings.c_str()) != 0,
                "installer settings write failed");
        Require(FindLibreOffice() == configured, "installer-selected dependency path was not used");
    }
    auto a = root / L"中文 A.pdf", b = root / L"B with spaces.pdf", merged = root / L"merged.pdf";
    Fixture(a, {301, 302}); Fixture(b, {401, 402, 403});
    Merge({b, a, b}, merged);
    auto count = RunProcess(qpdf, {L"--show-npages", merged.wstring()}, root);
    Require(count.exitCode == 0 && count.output.find("8") != std::string::npos, "page count mismatch");
    auto qdf = root / L"ordered.qdf";
    Require(RunProcess(qpdf, {L"--qdf", L"--object-streams=disable", merged.wstring(), qdf.wstring()}, root).exitCode == 0, "qdf failed");
    const auto content = Read(qdf);
    const std::regex mediaBox(R"(/MediaBox\s*\[\s*0\s+0\s+(\d+))");
    std::vector<int> widths;
    for (auto it = std::sregex_iterator(content.begin(), content.end(), mediaBox); it != std::sregex_iterator(); ++it) widths.push_back(std::stoi((*it)[1]));
    Require(widths == std::vector<int>({401, 402, 403, 301, 302, 401, 402, 403}), "file/page order changed");
    const auto before = Read(merged);
    const auto inputBefore = Read(a);
    ExpectError([&] { Merge({a}, a); });
    Require(Read(a) == inputBefore, "input was overwritten");
    auto bad = root / L"bad.pdf";
    { std::ofstream stream(bad); stream << "not a pdf"; }
    ExpectError([&] { Merge({a, bad}, merged); });
    Require(Read(merged) == before, "failed merge changed existing output");
    ExpectError([&] { Merge({}, merged); });
    auto encrypted = root / L"encrypted.pdf";
    Require(RunProcess(qpdf, {L"--encrypt", L"secret", L"owner", L"256", L"--", a.wstring(), encrypted.wstring()}, root).exitCode == 0, "encryption fixture failed");
    ExpectError([&] { Merge({encrypted}, merged); });
    Require(Read(merged) == before, "encrypted input changed existing output");

    Gdiplus::Bitmap bitmap(160, 90, PixelFormat24bppRGB);
    { Gdiplus::Graphics graphics(&bitmap); graphics.Clear(Gdiplus::Color(255, 30, 100, 180)); }
    const CLSID png = {0x557cf406, 0x1a04, 0x11d3, {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}};
    auto image = root / L"蓝色图片.png";
    Require(bitmap.Save(image.c_str(), &png, nullptr) == Gdiplus::Ok, "PNG fixture failed");
    Merge({a, image, b}, merged);
    count = RunProcess(qpdf, {L"--show-npages", merged.wstring()}, root);
    Require(count.exitCode == 0 && count.output.find("6") != std::string::npos, "mixed page count mismatch");
    auto check = RunProcess(qpdf, {L"--check", merged.wstring()}, root);
    Require(check.exitCode == 0, "mixed output is invalid");
    Require(Lower(L"ABC.PDF") == L"abc.pdf" && IsSupported(L"file.DOCX") && !IsSupported(L"file.exe"), "extension handling failed");
    std::cout << "PASS: installer dependency path, whole-file order, internal page order, duplicates, Unicode paths, images, invalid/encrypted inputs, output preservation.\n";
}
} // namespace

int wmain(int argc, wchar_t** argv) {
    Gdiplus::GdiplusStartupInput startup;
    ULONG_PTR token = 0;
    if (Gdiplus::GdiplusStartup(&token, &startup, nullptr) != Gdiplus::Ok) return 1;
    int code = 0;
    try {
        if (argc >= 4 && std::wstring(argv[1]) == L"--merge") {
            std::vector<fs::path> inputs;
            for (int i = 3; i < argc; ++i) inputs.emplace_back(fs::absolute(argv[i]));
            Merge(inputs, fs::absolute(argv[2]));
        } else {
            auto root = fs::temp_directory_path() / (L"ordered-pdf-tests-" + std::to_wstring(GetCurrentProcessId()));
            fs::create_directories(root);
            try { RunTests(root); } catch (...) { std::error_code ignored; fs::remove_all(root, ignored); throw; }
            fs::remove_all(root);
        }
    } catch (const Error& error) { std::cerr << Utf8(error.message) << '\n'; code = 1; }
      catch (const std::exception& error) { std::cerr << error.what() << '\n'; code = 1; }
    Gdiplus::GdiplusShutdown(token);
    return code;
}
