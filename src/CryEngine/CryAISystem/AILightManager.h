/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   AILightManager.h
$Id$
Description: Keeps track of the logical light level in the game world and
						 provides services to make light levels queries.

-------------------------------------------------------------------------
History:
- 2007          : Created by Mikko Mononen
- 2 Mar 2009	: Evgeny Adamenkov: Removed IRenderer

*********************************************************************/

#ifndef _AILIGHTMANAGER_H_
#define _AILIGHTMANAGER_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include "IAISystem.h"
#include "CAISystem.h"
#include "StlUtils.h"

class CAILightManager
{
public:
	CAILightManager();
	~CAILightManager();

	void Reset();
	void Update(bool forceUpdate);
	void DebugDraw();
	void OnObjectRemoved(IAIObject* pObject);
	void Serialize( TSerialize ser, CObjectTracker& objectTracker );

	void DynOmniLightEvent(const Vec3& pos, float radius, EAILightEventType type, CAIActor* pShooter, float time);
	void DynSpotLightEvent(const Vec3& pos, const Vec3& dir, float radius, float fov, EAILightEventType type, CAIActor* pShooter, float time);

	//Returns light level at slecified location.
	EAILightLevel	GetLightLevelAt(const Vec3& pos, const CAIActor* pAgent = 0, bool* outUsingCombatLight = 0);

private:

	void DebugDrawArea(const ListPositions& poly, float zmin, float zmax, ColorB color);
	void UpdateTimeOfDay();
	void UpdateLights();

	struct SAIDynLightSource
	{
		SAIDynLightSource(const Vec3& pos, const Vec3& dir, float radius, float fov, EAILightLevel level, EAILightEventType type, CWeakRef<CAIActor> _refShooter, CCountedRef<CAIObject> _refAttrib, float t) :
			pos(pos), dir(dir), radius(radius), fov(fov), level(level), type(type), refShooter(_refShooter), refAttrib(_refAttrib), t(0), tmax(t) {}
		SAIDynLightSource() :
			pos(0,0,0), dir(0,1,0), radius(0), fov(0), level(AILL_NONE), type(AILE_GENERIC), t(0), tmax(t) {}
		SAIDynLightSource(const SAIDynLightSource &that) :
			pos(that.pos), dir(that.dir), radius(that.radius), fov(that.fov), level(that.level), type(that.type), t(that.t), tmax(that.tmax)
			,refShooter(that.refShooter), refAttrib(that.refAttrib) {}

		void Serialize( TSerialize ser, CObjectTracker& objectTracker );
		Vec3 pos;
		Vec3 dir;
		float radius;
		float fov;
		EAILightLevel level;
		CWeakRef<CAIActor> refShooter;
		CCountedRef<CAIObject> refAttrib;
		EAILightEventType type;
		float t;
		float tmax;
	};
	std::vector<SAIDynLightSource> m_dynLights;

	struct SAILightSource
	{
		SAILightSource(const Vec3& pos, float radius, EAILightLevel level) :
			pos(pos), radius(radius), level(level)
		{
			// Empty
		}

		Vec3 pos;
		float radius;
		EAILightLevel level;
	};

	std::vector<SAILightSource> m_lights;
	bool m_updated;
	EAILightLevel	m_ambientLight;

	CTimeValue	m_lastUpdateTime;
};


#endif