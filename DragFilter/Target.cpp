#include "pch.h"
#include "DragFilter.h"

//--------------------------------------------------------------------
namespace TargetMark {
//--------------------------------------------------------------------

BOOL g_enable = TRUE;
BYTE g_alpha = 192;
Color g_penColor(192, 0, 0, 0);
REAL g_penWidth = 2.0f;
Color g_brushColor(255, 255, 255, 255);
int g_base = 16;
int g_width = 8;
_bstr_t g_fontName = L"Segoe UI";
REAL g_fontSize = 32.0f;
REAL g_rotate = 7.77f;
Point g_beginMove(0, 100);

//--------------------------------------------------------------------
} // namespace TargetMark
//--------------------------------------------------------------------
