////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   sequencerundo.cpp
//  Version:     v1.00
//  Created:     30/8/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "SequencerUndo.h"
#include "Objects\SequenceObject.h"
#include "ISequencerSystem.h"
#include "SequencerSequence.h"
#include "AnimationContext.h"

//////////////////////////////////////////////////////////////////////////
CUndoSequencerSequenceModifyObject::CUndoSequencerSequenceModifyObject( CSequencerTrack *track, CSequencerSequence* pSequence )
{
	// Stores the current state of this track.
	assert( track != 0 );

	m_pTrack = track;
	m_pSequence = pSequence;

	// Store undo info.
	m_undo = XmlHelpers::CreateXmlNode("Undo");
	m_pTrack->Serialize( m_undo,false );
}

//////////////////////////////////////////////////////////////////////////
void CUndoSequencerSequenceModifyObject::Undo( bool bUndo )
{
	if (!m_undo)
		return;

	if (bUndo)
	{
		m_redo = XmlHelpers::CreateXmlNode("Redo");
		m_pTrack->Serialize( m_redo,false );
	}
	// Undo track state.
	m_pTrack->Serialize( m_undo,true );

	if (bUndo)
	{
		// Refresh stuff after undo.
		GetIEditor()->UpdateSequencer(true);
	}
}

//////////////////////////////////////////////////////////////////////////
void CUndoSequencerSequenceModifyObject::Redo()
{
	if (!m_redo)
		return;

	// Redo track state.
	m_pTrack->Serialize( m_redo,true );

	// Refresh stuff after undo.
	GetIEditor()->UpdateSequencer(true);
}
