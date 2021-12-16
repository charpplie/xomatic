////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   animlightnode.h
//  Version:     v1.00
//  Created:     11/29/2010 by Jaesik.
//  Compilers:   Microsoft Visual C++ 2008
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __AnimLightNode_H_
#define __AnimLightNode_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include "EntityNode.h"

template <class ValueType> class TAnimSplineTrack;
typedef TAnimSplineTrack<Vec2> C2DSplineTrack;

class CAnimLightNode : public CAnimEntityNode
{
private:

public:

	virtual EAnimNodeType GetType() const { return ANODE_LIGHT; }

	CAnimLightNode();
	virtual ~CAnimLightNode();	
	virtual void Animate( SAnimContext &ec );

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
	
	virtual void CreateDefaultTracks();
	virtual void Activate( bool bActivate );

	void Serialize( XmlNodeRef &xmlNode,bool bLoading,bool bLoadEmptyTracks );

	int GetParamCount() const;
	bool GetParamInfo( int nIndex, SParamInfo &info ) const;
	bool GetParamInfoFromId( int paramId, SParamInfo &info ) const;

	bool SetParamValue( float time,AnimParamType param,float value );
	bool SetParamValue( float time,AnimParamType param,const Vec3 &value );	

	virtual IAnimTrack* CreateTrack( int paramId );

private:

	enum
	{
		NumNodeParams = 10
	};

	static IAnimNode::SParamInfo s_nodeParams[NumNodeParams];

private:

	bool GetValueFromTrack( AnimParamType type, float time, float& value ) const;
	bool GetValueFromTrack( AnimParamType type, float time, Vec3& value ) const;


private:

	float m_fRadius;
	float m_fDiffuseMultiplier;
	float m_fHDRDynamic;
	float m_fSpecularMultiplier;
	float m_fSpecularPercentage;
	Vec3 m_clrDiffuseColor;	
	bool m_bJustActivated;

};

#endif // __AnimLightNode_H_
