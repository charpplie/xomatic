////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   animcameranode.cpp
//  Version:     v1.00
//  Created:     11/29/2010 by Jaesik.
//  Compilers:   Microsoft Visual C++ 2008
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "AnimLightNode.h"
#include <IScriptSystem.h>

IAnimNode::SParamInfo CAnimLightNode::s_nodeParams[CAnimLightNode::NumNodeParams] = {
		IAnimNode::SParamInfo( "Position",APARAM_POS,AVALUE_VECTOR, 0 ),
		IAnimNode::SParamInfo( "Rotation",APARAM_ROT,AVALUE_QUAT, 0 ),
		IAnimNode::SParamInfo( "Radius", APARAM_LIGHT_RADIUS, AVALUE_FLOAT, 0 ),
		IAnimNode::SParamInfo( "DiffuseColor", APARAM_LIGHT_DIFFUSECOLOR, AVALUE_VECTOR, 0 ),
		IAnimNode::SParamInfo( "DiffuseMultiplier", APARAM_LIGHT_DIFFUSEMULTIPLIER, AVALUE_FLOAT, 0 ),
		IAnimNode::SParamInfo( "HDRDynamic", APARAM_LIGHT_HDRDYNAMIC, AVALUE_FLOAT, 0 ),
		IAnimNode::SParamInfo( "SpecularMultiplier", APARAM_LIGHT_SPECULARMULTIPLIER, AVALUE_FLOAT, 0 ),
		IAnimNode::SParamInfo( "SpecularPercentage", APARAM_LIGHT_SPECULARPERCENTAGE, AVALUE_FLOAT, 0 ),
		IAnimNode::SParamInfo( "Visibility",APARAM_VISIBLE,AVALUE_BOOL, 0 ),
		IAnimNode::SParamInfo( "Event",APARAM_EVENT,AVALUE_EVENT, 0 )
};

CAnimLightNode::CAnimLightNode()
: m_fRadius(10.0f),
	m_fDiffuseMultiplier(1),
	m_fHDRDynamic(0),
	m_fSpecularMultiplier(1),
	m_clrDiffuseColor(255.0f,255.0f,255.0f),
	m_fSpecularPercentage(100.0f),
	m_bJustActivated(false)
{
}


CAnimLightNode::~CAnimLightNode()
{
}


void CAnimLightNode::Animate( SAnimContext &ec )
{
	CAnimEntityNode::Animate(ec);

	bool bNodeAnimated(false);

	float radius(m_fRadius);
	Vec3 diffuseColor(m_clrDiffuseColor);
	float diffuseMultiplier(m_fDiffuseMultiplier);
	float hdrDynamic(m_fHDRDynamic);
	float specularMultiplier(m_fSpecularMultiplier);
	float specularPercentage(m_fSpecularPercentage);

	GetValueFromTrack( APARAM_LIGHT_RADIUS, ec.time, radius );	
	GetValueFromTrack( APARAM_LIGHT_DIFFUSECOLOR,  ec.time, diffuseColor );	
	GetValueFromTrack( APARAM_LIGHT_DIFFUSEMULTIPLIER, ec.time, diffuseMultiplier );	
	GetValueFromTrack( APARAM_LIGHT_HDRDYNAMIC, ec.time, hdrDynamic );
	GetValueFromTrack( APARAM_LIGHT_SPECULARMULTIPLIER, ec.time, specularMultiplier );
	GetValueFromTrack( APARAM_LIGHT_SPECULARPERCENTAGE, ec.time, specularPercentage );

	if( m_bJustActivated == true )
	{
		bNodeAnimated = true;
		m_bJustActivated = false;
	}

	if( m_fRadius!= radius )
	{
		bNodeAnimated = true;
		m_fRadius = radius;
	}

	if( m_clrDiffuseColor != diffuseColor )
	{
		bNodeAnimated = true;
		m_clrDiffuseColor = diffuseColor;
	}

	if( m_fDiffuseMultiplier != diffuseMultiplier )
	{
		bNodeAnimated = true;
		m_fDiffuseMultiplier = diffuseMultiplier;
	}

	if( m_fHDRDynamic != hdrDynamic )
	{
		bNodeAnimated = true;
		m_fHDRDynamic = hdrDynamic;
	}

	if( m_fSpecularMultiplier != specularMultiplier )
	{
		bNodeAnimated = true;
		m_fSpecularMultiplier = specularMultiplier;
	}

	if( m_fSpecularPercentage != specularPercentage )
	{
		bNodeAnimated = true;
		m_fSpecularPercentage = specularPercentage;
	}

	if(bNodeAnimated && m_pOwner)
	{
		m_bIgnoreSetParam = true;
		if( m_pOwner )
			m_pOwner->OnNodeAnimated(this);
		IEntity* pEntity = GetEntity();
		if( pEntity )
		{
			IScriptTable *scriptObject = pEntity->GetScriptTable();
			if(scriptObject)
			{
				scriptObject->SetValue("Radius", m_fRadius);
				scriptObject->SetValue("clrDiffuse", m_clrDiffuseColor);
				scriptObject->SetValue("fDiffuseMultiplier", m_fDiffuseMultiplier );
				scriptObject->SetValue("fSpecularMultiplier", m_fSpecularMultiplier );
				scriptObject->SetValue("fHDRDynamic", m_fHDRDynamic );
				scriptObject->SetValue("fSpecularPercentage", m_fSpecularPercentage );
			}
		}
		m_bIgnoreSetParam = false;
	}
}


bool CAnimLightNode::GetValueFromTrack( AnimParamType type, float time, float& value ) const
{
	IAnimTrack* pTrack = GetTrackForParameter(type);

	if( pTrack == NULL )
		return false;	

	pTrack->GetValue(time, value);

	return true;
}


bool CAnimLightNode::GetValueFromTrack( AnimParamType type, float time, Vec3& value ) const
{
	IAnimTrack* pTrack = GetTrackForParameter(type);

	if( pTrack == NULL )
		return false;	

	pTrack->GetValue(time, value);

	return true;
}


void CAnimLightNode::CreateDefaultTracks()
{
	CreateTrack(APARAM_POS);
	CreateTrack(APARAM_ROT);
	CreateTrack(APARAM_LIGHT_RADIUS);
	CreateTrack(APARAM_LIGHT_DIFFUSECOLOR);	
	CreateTrack(APARAM_EVENT);	
}


IAnimTrack* CAnimLightNode::CreateTrack( int paramId )
{
	IAnimTrack* pTrack = CAnimNode::CreateTrack(paramId);

	switch( paramId )
	{
	case APARAM_LIGHT_RADIUS:
		pTrack->SetValue( 0, m_fRadius, true );
		break;

	case APARAM_LIGHT_DIFFUSECOLOR:
		pTrack->SetValue( 0, m_clrDiffuseColor, true );
		break;

	case APARAM_LIGHT_DIFFUSEMULTIPLIER:
		pTrack->SetValue( 0, m_fDiffuseMultiplier, true );
		break;

	case APARAM_LIGHT_HDRDYNAMIC:
		pTrack->SetValue( 0, m_fHDRDynamic, true );
		break;

	case APARAM_LIGHT_SPECULARMULTIPLIER:
		pTrack->SetValue( 0, m_fSpecularMultiplier, true );
		break;

	case APARAM_LIGHT_SPECULARPERCENTAGE:
		pTrack->SetValue( 0, m_fSpecularPercentage, true );
		break;
	}	

	return pTrack;
}


void CAnimLightNode::Activate( bool bActivate )
{
	CAnimEntityNode::Activate(bActivate);
	m_bJustActivated = bActivate;
}


void CAnimLightNode::Serialize( XmlNodeRef &xmlNode,bool bLoading,bool bLoadEmptyTracks )
{
	CAnimEntityNode::Serialize( xmlNode,bLoading,bLoadEmptyTracks );
	if (bLoading)
	{
		xmlNode->getAttr( "Radius",m_fRadius );
		xmlNode->getAttr( "DiffuseMultiplier",m_fDiffuseMultiplier );
		xmlNode->getAttr( "DiffuseColor",m_clrDiffuseColor );
		xmlNode->getAttr( "HDRDynamic", m_fHDRDynamic );
		xmlNode->getAttr( "SpecularMultiplier", m_fSpecularMultiplier );
		xmlNode->getAttr( "SpecularPercentage", m_fSpecularPercentage );
	}
	else
	{
		xmlNode->setAttr( "Radius",m_fRadius );
		xmlNode->setAttr( "DiffuseMultiplier",m_fDiffuseMultiplier );
		xmlNode->setAttr( "DiffuseColor",m_clrDiffuseColor );
		xmlNode->setAttr( "HDRDynamic", m_fHDRDynamic );
		xmlNode->setAttr( "SpecularMultiplier", m_fSpecularMultiplier );
		xmlNode->setAttr( "SpecularPercentage", m_fSpecularPercentage );
	}
}

bool CAnimLightNode::SetParamValue( float time,AnimParamType param,float value )
{
	switch( param )
	{
	case APARAM_LIGHT_RADIUS:
		m_fRadius = value;
		break;

	case APARAM_LIGHT_DIFFUSEMULTIPLIER:
		m_fDiffuseMultiplier = value;
		break;

	case APARAM_LIGHT_HDRDYNAMIC:
		m_fHDRDynamic = value;
		break;

	case APARAM_LIGHT_SPECULARMULTIPLIER:
		m_fSpecularMultiplier = value;
		break;

	case APARAM_LIGHT_SPECULARPERCENTAGE:
		m_fSpecularPercentage = value;
		break;
	}

	return CAnimEntityNode::SetParamValue( time,param,value );
}


bool CAnimLightNode::SetParamValue( float time,AnimParamType param,const Vec3 &value )
{
	if( param == APARAM_LIGHT_DIFFUSECOLOR )
		m_clrDiffuseColor = value;

	return CAnimEntityNode::SetParamValue( time,param,value );
}


int CAnimLightNode::GetParamCount() const
{
	return NumNodeParams;
}


bool CAnimLightNode::GetParamInfo( int nIndex, SParamInfo &info ) const
{
	if (nIndex >= 0 && nIndex < (int)NumNodeParams)
	{
		info = s_nodeParams[nIndex];
		return true;
	}
	return false;
}


bool CAnimLightNode::GetParamInfoFromId( int paramId, SParamInfo &info ) const
{
	for (unsigned int i = 0; i < NumNodeParams; i++)
	{
		if (s_nodeParams[i].paramId == paramId)
		{
			info = s_nodeParams[i];
			return true;
		}
	}
	return false;
}