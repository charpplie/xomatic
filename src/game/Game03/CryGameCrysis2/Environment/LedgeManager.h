/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Climb-able Ledge system 

-------------------------------------------------------------------------
History:
- 20:04:2009: Created by Michelle Martin

*************************************************************************/

#ifndef __LEDGEMANAGER_H__
#define __LEDGEMANAGER_H__


struct SLedge
{
	SLedge()
		: m_fWidth(0.0f),
		m_entityId(-1)
	{
	}

	SLedge(EntityId entityId, float fWidth)
	{
		Set(entityId, fWidth);
	}

	void Set(EntityId entityId, float fWidth)
	{
		m_entityId = entityId;
		m_fWidth = fWidth;
	}

	void DebugRender(const ColorB &color);

	bool IsValid() const;

	int m_entityId;
	float m_fWidth;
};
typedef std::vector<SLedge> LedgeContainer;


//! Description:
//!		This class stores the ledges in a level that can be grabbed onto by a character.
//!		The ledge-entities register themselves here upon level loading (and unregister when destroyed again)
class CLedgeManager
{
public:

	// constructors
	// ================

	CLedgeManager();
	~CLedgeManager();


	// methods
	// ================

	void Reset();

	//! Description:
	//!		Adds a new ledge. If ledge already exists, the data will be updated.
	//!		If data is invalid, the ledge will be removed.
	void	AddLedge(EntityId entityId, float fWidth);

	void	RemoveLedgeByEntityId(EntityId entityId);
	void	RemoveLedgeByIndex(int iIdx);
	
	ILINE int	GetLedgeCount() const { return m_ledges.size(); };

	//! Description:
	//!		Returns the index of the nearest ledge that is in the direction and angle range.
	//!		Returns -1 if no ledge could be found
	int FindNearestLedge(const Vec3 &vPos, const Vec3 &vDir, float fMaxDistance = 2.0f, float fAngleRange = DEG2RAD(35.0f));

	//! Description:
	//!		Returns the ledge with the given index in the container
	//!		If the index is invalid it will return an empty (and invalid) ledge;
	//!		This does not take an entity ID!
	SLedge GetLedgeByIndex(int iIdx) const;

	//! Description:
	//!		Returns the index in the vector that this ledge has
	//!		returns -1 if the ledge could not be found
	int	GetLedgeIndex(EntityId entityId); 

	//!	Description:
	//!		The output vector (vOutput) will have the length of the distance 
	//!		from the point to the line.
	//!		Returns false only if a problem occured (no entity pointer, invalid data...)
	bool	FindVectorToClosestPointOnLedge(const Vec3 &vPoint, const SLedge &ledge, Vec3 *vOutput) const;

	// members
	// ================

private:

	// methods
	// ================

	//! Description:
	//!		Returns the iterator in the vector that this ledge has
	//!		returns end()-iterator if the ledge could not be found
	LedgeContainer::iterator GetLedgeIterator(EntityId entityId); 


	// members
	// ================

	LedgeContainer		m_ledges;

};

#endif // __LEDGEMANAGER_H__