////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name: AnimScreenFaderNode.cpp
//  Version:   v1.00
//  Created:   27-04-2010 by Dongjoon Kim
//  Description:
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 

#include "StdAfx.h"
#include "AnimScreenFaderNode.h"
#include "ScreenFaderTrack.h"
#include <IRenderer.h>

//-----------------------------------------------------------------------------
namespace
{
	bool s_screenFaderNodeParamsInitialized = false;
	std::vector<IAnimNode::SParamInfo> s_screenFaderNodeParams;

	void AddSupportedParams( const char *sName,int paramId,EAnimValue valueType )
	{
		IAnimNode::SParamInfo param;
		param.name = sName;
		param.paramId = paramId;
		param.valueType = valueType;
		s_screenFaderNodeParams.push_back( param );
	}
};

//-----------------------------------------------------------------------------
bool CalculateIsolatedKeyColor(const IScreenFaderKey& key, float fTime, Vec4& colorOut)
{
	float ratio = fTime - key.time;

	if(ratio<0.f)
		return false;

	if(key.m_fadeTime == 0.f)
	{
		colorOut = key.m_fadeColor;
		if(key.m_fadeType == IScreenFaderKey::eFT_FadeIn)
			colorOut.w = 0.f;
		else
			colorOut.w = 1.f;
	}
	else
	{
		colorOut = key.m_fadeColor;
		ratio = ratio / key.m_fadeTime;
		if(key.m_fadeType == IScreenFaderKey::eFT_FadeIn)
			colorOut.w = MAX(0.f,1.f-ratio);
		else
			colorOut.w = MIN(1.f,ratio);
	}

	return true;
}

//-----------------------------------------------------------------------------
CAnimScreenFaderNode::CAnimScreenFaderNode()
:m_pTexture(0),m_bActive(false),m_bUseTexture(false)
,m_screenWidth(800.f),m_screenHeight(600.f),m_lastActivatedKey(-1)
,m_texPrecached(false)
{
	m_drawColor = Vec4(1,1,1,1);
	m_startColor = Vec4(1,1,1,1);

	CAnimScreenFaderNode::Initialize();
	PrecacheTexData();
}

CAnimScreenFaderNode::~CAnimScreenFaderNode()
{
}

//-----------------------------------------------------------------------------
void CAnimScreenFaderNode::Initialize()
{
	if (!s_screenFaderNodeParamsInitialized)
	{
		s_screenFaderNodeParamsInitialized = true;
		s_screenFaderNodeParams.reserve(1);
		AddSupportedParams( "Fader",APARAM_SCREENFADER,AVALUE_SCREENFADER );
	}
}

//-----------------------------------------------------------------------------
void CAnimScreenFaderNode::Animate(SAnimContext &ac)
{
	CScreenFaderTrack* pTrack =
		static_cast<CScreenFaderTrack*>(GetTrackForParameter(APARAM_SCREENFADER));

	if (!pTrack)
		return;

	if(pTrack->GetNumKeys()==0)
		return;

	if(ac.trackMask && ((ac.trackMask & (1 << pTrack->GetType())) == 0))
		return;

	if(ac.bSingleFrame)
		m_lastActivatedKey = -1;

	IScreenFaderKey key;
	int nActiveKeyIndex = pTrack->GetActiveKey(ac.time,&key);

	if(nActiveKeyIndex>=0)
	{
		if(m_lastActivatedKey!=nActiveKeyIndex)
		{
			m_lastActivatedKey = nActiveKeyIndex;
			m_bActive = true;

			if(key.m_strTexture[0])
			{
				if(SetTexture(pTrack, nActiveKeyIndex))
					m_bUseTexture = true;
				else
					m_bUseTexture = false;
			}
			else
			{
				m_bUseTexture = false;
			}
		}

		if(m_bActive || key.m_fadeTime + key.time > ac.time)
		{
			float ratio = (key.m_fadeTime >0) ? (ac.time - key.time) / key.m_fadeTime : 1.f;
			if(ratio < 0.f) ratio = 0.f;
			ratio = MIN(ratio, 1.f);

			switch( key.m_fadeChangeType )
			{
			case IScreenFaderKey::eFCT_Square:
				ratio = ratio * ratio;	
				break;

			case  IScreenFaderKey::eFCT_CubicSquare:
				ratio = ratio * ratio * ratio;
				break;

			case IScreenFaderKey::eFCT_SquareRoot:
				ratio = sqrt(ratio);
				break;

			case IScreenFaderKey::eFCT_Sin:
				ratio = sinf(ratio*3.14159265f*0.5f);
				break;
			}

			if(!key.m_bUseCurColor || nActiveKeyIndex==0)
			{
				m_startColor = key.m_fadeColor;
			}
			else
			{
				IScreenFaderKey preKey;
				pTrack->GetKey(nActiveKeyIndex-1,&preKey);
				CalculateIsolatedKeyColor(preKey, ac.time, m_startColor);
			}
			
			if(key.m_fadeType == IScreenFaderKey::eFT_FadeIn)
			{
				if(!key.m_bUseCurColor || nActiveKeyIndex==0)
				{
					m_startColor.w = 1.f;
				}
				key.m_fadeColor.w = 0.f;
			}
			else
			{
				if(!key.m_bUseCurColor || nActiveKeyIndex==0)
				{
					m_startColor.w = 0.f;
				}
				key.m_fadeColor.w = 1.f;
			}

			m_drawColor = m_startColor + ( key.m_fadeColor - m_startColor) * ratio;

			if(m_drawColor.w < 0.01f)
				m_bActive = false;
			else
				m_bActive = true;
		}
	}
	else
	{
		m_bActive = false;
	}
}

//-----------------------------------------------------------------------------
void CAnimScreenFaderNode::CreateDefaultTracks()
{
	CreateTrack(APARAM_SCREENFADER);
}

//-----------------------------------------------------------------------------
void CAnimScreenFaderNode::Reset()
{
	CAnimNode::Reset();
	m_bActive = false;
}

//-----------------------------------------------------------------------------
void CAnimScreenFaderNode::Activate( bool bActivate )
{
	if(bActivate)
	{
		m_bActive = false;
	}

	if (m_texPrecached == false)
		PrecacheTexData();
}

//-----------------------------------------------------------------------------
void CAnimScreenFaderNode::Serialize
( XmlNodeRef &xmlNode,bool bLoading,bool bLoadEmptyTracks )
{
	CAnimNode::Serialize(xmlNode, bLoading, bLoadEmptyTracks);

	if (bLoading)
		PrecacheTexData();
}

//-----------------------------------------------------------------------------
int CAnimScreenFaderNode::GetParamCount() const
{
	return s_screenFaderNodeParams.size();
}

//-----------------------------------------------------------------------------
bool CAnimScreenFaderNode::GetParamInfo( int nIndex, SParamInfo &info ) const
{
	if (nIndex >= 0 && nIndex < (int)s_screenFaderNodeParams.size())
	{
		info = s_screenFaderNodeParams[nIndex];
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool CAnimScreenFaderNode::GetParamInfoFromId( int paramId, SParamInfo &info ) const
{
	for (size_t i = 0; i < s_screenFaderNodeParams.size(); ++i)
	{
		if (s_screenFaderNodeParams[i].paramId == paramId)
		{
			info = s_screenFaderNodeParams[i];
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
void CAnimScreenFaderNode::Render()
{
	if(!m_bActive)
		return;

	if (gEnv->pRenderer)
	{
		gEnv->pRenderer->SetState(GS_BLSRC_SRCALPHA|GS_BLDST_ONEMINUSSRCALPHA|GS_NODEPTHTEST);
		gEnv->pRenderer->Draw2dImage(0, 0, m_screenWidth, m_screenHeight,
			(m_bUseTexture && m_pTexture) ? m_pTexture->GetTextureID() : -1,
			0.0f,0.0f,1.0f,1.0f,
			0.f,
			m_drawColor.x,m_drawColor.y,m_drawColor.z,m_drawColor.w,0.f);
	}
}

//-----------------------------------------------------------------------------
bool CAnimScreenFaderNode::SetTexture(CScreenFaderTrack* pFaderTrack, int index)
{
	m_pTexture = pFaderTrack->GetTexture(index);
	if (gEnv->IsEditor())
	{
		// Check if textures should be reloaded.
		bool bNeedTexReload = m_pTexture == NULL; // Not yet loaded
		if (m_pTexture)
		{
			IScreenFaderKey key;
			pFaderTrack->GetKey(index, &key);
			if (strcmp(key.m_strTexture, m_pTexture->GetName()) != 0)
				bNeedTexReload = true;                // Loaded, but a different texture
		}
		if (bNeedTexReload)
		{
			// Ok, try to reload.
			pFaderTrack->PreloadTextures();
			m_pTexture = pFaderTrack->GetTexture(index);
		}
	}
	return m_pTexture != 0;
}

void CAnimScreenFaderNode::PrecacheTexData()
{
	size_t const nParamCount = m_tracks.size();
	for (size_t nParamIndex = 0; nParamIndex < nParamCount; ++nParamIndex)
	{
		int const nTrackType = m_tracks[nParamIndex].paramId;
		IAnimTrack* pTrack = m_tracks[nParamIndex].track;

		switch (nTrackType)
		{
		case APARAM_SCREENFADER:
			{
				CScreenFaderTrack* pFaderTrack = static_cast<CScreenFaderTrack*>(pTrack);
				pFaderTrack->PreloadTextures();
			}
			break;
		}
	}
	m_texPrecached = true;
}