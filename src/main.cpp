#include "merger.h"
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <gdiplus.h>
#include <thread>
#include <memory>

using namespace ordered_pdf;
namespace {
constexpr UINT ProgressMessage = WM_APP + 1;
constexpr UINT FinishedMessage = WM_APP + 2;
enum Control { Add = 100, Up, Down, Remove, Clear, MergeNow, List, Status, RecentLink, SaveAs, DeleteRecent, RecentLabel };
struct Completion { bool success; std::wstring message; };
struct State {
    HWND window{}, list{}, status{};
    HWND recentLink{};
    HFONT font{};
    std::vector<fs::path> files;
    std::thread worker;
    bool busy = false;
    int scale = 96;
    fs::path recentPdf;
} app;

int Px(int value) { return MulDiv(value, app.scale, 96); }
void ShowError(const std::wstring& text) { MessageBoxW(app.window, text.c_str(), L"PDF Merger", MB_OK | MB_ICONWARNING); }
int Selected() { return ListView_GetNextItem(app.list, -1, LVNI_SELECTED); }

bool WritableDirectory(const fs::path& directory) {
    std::error_code error;
    fs::create_directories(directory, error);
    if (error) return false;
    const auto probe = directory / (L".ordered-pdf-write-test-" + std::to_wstring(GetCurrentProcessId()));
    HANDLE file = CreateFileW(probe.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                              FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    CloseHandle(file);
    return true;
}

fs::path RecentPdfPath() {
    const auto installed = ExecutableDirectory() / L"最近合并.pdf";
    bool existingWritable = false;
    if (fs::exists(installed)) {
        HANDLE file = CreateFileW(installed.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                  nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        existingWritable = file != INVALID_HANDLE_VALUE;
        if (existingWritable) CloseHandle(file);
    }
    if (existingWritable || (!fs::exists(installed) && WritableDirectory(ExecutableDirectory()))) return installed;
    PWSTR localAppData = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &localAppData))) {
        const fs::path root(localAppData);
        CoTaskMemFree(localAppData);
        const auto previous = root / L"Ordered PDF Merger" / L"最近合并.pdf";
        if (fs::is_regular_file(previous)) return previous;
        const auto directory = root / L"PDF Merger";
        if (WritableDirectory(directory)) return directory / L"最近合并.pdf";
    }
    throw Error(L"无法创建合并文档保存目录。请检查用户文件夹权限和磁盘空间。");
}

void UpdateRecentDocument() {
    const bool exists = !app.recentPdf.empty() && fs::is_regular_file(app.recentPdf);
    SetWindowTextW(app.recentLink, exists ? app.recentPdf.c_str() : L"尚无合并文档");
    EnableWindow(app.recentLink, exists);
    EnableWindow(GetDlgItem(app.window, SaveAs), exists && !app.busy);
    EnableWindow(GetDlgItem(app.window, DeleteRecent), exists && !app.busy);
}

void UpdateButtons() {
    const int selected = Selected();
    for (auto id : {Add, Clear, MergeNow}) EnableWindow(GetDlgItem(app.window, id), !app.busy && (id == Add || !app.files.empty()));
    EnableWindow(GetDlgItem(app.window, Up), !app.busy && selected > 0);
    EnableWindow(GetDlgItem(app.window, Down), !app.busy && selected >= 0 && selected + 1 < static_cast<int>(app.files.size()));
    EnableWindow(GetDlgItem(app.window, Remove), !app.busy && selected >= 0);
    SetWindowTextW(GetDlgItem(app.window, MergeNow), app.busy ? L"正在处理…" : L"合并并保留 PDF");
    UpdateRecentDocument();
}

void Refresh(int select = -1) {
    ListView_DeleteAllItems(app.list);
    for (size_t i = 0; i < app.files.size(); ++i) {
        std::wstring number = std::to_wstring(i + 1);
        LVITEMW item{};
        item.mask = LVIF_TEXT; item.iItem = static_cast<int>(i); item.pszText = number.data();
        ListView_InsertItem(app.list, &item);
        auto name = app.files[i].filename().wstring();
        auto path = app.files[i].wstring();
        ListView_SetItemText(app.list, static_cast<int>(i), 1, name.data());
        ListView_SetItemText(app.list, static_cast<int>(i), 2, path.data());
    }
    if (select >= 0 && select < static_cast<int>(app.files.size())) {
        ListView_SetItemState(app.list, select, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(app.list, select, FALSE);
    }
    auto text = L"已添加 " + std::to_wstring(app.files.size()) + L" 个文件。每份文件的全部页面连续保留；可重复添加同一文件。";
    SetWindowTextW(app.status, text.c_str());
    UpdateButtons();
}

void AddFiles(const std::vector<fs::path>& files) {
    if (app.busy || files.empty()) return;
    try {
        if (app.files.size() + files.size() > 100) throw Error(L"最多添加 100 个文件。");
        uint64_t bytes = 0;
        for (const auto& path : app.files) bytes += fs::file_size(path);
        for (const auto& path : files) {
            if (!IsSupported(path)) throw Error(L"本批文件未添加。不支持：" + path.filename().wstring());
            if (!fs::is_regular_file(path) || !fs::file_size(path)) throw Error(L"本批文件未添加。文件不存在或为空：" + path.wstring());
            bytes += fs::file_size(path);
        }
        if (bytes > 200ull * 1024 * 1024) throw Error(L"文件总大小不能超过 200 MB。");
        int first = static_cast<int>(app.files.size());
        app.files.insert(app.files.end(), files.begin(), files.end());
        Refresh(first);
    } catch (const Error& error) { ShowError(error.message); }
      catch (const std::exception&) { ShowError(L"读取文件失败，请检查文件路径和权限。"); }
}

void Browse() {
    std::vector<wchar_t> buffer(65536, L'\0');
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog); dialog.hwndOwner = app.window;
    dialog.lpstrFilter = L"支持的文件\0*.pdf;*.doc;*.docx;*.xls;*.xlsx;*.jpg;*.jpeg;*.png;*.bmp;*.tif;*.tiff\0PDF\0*.pdf\0Word / Excel\0*.doc;*.docx;*.xls;*.xlsx\0图片\0*.jpg;*.jpeg;*.png;*.bmp;*.tif;*.tiff\0\0";
    dialog.lpstrFile = buffer.data(); dialog.nMaxFile = static_cast<DWORD>(buffer.size());
    dialog.lpstrTitle = L"添加文件：顺序以返回列表为准，可在主窗口调整";
    dialog.Flags = OFN_EXPLORER | OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&dialog)) {
        if (CommDlgExtendedError()) ShowError(L"文件选择失败。请减少一次选择的文件数。");
        return;
    }
    fs::path first(buffer.data());
    const wchar_t* next = buffer.data() + wcslen(buffer.data()) + 1;
    std::vector<fs::path> files;
    if (!*next) files.push_back(first);
    else while (*next) { files.push_back(first / next); next += wcslen(next) + 1; }
    AddFiles(files);
}

void SaveRecentAs() {
    if (app.recentPdf.empty() || !fs::is_regular_file(app.recentPdf)) return;
    std::vector<wchar_t> buffer(32768, L'\0');
    const std::wstring defaultName = L"merged.pdf";
    std::copy(defaultName.begin(), defaultName.end(), buffer.begin());
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog); dialog.hwndOwner = app.window;
    dialog.lpstrFilter = L"PDF 文件\0*.pdf\0\0"; dialog.lpstrDefExt = L"pdf";
    dialog.lpstrFile = buffer.data(); dialog.nMaxFile = static_cast<DWORD>(buffer.size());
    dialog.lpstrTitle = L"另存合并文档";
    dialog.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetSaveFileNameW(&dialog)) return;
    fs::path destination(buffer.data());
    if (Lower(destination.extension().wstring()) != L".pdf") destination += L".pdf";
    if (!CopyFileW(app.recentPdf.c_str(), destination.c_str(), FALSE))
        ShowError(L"无法保存副本，请检查目标目录权限或关闭已打开的同名 PDF。\nWindows 错误码：" + std::to_wstring(GetLastError()));
}

void DeleteRecentPdf() {
    if (app.recentPdf.empty() || !fs::is_regular_file(app.recentPdf)) return;
    if (MessageBoxW(app.window, (L"删除最近合并文档？\n" + app.recentPdf.wstring()).c_str(),
                    L"PDF Merger", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES) return;
    if (!DeleteFileW(app.recentPdf.c_str())) {
        ShowError(L"删除失败。请先关闭 PDF 阅读器中的该文件。\nWindows 错误码：" + std::to_wstring(GetLastError()));
        return;
    }
    UpdateRecentDocument();
}

void MergePdf() {
    if (app.busy || app.files.empty()) return;
    const auto inputs = app.files;
    fs::path output;
    try { output = RecentPdfPath(); }
    catch (const Error& error) { ShowError(error.message); return; }
    app.recentPdf = output;
    app.busy = true;
    UpdateButtons();
    SetWindowTextW(app.status, L"正在准备文件…");
    try {
        app.worker = std::thread([inputs, output, window = app.window] {
            auto done = std::make_unique<Completion>();
            try {
                Merge(inputs, output, [window](const std::wstring& text) {
                    auto update = std::make_unique<std::wstring>(text);
                    if (PostMessageW(window, ProgressMessage, 0, reinterpret_cast<LPARAM>(update.get()))) update.release();
                });
                done->success = true;
                done->message = L"合并完成，文档已保留在：\n" + output.wstring() + L"\n\n可从主界面打开、另存或删除。";
            } catch (const Error& error) {
                done->success = false; done->message = error.message.substr(0, 6000);
            } catch (const std::exception& error) {
                done->success = false; done->message = L"处理失败：\n" + Wide(error.what());
            } catch (...) {
                done->success = false; done->message = L"处理失败，未保存结果。";
            }
            if (PostMessageW(window, FinishedMessage, 0, reinterpret_cast<LPARAM>(done.get()))) done.release();
        });
    } catch (const std::exception&) {
        app.busy = false; UpdateButtons(); ShowError(L"无法启动后台线程。");
    }
}

void Layout() {
    RECT client{}; GetClientRect(app.window, &client);
    const int width = client.right, height = client.bottom;
    const int margin = Px(20), buttonW = Px(105), buttonH = Px(32), gap = Px(9);
    int x = margin;
    for (auto id : {Add, Up, Down, Remove, Clear}) {
        MoveWindow(GetDlgItem(app.window, id), x, Px(18), buttonW, buttonH, TRUE);
        x += buttonW + gap;
    }
    MoveWindow(app.list, margin, Px(64), width - margin * 2, (std::max)(Px(80), height - Px(217)), TRUE);
    ListView_SetColumnWidth(app.list, 0, Px(55));
    ListView_SetColumnWidth(app.list, 1, Px(240));
    ListView_SetColumnWidth(app.list, 2, (std::max)(Px(180), width - Px(350)));
    MoveWindow(app.status, margin, height - Px(142), width - margin * 2, Px(30), TRUE);
    MoveWindow(GetDlgItem(app.window, RecentLabel), margin, height - Px(91), Px(120), Px(28), TRUE);
    MoveWindow(app.recentLink, margin + Px(120), height - Px(91), width - Px(560), Px(28), TRUE);
    MoveWindow(GetDlgItem(app.window, SaveAs), width - Px(420), height - Px(96), Px(110), Px(38), TRUE);
    MoveWindow(GetDlgItem(app.window, DeleteRecent), width - Px(300), height - Px(96), Px(85), Px(38), TRUE);
    MoveWindow(GetDlgItem(app.window, MergeNow), width - Px(205), height - Px(96), Px(185), Px(38), TRUE);
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        app.window = window;
        HDC dc = GetDC(window); app.scale = GetDeviceCaps(dc, LOGPIXELSX); ReleaseDC(window, dc);
        app.font = CreateFontW(-MulDiv(10, app.scale, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        auto control = [window](const wchar_t* cls, const wchar_t* text, DWORD style, int id) {
            HWND child = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style, 0, 0, 0, 0,
                                          window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
            SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(app.font), TRUE);
            return child;
        };
        control(L"BUTTON", L"添加文件", WS_TABSTOP | BS_PUSHBUTTON, Add);
        control(L"BUTTON", L"上移", WS_TABSTOP | BS_PUSHBUTTON, Up);
        control(L"BUTTON", L"下移", WS_TABSTOP | BS_PUSHBUTTON, Down);
        control(L"BUTTON", L"移除", WS_TABSTOP | BS_PUSHBUTTON, Remove);
        control(L"BUTTON", L"清空", WS_TABSTOP | BS_PUSHBUTTON, Clear);
        control(L"BUTTON", L"合并并保留 PDF", WS_TABSTOP | BS_DEFPUSHBUTTON, MergeNow);
        app.list = control(WC_LISTVIEWW, L"", WS_TABSTOP | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, List);
        ListView_SetExtendedListViewStyle(app.list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);
        for (int i = 0; i < 3; ++i) {
            const wchar_t* titles[] = {L"顺序", L"文件名", L"完整路径"};
            LVCOLUMNW column{}; column.mask = LVCF_TEXT | LVCF_WIDTH; column.cx = Px(150);
            column.pszText = const_cast<wchar_t*>(titles[i]); ListView_InsertColumn(app.list, i, &column);
        }
        app.status = control(L"STATIC", L"", 0, Status);
        control(L"STATIC", L"最近合并文档：", 0, RecentLabel);
        app.recentLink = control(L"STATIC", L"尚无合并文档", WS_TABSTOP | SS_NOTIFY, RecentLink);
        control(L"BUTTON", L"另存为…", WS_TABSTOP | BS_PUSHBUTTON, SaveAs);
        control(L"BUTTON", L"删除", WS_TABSTOP | BS_PUSHBUTTON, DeleteRecent);
        try { app.recentPdf = RecentPdfPath(); } catch (...) {}
        UpdateRecentDocument();
        DragAcceptFiles(window, TRUE);
        Refresh(); Layout(); return 0;
    }
    case WM_SIZE: if (app.list) Layout(); return 0;
    case WM_GETMINMAXINFO: {
        auto limits = reinterpret_cast<MINMAXINFO*>(lParam);
        limits->ptMinTrackSize = {Px(650), Px(460)}; return 0;
    }
    case WM_NOTIFY: {
        auto notification = reinterpret_cast<NMHDR*>(lParam);
        if (notification->idFrom == List && notification->code == LVN_ITEMCHANGED) UpdateButtons();
        return 0;
    }
    case WM_DROPFILES: {
        HDROP drop = reinterpret_cast<HDROP>(wParam);
        std::vector<fs::path> paths;
        if (!app.busy) for (UINT i = 0, count = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0); i < count; ++i) {
            UINT length = DragQueryFileW(drop, i, nullptr, 0);
            std::wstring path(length + 1, L'\0'); DragQueryFileW(drop, i, path.data(), length + 1);
            path.resize(length); paths.emplace_back(path);
        }
        DragFinish(drop); AddFiles(paths); return 0;
    }
    case WM_COMMAND: {
        if (app.busy) return 0;
        if (LOWORD(wParam) == RecentLink && HIWORD(wParam) == STN_CLICKED && !app.recentPdf.empty()) {
            ShellExecuteW(window, L"open", app.recentPdf.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            return 0;
        }
        const int selected = Selected();
        switch (LOWORD(wParam)) {
        case Add: Browse(); break;
        case Up: if (selected > 0) { std::swap(app.files[selected], app.files[selected - 1]); Refresh(selected - 1); } break;
        case Down: if (selected >= 0 && selected + 1 < static_cast<int>(app.files.size())) { std::swap(app.files[selected], app.files[selected + 1]); Refresh(selected + 1); } break;
        case Remove: if (selected >= 0) { app.files.erase(app.files.begin() + selected); Refresh((std::min)(selected, static_cast<int>(app.files.size()) - 1)); } break;
        case Clear: app.files.clear(); Refresh(); break;
        case MergeNow: MergePdf(); break;
        case SaveAs: SaveRecentAs(); break;
        case DeleteRecent: DeleteRecentPdf(); break;
        }
        return 0;
    }
    case ProgressMessage: {
        std::unique_ptr<std::wstring> text(reinterpret_cast<std::wstring*>(lParam));
        SetWindowTextW(app.status, text->c_str()); return 0;
    }
    case FinishedMessage: {
        std::unique_ptr<Completion> done(reinterpret_cast<Completion*>(lParam));
        if (app.worker.joinable()) app.worker.join();
        app.busy = false; UpdateButtons();
        SetWindowTextW(app.status, done->success ? L"合并完成。文件列表已保留，可继续调整。" : L"合并失败。修正后可直接重试。");
        MessageBoxW(window, done->message.c_str(), L"PDF Merger", MB_OK | (done->success ? MB_ICONINFORMATION : MB_ICONWARNING));
        return 0;
    }
    case WM_CLOSE:
        if (app.busy) { ShowError(L"正在转换文件，请等待任务完成后再关闭。"); return 0; }
        DestroyWindow(window); return 0;
    case WM_DESTROY:
        if (app.worker.joinable()) app.worker.join();
        if (app.font) DeleteObject(app.font);
        PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}
} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    SetProcessDPIAware();
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_LISTVIEW_CLASSES}; InitCommonControlsEx(&controls);
    Gdiplus::GdiplusStartupInput startup;
    ULONG_PTR token = 0;
    if (Gdiplus::GdiplusStartup(&token, &startup, nullptr) != Gdiplus::Ok) return 1;
    WNDCLASSW cls{};
    cls.lpfnWndProc = WindowProc; cls.hInstance = instance; cls.lpszClassName = L"PDFMerger";
    cls.hCursor = LoadCursorW(nullptr, IDC_ARROW); cls.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(101));
    cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassW(&cls);
    HWND window = CreateWindowExW(0, cls.lpszClassName, L"PDF Merger", WS_OVERLAPPEDWINDOW,
                                  CW_USEDEFAULT, CW_USEDEFAULT, 1000, 680, nullptr, nullptr, instance, nullptr);
    int result = 1;
    if (window) {
        ShowWindow(window, show); UpdateWindow(window);
        MSG message{};
        while (GetMessageW(&message, nullptr, 0, 0) > 0) {
            if (!IsDialogMessageW(window, &message)) { TranslateMessage(&message); DispatchMessageW(&message); }
        }
        result = static_cast<int>(message.wParam);
    }
    Gdiplus::GdiplusShutdown(token);
    return result;
}
