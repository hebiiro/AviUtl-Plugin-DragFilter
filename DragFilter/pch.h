#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include <vsstyle.h>
#include <vssym32.h>
#include <uxtheme.h>
#pragma comment(lib, "uxtheme.lib")

#include <tchar.h>
#include <crtdbg.h>
#include <strsafe.h>
#include <locale.h>

#include <vector>

typedef const BYTE* LPCBYTE;
#include "../AviUtl/aulslib/exedit.h"
#include "../Common/MyTracer.h"
#include "../Detours.4.0.1/detours.h"
#pragma comment(lib, "../Detours.4.0.1/detours.lib")
