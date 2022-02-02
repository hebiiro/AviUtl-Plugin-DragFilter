#include "pch.h"
#include "DragFilter.h"

//---------------------------------------------------------------------

// デバッグ用コールバック関数。デバッグメッセージを出力する
void ___outputLog(LPCTSTR text, LPCTSTR output)
{
	::OutputDebugString(output);
}

//---------------------------------------------------------------------

// この DLL のインスタンスハンドル。
HINSTANCE g_instance = 0;
// このプラグインのウィンドウハンドル。
HWND g_filterWindow = 0;
// 拡張編集のオブジェクトダイアログのハンドル。
HWND g_exeditObjectDialog = 0;

// クリックしたフィルタのインデックス。
int g_filterIndex = 0;
// ドラッグ中か判定するためのフラグ。
BOOL g_isFilterDragging = FALSE;

// カレントオブジェクトのインデックスへのポインタ。
int* g_objectIndex = 0;
// オブジェクトデータへのポインタ。
DWORD* g_objectData = 0;

//---------------------------------------------------------------------

// フックをセットする。
void initHook()
{
	MY_TRACE(_T("initHook()\n"));

	DetourTransactionBegin();
	DetourUpdateThread(::GetCurrentThread());

	ATTACH_HOOK_PROC(CreateWindowExA);

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
			g_filterIndex = GetFilterIndexFromY(pos.y);
			MY_TRACE_INT(g_filterIndex);

			if (g_filterIndex > 0)
			{
				MY_TRACE(_T("フィルタのドラッグを開始します\n"));

				::SetCursor(::LoadCursor(0, IDC_SIZENS));
				::SetCapture(hwnd);
				g_isFilterDragging = TRUE;
			}

			break;
		}
	case WM_LBUTTONUP:
		{
			MY_TRACE(_T("WM_LBUTTONUP\n"));

			// マウスをキャプチャーしているかチェックする。
			if (::GetCapture() == hwnd)
			{
				::ReleaseCapture();

				// フィルタをドラッグ中かチェックする。
				if (g_isFilterDragging)
				{
					// オブジェクトのインデックスを取得する。
					DWORD EAX = *g_objectIndex;
					DWORD EDX = EAX*8+EAX;
					DWORD ECX = EDX*4+EAX;
					EDX = *g_objectData;
					ECX = ECX*4+ECX;
					DWORD ESI = *(DWORD*)(ECX*8+EDX+0x50);
					if ((LONG)ESI < 0) ESI = EAX;
					int objectIndex = ESI;

					POINT pos;
					pos.x = GET_X_LPARAM(lParam);
					pos.y = GET_Y_LPARAM(lParam);

					// クリックアップした位置にあるフィルタを取得する。
					int filterIndex = GetFilterIndexFromY(pos.y);
					if (filterIndex <= 0) filterIndex = 1;
					MY_TRACE_INT(filterIndex);

					// インデックスの差分を取得する。
					int sub = filterIndex - g_filterIndex;

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
								SwapFilter(objectIndex, g_filterIndex--, -1);
						}
						else
						{
							// 下に移動
							for (int i = sub; i > 0; i--)
								SwapFilter(objectIndex, g_filterIndex++, 1);
						}

						// ダイアログを再描画する。
						DrawObjectDialog(objectIndex);
						// コントロール群を再配置する。
						HideControls();
						ShowControls(*g_objectIndex);

						::PostMessage(g_filterWindow, WM_FILTER_COMMAND, 0, 0);
					}
				}
			}

			// ドラッグフラグをリセットする。
			g_isFilterDragging = FALSE;

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
	static TCHAR g_filterInformation[] = TEXT("フィルタドラッグ移動 version 1.0.0 by 蛇色");

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

	g_filterWindow = fp->hwnd;
	MY_TRACE_HEX(g_filterWindow);

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
