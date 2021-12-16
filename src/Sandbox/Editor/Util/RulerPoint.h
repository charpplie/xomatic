/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2009.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Ruler point helper, used by CRuler
  
 -------------------------------------------------------------------------
  History:
  - 02:11:2009: Created by Kevin Kirst

*************************************************************************/

#ifndef __RULERPOINT_H__
#define __RULERPOINT_H__

class CRuler;

//! Ruler point helper - Defines a point for the ruler
class CRulerPoint
{
public:
	CRulerPoint();
	CRulerPoint& operator =(CRulerPoint const& other);

	void Reset();
	void Render(IRenderer *pRenderer);

	//! Set helpers
	void Set(const Vec3 &vPos);
	void Set(CBaseObject *pObject);

	//! Returns is point has valid data in it (in use)
	bool IsEmpty() const;

	//! Helpers to get correct data out
	Vec3 GetPos() const;
	Vec3 GetMidPoint(const CRulerPoint& otherPoint) const;
	float GetDistance(const CRulerPoint& otherPoint) const;
	CBaseObject* GetObject() const;

private:
	enum EType
	{
		eType_Invalid,
		eType_Point,
		eType_Object,

		eType_COUNT,
	};
	EType m_type;

	Vec3 m_vPoint;
	GUID m_ObjectGUID;
};

#endif //__RULERPOINT_H__
