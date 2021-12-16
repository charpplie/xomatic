// Refactored version of GLU tesselator from OGL sample by Mikko Mononen 2008.

/*
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.1 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
** 
** http://oss.sgi.com/projects/FreeB
** 
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
** 
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
** 
** Additional Notice Provisions: The application programming interfaces
** established by SGI in conjunction with the Original Code are The
** OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
** April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
** 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
** Window System(R) (Version 1.3), released October 19, 1998. This software
** was created using the OpenGL(R) version 1.2.1 Sample Implementation
** published by SGI, but has not been independently verified as being
** compliant with the OpenGL(R) version 1.2.1 Specification.
**
** $Date$ $Revision$
** $Header: //depot/main/glx/lib/pixel.c#12 $
*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "Tess.h"
#include "TessMesh.h"
#include "TessGeom.h"

struct Tess::DictNode
{
	ActiveRegion* key;
	DictNode* next;
	DictNode* prev;
};

class Tess::Dict
{
public:

	inline Dict() :
		m_event(0)
	{
		m_head.key = 0;
		m_head.next = &m_head;
		m_head.prev = &m_head;
	}
	
	inline ~Dict()
	{
//		for (DictNode* node = m_head.next; node != &m_head; node = node->next)
//			delete node;
	}

	inline Tess::DictNode* InsertBefore(Tess::DictNode* node, Tess::ActiveRegion* key)
	{
		do
		{
			node = node->prev;
		}
		while (node->key != 0 && !EdgeLeq(m_event, node->key, key));

		DictNode* newNode = m_nodePool.Alloc(); //new DictNode;
		if (!newNode)
			return 0;

		newNode->key = key;
		newNode->next = node->next;
		node->next->prev = newNode;
		newNode->prev = node;
		node->next = newNode;

		return newNode;
	}

	inline Tess::DictNode* Insert(Tess::ActiveRegion* key)
	{
		return InsertBefore(&m_head, key);
	}

	inline void Delete(Tess::DictNode* node)
	{
		node->next->prev = node->prev;
		node->prev->next = node->next;
		m_nodePool.Free(node);
//		delete node;
	}

	inline DictNode* Search(Tess::ActiveRegion* key)
	{
		DictNode* node = &m_head;
		do
		{
			node = node->next;
		}
		while (node->key != 0 && !EdgeLeq(m_event, key, node->key));
		return node;
	}

	inline DictNode* First()
	{
		return m_head.next;
	}

	inline DictNode* Last()
	{
		return m_head.prev;
	}
	
	TessVertex* m_event;

private:

	// Both edges must be directed from right to left (this is the canonical
	// direction for the upper edge of each region).
	//
	// The strategy is to evaluate a "t" value for each edge at the
	// current sweep line position, given by tess->event.  The calculations
	// are designed to be very stable, but of course they are not perfect.
	//
	// Special case: if both edge destinations are at the sweep event,
	// we sort the edges by slope (they would otherwise compare equally).

	static bool EdgeLeq(TessVertex* event, Tess::ActiveRegion* reg1, Tess::ActiveRegion* reg2)
	{
		TessHalfEdge* e1 = reg1->eUp;
		TessHalfEdge* e2 = reg2->eUp;
		
		if (e1->Dst() == event)
		{
			if (e2->Dst() == event)
			{
				// Two edges right of the sweep line which meet at the sweep event.
				// Sort them by slope.
				if (VertLeq(e1->Org(), e2->Org()))
					return EdgeSign(e2->Dst(), e1->Org(), e2->Org()) <= 0;
				return EdgeSign(e1->Dst(), e2->Org(), e1->Org()) >= 0;
			}
			return EdgeSign(e2->Dst(), event, e2->Org()) <= 0;
		}
		if (e2->Dst() == event)
			return EdgeSign(e1->Dst(), event, e1->Org()) >= 0;
		
		// General case - compute signed distance *from* e1, e2 to event
		float t1 = EdgeEval(e1->Dst(), event, e1->Org());
		float t2 = EdgeEval(e2->Dst(), event, e2->Org());
		return t1 >= t2;
	}

	LayeredNavMesh::PoolAllocator<DictNode> m_nodePool;
	DictNode m_head;
};



class Tess::EventQueue
{
public:
	
	typedef int Handle;
	static const int INVALID_HANDLE = -1;
	static const int INIT_SIZE = 32;

	EventQueue(int s = INIT_SIZE) :
		m_size(0),
		m_max(s),
		m_initialized(false),
		m_freeList(0)
	{
		m_nodes = new Node[s+1];
		m_handles = new HandleElem[s+1];
		m_nodes[1].handle = 1;	// so that Minimum() returns NULL
		m_handles[1].key = 0;
	}
	
	~EventQueue()
	{
		delete [] m_handles;
		delete [] m_nodes;
	}
	
	inline bool Init()
	{
		// This method of building a heap is O(n), rather than O(n lg n).
		for (int i = m_size; i >= 1; --i)
			FloatDown(i);
		m_initialized = true;
		return true;
	}
	
	inline Handle Insert(TessVertex* keyNew)
	{
		if (!keyNew)
			return INVALID_HANDLE;
		Handle free = INVALID_HANDLE;
		int curr = ++m_size;
		if (curr*2 > m_max)
		{
			// If the heap overflows, double its size.
			int saveSize = m_max+1;
			m_max *= 2;

			Node* saveNodes = m_nodes;
			m_nodes = new Node[m_max+1];
			memcpy(m_nodes, saveNodes, saveSize * sizeof(Node));
			delete [] saveNodes; 

			HandleElem* saveHandles = m_handles;
			m_handles = new HandleElem[m_max+1];
			memcpy(m_handles, saveHandles, saveSize * sizeof(HandleElem));
			delete [] saveHandles;
		}
		
		if (m_freeList == 0)
		{
			free = curr;
		}
		else
		{
			free = m_freeList;
			m_freeList = m_handles[free].node;
		}
		
		m_nodes[curr].handle = free;
		m_handles[free].node = curr;
		m_handles[free].key = keyNew;
		
		if (m_initialized)
			FloatUp(curr);

		assert(free != INVALID_HANDLE);
		return free;
	}
	
	inline TessVertex* ExtractMin()
	{
		Handle hMin = m_nodes[1].handle;
		TessVertex* min = m_handles[hMin].key;
		
		if (m_size > 0)
		{
			m_nodes[1].handle = m_nodes[m_size].handle;
			m_handles[m_nodes[1].handle].node = 1;
			
			m_handles[hMin].key = 0;
			m_handles[hMin].node = m_freeList;
			m_freeList = hMin;
			
			if (--m_size > 0)
				FloatDown(1);
		}
		return min;
	}
	
	void Delete(Handle hCurr)
	{
		assert(hCurr >= 1 && hCurr <= m_max && m_handles[hCurr].key != 0);
		int curr = m_handles[hCurr].node;
		m_nodes[curr].handle = m_nodes[m_size].handle;
		m_handles[m_nodes[curr].handle].node = curr;
		--m_size;
		
		if (curr <= m_size)
		{
			if (curr <= 1 || VertLeq(m_handles[m_nodes[curr>>1].handle].key, m_handles[m_nodes[curr].handle].key))
				FloatDown(curr);
			else
				FloatUp(curr);
		}
		m_handles[hCurr].key = NULL;
		m_handles[hCurr].node = m_freeList;
		m_freeList = hCurr;
	}

	inline TessVertex* Minimum()
	{
		return m_handles[m_nodes[1].handle].key;
	}

	inline bool IsEmpty() const
	{
		return m_size == 0;
	}

private:

	inline void FloatDown(int curr)
	{
		Handle hCurr = m_nodes[curr].handle;
		for ( ;; )
		{
			int child = curr << 1;
			if (child < m_size && VertLeq(m_handles[m_nodes[child+1].handle].key,
																	 m_handles[m_nodes[child].handle].key))
			{
				++child;
			}
			
			assert(child <= m_max);
			
			Handle hChild = m_nodes[child].handle;
			if (child > m_size || VertLeq(m_handles[hCurr].key, m_handles[hChild].key))
			{
				m_nodes[curr].handle = hCurr;
				m_handles[hCurr].node = curr;
				break;
			}
			m_nodes[curr].handle = hChild;
			m_handles[hChild].node = curr;
			curr = child;
		}
	}
	
	inline void FloatUp(long curr)
	{
		Handle hCurr = m_nodes[curr].handle;
		for ( ;; )
		{
			int parent = curr >> 1;
			Handle hParent = m_nodes[parent].handle;
			if (parent == 0 || VertLeq(m_handles[hParent].key, m_handles[hCurr].key))
			{
				m_nodes[curr].handle = hCurr;
				m_handles[hCurr].node = curr;
				break;
			}
			m_nodes[curr].handle = hParent;
			m_handles[hParent].node = curr;
			curr = parent;
		}
	}
	
	struct Node
	{
		Handle handle;
	};
	
	struct HandleElem
	{
		TessVertex* key;
		Handle node;
	};


  Node* m_nodes;
  HandleElem* m_handles;
  int m_size, m_max;
  Handle m_freeList;
  bool m_initialized;
};

template <typename T>
inline T Max(T x, T y) { return x >= y ? x : y; }

template <typename T>
inline T Min(T x, T y) { return x <= y ? x : y; }

template <typename T>
inline float Abs(T a) { return a < 0 ? -a : a; } 


Tess::Tess(TessWindingRule winding, int flags, int maxPolyVerts) :
	m_mesh(0),
	m_queue(0),
	m_dict(0),
	m_winding(winding),
	m_flags(flags),
	m_maxPolyVerts(maxPolyVerts),
	m_bminx(0),
	m_bmaxx(0),
	m_bminy(0),
	m_bmaxy(0),
	m_processed(false)
{
}

Tess::~Tess()
{
	Clear();
}

inline Tess::ActiveRegion* Tess::RegionBelow(Tess::ActiveRegion* r)
{
	return r->nodeUp->prev->key;
}

inline Tess::ActiveRegion* Tess::RegionAbove(Tess::ActiveRegion* r)
{
	return r->nodeUp->next->key;
}

inline bool Tess::IsWindingInside(int n)
{
	switch (m_winding)
	{
	case TESS_WINDING_ODD:
		return (n & 1);
	case TESS_WINDING_NONZERO:
		return (n != 0);
	case TESS_WINDING_POSITIVE:
		return (n > 0);
	case TESS_WINDING_NEGATIVE:
		return (n < 0);
	case TESS_WINDING_ABS_GEQ_TWO:
		return (n >= 2) || (n <= -2);
	}
	assert(false);
	return false;
}
	
inline void Tess::ComputeWinding(ActiveRegion* reg)
{
	reg->windingNumber = RegionAbove(reg)->windingNumber + reg->eUp->winding;
	reg->inside = IsWindingInside(reg->windingNumber);
}

void Tess::Clear()
{
	// At the end of all processing, the dictionary should contain
	// only the two sentinel edges, plus at most one "fixable" edge
	// created by ConnectRightVertex().
	if (m_dict)
	{
		ActiveRegion* reg;
		int fixedEdges = 0;
		while ((reg = m_dict->First()->key) != 0)
		{
			if (!reg->sentinel)
			{
				assert(reg->fixUpperEdge);
				assert(++fixedEdges == 1);
			}
			assert(reg->windingNumber == 0);
			DeleteRegion(reg);
		}
		(void)fixedEdges;
	}
	delete m_dict;
	m_dict = 0;
	delete m_queue;
	m_queue = 0;
	delete m_mesh;
	m_mesh = 0;
	m_processed = false;
}


bool Tess::AddContour(const float* verts, int nv)
{
	// Do not allow to add more contours after the polygons have been processed.
	if (m_processed)
		return false;
		
	// Skip degenerate polygons.
	if (nv < 3)
		return true;

	if (!m_mesh)
	{
		m_mesh = new TessMesh;
		m_bminx = m_bmaxx = verts[0];
		m_bminy = m_bmaxy = verts[1];
	}
	if (!m_mesh)
		return false;

	TessHalfEdge* e = 0;
	for (int i = 0; i < nv; ++i)
	{
		float x = *verts++;
		float y = *verts++;
		float z = *verts++;

		m_bminx = Min(m_bminx, x);
		m_bmaxx = Max(m_bmaxx, x);
		m_bminy = Min(m_bminy, y);
		m_bmaxy = Max(m_bmaxy, y);

		if (!e)
		{
			// Make a self-loop (one vertex, one edge).
			e = m_mesh->CreateEdge();
			if (!e) return false;
			if (!m_mesh->SpliceEdge(e, e->Sym())) return false;
		}
		else
		{
			// Create a new vertex and edge which immediately follow e
			// in the ordering around the left face.
			if (!m_mesh->SplitEdge(e)) return false;
			e = e->LNext();
		}
		
		// The new vertex is now e->Org.
		e->org->x = x;
		e->org->y = y;
		e->org->z = z;
		
		// The winding of an edge says how the winding number changes as we
		// cross from the edge''s right face to its left face.  We add the
		// vertices in such an order that a CCW contour will add +1 to
		// the winding number of the region inside the contour.
		e->winding = 1;
		e->Sym()->winding = -1;
	}

	m_mesh->CheckMesh();

	return true;
}


bool Tess::Process()
{
	// Do not allow to process twice.
	if (m_processed)
		return false;

	m_processed = true;
	
	// ComputeInterior() computes the planar arrangement specified
	// by the given contours, and further subdivides this arrangement
	// into regions.  Each region is marked "inside" if it belongs
	// to the polygon, according to the rule given by tess->windingRule.
	// Each interior region is guaranteed be monotone.
	if (!ComputeInterior())
		return false;
	m_mesh->CheckMesh();

	if (m_flags & TESS_PROCESS_TRIANGLES)
	{
		if (!m_mesh->TessellateInterior())
			return false;
		m_mesh->CheckMesh();
		if (!m_mesh->OptimizeTriangles())
			return false;
	}
	else if (m_flags & TESS_PROCESS_CONVEX_POLYGONS)
	{
		if (!m_mesh->TessellateInterior())
			return false;
		m_mesh->CheckMesh();
		if (!m_mesh->OptimizeTriangles())
			return false;
		m_mesh->CheckMesh();
		if (!m_mesh->MergeConvexFaces(m_maxPolyVerts))
			return false;
	}
	else if (m_flags & TESS_PROCESS_CONTOURS)
	{
		if (!m_mesh->SetWindingNumber(1, true))
			return false;
	}

	m_mesh->CheckMesh();

	return true;	
}

// Invariants for the Edge Dictionary.
// - each pair of adjacent edges e2=Succ(e1) satisfies EdgeLeq(e1,e2)
//   at any valid location of the sweep event
// - if EdgeLeq(e2,e1) as well (at any valid sweep event), then e1 and e2
//   share a common endpoint
// - for each e, e->Dst has been processed, but not e->Org
// - each edge e satisfies VertLeq(e->Dst,event) && VertLeq(event,e->Org)
//   where "event" is the current sweep line event.
// - no edge e has zero length
//
// Invariants for the Mesh (the processed portion).
// - the portion of the mesh left of the sweep line is a planar graph,
//   ie. there is *some* way to embed it in the plane
// - no processed edge has zero length
// - no two processed vertices have identical coordinates
// - each "inside" region is monotone, ie. can be broken into two chains
//   of monotonically increasing vertices according to VertLeq(v1,v2)
//   - a non-invariant: these chains may intersect (very slightly)
//
// Invariants for the Sweep.
// - if none of the edges incident to the event vertex have an activeRegion
//   (ie. none of these edges are in the edge dictionary), then the vertex
//   has only right-going edges.
// - if an edge is marked "fixUpperEdge" (it is a temporary edge introduced
//   by ConnectRightVertex), then it is the only right-going edge from
//   its associated vertex.  (This says that these edges exist only
//   when it is necessary.)

void Tess::DeleteRegion(ActiveRegion* reg)
{
  if (reg->fixUpperEdge)
	{
    // It was created with zero winding number, so it better be
    // deleted with zero winding number (ie. it better not get merged
    // with a real edge).
    assert(reg->eUp->winding == 0);
  }
  reg->eUp->activeRegion = 0;
  m_dict->Delete(reg->nodeUp);
	m_actRegPool.Free(reg);
//	delete reg;
}


// Replace an upper edge which needs fixing (see ConnectRightVertex).
bool Tess::FixUpperEdge(Tess::ActiveRegion* reg, TessHalfEdge* newEdge)
{
  assert(reg->fixUpperEdge);
  if (!m_mesh->DeleteEdge(reg->eUp)) return false;
  reg->fixUpperEdge = false;
  reg->eUp = newEdge;
  newEdge->activeRegion = reg;

  return true; 
}

Tess::ActiveRegion* Tess::TopLeftRegion(Tess::ActiveRegion* reg)
{
  TessVertex*org = reg->eUp->Org();

  // Find the region above the uppermost edge with the same origin
  do
	{
    reg = RegionAbove(reg);
  }
	while (reg->eUp->Org() == org);

  // If the edge above was a temporary edge introduced by ConnectRightVertex,
  // now is the time to fix it.
  if (reg->fixUpperEdge)
	{
    TessHalfEdge* e = m_mesh->ConnectEdge(RegionBelow(reg)->eUp->Sym(), reg->eUp->LNext());
    if (!e) return 0;
    if (!FixUpperEdge(reg, e)) return 0;
    reg = RegionAbove(reg);
  }
  return reg;
}

Tess::ActiveRegion* Tess::TopRightRegion(Tess::ActiveRegion* reg)
{
  TessVertex* dst = reg->eUp->Dst();
  // Find the region above the uppermost edge with the same destination
  do
	{
    reg = RegionAbove(reg);
  }
	while (reg->eUp->Dst() == dst);
  return reg;
}

// Add a new active region to the sweep line, *somewhere* below "regAbove"
// (according to where the new edge belongs in the sweep-line dictionary).
// The upper edge of the new region will be "eNewUp".
// Winding number and "inside" flag are not updated.
Tess::ActiveRegion* Tess::AddRegionBelow(ActiveRegion* regAbove, TessHalfEdge* eNewUp)
{
  ActiveRegion* regNew = m_actRegPool.Alloc(); //new ActiveRegion;
	if (!regNew) return 0;

  regNew->eUp = eNewUp;
  regNew->nodeUp = m_dict->InsertBefore(regAbove->nodeUp, regNew);
  if (!regNew->nodeUp) return 0;
  regNew->fixUpperEdge = false;
  regNew->sentinel = false;
  regNew->dirty = false;

  eNewUp->activeRegion = regNew;
  return regNew;
}


// Delete a region from the sweep line.  This happens when the upper
// and lower chains of a region meet (at a vertex on the sweep line).
// The "inside" flag is copied to the appropriate mesh face (we could
// not do this before -- since the structure of the mesh is always
// changing, this face may not have even existed until now).
bool Tess::FinishRegion(ActiveRegion* reg)
{
  TessHalfEdge* e = reg->eUp;
  TessFace* f = e->LFace();

  f->inside = reg->inside;
  f->anEdge = e;   // optimization for __gl_meshTessellateMonoRegion()
  DeleteRegion(reg);

	return true;
}


// We are given a vertex with one or more left-going edges.  All affected
// edges should be in the edge dictionary.  Starting at regFirst->eUp,
// we walk down deleting all regions where both edges have the same
// origin vOrg.  At the same time we copy the "inside" flag from the
// active region to the face, since at this point each face will belong
// to at most one region (this was not necessarily true until this point
// in the sweep).  The walk stops at the region above regLast; if regLast
// is NULL we walk as far as possible.  At the same time we relink the
// mesh if necessary, so that the ordering of edges around vOrg is the
// same as in the dictionary.
TessHalfEdge* Tess::FinishLeftRegions(ActiveRegion* regFirst, ActiveRegion* regLast)
{
	ActiveRegion* regPrev = regFirst;
	TessHalfEdge* ePrev = regFirst->eUp;
	while (regPrev != regLast)
	{
		regPrev->fixUpperEdge = false;	// placement was OK
		ActiveRegion* reg = RegionBelow(regPrev);
		TessHalfEdge* e = reg->eUp;
		if (e->Org() != ePrev->Org())
		{
			if (!reg->fixUpperEdge)
			{
				// Remove the last left-going edge.  Even though there are no further
				// edges in the dictionary with this origin, there may be further
				// such edges in the mesh (if we are adding left edges to a vertex
				// that has already been processed).  Thus it is important to call
				// FinishRegion rather than just DeleteRegion.
				if (!FinishRegion(regPrev))
					return 0;
				break;
			}
			// If the edge below was a temporary edge introduced by
			// ConnectRightVertex, now is the time to fix it.
			e = m_mesh->ConnectEdge(ePrev->LPrev(), e->Sym());
			if (!e) return 0;
			if (!FixUpperEdge(reg, e)) return 0;
		}
		
		// Relink edges so that ePrev->Onext == e
		if (ePrev->ONext() != e)
		{
			if (!m_mesh->SpliceEdge(e->OPrev(), e)) return 0;
			if (!m_mesh->SpliceEdge(ePrev, e)) return 0;
		}
		if (!FinishRegion(regPrev))	// may change reg->eUp
			return 0;
		ePrev = reg->eUp;
		regPrev = reg;
	}
	
	return ePrev;
}


// Purpose: insert right-going edges into the edge dictionary, and update
// winding numbers and mesh connectivity appropriately.  All right-going
// edges share a common origin vOrg.  Edges are inserted CCW starting at
// eFirst; the last edge inserted is eLast->Oprev.  If vOrg has any
// left-going edges already processed, then eTopLeft must be the edge
// such that an imaginary upward vertical segment from vOrg would be
// contained between eTopLeft->Oprev and eTopLeft; otherwise eTopLeft
// should be NULL.
bool Tess::AddRightEdges(ActiveRegion* regUp,
       TessHalfEdge* eFirst, TessHalfEdge* eLast, TessHalfEdge* eTopLeft,
       bool cleanUp)
{
  // Insert the new right-going edges in the dictionary
  TessHalfEdge* e = eFirst;
  do
	{
    assert(VertLeq(e->Org(), e->Dst()));
    if (!AddRegionBelow(regUp, e->Sym()))
			return false;
    e = e->ONext();
  }
	while (e != eLast);

  // Walk *all* right-going edges from e->Org, in the dictionary order,
  // updating the winding numbers of each region, and re-linking the mesh
  // edges to match the dictionary ordering (if necessary).
  if (!eTopLeft)
	{
    eTopLeft = RegionBelow(regUp)->eUp->RPrev();
  }
  ActiveRegion* regPrev = regUp;
	ActiveRegion* reg = 0;
  TessHalfEdge* ePrev = eTopLeft;
  bool firstTime = true;
  for ( ;; )
	{
    reg = RegionBelow(regPrev);
    e = reg->eUp->Sym();
    if (e->Org() != ePrev->Org()) break;

    if (e->ONext() != ePrev)
		{
      // Unlink e from its current position, and relink below ePrev
      if (!m_mesh->SpliceEdge(e->OPrev(), e)) return false;
      if (!m_mesh->SpliceEdge(ePrev->OPrev(), e)) return false;
		}
    // Compute the winding number and "inside" flag for the new regions
    reg->windingNumber = regPrev->windingNumber - e->winding;
    reg->inside = IsWindingInside(reg->windingNumber);

    // Check for two outgoing edges with same slope -- process these
    // before any intersection tests (see example in __gl_computeInterior).
    regPrev->dirty = true;
    if (!firstTime)
		{
			bool splice = false;
			if (!CheckForRightSplice(regPrev, splice)) return false;
			if (splice)
			{
				AddWinding(e, ePrev);
				DeleteRegion(regPrev);
				if (!m_mesh->DeleteEdge(ePrev)) return false;
			}
    }
    firstTime = false;
    regPrev = reg;
    ePrev = e;
  }
  regPrev->dirty = true;
  assert(regPrev->windingNumber - e->winding == reg->windingNumber);

  if (cleanUp)
	{
    // Check for intersections between newly adjacent edges. */
    if (!WalkDirtyRegions(regPrev))
			return false;
  }

	return true;
}


// Check the upper and lower edge of "regUp", to make sure that the
// eUp->Org is above eLo, or eLo->Org is below eUp (depending on which
// origin is leftmost).
//
// The main purpose is to splice right-going edges with the same
// dest vertex and nearly identical slopes (ie. we can't distinguish
// the slopes numerically).  However the splicing can also help us
// to recover from numerical errors.  For example, suppose at one
// point we checked eUp and eLo, and decided that eUp->Org is barely
// above eLo.  Then later, we split eLo into two edges (eg. from
// a splice operation like this one).  This can change the result of
// our test so that now eUp->Org is incident to eLo, or barely below it.
// We must correct this condition to maintain the dictionary invariants.
//
// One possibility is to check these edges for intersection again
// (ie. CheckForIntersect).  This is what we do if possible.  However
// CheckForIntersect requires that tess->event lies between eUp and eLo,
// so that it has something to fall back on when the intersection
// calculation gives us an unusable answer.  So, for those cases where
// we can't check for intersection, this routine fixes the problem
// by just splicing the offending vertex into the other edge.
// This is a guaranteed solution, no matter how degenerate things get.
// Basically this is a combinatorial solution to a numerical problem.
bool Tess::CheckForRightSplice(ActiveRegion* regUp, bool& retSplice)
{
  ActiveRegion* regLo = RegionBelow(regUp);
  TessHalfEdge* eUp = regUp->eUp;
  TessHalfEdge* eLo = regLo->eUp;

  if (VertLeq(eUp->Org(), eLo->Org()))
	{
    if (EdgeSign(eLo->Dst(), eUp->Org(), eLo->Org()) > 0)
		{
			retSplice = false;
			return true;
		}

    // eUp->Org appears to be below eLo
    if (!VertEq(eUp->Org(), eLo->Org()))
		{
      // Splice eUp->Org into eLo
      if (!m_mesh->SplitEdge(eLo->Sym())) return false;
      if (!m_mesh->SpliceEdge(eUp, eLo->OPrev())) return false;
      regUp->dirty = regLo->dirty = true;
    }
		else if (eUp->Org() != eLo->Org())
		{
      // merge the two vertices, discarding eUp->Org
			m_queue->Delete(eUp->Org()->pqHandle);
      if (!m_mesh->SpliceEdge(eLo->OPrev(), eUp)) return false;
			// SpliceMergeVertices(eLo->Oprev, eUp)
    }
  }
	else
	{
    if (EdgeSign(eUp->Dst(), eLo->Org(), eUp->Org()) < 0)
		{
			retSplice = false;
			return true;
		}

    // eLo->Org appears to be above eUp, so splice eLo->Org into eUp
    RegionAbove(regUp)->dirty = regUp->dirty = true;
    if (!m_mesh->SplitEdge(eUp->Sym())) return false;
    if (!m_mesh->SpliceEdge(eLo->OPrev(), eUp)) return false;
  }

	retSplice = true;
	return true;
}

// Check the upper and lower edge of "regUp", to make sure that the
// eUp->Dst is above eLo, or eLo->Dst is below eUp (depending on which
// destination is rightmost).
//
// Theoretically, this should always be true.  However, splitting an edge
// into two pieces can change the results of previous tests.  For example,
// suppose at one point we checked eUp and eLo, and decided that eUp->Dst
// is barely above eLo.  Then later, we split eLo into two edges (eg. from
// a splice operation like this one).  This can change the result of
// the test so that now eUp->Dst is incident to eLo, or barely below it.
// We must correct this condition to maintain the dictionary invariants
// (otherwise new edges might get inserted in the wrong place in the
// dictionary, and bad stuff will happen).
//
// We fix the problem by just splicing the offending vertex into the
// other edge.
bool Tess::CheckForLeftSplice(ActiveRegion* regUp, bool& retSplice)
{
  ActiveRegion* regLo = RegionBelow(regUp);
  TessHalfEdge* eUp = regUp->eUp;
  TessHalfEdge* eLo = regLo->eUp;

  assert(!VertEq(eUp->Dst(), eLo->Dst()));

  if (VertLeq(eUp->Dst(), eLo->Dst()))
	{
    if (EdgeSign(eUp->Dst(), eLo->Dst(), eUp->Org()) < 0)
		{
			retSplice = false;
			return true;
		}

    // eLo->Dst is above eUp, so splice eLo->Dst into eUp
    RegionAbove(regUp)->dirty = regUp->dirty = true;
    TessHalfEdge* e = m_mesh->SplitEdge(eUp);
    if (!e) return false;
    if (!m_mesh->SpliceEdge(eLo->Sym(), e)) return false;
    e->LFace()->inside = regUp->inside;
  }
	else
	{
    if (EdgeSign(eLo->Dst(), eUp->Dst(), eLo->Org()) > 0)
		{
			retSplice = false;
			return true;
		}

    // eUp->Dst is below eLo, so splice eUp->Dst into eLo
    regUp->dirty = regLo->dirty = true;
    TessHalfEdge* e = m_mesh->SplitEdge(eLo);
    if (!e) return false;    
    if (!m_mesh->SpliceEdge(eUp->LNext(), eLo->Sym())) return false;
    e->RFace()->inside = regUp->inside;
  }

	retSplice = true;
	return true;
}

inline float VertL1dist(TessVertex* u, TessVertex* v)
{
	return Abs(u->x - v->x) + Abs(u->y - v->y);
}

// Find some weights which describe how the intersection vertex is
// a linear combination of "org" and "dest".  Each of the two edges
// which generated "isect" is allocated 50% of the weight; each edge
// splits the weight between its org and dst according to the
// relative distance to "isect".
static void VertexWeights(TessVertex* isect, TessVertex* org, TessVertex* dst)
{
	float t1 = VertL1dist(org, isect);
	float t2 = VertL1dist(dst, isect);
	float w1 = 0.5f * t2 / (t1 + t2);
	float w2 = 0.5f * t1 / (t1 + t2);
	isect->z += w1*org->z + w2*dst->z;
}

// We've computed a new intersection point, now we need a "data" pointer
// from the user so that we can refer to this new vertex in the
// rendering callbacks.
static void GetIntersectData(TessVertex* isect,
       TessVertex* orgUp, TessVertex* dstUp,
       TessVertex* orgLo, TessVertex* dstLo)
{
  isect->z = 0;
  VertexWeights(isect, orgUp, dstUp);
  VertexWeights(isect, orgLo, dstLo);
}


// Check the upper and lower edges of the given region to see if
// they intersect.  If so, create the intersection and add it
// to the data structures.
//
// Returns TRUE if adding the new intersection resulted in a recursive
// call to AddRightEdges(); in this case all "dirty" regions have been
// checked for intersections, and possibly regUp has been deleted.
bool Tess::CheckForIntersect(ActiveRegion* regUp, bool& retIntr)
{
  ActiveRegion* regLo = RegionBelow(regUp);
  TessHalfEdge* eUp = regUp->eUp;
  TessHalfEdge* eLo = regLo->eUp;
  TessVertex* orgUp = eUp->Org();
  TessVertex* orgLo = eLo->Org();
  TessVertex* dstUp = eUp->Dst();
  TessVertex* dstLo = eLo->Dst();
//  GLdouble tMinUp, tMaxLo;
//  GLUvertex isect, *orgMin;
 // GLUhalfEdge *e;

  assert(!VertEq(dstLo, dstUp));
  assert(EdgeSign(dstUp, m_dict->m_event, orgUp) <= 0);
  assert(EdgeSign(dstLo, m_dict->m_event, orgLo) >= 0);
  assert(orgUp != m_dict->m_event && orgLo != m_dict->m_event);
  assert(!regUp->fixUpperEdge && !regLo->fixUpperEdge);

  if (orgUp == orgLo)
	{
		// right endpoints are the same
		retIntr = false;
		return true;
	}

  float tMinUp = Min(orgUp->y, dstUp->y);
  float tMaxLo = Max(orgLo->y, dstLo->y);
  if (tMinUp > tMaxLo)
	{
		// y ranges do not overlap
		retIntr = false;
		return true;
	}

  // NOTE Jun 24, 2009: <pvl> changed EdgeSign() inequalities to non-strict
  // since we don't consider self-touching contours as self-intersecting
  if (VertLeq(orgUp, orgLo))
	{
    if (EdgeSign(dstLo, orgUp, orgLo) >= 0)
		{
			retIntr = false;
			return true;
		}
  }
	else
	{
    if (EdgeSign(dstUp, orgLo, orgUp) <= 0)
		{
			retIntr = false;
			return true;
		}
  }

  // At this point the edges intersect, at least marginally
//	DebugEvent();

	if (m_flags & TESS_PROCESS_NO_INTERSECTIONS)
	{
		// Fail
#if defined (_WIN32) && defined (USER_pavel)
		float where[8];
		where[0] = dstUp->x;
		where[1] = dstUp->y;
		where[2] = orgUp->x;
		where[3] = orgUp->y;
		where[4] = dstLo->x;
		where[5] = dstLo->y;
		where[6] = orgLo->x;
		where[7] = orgLo->y;
		throw where;
#endif
		return false;
	}
	
	TessVertex isect;
	EdgeIntersect(dstUp, orgUp, dstLo, orgLo, &isect);
	
  // The following properties are guaranteed:
  assert(Min(orgUp->y, dstUp->y) <= isect.y);
  assert(isect.y <= Max(orgLo->y, dstLo->y));
  assert(Min(dstLo->x, dstUp->x) <= isect.x);
  assert(isect.x <= Max(orgLo->x, orgUp->x));

  if (VertLeq(&isect, m_dict->m_event))
	{
    // The intersection point lies slightly to the left of the sweep line,
    // so move it until it''s slightly to the right of the sweep line.
    // (If we had perfect numerical precision, this would never happen
    // in the first place).  The easiest and safest thing to do is
    // replace the intersection by tess->event.
    isect.x = m_dict->m_event->x;
    isect.y = m_dict->m_event->y;
  }
  // Similarly, if the computed intersection lies to the right of the
  // rightmost origin (which should rarely happen), it can cause
  // unbelievable inefficiency on sufficiently degenerate inputs.
  // (If you have the test program, try running test54.d with the
  // "X zoom" option turned on).
  TessVertex* orgMin = VertLeq(orgUp, orgLo) ? orgUp : orgLo;
  if (VertLeq(orgMin, &isect))
	{
    isect.x = orgMin->x;
    isect.y = orgMin->y;
  }

  if (VertEq(&isect, orgUp) || VertEq(&isect, orgLo))
	{
    // Easy case -- intersection at one of the right endpoints
		bool splice = false;
		if (!CheckForRightSplice(regUp, splice))
			return false;
		retIntr = false;
		return true;
  }

  if ((!VertEq(dstUp, m_dict->m_event) && EdgeSign(dstUp, m_dict->m_event, &isect) >= 0) ||
      (!VertEq(dstLo, m_dict->m_event) && EdgeSign(dstLo, m_dict->m_event, &isect) <= 0))
  {
    // Very unusual -- the new upper or lower edge would pass on the
    // wrong side of the sweep event, or through it.  This can happen
    // due to very small numerical errors in the intersection calculation.
    if (dstLo == m_dict->m_event)
		{
      // Splice dstLo into eUp, and process the new region(s)
      if (!m_mesh->SplitEdge(eUp->Sym())) return false;
      if (!m_mesh->SpliceEdge(eLo->Sym(), eUp)) return false;
      regUp = TopLeftRegion(regUp);
      if (!regUp) return false;
      eUp = RegionBelow(regUp)->eUp;
      if (!FinishLeftRegions(RegionBelow(regUp), regLo))
				return false;
      if (!AddRightEdges(regUp, eUp->OPrev(), eUp, eUp, true))
				return false;
			// True
			retIntr = true;
			return true;
    }
    if (dstUp == m_dict->m_event)
		{
      // Splice dstUp into eLo, and process the new region(s)
      if (!m_mesh->SplitEdge(eLo->Sym())) return false;
      if (!m_mesh->SpliceEdge(eUp->LNext(), eLo->OPrev())) return false; 
      regLo = regUp;
      regUp = TopRightRegion(regUp);
      TessHalfEdge* e = RegionBelow(regUp)->eUp->RPrev();
      regLo->eUp = eLo->OPrev();
      eLo = FinishLeftRegions(regLo, 0);
			if (!eLo)
				return false;
      if (!AddRightEdges(regUp, eLo->ONext(), eUp->RPrev(), e, true))
				return false;
			// True
			retIntr = true;
			return true;
    }
    // Special case: called from ConnectRightVertex.  If either
    // edge passes on the wrong side of tess->event, split it
    // (and wait for ConnectRightVertex to splice it appropriately).
    if (EdgeSign(dstUp, m_dict->m_event, &isect) >= 0)
		{
      RegionAbove(regUp)->dirty = regUp->dirty = true;
      if (!m_mesh->SplitEdge(eUp->Sym())) return false;
      eUp->Org()->x = m_dict->m_event->x;
      eUp->Org()->y = m_dict->m_event->y;
    }
    if (EdgeSign(dstLo, m_dict->m_event, &isect) <= 0)
		{
      regUp->dirty = regLo->dirty = true;
      if (!m_mesh->SplitEdge(eLo->Sym())) return false;
      eLo->Org()->x = m_dict->m_event->x;
      eLo->Org()->y = m_dict->m_event->y;
    }
    // leave the rest for ConnectRightVertex
		retIntr = false;
		return true;
  }

  // General case -- split both edges, splice into new vertex.
  // When we do the splice operation, the order of the arguments is
  // arbitrary as far as correctness goes.  However, when the operation
  // creates a new face, the work done is proportional to the size of
  // the new face.  We expect the faces in the processed part of
  // the mesh (ie. eUp->Lface) to be smaller than the faces in the
  // unprocessed original contours (which will be eLo->Oprev->Lface).

  if (!m_mesh->SplitEdge(eUp->Sym())) return false;
  if (!m_mesh->SplitEdge(eLo->Sym())) return false;
  if (!m_mesh->SpliceEdge(eLo->OPrev(), eUp)) return false;
  eUp->Org()->x = isect.x;
  eUp->Org()->y = isect.y;
	// Interpolate Z
	GetIntersectData(eUp->Org(), orgUp, dstUp, orgLo, dstLo);
	
  eUp->Org()->pqHandle = m_queue->Insert(eUp->Org());
  if (eUp->Org()->pqHandle == EventQueue::INVALID_HANDLE)
		return false;

  RegionAbove(regUp)->dirty = regUp->dirty = regLo->dirty = true;

	retIntr = false;
	return true;
}

// When the upper or lower edge of any region changes, the region is
// marked "dirty".  This routine walks through all the dirty regions
// and makes sure that the dictionary invariants are satisfied
// (see the comments at the beginning of this file).  Of course
// new dirty regions can be created as we make changes to restore
// the invariants.
bool Tess::WalkDirtyRegions(ActiveRegion* regUp)
{
	ActiveRegion* regLo = RegionBelow(regUp);
	
	for ( ;; )
	{
		// Find the lowest dirty region (we walk from the bottom up).
		while (regLo->dirty)
		{
			regUp = regLo;
			regLo = RegionBelow(regLo);
		}
		if (!regUp->dirty)
		{
			regLo = regUp;
			regUp = RegionAbove(regUp);
			if (!regUp || ! regUp->dirty)
			{
				// We've walked all the dirty regions
				return true;
			}
		}
		regUp->dirty = false;
		TessHalfEdge* eUp = regUp->eUp;
		TessHalfEdge* eLo = regLo->eUp;
		
		if (eUp->Dst() != eLo->Dst())
		{
			// Check that the edge ordering is obeyed at the Dst vertices.
			bool splice = false;
			if (!CheckForLeftSplice(regUp, splice))
				return false;
			if (splice)
			{
				// If the upper or lower edge was marked fixUpperEdge, then
				// we no longer need it (since these edges are needed only for
				// vertices which otherwise have no right-going edges).
				if (regLo->fixUpperEdge)
				{
					DeleteRegion(regLo);
					if (!m_mesh->DeleteEdge(eLo)) return false;
					regLo = RegionBelow(regUp);
					eLo = regLo->eUp;
				}
				else if (regUp->fixUpperEdge)
				{
					DeleteRegion(regUp);
					if (!m_mesh->DeleteEdge(eUp)) return false;
					regUp = RegionAbove(regLo);
					eUp = regUp->eUp;
				}
			}
		}
		if (eUp->Org() != eLo->Org())
		{
			if (eUp->Dst() != eLo->Dst() &&
					!regUp->fixUpperEdge && ! regLo->fixUpperEdge &&
					(eUp->Dst() == m_dict->m_event || eLo->Dst() == m_dict->m_event))
			{
				// When all else fails in CheckForIntersect(), it uses tess->event
				// as the intersection location.  To make this possible, it requires
				// that tess->event lie between the upper and lower edges, and also
				// that neither of these is marked fixUpperEdge (since in the worst
				// case it might splice one of these edges into tess->event, and
				// violate the invariant that fixable edges are the only right-going
				// edge from their associated vertex).
				bool intr = false;
				if (!CheckForIntersect(regUp, intr))
					return false;
				if (intr)
				{
					// WalkDirtyRegions() was called recursively; we're done
					return true;
				}
			}
			else
			{
				// Even though we can't use CheckForIntersect(), the Org vertices
				// may violate the dictionary edge ordering.  Check and correct this.
				bool splice = false;
				if (!CheckForRightSplice(regUp, splice))
					return false;
			}
		}
		if (eUp->Org() == eLo->Org() && eUp->Dst() == eLo->Dst())
		{
			// A degenerate loop consisting of only two edges -- delete it.
			AddWinding(eLo, eUp);
			DeleteRegion(regUp);
			if (!m_mesh->DeleteEdge(eUp)) return false;
			regUp = RegionAbove(regLo);
		}
	}

	return true;
}


// Purpose: connect a "right" vertex vEvent (one where all edges go left)
// to the unprocessed portion of the mesh.  Since there are no right-going
// edges, two regions (one above vEvent and one below) are being merged
// into one.  "regUp" is the upper of these two regions.
//
// There are two reasons for doing this (adding a right-going edge):
//  - if the two regions being merged are "inside", we must add an edge
//    to keep them separated (the combined region would not be monotone).
//  - in any case, we must leave some record of vEvent in the dictionary,
//    so that we can merge vEvent with features that we have not seen yet.
//    For example, maybe there is a vertical edge which passes just to
//    the right of vEvent; we would like to splice vEvent into this edge.
//
// However, we don't want to connect vEvent to just any vertex.  We don''t
// want the new edge to cross any other edges; otherwise we will create
// intersection vertices even when the input data had no self-intersections.
// (This is a bad thing; if the user's input data has no intersections,
// we don't want to generate any false intersections ourselves.)
//
// Our eventual goal is to connect vEvent to the leftmost unprocessed
// vertex of the combined region (the union of regUp and regLo).
// But because of unseen vertices with all right-going edges, and also
// new vertices which may be created by edge intersections, we don''t
// know where that leftmost unprocessed vertex is.  In the meantime, we
// connect vEvent to the closest vertex of either chain, and mark the region
// as "fixUpperEdge".  This flag says to delete and reconnect this edge
// to the next processed vertex on the boundary of the combined region.
// Quite possibly the vertex we connected to will turn out to be the
// closest one, in which case we won''t need to make any changes.
bool Tess::ConnectRightVertex(ActiveRegion* regUp, TessHalfEdge* eBottomLeft)
{
  TessHalfEdge* eTopLeft = eBottomLeft->ONext();
  ActiveRegion* regLo = RegionBelow(regUp);
  TessHalfEdge* eUp = regUp->eUp;
  TessHalfEdge* eLo = regLo->eUp;
  bool degenerate = false;

  if (eUp->Dst() != eLo->Dst())
	{
		bool intr = false;
		if (!CheckForIntersect(regUp, intr))
			return false;
  }

  // Possible new degeneracies: upper or lower edge of regUp may pass
  // through vEvent, or may coincide with new intersection vertex
  if (VertEq(eUp->Org(), m_dict->m_event))
	{
    if (!m_mesh->SpliceEdge(eTopLeft->OPrev(), eUp)) return false;
    regUp = TopLeftRegion(regUp);
    if (!regUp) return false;
    eTopLeft = RegionBelow(regUp)->eUp;
    if (!FinishLeftRegions(RegionBelow(regUp), regLo))
			return false;
    degenerate = true;
  }
  if (VertEq(eLo->Org(), m_dict->m_event))
	{
    if (!m_mesh->SpliceEdge(eBottomLeft, eLo->OPrev())) return false;
    eBottomLeft = FinishLeftRegions(regLo, 0);
		if (!eBottomLeft)
			return false;
    degenerate = true;
  }
  if (degenerate)
	{
    if (!AddRightEdges(regUp, eBottomLeft->ONext(), eTopLeft, eTopLeft, true))
			return false;
    return true;
  }

  // Non-degenerate situation -- need to add a temporary, fixable edge.
  // Connect to the closer of eLo->Org, eUp->Org.
	TessHalfEdge* eNew = eUp;
  if (VertLeq(eLo->Org(), eUp->Org()))
    eNew = eLo->OPrev();
  eNew = m_mesh->ConnectEdge(eBottomLeft->LPrev(), eNew);
  if (!eNew) return false;

  // Prevent cleanup, otherwise eNew might disappear before we've even
  // had a chance to mark it as a temporary edge.
  if (!AddRightEdges(regUp, eNew, eNew->ONext(), eNew->ONext(), false))
		return false;
  reinterpret_cast<ActiveRegion*>(eNew->Sym()->activeRegion)->fixUpperEdge = true;
  if (!WalkDirtyRegions(regUp))
		return false;
	
	return true;
}

// Because vertices at exactly the same location are merged together
// before we process the sweep event, some degenerate cases can't occur.
// However if someone eventually makes the modifications required to
// merge features which are close together, the cases below marked
// TOLERANCE_NONZERO will be useful.  They were debugged before the
// code to merge identical vertices in the main loop was added.

#define TOLERANCE_NONZERO	false

// The event vertex lies exacty on an already-processed edge or vertex.
// Adding the new vertex involves splicing it into the already-processed
// part of the mesh.
bool Tess::ConnectLeftDegenerate(ActiveRegion* regUp, TessVertex* vEvent)
{
  TessHalfEdge* e = regUp->eUp;
  if (VertEq(e->Org(), vEvent))
	{
    // e->Org is an unprocessed vertex - just combine them, and wait
    // for e->Org to be pulled from the queue
    assert(TOLERANCE_NONZERO);
		if (!m_mesh->SpliceEdge(e, vEvent->anEdge)) return false;
		// SpliceMergeVertices(e, vEvent->anEdge);
    return true;
  }
  
  if (!VertEq(e->Dst(), vEvent))
	{
    // General case -- splice vEvent into edge e which passes through it
    if (m_mesh->SplitEdge(e->Sym()) == 0) return false;
    if (regUp->fixUpperEdge)
		{
      // This edge was fixable -- delete unused portion of original edge
      if (!m_mesh->DeleteEdge(e->ONext())) return false;
      regUp->fixUpperEdge = false;
    }
    if (!m_mesh->SpliceEdge(vEvent->anEdge, e)) return false;
    if (!SweepEvent(vEvent)) // recurse
			return false;
    return true;
  }

  // vEvent coincides with e->Dst, which has already been processed.
  // Splice in the additional right-going edges.
  assert(TOLERANCE_NONZERO);
  regUp = TopRightRegion(regUp);
  ActiveRegion* reg = RegionBelow(regUp);
  TessHalfEdge* eTopRight = reg->eUp->Sym();
  TessHalfEdge* eTopLeft = eTopRight->ONext();
	TessHalfEdge* eLast = eTopLeft;
  if (reg->fixUpperEdge)
	{
    // Here e->Dst has only a single fixable edge going right.
    // We can delete it since now we have some real right-going edges.
    assert(eTopLeft != eTopRight);   // there are some left edges too
    DeleteRegion(reg);
    if (!m_mesh->DeleteEdge(eTopRight)) return false;
    eTopRight = eTopLeft->OPrev();
  }
  if (!m_mesh->SpliceEdge(vEvent->anEdge, eTopRight)) return false;
  if (!EdgeGoesLeft(eTopLeft))
	{
    // e->Dst had no left-going edges -- indicate this to AddRightEdges()
    eTopLeft = 0;
  }
  if (!AddRightEdges(regUp, eTopRight->ONext(), eLast, eTopLeft, true))
		return false;
	return true;
}


// Purpose: connect a "left" vertex (one where both edges go right)
// to the processed portion of the mesh.  Let R be the active region
// containing vEvent, and let U and L be the upper and lower edge
// chains of R.  There are two possibilities:
//
// - the normal case: split R into two regions, by connecting vEvent to
//   the rightmost vertex of U or L lying to the left of the sweep line
//
// - the degenerate case: if vEvent is close enough to U or L, we
//   merge vEvent into that edge chain.  The subcases are:
//	- merging with the rightmost vertex of U or L
//	- merging with the active edge of U or L
//	- merging with an already-processed portion of U or L
bool Tess::ConnectLeftVertex(TessVertex* vEvent)
{
  /* assert( vEvent->anEdge->Onext->Onext == vEvent->anEdge ); */

  // Get a pointer to the active region containing vEvent
	ActiveRegion tmp;
  tmp.eUp = vEvent->anEdge->Sym();
  ActiveRegion* regUp = m_dict->Search(&tmp)->key;
  ActiveRegion* regLo = RegionBelow(regUp);
  TessHalfEdge* eUp = regUp->eUp;
  TessHalfEdge* eLo = regLo->eUp;

  // Try merging with U or L first
  if (EdgeSign(eUp->Dst(), vEvent, eUp->Org()) == 0)
	{
    if (!ConnectLeftDegenerate(regUp, vEvent))
			return false;
    return true;
  }

  // Connect vEvent to rightmost processed vertex of either chain.
  // e->Dst is the vertex that we will connect to vEvent.
  ActiveRegion* reg = VertLeq(eLo->Dst(), eUp->Dst()) ? regUp : regLo;

  if (regUp->inside || reg->fixUpperEdge)
	{
		TessHalfEdge* eNew = 0;
    if (reg == regUp)
		{
      eNew = m_mesh->ConnectEdge(vEvent->anEdge->Sym(), eUp->LNext());
			if (!eNew)
				return false;
    }
		else
		{
      TessHalfEdge* tempHalfEdge = m_mesh->ConnectEdge(eLo->DNext(), vEvent->anEdge);
			if (!tempHalfEdge)
				return false;
      eNew = tempHalfEdge->Sym();
    }
    if (reg->fixUpperEdge)
		{
      if (!FixUpperEdge(reg, eNew))
				return false;
    }
		else
		{
			ActiveRegion* regNew = AddRegionBelow(regUp, eNew);
			if (!regNew)
				return false;
      ComputeWinding(regNew);
    }
    if (!SweepEvent(vEvent))
			return false;
  }
	else
	{
    // The new vertex is in a region which does not belong to the polygon.
    // We don''t need to connect this vertex to the rest of the mesh.
    if (!AddRightEdges(regUp, vEvent->anEdge, vEvent->anEdge, 0, true))
			return false;
  }
	
	return true;
}


// Does everything necessary when the sweep line crosses a vertex.
// Updates the mesh and the edge dictionary.
bool Tess::SweepEvent(TessVertex* vEvent)
{
  m_dict->m_event = vEvent; // for access in EdgeLeq()
//	DebugEvent();
  
  // Check if this vertex is the right endpoint of an edge that is
  // already in the dictionary.  In this case we don't need to waste
  // time searching for the location to insert new edges.

  TessHalfEdge* e = vEvent->anEdge;
  while (e->activeRegion == 0)
	{
    e = e->ONext();
    if (e == vEvent->anEdge)
		{
      // All edges go right -- not incident to any processed edges
      if (!ConnectLeftVertex(vEvent))
				return false;
      return true;
    }
  }

  // Processing consists of two phases: first we "finish" all the
  // active regions where both the upper and lower edges terminate
  // at vEvent (ie. vEvent is closing off these regions).
  // We mark these faces "inside" or "outside" the polygon according
  // to their winding number, and delete the edges from the dictionary.
  // This takes care of all the left-going edges from vEvent.
  ActiveRegion* regUp = TopLeftRegion(reinterpret_cast<ActiveRegion*>(e->activeRegion));
  if (!regUp)
		return false;
  ActiveRegion* reg = RegionBelow(regUp);
  TessHalfEdge* eTopLeft = reg->eUp;
  TessHalfEdge* eBottomLeft = FinishLeftRegions(reg, 0);
	if (!eBottomLeft)
		return false;

  // Next we process all the right-going edges from vEvent.  This
  // involves adding the edges to the dictionary, and creating the
  // associated "active regions" which record information about the
  // regions between adjacent dictionary edges.
  if (eBottomLeft->ONext() == eTopLeft)
	{
    // No right-going edges -- add a temporary "fixable" edge
    if (!ConnectRightVertex(regUp, eBottomLeft))
			return false;
  }
	else
	{
    if (!AddRightEdges(regUp, eBottomLeft->ONext(), eTopLeft, eTopLeft, true))
			return false;
  }
	
	return true;
}


// We add two sentinel edges above and below all other edges,
// to avoid special cases at the top and bottom.
bool Tess::AddSentinel(Tess::Dict* dict, TessMesh* mesh, float x0, float x1, float y)
{
  ActiveRegion* reg = m_actRegPool.Alloc(); //new ActiveRegion;

  TessHalfEdge* e = mesh->CreateEdge();
	if (!e) return false;

  e->Org()->x = x1;
  e->Org()->y = y;
  e->Org()->z = 0;
  e->Dst()->x = x0;
  e->Dst()->y = y;
  e->Dst()->z = 0;
  dict->m_event = e->Dst(); // initialize it

  reg->eUp = e;
  reg->windingNumber = 0;
  reg->inside = false;
  reg->fixUpperEdge = false;
  reg->sentinel = true;
  reg->dirty = false;
  reg->nodeUp = dict->Insert(reg);
	if (!reg->nodeUp) return false;
	
	return true;
}

bool Tess::ComputeInterior()
{
//  tess->fatalError = FALSE;
	if (!m_mesh)
		return false;

  if (!m_mesh->RemoveDegenerateEdges())
		return false;

  // Each vertex defines an event for our sweep line.  Start by inserting
  // all the vertices in a priority queue.  Events are processed in
  // lexicographic order, ie.
  //	e1 < e2  iff  e1.x < e2.x || (e1.x == e2.x && e1.y < e2.y)
	assert(m_queue == 0);

	TessVertex* vHead = m_mesh->GetVHead();
	TessVertex* v = 0;
	int vertCount = 0;
	for (v = vHead->next; v != vHead; v = v->next)
		vertCount++;
	m_queue = new EventQueue(vertCount*3);

	for (v = vHead->next; v != vHead; v = v->next)
	{
		v->pqHandle = m_queue->Insert(v);
		if (v->pqHandle == EventQueue::INVALID_HANDLE) break;
	}
	if (v != vHead || !m_queue->Init())
		return false;


	// Init edge dict
	assert(m_dict == 0);
	m_dict = new Dict;

	float spad = Max(1.0f, Max(m_bmaxx-m_bminx, m_bmaxy-m_bminy)) / 10;
  if (!AddSentinel(m_dict, m_mesh, m_bminx-spad, m_bmaxx+spad, m_bminy-spad)) return false;
  if (!AddSentinel(m_dict, m_mesh, m_bminx-spad, m_bmaxx+spad, m_bmaxy+spad)) return false;

  while ((v = m_queue->ExtractMin()) != 0)
	{
    for ( ;; )
		{
      TessVertex* vNext = m_queue->Minimum();
      if (!vNext || !VertEq(vNext, v)) break;
      
      // Merge together all vertices at exactly the same location.
      // This is more efficient than processing them one at a time,
      // simplifies the code (see ConnectLeftDegenerate), and is also
      // important for correct handling of certain degenerate cases.
      // For example, suppose there are two identical edges A and B
      // that belong to different contours (so without this code they would
      // be processed by separate sweep events).  Suppose another edge C
      // crosses A and B from above.  When A is processed, we split it
      // at its intersection point with C.  However this also splits C,
      // so when we insert B we may compute a slightly different
      // intersection point.  This might leave two edges with a small
      // gap between them.  This kind of error is especially obvious
      // when using boundary extraction (GLU_TESS_BOUNDARY_ONLY).
      vNext = m_queue->ExtractMin();
			// SpliceMergeVertices(v->anEdge, vNext->anEdge);
			if (!m_mesh->SpliceEdge(v->anEdge, vNext->anEdge)) return false;
    }
    if (!SweepEvent(v))
			return false;
  }

  // Set tess->event for debugging purposes
  // __GL_DICTLISTKEY  __GL_DICTLISTMIN
//  tess->event = ((ActiveRegion *) dictKey( dictMin( tess->dict )))->eUp->Org;
	m_dict->m_event = m_dict->First()->key->eUp->Org();
//	DebugEvent();

  if (!m_mesh->RemoveDegenerateFaces())
		return false;

	m_mesh->CheckMesh();

  return true;
}
