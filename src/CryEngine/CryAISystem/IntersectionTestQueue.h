#ifndef __IntersectionTestQueue_h__
#define __IntersectionTestQueue_h__

#pragma once


#include "DeferredActionQueue.h"
#include "STLPoolAllocator.h"


struct IntersectionTestResult
{
	operator bool() const
	{
		return distance <= 0.0f;
	}

	Vec3 point;
	Vec3 normal;

	float distance;
	int partId;
	int idxMat;
};


struct IntersectionTestRequest
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

	IntersectionTestRequest(int _primitiveType, primitives::primitive* _primitive, const Vec3& sweepDir, int _objTypes,
		int _flagsAll = 0, int _flagsAny = geom_colltype0|geom_colltype_player, void** _skipList = 0, 
		uint8 _skipListCount = 0)
		: primitiveType(_primitiveType)
		, objTypes(_objTypes)
		, flagsAll(_flagsAll)
		, flagsAny(_flagsAny)
		, skipListCount(_skipListCount)
	{
		assert(skipListCount <= MaxSkipListCount);

		if (skipListCount)
			memcpy(skipList, _skipList, skipListCount * sizeof(void*));
	}

	int primitiveType;
	primitives::primitive* primitive;

	Vec3 sweepDir;
	int objTypes;
	int flagsAll;
	int flagsAny;
	void** skipList;
	uint8 skipListCount;
};


template<int IntersectionTesterID>
struct DefaultIntersectionTester
{
protected:
	typedef DefaultIntersectionTester<IntersectionTesterID> Type;
	typedef Functor2<uint16, const IntersectionTestResult&> Callback;

	DefaultIntersectionTester()
		: callback(0)
	{
		gEnv->pPhysicalWorld->AddEventClient(EventPhysPWIResult::id, OnPWIResult, 1);
	}

	~DefaultIntersectionTester()
	{
		if (callback)
			gEnv->pPhysicalWorld->RemoveEventClient(EventPhysPWIResult::id, OnPWIResult, 1);
	}

	enum
	{
		MyID = IntersectionTesterID << 24, // 
		MyIDBitMask = ~((1 << 24) - 1),
		TestIDBitMask = ~MyIDBitMask,
	};

	inline const IntersectionTestResult& Cast(const IntersectionTestRequest& request)
	{
		geom_contact* contact = 0;
		m_resultBuf.distance = gEnv->pPhysicalWorld->PrimitiveWorldIntersection(request.primitiveType, request.primitive,
			request.sweepDir, request.objTypes, &contact, request.flagsAll, request.flagsAny, 0, 0, 0, 
			request.skipListCount ? (IPhysicalEntity**)request.skipList : 0, request.skipListCount);

		m_resultBuf.point = contact->pt;
		m_resultBuf.normal = contact->n;
		m_resultBuf.partId = contact->iPrim[1];
		m_resultBuf.idxMat = contact->id[1];

		return m_resultBuf;
	}

	inline void Queue(uint16 testID, const IntersectionTestRequest& request)
	{
		gEnv->pPhysicalWorld->PrimitiveWorldIntersection(request.primitiveType, request.primitive, request.sweepDir, 
			request.objTypes | rwi_queue,	0, request.flagsAll, request.flagsAny, 0, this, MyID | testID,
			request.skipListCount ? (IPhysicalEntity**)request.skipList : 0, request.skipListCount);
	}

	inline void SetCallback(const Callback& _callback)
	{
		callback = _callback;
	}

	static int OnPWIResult(const EventPhys *pEvent)
	{
		const EventPhysPWIResult* result = static_cast<const EventPhysPWIResult*>(pEvent);
		if ((result->iForeignData & MyIDBitMask) != MyID)
			return 0;

		Type* _this = static_cast<Type*>(result->pForeignData);
		uint16 testID = (result->iForeignData & TestIDBitMask);

		_this->m_resultBuf.point = result->pt;
		_this->m_resultBuf.normal = result->n;
		_this->m_resultBuf.partId = result->partId;
		_this->m_resultBuf.idxMat = result->idxMat;

		_this->callback(testID, _this->m_resultBuf);

		return 1;
	}

private:
	Callback callback;
	
	IntersectionTestResult m_resultBuf;
};


typedef uint32 QueuedIntersectionID;


template<int left, int right>
struct static_max
{
	enum
	{
		value = (left > right ? left : right)
	};
};


template<int IntersectionTesterID>
class IntersectionTestQueue :
	public DeferredActionQueue<DefaultIntersectionTester<IntersectionTesterID>,
		IntersectionTestRequest, IntersectionTestResult>
{
	typedef DeferredActionQueue<DefaultIntersectionTester<IntersectionTesterID>,
		IntersectionTestRequest, IntersectionTestResult> BaseType;
	typedef typename BaseType::QueuedRequest QueuedRequest;

public:
	typedef typename BaseType::PriorityType PriorityType;
	typedef typename BaseType::ResultCallback ResultCallback;

	inline uint32 Queue(const PriorityType& priority, const IntersectionTestRequest& request, 
		const ResultCallback& resultCallback)
	{
		QueuedRequest& back = this->m_priorityQueue.push_back(QueuedRequest(priority, request, resultCallback));
		back.request.primitive = AllocPrimitive(request.primitiveType, request.primitive);
	}

	inline void Cancel(const uint32& queuedID)
	{
		typename BaseType::Sent::iterator it = this->m_sent.find(queuedID);
		if (it == this->m_sent.end())
		{
			DeallocPrimitive(this->m_priorityQueue[queuedID].request.primitive);
			this->m_priorityQueue.erase(queuedID);
		}
		else
		{
			this->m_slots.erase(it->second);
			this->m_sent.erase(it);
		}
	}

protected:
	inline void SubmitQueuedCast(uint16 slotID, const QueuedRequest& queued)
	{
		BaseType::SubmitQueuedCast(slotID, queued);

		DeallocPrimitive(queued.request.primitiveType, queued.request.primitive);
	}

	inline primitives::primitive* AllocPrimitive(int primitiveType, const primitives::primitive* primitive)
	{
		switch (primitiveType)
		{
		case primitives::box::type:
			return new (m_primitiveAlloc.Allocate()) primitives::box(*(primitives::box*)primitive);
		case primitives::cylinder::type:
			return new (m_primitiveAlloc.Allocate()) primitives::cylinder(*(primitives::cylinder*)primitive);
		case primitives::capsule::type:
			return new (m_primitiveAlloc.Allocate()) primitives::capsule(*(primitives::capsule*)primitive);
		case primitives::sphere::type:
			return new (m_primitiveAlloc.Allocate()) primitives::sphere(*(primitives::sphere*)primitive);
		default: return 0;
		}
	}

	inline void DeallocPrimitive(int primitiveType, void* primitive)
	{
		switch (primitiveType)
		{
		case primitives::box::type:
			return (*(primitives::box*)primitive).~box();
		case primitives::cylinder::type:
			return (*(primitives::cylinder*)primitive).~cylinder();
		case primitives::capsule::type:
			return (*(primitives::capsule*)primitive).~capsule();
		case primitives::sphere::type:
			return (*(primitives::sphere*)primitive).~sphere();
		default: return 0;
		}
		m_primitiveAlloc.Deallocate(primitive);
	}

	enum
	{
		MaxPrimitiveSize =
		static_max<sizeof(primitives::box),
			static_max<sizeof(primitives::cylinder),
				static_max<sizeof(primitives::capsule), sizeof(primitives::sphere)>::value
			>::value
		>::value,
	};

	typedef stl::PoolAllocator<MaxPrimitiveSize, stl::PoolAllocatorSynchronizationSinglethreaded> PrimitiveAllocator;
	PrimitiveAllocator m_primitiveAlloc;
};


#endif