# PDF Merger

![PDF Merger](static/logo.png)

Windows 桌面文件合并工具，使用 **C++17 + Win32**。选择本机 PDF、Word、Excel 和图片，
按列表顺序转换并合并成一个 PDF。无需 Python、浏览器、账号或云服务。

例如，依次添加 A.pdf（3 页）、B.docx（2 页）、C.png（1 页），输出顺序为：

```text
A 第 1 页、A 第 2 页、A 第 3 页、B 第 1 页、B 第 2 页、C 图片
```

每个文件作为一个连续整体，绝不交叉拼页。重复添加同一文件会重复包含其全部页面。

## 普通用户使用

本次交付的 `PDF-Merger-source-and-setup.zip` 同时包含源码和已编译安装器。
解压后直接点击根目录的 `PDF-Merger-Setup-windows-x64.exe`，不必先编译源码。
GitHub 自动生成的 Source code ZIP 则只包含源码；仓库维护者应另外上传安装器到 Releases。

推荐下载 `PDF-Merger-Setup-windows-x64.exe`，双击后按向导操作：

1. 选择本工具的安装目录。
2. 配置 Word / Excel 支持：自动识别已安装的 LibreOffice（读取安装记录和已有配置），默认复用并跳过文件选择。未识别时只需选 LibreOffice 安装文件夹或 `program` 文件夹，无需翻找 EXE；也可下载安装或暂时跳过。
3. 如选择新安装，可单独设置 LibreOffice 安装目录。选择已有文件的文件夹时，向导自动补上 `LibreOffice` 子目录，重名时自动编号；无需先创建目录。最终路径会显示在安装前的确认页。向导下载官方 MSI，并校验固定 SHA-256。
4. 选择是否创建桌面快捷方式，完成安装后启动。

PDF 引擎及其 DLL 已内置，无需配置 PATH。LibreOffice 路径保存在应用目录的 `settings.ini`，
不修改系统 PATH。支持开始菜单启动和 Windows“已安装的应用”卸载。
安装器需要管理员权限；首次下载安装 LibreOffice 需要网络和数百 MB 流量。
已有 LibreOffice 时请选择“使用已安装的 LibreOffice”，避免更改已有部署。
LibreOffice 是独立安装的软件，卸载本工具不会卸载它。
静默安装默认跳过 LibreOffice 下载；完整配置请使用交互式向导。

也可使用便携版：

1. 解压完整的 `PDF-Merger-windows-x64.zip`，不要只拷贝其中的 EXE。
2. 双击 `OrderedPdfMerger.exe`。
3. 点击“添加文件”，或从资源管理器拖入文件。允许分批添加。
4. 检查顺序。选中文件，用“上移 / 下移”调整；也可移除或清空。
5. 点击“合并并保留 PDF”。程序先生成并保留“最近合并.pdf”，不再要求当场选择保存位置。
6. 点击界面中的文档路径可直接打开；也可“另存为…”复制到其他位置，或“删除”最近文档。

最近文档优先保存在程序目录。目录不可写时，自动保存在 `%LOCALAPPDATA%\PDF Merger`，不修改权限或系统设置。重新打开程序后仍可访问；卸载程序时保留用户生成的 PDF。

Windows 多选框和拖放接口返回的文件顺序**不保证是鼠标点击顺序**。程序保留接口返回的
顺序，不按名称自行排序。**主窗口列表才是最终合并顺序**；若要求精确的添加顺序，可逐个
添加或添加后调整。文件内部所有页面保持连续。

支持 Windows 10 / 11 x64。输入最多 100 个文件，总大小 200 MB。

### Word / Excel 依赖

仅合并 PDF / 图片时不需要 Office。处理 `.doc`、`.docx`、`.xls`、`.xlsx`，需先安装
[LibreOffice](https://www.libreoffice.org/download/download-libreoffice/)。不要求 Microsoft Office。

程序自动查找 `Program Files`、`Program Files (x86)`、`LOCALAPPDATA` 中的标准安装位置。
自定义安装可在 PowerShell 中指定：

```powershell
$env:LIBREOFFICE_PATH = 'D:\LibreOffice\program\soffice.exe'
.\OrderedPdfMerger.exe
```

## 支持格式与输出规则

| 类型 | 扩展名 | 行为 |
| --- | --- | --- |
| PDF | `.pdf` | 按原始页面顺序直接拼接，不把页面重新截图 |
| Word | `.doc`, `.docx` | LibreOffice 导出完整文档，再拼接 |
| Excel | `.xls`, `.xlsx` | LibreOffice 按工作簿打印设置导出，再拼接 |
| 图片 | `.jpg`, `.jpeg`, `.png`, `.bmp`, `.tif`, `.tiff` | 先转成统一 A4 竖版 PDF，再合并；每帧一页，等比居中、不裁切，四边至少留 18 pt 空白 |

- 多页 TIFF 按图像帧顺序输出；支持 JPEG EXIF 方向，透明背景铺白。
- 图片最长边最多 3508 像素，JPEG 质量 95；每帧最多 4000 万像素、每份图片最多 1000 帧。
- Excel 的分页、打印区域、隐藏表、工作表顺序遵循 LibreOffice 导出规则。
  如要每张表适配一页，请先设置打印区域和缩放。程序不会自动缩放整个工作簿到一页。
- Word 字体、分页，Excel 公式、图表的显示可能与 Microsoft Office 不同。
  要求严格还原时，请先用原始软件导出 PDF，再合并。
- 不提供密码输入。受密码保护的文件可能无法处理，请先解密。
- 合并是页面组装，不承诺保留源文件书签、附件、JavaScript、电子签名有效性或复杂表单行为。
  数字签名 PDF 请保留原件；合并结果不能替代签名原件。
- 某份文件失败会终止整个任务。不会跳过该文件，也不会用部分结果覆盖旧输出。

## 从源码构建

**GitHub 的 Source code ZIP 只包含源码，不包含 EXE。** 普通用户应下载 Releases 中的
`Setup.exe`；开发者编译源码后生成安装器。不要把 `OrderedPdfMerger.exe` 当作安装器。

安装 Visual Studio 2022 Build Tools：勾选“使用 C++ 的桌面开发”、Windows SDK 和 CMake。
安装 Git（或直接下载仓库 ZIP）。在 **Developer PowerShell for VS 2022** 中运行：

```powershell
Set-Location path\to\ordered-pdf-merger
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build.ps1
```

脚本使用 CMake 编译 C++，下载并校验 qpdf 12.2.0 官方 Windows x64 包，执行原生测试，
再生成 `dist/PDF-Merger-windows-x64.zip`。首次构建需联网，运行程序无需联网。
Microsoft C++ 运行库静态链接；qpdf 的 DLL 与许可证随其完整目录分发。

也可手动运行：

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
.\scripts\fetch-qpdf.ps1
$env:QPDF_PATH = "$pwd\tools\qpdf\bin\qpdf.exe"
ctest --test-dir build -C Release --output-on-failure
```

手动运行构建出的 EXE 时，设置 `QPDF_PATH`，或将完整 `tools/qpdf` 放到 EXE 旁边。
MinGW-w64 也支持；请用 CMake 的 MinGW Makefiles / Ninja 生成器，不传 `-A x64`。

### 生成标准 Windows 安装程序

安装 [Inno Setup 6.7.3 或更新版本](https://jrsoftware.org/isinfo.php)，然后运行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build-installer.ps1
```

已完成 `build.ps1` 时可传 `-SkipBuild`。非标准 Inno Setup 路径可传 `-Compiler`。
输出为 `dist/PDF-Merger-Setup-windows-x64.exe`。`installer/setup.iss` 定义向导、
路径选择、依赖安装、快捷方式与卸载。安装器只内置 PDF 依赖，LibreOffice 按用户选择联网下载。
LibreOffice 下载版本、地址和 SHA-256 固定在该文件头部，升级依赖时应同步更新并验证。
第三方中文版 Inno Setup 语言文件保留原始版权和许可头部。
GitHub Actions 另外执行安装 / 卸载冒烟测试：选择临时安装目录，验证 PDF 引擎与
Unicode 路径配置文件，再卸载。此测试不下载 LibreOffice。

发布的安装器当前未做代码签名；正式分发可使用自己的 Windows 代码签名证书签名。

## 测试

原生测试验证多页文件整体顺序、内部页序、重复文件、中文和空格路径、图片、损坏 / 加密
输入以及失败时不覆盖原输出。测试入口 `merger_tests.exe` 同时提供仅供测试的命令行模式：

```powershell
.\build\Release\merger_tests.exe --merge output.pdf B.pdf A.pdf
```

可选 Python 集成测试只用于开发，不是产品运行依赖。它另外验证 PNG、多页 TIFF、
两页 Word 和两工作表 Excel 的实际输出：

```powershell
python -m pip install "pypdf>=6,<7" "Pillow>=12,<13" "python-docx>=1.1,<2" "openpyxl>=3.1,<4"
python tests/integration.py build/Release/merger_tests.exe --require-office
```

不加 `--require-office` 时，无 LibreOffice 会明确跳过 Office 测试。GitHub Actions
会安装 LibreOffice 并要求 Office 测试通过，上传 Windows 便携包到该次运行的 Artifacts。

## 发布到 GitHub

在 GitHub 新建空仓库，然后在源码目录运行（替换为自己的仓库地址）：

```powershell
git init
git add .
git commit -m "Initial Windows C++ PDF merger"
git branch -M main
git remote add origin https://github.com/syjyy/PDF-Merger.git
git push -u origin main
```

Actions 通过后，可下载便携包并手动附到 GitHub Release，便于普通用户直接使用。
同次构建也生成 `PDF-Merger-Setup-windows-x64.exe`，推荐将它作为主要下载入口。
GitHub Pages 不能直接运行 Win32 C++ 桌面程序。

## 实现与隐私

- 所有处理在本机完成，不监听端口，不上传文件。
- 使用 Windows 原生控件，后台线程处理转换，界面显示当前文件。
- 通过 `CreateProcessW` 启动明确路径的程序，不使用 `cmd.exe` 或 shell 拼接命令。
- 单次子进程最长 120 秒；Windows Job Object 在超时或退出时终止子进程树。
- 每个 Office 文件使用独立 LibreOffice 配置目录，设置最高宏安全级别，不自动刷新 Calc 外链。
- 输入复制到独立临时目录；正常成功 / 失败均清理。系统强制退出或断电可能留下临时目录，
  可删除 `%TEMP%\ordered-pdf-*`。临时文件删除不是安全擦除。
- 先写入目标目录中的随机临时文件，qpdf 校验成功后才替换目标；拒绝覆盖输入文件。
- 这些措施不等于文档沙箱。保持 Windows、qpdf、LibreOffice 更新，仅处理可信文件。

## 许可证

项目自身代码采用 [MIT License](LICENSE)。依赖保留各自许可，见
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
