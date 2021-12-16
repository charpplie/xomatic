/*************************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 27:10:2004   11:29 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "BlackBoard.h"

CBlackBoard::CBlackBoard()
{
	m_BB.Create(gEnv->pSystem->GetIScriptSystem());
}

void CBlackBoard::SetFromScript( SmartScriptTable& sourceBB )
{
	m_BB->Clone( sourceBB, true );
}

#include UNIQUE_VIRTUAL_WRAPPER(IBlackBoard)