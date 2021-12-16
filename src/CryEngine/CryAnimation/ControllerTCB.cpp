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

#include "stdafx.h"
#include "CharacterManager.h"



Status4 CControllerTCB::GetOPS( f32 ntime, Quat& rot, Vec3& pos, Diag33& scl)
{
	f32 key = ntime;
	if (m_active.o)
	{
		Quat out;
		m_rotTrack.interpolate( key, out );
		rot = !out;
	}

	if (m_active.p)
	{
		Vec3 out;
		m_posTrack.interpolate( key,out );
		pos=out/100.0f;	// Position controller from Max must be scalled 100 times down.
	}

	if (m_active.s) 
	{
		Vec3 out;
		m_sclTrack.interpolate( key,out );
		scl=Diag33(out);
	}
	return m_active;
}

Status4 CControllerTCB::GetOP ( f32 ntime, Quat& rot, Vec3& pos)
{
	f32 key = ntime;
	if (m_active.o)
	{
		Quat out;
		m_rotTrack.interpolate( key, out );
		rot = !out;
	}

	if (m_active.p)
	{
		Vec3 out;
		m_posTrack.interpolate( key,out );
		pos=out/100.0f;	// Position controller from Max must be scalled 100 times down.
	}

	return m_active;
}



uint32 CControllerTCB::GetO( f32 ntime, Quat& rot )
{
	f32 key = ntime;
	if (m_active.o)
	{
		Quat out;
		m_rotTrack.interpolate( key, out );
		rot = !out;
	}
	return m_active.o;
}

uint32 CControllerTCB::GetP( f32 ntime, Vec3& pos )
{
	f32 key = ntime;
	if (m_active.p)
	{
		Vec3 out;
		m_posTrack.interpolate( key,out );
		pos=out/100.0f;	// Position controller from Max must be scalled 100 times down.
	}
	return m_active.p;
}

uint32 CControllerTCB::GetS( f32 ntime, Diag33& scl )
{
	f32 key = ntime;
	if (m_active.s)
	{
		Vec3 out;
		m_sclTrack.interpolate( key,out );
		scl=Diag33(out);
	}
	return m_active.s;
}

