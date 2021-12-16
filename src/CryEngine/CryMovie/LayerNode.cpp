////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name: LayerNode.cpp
//  Version:   v1.00
//  Created:   19-03-2010 by Dongjoon Kim
//  Description: 
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 

#include "StdAfx.h"
#include "LayerNode.h"

//////////////////////////////////////////////////////////////////////////
namespace
{
	bool s_nodeParamsInitialized = false;
	std::vector<IAnimNode::SParamInfo> s_nodeParams;

	void AddSupportedParam( const char *sName,int paramId,EAnimValue valueType )
	{
		IAnimNode::SParamInfo param;
		param.name = sName;
		param.paramId = paramId;
		param.valueType = valueType;
		s_nodeParams.push_back( param );
	}
};

//-----------------------------------------------------------------------------
CLayerNode::CLayerNode()
:m_bInit(false)
,m_bPreVisibility(true)
{
	if (!s_nodeParamsInitialized)
	{
		s_nodeParamsInitialized = true;
		AddSupportedParam( "Visibility",APARAM_VISIBLE,AVALUE_BOOL );
	}
}

//-----------------------------------------------------------------------------
void CLayerNode::Animate(SAnimContext &ec)
{
	bool bVisibilityModified = false;

	int trackCount = NumTracks();
	for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
	{
		int trackType = m_tracks[paramIndex].paramId;
		IAnimTrack *pTrack = m_tracks[paramIndex].track;
		if(pTrack->GetNumKeys() == 0)
			continue;

		if(ec.trackMask && ((ec.trackMask & (1 << pTrack->GetType())) == 0))
			continue;

		switch (trackType)
		{
		case APARAM_VISIBLE:
			if (!ec.bResetting)
			{
				IAnimTrack *visTrack = pTrack;
				bool visible = true;
				visTrack->GetValue( ec.time,visible );

				if(m_bInit)
				{
					if(visible!=m_bPreVisibility)
					{
						m_bPreVisibility = visible;
						bVisibilityModified = true;
					}
				}
				else
				{
					m_bInit = true;
					m_bPreVisibility = visible;
					bVisibilityModified = true;
				}

			}
			break;
		}

		// Layer entities visibility control
		if(bVisibilityModified)
		{
			if(m_pAnimator)
			{
				// This is only for Editor.
				m_pAnimator->Animate(this,ec);
			}
			else
			{
				// This is for game mode, in case of the layer data being exported.
				if(gEnv->pEntitySystem)
				{
					gEnv->pEntitySystem->EnableLayer(GetName(), m_bPreVisibility);
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
void CLayerNode::CreateDefaultTracks()
{
	CreateTrack(APARAM_VISIBLE);
}

//-----------------------------------------------------------------------------
void CLayerNode::Reset()
{
	m_bInit = false;
}

//-----------------------------------------------------------------------------
void CLayerNode::Activate( bool bActivate )
{

}

//-----------------------------------------------------------------------------
void CLayerNode::Serialize( XmlNodeRef &xmlNode,bool bLoading,bool bLoadEmptyTracks )
{
	CAnimNode::Serialize(xmlNode, bLoading, bLoadEmptyTracks);

	//Nothing to be serialized at this moment.
}

//-----------------------------------------------------------------------------
int CLayerNode::GetParamCount() const
{
	return s_nodeParams.size();
}

//-----------------------------------------------------------------------------
bool CLayerNode::GetParamInfo( int nIndex, SParamInfo &info ) const
{
	if (nIndex >= 0 && nIndex < (int)s_nodeParams.size())
	{
		info = s_nodeParams[nIndex];
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool CLayerNode::GetParamInfoFromId( int paramId, SParamInfo &info ) const
{
	for (unsigned int i = 0; i < s_nodeParams.size(); i++)
	{
		if (s_nodeParams[i].paramId == paramId)
		{
			info = s_nodeParams[i];
			return true;
		}
	}
	return false;
}