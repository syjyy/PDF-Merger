# Third-party components

The project's own C++ source is MIT licensed. Bundled dependencies retain their
own licenses; the project's MIT license does not replace them.

- **qpdf 12.2.0**: https://github.com/qpdf/qpdf/tree/v12.2.0 . qpdf uses the Apache
  License 2.0. Its upstream Windows binary distribution also contains third-party
  libraries and their respective notices. The build script preserves the entire
  original distribution under `tools/qpdf/`, including its licenses and notices.
- **LibreOffice**: https://www.libreoffice.org/about-us/licenses/ . It is installed
  separately by the user, not bundled or redistributed by this project.
- **Windows SDK / GDI+**: system components provided by Windows. Not bundled.
- **Locally provided MinGW-w64 portable build**: the accompanying portable ZIP
  was built using GCC / MinGW-w64 with statically linked GCC runtime and
  winpthreads. Their notices are included under `licenses/`. The GCC runtime
  exception permits eligible compiled applications to use their own license;
  retain the supplied runtime notices when redistributing this binary. The
  GitHub Actions MSVC build does not use these MinGW runtime libraries.

If you change the packaged dependencies, retain and review their accompanying
license files and redistribution requirements.
