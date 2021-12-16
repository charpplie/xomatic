/*************************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a hashed space partitioning structure.

-------------------------------------------------------------------------
History:
- 5:3:2009   15:28 : Created by Márcio Martins

*************************************************************************/
#ifndef __HashedSpace_H__
#define __HashedSpace_H__

#pragma once

#include "PoolAllocator.h"

template<typename Obj>
struct DefaultPositionRetriever
{
	const Vec3& operator()(const Obj* pObj) const
	{
		return pObj->GetPos();
	}
};


template<typename Obj, int BucketCount=64, typename PositionRetriever=DefaultPositionRetriever<Obj> >
class HashedSpace
{
public:
	typedef Obj								object_type;
	typedef PositionRetriever position_retriever;

	class radial_iterator
	{
	public:
		typedef Obj								object_type;
		typedef PositionRetriever position_retriever;

		radial_iterator(const Vec3& center, float radius, const HashedSpace<Obj, BucketCount, PositionRetriever>& hashedSpace);
		operator object_type*()
		{
			return const_cast<object_type*>(m_pObject);
		}
		operator const object_type*() const
		{
			return m_pObject;
		}

		object_type *operator->()
		{
			return const_cast<object_type*>(m_pObject);
		}

		const object_type *operator->() const
		{
			return m_pObject;
		}

		radial_iterator operator++	(int);
		void operator++();

		float	GetDistSq() const;

		int	m_startY;
		int	m_startZ;

		int	m_endX;
		int	m_endY;
		int	m_endZ;

		int m_curX;
		int	m_curY;
		int	m_curZ;

		uint32 m_curIdx;
		float	m_radiusSq;

		Vec3 m_center;

		const object_type	*m_pObject;
		float m_curDistSq;

		const HashedSpace<Obj, BucketCount, PositionRetriever>	&m_hashedSpace;
	};

	class seq_iterator
	{
	public:
		typedef Obj								object_type;
		typedef PositionRetriever position_retriever;

		seq_iterator(const HashedSpace<Obj, BucketCount, PositionRetriever>& hashedSpace);
		operator object_type*()
		{
			return const_cast<object_type*>(m_pObject);
		}
		operator const object_type*() const
		{
			return m_pObject;
		}

		seq_iterator operator++	(int);
		void operator++();

		int	m_bucketIdx;
		int	m_objectIdx;

		const object_type	*m_pObject;

		const HashedSpace<Obj, BucketCount, PositionRetriever>	&m_hashedSpace;
	};


	static const int bucketCount = BucketCount;

	HashedSpace(const Vec3& bucketSize);
	~HashedSpace();

	radial_iterator BeginRadial(const Vec3& center, float radius) const;
	seq_iterator BeginSeq() const;

	void Clear();
	void RegisterObject(object_type* pObject);
	void UnregisterObject(object_type* pObject);
	void ObjectMoved(object_type* pObject, const Vec3& oldpos);

	struct Stats
	{
		uint32 objectCount;
		uint32 bucketCount;
		uint32 emptyBuckets;
		uint32 maxOccupancy;
		uint32 minOccupancy;
		uint32 moveCount;
		float averageOccupancy;
	};
	void GetStats(Stats *stats) const;

protected:
	void GetXYZ(const Vec3& pos, int *x, int *y, int *z) const;
	uint32 GetBucketIdx(int x, int y, int z) const;
	uint32 GetMask(int x, int y, int z) const;
	uint32 GetBucketIdx(const Vec3& pos) const;


	typedef std::vector<object_type*> Objects;
	typedef std::vector<uint32>				Masks;

	typedef struct Bucket
	{
		Objects objects;
		Masks		masks;		// to resolve bucket collision
	}Bucket;

	typedef std::vector<Bucket> Buckets;

	Vec3 m_bucketSize;
	Vec3 m_bucketScale;

	Buckets m_buckets;
	uint32	m_objectCount;
	uint32	m_moveCount;
};


template<typename Obj, int BucketCount, typename PositionRetriever>
inline HashedSpace<Obj, BucketCount, PositionRetriever>::HashedSpace(const Vec3& bucketSize)
: m_bucketSize(bucketSize)
, m_bucketScale(1.0f/bucketSize.x, 1.0f/bucketSize.y, 1.0f/bucketSize.z)
, m_objectCount(0)
, m_moveCount(0)
{
	m_buckets.resize(bucketCount);
}

template<typename Obj, int BucketCount, typename PositionRetriever>
inline HashedSpace<Obj, BucketCount, PositionRetriever>::~HashedSpace()
{
}

template<typename Obj, int BucketCount, typename PositionRetriever>
inline typename HashedSpace<Obj, BucketCount, PositionRetriever>::radial_iterator HashedSpace<Obj, BucketCount, PositionRetriever>::BeginRadial(const Vec3& center, float radius) const
{
	return radial_iterator(center, radius, *this);
}


template<typename Obj, int BucketCount, typename PositionRetriever>
inline typename HashedSpace<Obj, BucketCount, PositionRetriever>::seq_iterator HashedSpace<Obj, BucketCount, PositionRetriever>::BeginSeq() const
{
	return seq_iterator(*this);
}


template<typename Obj, int BucketCount, typename PositionRetriever>
inline void HashedSpace<Obj, BucketCount, PositionRetriever>::Clear()
{
	for (uint32 i = 0; i < m_buckets.size(); ++i)
	{
		m_buckets[i].objects.clear();
		m_buckets[i].masks.clear();
	}
	m_objectCount = 0;
}

template<typename Obj, int BucketCount, typename PositionRetriever>
inline void HashedSpace<Obj, BucketCount, PositionRetriever>::RegisterObject(object_type* pObject)
{
	int x;
	int y;
	int z;

	const Vec3& pos = position_retriever()(pObject);

	GetXYZ(pos, &x, &y, &z);

	uint32 idx = GetBucketIdx(x, y, z);
	uint32 mask = GetMask(x, y, z);

	m_buckets[idx].objects.push_back(pObject);
	m_buckets[idx].masks.push_back(mask);

	++m_objectCount;
}

template<typename Obj, int BucketCount, typename PositionRetriever>
inline void HashedSpace<Obj, BucketCount, PositionRetriever>::UnregisterObject(object_type* pObject)
{
	int x;
	int y;
	int z;

	const Vec3& pos = position_retriever()(pObject);

	GetXYZ(pos, &x, &y, &z);

	uint32 idx = GetBucketIdx(x, y, z);
	Bucket& bucket = m_buckets[idx];
	size_t objCount = bucket.objects.size();

	for (uint32  i = 0; i < objCount; ++i)
	{
		if (bucket.objects[i] == pObject)
		{
			bucket.objects[i] = bucket.objects.back();
			bucket.masks[i] = bucket.masks.back();
			
			bucket.objects.pop_back();
			bucket.masks.pop_back();

			--m_objectCount;
			
			return;
		}
	}
}


template<typename Obj, int BucketCount, typename PositionRetriever>
inline void HashedSpace<Obj, BucketCount, PositionRetriever>::ObjectMoved(object_type* pObject, const Vec3& oldpos)
{
	uint32 oldBucketIdx = GetBucketIdx(oldpos);

	int x;
	int y;
	int z;

	GetXYZ(position_retriever()(pObject), &x, &y, &z);
	uint32 bucketIdx = GetBucketIdx(x, y, z);
	
	if (bucketIdx == oldBucketIdx)
	{
		Bucket& bucket = m_buckets[bucketIdx];
		size_t objCount = bucket.objects.size();

		for (uint32  i = 0; i < objCount; ++i)
		{
			if (bucket.objects[i] == pObject)
			{
				bucket.masks[i] = GetMask(x, y, z);

				return;
			}
		}
		assert(false);
	}
	else
	{
		Bucket& bucket = m_buckets[oldBucketIdx];
		size_t objCount = bucket.objects.size();

		bool removed = false;

		for (uint32  i = 0; i < objCount; ++i)
		{
			if (bucket.objects[i] == pObject)
			{
				bucket.objects[i] = bucket.objects.back();
				bucket.masks[i] = bucket.masks.back();

				bucket.objects.pop_back();
				bucket.masks.pop_back();

				removed = true;

				break;
			}
		}

		assert(removed);(void)removed;

		Bucket& newBucket = m_buckets[bucketIdx];
		newBucket.objects.push_back(pObject);
		newBucket.masks.push_back(GetMask(x, y, z));

		++m_moveCount;
	}
}

template<typename Obj, int BucketCount, typename PositionRetriever>
inline void HashedSpace<Obj, BucketCount, PositionRetriever>::GetXYZ(const Vec3& pos, int *x, int *y, int *z) const
{
	*x = (int)cry_floorf(pos.x * m_bucketScale.x);
	*y = (int)cry_floorf(pos.y * m_bucketScale.y);
	*z = (int)cry_floorf(pos.z * m_bucketScale.z);
}


template<typename Obj, int BucketCount, typename PositionRetriever>
inline uint32 HashedSpace<Obj, BucketCount, PositionRetriever>::GetBucketIdx(int x, int y, int z) const
{
	const int h1 = 0x8da6b343;
	const int h2 = 0xd8163841;
	const int h3 = 0xcb1ab31f;

	int n = h1 * x + h2 * y + h3 * z;
	return (uint32)(n & (BucketCount-1));
}

template<typename Obj, int BucketCount, typename PositionRetriever>
inline uint32 HashedSpace<Obj, BucketCount, PositionRetriever>::GetMask(int x, int y, int z) const
{
	const uint32 mx = (1 << 12)-1;	// 12 bits for x
	const uint32 my = (1 << 12)-1;	// 12 bits for y
	const uint32 mz = (1 << 8)-1;		// 8bits for z

	uint32 ux = (uint32 )(x & mx);
	uint32 uy = (uint32 )(y & my);
	uint32 uz = (uint32 )(z & mz);

	return ux | (uy << 12) | (uz << 24);
}


template<typename Obj, int BucketCount, typename PositionRetriever>
inline uint32 HashedSpace<Obj, BucketCount, PositionRetriever>::GetBucketIdx(const Vec3& pos) const
{
	int x;
	int y;
	int z;

	GetXYZ(pos, &x, &y, &z);

	return GetBucketIdx(x, y, z);
}


template<typename Obj, int BucketCount, typename PositionRetriever>
inline void HashedSpace<Obj, BucketCount, PositionRetriever>::GetStats(Stats* stats) const
{
	stats->objectCount = m_objectCount;
	stats->moveCount = m_moveCount;
	stats->bucketCount = m_buckets.size();

	stats->emptyBuckets = 0;
	stats->maxOccupancy = 0;
	stats->minOccupancy = m_objectCount;

	uint32 totalOccupancy = 0;

	uint32 bucketCount = m_buckets.size();

	for (uint32 i = 0; i < bucketCount; ++i)
	{
		if (!m_buckets[i].objects.empty())
		{
			uint32 objectCount = m_buckets[i].objects.size();
			
			totalOccupancy += objectCount;

			if (objectCount > stats->maxOccupancy)
				stats->maxOccupancy = objectCount;

			if (objectCount < stats->minOccupancy)
				stats->minOccupancy = objectCount;
		}
		else
			++stats->emptyBuckets;
	}

	if (stats->bucketCount == stats->emptyBuckets)
		stats->averageOccupancy = 0.0f;
	else
		stats->averageOccupancy = totalOccupancy/float(stats->bucketCount-stats->emptyBuckets);
}



template<typename Obj, int BucketCount, typename PositionRetriever>
inline HashedSpace<Obj, BucketCount, PositionRetriever>::radial_iterator::radial_iterator(const Vec3& center, float radius, const HashedSpace<Obj, BucketCount, PositionRetriever>& hashedSpace)
: m_curIdx(0)
, m_radiusSq(radius*radius)
, m_center(center)
, m_pObject(0)
, m_curDistSq(0.0f)
, m_hashedSpace(hashedSpace)
{
	int x;
	hashedSpace.GetXYZ(center - Vec3(radius, radius, radius), &x, &m_startY, &m_startZ);
	hashedSpace.GetXYZ(center + Vec3(radius, radius, radius), &m_endX, &m_endY, &m_endZ);

	m_curX = x;
	m_curY = m_startY;
	m_curZ = m_startZ;

	operator++();
}

template<typename Obj, int BucketCount, typename PositionRetriever>
inline float HashedSpace<Obj, BucketCount, PositionRetriever>::radial_iterator::GetDistSq() const
{
	return m_curDistSq;
}

template<typename Obj, int BucketCount, typename PositionRetriever>
inline typename HashedSpace<Obj, BucketCount, PositionRetriever>::radial_iterator HashedSpace<Obj, BucketCount, PositionRetriever>::radial_iterator::operator ++(int)
{
	radial_iterator tmp = *this;

	operator++();

	return tmp;
}

template<typename Obj, int BucketCount, typename PositionRetriever>
inline void HashedSpace<Obj, BucketCount, PositionRetriever>::radial_iterator::operator ++()
{
	m_pObject = 0;

	for ( ; m_curX <= m_endX; ++m_curX)
	{
		for ( ; m_curY <= m_endY; ++m_curY)
		{
			for ( ; m_curZ <= m_endZ; ++m_curZ)
			{
				uint32 idx = m_hashedSpace.GetBucketIdx(m_curX, m_curY, m_curZ);
				uint32 mask = m_hashedSpace.GetMask(m_curX, m_curY, m_curZ);

				const typename HashedSpace<Obj, BucketCount, PositionRetriever>::Objects &objects = m_hashedSpace.m_buckets[idx].objects;
				const typename HashedSpace<Obj, BucketCount, PositionRetriever>::Masks &masks = m_hashedSpace.m_buckets[idx].masks;
				
				uint32 objectCount = objects.size();
				for (; m_curIdx < objectCount ; ++m_curIdx)
				{
					if (mask != masks[m_curIdx])
						continue;

					const object_type* pObject = objects[m_curIdx];
					m_curDistSq = Distance::Point_PointSq(m_center, position_retriever()(pObject));
					if (m_curDistSq <= m_radiusSq)
					{
						m_pObject = pObject;
						++m_curIdx;

						return;
					}
				}
				m_curIdx = 0;
			}
			m_curZ = m_startZ;
		}
		m_curY = m_startY;
	}
	m_curDistSq = -1.0f;
}




template<typename Obj, int BucketCount, typename PositionRetriever>
inline HashedSpace<Obj, BucketCount, PositionRetriever>::seq_iterator::seq_iterator(const HashedSpace<Obj, BucketCount, PositionRetriever>& hashedSpace)
: m_bucketIdx(0)
, m_objectIdx(0)
{
	operator++();
}

template<typename Obj, int BucketCount, typename PositionRetriever>
inline typename HashedSpace<Obj, BucketCount, PositionRetriever>::seq_iterator HashedSpace<Obj, BucketCount, PositionRetriever>::seq_iterator::operator ++(int)
{
	radial_iterator tmp = *this;

	operator++();

	return tmp;
}

template<typename Obj, int BucketCount, typename PositionRetriever>
inline void HashedSpace<Obj, BucketCount, PositionRetriever>::seq_iterator::operator ++()
{
	m_pObject = 0;

	size_t bucketCount = m_hashedSpace.m_buckets.size();
	for ( ; m_bucketIdx < bucketCount; )
	{
		typename HashedSpace<Obj, BucketCount, PositionRetriever>::Objects& objects = m_buckets[m_bucketIdx];
		size_t objectCount = objects.size();

		for ( ; m_objectIdx < objectCount; )
			return objects[m_objectIdx++];

		m_objectIdx = 0;
	}
}


#endif //__HashedSpace_H__
