#pragma once

#include "DragFilter.h"

//---------------------------------------------------------------------

class FileUpdateChecker
{
private:

	WCHAR m_fileName[MAX_PATH];
	FILETIME m_fileTime;

public:

	FileUpdateChecker(LPCWSTR fileName)
		: m_fileName()
		, m_fileTime()
	{
		::StringCbCopyW(m_fileName, sizeof(m_fileName), fileName);
		getFileTime(fileName, &m_fileTime);
	}

	LPCWSTR getFileName() const
	{
		return m_fileName;
	}

	BOOL isFileUpdated()
	{
		FILETIME fileTime;
		if (!getFileTime(m_fileName, &fileTime)) return FALSE;
		if (!::CompareFileTime(&m_fileTime, &fileTime)) return FALSE;
		m_fileTime = fileTime;
		return TRUE;
	}

	static BOOL getFileTime(LPCWSTR filePath, FILETIME* fileTime)
	{
		HANDLE file = ::CreateFileW(filePath, GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
		if (file == INVALID_HANDLE_VALUE) return FALSE;
		BOOL retValue = ::GetFileTime(file, 0, 0, fileTime);
		::CloseHandle(file);
		return retValue;
	}
};

//---------------------------------------------------------------------

class CursorPos : public POINT
{
public:

	CursorPos(HWND hwnd)
	{
		::GetCursorPos(this);
		::ScreenToClient(hwnd, this);
	}
};

class ObjectHolder
{
private:

	int m_objectIndex;
	auls::EXEDIT_OBJECT* m_object;

public:

	ObjectHolder()
		: m_objectIndex(-1)
		, m_object(0)
	{
	}

	ObjectHolder(int objectIndex)
		: m_objectIndex(-1)
		, m_object(0)
	{
		m_objectIndex = objectIndex;
		if (m_objectIndex >= 0)
		{
			m_object = Exedit_GetObject(m_objectIndex);
			if (!m_object)
				m_objectIndex = -1;
		}
	}

	int getObjectIndex() const
	{
		return m_objectIndex;
	}

	auls::EXEDIT_OBJECT* getObject() const
	{
		return m_object;
	}

	BOOL isValid() const
	{
		if (m_objectIndex < 0) return FALSE;
		if (!m_object) return FALSE;
		return TRUE;
	}

	bool operator==(const ObjectHolder& x) const
	{
		return m_objectIndex == x.m_objectIndex && m_object == x.m_object;
	}

	bool operator!=(const ObjectHolder& x) const
	{
		return !operator==(x);
	}
};

class FilterHolder
{
private:

	ObjectHolder m_object;
	int m_filterIndex;
	auls::EXEDIT_FILTER* m_filter;

public:

	FilterHolder()
		: m_filterIndex(-1)
		, m_filter(0)
	{
	}

	FilterHolder(ObjectHolder object, int filterIndex)
		: m_object(object)
		, m_filterIndex(-1)
		, m_filter(0)
	{
		if (m_object.isValid())
		{
			m_filterIndex = filterIndex;
			if (m_filterIndex >= 0)
			{
				m_filter = Exedit_GetFilter(m_object.getObject(), m_filterIndex);
				if (!m_filter)
					m_filterIndex = -1;
			}
		}
	}

	const ObjectHolder& getObject() const
	{
		return m_object;
	}

	int getFilterIndex() const
	{
		return m_filterIndex;
	}

	auls::EXEDIT_FILTER* getFilter() const
	{
		return m_filter;
	}

	BOOL isValid() const
	{
		if (m_filterIndex < 0) return FALSE;
		if (!m_filter) return FALSE;
		return TRUE;
	}

	bool operator==(const FilterHolder& x) const
	{
		return m_object == x.m_object && m_filterIndex == x.m_filterIndex && m_filter == x.m_filter;
	}

	bool operator!=(const FilterHolder& x) const
	{
		return !operator==(x);
	}

	BOOL isMoveable() const
	{
		int id = m_object.getObject()->filter_param[m_filterIndex].id;
		switch (id)
		{
		case 0x00: // 動画ファイル
		case 0x01: // 画像ファイル
		case 0x02: // 音声ファイル
		case 0x03: // テキスト
		case 0x04: // 図形
		case 0x05: // フレームバッファ
		case 0x06: // 音声波形
		case 0x07: // シーン
		case 0x08: // シーン(音声)
		case 0x09: // 直前オブジェクト
		case 0x0A: // 標準描画
		case 0x0B: // 拡張描画
		case 0x0C: // 標準再生
		case 0x0D: // パーティクル出力
		case 0x50: // カスタムオブジェクト
		case 0x5D: // 時間制御
		case 0x5E: // グループ制御
		case 0x5F: // カメラ制御
			{
				return FALSE;
			}
		}
		return TRUE;
	}

	LPCSTR getName() const
	{
		int objectIndex = getObject().getObjectIndex();
		int midptLeader = getObject().getObject()->index_midpt_leader;
		MY_TRACE_INT(midptLeader);
		if (midptLeader >= 0) objectIndex = midptLeader;

		ObjectHolder object(objectIndex);

		int id = object.getObject()->filter_param[m_filterIndex].id;
		if (id == 79) // アニメーション効果
		{
			BYTE* exdataTable = *g_objectExdata;
			DWORD offset = object.getObject()->ExdataOffset(m_filterIndex);
			BYTE* exdata = exdataTable + offset + 0x0004;

			return (LPCSTR)(exdata + 0x04);
		}
		else
		{
			return m_filter->name;
		}
	}
};

typedef std::vector<RECT> PaneContainer;

class DialogInfo
{
private:

	HWND m_dialog;
	PaneContainer m_panes;

	void getPaneRect()
	{
		for (int i = 0; i < auls::EXEDIT_OBJECT::MAX_FILTER; i++)
		{
			HWND arrow = ::GetDlgItem(m_dialog, 4500 + i);
			HWND name = ::GetDlgItem(m_dialog, 4400 + i);

			if (!::IsWindowVisible(arrow)) break;

			RECT arrowRect;
			::GetClientRect(arrow, &arrowRect);
			::MapWindowPoints(arrow, m_dialog, (LPPOINT)&arrowRect, 2);

			RECT nameRect;
			::GetClientRect(name, &nameRect);
			::MapWindowPoints(name, m_dialog, (LPPOINT)&nameRect, 2);

			RECT pane;
			pane.left = arrowRect.left;
			pane.top = arrowRect.top;
			pane.right = nameRect.right;
			pane.bottom = arrowRect.bottom;
			m_panes.push_back(pane);

			if (i > 0)
			{
				RECT& prev = m_panes[i - 1];
				if (prev.top < pane.top)
				{
					prev.bottom = pane.top;
				}
				else
				{
					prev.bottom = getBottom(prev);
				}
			}
		}

		if (!m_panes.empty())
		{
			RECT& pane = m_panes.back();

			pane.bottom = getBottom(pane);
		}
#if 0
		for (int i = 0; i < (int)m_panes.size(); i++)
		{
			MY_TRACE_RECT2(m_panes[i]);
		}
#endif
	}

	int getBottom(const RECT& pane)
	{
		LONG result = 0;

		HWND child = ::GetWindow(m_dialog, GW_CHILD);
		while (child)
		{
			if (::IsWindowVisible(child))
			{
				RECT rc; ::GetClientRect(child, &rc);
				::MapWindowPoints(child, m_dialog, (LPPOINT)&rc, 2);
				if (rc.left >= pane.left && rc.right <= pane.right)
				{
					result = yulib::Max(result, rc.bottom);
				}
			}

			child = ::GetWindow(child, GW_HWNDNEXT);
		}

		return result;
	}

public:

	DialogInfo(HWND dialog)
		: m_dialog(dialog)
	{
		getPaneRect();
	}

	HWND getDialog() const
	{
		return m_dialog;
	}

	FilterHolder getSrcFilter(POINT pos, ObjectHolder object) const
	{
		for (int i = 0; i < (int)m_panes.size(); i++)
		{
			if (::PtInRect(&m_panes[i], pos))
			{
				FilterHolder filter(object, i);

				if (filter.isMoveable())
					return filter;

				break;
			}
		}

		return FilterHolder(object, -1);
	}

	FilterHolder getDstFilter(POINT pos, ObjectHolder object) const
	{
		for (int i = 0; i < (int)m_panes.size(); i++)
		{
			if (::PtInRect(&m_panes[i], pos))
			{
				FilterHolder filter(object, i);

				if (filter.isMoveable())
				{
					return filter;
				}
				else
				{
					if (i == 0)
					{
						// フィルタが先頭要素の場合はその次のフィルタを返す。
						FilterHolder filter(object, 1);
					}
					else
					{
						// フィルタが先頭要素ではない場合は無効。
						return FilterHolder(object, -1);
					}
				}
			}
		}

		return FilterHolder(object, -1);
	}

/*
	// フィルタの高さを返す。
	int getFilterHeight(int filterIndex, int maxHeight)
	{
		int y1 = getFilterPosY(filterIndex);
		int y2 = getFilterPosY(filterIndex + 1);
		if (y2 > 0) return y2 - y1;
		int h = maxHeight - (y1 - FILTER_HEADER_HEIGHT / 2);
		if (h > 0) return h;
		return FILTER_HEADER_HEIGHT;
	}
*/
	// フィルタ矩形を返す。
	void getFilterRect(FilterHolder filter, LPRECT rc) const
	{
		int i = filter.getFilterIndex();
		if (i >= 0 && i < (int)m_panes.size())
			*rc = m_panes[i];
		else
			::GetClientRect(m_dialog, rc);
	}
};

#define MY_TRACE_OBJECT_HOLDER(object) \
do \
{ \
	MY_TRACE(_T(#object) _T(" = %d\n"), object.getObjectIndex()); \
} \
while (0)

#define MY_TRACE_FILTER_HOLDER(filter) \
do \
{ \
	MY_TRACE(_T(#filter) _T(" = %d\n"), filter.getFilterIndex()); \
} \
while (0)

//---------------------------------------------------------------------
