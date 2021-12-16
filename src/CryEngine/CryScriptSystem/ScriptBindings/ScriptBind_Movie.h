////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   ScriptBind_Movie.h
//  Version:     v1.00
//  Created:     8/7/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __ScriptBind_Movie_h__
#define __ScriptBind_Movie_h__
#pragma once


#include "IScriptSystem.h"

// <title Movie>
// Syntax: Movie
// Description:
//    Interface to movie system.
class CScriptBind_Movie : public CScriptableBase
{
public:
	CScriptBind_Movie(IScriptSystem *pScriptSystem, ISystem *pSystem);
	virtual ~CScriptBind_Movie();
	virtual void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}

	// <title PlaySequence>
	// Syntax: Movie.PlaySequence(const char *sSequenceName)
	// Arguments:
	//		sSequenceName - Sequence name.
	// Description:
	//		Plays the specified sequence.
	int PlaySequence( IFunctionHandler *pH,const char *sSequenceName );
	// <title StopSequence>
	// Syntax: Movie.StopSequence(const char *sSequenceName)
	// Arguments:
	//		sSequenceName - Sequence name.
	// Description:
	//		Stops the specified sequence.
	int StopSequence( IFunctionHandler *pH,const char *sSequenceName );
	// <title AbortSequence>
	// Syntax: Movie.AbortSequence(const char *sSequenceName)
	// Arguments:
	//		sSequenceName - Sequence name.
	// Description:
	//		Aborts the specified sequence.
	int AbortSequence( IFunctionHandler *pH,const char *sSequenceName );
	// <title StopAllSequences>
	// Syntax: Movie.StopAllSequences()
	// Description:
	//		Stops all the video sequences.
	int StopAllSequences(IFunctionHandler *pH);
	// <title StopAllCutScenes>
	// Syntax: Movie.StopAllCutScenes()
	// Description:
	//		Stops all the cut scenes.
	int StopAllCutScenes(IFunctionHandler *pH);
	// <title PauseSequences>
	// Syntax: Movie.PauseSequences()
	// Description:
	//		Pauses all the sequences.
	int PauseSequences(IFunctionHandler *pH);
	// <title ResumeSequences>
	// Syntax: Movie.ResumeSequences()
	// Description:
	//		Resume all the sequences.
	int ResumeSequences(IFunctionHandler *pH);
private:
	ISystem *m_pSystem;
	IMovieSystem *m_pMovieSystem;
};



#endif // __ScriptBind_Movie_h__