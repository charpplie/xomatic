////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   controllertcb.h
//  Version:     v1.00
//  Created:     12/11/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: TCB controller implementation.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __controllertcb_h__
#define __controllertcb_h__
#pragma once

#include "TCBSpline.h"


// CJoint animation.
struct CControllerTCB : public IController
{

	//uint32 GetID() const {
	//	return m_ID;
	//}
	size_t SizeOfController() const	{
		return sizeof(CControllerTCB) + m_posTrack.sizeofThis() +  m_rotTrack.sizeofThis() + m_sclTrack.sizeofThis();
	} 

	size_t ApproximateSizeOfThis() const	{
		return SizeOfController();
	} 

	Status4 GetOPS( f32 t, Quat& rot, Vec3& pos, Diag33& scl);
	Status4 GetOP( f32 t, Quat& rot, Vec3& pos);
	uint32 GetO( f32 t, Quat& rot );
	uint32 GetP( f32 t, Vec3& pos );
	uint32 GetS( f32 t, Diag33& scl );
	uint32 HasPositionChannel() { return 0; };

	virtual EControllerInfo GetControllerType() 
	{
		return eTCB;
	}

	CInfo GetControllerInfo() const
	{
		CInfo info;
		info.m_numKeys = 0;
		return info;
	}

	virtual size_t GetRotationKeysNum() {
		return 0;
	}

	virtual size_t GetPositionKeysNum() {
		return 0;
	}

	Status4 m_active;
//	uint32 m_ID;
	spline::TCBSpline<Vec3> m_posTrack;
	spline::TCBAngleAxisSpline m_rotTrack;
	spline::TCBSpline<Vec3> m_sclTrack;

	CControllerTCB()
	{
	//	m_active.o = m_active.p = m_active.s = 0;
		m_nControllerId=0xaaaa5555;
	}

	void GetMemoryUsage( ICrySizer *pSizer ) const 
	{ 
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_posTrack);
		pSizer->AddObject(m_rotTrack);
		pSizer->AddObject(m_sclTrack);
	}
};


#endif // __controllertcb_h__

