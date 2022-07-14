#pragma once

#include "Classes.h"
#include "Target.h"

//---------------------------------------------------------------------

const UINT ID_CREATE_CLONE			= 12020;
const UINT ID_CREATE_SAME_ABOVE		= 12021;
const UINT ID_CREATE_SAME_BELOW		= 12022;

const UINT TIMER_ID_CHECK_UPDATE	= 1000;

//---------------------------------------------------------------------

typedef std::shared_ptr<FileUpdateChecker> FileUpdateCheckerPtr;
typedef std::shared_ptr<TargetMarkWindow> TargetMarkWindowPtr;

extern AviUtlInternal g_auin;

extern HINSTANCE g_instance; // この DLL のインスタンスハンドル。
extern HWND g_filterWindow; // このプラグインのウィンドウハンドル。
extern HWND g_dragSrcWindow; // ドラッグ元をマークするウィンドウ。
extern HWND g_dragDstWindow; // ドラッグ先をマークするウィンドウ。
extern FileUpdateCheckerPtr g_settingsFile;
extern TargetMarkWindowPtr g_targetMarkWindow;

extern ObjectHolder g_srcObject; // ドラッグ元のオブジェクト。
extern FilterHolder g_srcFilter; // ドラッグ元のフィルタ。
extern FilterHolder g_dstFilter; // ドラッグ先のフィルタ。
extern BOOL g_isFilterDragging; // ドラッグ中か判定するためのフラグ。

//---------------------------------------------------------------------

void initHook();
void termHook();

HWND createMarkWindow(COLORREF color);

void loadSettings(LPCWSTR fileName);

DECLARE_HOOK_PROC(LRESULT, WINAPI, SettingDialogProc, (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam));
DECLARE_HOOK_PROC(void, CDECL, SwapFilter, (int objectIndex, int filterIndex, int relativeIndex));
DECLARE_HOOK_PROC(void, CDECL, Unknown1, (int objectIndex, int filterIndex));

//---------------------------------------------------------------------
