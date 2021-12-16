/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Tracer Manager

-------------------------------------------------------------------------
History:
- 17:1:2006   11:12 : Created by Márcio Martins

*************************************************************************/
#ifndef __TRACERMANAGER_H__
#define __TRACERMANAGER_H__

#if _MSC_VER > 1000
# pragma once
#endif

enum
{
	kTracerFlag_scaleToDistance = (1 << 0),
	kTracerFlag_useGeometry			=	(1 << 1),
	kTracerFlag_active					= (1 << 2)
};


class CTracer
{
	friend class CTracerManager;
public:
	CTracer();
	CTracer(const Vec3 &pos);
	virtual ~CTracer();

	void Reset(const Vec3 &pos);
	void CreateEntity();
	void DeleteEntity();
	void SetGeometry(const char *name, float scale);
	void SetEffect(const char *name, float scale);
	void SetLifeTime(float lifeTime);
	bool Update(float frameTime, const Vec3 &campos);
	float GetAge() const;
	void GetMemoryStatistics(ICrySizer * s) const;

private:
	float				m_speed;
	Vec3				m_pos;
	Vec3				m_dest;
	Vec3				m_startingpos;
	float				m_age;
	float				m_lifeTime;
	float				m_fadeOutTime;
	float				m_startFadeOutTime;	
	float				m_scale;
	float				m_geometryOpacity;
	EntityId		m_entityId;
	uint16			m_tracerFlags;
	int8				m_effectSlot;
	int8        m_geometrySlot;
};


class CTracerManager
{
	const static int kMaxNumTracers = 96;
	
public:
	CTracerManager();
	virtual ~CTracerManager();

	typedef struct STracerParams
	{
		STracerParams()
		{
			geometry = NULL;
			effect = NULL;
			speed = 0.0f;
			lifetime = 0.0f;
			delayBeforeDestroy = 0.0f;
			scaleToDistance = false;
			position.Set(0.0f,0.0f,0.0f);
			destination.Set(0.0f,0.0f,0.0f);
			startFadeOutTime = 0.0f;
			scale = 1.0f;
			geometryOpacity = 0.99f;
		}
		const char *geometry;
		const char *effect;
		Vec3				position;
		Vec3				destination;
		float				speed;
		float				lifetime;
		float				delayBeforeDestroy;
		float				startFadeOutTime;
		float				scale;
		float				geometryOpacity;
		bool				scaleToDistance;
	};

	void EmitTracer(const STracerParams &params);
	void Update(float frameTime);
	void Reset();
	void GetMemoryStatistics(ICrySizer *);

private:
	CTracer					m_tracerPool[kMaxNumTracers];

	int16						m_numActiveTracers;
};


#endif //__TRACERMANAGER_H__