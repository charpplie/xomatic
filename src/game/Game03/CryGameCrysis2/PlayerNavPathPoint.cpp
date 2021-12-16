/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Player nav path point helper, used by CPlayerNavPathAgent

-------------------------------------------------------------------------
History:
- 02:11:2009: Created by Kevin Kirst
- 18:11:2009: Made Nav Path code based on Kevin's Ruler code

*************************************************************************/

#include "StdAfx.h"
#include "PlayerNavPathPoint.h"

#include "IRenderAuxGeom.h"

//////////////////////////////////////////////////////////////////////////
CPlayerNavPathPoint::CPlayerNavPathPoint()
: m_type(eType_Invalid)
, m_vPoint(ZERO)
, m_targetEntityId(0)
, m_pathName("")
{
	Reset();
}

//////////////////////////////////////////////////////////////////////////
CPlayerNavPathPoint& CPlayerNavPathPoint::operator =(CPlayerNavPathPoint const& other)
{
	if (this != &other)
	{
		Reset(); // Manage deselect of current object, etc.
		
		m_type = other.m_type;
		m_vPoint = other.m_vPoint;
		m_targetEntityId = other.m_targetEntityId;
	}

	return *this;
}

//////////////////////////////////////////////////////////////////////////
void CPlayerNavPathPoint::Reset()
{
	m_type = eType_Invalid;
	m_vPoint.zero();
	m_targetEntityId = 0;
	m_pathName = "";
}

//////////////////////////////////////////////////////////////////////////
void CPlayerNavPathPoint::Set(const Vec3 &vPos)
{
	Reset();

	m_type = eType_Point;
	m_vPoint = vPos;
}

//////////////////////////////////////////////////////////////////////////
void CPlayerNavPathPoint::Set(EntityId targetEntityId)
{
	Reset();

	m_type = eType_Entity;
	m_targetEntityId = targetEntityId;
}

//////////////////////////////////////////////////////////////////////////
void CPlayerNavPathPoint::Set(const char* pathName)
{
	Reset();

	m_type = eType_Path;
	m_pathName = pathName;
}

//////////////////////////////////////////////////////////////////////////
bool CPlayerNavPathPoint::IsEmpty() const
{
	bool bResult = true;

	switch (m_type)
	{
		case eType_Invalid:
			bResult = true;
			break;

		case eType_Point:
			bResult = m_vPoint.IsZero();
			break;

		case eType_Entity:
			bResult = (m_targetEntityId == 0);
			break;

		case eType_Path:
			bResult = m_pathName.empty();
			break;
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
Vec3 CPlayerNavPathPoint::GetPos() const
{
	Vec3 vResult(ZERO);

	switch (m_type)
	{
		case eType_Point:
			vResult = m_vPoint;
			break;

		case eType_Entity:
			{
				IEntity *pEnt = gEnv->pEntitySystem->GetEntity(m_targetEntityId);
				if (pEnt)
					vResult = pEnt->GetWorldPos();
			}
			break;
	}

	return vResult;
}

//////////////////////////////////////////////////////////////////////////
const char* CPlayerNavPathPoint::GetPath() const
{
	if(m_type == eType_Path)
	{
		return m_pathName.c_str();
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////
Vec3 CPlayerNavPathPoint::GetMidPoint(const CPlayerNavPathPoint& otherPoint) const
{
	Vec3 vResult(ZERO);

	if (!IsEmpty() && !otherPoint.IsEmpty())
	{
		vResult = GetPos() + (otherPoint.GetPos()-GetPos())*0.5f;
	}
	else if (!IsEmpty())
	{
		vResult = GetPos();
	}
	else
	{
		vResult = otherPoint.GetPos();
	}

	return vResult;
}

//////////////////////////////////////////////////////////////////////////
float CPlayerNavPathPoint::GetDistance(const CPlayerNavPathPoint& otherPoint) const
{
	float fResult = 0.0f;

	if (!IsEmpty() && !otherPoint.IsEmpty())
	{
		fResult = GetPos().GetDistance(otherPoint.GetPos());
	}

	return fResult;
}
