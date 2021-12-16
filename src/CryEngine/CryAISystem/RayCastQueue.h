#ifndef __RayCastQueue_h__
#define __RayCastQueue_h__

#pragma once


#include "DeferredActionQueue.h"


struct RayCastResult
{
	enum
	{
		MaxHitCount = 1, // aggressively set to 1
	};

	operator bool() const
	{
		return hitCount != 0;
	}

	const ray_hit& operator[](uint32 index) const
	{
		assert(index < hitCount);
		return hits[index];
	}

	ray_hit& operator[](uint32 index)
	{
		assert(index < hitCount);
		return hits[index];
	}

	ray_hit hits[MaxHitCount];
	int hitCount;
};


struct RayCastRequest
{
	enum
	{
		MaxSkipListCount = 8,
	};

	enum Priority
	{
		LowPriority = 0,
		MediumPriority,
		HighPriority,
		HighestPriority,
	};

	RayCastRequest(const Vec3& _pos, const Vec3& _dir, int _objTypes, int _flags = 0, IPhysicalEntity** _skipList = 0,
		uint8 _skipListCount = 0, uint8 _maxHitCount = 1)
		: pos(_pos)
		, dir(_dir)
		, objTypes(_objTypes)
		, flags(_flags)
		, skipListCount(_skipListCount)
		, maxHitCount(_maxHitCount)
	{
		assert(maxHitCount <= RayCastResult::MaxHitCount);
		assert(skipListCount <= MaxSkipListCount);
		
		if (skipListCount)
			memcpy(skipList, _skipList, skipListCount * sizeof(void*));
	}

	Vec3 pos;
	Vec3 dir;

	int objTypes;
	int flags;

	uint8 skipListCount;
	void* skipList[MaxSkipListCount];
	uint8 maxHitCount;
};


template<int RayCasterID>
struct DefaultRayCaster
{
protected:
	typedef DefaultRayCaster<RayCasterID> Type;
	typedef Functor2<uint16, const RayCastResult&> Callback;

	DefaultRayCaster()
		: callback(0)
	{
		gEnv->pPhysicalWorld->AddEventClient(EventPhysRWIResult::id, OnRWIResult, 1);
	}

	~DefaultRayCaster()
	{
		if (callback)
			gEnv->pPhysicalWorld->RemoveEventClient(EventPhysRWIResult::id, OnRWIResult, 1);
	}

	enum
	{
		MyID = RayCasterID << 24, // 
		MyIDBitMask = ~((1 << 24) - 1),
		RayIDBitMask = ~MyIDBitMask,
	};

	inline const RayCastResult& Cast(const RayCastRequest& request)
	{
		assert(request.maxHitCount <= RayCastResult::MaxHitCount);
		m_resultBuf.hitCount = (uint32)gEnv->pPhysicalWorld->RayWorldIntersection(request.pos, request.dir, request.objTypes, request.flags,
			m_resultBuf.hits, request.maxHitCount, request.skipListCount ? (IPhysicalEntity**)request.skipList : 0, request.skipListCount);

		return m_resultBuf;
	}

	inline void Queue(uint16 rayID, const RayCastRequest& request)
	{
		gEnv->pPhysicalWorld->RayWorldIntersection(request.pos, request.dir, request.objTypes, request.flags | rwi_queue,
			0, request.maxHitCount, request.skipListCount ? (IPhysicalEntity**)request.skipList : 0, request.skipListCount, this, MyID | rayID);
	}

	inline void SetCallback(const Callback& _callback)
	{
		callback = _callback;
	}

	static int OnRWIResult(const EventPhys *pEvent)
	{
/* //Diesel cut
		const EventPhysRWIResult* result = static_cast<const EventPhysRWIResult*>(pEvent);
		if ((result->iForeignData & MyIDBitMask) != MyID)
			return 0;

		Type* _this = static_cast<Type*>(result->pForeignData);
		uint16 rayID = (result->iForeignData & RayIDBitMask);

		assert(result->nHits <= RayCastResult::MaxHitCount);
		
		_this->m_resultBuf.hitCount = (uint32)result->nHits;

		for (uint32 i = 0; i < _this->m_resultBuf.hitCount; ++i)
			_this->m_resultBuf.hits[i] = result->pHits[i];

		_this->callback(rayID, _this->m_resultBuf);
*/ //Diesel cut
		return 1;
	}

private:
	Callback callback;
	RayCastResult m_resultBuf;
};


typedef uint32 QueuedRayID;


template<int RayCasterID>
class RayCastQueue :
	public DeferredActionQueue<DefaultRayCaster<RayCasterID>, RayCastRequest, RayCastResult>
{
};


#endif