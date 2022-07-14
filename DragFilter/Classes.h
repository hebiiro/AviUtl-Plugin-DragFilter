#pragma once

//---------------------------------------------------------------------

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

class CursorPos : public POINT
{
public:

	CursorPos(HWND hwnd);
};

class ObjectHolder
{
private:

	int m_objectIndex;
	ExEdit::Object* m_object;

public:

	ObjectHolder();
	ObjectHolder(int objectIndex);

	int getObjectIndex() const;
	ExEdit::Object* getObject() const;
	BOOL isValid() const;

	bool operator==(const ObjectHolder& x) const;
	bool operator!=(const ObjectHolder& x) const;
};

class FilterHolder
{
private:

	ObjectHolder m_object;
	int m_filterIndex;
	ExEdit::Filter* m_filter;

public:

	FilterHolder();
	FilterHolder(ObjectHolder object, int filterIndex);

	const ObjectHolder& getObject() const;
	int getFilterIndex() const;
	ExEdit::Filter* getFilter() const;
	BOOL isValid() const;

	bool operator==(const FilterHolder& x) const;
	bool operator!=(const FilterHolder& x) const;

	BOOL isMoveable() const;
	LPCSTR getName() const;
};

typedef std::vector<RECT> PaneContainer;

class DialogInfo
{
private:

	HWND m_dialog;
	PaneContainer m_panes;

	void getPaneRect();
	int getBottom(const RECT& pane);

public:

	DialogInfo(HWND dialog);

	HWND getDialog() const;
	FilterHolder getSrcFilter(POINT pos, ObjectHolder object) const;
	FilterHolder getDstFilter(POINT pos, ObjectHolder object) const;
	void getFilterRect(FilterHolder filter, LPRECT rc) const;
};

//---------------------------------------------------------------------
