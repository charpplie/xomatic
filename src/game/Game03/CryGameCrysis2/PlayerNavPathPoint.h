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


#ifndef __PLAYERNAVPATHPOINT_H__
#define __PLAYERNAVPATHPOINT_H__


//! Nav path point helper - Defines a destination point
class CPlayerNavPathPoint
{
public:
	CPlayerNavPathPoint();
	CPlayerNavPathPoint& operator =(CPlayerNavPathPoint const& other);

	void Reset();
	
	//! Set helpers
	void Set(const Vec3 &vPos);
	void Set(EntityId targetEntityId);
	void Set(const char* pathName);

	//! Returns if point has valid data in it (in use)
	bool IsEmpty() const;

	//! Helpers to get correct data out
	Vec3 GetPos() const; // valid to call for both point and entity data types
	Vec3 GetMidPoint(const CPlayerNavPathPoint& otherPoint) const;
	float GetDistance(const CPlayerNavPathPoint& otherPoint) const;
	const char* GetPath() const;
	EntityId GetTargetEntityID() const;

private:
	enum EType
	{
		eType_Invalid,
		eType_Point,
		eType_Entity,
		eType_Path,

		eType_COUNT,
	};
	EType m_type;

	Vec3 m_vPoint;
	EntityId m_targetEntityId;
	CryFixedStringT<32> m_pathName;
};


//////////////////////////////////////////////////////////////////////////
inline EntityId CPlayerNavPathPoint::GetTargetEntityID() const
{
	return m_targetEntityId;
}

#endif //__PLAYERNAVPATHPOINT_H__
