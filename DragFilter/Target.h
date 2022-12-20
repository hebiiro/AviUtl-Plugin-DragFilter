#pragma once

//--------------------------------------------------------------------
namespace TargetMark {
//--------------------------------------------------------------------

extern BOOL g_enable;
extern BYTE g_alpha;
extern Color g_penColor;
extern REAL g_penWidth;
extern Color g_brushColor;
extern int g_base;
extern int g_width;
extern _bstr_t g_fontName;
extern REAL g_fontSize;
extern REAL g_rotate;
extern Point g_beginMove;

//--------------------------------------------------------------------
} // namespace TargetMark
//--------------------------------------------------------------------

class LayeredWindowInfo
{
private:
	POINT m_sourcePosition;
	POINT m_windowPosition;
	SIZE m_size;
	BLENDFUNCTION m_blend;
	UPDATELAYEREDWINDOWINFO m_info;

public:

	LayeredWindowInfo(UINT width, UINT height)
		: m_sourcePosition()
		, m_windowPosition()
		, m_size()
		, m_blend()
		, m_info()
	{
		m_size.cx = width;
		m_size.cy = height;

		m_info.cbSize = sizeof(UPDATELAYEREDWINDOWINFO);
		m_info.pptSrc = &m_sourcePosition;
		m_info.pptDst = &m_windowPosition;
		m_info.psize = &m_size;
		m_info.pblend = &m_blend;
		m_info.dwFlags = ULW_ALPHA;

		m_blend.SourceConstantAlpha = 255;
		m_blend.AlphaFormat = AC_SRC_ALPHA;
	}

	void Update(HWND window, HDC source, BYTE alpha)
	{
		m_info.hdcSrc = source;
		m_blend.SourceConstantAlpha = alpha;
		BOOL result = ::UpdateLayeredWindowIndirect(window, &m_info);
		DWORD error = ::GetLastError();
		MY_TRACE(_T("error = %d, 0x%08X\n"), result, error);
	}

	UINT GetWidth() const { return m_size.cx; }
	UINT GetHeight() const { return m_size.cy; }
	void SetWindowPosition(int x, int y)
	{
		m_windowPosition.x = x;
		m_windowPosition.y = y;
	}
};

class GdiBitmap
{
	const UINT m_width;
	const UINT m_height;
	const UINT m_stride;
	void* m_bits;

	HDC m_dc;
	HBITMAP m_bitmap;
	HBITMAP m_oldBitmap;

public:

	GdiBitmap(UINT width, UINT height)
		: m_width(width)
		, m_height(height)
		, m_stride((width * 32 + 31) / 32 * 4)
		, m_bits(0)
		, m_dc(0)
		, m_bitmap(0)
		, m_oldBitmap(0)
	{
		m_dc = ::CreateCompatibleDC(0);
		MY_TRACE_HEX(m_dc);

		BITMAPINFO bitmapInfo = {};
		bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
		bitmapInfo.bmiHeader.biWidth = width;
		bitmapInfo.bmiHeader.biHeight = 0 - height;
		bitmapInfo.bmiHeader.biPlanes = 1;
		bitmapInfo.bmiHeader.biBitCount = 32;
		bitmapInfo.bmiHeader.biCompression = BI_RGB;
		m_bitmap = ::CreateDIBSection(
			0, &bitmapInfo, DIB_RGB_COLORS, &m_bits, 0, 0);
		MY_TRACE_HEX(m_bitmap);

		m_oldBitmap = (HBITMAP)::SelectObject(m_dc, m_bitmap);
		MY_TRACE_HEX(m_oldBitmap);
	}

	~GdiBitmap()
	{
		::SelectObject(m_dc, m_oldBitmap);
		::DeleteObject(m_bitmap);
		::DeleteDC(m_dc);
	}

	UINT GetWidth() const { return m_width; }
	UINT GetHeight() const { return m_height; }
	UINT GetStride() const { return m_stride; }
	void* GetBits() const { return m_bits; }
	HDC GetDC() const { return m_dc; }
};

class UxBitmap
{
private:

	HDC m_dc;
	HPAINTBUFFER m_pb;

public:

	UxBitmap(int width, int height)
		: m_dc(0)
		, m_pb(0)
	{
		HDC dc = ::GetDC(0);

		RECT rc = { 0, 0, width, height };
		BP_PAINTPARAMS pp = { sizeof(pp) };
		m_pb = ::BeginBufferedPaint(dc, &rc, BPBF_TOPDOWNDIB, &pp, &m_dc);
		MY_TRACE_HEX(m_pb);
		MY_TRACE_HEX(m_dc);

		::ReleaseDC(0, dc);
	}

	~UxBitmap()
	{
		::EndBufferedPaint(m_pb, TRUE);
	}

	HDC GetDC() const { return m_dc; }
};

class TargetMarkWindow
{
public:

	LayeredWindowInfo m_info;
	GdiBitmap m_bitmap;
//	UxBitmap m_bitmap;
	Graphics m_graphics;

	HWND m_hwnd;
	POINT m_beginPos;
	POINT m_endPos;
	POINT m_currentPos;
	int m_currentCount;

	static const UINT TIMER_ID = 1000;
	static const int MAX_COUNT = 30;

public:

	TargetMarkWindow()
		: m_info(600, 400)
		, m_bitmap(m_info.GetWidth(), m_info.GetHeight())
		, m_graphics(m_bitmap.GetDC())
		, m_hwnd(0)
		, m_beginPos()
		, m_endPos()
		, m_currentPos()
		, m_currentCount()
	{
		m_graphics.SetSmoothingMode(SmoothingModeAntiAlias);
//		m_graphics.SetCompositingMode(CompositingModeSourceOver);
//		m_graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);
		m_graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
//		m_graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
		m_graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
		m_graphics.TranslateTransform(-0.5f, -0.5f);
	}

	void Create(HINSTANCE instance)
	{
		MY_TRACE(_T("TargetMarkWindow::Create()\n"));

		WNDCLASS wc = {};
		wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_NOCLOSE;
		wc.hCursor = ::LoadCursor(0, IDC_ARROW);
		wc.lpfnWndProc = wndProc;
		wc.hInstance = instance;
		wc.lpszClassName = _T("DragFilter.Target");
		::RegisterClass(&wc);

		m_hwnd = ::CreateWindowEx(
			WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
			_T("DragFilter.Target"),
			_T("DragFilter.Target"),
			WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
			0, 0, 0, 0,
			0, 0, instance, 0);
		MY_TRACE_HEX(m_hwnd);

		::SetProp(m_hwnd, _T("DragFilter.Target"), this);
	}

	void Destroy()
	{
		MY_TRACE(_T("TargetMarkWindow::Destroy()\n"));

		::DestroyWindow(m_hwnd);
	}

	void Render(LPCSTR name, BYTE alpha)
	{
		MY_TRACE(_T("TargetMarkWindow::Render(%hs, %d)\n"), name, alpha);

		Pen pen(TargetMark::g_penColor, TargetMark::g_penWidth);
		SolidBrush brush(TargetMark::g_brushColor);

		Status status = m_graphics.Clear(Color(0, 0, 0, 0));

		int w = m_info.GetWidth();
		int h = m_info.GetHeight();

		int cx = 0;
		int cy = 0;

		int i = TargetMark::g_base;
		int width = TargetMark::g_width;

		REAL ox = w / 6.0f;
		REAL oy = h / 2.0f;

		Matrix matrix;
		m_graphics.GetTransform(&matrix);
		m_graphics.TranslateTransform(ox, oy);

		m_graphics.RotateTransform(TargetMark::g_rotate);

		GraphicsPath cornerPath;
		GraphicsPath linePath;
		GraphicsPath centerPath;

		{
			Point points[6];
			points[0].X = cx - i * 3;
			points[0].Y = cy - i * 3;
			points[1].X = cx - i * 1;
			points[1].Y = cy - i * 3;
			points[2].X = points[1].X;
			points[2].Y = points[1].Y + width;
			points[3].X = points[0].X + width;
			points[3].Y = points[0].Y + width;
			points[5].X = cx - i * 3;
			points[5].Y = cy - i * 1;
			points[4].X = points[5].X + width;
			points[4].Y = points[5].Y;
			cornerPath.AddPolygon(points, 6);

			linePath.AddRectangle(Rect(cx - i * 5, cy - width / 2, i * 3, width));

			centerPath.AddEllipse(Rect(cx - i * 0 - width / 2, cy - i * 0 - width / 2, width, width));
		}

		m_graphics.DrawPath(&pen, &centerPath);
		m_graphics.FillPath(&brush, &centerPath);

		for (int i = 0; i < 4; i++)
		{
//			if (i != 0)
			{
				m_graphics.DrawPath(&pen, &cornerPath);
				m_graphics.FillPath(&brush, &cornerPath);
			}
//			if (i != 1)
			{
			m_graphics.DrawPath(&pen, &linePath);
			m_graphics.FillPath(&brush, &linePath);
			}

			m_graphics.RotateTransform(90.0f);
		}

//		if (0)
		{
			FontFamily fontFamily(TargetMark::g_fontName);
			StringFormat stringFormat;
			stringFormat.SetAlignment(StringAlignmentNear);
			stringFormat.SetLineAlignment(StringAlignmentFar);
			GraphicsPath namePath;
			namePath.AddString((_bstr_t)name, -1, &fontFamily,
				FontStyleBold, TargetMark::g_fontSize,
				Point(cx + i * 3 + width, cy - width), &stringFormat);
			m_graphics.DrawPath(&pen, &namePath);
			m_graphics.FillPath(&brush, &namePath);
		}

		m_graphics.SetTransform(&matrix);

		m_info.SetWindowPosition(m_currentPos.x, m_currentPos.y);
		m_info.Update(m_hwnd, m_bitmap.GetDC(), alpha);
	}

	void Show(int x, int y)
	{
		MY_TRACE(_T("TargetMarkWindow::Show(%d, %d)\n"), x, y);
#if 1
		m_beginPos.x = x - m_info.GetWidth() / 2 + TargetMark::g_beginMove.X;
		m_beginPos.y = y - m_info.GetHeight() / 2 + TargetMark::g_beginMove.Y;
		m_endPos.x = m_beginPos.x;
		m_endPos.y = m_beginPos.y;
		m_currentPos.x = m_beginPos.x;
		m_currentPos.y = m_beginPos.y;
		m_currentCount = 0;

		::SetWindowPos(m_hwnd, HWND_TOPMOST,
			m_beginPos.x, m_beginPos.y,
			m_info.GetWidth(), m_info.GetHeight(),
			SWP_NOACTIVATE | SWP_SHOWWINDOW);

		Move(x, y);
#else
		m_beginPos.x = x - m_info.GetWidth() / 2;
		m_beginPos.y = y - m_info.GetHeight() / 2;
		m_endPos.x = m_beginPos.x;
		m_endPos.y = m_beginPos.y;
		m_currentPos.x = m_beginPos.x;
		m_currentPos.y = m_beginPos.y;
		m_currentCount = 0;

		::SetWindowPos(m_hwnd, HWND_TOPMOST,
			m_beginPos.x, m_beginPos.y,
			m_info.GetWidth(), m_info.GetHeight(),
			SWP_NOACTIVATE | SWP_SHOWWINDOW);
#endif
	}

	void Hide()
	{
		MY_TRACE(_T("TargetMarkWindow::Hide()\n"));

		::ShowWindow(m_hwnd, SW_HIDE);
//		::KillTimer(m_hwnd, TIMER_ID);
	}

	void Move(int x, int y)
	{
		m_beginPos.x = m_currentPos.x;
		m_beginPos.y = m_currentPos.y;
		m_endPos.x = x - m_info.GetWidth() / 2;
		m_endPos.y = y - m_info.GetHeight() / 2;
		m_currentCount = MAX_COUNT;

		MoveInternal();
	}

	void MoveInternal()
	{
		if (m_currentCount == MAX_COUNT)
			::SetTimer(m_hwnd, TIMER_ID, 10, 0);

		double a = (double)m_currentCount / MAX_COUNT;
		a = a * a;
		double b = 1.0 - a;
		double tolerance = 0.0001;
		m_currentPos.x = (int)(m_beginPos.x * a + m_endPos.x * b + tolerance);
		m_currentPos.y = (int)(m_beginPos.y * a + m_endPos.y * b + tolerance);

		::SetWindowPos(m_hwnd, HWND_TOPMOST,
			m_currentPos.x, m_currentPos.y, 0, 0,
			SWP_NOSIZE | SWP_NOACTIVATE);

		if (m_currentCount <= 0)
			::KillTimer(m_hwnd, TIMER_ID);

		m_currentCount--;
	}

	static LRESULT CALLBACK wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_TIMER:
			{
				if (wParam == TIMER_ID)
				{
					TargetMarkWindow* p = (TargetMarkWindow*)::GetProp(hwnd, _T("DragFilter.Target"));

					p->MoveInternal();
				}

				break;
			}
		}

		return ::DefWindowProc(hwnd, message, wParam, lParam);
	}
};

//--------------------------------------------------------------------
