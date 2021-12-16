////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   trackviewnodes.h
//  Version:     v1.00
//  Created:     29/4/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: TrackView's tree control.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __trackviewnodes_h__
#define __trackviewnodes_h__

//#include "Controls\MltiTree.h"
#include "Controls\TreeCtrlReport.h"
#include <IMovieSystem.h>

#if _MSC_VER > 1000
#pragma once
#endif


// forward declarations.
struct IAnimNode;
struct IAnimTrack;
struct IAnimSequence;
class CTrackViewKeys;

//////////////////////////////////////////////////////////////////////////
class CTrackViewNodesCtrl : public CTreeCtrlReport
{
public:
	typedef std::vector<IAnimNode*> AnimNodes;

	class CRecord : public CTreeItemRecord
	{
	public:
		CRecord(bool bIsGroup=true, const CString& name="") : m_name(name), m_bIsGroup(bIsGroup) {}
		bool IsGroup() const { return m_bIsGroup; }
		const CString& GetName() const { return m_name; }

	public:
		CString m_name;
		bool m_bIsGroup;
		_smart_ptr<CBaseObject> baseObj;
		int paramId;
		_smart_ptr<IAnimNode> node;
		_smart_ptr<IAnimTrack> track;
	};
	typedef CRecord SItemInfo;

	void SetSequence( IAnimSequence *seq );
	void SetKeyListCtrl( CTrackViewKeys *keysCtrl );
	void SyncKeyCtrl();
	void ExpandNode( IAnimNode *node );
	void DeleteSelectedNodes();
	void SelectNode( const char *sName );
	bool CopySelectedNodes();
	bool PasteNodes();
	CRecord* GetSelectedNode();

	bool GetSelectedNodes( AnimNodes &nodes );

public:
	CTrackViewNodesCtrl();

	//////////////////////////////////////////////////////////////////////////
	// Callbacks.
	//////////////////////////////////////////////////////////////////////////
	virtual void Reload();
	virtual void OnFillItems();
	virtual void OnItemExpanded( CXTPReportRow *pRow,bool bExpanded );
	virtual void OnSelectionChanged();
	virtual void OnVerticalScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	virtual bool OnBeginDragAndDrop( CXTPReportRow *pRow, CPoint point );
	virtual void OnDragAndDrop( CXTPReportRow *pRow, CPoint absoluteCursorPos );
	virtual void OnItemDblClick( CXTPReportRow *pRow );
	//////////////////////////////////////////////////////////////////////////

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnNMRclick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSize(UINT nType, int cx, int cy);

	void ShowHideTrack( IAnimNode * node, int trackIndex );
	void RemoveTrack( SItemInfo * pItemInfo );
	void AddTrack( int paramId, IAnimNode * node );
	void CreateFolder( IAnimNode * pGroupNode );
	void RenameNode( SItemInfo * pItemInfo );
	void EditEvents();
	void SetAsViewCamera();
	void SaveNodeToCollada();
	void LoadNodeFromCollada();

	int ShowPopupMenu(CPoint point, const SItemInfo *pItemInfo );

	void RecordSequenceUndo();
	void InvalidateNodes();
	
	int GetIconIndexForParam( int nParamId );

	void FillNodes( CRecord *pRecord, IAnimNode *pParent = NULL );
	void FillTracks( CRecord *pNodeRecord, IAnimNode *pNode );

	bool HasDirectorNode() const;

	void ExpandNodes(EAnimNodeType nodeType = ANODE_ANYTRACK);
	void CollapseNodes(EAnimNodeType nodeType = ANODE_ANYTRACK);

	struct CAnimSequenceUndo : public CUndo
	{
		CAnimSequenceUndo( IAnimSequence *pSeq,const char *sName );
	};

protected:
	IAnimSequence *m_sequence;
	CTrackViewKeys* m_keysCtrl;

	// Must not be vector, vector may invalidate pointers on add/remove.
	typedef std::vector<SItemInfo*> ItemInfos;
	ItemInfos m_itemInfos;

	class CTrackViewDialog *m_pTrackViewDialog;
};

#endif // __trackviewnodes_h__
