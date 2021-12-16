////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   sequencerundo.h
//  Version:     v1.00
//  Created:     30/8/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __sequencerundo_h__
#define __sequencerundo_h__

#if _MSC_VER > 1000
#pragma once
#endif

class CSequencerTrack;
class CSequencerNode;
class CSequencerSequence;

/** Undo object stored when track is modified.
*/
class CUndoSequencerSequenceModifyObject : public IUndoObject
{
public:
	CUndoSequencerSequenceModifyObject( CSequencerTrack *track, CSequencerSequence* pSequence );
protected:
	virtual int GetSize() { return sizeof(*this); }
	virtual const char* GetDescription() { return "Track Modify"; };

	virtual void Undo( bool bUndo );
	virtual void Redo();

private:
	TSmartPtr<CSequencerTrack> m_pTrack;
	TSmartPtr<CSequencerSequence> m_pSequence;
	XmlNodeRef m_undo;
	XmlNodeRef m_redo;
};

class CAnimSequenceUndo
	: public CUndo
{
public:
	CAnimSequenceUndo( CSequencerSequence *pSeq, const char *sName )
		: CUndo( sName )
	{
		//	CUndo::Record( new CUndoSequencerAnimSequenceObject(pSeq) );
	}
};

#endif // __sequencerundo_h__
