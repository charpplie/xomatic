////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name: LayerNode.h
//  Version:   v1.00
//  Created:   19-03-2010 by Dongjoon Kim
//  Description:	Header of layer node to control entities properties in the
//					specific layer.
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 

#ifndef __LAYERNODE_H__
#define __LAYERNODE_H__

#pragma once

#include "AnimNode.h"

class CLayerNode : public CAnimNode
{
public:
	//-----------------------------------------------------------------------------
	//!
	CLayerNode();

	//-----------------------------------------------------------------------------
	//!
	virtual EAnimNodeType GetType() const { return ANODE_LAYER; }

	//-----------------------------------------------------------------------------
	//! Overrides from CAnimNode
	virtual void Animate(SAnimContext &ec);

	virtual void CreateDefaultTracks();

	virtual void Reset();

	virtual void Activate( bool bActivate );

	virtual void Serialize( XmlNodeRef &xmlNode,bool bLoading,bool bLoadEmptyTracks );

	//-----------------------------------------------------------------------------
	//! Overrides from IAnimNode
	virtual int GetParamCount() const;

	virtual bool GetParamInfo( int nIndex, SParamInfo &info ) const;

	virtual bool GetParamInfoFromId( int paramId, SParamInfo &info ) const;

private:

	bool m_bInit;
	bool m_bPreVisibility;

};

#endif//__LAYERNODE_H__