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


#ifndef TESS_H
#define TESS_H

#include "TessMesh.h"
#include "../PoolAllocator.h"


enum TessWindingRule
{
	TESS_WINDING_ODD,
	TESS_WINDING_NONZERO,
	TESS_WINDING_POSITIVE,
	TESS_WINDING_NEGATIVE,
	TESS_WINDING_ABS_GEQ_TWO,
};

enum TessProcessFlags
{
	TESS_PROCESS_TRIANGLES					= 0x0001,
	TESS_PROCESS_CONVEX_POLYGONS		= 0x0002,
	TESS_PROCESS_CONTOURS					= 0x0004,
	TESS_PROCESS_NO_INTERSECTIONS	= 0x0010,
};


class Tess
{
public:
	Tess(TessWindingRule winding, int flags, int maxPolyVerts = 256);
	~Tess();

	bool AddContour(const float* verts, int nv);

	bool Process();
	
	inline TessMesh* GetMesh() { return m_mesh; }
	
	void Clear();
	
private:

	class EventQueue;
	class Dict;
	struct DictNode;
	struct ActiveRegion;

	// For each pair of adjacent edges crossing the sweep line, there is
	// an ActiveRegion to represent the region between them.  The active
	// regions are kept in sorted order in a dynamic dictionary.  As the
	// sweep line crosses each vertex, we update the affected regions.

	struct ActiveRegion
	{
		TessHalfEdge* eUp; // upper edge, directed right to left
		DictNode* nodeUp; // dictionary node corresponding to eUp
		int windingNumber;	// used to determine which regions are inside the polygon
		bool inside; //  is this region inside the polygon?
		bool sentinel; // marks fake edges at t = +/-infinity
		bool dirty; // marks regions where the upper or lower edge has changed, but we haven't checked whether they intersect yet
		bool fixUpperEdge;	// marks temporary edges introduced when we process a "right vertex" (one without any edges leaving to the right)
	};

	ActiveRegion* RegionBelow(ActiveRegion* r);
	ActiveRegion* RegionAbove(ActiveRegion* r);
	bool IsWindingInside(int n);
	void ComputeWinding(ActiveRegion* reg);
	bool ComputeInterior();
	void DeleteRegion(ActiveRegion* reg);
	bool FixUpperEdge(ActiveRegion* reg, TessHalfEdge* newEdge);
	ActiveRegion* TopLeftRegion(ActiveRegion* reg);
	ActiveRegion* TopRightRegion(ActiveRegion* reg);
	ActiveRegion* AddRegionBelow(ActiveRegion* regAbove, TessHalfEdge* eNewUp);
	bool FinishRegion(ActiveRegion* reg);
	TessHalfEdge* FinishLeftRegions(ActiveRegion* regFirst, ActiveRegion* regLast);
	bool AddRightEdges(ActiveRegion* regUp, TessHalfEdge* eFirst, TessHalfEdge* eLast, TessHalfEdge* eTopLeft, bool cleanUp);
	bool CheckForRightSplice(ActiveRegion* regUp, bool& retSplice);
	bool CheckForLeftSplice(ActiveRegion* regUp, bool& retSplice);
	bool CheckForIntersect(ActiveRegion* regUp, bool& retIntr);
	bool WalkDirtyRegions(ActiveRegion* regUp);
	bool ConnectRightVertex(ActiveRegion* regUp, TessHalfEdge* eBottomLeft);
	bool ConnectLeftDegenerate(ActiveRegion* regUp, TessVertex* vEvent);
	bool ConnectLeftVertex(TessVertex* vEvent);
	bool SweepEvent(TessVertex* vEvent);
	bool AddSentinel(Dict* dict, TessMesh* mesh, float x0, float x1, float y);

	void DebugEvent();

	float m_bminx, m_bmaxx, m_bminy, m_bmaxy;
	TessMesh* m_mesh;
	EventQueue* m_queue;
	Dict* m_dict;
	TessWindingRule m_winding;
	int m_flags;
	int m_maxPolyVerts;
	bool m_processed;
	LayeredNavMesh::PoolAllocator<ActiveRegion> m_actRegPool;
};

#endif