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
// Function

// 絶対アドレスを書き換える。
template<class T>
inline T writeAbsoluteAddress(DWORD address, T x)
{
	// 絶対アドレスから読み込む。
	T retValue = 0;
	::ReadProcessMemory(::GetCurrentProcess(), (LPVOID)address, &retValue, sizeof(retValue), NULL);
	// 絶対アドレスを書き換える。
	::WriteProcessMemory(::GetCurrentProcess(), (LPVOID)address, &x, sizeof(x), NULL);
	// 命令キャッシュをフラッシュする。
	::FlushInstructionCache(::GetCurrentProcess(), (LPVOID)address, sizeof(x));
	return retValue;
}

//---------------------------------------------------------------------
// Api Hook

DECLARE_HOOK_PROC(LRESULT, WINAPI, Exedit_SettingDialog_WndProc, (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam));

//---------------------------------------------------------------------
// Internal Function and Variable

DECLARE_HOOK_PROC(void, CDECL, SwapFilter, (int objectIndex, int filterIndex, int relativeIndex));
DECLARE_HOOK_PROC(void, CDECL, Unknown1, (int objectIndex, int filterIndex));

extern HWND* g_exeditWindow;
extern HWND* g_settingDialog;
extern HMENU* g_menu[5];
extern auls::EXEDIT_OBJECT** g_objectTable;
extern auls::EXEDIT_FILTER** g_filterTable;
extern int* g_objectIndex;
extern int* g_filterIndex;
extern auls::EXEDIT_OBJECT** g_objectData;
extern BYTE** g_objectExdata;
extern int* g_nextObject;

int Exedit_GetCurrentObjectIndex();
int Exedit_GetCurrentFilterIndex();
auls::EXEDIT_OBJECT* Exedit_GetObject(int objectIndex);
auls::EXEDIT_FILTER* Exedit_GetFilter(int filterId);
auls::EXEDIT_FILTER* Exedit_GetFilter(auls::EXEDIT_OBJECT* object, int filterIndex);
int Exedit_GetNextObjectIndex(int objectIndex);

//---------------------------------------------------------------------
