////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name: CommentNodeAnimator.h
//  Version:   v1.00
//  Created:   09-04-2010 by Dongjoon Kim
//  Description:
// -------------------------------------------------------------------------  
//  History: Animate CommentNode in TracView only for in editor.
//
//////////////////////////////////////////////////////////////////////////// 

#ifndef __COMMENTNODEANIMATOR_H__
#define __COMMENTNODEANIMATOR_H__

#pragma once

class CCommentNodeAnimator : public IAnimNodeAnimator
{
public:
	//-----------------------------------------------------------------------------
	//!
	CCommentNodeAnimator(IAnimNode* pCommentNode, CCommentContext* pCC);

	//-----------------------------------------------------------------------------
	//!
	void Animate( IAnimNode *pNode , const SAnimContext& ac );

protected:
	//-----------------------------------------------------------------------------
	//!
	virtual ~CCommentNodeAnimator();

	//-----------------------------------------------------------------------------
	//!
	void AnimateCommentTextTrack(IAnimTrack* pTrack, const SAnimContext& ac);

	//-----------------------------------------------------------------------------
	//!
	static int GetActiveKeyIndex(const IAnimTrack* pTrack, float fTime);

	IAnimNode*			m_pCommentNode;
	CCommentContext*	m_pCommentContext; 
};

#endif//__COMMENTNODEANIMATOR_H__