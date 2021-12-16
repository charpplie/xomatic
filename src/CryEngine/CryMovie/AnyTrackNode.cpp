////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   AnyTrackNode.cpp
//  Version:     v1.00
//  Created:     11/5/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "AnyTrackNode.h"
#include "AnimTrack.h"

#include <ISystem.h>
#include <IScriptSystem.h>

//////////////////////////////////////////////////////////////////////////
CAnimAnyTrackNode::CAnimAnyTrackNode()
{
	SetFlags( GetFlags()|ANODE_FLAG_CAN_CHANGE_NAME );
}

//////////////////////////////////////////////////////////////////////////
int CAnimAnyTrackNode::GetParamCount() const
{
	return 1;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimAnyTrackNode::GetParamInfo( int nIndex, SParamInfo &info ) const
{
	if (nIndex == 0)
	{
		info.flags = 0;
		info.name = "Value";
		info.paramId = APARAM_USER;
		info.valueType = AVALUE_FLOAT;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimAnyTrackNode::GetParamInfoFromId( int paramId, SParamInfo &info ) const
{
	if (paramId == APARAM_USER)
	{
		GetParamInfo( 0,info );
		return true;
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
void CAnimAnyTrackNode::Animate( SAnimContext &ec )
{	
	IAnimTrack *pTrack = GetTrackByIndex(0);
	if (pTrack)
	{
		pTrack->Animate( ec );
	}
}
