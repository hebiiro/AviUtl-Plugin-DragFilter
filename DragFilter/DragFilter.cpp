#include "pch.h"
#include "DragFilter.h"

//---------------------------------------------------------------------

// デバッグ用コールバック関数。デバッグメッセージを出力する
void ___outputLog(LPCTSTR text, LPCTSTR output)
{
	::OutputDebugString(output);
}

//---------------------------------------------------------------------

HINSTANCE g_instance = 0; // この DLL のインスタンスハンドル。
HWND g_filterWindow = 0; // このプラグインのウィンドウハンドル。
HWND g_dragSrcWindow = 0; // ドラッグ元をマークするウィンドウ。
HWND g_dragDstWindow = 0; // ドラッグ先をマークするウィンドウ。
HWND g_exeditObjectDialog = 0; // 拡張編集のオブジェクトダイアログのハンドル。

int g_srcFilterIndex = 0; // ドラッグ元のフィルタのインデックス。
int g_dstFilterIndex = 0; // ドラッグ先のフィルタのインデックス。
BOOL g_isFilterDragging = FALSE; // ドラッグ中か判定するためのフラグ。

HMENU* g_menu[5] = {}; // 拡張編集のメニューへのポインタ。
auls::EXEDIT_OBJECT** g_objectTable = 0;
auls::EXEDIT_FILTER** g_filterTable = 0;
int* g_objectIndex = 0; // カレントオブジェクトのインデックスへのポインタ。
int* g_filterIndex = 0; // カレントフィルタのインデックスへのポインタ。
auls::EXEDIT_OBJECT** g_objectData = 0; // オブジェクトデータへのポインタ。
BYTE** g_objectExdata = 0; // オブジェクト拡張データへのポインタ。
int* g_filterPosY = 0; // フィルタの Y 座標配列へのポインタ。
int* g_nextObject = 0; // 次のオブジェクトの配列へのポインタ。

int Exedit_GetCurrentObjectIndex()
{
	return *g_objectIndex;
}

int Exedit_GetCurrentFilterIndex()
{
	return *g_filterIndex;
}

auls::EXEDIT_OBJECT* Exedit_GetObject(int objectIndex)
{
	return *g_objectData + objectIndex;
//	return g_objectTable[objectIndex];
}

auls::EXEDIT_FILTER* Exedit_GetFilter(int filterId)
{
	return g_filterTable[filterId];
}

auls::EXEDIT_FILTER* Exedit_GetFilter(auls::EXEDIT_OBJECT* object, int filterIndex)
{
	if (!object) return 0;
	int id = object->filter_param[filterIndex].id;
	if (id < 0) return 0;
	return Exedit_GetFilter(id);
}

int Exedit_GetNextObjectIndex(int objectIndex)
{
	return g_nextObject[objectIndex];
}

//---------------------------------------------------------------------

const int FILTER_HEADER_HEIGHT = 18; // フィルタの先頭付近の高さ。

// フィルタ名を返す。
LPCSTR getFilterName(int objectIndex, int filterIndex)
{
	auls::EXEDIT_OBJECT* object = Exedit_GetObject(objectIndex);
	if (!object) return "";
	auls::EXEDIT_FILTER* filter = Exedit_GetFilter(object, filterIndex);
	if (!filter) return "";
	return filter->name;
}

// フィルタ ID を返す。
int getFilterId(int objectIndex, int filterIndex)
{
	auls::EXEDIT_OBJECT* object = Exedit_GetObject(objectIndex);
	if (!object) return auls::EXEDIT_OBJECT::FILTER_PARAM::INVALID_ID;
	return object->filter_param[filterIndex].id;
}

// フィルタの数を返す。
int getFilterCount(int objectIndex)
{
	auls::EXEDIT_OBJECT* object = Exedit_GetObject(objectIndex);
	if (!object) return 0;
	return object->GetFilterNum();
}

int getLastFilterIndex(int objectIndex, int filterId)
{
	auls::EXEDIT_OBJECT* object = Exedit_GetObject(objectIndex);
	if (!object) return -1;
	for (int i = auls::EXEDIT_OBJECT::MAX_FILTER - 1; i >= 0; i--)
	{
		if (object->filter_param[i].id == filterId)
			return i;
	}

	return -1;
}

// 垂直スクロール量を返す。
int getScrollY()
{
#if 1
	HWND control = ::GetDlgItem(g_exeditObjectDialog, 1120);
	RECT rc; ::GetWindowRect(control, &rc);
	::MapWindowPoints(0, g_exeditObjectDialog, (POINT*)&rc, 2);
	return 7 - rc.top;
#else
	return ::GetScrollPos(g_exeditObjectDialog, SB_VERT);
#endif
}

// マウス座標を返す。
POINT getMousePoint(LPARAM lParam)
{
	int sy = getScrollY();
	POINT pos;
	pos.x = GET_X_LPARAM(lParam);
	pos.y = GET_Y_LPARAM(lParam) + sy;
	return pos;
}

// フィルタの Y 座標を返す。
int getFilterPosY(int filterIndex)
{
	if (filterIndex >= 12) return 0;

	if (g_filterPosY[1] == 0)
		return g_filterPosY[filterIndex + 1];
	else
		return g_filterPosY[filterIndex];
}

// フィルタの高さを返す。
int getFilterHeight(int filterIndex, int maxHeight)
{
	int y1 = getFilterPosY(filterIndex);
	int y2 = getFilterPosY(filterIndex + 1);
	if (y2 > 0) return y2 - y1;
	int sy = getScrollY();
	int h = maxHeight - (y1 - FILTER_HEADER_HEIGHT / 2 - sy);
	if (h > 0) return h;
	return FILTER_HEADER_HEIGHT;
}

// フィルタ矩形を返す。
void getFilterRect(int filterIndex, LPRECT rc)
{
	::GetClientRect(g_exeditObjectDialog, rc);
	int y = getFilterPosY(filterIndex) - FILTER_HEADER_HEIGHT / 2;
	int h = getFilterHeight(filterIndex, rc->bottom);
	rc->top = y;
	rc->bottom = y + h;
	int sy = getScrollY();
	::OffsetRect(rc, 0, -sy);
}

// y 座標にあるドラッグ元フィルタのインデックスを返す。
int getSrcFilterIndexFromY(int y)
{
	// オブジェクトインデックスを取得する。
	int objectIndex = *g_objectIndex;
	MY_TRACE_INT(objectIndex);
	if (objectIndex < 0) return -1;

	// フィルタインデックスを取得する。
	int filterIndex = GetFilterIndexFromY(y);
	MY_TRACE_INT(filterIndex);
	if (filterIndex < 0) return -1;

	// オブジェクトを取得する。
	auls::EXEDIT_OBJECT* object = Exedit_GetObject(objectIndex);
	if (!object) return -1;
	MY_TRACE_HEX(object);

	// フィルタを取得する。
	auls::EXEDIT_FILTER* filter = Exedit_GetFilter(object, filterIndex);
	if (!filter) return -1;
	MY_TRACE_HEX(filter);
	MY_TRACE_STR(filter->name);
	MY_TRACE_HEX(filter->flag);

	// フィルタが移動できるものかどうかチェックする。
	if (filter->flag & IGNORE_FILTER)
		return -1;

	return filterIndex;
}

// y 座標にあるドラッグ先フィルタのインデックスを返す。
int getDstFilterIndexFromY(int y)
{
	// オブジェクトインデックスを取得する。
	int objectIndex = *g_objectIndex;
	MY_TRACE_INT(objectIndex);
	if (objectIndex < 0) return -1;

	// フィルタインデックスを取得する。
	int filterIndex = GetFilterIndexFromY(y);
	MY_TRACE_INT(filterIndex);
	if (filterIndex < 0) filterIndex = 0;

	// オブジェクトを取得する。
	auls::EXEDIT_OBJECT* object = Exedit_GetObject(objectIndex);
	if (!object) return -1;
	MY_TRACE_HEX(object);

	// フィルタを取得する。
	auls::EXEDIT_FILTER* filter = Exedit_GetFilter(object, filterIndex);
	if (!filter) return -1;
	MY_TRACE_HEX(filter);
	MY_TRACE_STR(filter->name);
	MY_TRACE_HEX(filter->flag);

	// フィルタが移動できるものかどうかチェックする。
	if (filter->flag & IGNORE_FILTER)
	{
		if (filterIndex == 0)
		{
			// フィルタが先頭要素の場合はその次のフィルタを返す。
			return 1;
		}
		else
		{
			// フィルタが先頭要素ではない場合は無効。
			return -1;
		}
	}

	return filterIndex;
}

//---------------------------------------------------------------------

UINT g_createCloneId = 0;

void createClone(int origObjectIndex, int newFilterIndex)
{
	if (!g_createCloneId) return;

	MY_TRACE(_T("複製を作成します\n"));

	int objectIndex = origObjectIndex;
	MY_TRACE_INT(objectIndex);

	int midptLeader = Exedit_GetObject(objectIndex)->index_midpt_leader;
	MY_TRACE_INT(midptLeader);
	if (midptLeader >= 0)
		objectIndex = midptLeader; // 中間点がある場合は中間点元のオブジェクト ID を取得

	while (objectIndex >= 0)
	{
		// オブジェクトインデックスを取得する。
		MY_TRACE_INT(objectIndex);
		if (objectIndex < 0) break;

		// オブジェクトを取得する。
		auls::EXEDIT_OBJECT* object = Exedit_GetObject(objectIndex);
		MY_TRACE_HEX(object);
		if (!object) break;

		int midptLeader2 = object->index_midpt_leader;
		MY_TRACE_INT(midptLeader2);
		if (midptLeader2 != midptLeader) break;

		// コピー元フィルタのインデックスを取得する。
		int srcFilterIndex = Exedit_GetCurrentFilterIndex();
		MY_TRACE_INT(srcFilterIndex);
		if (srcFilterIndex < 0) break;

		// コピー先フィルタのインデックスを取得する。
		int dstFilterIndex = newFilterIndex;
		MY_TRACE_INT(dstFilterIndex);
		if (dstFilterIndex < 0) break;

		// コピー元フィルタを取得する。
		auls::EXEDIT_FILTER* srcFilter = Exedit_GetFilter(object, srcFilterIndex);
		MY_TRACE_HEX(srcFilter);
		if (!srcFilter) break;

		// コピー先フィルタを取得する。
		auls::EXEDIT_FILTER* dstFilter = Exedit_GetFilter(object, dstFilterIndex);
		MY_TRACE_HEX(dstFilter);
		if (!dstFilter) break;

		if (g_createCloneId == ID_CREATE_CLONE)
		{
			// 拡張データをコピーする。
			BYTE* objectExdata = *g_objectExdata;
			BYTE* srcFilterExdata = objectExdata + object->ExdataOffset(srcFilterIndex) + 0x0004;
			BYTE* dstFilterExdata = objectExdata + object->ExdataOffset(dstFilterIndex) + 0x0004;
			memcpy(dstFilterExdata, srcFilterExdata, srcFilter->exdata_size);

			// トラックデータをコピーする。
			for (int i = 0; i < srcFilter->track_num; i++)
			{
				int srcTrackIndex = object->filter_param[srcFilterIndex].track_begin + i;
				int dstTrackIndex = object->filter_param[dstFilterIndex].track_begin + i;
				object->track_value_left[dstTrackIndex] = object->track_value_left[srcTrackIndex];
				object->track_value_right[dstTrackIndex] = object->track_value_right[srcTrackIndex];
				object->track_mode[dstTrackIndex] = object->track_mode[srcTrackIndex];
				object->track_param[dstTrackIndex] = object->track_param[srcTrackIndex];
			}

			// チェックデータをコピーする。
			for (int i = 0; i < srcFilter->check_num; i++)
			{
				int srcCheckIndex = object->filter_param[srcFilterIndex].check_begin + i;
				int dstCheckIndex = object->filter_param[dstFilterIndex].check_begin + i;
				object->check_value[dstCheckIndex] = object->check_value[srcCheckIndex];
			}
		}

		if (midptLeader < 0) break;

		objectIndex = Exedit_GetNextObjectIndex(objectIndex);
	}

	// コピー元フィルタのインデックスを取得する。
	int srcFilterIndex = *g_filterIndex;
	MY_TRACE_INT(srcFilterIndex);
	if (srcFilterIndex < 0) return;

	// コピー先フィルタのインデックスを取得する。
	int dstFilterIndex = newFilterIndex;
	MY_TRACE_INT(dstFilterIndex);
	if (dstFilterIndex < 0) return;

	switch (g_createCloneId)
	{
	case ID_CREATE_SAME_ABOVE:
		{
			// コピー元のすぐ上に移動
			int c = dstFilterIndex - srcFilterIndex;
			for (int i = 0; i < c; i++)
				SwapFilter(origObjectIndex, dstFilterIndex--, -1);

			break;
		}
	case ID_CREATE_CLONE:
	case ID_CREATE_SAME_BELOW:
		{
			// コピー元のすぐ下に移動
			int c = dstFilterIndex - srcFilterIndex - 1;
			for (int i = 0; i < c; i++)
				SwapFilter(origObjectIndex, dstFilterIndex--, -1);

			break;
		}
	}
}

IMPLEMENT_HOOK_PROC_NULL(void, CDECL, Unknown1, (int objectIndex, int filterIndex))
{
	MY_TRACE(_T("Unknown1(%d, %d)\n"), objectIndex, filterIndex);

	true_Unknown1(objectIndex, filterIndex);

	createClone(objectIndex, filterIndex);
}

//---------------------------------------------------------------------

void drawText(HWND hwnd, HDC dc, LPCWSTR text, int c, LPRECT rc, UINT format)
{
#if 0
	HTHEME theme = ::OpenThemeData(hwnd, VSCLASS_WINDOW);

	DTTOPTS op = { sizeof(op) };
	op.dwFlags =
		DTT_TEXTCOLOR |
		DTT_BORDERCOLOR | DTT_BORDERSIZE |
		DTT_APPLYOVERLAY;
/*
		DTT_SHADOWCOLOR | DTT_SHADOWTYPE | DTT_SHADOWOFFSET |
		DTT_GLOWSIZE |
*/
	op.crText = RGB(0xff, 0xff, 0xff);
	op.crBorder = RGB(0x00, 0x00, 0x00);
	op.crShadow = RGB(0x00, 0x00, 0x00);
	op.iTextShadowType = TST_CONTINUOUS;
	op.ptShadowOffset.x = 2;
	op.ptShadowOffset.y = 2;
	op.iBorderSize = 2;
	op.fApplyOverlay = TRUE;
	op.iGlowSize = 4;
	HRESULT hr = ::DrawThemeTextEx(theme, dc, TEXT_BODYTITLE, 0, text, -1, format, rc, &op);

	::CloseThemeData(theme);
#else
//	::SetBkMode(dc, OPAQUE);
	::SetBkMode(dc, TRANSPARENT);
	::SetTextColor(dc, RGB(0xff, 0xff, 0xff));
	::SetBkColor(dc, RGB(0x00, 0x00, 0x00));
	::DrawTextW(dc, text, -1, rc, format);
#endif
}

// マークウィンドウのウィンドウ関数。
LRESULT CALLBACK markWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC dc = ::BeginPaint(hwnd, &ps);
			RECT rc = ps.rcPaint;

			BP_PAINTPARAMS pp = { sizeof(pp) };
			HDC mdc = 0;
			HPAINTBUFFER pb = ::BeginBufferedPaint(dc, &rc, BPBF_TOPDOWNDIB, &pp, &mdc);

			{
				HDC dc = mdc;

				// 背景を color で塗りつぶす。
				COLORREF color = (COLORREF)::GetProp(hwnd, _T("DragFilter.Color"));
				HBRUSH brush = ::CreateSolidBrush(color);
				FillRect(dc, &ps.rcPaint, brush);
				::DeleteObject(brush);

				// ウィンドウテキストを描画する。
				WCHAR text[MAX_PATH];
				::GetWindowTextW(hwnd, text, MAX_PATH);
//				HFONT font = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
//				LOGFONT lf; ::GetObject(font, sizeof(lf), &lf);
//				lf.lfWeight = FW_BLACK;
//				font = ::CreateFontIndirect(&lf);
//				HFONT oldFont = (HFONT)::SelectObject(dc, font);
				drawText(hwnd, dc, text, -1, &ps.rcPaint, DT_CENTER | DT_TOP | DT_NOCLIP);
//				::SelectObject(dc, oldFont);
//				::DeleteObject(font);
			}

			::EndBufferedPaint(pb, TRUE);

			EndPaint(hwnd, &ps);

			return 0;
		}
	}

	return ::DefWindowProc(hwnd, message, wParam, lParam);
}

// マークウィンドウを作成して返す。
HWND createMarkWindow(COLORREF color)
{
	WNDCLASS wc = {};
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_NOCLOSE;
	wc.hCursor = ::LoadCursor(0, IDC_ARROW);
	wc.lpfnWndProc = markWindowProc;
	wc.hInstance = g_instance;
	wc.lpszClassName = _T("DragFilter.Mark");
	::RegisterClass(&wc);

	HWND hwnd = ::CreateWindowEx(
		WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
		_T("DragFilter.Mark"),
		_T("DragFilter.Mark"),
		WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0, 0, 0, 0,
		0, 0, g_instance, 0);

	::SetProp(hwnd, _T("DragFilter.Color"), (HANDLE)color);

	return hwnd;
}

// 指定されたマークウィンドウを表示する。
void showMarkWindow(HWND hwnd, int filterIndex)
{
	LPCSTR filterName = getFilterName(*g_objectIndex, filterIndex);
	MY_TRACE_STR(filterName);
	::SetWindowTextA(hwnd, filterName);
	::InvalidateRect(hwnd, 0, FALSE);

	RECT rc; getFilterRect(filterIndex, &rc);
	POINT pos = { rc.left, rc.top };
	SIZE size = { rc.right - rc.left, rc.bottom - rc.top };
	::ClientToScreen(g_exeditObjectDialog, &pos);
	::SetLayeredWindowAttributes(hwnd, 0, 96, LWA_ALPHA);
	::SetWindowPos(hwnd, HWND_TOPMOST, pos.x, pos.y, size.cx, size.cy, SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

// 指定されたマークウィンドウを非表示にする。
void hideMarkWindow(HWND hwnd)
{
	::ShowWindow(hwnd, SW_HIDE);
}

//---------------------------------------------------------------------

// フックをセットする。
void initHook()
{
	MY_TRACE(_T("initHook()\n"));

	DetourTransactionBegin();
	DetourUpdateThread(::GetCurrentThread());

	ATTACH_HOOK_PROC(CreateWindowExA);
	ATTACH_HOOK_PROC(GetMessageA);

	if (DetourTransactionCommit() == NO_ERROR)
	{
		MY_TRACE(_T("API フックに成功しました\n"));
	}
	else
	{
		MY_TRACE(_T("API フックに失敗しました\n"));
	}
}

// 何もしない。
void termHook()
{
	MY_TRACE(_T("termHook()\n"));
}

// 拡張編集のフックをセットする。
void initExeditHook(HWND hwnd)
{
	MY_TRACE(_T("initExeditHook(0x%08X)\n"), hwnd);

	// 拡張編集のオブジェクトダイアログのハンドルを保存しておく。
	g_exeditObjectDialog = hwnd;
	// 拡張編集のオブジェクトダイアログのウィンドウプロシージャを保存しておく。
	true_Exedit_ObjectDialog_WndProc = (WNDPROC)::GetClassLong(hwnd, GCL_WNDPROC);

	// 拡張編集の関数や変数を取得する。
	DWORD exedit = (DWORD)::GetModuleHandle(_T("exedit.auf"));
	true_Unknown1 = (Type_Unknown1)(exedit + 0x34FF0);
	GetFilterIndexFromY = (Type_GetFilterIndexFromY)(exedit + 0x2CC30);
	PushUndo = (Type_PushUndo)(exedit + 0x8D150);
	CreateUndo = (Type_CreateUndo)(exedit + 0x8D290);
	SwapFilter = (Type_SwapFilter)(exedit + 0x33B30);
	DrawObjectDialog = (Type_DrawObjectDialog)(exedit + 0x39490);
	HideControls = (Type_HideControls)(exedit + 0x30500);
	ShowControls = (Type_ShowControls)(exedit + 0x305E0);
	g_menu[0] = (HMENU*)(exedit + 0x158D20);
	g_menu[1] = (HMENU*)(exedit + 0x158D24);
	g_menu[2] = (HMENU*)(exedit + 0x158D2C);
	g_menu[3] = (HMENU*)(exedit + 0x167D40);
	g_menu[4] = (HMENU*)(exedit + 0x167D44);
	g_objectTable = (auls::EXEDIT_OBJECT**)(exedit + 0x168FA8);
	g_filterTable = (auls::EXEDIT_FILTER**)(exedit + 0x187C98);
	g_objectIndex = (int*)(exedit + 0x177A10);
	g_filterIndex = (int*)(exedit + 0x14965C);
	g_objectData = (auls::EXEDIT_OBJECT**)(exedit + 0x1E0FA4);
	g_objectExdata = (BYTE**)(exedit + 0x1E0FA8);
	g_filterPosY = (int*)(exedit + 0x196714);
	g_nextObject = (int*)(exedit + 0x1592d8);

	// 拡張編集の関数をフックする。
	DetourTransactionBegin();
	DetourUpdateThread(::GetCurrentThread());

	ATTACH_HOOK_PROC(Exedit_ObjectDialog_WndProc);
	ATTACH_HOOK_PROC(Unknown1);

	if (DetourTransactionCommit() == NO_ERROR)
	{
		MY_TRACE(_T("拡張編集のフックに成功しました\n"));
	}
	else
	{
		MY_TRACE(_T("拡張編集のフックに失敗しました\n"));
	}
#if 1
	// コンテキストメニューを拡張するならこうする。
	for (int i = 0; i < sizeof(g_menu) / sizeof(g_menu[0]); i++)
	{
		HMENU menu = *g_menu[i];
		HMENU subMenu = ::GetSubMenu(menu, 0);
		::AppendMenu(subMenu, MF_SEPARATOR, 0, 0);
		::AppendMenu(subMenu, MF_STRING, ID_CREATE_CLONE, _T("完全な複製を隣に作成"));
		::AppendMenu(subMenu, MF_STRING, ID_CREATE_SAME_ABOVE, _T("同じフィルタ効果を上に作成"));
		::AppendMenu(subMenu, MF_STRING, ID_CREATE_SAME_BELOW, _T("同じフィルタ効果を下に作成"));
	}
#endif
}

//---------------------------------------------------------------------

EXTERN_C BOOL APIENTRY DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		{
			// ロケールを設定する。
			// これをやらないと日本語テキストが文字化けするので最初に実行する。
			_tsetlocale(LC_ALL, _T(""));

			MY_TRACE(_T("DLL_PROCESS_ATTACH\n"));

			// この DLL のハンドルをグローバル変数に保存しておく。
			g_instance = instance;
			MY_TRACE_HEX(g_instance);

			// この DLL の参照カウンタを増やしておく。
			WCHAR moduleFileName[MAX_PATH] = {};
			::GetModuleFileNameW(g_instance, moduleFileName, MAX_PATH);
			::LoadLibraryW(moduleFileName);

			initHook();

			break;
		}
	case DLL_PROCESS_DETACH:
		{
			MY_TRACE(_T("DLL_PROCESS_DETACH\n"));

			termHook();

			break;
		}
	}

	return TRUE;
}

//---------------------------------------------------------------------

IMPLEMENT_HOOK_PROC(HWND, WINAPI, CreateWindowExA, (DWORD exStyle, LPCSTR className, LPCSTR windowName, DWORD style, int x, int y, int w, int h, HWND parent, HMENU menu, HINSTANCE instance, LPVOID param))
{
	HWND result = true_CreateWindowExA(exStyle, className, windowName, style, x, y, w, h, parent, menu, instance, param);

	if (::lstrcmpiA(className, "ExtendedFilterClass") == 0)
	{
		// 拡張編集をフックする。
		initExeditHook(result);
	}

	return result;
}

IMPLEMENT_HOOK_PROC(BOOL, WINAPI, GetMessageA, (LPMSG msg, HWND hwnd, UINT msgFilterMin, UINT msgFilterMax))
{
#if 1
	BOOL result = ::GetMessageW(msg, hwnd, msgFilterMin, msgFilterMax);
#else
	BOOL result = true_GetMessageA(msg, hwnd, msgFilterMin, msgFilterMax);
#endif
#if 1
/*
	// この処理を実行しても ESC キーでダイアログが非表示になってしまう。
	if (msg->message == WM_KEYDOWN ||
		msg->message == WM_KEYUP ||
		msg->message == WM_CHAR)
	{
		if (msg->wParam == VK_ESCAPE ||
			msg->wParam == VK_TAB ||
			msg->wParam == VK_RETURN)
		{
			return result;
		}
	}
*/
	// 親ウィンドウを取得する。
	HWND dlg = ::GetParent(msg->hwnd);
	if (!dlg) return result;

	// 親ウィンドウがオブジェクトダイアログか確認する。
	TCHAR className[MAX_PATH] = {};
	::GetClassName(dlg, className, MAX_PATH);
//	MY_TRACE_TSTR(className);
	if (::lstrcmpi(className, "ExtendedFilterClass") == 0)
	{
		// ダイアログメッセージを処理する。
		if (::IsDialogMessageW(dlg, msg))
		{
			// このメッセージはディスパッチしてはならないので WM_NULL に置き換える。
			msg->hwnd = 0;
			msg->message = WM_NULL;
			msg->wParam = 0;
			msg->lParam = 0;
		}
	}
#endif
	return result;
}

IMPLEMENT_HOOK_PROC_NULL(LRESULT, WINAPI, Exedit_ObjectDialog_WndProc, (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam))
{
	switch (message)
	{
	case WM_SETCURSOR:
		{
			if ((HWND)wParam == hwnd && LOWORD(lParam) == HTCLIENT)
			{
				POINT pos; ::GetCursorPos(&pos);
				::ScreenToClient(hwnd, &pos);
				pos = getMousePoint(MAKELPARAM(pos.x, pos.y));

				int filterIndex = getSrcFilterIndexFromY(pos.y);
				MY_TRACE_INT(filterIndex);
				if (filterIndex < 0) break;

				::SetCursor(::LoadCursor(0, IDC_SIZENS));
				return TRUE;
			}

			break;
		}
	case WM_LBUTTONDOWN:
		{
			MY_TRACE(_T("WM_LBUTTONDOWN\n"));

			POINT pos = getMousePoint(lParam);

			// クリックしたフィルタを記憶しておく。
			g_srcFilterIndex = getSrcFilterIndexFromY(pos.y);
			MY_TRACE_INT(g_srcFilterIndex);
			g_dstFilterIndex = g_srcFilterIndex;
			if (g_srcFilterIndex < 0) break;

			MY_TRACE(_T("フィルタのドラッグを開始します\n"));

			// マウスをキャプチャーする。
			::SetCapture(hwnd);
			// フラグを立てる。
			g_isFilterDragging = TRUE;

			// ドラッグ元をマークする。
			showMarkWindow(g_dragSrcWindow, g_srcFilterIndex);

			break;
		}
	case WM_LBUTTONUP:
		{
			MY_TRACE(_T("WM_LBUTTONUP\n"));

			// フィルタをドラッグ中かチェックする。
			if (::GetCapture() == hwnd && g_isFilterDragging)
			{
				::ReleaseCapture(); // ここで WM_CAPTURECHANGED が呼ばれる。

				MY_TRACE_INT(g_srcFilterIndex);
				MY_TRACE_INT(g_dstFilterIndex);

				// オブジェクトのインデックスを取得する。
				int objectIndex = *g_objectIndex;
				int midptLeader = Exedit_GetObject(objectIndex)->index_midpt_leader;
				MY_TRACE_INT(midptLeader);
				if (midptLeader >= 0)
					objectIndex = midptLeader; // 中間点がある場合は中間点元のオブジェクト ID を取得
				MY_TRACE_INT(objectIndex);

				POINT pos;
				pos.x = GET_X_LPARAM(lParam);
				pos.y = GET_Y_LPARAM(lParam);

				// フィルタのインデックスの差分を取得する。
				int sub = g_dstFilterIndex - g_srcFilterIndex;
				MY_TRACE_INT(sub);

				if (sub != 0)
				{
					MY_TRACE(_T("ドラッグ先にフィルタを移動します\n"));

					// Undo を構築する。
					PushUndo();
					CreateUndo(objectIndex, 1);

					// フィルタを移動する。
					if (sub < 0)
					{
						// 上に移動
						for (int i = sub; i < 0; i++)
							SwapFilter(objectIndex, g_srcFilterIndex--, -1);
					}
					else
					{
						// 下に移動
						for (int i = sub; i > 0; i--)
							SwapFilter(objectIndex, g_srcFilterIndex++, 1);
					}

					// ダイアログを再描画する。
					DrawObjectDialog(objectIndex);
					// コントロール群を再配置する。
					HideControls();
					ShowControls(*g_objectIndex);

					// フレームの再描画を促す。
					::PostMessage(g_filterWindow, WM_FILTER_COMMAND, 0, 0);
				}
			}

			// ドラッグフラグなどをリセットする。
			g_isFilterDragging = FALSE;
			hideMarkWindow(g_dragSrcWindow);
			hideMarkWindow(g_dragDstWindow);

			break;
		}
	case WM_MOUSEMOVE:
		{
			// フィルタをドラッグ中かチェックする。
			if (::GetCapture() == hwnd && g_isFilterDragging)
			{
				POINT pos = getMousePoint(lParam);

				// マウスカーソルの位置にあるフィルタを取得する。
				g_dstFilterIndex = getDstFilterIndexFromY(pos.y);
				if (g_dstFilterIndex < 0) g_dstFilterIndex = g_srcFilterIndex;
//				MY_TRACE_INT(g_dstFilterIndex);

				if (g_dstFilterIndex != g_srcFilterIndex)
				{
					// ドラッグ先をマークする。
					showMarkWindow(g_dragDstWindow, g_dstFilterIndex);
				}
				else
				{
					// ドラッグ先のマークを隠す。
					hideMarkWindow(g_dragDstWindow);
				}

				// ドラッグ元のマークを再配置する。
				showMarkWindow(g_dragSrcWindow, g_srcFilterIndex);
			}

			break;
		}
	case WM_CAPTURECHANGED:
		{
			MY_TRACE(_T("WM_CAPTURECHANGED\n"));

			// ドラッグフラグなどをリセットする。
			g_isFilterDragging = FALSE;
			hideMarkWindow(g_dragSrcWindow);
			hideMarkWindow(g_dragDstWindow);

			break;
		}
	case WM_COMMAND:
		{
			switch (wParam)
			{
			case ID_CREATE_CLONE:
			case ID_CREATE_SAME_ABOVE:
			case ID_CREATE_SAME_BELOW:
				{
					// オブジェクトインデックスを取得する。
					int objectIndex = Exedit_GetCurrentObjectIndex();
					MY_TRACE_INT(objectIndex);
					if (objectIndex < 0) break;

					// フィルタインデックスを取得する。
					int filterIndex = Exedit_GetCurrentFilterIndex();
					MY_TRACE_INT(filterIndex);
					if (filterIndex < 0) break;

					// オブジェクトを取得する。
					auls::EXEDIT_OBJECT* object = Exedit_GetObject(objectIndex);
					if (!object) break;
					MY_TRACE_HEX(object);

					// フィルタを取得する。
					auls::EXEDIT_FILTER* filter = Exedit_GetFilter(object, filterIndex);
					if (!filter) break;
					MY_TRACE_HEX(filter);
					MY_TRACE_STR(filter->name);
					MY_TRACE_HEX(filter->flag);

					// フィルタが複製できるものかどうかチェックする。
					if (filter->flag & IGNORE_FILTER)
						break;

					// フィルタ ID を取得する。
					int filterId = object->filter_param[filterIndex].id;
					MY_TRACE_HEX(filterId);
					if (filterId < 0) break;

					// フィルタを作成するコマンドを発行する。
					g_createCloneId = wParam;
					LRESULT result = true_Exedit_ObjectDialog_WndProc(hwnd, message, 2000 + filterId, lParam);
					g_createCloneId = 0;
					return result;
				}
			}

			break;
		}
	}

	return true_Exedit_ObjectDialog_WndProc(hwnd, message, wParam, lParam);
}

//---------------------------------------------------------------------
//		フィルタ構造体のポインタを渡す関数
//---------------------------------------------------------------------
EXTERN_C FILTER_DLL __declspec(dllexport) * __stdcall GetFilterTable(void)
{
	static TCHAR g_filterName[] = TEXT("フィルタドラッグ移動");
	static TCHAR g_filterInformation[] = TEXT("フィルタドラッグ移動 version 5.0.1 by 蛇色");

	static FILTER_DLL g_filter =
	{
//		FILTER_FLAG_NO_CONFIG | // このフラグを指定するとウィンドウが作成されなくなってしまう。
		FILTER_FLAG_ALWAYS_ACTIVE | // このフラグがないと「フィルタ」に ON/OFF を切り替えるための項目が追加されてしまう。
		FILTER_FLAG_DISP_FILTER | // このフラグがないと「設定」の方にウィンドウを表示するための項目が追加されてしまう。
		FILTER_FLAG_WINDOW_THICKFRAME |
		FILTER_FLAG_WINDOW_SIZE |
		FILTER_FLAG_EX_INFORMATION,
		400, 400,
		g_filterName,
		NULL, NULL, NULL,
		NULL, NULL,
		NULL, NULL, NULL,
		func_proc,
		func_init,
		func_exit,
		NULL,
		func_WndProc,
		NULL, NULL,
		NULL,
		NULL,
		g_filterInformation,
		NULL, NULL,
		NULL, NULL, NULL, NULL,
		NULL,
	};

	return &g_filter;
}

//---------------------------------------------------------------------
//		初期化
//---------------------------------------------------------------------

BOOL func_init(FILTER *fp)
{
	MY_TRACE(_T("func_init()\n"));

	// このフィルタのウィンドウハンドルを保存しておく。
	g_filterWindow = fp->hwnd;
	MY_TRACE_HEX(g_filterWindow);

	// マークウィンドウを作成する。
	g_dragSrcWindow = createMarkWindow(RGB(0x00, 0x00, 0xff));
	MY_TRACE_HEX(g_dragSrcWindow);
	g_dragDstWindow = createMarkWindow(RGB(0xff, 0x00, 0x00));
	MY_TRACE_HEX(g_dragDstWindow);

	return TRUE;
}

//---------------------------------------------------------------------
//		終了
//---------------------------------------------------------------------
BOOL func_exit(FILTER *fp)
{
	MY_TRACE(_T("func_exit()\n"));

	return TRUE;
}

//---------------------------------------------------------------------
//		フィルタされた画像をバッファにコピー
//---------------------------------------------------------------------
BOOL func_proc(FILTER *fp, FILTER_PROC_INFO *fpip)
{
	MY_TRACE(_T("func_proc() : %d\n"), ::GetTickCount());

	return TRUE;
}

//---------------------------------------------------------------------
//		WndProc
//---------------------------------------------------------------------
BOOL func_WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, void *editp, FILTER *fp)
{
	// TRUE を返すと全体が再描画される

	switch (message)
	{
	case WM_FILTER_UPDATE:
		{
			MY_TRACE(_T("func_WndProc(WM_FILTER_UPDATE)\n"));

			if (fp->exfunc->is_editing(editp) != TRUE) break; // 編集中でなければ終了

			break;
		}
	case WM_FILTER_CHANGE_EDIT:
		{
			MY_TRACE(_T("func_WndProc(WM_FILTER_CHANGE_EDIT)\n"));

			if (fp->exfunc->is_editing(editp) != TRUE) break; // 編集中でなければ終了

			break;
		}
	case WM_FILTER_COMMAND:
		{
			MY_TRACE(_T("func_WndProc(WM_FILTER_COMMAND)\n"));

			return TRUE;
		}
	}

	return FALSE;
}

//---------------------------------------------------------------------
