#include "pch.h"
#include "DragFilter.h"

//--------------------------------------------------------------------
//		初期化
//--------------------------------------------------------------------

BOOL func_init(AviUtl::FilterPlugin *fp)
{
	MY_TRACE(_T("func_init()\n"));

	g_auin.initExEditAddress();

	// 設定ファイルを読み込む。
	WCHAR fileName[MAX_PATH] = {};
	::GetModuleFileNameW(g_instance, fileName, MAX_PATH);
	::PathRenameExtensionW(fileName, L".ini");
	MY_TRACE_WSTR(fileName);
	g_settingsFile = FileUpdateCheckerPtr(new FileUpdateChecker(fileName));
	loadSettings(g_settingsFile->getFilePath());

	initHook();

	return TRUE;
}

//--------------------------------------------------------------------
//		終了
//--------------------------------------------------------------------
BOOL func_exit(AviUtl::FilterPlugin *fp)
{
	MY_TRACE(_T("func_exit()\n"));

	g_settingsFile = 0;

	termHook();

	return TRUE;
}

BOOL func_WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, AviUtl::EditHandle* editp, AviUtl::FilterPlugin* fp)
{
	// TRUE を返すと全体が再描画される

	switch (message)
	{
	case AviUtl::detail::FilterPluginWindowMessage::Init:
		{
			MY_TRACE(_T("func_WndProc(Init, 0x%08X, 0x%08X)\n"), wParam, lParam);

			// このフィルタのウィンドウハンドルを保存しておく。
			g_filterWindow = fp->hwnd;
			MY_TRACE_HEX(g_filterWindow);
			::SetTimer(g_filterWindow, TIMER_ID_CHECK_UPDATE, 1000, 0);

			// マークウィンドウを作成する。
			g_dragSrcWindow = createMarkWindow(RGB(0x00, 0x00, 0xff));
			MY_TRACE_HEX(g_dragSrcWindow);
			g_dragDstWindow = createMarkWindow(RGB(0xff, 0x00, 0x00));
			MY_TRACE_HEX(g_dragDstWindow);

			g_targetMarkWindow = TargetMarkWindowPtr(new TargetMarkWindow());
			g_targetMarkWindow->Create(g_instance);

			break;
		}
	case AviUtl::detail::FilterPluginWindowMessage::Exit:
		{
			MY_TRACE(_T("func_WndProc(Exit, 0x%08X, 0x%08X)\n"), wParam, lParam);

			// マークウィンドウを削除する。
			::DestroyWindow(g_dragSrcWindow), g_dragSrcWindow = 0;
			::DestroyWindow(g_dragDstWindow), g_dragDstWindow = 0;

			g_targetMarkWindow->Destroy();
			g_targetMarkWindow = 0;

			break;
		}
	case AviUtl::detail::FilterPluginWindowMessage::Command:
		{
			MY_TRACE(_T("func_WndProc(Command, 0x%08X, 0x%08X)\n"), wParam, lParam);

			if (wParam == 0 && lParam == 0)
			{
				MY_TRACE(_T("フレームを更新します\n"));
				return TRUE;
			}

			break;
		}
	case WM_TIMER:
		{
			if (wParam == TIMER_ID_CHECK_UPDATE)
			{
				if (g_settingsFile->isFileUpdated())
					loadSettings(g_settingsFile->getFilePath());
			}

			break;
		}
	}

	return FALSE;
}

//--------------------------------------------------------------------

EXTERN_C AviUtl::FilterPluginDLL* CALLBACK GetFilterTable()
{
	LPCSTR name = "フィルタドラッグ移動";
	LPCSTR information = "フィルタドラッグ移動 9.1.0 by 蛇色";

	static AviUtl::FilterPluginDLL filter =
	{
		.flag =
//			AviUtl::detail::FilterPluginFlag::NoConfig | // このフラグを指定するとウィンドウが作成されなくなってしまう。
			AviUtl::detail::FilterPluginFlag::AlwaysActive | // このフラグがないと「フィルタ」に ON/OFF を切り替えるための項目が追加されてしまう。
			AviUtl::detail::FilterPluginFlag::DispFilter | // このフラグがないと「設定」の方にウィンドウを表示するための項目が追加されてしまう。
			AviUtl::detail::FilterPluginFlag::WindowThickFrame |
			AviUtl::detail::FilterPluginFlag::WindowSize |
			AviUtl::detail::FilterPluginFlag::ExInformation,
		.x = 400,
		.y = 400,
		.name = name,
		.func_init = func_init,
		.func_exit = func_exit,
		.func_WndProc = func_WndProc,
		.information = information,
	};

	return &filter;
}

//--------------------------------------------------------------------

EXTERN_C BOOL APIENTRY DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		{
			// ロケールを設定する。
			// これをやらないと日本語テキストが文字化けするので最初に実行する。
			_tsetlocale(LC_CTYPE, _T(""));

			MY_TRACE(_T("DLL_PROCESS_ATTACH\n"));

			// この DLL のハンドルをグローバル変数に保存しておく。
			g_instance = instance;
			MY_TRACE_HEX(g_instance);

			// この DLL の参照カウンタを増やしておく。
			WCHAR moduleFileName[MAX_PATH] = {};
			::GetModuleFileNameW(g_instance, moduleFileName, MAX_PATH);
			::LoadLibraryW(moduleFileName);

			break;
		}
	case DLL_PROCESS_DETACH:
		{
			MY_TRACE(_T("DLL_PROCESS_DETACH\n"));

			break;
		}
	}

	return TRUE;
}

//--------------------------------------------------------------------
