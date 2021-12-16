////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name: AnimScreenFaderNode.h
//  Version:   v1.00
//  Created:   27-04-2010 by Dongjoon Kim
//  Description:
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 

#ifndef __ANIMSCREENFADERNODE_H__
#define __ANIMSCREENFADERNODE_H__

#pragma once

#include "AnimNode.h"

class CScreenFaderTrack;

class CAnimScreenFaderNode : public CAnimNode
{
public:
	//-----------------------------------------------------------------------------
	//!
	CAnimScreenFaderNode();
	~CAnimScreenFaderNode();
	static void Initialize();

	//-----------------------------------------------------------------------------
	//!
	virtual EAnimNodeType GetType() const { return ANODE_SCREENFADER; }

	//-----------------------------------------------------------------------------
	//! Overrides from CAnimNode
	virtual void Animate(SAnimContext &ac);

	virtual void CreateDefaultTracks();

	virtual void Reset();

	virtual void Activate( bool bActivate );

	virtual void Serialize( XmlNodeRef &xmlNode,bool bLoading,bool bLoadEmptyTracks );

	//-----------------------------------------------------------------------------
	//! Overrides from IAnimNode
	virtual int GetParamCount() const;

	virtual bool GetParamInfo( int nIndex, SParamInfo &info ) const;

	virtual bool GetParamInfoFromId( int paramId, SParamInfo &info ) const;

	virtual void Render();

protected:

	virtual bool NeedToRender() const { return true; }

private:
	CAnimScreenFaderNode(const CAnimScreenFaderNode&);
	CAnimScreenFaderNode& operator = (const CAnimScreenFaderNode&);

private:
	//-----------------------------------------------------------------------------
	//!
	bool SetTexture(CScreenFaderTrack* pFaderTrack, int index);
	void PrecacheTexData();

	Vec4		m_drawColor;
	Vec4		m_startColor;
	ITexture*	m_pTexture;
	bool		m_bActive;
	bool		m_bUseTexture;

	float		m_screenWidth, m_screenHeight;
	int			m_lastActivatedKey;

	bool		m_texPrecached;
};

#endif//__ANIMSCREENFADERNODE_H__
