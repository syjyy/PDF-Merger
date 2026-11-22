#pragma once
#include "merger.h"
#include <gdiplus.h>
#include <memory>

namespace ordered_pdf {
// Detect a light, complete sheet, rectify perspective, and normalize lighting.
// Throws Error instead of silently cropping an uncertain boundary.
std::unique_ptr<Gdiplus::Bitmap> ScanDocument(Gdiplus::Bitmap& source);
}
