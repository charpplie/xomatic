////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2013.
// -------------------------------------------------------------------------
//  File name:   SegmentDataStatusPanel.h
//  Version:     v1.00
//  Created:     30/9/2013 by Allen Chen
//  Compilers:   Visual Studio.NET
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SEGMENT_DATA_STATUS_PANEL_H__
#define __SEGMENT_DATA_STATUS_PANEL_H__

#include "SLDataPanel.h"
#include "SWCommon.h"

class CSegmentTerrainDataRecord : public CSLBaseItemRecord
{
public:
	CSegmentTerrainDataRecord(int row, int col);

	void CreateItems();
	uint32 GetFileSCMAttributes();
	void GetFilename(std::vector<CString> &filenames) const;

protected:
	CString m_filename[sw::SDB_SOURCEDATA_END - sw::SDB_SOURCEDATA_BEGIN];
};

class CSegmentLayerDataRecord : public CSLBaseItemRecord
{
public:
	CSegmentLayerDataRecord(int row, int col);

	void InitFilename(std::vector<CString> &layerNames, int row, int col);
	void CreateItems();
	uint32 GetFileSCMAttributes();
	void GetFilename(std::vector<CString> &filenames) const;

protected:
	CString m_metadata;
	std::vector<CString> m_filename;
};

class CSegmentDataRecord : public CSLBaseItemRecord
{
public:
	CSegmentDataRecord(int row, int col);

	void CreateItems();
	uint32 GetFileSCMAttributes();
	void GetFilename(std::vector<CString> &filenames) const;

	void AddChild(CSLBaseItemRecord *pChild);
	void AddRecordToTree(CTreeCtrlReport *pTree);

protected:
	int m_row;
	int m_col;
	CString m_filename;
	std::vector<CSLBaseItemRecord *> m_child;
};

class CSegmentDataStatusPanel : public CSLDataPanel
{
public:
	CSegmentDataStatusPanel(CWnd *pParent = NULL);

	enum { IDD = IDD_PANEL_SEGMENT_STATUS };

	void UpdateEntries(Recti &rcWorld);
	void ShowContextMenu(CSLBaseItemRecord *pRecord, CPoint point);

protected:
	virtual BOOL OnInitDialog();
};

#endif // __SEGMENT_DATA_STATUS_PANEL_H__