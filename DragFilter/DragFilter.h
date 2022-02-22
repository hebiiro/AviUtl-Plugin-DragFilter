#pragma once

//---------------------------------------------------------------------
// Define and Const

#define DECLARE_INTERNAL_PROC(resultType, callType, procName, args) \
	typedef resultType (callType *Type_##procName) args; \
	Type_##procName procName = NULL

#define DECLARE_HOOK_PROC(resultType, callType, procName, args) \
	typedef resultType (callType *Type_##procName) args; \
	extern Type_##procName true_##procName; \
	resultType callType hook_##procName args

#define IMPLEMENT_HOOK_PROC(resultType, callType, procName, args) \
	Type_##procName true_##procName = procName; \
	resultType callType hook_##procName args

#define IMPLEMENT_HOOK_PROC_NULL(resultType, callType, procName, args) \
	Type_##procName true_##procName = NULL; \
	resultType callType hook_##procName args

#define GET_INTERNAL_PROC(module, procName) \
	procName = (Type_##procName)::GetProcAddress(module, #procName)

#define GET_HOOK_PROC(module, procName) \
	true_##procName = (Type_##procName)::GetProcAddress(module, #procName)

#define ATTACH_HOOK_PROC(name) DetourAttach((PVOID*)&true_##name, hook_##name)

const UINT ID_CREATE_CLONE			= 12020;
const UINT ID_CREATE_SAME_ABOVE		= 12021;
const UINT ID_CREATE_SAME_BELOW		= 12022;

const UINT TIMER_ID_CHECK_UPDATE	= 1000;

//---------------------------------------------------------------------
// Api Hook

DECLARE_HOOK_PROC(HWND, WINAPI, CreateWindowExA, (DWORD exStyle, LPCSTR className, LPCSTR windowName, DWORD style, int x, int y, int w, int h, HWND parent, HMENU menu, HINSTANCE instance, LPVOID param));

DECLARE_HOOK_PROC(LRESULT, WINAPI, Exedit_ObjectDialog_WndProc, (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam));

//---------------------------------------------------------------------
// Internal Function and Variable

DECLARE_HOOK_PROC(void, CDECL, Unknown1, (int objectIndex, int filterIndex));

DECLARE_INTERNAL_PROC(int, CDECL, GetFilterIndexFromY, (int y));
DECLARE_INTERNAL_PROC(void, CDECL, PushUndo, ());
DECLARE_INTERNAL_PROC(void, CDECL, CreateUndo, (int objectIndex, BOOL flag));
DECLARE_INTERNAL_PROC(void, CDECL, SwapFilter, (int objectIndex, int filterIndex, int relativeIndex));
DECLARE_INTERNAL_PROC(void, CDECL, DrawObjectDialog, (int objectIndex));
DECLARE_INTERNAL_PROC(void, CDECL, HideControls, ());
DECLARE_INTERNAL_PROC(BOOL, CDECL, ShowControls, (int objectIndex));

extern HMENU* g_menu[5];
extern auls::EXEDIT_OBJECT** g_objectTable;
extern auls::EXEDIT_FILTER** g_filterTable;
extern int* g_objectIndex;
extern int* g_filterIndex;
extern auls::EXEDIT_OBJECT** g_objectData;
extern BYTE** g_objectExdata;
extern int* g_filterPosY;
extern int* g_nextObject;

int Exedit_GetCurrentObjectIndex();
int Exedit_GetCurrentFilterIndex();
auls::EXEDIT_OBJECT* Exedit_GetObject(int objectIndex);
auls::EXEDIT_FILTER* Exedit_GetFilter(int filterId);
auls::EXEDIT_FILTER* Exedit_GetFilter(auls::EXEDIT_OBJECT* object, int filterIndex);
int Exedit_GetNextObjectIndex(int objectIndex);

//---------------------------------------------------------------------
