////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name: CommentNodeAnimator.cpp
//  Version:   v1.00
//  Created:   09-04-2010 by Dongjoon Kim
//  Description:
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 

#include "StdAfx.h"
#include "CommentNodeAnimator.h"

//-----------------------------------------------------------------------------
CCommentNodeAnimator::CCommentNodeAnimator(IAnimNode* pCommentNode, CCommentContext* pCC)
{
	assert(pCommentNode && pCC);

	m_pCommentNode = pCommentNode;
	m_pCommentContext = pCC;
}

//-----------------------------------------------------------------------------
CCommentNodeAnimator::~CCommentNodeAnimator()
{
	m_pCommentNode = 0;
	m_pCommentContext = 0;
}

//-----------------------------------------------------------------------------
void CCommentNodeAnimator::Animate( IAnimNode *pNode , const SAnimContext& ac )
{
	if(pNode!=m_pCommentNode)
		return;

	int trackCount = pNode->GetTrackCount();
	Vec2 pos(0,0);
	for (int i = 0; i < trackCount; ++i)
	{
		IAnimTrack *pTrack = pNode->GetTrackByIndex(i);

		if(ac.trackMask && ((ac.trackMask & (1 << pTrack->GetType())) == 0))
			continue;

		switch (pTrack->GetParameterType())
		{
			case APARAM_COMMENT_TEXT:
				{
					AnimateCommentTextTrack(pTrack, ac);
				}
				break;
			case APARAM_POS_X:
				{
					pTrack->GetValue(ac.time, pos.x);
				}
				break;
			case APARAM_POS_Y:
				{
					pTrack->GetValue(ac.time, pos.y);
				}
				break;
		}
	}

	// Position mapping from [0,100] to [-1,1]
	pos = (pos - Vec2(50,50))/50.f;
	
	m_pCommentContext->m_unitPos = pos;
}

//-----------------------------------------------------------------------------
void CCommentNodeAnimator::AnimateCommentTextTrack(IAnimTrack* pTrack, const SAnimContext &ac)
{
	if(pTrack->GetNumKeys() == 0)
		return;

	int nActiveKeyIndex = GetActiveKeyIndex(pTrack, ac.time);

	if(nActiveKeyIndex>=0)
	{
		ICommentKey key;
		pTrack->GetKey(nActiveKeyIndex, &key);

		if(key.m_duration>0 && ac.time < pTrack->GetKeyTime(nActiveKeyIndex)+key.m_duration)
		{
			m_pCommentContext->m_strComment = key.m_strComment.c_str();
			strcpy_s(m_pCommentContext->m_strFont,  key.m_strFont);
			m_pCommentContext->m_color = key.m_color;
			m_pCommentContext->m_align = key.m_align;
			m_pCommentContext->m_size = key.m_size;
		}
		else
		{
			m_pCommentContext->m_strComment = 0;
		}
	}
	else
	{
		m_pCommentContext->m_strComment = 0;
	}
}

int CCommentNodeAnimator::GetActiveKeyIndex(const IAnimTrack* pTrack, float fTime)
{
	int nkeys = pTrack->GetNumKeys();
	if (nkeys == 0)
	{
		return -1;
	}

	// Time is before first key.
	if (pTrack->GetKeyTime(0) > fTime)
	{
		return -1;
	}

	for (int i = 0; i < nkeys; i++)
	{
		if (fTime >= pTrack->GetKeyTime(i))
		{
			if( (i == nkeys-1) || (fTime < pTrack->GetKeyTime(i+1) ) )
			{
				return i;
			}
		}
		else
		{
			break;
		}
	}

	return -1;
}