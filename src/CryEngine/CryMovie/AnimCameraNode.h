////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   animcameranode.h
//  Version:     v1.00
//  Created:     16/8/2002 by Lennert.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __animcameranode_h__
#define __animcameranode_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include "EntityNode.h"

/** Camera node controls camera entity.
*/
class CAnimCameraNode : public CAnimEntityNode
{
private:
	// Field of view in DEGREES! To Display it nicely for user.
	float m_fFOV;
  int m_lastFocusKey;
  EntityId m_focusEntityId;

	static const int SHAKE_COUNT = 2;
	struct ShakeParam
	{
		Vec3 amplitude;
		float amplitudeMult;
		Vec3 frequency;
		float frequencyMult;
		float noiseAmpMult;
		float noiseFreqMult;
		float timeOffset;

		bool resetPhase;
		Vec3 phase;
		Vec3 phaseNoise;

		ShakeParam()
		{
			amplitude = Vec3(1.0f,1.0f,1.0f);
			amplitudeMult = 0.0f;
			frequency = Vec3(1.0f,1.0f,1.0f);
			frequencyMult = 0.0f;
			noiseAmpMult = 0.0f;
			noiseFreqMult = 0.0f;
			timeOffset = 0.0f;

			resetPhase = true;
		}

		Ang3 ApplyCameraShake( float time, float prevTime, Ang3 angles );
	};
	ShakeParam m_shakeParam[SHAKE_COUNT];

	float m_prevTime;
	Vec3 m_fDOF; // depth of field
	ICVar *m_cv_r_PostProcessEffects;
public:
	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
	CAnimCameraNode();
	virtual ~CAnimCameraNode();
	virtual EAnimNodeType GetType() const { return ANODE_CAMERA; }
	virtual void Animate( SAnimContext &ec );

	virtual void CreateDefaultTracks();
	virtual void Reset();
	float GetFOV() { return m_fFOV; }
  void ApplyFocusKey( ISelectKey &key,SAnimContext &ec );

	//////////////////////////////////////////////////////////////////////////
	int GetParamCount() const;
	bool GetParamInfo( int nIndex, SParamInfo &info ) const;
	bool GetParamInfoFromId( int paramId, SParamInfo &info ) const;

	//////////////////////////////////////////////////////////////////////////
	bool SetParamValue( float time,AnimParamType param,float value );
	bool SetParamValue( float time,AnimParamType param,const Vec3 &value );
	bool SetParamValue( float time,AnimParamType param,const Vec4 &value );
	bool GetParamValue( float time,AnimParamType param,float &value );
	bool GetParamValue( float time,AnimParamType param,Vec3 &value );
	bool GetParamValue( float time,AnimParamType param,Vec4 &value );

	IAnimTrack* CreateTrack(int nParamType);
	void Serialize( XmlNodeRef &xmlNode,bool bLoading,bool bLoadEmptyTracks );

	virtual XmlNodeRef SaveToColladaInFixedFPS(float fps) const;
	virtual bool LoadFromCollada(XmlNodeRef xmlNode);
};

#endif // __animcameranode_h__