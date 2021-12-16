////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   StatCGFPhysicalize.h
//  Version:     v1.00
//  Created:     8/11/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __StatCGFPhysicalize_h__
#define __StatCGFPhysicalize_h__
#pragma once

#include "PhysWorld.h"
#include "CGFContent.h"

//////////////////////////////////////////////////////////////////////////
class CPhysicsInterface
{
public:
	CPhysicsInterface();
	~CPhysicsInterface();

	enum EPhysicalizeResult
	{
		ePR_Empty,
		ePR_Ok,
		ePR_Fail
	};

	EPhysicalizeResult Physicalize( CNodeCGF *pNodeCGF,CContentCGF *pCGF );
	bool DeletePhysicalProxySubsets( CNodeCGF *pNodeCGF,bool bHasFaceMap );
	void ProcessBreakablePhysics( CContentCGF *pCompiledCGF,CContentCGF *pSrcCGF );
	IPhysicalWorld *GetPhysicalWorld() { return m_pPhysicalWorld; }

	// When node already physicalized, physicalize it again.
	void RephysicalizeNode( CNodeCGF *pNodeCGF,CContentCGF *pCGF );

private:
	EPhysicalizeResult PhysicalizeGeomType( int nGeomType,CNodeCGF *pNodeCGF,CContentCGF *pCGF );

	IPhysicalWorld *m_pPhysicalWorld;
	CPhysWorldLoader m_physLoader;
};

#endif // __StatCGFPhysicalize_h__
