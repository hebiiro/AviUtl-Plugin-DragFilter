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

int* g_objectIndex = 0; // カレントオブジェクトのインデックスへのポインタ。
DWORD* g_objectData = 0; // オブジェクトデータへのポインタ。
int* g_filterPosY = 0; // フィルタの Y 座標配列へのポインタ。

const int FILTER_HEADER_HEIGHT = 16; // フィルタの先頭付近の高さ。

// フィルタの Y 座標を返す。
int GetFilterPosY(int filterIndex, LPCRECT rc)
{
	return g_filterPosY[filterIndex + 1] - FILTER_HEADER_HEIGHT / 2;
}

// フィルタの高さを返す。
int GetFilterHeight(int filterIndex, LPCRECT rc)
{
	int y1 = GetFilterPosY(filterIndex, rc);
	int y2 = GetFilterPosY(filterIndex + 1, rc);
	if (y2 > 0) return y2 - y1;
	return rc->bottom - y1;
}

//---------------------------------------------------------------------

// マークウィンドウのウィンドウ関数。
LRESULT CALLBACK markWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC dc = ::BeginPaint(hwnd, &ps);
			// color で塗りつぶす。
			COLORREF color = (COLORREF)::GetProp(hwnd, _T("DragFilter.Color"));
			HBRUSH brush = ::CreateSolidBrush(color);
			FillRect(dc, &ps.rcPaint, brush);
			::DeleteObject(brush);
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
	RECT rc; ::GetClientRect(g_exeditObjectDialog, &rc);
	int x = rc.left;
	int y = GetFilterPosY(filterIndex, &rc);
	int w = rc.right - rc.left;
	int h = GetFilterHeight(filterIndex, &rc);

	POINT pos = { x, y };
	::ClientToScreen(g_exeditObjectDialog, &pos);
	::SetLayeredWindowAttributes(hwnd, 0, 92, LWA_ALPHA);
	::SetWindowPos(hwnd, HWND_TOPMOST, pos.x, pos.y, w, h, SWP_NOACTIVATE | SWP_SHOWWINDOW);
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
	GetFilterIndexFromY = (Type_GetFilterIndexFromY)(exedit + 0x2CC30);
	PushUndo = (Type_PushUndo)(exedit + 0x8D150);
	CreateUndo = (Type_CreateUndo)(exedit + 0x8D290);
	SwapFilter = (Type_SwapFilter)(exedit + 0x33B30);
	DrawObjectDialog = (Type_DrawObjectDialog)(exedit + 0x39490);
	HideControls = (Type_HideControls)(exedit + 0x30500);
	ShowControls = (Type_ShowControls)(exedit + 0x305E0);
	g_objectIndex = (int*)(exedit + 0x177A10);
	g_objectData = (DWORD*)(exedit + 0x1E0FA4);
	g_filterPosY = (int*)(exedit + 0x196714);

	// 拡張編集の関数をフックする。
	DetourTransactionBegin();
	DetourUpdateThread(::GetCurrentThread());

	ATTACH_HOOK_PROC(Exedit_ObjectDialog_WndProc);

	if (DetourTransactionCommit() == NO_ERROR)
	{
		MY_TRACE(_T("拡張編集のフックに成功しました\n"));
	}
	else
	{
		MY_TRACE(_T("拡張編集のフックに失敗しました\n"));
	}
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
	case WM_LBUTTONDOWN:
		{
			MY_TRACE(_T("WM_LBUTTONDOWN\n"));

			POINT pos;
			pos.x = GET_X_LPARAM(lParam);
			pos.y = GET_Y_LPARAM(lParam);

			// クリックしたフィルタを記憶しておく。
			g_srcFilterIndex = GetFilterIndexFromY(pos.y);
			MY_TRACE_INT(g_srcFilterIndex);
			g_dstFilterIndex = g_srcFilterIndex;

			if (g_srcFilterIndex > 0)
			{
				MY_TRACE(_T("フィルタのドラッグを開始します\n"));

				// マウスカーソルを変えて、マウスをキャプチャーする。
				::SetCursor(::LoadCursor(0, IDC_SIZENS));
				::SetCapture(hwnd);
				// フラグを立てる。
				g_isFilterDragging = TRUE;

				// ドラッグ元をマークする。
				showMarkWindow(g_dragSrcWindow, g_srcFilterIndex);
			}

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
				DWORD EAX = *g_objectIndex;
				DWORD EDX = EAX*8+EAX;
				DWORD ECX = EDX*4+EAX;
				EDX = *g_objectData;
				ECX = ECX*4+ECX;
				DWORD ESI = *(DWORD*)(ECX*8+EDX+0x50);
				if ((LONG)ESI < 0) ESI = EAX;
				int objectIndex = ESI;
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
				POINT pos;
				pos.x = GET_X_LPARAM(lParam);
				pos.y = GET_Y_LPARAM(lParam);

				// マウスカーソルの位置にあるフィルタを取得する。
				g_dstFilterIndex = GetFilterIndexFromY(pos.y);
				if (g_dstFilterIndex <= 0) g_dstFilterIndex = 1;
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
	}

	return true_Exedit_ObjectDialog_WndProc(hwnd, message, wParam, lParam);
}

//---------------------------------------------------------------------
//		フィルタ構造体のポインタを渡す関数
//---------------------------------------------------------------------
EXTERN_C FILTER_DLL __declspec(dllexport) * __stdcall GetFilterTable(void)
{
	static TCHAR g_filterName[] = TEXT("フィルタドラッグ移動");
	static TCHAR g_filterInformation[] = TEXT("フィルタドラッグ移動 version 3.0.0 by 蛇色");

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
