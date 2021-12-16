#ifndef __DeferredActionQueue_h__
#define __DeferredActionQueue_h__


#pragma once


#include <StlUtils.h>

#include "AgePriorityQueue.h"
#include "STLPoolAllocator.h"
#include "STLPoolAllocator_ManyElems.h"


struct NoContention
{
protected:
	NoContention(uint32 quota) {}

	void PerformedSync() {}
	void PerformedAsync() {}

	bool CanPerformAsync()
	{
		return true;
	}

	void UpdateStart() {}
	void UpdateComplete() {}
};


struct DefaultContention
{
	void SetQuota(uint32 quota)
	{
		m_quota = quota;
	}

	uint32 GetQuota() const
	{
		return m_quota;
	}

	struct ContentionStats
	{
		uint32 quota;
		uint32 queueSize;
		uint32 maxQueueSize;

		uint32 immediateCount;
		uint32 deferredCount;

		float immediateAverage;
		float deferredAverage;
	};

	ContentionStats GetContentionStats()
	{
		ContentionStats stats;
		stats.quota = m_quota;
		stats.immediateCount = m_immWindow[(m_averageHead - 1) % AverageWindowWidth];
		stats.deferredCount = m_defWindow[(m_averageHead - 1) % AverageWindowWidth];

		uint32 immSum = 0;
		uint32 defSum = 0;

		uint32 count = std::min<uint32>(AverageWindowWidth, m_averageHead - 5);
		for (uint32 i = 0; i < count; ++i)
		{
			immSum += m_immWindow[i];
			defSum += m_defWindow[i];
		}

		stats.immediateAverage = immSum / (float)AverageWindowWidth;
		stats.deferredAverage = defSum / (float)AverageWindowWidth;
		
		stats.queueSize = m_queueSize;
		stats.maxQueueSize = m_maxQueueSize;

		return stats;
	}

protected:
	DefaultContention(uint32 quota = 64)
		: m_quota(quota)
		, m_immCount(0)
		, m_defCount(0)
		, m_queueSize(0)
		, m_maxQueueSize(0)
		, m_averageHead(0)
	{
		for (uint32 i = 0; i < AverageWindowWidth; ++i)
		{
			m_immWindow[i] = 0;
			m_defWindow[i] = 0;
		}
	}

	inline void PerformedImmediate()
	{
		++m_immCount;
	}

	inline void PerformedDeferred()
	{
		++m_defCount;
	}

	inline bool CanPerformDeferred()
	{
		return (m_immCount + m_defCount < m_quota);
	}

	void UpdateStart(uint32 queueSize)
	{
		m_queueSize = queueSize;
		if (m_maxQueueSize < queueSize)
			m_maxQueueSize = queueSize;
	}

	void UpdateComplete(uint32 queueSize)
	{
		m_immWindow[m_averageHead % AverageWindowWidth] = m_immCount;
		m_defWindow[m_averageHead % AverageWindowWidth] = m_defCount;

		++m_averageHead;

		m_immCount = 0;
		m_defCount = 0;
	}

private:
	uint32 m_quota;

	uint32 m_immCount;
	uint32 m_defCount;

	uint32 m_queueSize;
	uint32 m_maxQueueSize;

	enum
	{
		AverageWindowWidth = 10,
	};

	uint32 m_averageHead;
	uint32 m_immWindow[AverageWindowWidth];
	uint32 m_defWindow[AverageWindowWidth];
};

template<typename CasterType, typename RequestType, typename ResultType, typename ContentionPolicyType = DefaultContention>
class DeferredActionQueue :
	public CasterType,
	public ContentionPolicyType
{
	typedef DeferredActionQueue<CasterType, RequestType, ResultType, ContentionPolicyType> Type;
public:
	typedef CasterType Caster;
	typedef ContentionPolicyType ContentionPolicy;
	typedef typename RequestType::Priority PriorityType;
	typedef Functor2<const uint32&, const ResultType&> ResultCallback;

	struct PriorityClass
	{
		PriorityClass()
			: basePriority(1.0f)
			, growthFactor(1.0f)
			, growthTime(1.0f)
		{
		}

		PriorityClass(float _basePriority, float _growthFactor, float _growthTime)
			: basePriority(_basePriority)
			, growthFactor(_growthFactor)
			, growthTime(_growthTime)
		{
		}

		float basePriority;
		float growthFactor;
		float growthTime;
	};

	DeferredActionQueue()
		: m_slotGenID(0)
	{
		Caster::SetCallback(functor(*this, &Type::CastComplete));

		m_priorityClasses.resize(RequestType::HighestPriority + 1);
		m_priorityClasses[RequestType::LowPriority]		= PriorityClass(1.0f, 100.0f, 0.5f);
		m_priorityClasses[RequestType::MediumPriority] = PriorityClass(10.0f, 10.0f, 0.4f);
		m_priorityClasses[RequestType::HighPriority]		= PriorityClass(25.0f, 5.0f, 0.3f);
		m_priorityClasses[RequestType::HighestPriority]= PriorityClass(50.0f, 2.5f, 0.2f);
	}

	inline const ResultType& Cast(const RequestType& request)
	{
		ContentionPolicy::PerformedImmediate();

		return Caster::Cast(request);
	}

	inline uint32 Queue(const PriorityType& priority, const RequestType& request, 
		const ResultCallback& resultCallback)
	{
		return m_priorityQueue.push_back(QueuedRequest(priority, request, resultCallback));
	}

	inline void Cancel(const uint32& queuedID)
	{
		typename Submitted::iterator it = m_submitted.find(queuedID);
		if (it == m_submitted.end())
			m_priorityQueue.erase(queuedID);
		else
		{
			m_slots.erase(it->second);
			m_submitted.erase(it);
		}
	}

	inline void Update(float updateTime)
	{
		ContentionPolicy::UpdateStart(m_priorityQueue.size());

		if (!m_priorityQueue.empty() && ContentionPolicy::CanPerformDeferred())
		{
			PriorityClassUpdate doUpdate(m_priorityClasses);
			typename PriorityQueue::DefaultCompare doCompare;
			m_priorityQueue.update(updateTime, doUpdate, doCompare);

			while (!m_priorityQueue.empty() && ContentionPolicy::CanPerformDeferred())
			{
				QueuedRequest& queued = m_priorityQueue.front();
				Submit(m_priorityQueue.front());

				m_priorityQueue.pop_front();
			}
		}

		ContentionPolicy::UpdateComplete(m_priorityQueue.size());
	}

	inline void SetPriorityClass(const PriorityType& priority, const PriorityClass& priorityClass)
	{
		m_priorityClasses[priority] = priorityClass;
	}

protected:
	struct Slot
	{
		Slot()
			: queuedID(0)
			, callback(0)
		{
		}
		Slot(const uint32& _queuedID, const ResultCallback& _callback)
			: queuedID(_queuedID)
			, callback(_callback)
		{
		}

		uint32 queuedID;
		ResultCallback callback;
	};

	struct slot_hash_traits :
		public stl::hash_uint32
	{
		enum
		{
			bucket_size = 1,
			min_buckets = 256
		};
	};

	uint16 m_slotGenID;
	typedef stl::hash_map<uint16, Slot, slot_hash_traits,
		stl::STLPoolAllocator_ManyElems<std::pair<uint16, Slot> > > Slots;
	Slots m_slots;

	struct sent_hash_traits :
		public stl::hash_uint32
	{
		enum
		{
			bucket_size = 2,
			min_buckets = 128
		};
	};

	typedef stl::hash_map<uint32, uint16, sent_hash_traits,
		stl::STLPoolAllocator_ManyElems<std::pair<uint32, uint16> > > Submitted;
	Submitted m_submitted;

	struct QueuedRequest
	{
		QueuedRequest(const PriorityType& _priority, const RequestType& _request, const ResultCallback& _callback)
			: priority(_priority)
			, request(_request)
			, resultCallback(_callback)
		{
		}

		PriorityType priority;
		RequestType request;
		ResultCallback resultCallback;
	};

	typedef AgePriorityQueue<QueuedRequest> PriorityQueue;
	PriorityQueue m_priorityQueue;

	typedef std::vector<PriorityClass> PriorityClasses;
	PriorityClasses m_priorityClasses;

	struct PriorityClassUpdate
	{
		PriorityClassUpdate(const PriorityClasses& _priorityClasses)
			: priorityClasses(_priorityClasses)
		{
		}

		float operator()(const float& age, QueuedRequest& value)
		{
			const PriorityClass& priorityClass = priorityClasses[value.priority];
			return priorityClass.basePriority * cry_powf(priorityClass.growthFactor, age / priorityClass.growthTime);
		}

		const PriorityClasses& priorityClasses;
	};

	inline void Submit(const QueuedRequest& queued)
	{
		++m_slotGenID;
		while (!m_slotGenID)
			++m_slotGenID;

		m_slots.insert(typename Slots::value_type(m_slotGenID, Slot(m_priorityQueue.front_id(), queued.resultCallback)));
		m_submitted.insert(typename Submitted::value_type(m_priorityQueue.front_id(), m_slotGenID));

		SubmitQueuedCast(m_slotGenID, queued);

		ContentionPolicy::PerformedDeferred();
	}

	inline void SubmitQueuedCast(uint16 slotID, const QueuedRequest& queued)
	{
		Caster::Queue(slotID, queued.request);
	}

	void CastComplete(uint16 slotID, const ResultType& result)
	{
		typename Slots::iterator it = m_slots.find(slotID);
		if (it != m_slots.end())
		{
			const Slot& slot = m_slots[slotID];
			slot.callback(slot.queuedID, result);
			m_submitted.erase(slot.queuedID);
			m_slots.erase(it);
		}
	}
};


#endif