////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2013.
// -------------------------------------------------------------------------
//  File name:   GridMapTool.h
//  Version:     v1.00
//  Created:     15/11/2013 by Allen Chen
//  Compilers:   Visual Studio.NET
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __GRID_MAP_TOOL_H__
#define __GRID_MAP_TOOL_H__

// {5A64B647-CE76-6FD1-8322-BCF95C554036}
DEFINE_GUID( SEGMENT_SELECT_TOOL_GUID, 0x5a64b647, 0xce76, 0x6fd1, 0x83, 0x22, 0xbc, 0xf9, 0x5c, 0x55, 0x40, 0x36);

class CSegmentDataStatusPanel;

enum SegmentSelectorType
{
	eSelectorEdit,
	eSelectorCheck,
	eSelectorMerge,
	eSelectorCount,
};

struct CSegmentSelector
{
	// type of this selector
	SegmentSelectorType type;
	// selected segment number in row or column
	int selSize;
	// number segments to be merged in row and column
	Vec2i mergeSize;

	CSegmentSelector()
	{
		type = eSelectorEdit;
		selSize = 1;
		mergeSize = Vec2i(1, 1);
	}
};

class CSegmentSelectTool : public CEditTool
{
	DECLARE_DYNCREATE(CSegmentSelectTool)
public:
	CSegmentSelectTool();
	virtual ~CSegmentSelectTool();

	static void RegisterTool(CRegistrationContext &rc);

	void SetExnernalUIPanel(CSegmentDataStatusPanel *pPanel);
	void SetWorldName(const CString &name) { m_worldName = name; }

	void SetPerSegmentSize(int nSize) { m_segmentSize = nSize; }
	int GetPerSegmentSize() const { return m_segmentSize; }
	int GetEditSize() { return m_selector[eSelectorEdit].selSize; }
	void SetEditSize(int size) { m_selector[eSelectorEdit].selSize = size; }
	Recti GetWorldBoundary() const { return m_worldBoundary; }
	Recti GetSegmentBoundary() const { return m_worldBoundary / m_segmentSize; }

	SegmentSelectorType GetSelectorType() const { return m_currentSelectorType; }
	void SetSelectorType(SegmentSelectorType type);
	void RestoreSelectorType();

	bool OpenWorldInteranl(CString &outWorldName);
	void UpdateWorldBound();

	void MoveToSelected();
	void PrepareForMerge();
	void MergeToSelected();
	void RemoveSelected();

	// CEditTool
	virtual void Display(DisplayContext &dc);
	virtual bool MouseCallback(CViewport *view,EMouseEvent event,CPoint &point,int flags);
	virtual bool OnKeyUp(CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags);
	// ~CEditTool

protected:
	Vec2i GetSelSize();
	Recti GetSelRectMt();

	virtual void DeleteThis() { delete this; }

private:
	Vec3 m_pointerPos;

	CSegmentDataStatusPanel *m_panel;

	// per-segment size in meters
	int m_segmentSize;

	// selected area in meters
	Recti m_mouseHoveredArea;
	Recti m_selectedArea;
	Recti m_worldBoundary;

	typedef std::list<Recti> RectList;
	RectList m_removedAreas;

	// world name
	CString m_worldName;
	CString m_mergeWorldName;
	CString m_levelPathTpl;

	CSegmentSelector *m_pSelector;
	CSegmentSelector *m_prevSelector;
	static SegmentSelectorType m_currentSelectorType;
	static CSegmentSelector m_selector[eSelectorCount];
};

#endif // __GRID_MAP_TOOL_H__