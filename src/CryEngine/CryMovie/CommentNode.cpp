////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name: CommentNode.cpp
//  Version:   v1.00
//  Created:   08-04-2010 by Dongjoon Kim
//  Description:
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 

#include "StdAfx.h"
#include "CommentNode.h"
#include "AnimSplineTrack.h"
#include "CommentTrack.h"

//////////////////////////////////////////////////////////////////////////
namespace
{
	bool s_nodeParamsInit = false;
	std::vector<IAnimNode::SParamInfo> s_nodeParameters;

	void AddSupportedParameters( const char *sName,int paramId,EAnimValue valueType )
	{
		IAnimNode::SParamInfo param;
		param.name = sName;
		param.paramId = paramId;
		param.valueType = valueType;
		s_nodeParameters.push_back( param );
	}
};

//-----------------------------------------------------------------------------
CCommentNode::CCommentNode()
{
	CCommentNode::Initialize();
}

//-----------------------------------------------------------------------------
void CCommentNode::Initialize()
{
	if (!s_nodeParamsInit)
	{
		s_nodeParamsInit = true;
		s_nodeParameters.reserve(3);
		AddSupportedParameters( "Text",APARAM_COMMENT_TEXT,AVALUE_COMMENT_TEXT );
		AddSupportedParameters( "Unit Pos X",APARAM_POS_X,AVALUE_FLOAT );
		AddSupportedParameters( "Unit Pos Y",APARAM_POS_Y,AVALUE_FLOAT );
	}
}

//-----------------------------------------------------------------------------
void CCommentNode::Animate(SAnimContext &ac)
{
	// It's only for valid operation of key time editing.
	// Actual animation process is in the editor side.
	CCommentTrack* pCommentTrack = static_cast<CCommentTrack*>(GetTrackForParameter(APARAM_COMMENT_TEXT));
	if(pCommentTrack)
		pCommentTrack->ValidateKeyOrder();
}

//-----------------------------------------------------------------------------
void CCommentNode::CreateDefaultTracks()
{
	CreateTrack(APARAM_COMMENT_TEXT);

	C2DSplineTrack* pTrack = 0;

	pTrack = (C2DSplineTrack*)CreateTrack(APARAM_POS_X);
	pTrack->SetDefaultValue(Vec2(0,50));

	pTrack = (C2DSplineTrack*)CreateTrack(APARAM_POS_Y);
	pTrack->SetDefaultValue(Vec2(0,50));
}

//-----------------------------------------------------------------------------
void CCommentNode::Reset()
{

}

//-----------------------------------------------------------------------------
void CCommentNode::Activate( bool bActivate )
{
	CAnimNode::Activate(bActivate);
}

//-----------------------------------------------------------------------------
void CCommentNode::Serialize( XmlNodeRef &xmlNode,bool bLoading,bool bLoadEmptyTracks )
{
	CAnimNode::Serialize(xmlNode, bLoading, bLoadEmptyTracks);
}

//-----------------------------------------------------------------------------
int CCommentNode::GetParamCount() const
{
	return s_nodeParameters.size();
}

//-----------------------------------------------------------------------------
bool CCommentNode::GetParamInfo( int nIndex, SParamInfo &info ) const
{
	if (nIndex >= 0 && nIndex < (int)s_nodeParameters.size())
	{
		info = s_nodeParameters[nIndex];
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool CCommentNode::GetParamInfoFromId( int paramId, SParamInfo &info ) const
{
	for (size_t i = 0; i < s_nodeParameters.size(); ++i)
	{
		if (s_nodeParameters[i].paramId == paramId)
		{
			info = s_nodeParameters[i];
			return true;
		}
	}
	return false;
}
