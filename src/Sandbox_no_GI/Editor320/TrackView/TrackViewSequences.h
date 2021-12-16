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

#ifndef __TrackViewSequencesCtrl_h__
#define __TrackViewSequencesCtrl_h__

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

//////////////////////////////////////////////////////////////////////////
class CTrackViewSequencesCtrl : public CTreeCtrlReport
{
public:
	class CRecord : public CTreeItemRecord
	{
	public:
		CRecord(bool bIsGroup, const CString& name) : CTreeItemRecord(bIsGroup,name) {}
	public:
		CString sequenceName;
	};

public:
	CTrackViewSequencesCtrl();
	void SetTrackViewDialog( class CTrackViewDialog* pDlg ) { m_pTrackViewDialog = pDlg; }

	//////////////////////////////////////////////////////////////////////////
	// Callbacks.
	//////////////////////////////////////////////////////////////////////////
	virtual void OnFillItems();
	virtual void OnSelectionChanged();
	//////////////////////////////////////////////////////////////////////////

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnNMRclick(NMHDR *pNMHDR, LRESULT *pResult);


protected:
	class CTrackViewDialog *m_pTrackViewDialog;
};

#endif // __TrackViewSequences_h__