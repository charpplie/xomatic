/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: SuitDisruptionField

-------------------------------------------------------------------------
History:
- 26:05:2009   11:50 : Created by Tom Berry.

*************************************************************************/
#ifndef __SUITDISRUPTIONFIELD_H__
#define __SUITDISRUPTIONFIELD_H__

#if _MSC_VER > 1000
# pragma once
#endif


class CSuitDisruptionField
{
public:
	CSuitDisruptionField(EntityId id);
	~CSuitDisruptionField();

	void Update( float timePassed);
	inline void SetTeamID(int teamID);
	inline void SetPosition(const Vec3 &position);
	inline void SetRadii(float inner, float outer);
	void SetActive(bool active);

private:

	struct EffectedEntity
	{
		EntityId id;
	};
	typedef std::list<EffectedEntity> TEntityList;

	EntityId		m_entityId;
	int				m_teamID;
	bool			m_active;

	Vec3			m_pos;
	float			m_outerRadius;
	float			m_innerRadius;
	float			m_maxEffect;	//--- Scaled by a linear 1 to 0 gradient between inner & outer radii

	IParticleEffect *m_activeEffect;
	int				 m_effectSlot;

	TEntityList		m_effectedEntities;
};

inline void CSuitDisruptionField::SetTeamID(int teamID)
{
	m_teamID = teamID;
}

inline void CSuitDisruptionField::SetPosition(const Vec3 &position)
{
	m_pos = position;
}

inline void CSuitDisruptionField::SetRadii(float inner, float outer)
{
	CRY_ASSERT_MESSAGE(inner < outer, "Inner radius must be smaller than the outer");
	m_outerRadius = outer;
	m_innerRadius = inner;
}


#endif // __SUITDISRUPTIONFIELD_H__