#ifndef __DescEditorTree_h__
#define __DescEditorTree_h__

#if _MSC_VER > 1000
# pragma once
#endif

#include "Dialogs\BaseFrameWnd.h"
#include "DescEditor.h"

namespace CryGame
{
	class CDescEditor;
	class CDescPropertyInfo;

	//////////////////////////////////////////////////////////////////////////
	// This is the tree class to display all desc files and property objects for the user to select to display properties in the property window
	class CDescEditorTree : public CXTPReportControl
	{
	public:
		CDescEditorTree(CDescEditor* pDescEditor) : m_pDescEditor(pDescEditor), m_pSelectedRecord(NULL), m_pSelectedItem(NULL) {};
		virtual ~CDescEditorTree() { CleanupAllUserdata(); };

		virtual void OnSelectionChanged();
		virtual DROPEFFECT OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point, int nState);
		virtual BOOL OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);

		CXTPReportRecord* GetSelectedDescRecord() { return m_pSelectedRecord; }
		void SetSelectedDescRecord(CXTPReportRecord* pValue);
		CXTPReportRecordItem* GetSelectedDescRecordItem() { return m_pSelectedItem; }
		void SetSelectedDescRecordItem(CXTPReportRecordItem* pValue) { m_pSelectedItem = pValue; }
		CXTPReportRecord* GetRootRecord();
		CDescPropertyInfo* GetItemData(CXTPReportRow* pRow);
		CDescPropertyInfo* GetItemData(CXTPReportRecord* pRecord);
		void CloneToClipboard();

		void CleanupAllUserdata();
		void CleanupChildrenUserdata(CXTPReportRecord* pRecord);
		void UpdateTree();

	protected:
		void GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics);

		CDescEditor* m_pDescEditor;
		CXTPReportRecord* m_pSelectedRecord;
		CXTPReportRecordItem* m_pSelectedItem;

	public:
		DECLARE_MESSAGE_MAP()
		afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
		afx_msg void OnSetFocus(CWnd* pOldWnd);
	};
}  // namespace CryGame

#endif
