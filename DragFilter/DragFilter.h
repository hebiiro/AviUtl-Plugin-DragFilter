﻿#pragma once

#include "Classes.h"
#include "Target.h"

//---------------------------------------------------------------------

const UINT ID_CREATE_CLONE			= 12020;
const UINT ID_CREATE_SAME_ABOVE		= 12021;
const UINT ID_CREATE_SAME_BELOW		= 12022;

//---------------------------------------------------------------------

typedef std::shared_ptr<FileUpdateChecker> FileUpdateCheckerPtr;
typedef std::shared_ptr<TargetMarkWindow> TargetMarkWindowPtr;

extern AviUtlInternal g_auin;

extern HINSTANCE g_instance; // この DLL のインスタンスハンドル。
extern UINT g_checkUpdateTimerId;
extern HWND g_filterWindow; // このプラグインのウィンドウハンドル。
extern HWND g_dragSrcWindow; // ドラッグ元をマークするウィンドウ。
extern HWND g_dragDstWindow; // ドラッグ先をマークするウィンドウ。
extern FileUpdateCheckerPtr g_settingsFile;
extern TargetMarkWindowPtr g_targetMarkWindow;
extern HHOOK g_keyboardHook;

extern COLORREF g_dragSrcColor; // ドラッグ元の色。
extern COLORREF g_dragDstColor; // ドラッグ先の色。
extern BOOL g_useShiftKey; // ドラッグ開始に Shift キーを必須にする。
extern BOOL g_useCtrlKey; // ドラッグ開始に Ctrl キーを必須にする。
extern BOOL g_useAltKey; // ドラッグ開始に Alt キーを必須にする。
extern BOOL g_useWinKey; // ドラッグ開始に Win キーを必須にする。

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

LRESULT CALLBACK keyboardHookProc(int code, WPARAM wParam, LPARAM lParam);

//---------------------------------------------------------------------
