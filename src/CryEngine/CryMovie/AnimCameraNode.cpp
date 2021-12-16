////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   animcameranode.cpp
//  Version:     v1.00
//  Created:     16/8/2002 by Lennert.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "AnimCameraNode.h"
#include "SelectTrack.h"
#include "CompoundSplineTrack.h"
#include "PNoise3.h"

#include <ISystem.h>
#include <I3DEngine.h>
#include <IConsole.h>
#include <IEntitySystem.h>
#include <ITimer.h>

enum {
  APARAM_FOCUS_ENTITY = APARAM_USER,
};

#define s_nodeParamsInitialized s_nodeParamsInitializedCam
#define s_nodeParams s_nodeParamsCam
#define AddSupportedParam AddSupportedParamCam

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

CAnimCameraNode::CAnimCameraNode()
{
	m_fDOF = Vec3(ZERO);
	m_fFOV = 60.0f;
  m_lastFocusKey = -1;
	m_cv_r_PostProcessEffects = NULL;
	m_focusEntityId = 0;

	if (!s_nodeParamsInitialized)
	{
		s_nodeParamsInitialized = true;
		AddSupportedParam( "Position",APARAM_POS,AVALUE_VECTOR );
		AddSupportedParam( "Rotation",APARAM_ROT,AVALUE_QUAT );
		AddSupportedParam( "Fov",APARAM_FOV,AVALUE_FLOAT );
		AddSupportedParam( "Event",APARAM_EVENT,AVALUE_EVENT );
    AddSupportedParam( "FocusEntity",APARAM_FOCUS_ENTITY,AVALUE_SELECT );
		AddSupportedParam( "Shake",APARAM_SHAKEMULT,AVALUE_VECTOR4 );
		AddSupportedParam( "Depth of Field", APARAM_DEPTH_OF_FIELD, AVALUE_VECTOR);
	}
}

//////////////////////////////////////////////////////////////////////////
CAnimCameraNode::~CAnimCameraNode()
{
}

//////////////////////////////////////////////////////////////////////////
int CAnimCameraNode::GetParamCount() const
{
	return s_nodeParams.size();
}

//////////////////////////////////////////////////////////////////////////
bool CAnimCameraNode::GetParamInfo( int nIndex, SParamInfo &info ) const
{
	if (nIndex >= 0 && nIndex < (int)s_nodeParams.size())
	{
		info = s_nodeParams[nIndex];
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimCameraNode::GetParamInfoFromId( int paramId, SParamInfo &info ) const
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

//////////////////////////////////////////////////////////////////////////
void CAnimCameraNode::CreateDefaultTracks()
{
	CreateTrack(APARAM_POS);
	CreateTrack(APARAM_ROT);
	CreateTrack(APARAM_FOV);
};

//////////////////////////////////////////////////////////////////////////
void CAnimCameraNode::Animate( SAnimContext &ec )
{
	CAnimEntityNode::Animate(ec);

	//------------------------------------------------------------------------------
	///Depth of field track

	IAnimTrack *pDOFTrack = GetTrackForParameter(APARAM_DEPTH_OF_FIELD);
	
	Vec3 dof = Vec3(ZERO);
	if(pDOFTrack)
	{
		pDOFTrack->GetValue(ec.time, dof);
	}

	IAnimTrack *pFOVTrack = GetTrackForParameter(APARAM_FOV);
	float fov = m_fFOV;
	
	if (pFOVTrack)
		pFOVTrack->GetValue(ec.time, fov);

	// is this camera active ? if so, set the current fov
	if (gEnv->pMovieSystem->GetCameraParams().cameraEntityId == m_EntityId)
	{
		IEntity *pEntity = gEnv->pEntitySystem->GetEntity(m_EntityId);
		if (pEntity)
		{
			pEntity->SetFlags(pEntity->GetFlags() | ENTITY_FLAG_TRIGGER_AREAS);
			//pEntity->AddFlags(ENTITY_FLAG_TRIGGER_AREAS);
		}

		SCameraParams CamParams = gEnv->pMovieSystem->GetCameraParams();
		CamParams.fFOV = DEG2RAD(fov);
		gEnv->pMovieSystem->SetCameraParams(CamParams);

		// If there is a "Depth of Field Track" with current camera
		if(pDOFTrack)
		{
			// Check whether the post fx is enabled or not.
			if(m_cv_r_PostProcessEffects == NULL)				m_cv_r_PostProcessEffects = gEnv->pConsole->GetCVar("r_PostProcessEffects");			// If not enabled, we might spit some warning if currently in the editor.			// if(m_cv_r_PostProcessEffects && m_cv_r_PostProcessEffects->GetIVal() == 0)			// 	;

			// Active Depth of field
			gEnv->p3DEngine->SetPostEffectParam("Dof_Active", 1);
			// Set parameters
			gEnv->p3DEngine->SetPostEffectParam("Dof_FocusDistance", dof.x);
			gEnv->p3DEngine->SetPostEffectParam("Dof_FocusRange", dof.y);
			gEnv->p3DEngine->SetPostEffectParam("Dof_BlurAmount", dof.z);
		}	
		else
		{
			// If a camera doesn't have a DoF track, it means it won't use the DoF processing.
			gEnv->p3DEngine->SetPostEffectParam("Dof_Active", 0);
		}

	}
	else
	{
		IEntity *pEntity = gEnv->pEntitySystem->GetEntity(m_EntityId);
		if (pEntity)
		{
			pEntity->ClearFlags(ENTITY_FLAG_TRIGGER_AREAS);
		}

	}

  CSelectTrack *pFocusTrack = (CSelectTrack*)GetTrackForParameter(APARAM_FOCUS_ENTITY);
  if (pFocusTrack)
  {
    ISelectKey key;
    int nkey = pFocusTrack->GetActiveKey(ec.time,&key);
    if (nkey != m_lastFocusKey)
    {
      if (!ec.bSingleFrame || key.time == ec.time) // If Single frame update key time must match current time.
        ApplyFocusKey( key,ec );
    }
    m_lastFocusKey = nkey;
  }

  if (m_focusEntityId)
  {
    IEntity *pEntity = gEnv->pEntitySystem->GetEntity(m_focusEntityId);
    if (pEntity)
    {
      Vec3 pos = pEntity->GetPos();
			assert("!GetI3DEngine()->SetCameraFocus() is not supported");
//      GetMovieSystem()->gEnv->p3DEngine->SetCameraFocus( pos );
    }
  }

	bool bNodeAnimated = false;

	if (fov != m_fFOV)
	{
		m_fFOV = fov;
		bNodeAnimated = true;
	}

	if(dof != m_fDOF)
	{
		m_fDOF = dof;
		bNodeAnimated = true;
	}

	Vec4 shakeMult = Vec4(m_shakeParam[0].amplitudeMult,m_shakeParam[1].amplitudeMult,
												m_shakeParam[0].frequencyMult,m_shakeParam[1].frequencyMult);
	CCompoundSplineTrack *pShakeMultTrack = static_cast<CCompoundSplineTrack*>(GetTrackForParameter(APARAM_SHAKEMULT));
	if (pShakeMultTrack)
		pShakeMultTrack->GetValue(ec.time,shakeMult);

	if ( shakeMult != Vec4(m_shakeParam[0].amplitudeMult,m_shakeParam[1].amplitudeMult,
												m_shakeParam[0].frequencyMult,m_shakeParam[1].frequencyMult))
	{
		m_shakeParam[0].amplitudeMult = shakeMult.x;
		m_shakeParam[1].amplitudeMult = shakeMult.y;
		m_shakeParam[0].frequencyMult = shakeMult.z;
		m_shakeParam[1].frequencyMult = shakeMult.w;
		bNodeAnimated = true;
	}

	bool shakeOn[SHAKE_COUNT];
	shakeOn[0] = m_shakeParam[0].amplitudeMult != 0;
	shakeOn[1] = m_shakeParam[1].amplitudeMult != 0;

	IEntity *pEntity = GetEntity();

	if (pEntity != NULL && !ec.bSingleFrame 
	&& pShakeMultTrack && (shakeOn[0] || shakeOn[1]))
	{
		float time = gEnv->pTimer->GetFrameStartTime().GetSeconds();

		Quat rotation;
		rotation = pEntity->GetRotation();

		Ang3 angles = Ang3::GetAnglesXYZ(Matrix33(rotation))*180.0f/gf_PI;

		for (int i=0; i<SHAKE_COUNT; ++i)
		{
			if (shakeOn[i])
			{
				angles = m_shakeParam[i].ApplyCameraShake(time, m_prevTime, angles);
			}
		}

		rotation.SetRotationXYZ(angles*gf_PI/180.0f);

		m_rotate = rotation;
		pEntity->SetRotation(rotation,ENTITY_XFORM_TRACKVIEW);

		m_prevTime = time;

		bNodeAnimated = true;
	}

	if (bNodeAnimated && m_pOwner)
	{
		m_bIgnoreSetParam = true;
		m_pOwner->OnNodeAnimated(this);
		m_bIgnoreSetParam = false;
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimCameraNode::Reset()
{
	CAnimEntityNode::Reset();
  m_lastFocusKey = -1;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimCameraNode::SetParamValue( float time,AnimParamType param,float value )
{
	if (param == APARAM_FOV)
	{
		// Set default value.
		m_fFOV = value;
	}
	return CAnimEntityNode::SetParamValue( time,param,value );
}

//////////////////////////////////////////////////////////////////////////
bool CAnimCameraNode::SetParamValue( float time,AnimParamType param,const Vec3 &value )
{
	if (param == APARAM_SHAKEAMPA)
	{
		m_shakeParam[0].amplitude = value;
	}
	else if (param == APARAM_SHAKEAMPB)
	{
		m_shakeParam[1].amplitude = value;
	}
	else if (param == APARAM_SHAKEFREQA)
	{
		m_shakeParam[0].frequency = value;
		m_shakeParam[0].resetPhase = true;
	}
	else if (param == APARAM_SHAKEFREQB)
	{
		m_shakeParam[1].frequency = value;
		m_shakeParam[1].resetPhase = true;
	}

	return CAnimEntityNode::SetParamValue( time,param,value );
}

//////////////////////////////////////////////////////////////////////////
bool CAnimCameraNode::SetParamValue( float time,AnimParamType param,const Vec4 &value )
{
	if (param == APARAM_SHAKEMULT)
	{
		m_shakeParam[0].amplitudeMult = value.x;
		m_shakeParam[1].amplitudeMult = value.y;
		m_shakeParam[0].frequencyMult = value.z;
		m_shakeParam[1].frequencyMult = value.w;
	}
	else if (param == APARAM_SHAKENOISE)
	{
		m_shakeParam[0].noiseAmpMult = value.x;
		m_shakeParam[1].noiseAmpMult = value.y;
		m_shakeParam[0].noiseFreqMult = value.z;
		m_shakeParam[1].noiseFreqMult = value.w;
	}
	else if (param == APARAM_SHAKEWORKING)
	{
		m_shakeParam[0].timeOffset = value.x;
		m_shakeParam[1].timeOffset = value.y;
		m_shakeParam[0].resetPhase = true;
		m_shakeParam[1].resetPhase = true;
	}

	return CAnimEntityNode::SetParamValue( time,param,value );
}

//////////////////////////////////////////////////////////////////////////
bool CAnimCameraNode::GetParamValue( float time,AnimParamType param,float &value )
{
	if (CAnimEntityNode::GetParamValue(time,param,value))
	{
		return true;
	}

	if (param == APARAM_FOV)
	{
		value = m_fFOV;
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimCameraNode::GetParamValue( float time,AnimParamType param,Vec3 &value )
{
	if (CAnimEntityNode::GetParamValue(time,param,value))
	{
		return true;
	}

	if (param == APARAM_SHAKEAMPA)
	{
		value = m_shakeParam[0].amplitude;
		return true;
	}
	else if (param == APARAM_SHAKEAMPB)
	{
		value = m_shakeParam[1].amplitude;
		return true;
	}
	else if (param == APARAM_SHAKEFREQA)
	{
		value = m_shakeParam[0].frequency;
		return true;
	}
	else if (param == APARAM_SHAKEFREQB)
	{
		value = m_shakeParam[1].frequency;
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimCameraNode::GetParamValue( float time,AnimParamType param,Vec4 &value )
{
	if (CAnimEntityNode::GetParamValue(time,param,value))
	{
		return true;
	}

	if (param == APARAM_SHAKEMULT)
	{
		value = Vec4(m_shakeParam[0].amplitudeMult,m_shakeParam[1].amplitudeMult,
								m_shakeParam[0].frequencyMult,m_shakeParam[1].frequencyMult);
		return true;
	}
	else if (param == APARAM_SHAKENOISE)
	{
		value = Vec4(m_shakeParam[0].noiseAmpMult,m_shakeParam[1].noiseAmpMult,
								m_shakeParam[0].noiseFreqMult,m_shakeParam[1].noiseFreqMult);
		return true;
	}
	else if (param == APARAM_SHAKEWORKING)
	{
		value = Vec4(m_shakeParam[0].timeOffset,m_shakeParam[1].timeOffset,
								0.0f,0.0f);
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
IAnimTrack* CAnimCameraNode::CreateTrack(int nParamType)
{
	IAnimTrack *pTrack = CAnimEntityNode::CreateTrack(nParamType);
	if (nParamType == APARAM_FOV)
	{
		if (pTrack)
			pTrack->SetValue(0,m_fFOV,true);
	}
	else if (nParamType == APARAM_SHAKEMULT)
	{
		if (pTrack)
		{
			static_cast<CCompoundSplineTrack*>(pTrack)->SetValue(0,
					Vec4(m_shakeParam[0].amplitudeMult,m_shakeParam[1].amplitudeMult,
							m_shakeParam[0].frequencyMult,m_shakeParam[1].frequencyMult),
					true);
			pTrack->SetSubTrackName(0, "Amplitude A");
			pTrack->SetSubTrackName(1, "Amplitude B");
			pTrack->SetSubTrackName(2, "Frequency A");
			pTrack->SetSubTrackName(3, "Frequency B");
		}
	}
	return pTrack;
}

//////////////////////////////////////////////////////////////////////////
void CAnimCameraNode::Serialize( XmlNodeRef &xmlNode,bool bLoading,bool bLoadEmptyTracks )
{
	CAnimEntityNode::Serialize( xmlNode,bLoading,bLoadEmptyTracks );
	if (bLoading)
	{
		xmlNode->getAttr( "FOV",m_fFOV );
		xmlNode->getAttr( "AmplitudeA",m_shakeParam[0].amplitude );
		xmlNode->getAttr( "AmplitudeAMult",m_shakeParam[0].amplitudeMult );
		xmlNode->getAttr( "FrequencyA",m_shakeParam[0].frequency );
		xmlNode->getAttr( "FrequencyAMult",m_shakeParam[0].frequencyMult );
		xmlNode->getAttr( "NoiseAAmpMult",m_shakeParam[0].noiseAmpMult );
		xmlNode->getAttr( "NoiseAFreqMult",m_shakeParam[0].noiseFreqMult );
		xmlNode->getAttr( "TimeOffsetA",m_shakeParam[0].timeOffset );
		xmlNode->getAttr( "AmplitudeB",m_shakeParam[1].amplitude );
		xmlNode->getAttr( "AmplitudeBMult",m_shakeParam[1].amplitudeMult );
		xmlNode->getAttr( "FrequencyB",m_shakeParam[1].frequency );
		xmlNode->getAttr( "FrequencyBMult",m_shakeParam[1].frequencyMult );
		xmlNode->getAttr( "NoiseBAmpMult",m_shakeParam[1].noiseAmpMult );
		xmlNode->getAttr( "NoiseBFreqMult",m_shakeParam[1].noiseFreqMult );
		xmlNode->getAttr( "TimeOffsetB",m_shakeParam[1].timeOffset );
		xmlNode->getAttr( "DepthOfField",m_fDOF );

		m_shakeParam[0].resetPhase = true;
		m_shakeParam[1].resetPhase = true;
	}
	else
	{
		xmlNode->setAttr( "FOV",m_fFOV );
		xmlNode->setAttr( "AmplitudeA",m_shakeParam[0].amplitude );
		xmlNode->setAttr( "AmplitudeAMult",m_shakeParam[0].amplitudeMult );
		xmlNode->setAttr( "FrequencyA",m_shakeParam[0].frequency );
		xmlNode->setAttr( "FrequencyAMult",m_shakeParam[0].frequencyMult );
		xmlNode->setAttr( "NoiseAAmpMult",m_shakeParam[0].noiseAmpMult );
		xmlNode->setAttr( "NoiseAFreqMult",m_shakeParam[0].noiseFreqMult );
		xmlNode->setAttr( "TimeOffsetA",m_shakeParam[0].timeOffset );
		xmlNode->setAttr( "AmplitudeB",m_shakeParam[1].amplitude );
		xmlNode->setAttr( "AmplitudeBMult",m_shakeParam[1].amplitudeMult );
		xmlNode->setAttr( "FrequencyB",m_shakeParam[1].frequency );
		xmlNode->setAttr( "FrequencyBMult",m_shakeParam[1].frequencyMult );
		xmlNode->setAttr( "NoiseBAmpMult",m_shakeParam[1].noiseAmpMult );
		xmlNode->setAttr( "NoiseBFreqMult",m_shakeParam[1].noiseFreqMult );
		xmlNode->setAttr( "TimeOffsetB",m_shakeParam[1].timeOffset );
		xmlNode->setAttr( "DepthOfField",m_fDOF );
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimCameraNode::ApplyFocusKey( ISelectKey &key,SAnimContext &ec )
{
  m_focusEntityId = 0;
  if (key.szSelection[0])
  {
    IEntity *pEntity = gEnv->pEntitySystem->FindEntityByName( key.szSelection );
    if (pEntity)
    {
      m_focusEntityId = pEntity->GetId();
    }
  }
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CAnimCameraNode::SaveToColladaInFixedFPS(float fps) const
{
	XmlNodeRef colladaNode = CAnimEntityNode::SaveToColladaInFixedFPS(fps);

	string buf;
	// <library_cameras>
	XmlNodeRef libraryCamerasNode = colladaNode->newChild("library_cameras");
		XmlNodeRef cameraNode = libraryCamerasNode->newChild("camera");
		buf.Format("%s-lib", m_name.c_str());
		cameraNode->setAttr("id", buf.c_str());
		buf.Format("%sMesh", m_name.c_str());
		cameraNode->setAttr("name", buf.c_str());
			XmlNodeRef opticsNode = cameraNode->newChild("optics");
				XmlNodeRef techniqueCommonNode = opticsNode->newChild("technique_common");
					XmlNodeRef perspectiveNode = techniqueCommonNode->newChild("perspective");
						XmlNodeRef yfovNode = perspectiveNode->newChild("yfov");
						yfovNode->setAttr("sid", "yfov");
						buf.Format("%f", m_fFOV);
						yfovNode->setContent(buf.c_str());
						perspectiveNode->newChild("aspect_ratio")->setContent("1.777777");
						XmlNodeRef znearNode = perspectiveNode->newChild("znear");
						znearNode->setAttr("sid", "znear");
						znearNode->setContent("0.010000");
						XmlNodeRef zfarNode = perspectiveNode->newChild("zfar");
						zfarNode->setAttr("sid", "zfar");
						zfarNode->setContent("1000.000000");
	
	// Link the camera to the node.
	XmlNodeRef libraryVisualScenesNode = colladaNode->findChild("library_visual_scenes");
	if(libraryVisualScenesNode)
	{
		XmlNodeRef visualSceneNode = libraryVisualScenesNode->findChild("visual_scene");
		if(visualSceneNode)
		{
			XmlNodeRef node = visualSceneNode->findChild("node");
			if(node)
			{
				XmlNodeRef instanceCameraNode = node->newChild("instance_camera");
				buf.Format("#%s-lib", m_name.c_str());
				instanceCameraNode->setAttr("url", buf.c_str());
			}
		}
	}

	// Save the FOV track, if exists.
	XmlNodeRef libraryAnimationsNode = colladaNode->findChild("library_animations");
	IAnimTrack *pFOVTrack = GetTrackForParameter(APARAM_FOV);
	if(pFOVTrack)
	{
		string trackName, targetName;
		// Note that for the FOV track we should use a camera library name instead of a camera name.
		trackName.Format("%s-lib-%s", m_name.c_str(), GetParamName(pFOVTrack->GetParameterType()));
		targetName.Format("%s-lib", m_name.c_str());
		pFOVTrack->SaveToColladaInFixedFPS(libraryAnimationsNode, targetName.c_str(), 
																			trackName.c_str(), fps);
	}

	// Reorder some childs
	if(libraryVisualScenesNode)
	{
		colladaNode->removeChild(libraryVisualScenesNode);
		colladaNode->addChild(libraryVisualScenesNode);
	}
	XmlNodeRef sceneNode = colladaNode->findChild("scene");
	if(sceneNode)
	{
		colladaNode->removeChild(sceneNode);
		colladaNode->addChild(sceneNode);
	}

	return colladaNode;
}

//////////////////////////////////////////////////////////////////////////
bool CAnimCameraNode::LoadFromCollada(XmlNodeRef colladaNode)
{
	// Check the <library_cameras> and a <camera> in it.
	XmlNodeRef libraryCamerasNode = colladaNode->findChild("library_cameras");
	if (!libraryCamerasNode)
		return false;
	XmlNodeRef cameraNode = libraryCamerasNode->findChild("camera");
	if (!cameraNode)
		return false;
	// Check the <instance_camrea> in the <node>.
	XmlNodeRef libraryVisualScenesNode = colladaNode->findChild("library_visual_scenes");
	if (!libraryVisualScenesNode)
		return false;
	XmlNodeRef visualSceneNode = libraryVisualScenesNode->findChild("visual_scene");
	if (!visualSceneNode)
		return false;
	XmlNodeRef node = visualSceneNode->findChild("node");
	if (!node)
		return false;
	XmlNodeRef instanceCameraNode = node->findChild("instance_camera");
	if (!instanceCameraNode)
		return false;
	// Check if the reference is correct.
	XmlString referencedCam;
	if (instanceCameraNode->getAttr("url", referencedCam) == false)
		return false;
	XmlString camName, camNameRef;
	if (cameraNode->getAttr("id", camName) == false)
		return false;
	camNameRef = ("#" + camName).c_str();
	if (referencedCam != camNameRef)
		return false;

	// Check whether there is an FOV animation data which are channeled to this camera.
	XmlNodeRef libraryAnimationsNode = colladaNode->findChild("library_animations");
	if (!libraryAnimationsNode)
		return false;
	XmlNodeRef parentNode = libraryAnimationsNode;
	// It can be the case of animations right under the library tag,
	// or the case of animations under another parent animation tag inside the library tag.
	if (libraryAnimationsNode->getChildCount() == 1
		&& libraryAnimationsNode->getChild(0)->findChild("channel") == 0)
		parentNode = libraryAnimationsNode->getChild(0);
	camName += "/";
	for (int i=0; i<parentNode->getChildCount(); ++i)
	{
		XmlNodeRef animationNode = parentNode->getChild(i);
		XmlNodeRef channelNode = animationNode->findChild("channel");
		if (!channelNode)
			continue;
		XmlString channelTarget;
		if (channelNode->getAttr("target", channelTarget) == false)
			continue;
		if (channelTarget.substr(0, camName.length()) != camName)
			continue;
		XmlString whichChannel(channelTarget.substr(camName.length()));
		// Now we've found a valid animation data for this camera's FOV.
		if (whichChannel == "yfov")
		{
			// Create a FOV track if not created already.
			if (GetTrackForParameter(APARAM_FOV) == NULL)
				CreateTrack(APARAM_FOV);
			IAnimTrack *pFOVTrack = GetTrackForParameter(APARAM_FOV);
			// Only the 2D Bezier spline track is supported currently.
			if (pFOVTrack->GetType() != ATRACK_BEZIER_FLOAT)
				return false;
			pFOVTrack->LoadFromCollada(animationNode);
		}
	}

	return CAnimEntityNode::LoadFromCollada(colladaNode);
}

Ang3 CAnimCameraNode::ShakeParam::ApplyCameraShake( float time, float prevTime, Ang3 angles )
{
	Ang3 rotation;
	Ang3 rotationNoise;

	float noiseAmpMult = this->amplitudeMult*this->noiseAmpMult;
	float noiseFreqMult = this->frequencyMult*this->noiseFreqMult;

	if (this->resetPhase == true)
	{
		float t = time+this->timeOffset;

		this->phase = Vec3((t+15.0f)*this->frequency.x*this->frequencyMult,
													(t+55.1f)*this->frequency.y*this->frequencyMult,
													(t+101.2f)*this->frequency.z*this->frequencyMult);
		this->phaseNoise = Vec3((t+70.0f)*this->frequency.x*noiseFreqMult,
															(t+10.0f)*this->frequency.y*noiseFreqMult,
															(t+30.5f)*this->frequency.z*noiseFreqMult);

		this->resetPhase = false;
	}
	else
	{
		float t = time-prevTime;

		this->phase += t*this->frequency*this->frequencyMult;
		this->phaseNoise += t*this->frequency*noiseFreqMult;
	}

	rotation.x = gEnv->pSystem->GetNoiseGen()->Noise1D(this->phase.x)*this->amplitude.x*this->amplitudeMult;
	rotationNoise.x = gEnv->pSystem->GetNoiseGen()->Noise1D(this->phaseNoise.x)*this->amplitude.x*noiseAmpMult;

	rotation.y = gEnv->pSystem->GetNoiseGen()->Noise1D(this->phase.y)*this->amplitude.y*this->amplitudeMult;
	rotationNoise.y = gEnv->pSystem->GetNoiseGen()->Noise1D(this->phaseNoise.y)*this->amplitude.y*noiseAmpMult;

	rotation.z = gEnv->pSystem->GetNoiseGen()->Noise1D(this->phase.z)*this->amplitude.z*this->amplitudeMult;
	rotationNoise.z = gEnv->pSystem->GetNoiseGen()->Noise1D(this->phaseNoise.z)*this->amplitude.z*noiseAmpMult;

	angles += rotation+rotationNoise;	
	
	return angles;
}

#undef s_nodeParamsInitialized
#undef s_nodeParams
#undef AddSupportedParam

