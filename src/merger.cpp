#include "merger.h"

#include <gdiplus.h>
#include <objbase.h>
#include <algorithm>
#include <cwctype>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>

namespace ordered_pdf {
namespace {
struct Handle {
    HANDLE value = INVALID_HANDLE_VALUE;
    explicit Handle(HANDLE handle = INVALID_HANDLE_VALUE) : value(handle) {}
    ~Handle() { if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
};

std::wstring UniqueId() {
    GUID id{};
    if (FAILED(CoCreateGuid(&id))) throw Error(L"无法生成临时文件名。");
    wchar_t text[40]{};
    StringFromGUID2(id, text, 40);
    return text;
}

struct TemporaryDirectory {
    fs::path path;
    TemporaryDirectory() : path(fs::temp_directory_path() / (L"ordered-pdf-" + UniqueId())) {
        fs::create_directory(path);
    }
    ~TemporaryDirectory() { std::error_code ignored; fs::remove_all(path, ignored); }
};

struct TemporaryFile {
    fs::path path;
    ~TemporaryFile() { std::error_code ignored; fs::remove(path, ignored); }
};

std::wstring Environment(const wchar_t* key) {
    DWORD size = GetEnvironmentVariableW(key, nullptr, 0);
    if (!size) return {};
    std::wstring value(size, L'\0');
    DWORD written = GetEnvironmentVariableW(key, value.data(), size);
    if (!written || written >= size) return {};
    value.resize(written);
    return value;
}

// Follow CommandLineToArgvW / MS CRT quoting rules, including trailing slashes.
std::wstring Quote(const std::wstring& arg) {
    std::wstring result = L"\"";
    size_t slashes = 0;
    for (wchar_t c : arg) {
        if (c == L'\\') { ++slashes; continue; }
        result.append(c == L'"' ? slashes * 2 + 1 : slashes, L'\\');
        result += c;
        slashes = 0;
    }
    result.append(slashes * 2, L'\\');
    return result + L'"';
}

std::string ReadBytes(const fs::path& path) {
    std::ifstream stream(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

void WriteText(const fs::path& path, const std::string& text) {
    std::ofstream stream(path, std::ios::binary);
    stream.write(text.data(), static_cast<std::streamsize>(text.size()));
    if (!stream) throw Error(L"写入临时文件失败，请检查磁盘空间。");
}

std::wstring FileUri(const fs::path& path) {
    const std::string bytes = Utf8(fs::absolute(path).generic_wstring());
    const char* hex = "0123456789ABCDEF";
    std::string uri = "file:///";
    for (unsigned char c : bytes) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '/' || c == ':' || c == '-' || c == '_' || c == '.') {
            uri += static_cast<char>(c);
        } else {
            uri += '%'; uri += hex[c >> 4]; uri += hex[c & 15];
        }
    }
    return Wide(uri);
}

fs::path OfficeToPdf(const fs::path& source, const fs::path& folder, const fs::path& office) {
    fs::create_directories(folder / L"profile/user");
    WriteText(folder / L"profile/user/registrymodifications.xcu",
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<oor:items xmlns:oor=\"http://openoffice.org/2001/registry\">"
        "<item oor:path=\"/org.openoffice.Office.Common/Security/Scripting\">"
        "<prop oor:name=\"MacroSecurityLevel\" oor:op=\"fuse\"><value>3</value></prop>"
        "</item><item oor:path=\"/org.openoffice.Office.Calc/Content/Update\">"
        "<prop oor:name=\"Link\" oor:op=\"fuse\"><value>0</value></prop>"
        "</item></oor:items>");
    auto result = RunProcess(office, {
        L"-env:UserInstallation=" + FileUri(folder / L"profile"),
        L"--headless", L"--nologo", L"--nodefault", L"--nofirststartwizard",
        L"--convert-to", L"pdf", L"--outdir", folder.wstring(), source.wstring()
    }, folder);
    fs::path pdf = folder / (source.stem().wstring() + L".pdf");
    if (result.exitCode != 0 || !fs::is_regular_file(pdf) || !fs::file_size(pdf)) {
        throw Error(L"Office 转换失败。请检查文件、密码保护和 LibreOffice 安装。\n" + Wide(result.output));
    }
    return pdf;
}

struct ImagePage { fs::path jpeg; UINT width; UINT height; };

void WriteImagePdf(const std::vector<ImagePage>& pages, const fs::path& output) {
    std::ofstream stream(output, std::ios::binary);
    if (!stream) throw Error(L"无法创建图片 PDF。");
    stream.imbue(std::locale::classic());
    stream << "%PDF-1.4\n%\xE2\xE3\xCF\xD3\n";
    std::vector<std::streamoff> offsets(3 + pages.size() * 3, 0);
    auto start = [&](size_t object) {
        offsets.at(object) = stream.tellp();
        stream << object << " 0 obj\n";
    };
    start(1); stream << "<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
    start(2); stream << "<< /Type /Pages /Count " << pages.size() << " /Kids [";
    for (size_t i = 0; i < pages.size(); ++i) stream << 3 + i * 3 << " 0 R ";
    stream << "] >>\nendobj\n";
    for (size_t i = 0; i < pages.size(); ++i) {
        const auto& page = pages[i];
        const size_t object = 3 + i * 3;
        // Use one standard portrait sheet for every image, including wide frames.
        constexpr double pw = 595.28;
        constexpr double ph = 841.89;
        const double scale = (std::min)((pw - 36) / page.width, (ph - 36) / page.height);
        const double w = page.width * scale, h = page.height * scale;
        std::ostringstream drawing;
        drawing.imbue(std::locale::classic());
        drawing << "q " << w << " 0 0 " << h << " " << (pw - w) / 2 << " " << (ph - h) / 2 << " cm /Im0 Do Q\n";
        const auto content = drawing.str();
        start(object);
        stream << "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 " << pw << " " << ph
               << "] /Resources << /XObject << /Im0 " << object + 2 << " 0 R >> >> /Contents "
               << object + 1 << " 0 R >>\nendobj\n";
        start(object + 1);
        stream << "<< /Length " << content.size() << " >>\nstream\n" << content << "endstream\nendobj\n";
        start(object + 2);
        stream << "<< /Type /XObject /Subtype /Image /Width " << page.width << " /Height " << page.height
               << " /ColorSpace /DeviceRGB /BitsPerComponent 8 /Filter /DCTDecode /Length "
               << fs::file_size(page.jpeg) << " >>\nstream\n";
        std::ifstream jpeg(page.jpeg, std::ios::binary);
        stream << jpeg.rdbuf();
        stream << "\nendstream\nendobj\n";
    }
    const auto xref = stream.tellp();
    stream << "xref\n0 " << offsets.size() << "\n0000000000 65535 f \n";
    for (size_t i = 1; i < offsets.size(); ++i) stream << std::setw(10) << std::setfill('0') << offsets[i] << " 00000 n \n";
    stream << "trailer\n<< /Size " << offsets.size() << " /Root 1 0 R >>\nstartxref\n" << xref << "\n%%EOF\n";
    stream.close();
    if (!stream) throw Error(L"保存图片 PDF 失败，请检查磁盘空间。");
}
} // namespace

Error::Error(std::wstring text) : std::runtime_error("ordered-pdf error"), message(std::move(text)) {}
std::wstring Lower(std::wstring text) {
    std::transform(text.begin(), text.end(), text.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
    return text;
}
std::string Utf8(const std::wstring& text) {
    if (text.empty()) return {};
    int length = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    std::string result(length, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), length, nullptr, nullptr);
    return result;
}
std::wstring Wide(const std::string& text) {
    if (text.empty()) return {};
    int length = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring result(length, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), length);
    return result;
}
fs::path ExecutableDirectory() {
    std::wstring path(32768, L'\0');
    DWORD size = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (!size || size >= path.size()) throw Error(L"无法确定程序路径。");
    path.resize(size);
    return fs::path(path).parent_path();
}
bool IsSupported(const fs::path& path) {
    static const std::vector<std::wstring> types = {
        L".pdf", L".doc", L".docx", L".xls", L".xlsx", L".jpg", L".jpeg", L".png", L".bmp", L".tif", L".tiff"
    };
    auto ext = Lower(path.extension().wstring());
    return std::find(types.begin(), types.end(), ext) != types.end();
}
fs::path FindQpdf() {
    auto custom = Environment(L"QPDF_PATH");
    if (!custom.empty()) {
        if (!fs::is_regular_file(custom)) throw Error(L"QPDF_PATH 指定的程序不存在。");
        return custom;
    }
    for (auto path : {ExecutableDirectory() / L"tools/qpdf/bin/qpdf.exe", ExecutableDirectory() / L"qpdf.exe"}) {
        if (fs::is_regular_file(path)) return path;
    }
    throw Error(L"缺少 qpdf.exe。请下载完整便携包，或设置 QPDF_PATH。");
}
fs::path FindLibreOffice() {
    auto custom = Environment(L"LIBREOFFICE_PATH");
    if (!custom.empty()) {
        if (!fs::is_regular_file(custom)) throw Error(L"LIBREOFFICE_PATH 指定的程序不存在。");
        return custom;
    }
    // Installer stores the selected existing or newly installed program path.
    std::wstring configured(32768, L'\0');
    DWORD length = GetPrivateProfileStringW(L"Dependencies", L"LibreOffice", L"",
        configured.data(), static_cast<DWORD>(configured.size()),
        (ExecutableDirectory() / L"settings.ini").c_str());
    configured.resize(length);
    if (!configured.empty() && fs::is_regular_file(configured)) return configured;
    for (auto key : {L"ProgramFiles", L"ProgramFiles(x86)", L"LOCALAPPDATA"}) {
        auto base = Environment(key);
        if (base.empty()) continue;
        auto path = fs::path(base) / L"LibreOffice/program/soffice.exe";
        if (fs::is_regular_file(path)) return path;
    }
    throw Error(L"Word / Excel 转换需要 LibreOffice。\n请重新运行安装向导配置依赖，或设置 LIBREOFFICE_PATH 指向 soffice.exe。");
}

ProcessResult RunProcess(const fs::path& exe, const std::vector<std::wstring>& args,
                         const fs::path& workdir, DWORD timeoutMs) {
    fs::path log = workdir / (L"process-" + UniqueId() + L".log");
    TemporaryFile cleanup{log};
    SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    Handle output(CreateFileW(log.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                              &security, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr));
    Handle input(CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                            &security, OPEN_EXISTING, 0, nullptr));
    Handle job(CreateJobObjectW(nullptr, nullptr));
    if (output.value == INVALID_HANDLE_VALUE || input.value == INVALID_HANDLE_VALUE || !job.value) {
        throw Error(L"无法创建转换进程的输入输出。");
    }
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(job.value, JobObjectExtendedLimitInformation, &limits, sizeof(limits))) {
        throw Error(L"无法设置转换进程资源管理。");
    }
    std::wstring command = Quote(exe.wstring());
    for (const auto& arg : args) command += L" " + Quote(arg);
    if (command.size() >= 32767) throw Error(L"文件路径总长度过大，请减少本次文件数。");
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = input.value;
    startup.hStdOutput = startup.hStdError = output.value;
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(exe.c_str(), command.data(), nullptr, nullptr, TRUE,
                        CREATE_NO_WINDOW | CREATE_SUSPENDED, nullptr, workdir.c_str(), &startup, &process)) {
        throw Error(L"无法启动：" + exe.wstring() + L"\nWindows 错误码：" + std::to_wstring(GetLastError()));
    }
    Handle processHandle(process.hProcess), threadHandle(process.hThread);
    if (!AssignProcessToJobObject(job.value, process.hProcess)) {
        TerminateProcess(process.hProcess, 1);
        WaitForSingleObject(process.hProcess, 5000);
        throw Error(L"无法管理转换进程，请检查系统的作业对象限制。");
    }
    if (ResumeThread(process.hThread) == static_cast<DWORD>(-1)) throw Error(L"无法启动转换线程。");
    if (WaitForSingleObject(process.hProcess, timeoutMs) != WAIT_OBJECT_0) {
        TerminateJobObject(job.value, 1);
        WaitForSingleObject(process.hProcess, 5000);
        throw Error(L"转换超时，已停止子进程。请检查文件，或先手动导出 PDF。");
    }
    DWORD code = 1;
    GetExitCodeProcess(process.hProcess, &code);
    return {code, ReadBytes(log)};
}

void ImageToPdf(const fs::path& input, const fs::path& output) {
    TemporaryDirectory temporary;
    std::unique_ptr<Gdiplus::Bitmap> original(Gdiplus::Bitmap::FromFile(input.c_str()));
    if (!original || original->GetLastStatus() != Gdiplus::Ok) throw Error(L"图片无法读取或格式不匹配。");
    UINT dimensions = original->GetFrameDimensionsCount();
    if (!dimensions) throw Error(L"图片不包含有效图像帧。");
    std::vector<GUID> dimensionIds(dimensions);
    if (original->GetFrameDimensionsList(dimensionIds.data(), dimensions) != Gdiplus::Ok) throw Error(L"无法读取图片帧。");
    GUID dimension = dimensionIds.front();
    for (const auto& id : dimensionIds) if (IsEqualGUID(id, Gdiplus::FrameDimensionPage)) dimension = id;
    UINT count = original->GetFrameCount(&dimension);
    if (!count || count > 1000) throw Error(L"图片帧数不受支持（最多 1000 帧）。");
    UINT encoderCount = 0, encoderSize = 0;
    Gdiplus::GetImageEncodersSize(&encoderCount, &encoderSize);
    std::vector<BYTE> encoderData(encoderSize);
    auto encoders = reinterpret_cast<Gdiplus::ImageCodecInfo*>(encoderData.data());
    if (!encoderSize || Gdiplus::GetImageEncoders(encoderCount, encoderSize, encoders) != Gdiplus::Ok) throw Error(L"系统缺少图片编码器。");
    CLSID jpegEncoder{};
    bool found = false;
    for (UINT i = 0; i < encoderCount; ++i) if (wcscmp(encoders[i].MimeType, L"image/jpeg") == 0) {
        jpegEncoder = encoders[i].Clsid; found = true; break;
    }
    if (!found) throw Error(L"系统缺少 JPEG 编码器。");
    std::vector<ImagePage> pages;
    for (UINT i = 0; i < count; ++i) {
        if (original->SelectActiveFrame(&dimension, i) != Gdiplus::Ok) throw Error(L"无法读取图片页面。");
        UINT w = original->GetWidth(), h = original->GetHeight();
        if (!w || !h || static_cast<uint64_t>(w) * h > 40000000) throw Error(L"图片尺寸超限：每帧最多 4000 万像素。");
        auto frame = std::make_unique<Gdiplus::Bitmap>(w, h, PixelFormat32bppARGB);
        if (!frame || frame->GetLastStatus() != Gdiplus::Ok) throw Error(L"无法解码图片。");
        {
            Gdiplus::Graphics graphics(frame.get());
            graphics.Clear(Gdiplus::Color(0, 255, 255, 255));
            if (graphics.DrawImage(original.get(), 0, 0, static_cast<INT>(w), static_cast<INT>(h)) != Gdiplus::Ok) throw Error(L"无法绘制图片帧。");
        }
        const UINT propertySize = original->GetPropertyItemSize(PropertyTagOrientation);
        if (propertySize) {
            std::vector<BYTE> data(propertySize);
            auto property = reinterpret_cast<Gdiplus::PropertyItem*>(data.data());
            if (original->GetPropertyItem(PropertyTagOrientation, propertySize, property) == Gdiplus::Ok && property->length >= 2) {
                auto orientation = *static_cast<USHORT*>(property->value);
                const Gdiplus::RotateFlipType rotations[] = {
                    Gdiplus::RotateNoneFlipNone, Gdiplus::RotateNoneFlipNone, Gdiplus::RotateNoneFlipX,
                    Gdiplus::Rotate180FlipNone, Gdiplus::Rotate180FlipX, Gdiplus::Rotate90FlipX,
                    Gdiplus::Rotate90FlipNone, Gdiplus::Rotate270FlipX, Gdiplus::Rotate270FlipNone
                };
                if (orientation <= 8 && frame->RotateFlip(rotations[orientation]) != Gdiplus::Ok) throw Error(L"图片方向转换失败。");
            }
        }
        w = frame->GetWidth(); h = frame->GetHeight();
        const double ratio = (std::min)(1.0, 3508.0 / (std::max)(w, h));
        UINT targetW = (std::max)(1u, static_cast<UINT>(w * ratio));
        UINT targetH = (std::max)(1u, static_cast<UINT>(h * ratio));
        Gdiplus::Bitmap flattened(targetW, targetH, PixelFormat24bppRGB);
        {
            Gdiplus::Graphics graphics(&flattened);
            graphics.Clear(Gdiplus::Color::White);
            graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            if (graphics.DrawImage(frame.get(), 0, 0, static_cast<INT>(targetW), static_cast<INT>(targetH)) != Gdiplus::Ok) throw Error(L"图片绘制失败。");
        }
        ULONG quality = 95;
        Gdiplus::EncoderParameters parameters{};
        parameters.Count = 1;
        parameters.Parameter[0] = {Gdiplus::EncoderQuality, 1, Gdiplus::EncoderParameterValueTypeLong, &quality};
        fs::path jpeg = temporary.path / (std::to_wstring(i) + L".jpg");
        if (flattened.Save(jpeg.c_str(), &jpegEncoder, &parameters) != Gdiplus::Ok) throw Error(L"图片编码失败。");
        pages.push_back({jpeg, targetW, targetH});
    }
    WriteImagePdf(pages, output);
}

void Merge(const std::vector<fs::path>& inputs, const fs::path& output, const Progress& progress) {
    if (inputs.empty() || inputs.size() > 100) throw Error(L"请选择 1 至 100 个文件。");
    uint64_t total = 0;
    bool needsOffice = false;
    for (const auto& input : inputs) {
        if (!IsSupported(input)) throw Error(L"不支持的文件类型：" + input.filename().wstring());
        if (!fs::is_regular_file(input) || !fs::file_size(input)) throw Error(L"文件不存在或为空：" + input.wstring());
        if (Lower(fs::weakly_canonical(input).wstring()) == Lower(fs::weakly_canonical(output).wstring()) ||
            (fs::exists(output) && fs::equivalent(input, output))) throw Error(L"输出文件不能覆盖任何输入文件。");
        total += fs::file_size(input);
        auto ext = Lower(input.extension().wstring());
        needsOffice = needsOffice || ext == L".doc" || ext == L".docx" || ext == L".xls" || ext == L".xlsx";
    }
    if (total > 200ull * 1024 * 1024) throw Error(L"本次输入文件总大小超过 200 MB。");
    const auto qpdf = FindQpdf();
    const auto office = needsOffice ? FindLibreOffice() : fs::path{};
    TemporaryDirectory temporary;
    // Copy the complete ordered batch first; conversions never touch originals.
    std::vector<fs::path> sources, pdfs;
    for (size_t i = 0; i < inputs.size(); ++i) {
        auto source = temporary.path / (L"input-" + std::to_wstring(i) + Lower(inputs[i].extension().wstring()));
        fs::copy_file(inputs[i], source);
        sources.push_back(source);
    }
    for (size_t i = 0; i < sources.size(); ++i) {
        if (progress) progress(L"正在处理 " + std::to_wstring(i + 1) + L" / " + std::to_wstring(inputs.size()) + L"：" + inputs[i].filename().wstring());
        try {
            const auto ext = Lower(sources[i].extension().wstring());
            if (ext == L".pdf") pdfs.push_back(sources[i]);
            else if (ext == L".doc" || ext == L".docx" || ext == L".xls" || ext == L".xlsx") {
                pdfs.push_back(OfficeToPdf(sources[i], temporary.path / (L"office-" + std::to_wstring(i)), office));
            } else {
                auto pdf = temporary.path / (L"image-" + std::to_wstring(i) + L".pdf");
                ImageToPdf(sources[i], pdf);
                pdfs.push_back(pdf);
            }
        } catch (const Error& error) {
            throw Error(inputs[i].filename().wstring() + L"：\n" + error.message);
        }
    }
    if (progress) progress(L"正在按列表顺序合并全部页面…");
    // Work beside the destination so the final rename stays on the same volume.
    TemporaryFile staging{output.parent_path() / (L".ordered-pdf-" + UniqueId() + L".pdf")};
    std::vector<std::wstring> args{L"--empty", L"--pages"};
    for (const auto& pdf : pdfs) { args.push_back(pdf.wstring()); args.push_back(L"1-z"); }
    args.push_back(L"--"); args.push_back(staging.path.wstring());
    const auto result = RunProcess(qpdf, args, temporary.path);
    // qpdf exit 3 means recoverable warnings. Reject it rather than silently
    // delivering a repaired file whose page content could differ from the input.
    if (result.exitCode != 0 || !fs::is_regular_file(staging.path)) {
        throw Error(L"PDF 合并失败；请检查损坏或加密文件。原有输出文件未修改。\n" + Wide(result.output));
    }
    auto check = RunProcess(qpdf, {L"--check", staging.path.wstring()}, temporary.path);
    if (check.exitCode != 0) throw Error(L"输出 PDF 校验失败。\n" + Wide(check.output));
    if (!MoveFileExW(staging.path.c_str(), output.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        throw Error(L"无法保存结果，请检查目标目录权限，或关闭已打开的同名 PDF。");
    }
}
} // namespace ordered_pdf
