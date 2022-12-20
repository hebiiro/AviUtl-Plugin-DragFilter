#include "pch.h"
#include "DragFilter.h"

//--------------------------------------------------------------------

// デバッグ用コールバック関数。デバッグメッセージを出力する
void ___outputLog(LPCTSTR text, LPCTSTR output)
{
	::OutputDebugString(output);
}

//--------------------------------------------------------------------

AviUtlInternal g_auin;

HINSTANCE g_instance = 0; // この DLL のインスタンスハンドル。
UINT g_checkUpdateTimerId = (UINT)&g_checkUpdateTimerId;
HWND g_filterWindow = 0; // このプラグインのウィンドウハンドル。
HWND g_dragSrcWindow = 0; // ドラッグ元をマークするウィンドウ。
HWND g_dragDstWindow = 0; // ドラッグ先をマークするウィンドウ。
FileUpdateCheckerPtr g_settingsFile;
TargetMarkWindowPtr g_targetMarkWindow;
HHOOK g_keyboardHook = 0;

COLORREF g_dragSrcColor = RGB(0x00, 0x00, 0xff);
COLORREF g_dragDstColor = RGB(0xff, 0x00, 0x00);
BOOL g_useShiftKey = FALSE;
BOOL g_useCtrlKey = FALSE;
BOOL g_useAltKey = FALSE;
BOOL g_useWinKey = FALSE;

ObjectHolder g_srcObject; // ドラッグ元のオブジェクト。
FilterHolder g_srcFilter; // ドラッグ元のフィルタ。
FilterHolder g_dstFilter; // ドラッグ先のフィルタ。
BOOL g_isFilterDragging = FALSE; // ドラッグ中か判定するためのフラグ。

//--------------------------------------------------------------------

void moveFilter(int objectIndex)
{
	int srcFilterIndex = g_srcFilter.getFilterIndex();
	MY_TRACE_INT(srcFilterIndex);

	int dstFilterIndex = g_dstFilter.getFilterIndex();
	MY_TRACE_INT(dstFilterIndex);

	// フィルタのインデックスの差分を取得する。
	int sub = dstFilterIndex - srcFilterIndex;
	MY_TRACE_INT(sub);

	// フィルタを移動する。
	if (sub < 0)
	{
		// 上に移動
		for (int i = sub; i < 0; i++)
			true_SwapFilter(objectIndex, srcFilterIndex--, -1);
	}
	else
	{
		// 下に移動
		for (int i = sub; i > 0; i--)
			true_SwapFilter(objectIndex, srcFilterIndex++, 1);
	}
}

BOOL g_moveFilterFlag = FALSE;

IMPLEMENT_HOOK_PROC_NULL(void, CDECL, SwapFilter, (int objectIndex, int filterIndex, int relativeIndex))
{
	if (g_moveFilterFlag)
		moveFilter(objectIndex);
	else
		true_SwapFilter(objectIndex, filterIndex, relativeIndex);
}

//--------------------------------------------------------------------

void createClone(UINT createCloneId, int origObjectIndex, int newFilterIndex)
{
	MY_TRACE(_T("複製を作成します\n"));

	int objectIndex = origObjectIndex;
	MY_TRACE_INT(objectIndex);

	int midptLeader = g_auin.GetObject(objectIndex)->index_midpt_leader;
	MY_TRACE_INT(midptLeader);
	if (midptLeader >= 0)
		objectIndex = midptLeader; // 中間点がある場合は中間点元のオブジェクト ID を取得

	while (objectIndex >= 0)
	{
		// オブジェクトインデックスを取得する。
		MY_TRACE_INT(objectIndex);
		if (objectIndex < 0) break;

		// オブジェクトを取得する。
		ExEdit::Object* object = g_auin.GetObject(objectIndex);
		MY_TRACE_HEX(object);
		if (!object) break;

		int midptLeader2 = object->index_midpt_leader;
		MY_TRACE_INT(midptLeader2);
		if (midptLeader2 != midptLeader) break;

		// コピー元フィルタのインデックスを取得する。
		int srcFilterIndex = g_auin.GetCurrentFilterIndex();
		MY_TRACE_INT(srcFilterIndex);
		if (srcFilterIndex < 0) break;

		// コピー先フィルタのインデックスを取得する。
		int dstFilterIndex = newFilterIndex;
		MY_TRACE_INT(dstFilterIndex);
		if (dstFilterIndex < 0) break;

		// コピー元フィルタを取得する。
		ExEdit::Filter* srcFilter = g_auin.GetFilter(object, srcFilterIndex);
		MY_TRACE_HEX(srcFilter);
		if (!srcFilter) break;

		// コピー先フィルタを取得する。
		ExEdit::Filter* dstFilter = g_auin.GetFilter(object, dstFilterIndex);
		MY_TRACE_HEX(dstFilter);
		if (!dstFilter) break;

		if (createCloneId == ID_CREATE_CLONE)
		{
			// 拡張データをコピーする。
			BYTE* srcFilterExdata = g_auin.GetExdata(object, srcFilterIndex);
			BYTE* dstFilterExdata = g_auin.GetExdata(object, dstFilterIndex);
			memcpy(dstFilterExdata, srcFilterExdata, srcFilter->exdata_size);

			// トラックデータをコピーする。
			for (int i = 0; i < srcFilter->track_n; i++)
			{
				int srcTrackIndex = object->filter_param[srcFilterIndex].track_begin + i;
				int dstTrackIndex = object->filter_param[dstFilterIndex].track_begin + i;
				object->track_value_left[dstTrackIndex] = object->track_value_left[srcTrackIndex];
				object->track_value_right[dstTrackIndex] = object->track_value_right[srcTrackIndex];
				object->track_mode[dstTrackIndex] = object->track_mode[srcTrackIndex];
				object->track_param[dstTrackIndex] = object->track_param[srcTrackIndex];
			}

			// チェックデータをコピーする。
			for (int i = 0; i < srcFilter->check_n; i++)
			{
				int srcCheckIndex = object->filter_param[srcFilterIndex].check_begin + i;
				int dstCheckIndex = object->filter_param[dstFilterIndex].check_begin + i;
				object->check_value[dstCheckIndex] = object->check_value[srcCheckIndex];
			}
		}

		if (midptLeader < 0) break;

		objectIndex = g_auin.GetNextObjectIndex(objectIndex);
	}

	// コピー元フィルタのインデックスを取得する。
	int srcFilterIndex = g_auin.GetCurrentFilterIndex();
	MY_TRACE_INT(srcFilterIndex);
	if (srcFilterIndex < 0) return;

	// コピー先フィルタのインデックスを取得する。
	int dstFilterIndex = newFilterIndex;
	MY_TRACE_INT(dstFilterIndex);
	if (dstFilterIndex < 0) return;

	switch (createCloneId)
	{
	case ID_CREATE_SAME_ABOVE:
		{
			// コピー元のすぐ上に移動
			int c = dstFilterIndex - srcFilterIndex;
			for (int i = 0; i < c; i++)
				true_SwapFilter(origObjectIndex, dstFilterIndex--, -1);

			break;
		}
	case ID_CREATE_CLONE:
	case ID_CREATE_SAME_BELOW:
		{
			// コピー元のすぐ下に移動
			int c = dstFilterIndex - srcFilterIndex - 1;
			for (int i = 0; i < c; i++)
				true_SwapFilter(origObjectIndex, dstFilterIndex--, -1);

			break;
		}
	}
}

UINT g_createCloneId = 0;

IMPLEMENT_HOOK_PROC_NULL(void, CDECL, Unknown1, (int objectIndex, int filterIndex))
{
	MY_TRACE(_T("Unknown1(%d, %d)\n"), objectIndex, filterIndex);

	true_Unknown1(objectIndex, filterIndex);

	if (g_createCloneId)
		createClone(g_createCloneId, objectIndex, filterIndex);
}

//--------------------------------------------------------------------

void drawText(HWND hwnd, HDC dc, LPCWSTR text, int c, LPRECT rc, UINT format)
{
	::SetBkMode(dc, TRANSPARENT);
	::SetTextColor(dc, RGB(0xff, 0xff, 0xff));
	::SetBkColor(dc, RGB(0x00, 0x00, 0x00));
	::DrawTextW(dc, text, -1, rc, format);
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
			HPAINTBUFFER pb = ::BeginBufferedPaint(dc, &rc, BPBF_COMPATIBLEBITMAP, &pp, &mdc);

			if (pb)
			{
				HDC dc = mdc;

				// 背景を color で塗りつぶす。
				COLORREF color = (COLORREF)::GetProp(hwnd, _T("DragFilter.Color"));
				HBRUSH brush = ::CreateSolidBrush(color);
				::FillRect(dc, &ps.rcPaint, brush);
				::DeleteObject(brush);

				// ウィンドウテキストを描画する。
				WCHAR text[MAX_PATH];
				::GetWindowTextW(hwnd, text, MAX_PATH);
				drawText(hwnd, dc, text, -1, &ps.rcPaint, DT_CENTER | DT_TOP | DT_NOCLIP);

				::EndBufferedPaint(pb, TRUE);
			}

			::EndPaint(hwnd, &ps);

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
void showMarkWindow(HWND hwnd, const DialogInfo& di, FilterHolder filter)
{
	LPCSTR name = filter.getName();
//	MY_TRACE_STR(name);
	::SetWindowTextA(hwnd, name);
	::InvalidateRect(hwnd, 0, FALSE);

	RECT rc; di.getFilterRect(filter, &rc);
	POINT pos = { rc.left, rc.top };
	SIZE size = { rc.right - rc.left, rc.bottom - rc.top };
	::ClientToScreen(g_auin.GetSettingDialog(), &pos);
	::SetLayeredWindowAttributes(hwnd, 0, 96, LWA_ALPHA);
	::SetWindowPos(hwnd, HWND_TOPMOST, pos.x, pos.y, size.cx, size.cy, SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

// 指定されたマークウィンドウを非表示にする。
void hideMarkWindow(HWND hwnd)
{
	::ShowWindow(hwnd, SW_HIDE);
}

//--------------------------------------------------------------------

// フックをセットする。
void initHook()
{
	MY_TRACE(_T("initHook()\n"));

	// 拡張編集の関数や変数を取得する。
	true_SwapFilter = g_auin.GetSwapFilter();
	true_Unknown1 = g_auin.GetUnknown1();
	true_SettingDialogProc = g_auin.HookSettingDialogProc(hook_SettingDialogProc);
	MY_TRACE_HEX(true_SettingDialogProc);
	MY_TRACE_HEX(hook_SettingDialogProc);

	DetourTransactionBegin();
	DetourUpdateThread(::GetCurrentThread());

	ATTACH_HOOK_PROC(SwapFilter);
	ATTACH_HOOK_PROC(Unknown1);

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

// 設定ファイルを読み込む。
void loadSettings(LPCWSTR fileName)
{
	MY_TRACE(_T("loadSettings(%ws)\n"), fileName);

	getPrivateProfileBool(fileName, L"TargetMark", L"enable", TargetMark::g_enable);
	getPrivateProfileInt(fileName, L"TargetMark", L"alpha", TargetMark::g_alpha);
	getPrivateProfileColor(fileName, L"TargetMark", L"penColor", TargetMark::g_penColor);
	getPrivateProfileReal(fileName, L"TargetMark", L"penWidth", TargetMark::g_penWidth);
	getPrivateProfileColor(fileName, L"TargetMark", L"brushColor", TargetMark::g_brushColor);
	getPrivateProfileInt(fileName, L"TargetMark", L"base", TargetMark::g_base);
	getPrivateProfileInt(fileName, L"TargetMark", L"width", TargetMark::g_width);
	getPrivateProfileBSTR(fileName, L"TargetMark", L"fontName", TargetMark::g_fontName);
	getPrivateProfileReal(fileName, L"TargetMark", L"fontSize", TargetMark::g_fontSize);
	getPrivateProfileReal(fileName, L"TargetMark", L"rotate", TargetMark::g_rotate);
	getPrivateProfileInt(fileName, L"TargetMark", L"beginMoveX", TargetMark::g_beginMove.X);
	getPrivateProfileInt(fileName, L"TargetMark", L"beginMoveY", TargetMark::g_beginMove.Y);

	getPrivateProfileColor(fileName, L"Settings", L"dragSrcColor", g_dragSrcColor);
	getPrivateProfileColor(fileName, L"Settings", L"dragDstColor", g_dragDstColor);
	getPrivateProfileBool(fileName, L"Settings", L"useShiftKey", g_useShiftKey);
	getPrivateProfileBool(fileName, L"Settings", L"useCtrlKey", g_useCtrlKey);
	getPrivateProfileBool(fileName, L"Settings", L"useAltKey", g_useAltKey);
	getPrivateProfileBool(fileName, L"Settings", L"useWinKey", g_useWinKey);
}

//--------------------------------------------------------------------

// vk が押されていなかった場合は TRUE を返す。
BOOL isKeyUp(UINT vk)
{
	return ::GetKeyState(vk) >= 0;
}

BOOL canBeginDrag()
{
	// 修飾キーが設定されているのにそのキーが押されていなかった場合は FALSE を返す。

	if (g_useShiftKey && isKeyUp(VK_SHIFT)) return FALSE;
	if (g_useCtrlKey && isKeyUp(VK_CONTROL)) return FALSE;
	if (g_useAltKey && isKeyUp(VK_MENU)) return FALSE;
	if (g_useWinKey && (isKeyUp(VK_LWIN) && isKeyUp(VK_RWIN))) return FALSE;

	return TRUE;
}

void moveTargetMarkWindow(const DialogInfo& di, FilterHolder filter, BOOL show)
{
	if (!TargetMark::g_enable)
		return;

	LPCSTR name = filter.getName();
//	MY_TRACE_STR(name);

	g_targetMarkWindow->Render(name, TargetMark::g_alpha);

	RECT rc; di.getFilterRect(filter, &rc);
	POINT pos;
	pos.x = (rc.left + rc.right) / 2;
	pos.y = (rc.top + rc.bottom) / 2;
	::ClientToScreen(di.getDialog(), &pos);
	if (show)
	{
		g_targetMarkWindow->Show(pos.x, pos.y);
	}
	else
	{
		g_targetMarkWindow->Move(pos.x, pos.y);
	}
}

void beginDrag(const DialogInfo& di)
{
	MY_TRACE(_T("beginDrag()\n"));

	// マウスをキャプチャーする。
	::SetCapture(di.getDialog());
	// フラグを立てる。
	g_isFilterDragging = TRUE;

	// ドラッグ元をマークする。
	showMarkWindow(g_dragSrcWindow, di, g_srcFilter);

	// ターゲットマークを表示する。
	moveTargetMarkWindow(di, g_srcFilter, TRUE);
}

void endDrag()
{
	MY_TRACE(_T("endDrag()\n"));

	// ドラッグフラグなどをリセットする。
	g_isFilterDragging = FALSE;
	hideMarkWindow(g_dragSrcWindow);
	hideMarkWindow(g_dragDstWindow);

	g_targetMarkWindow->Hide();
}

void moveDrag(const DialogInfo& di)
{
	MY_TRACE(_T("moveDrag()\n"));

	if (g_dstFilter != g_srcFilter)
	{
		// ドラッグ先をマークする。
		showMarkWindow(g_dragDstWindow, di, g_dstFilter);
	}
	else
	{
		// ドラッグ先のマークを隠す。
		hideMarkWindow(g_dragDstWindow);
	}

	// ドラッグ元のマークを再配置する。
	showMarkWindow(g_dragSrcWindow, di, g_srcFilter);

	// ターゲットマークを動かす。
	moveTargetMarkWindow(di, g_dstFilter, FALSE);
}

IMPLEMENT_HOOK_PROC_NULL(LRESULT, WINAPI, SettingDialogProc, (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam))
{
	switch (message)
	{
	case WM_CREATE:
		{
			MY_TRACE(_T("WM_CREATE\n"));

			// 設定ダイアログのコンテキストメニューを拡張する。
			for (int i = 0; i < g_auin.GetSettingDialogMenuCount(); i++)
			{
				if (i != 2)
					continue;

				HMENU menu = g_auin.GetSettingDialogMenu(i);
				HMENU subMenu = ::GetSubMenu(menu, 0);
				::AppendMenu(subMenu, MF_SEPARATOR, 0, 0);
				::AppendMenu(subMenu, MF_STRING, ID_CREATE_CLONE, _T("完全な複製を隣に作成"));
				::AppendMenu(subMenu, MF_STRING, ID_CREATE_SAME_ABOVE, _T("同じフィルタ効果を上に作成"));
				::AppendMenu(subMenu, MF_STRING, ID_CREATE_SAME_BELOW, _T("同じフィルタ効果を下に作成"));
			}

			break;
		}
	case WM_SETCURSOR:
		{
			if ((HWND)wParam == hwnd && LOWORD(lParam) == HTCLIENT)
			{
//				MY_TRACE(_T("WM_SETCURSOR\n"));

				// ドラッグを開始できるかチェックする。
				if (!canBeginDrag())
					break;

				DialogInfo di(hwnd);

				// マウスカーソルの位置を取得する。
				CursorPos pos(hwnd);
//				MY_TRACE_POINT(pos);

				// オブジェクトを取得する。
				ObjectHolder object(g_auin.GetCurrentObjectIndex());
//				MY_TRACE_OBJECT_HOLDER(object);
				if (!object.isValid()) break;

				// マウスカーソルの位置に移動できるフィルタがあるかチェックする。
				FilterHolder filter = di.getSrcFilter(pos, object);
//				MY_TRACE_FILTER_HOLDER(filter);
				if (!filter.isValid()) break;

				// マウスカーソルを変更する。
				::SetCursor(::LoadCursor(0, IDC_SIZENS));
				return TRUE;
			}

			break;
		}
	case WM_LBUTTONDOWN:
		{
			MY_TRACE(_T("WM_LBUTTONDOWN\n"));

			// ドラッグを開始できるかチェックする。
			if (!canBeginDrag())
				break;

			DialogInfo di(hwnd);

			// マウスカーソルの位置を取得する。
			CursorPos pos(hwnd);
			MY_TRACE_POINT(pos);

			// オブジェクトを取得する。
			g_srcObject = ObjectHolder(g_auin.GetCurrentObjectIndex());
			MY_TRACE_OBJECT_HOLDER(g_srcObject);
			if (!g_srcObject.isValid()) break;

			// マウスカーソルの位置にあるドラッグ元フィルタを取得する。
			g_srcFilter = di.getSrcFilter(pos, g_srcObject);
			MY_TRACE_FILTER_HOLDER(g_srcFilter);
			g_dstFilter = g_srcFilter;
			if (!g_srcFilter.isValid()) break;

			MY_TRACE(_T("フィルタのドラッグを開始します\n"));

			// ドラッグを開始する。
			beginDrag(hwnd);

			break;
		}
	case WM_LBUTTONUP:
		{
			MY_TRACE(_T("WM_LBUTTONUP\n"));

			// リセットする前にフィルタをドラッグ中だったかチェックする。
			BOOL isDragging = ::GetCapture() == hwnd && g_isFilterDragging;

			// ドラッグを終了する。
			endDrag();

			// ドラッグ中だったならフィルタの移動を行う。
			if (isDragging)
			{
				::ReleaseCapture(); // ここで WM_CAPTURECHANGED が呼ばれる。

				MY_TRACE_FILTER_HOLDER(g_srcFilter);
				MY_TRACE_FILTER_HOLDER(g_dstFilter);

				// オブジェクトを取得する。
				ObjectHolder object(g_auin.GetCurrentObjectIndex());
				MY_TRACE_OBJECT_HOLDER(object);
				if (!object.isValid() || object != g_srcObject)
				{
					MY_TRACE(_T("ドラッグ開始時のオブジェクトではないのでフィルタの移動を中止します\n"));
					::ReleaseCapture(); endDrag();
					break;
				}

				// フィルタを取得する。
				FilterHolder filter(g_srcObject, g_srcFilter.getFilterIndex());
				MY_TRACE_FILTER_HOLDER(filter);
				if (!filter.isValid() || filter != g_srcFilter)
				{
					MY_TRACE(_T("ドラッグ開始時のフィルタではないのでフィルタの移動を中止します\n"));
					::ReleaseCapture(); endDrag();
					break;
				}

				int srcFilterIndex = g_srcFilter.getFilterIndex();
				MY_TRACE_INT(srcFilterIndex);

				int dstFilterIndex = g_dstFilter.getFilterIndex();
				MY_TRACE_INT(dstFilterIndex);

				// フィルタのインデックスの差分を取得する。
				int sub = dstFilterIndex - srcFilterIndex;
				MY_TRACE_INT(sub);

				if (sub != 0)
				{
					MY_TRACE(_T("ドラッグ先にフィルタを移動します\n"));

					g_moveFilterFlag = TRUE;
					::SendMessage(hwnd, WM_COMMAND, sub < 0 ? 1116 : 1117, 0);
					g_moveFilterFlag = FALSE;
				}
			}

			break;
		}
	case WM_MOUSEMOVE:
		{
			// フィルタをドラッグ中かチェックする。
			if (::GetCapture() == hwnd && g_isFilterDragging)
			{
				// オブジェクトを取得する。
				ObjectHolder object(g_auin.GetCurrentObjectIndex());
//				MY_TRACE_OBJECT_HOLDER(object);
				if (!object.isValid() || object != g_srcObject)
				{
					MY_TRACE(_T("ドラッグ開始時のオブジェクトではないのでドラッグを中止します\n"));
					::ReleaseCapture(); endDrag();
					break;
				}

				// フィルタを取得する。
				FilterHolder filter(g_srcObject, g_srcFilter.getFilterIndex());
//				MY_TRACE_FILTER_HOLDER(filter);
				if (!filter.isValid() || filter != g_srcFilter)
				{
					MY_TRACE(_T("ドラッグ開始時のフィルタではないのでドラッグを中止します\n"));
					::ReleaseCapture(); endDrag();
					break;
				}

				DialogInfo di(hwnd);

				// マウスカーソルの位置を取得する。
				CursorPos pos(hwnd);
//				MY_TRACE_POINT(pos);

				FilterHolder oldDstFilter = g_dstFilter;

				// マウスカーソルの位置にあるドラッグ元フィルタを取得する。
				g_dstFilter = di.getDstFilter(pos, g_srcObject);
				if (!g_dstFilter.isValid()) g_dstFilter = g_srcFilter;
//				MY_TRACE_FILTER_HOLDER(g_dstFilter);

				if (g_dstFilter != oldDstFilter)
				{
					// マークを動かす。
					moveDrag(di);
				}
			}

			break;
		}
	case WM_CAPTURECHANGED:
		{
			MY_TRACE(_T("WM_CAPTURECHANGED\n"));

			endDrag();

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
					// オブジェクトを取得する。
					ObjectHolder object(g_auin.GetCurrentObjectIndex());
					MY_TRACE_OBJECT_HOLDER(object);
					if (!object.isValid()) break;

					// フィルタを取得する。
					FilterHolder filter = FilterHolder(object, g_auin.GetCurrentFilterIndex());
					MY_TRACE_FILTER_HOLDER(filter);
					if (!filter.isValid()) break;
					MY_TRACE_STR(filter.getFilter()->name);
					MY_TRACE_HEX(filter.getFilter()->flag);

					// フィルタが複製できるものかどうかチェックする。
					if (!filter.isMoveable())
						break;

					// フィルタ ID を取得する。
					int filterId = object.getObject()->filter_param[filter.getFilterIndex()].id;
					MY_TRACE_HEX(filterId);
					if (filterId < 0) break;

					// フィルタを作成するコマンドを発行する。
					g_createCloneId = wParam;
					LRESULT result = true_SettingDialogProc(hwnd, message, 2000 + filterId, lParam);
					g_createCloneId = 0;
					return result;
				}
			}

			break;
		}
	}

	return true_SettingDialogProc(hwnd, message, wParam, lParam);
}

//--------------------------------------------------------------------

BOOL checkModifier(WPARAM wParam)
{
	// 設定ダイアログのウィンドウハンドルを取得する。
	HWND hwnd = g_auin.GetSettingDialog();

	// ::WindowFromPoint() の戻り値が設定ダイアログではない場合は何もしない。
	POINT pos; ::GetCursorPos(&pos);
	if (hwnd != ::WindowFromPoint(pos))
		return FALSE;

	// 修飾キーに指定されたキーの状態が変更された場合はマウスカーソルを更新する。
	if (
		(g_useShiftKey && wParam == VK_SHIFT) ||
		(g_useCtrlKey && wParam == VK_CONTROL) ||
		(g_useAltKey && wParam == VK_MENU) ||
		(g_useWinKey && (wParam == VK_LWIN || wParam == VK_RWIN))
		)
	{
		::PostMessage(hwnd, WM_SETCURSOR, (WPARAM)hwnd, HTCLIENT);

		return TRUE;
	}

	return FALSE;
}

LRESULT CALLBACK keyboardHookProc(int code, WPARAM wParam, LPARAM lParam)
{
	if (code != HC_ACTION)
		return ::CallNextHookEx(g_keyboardHook, code, wParam, lParam);

	checkModifier(wParam);

	return ::CallNextHookEx(g_keyboardHook, code, wParam, lParam);
}

//--------------------------------------------------------------------
