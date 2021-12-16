//
//  SpanBuffer.h - Run length encoded voxel map. 
//
//  Copyright (C) 2007-2008 Mikko Mononen
//
//  This software is provided 'as-is', without any express or implied
//  warranty.  In no event will the authors be held liable for any damages
//  arising from the use of this software.
//
//  Permission is granted to anyone to use this software for any purpose,
//  including commercial applications, and to alter it and redistribute it
//  freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//     claim that you wrote the original software. If you use this software
//     in a product, an acknowledgment in the product documentation would be
//     appreciated but is not required.
//  2. Altered source versions must be plainly marked as such, and must not be
//     misrepresented as being the original software.
//  3. This notice may not be removed or altered from any source distribution.
//
//  Mikko Mononen memon@inside.org
//

#ifndef SPANBUFFER_H
#define SPANBUFFER_H

#include "NavMesh.h"
#include "PoolAllocator.h"
#include "Vec3.h"

namespace LayeredNavMesh {

inline bool OverlapInterval(int amin, int amax, int bmin, int bmax)
{
	if (amax < bmin) return false;
	if (amin > bmax) return false;
	return true;
}


class SpanBuffer
{
public:

	struct Span
	{
		// NOTE Jan 26, 2010: <pvl> 0x1 is slope, 0x2 means walkable surface
		// element on top of this span.  If we get more flags in the future make
		// an enum and clean stuff up.
		unsigned char flags;
		int smin, smax;
		Span* next;

		bool WalkableSurfaceOnTop () const { return 0 != (flags & 0x2); }
		void WalkableSurfaceOnTop (bool val) { val ? (flags |= 0x2) : (flags &= ~0x2) ; }
	};

	SpanBuffer(int width, int height);
	~SpanBuffer();

	void AddSpan(int x, int y, int smin, int smax, unsigned char flags);
	void Compact();
	int GetSpanCount() const;
	int GetMemSize();
	inline int GetWidth() const { return m_width; }
	inline int GetHeight() const { return m_height; }
	inline int GetSpanMin() const { return m_spanMin; }
	inline int GetSpanMax() const { return m_spanMax; }

	inline const Span* GetSpans(int x, int y) const
	{
		int idx = x + y*m_width;
		return m_spans[idx];
	}

	// NOTE Jan 29, 2010: <pvl> for loading from file
	// ATTN Feb 1, 2010: <pvl> this function returns a totally uninitialized Span
	// instance!  It's up to the caller to init it with values read from file.
	Span * AllocSpan ();
	// NOTE Feb 1, 2010: <pvl> it is expected that the last argument points to
	// the lowest span of a span column at (x,y), with spans properly linked and
	// terminated with zero 'next' link of the highest span in the chain.
	void AddSpanChain (int x, int y, Span * );
	// ATTN Feb 2, 2010: <pvl> NEVER use these outside of file loader!!!
	void SetSpanMin (int spanMin) { m_spanMin = spanMin; }
	void SetSpanMax (int spanMax) { m_spanMax = spanMax; }

#ifdef DEBUG_DRAW	
	void DebugDraw(const BuildSettings& settings);
#endif
	
private:
	
	int m_width, m_height;
	int m_spanMin, m_spanMax;
	PoolAllocator<Span, 1024> m_pool;
	Span** m_spans;
};


inline void SpanBuffer::AddSpan(int x, int y, int smin, int smax, unsigned char flags)
{
	if (smin == smax) return;
	int idx = x + y*m_width;

	// Keep track of the bounds.
	if (m_spanMin == m_spanMax)
	{
		m_spanMin = smin;
		m_spanMax = smax;
	}
	else
	{
		if (smin < m_spanMin)
			m_spanMin = smin;
		if (smax > m_spanMax)
			m_spanMax = smax;
	}

	Span* s = m_pool.Alloc();
	s->flags = flags;
	s->smin = smin;
	s->smax = smax;
	s->next = 0;
	
	// Empty cell, add he first span.
	if (!m_spans[idx])
	{
		m_spans[idx] = s;
		return;
	}

	// Merge spans.
	Span* prev = 0;
	Span* cur = m_spans[idx];

	// Find the insertion point
	while (cur && cur->smin <= s->smin)
	{
		prev = cur;
		cur = cur->next;
	}

	if (prev)
	{
		if (prev->smax >= s->smin)
		{
			// Merge with prev
			if (s->smax >= prev->smax)
			{
				if (s->smax == prev->smax)
					prev->flags |= s->flags;
				else
					prev->flags = s->flags;
				prev->smax = s->smax;
			}
			m_pool.Free(s);
			s = prev;
		}
		else
			prev->next = s;
	}
	else
	{
		m_spans[idx] = s;
	}

	// Merge spans
	while (cur && OverlapInterval(cur->smin, cur->smax, s->smin, s->smax))
	{
		Span* next = cur->next;
		if (cur->smin < s->smin)
			s->smin = cur->smin;
		if (cur->smax >= s->smax)
		{
			if (cur->smax == s->smax)
				s->flags |= cur->flags;
			else
				s->flags = cur->flags;
			s->smax = cur->smax;
		}
		m_pool.Free(cur);
		cur = next;
	}
	s->next = cur;
}

inline SpanBuffer::Span * SpanBuffer::AllocSpan ()
{
	return m_pool.Alloc();
}

inline void SpanBuffer::AddSpanChain (int x, int y, Span * span)
{
	int idx = x + y*m_width;
	m_spans[idx] = span;
}


} // namespace LayeredNavMesh

#endif
