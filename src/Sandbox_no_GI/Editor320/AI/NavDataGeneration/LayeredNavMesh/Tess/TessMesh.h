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

#ifndef TESSMESH_H
#define TESSMESH_H

#include "../PoolAllocator.h"

// The mesh structure is similar in spirit, notation, and operations
// to the "quad-edge" structure (see L. Guibas and J. Stolfi, Primitives
// for the manipulation of general subdivisions and the computation of
// Voronoi diagrams, ACM Transactions on Graphics, 4(2):74-123, April 1985).
// For a simplified description, see the course notes for CS348a,
// "Mathematical Foundations of Computer Graphics", available at the
// Stanford bookstore (and taught during the fall quarter).
// The implementation also borrows a tiny subset of the graph-based approach
// use in Mantyla's Geometric Work Bench (see M. Mantyla, An Introduction
// to Sold Modeling, Computer Science Press, Rockville, Maryland, 1988).
//
// The fundamental data structure is the "half-edge".  Two half-edges
// go together to make an edge, but they point in opposite directions.
// Each half-edge has a pointer to its mate (the "symmetric" half-edge Sym),
// its origin vertex (Org), the face on its left side (Lface), and the
// adjacent half-edges in the CCW direction around the origin vertex
// (Onext) and around the left face (Lnext).  There is also a "next"
// pointer for the global edge list (see below).
//
// The notation used for mesh navigation:
//	Sym   = the mate of a half-edge (same edge, but opposite direction)
//	Onext = edge CCW around origin vertex (keep same origin)
//	Dnext = edge CCW around destination vertex (keep same dest)
//	Lnext = edge CCW around left face (dest becomes new origin)
//	Rnext = edge CCW around right face (origin becomes new dest)
//
// "prev" means to substitute CW for CCW in the definitions above.
//
// The mesh keeps global lists of all vertices, faces, and edges,
// stored as doubly-linked circular lists with a dummy header node.
// The mesh stores pointers to these dummy headers (vHead, fHead, eHead).
//
// The circular edge list is special; since half-edges always occur
// in pairs (e and e->Sym), each half-edge stores a pointer in only
// one direction.  Starting at eHead and following the e->next pointers
// will visit each *edge* once (ie. e or e->Sym, but not both).
// e->Sym stores a pointer in the opposite direction, thus it is
// always true that e->Sym->next->Sym->next == e.
//
// Each vertex has a pointer to next and previous vertices in the
// circular list, and a pointer to a half-edge with this vertex as
// the origin (NULL if this is the dummy header).  There is also a
// field "data" for client data.
//
// Each face has a pointer to the next and previous faces in the
// circular list, and a pointer to a half-edge with this face as
// the left face (NULL if this is the dummy header).  There is also
// a field "data" for client data.
//
// Note that what we call a "face" is really a loop; faces may consist
// of more than one loop (ie. not simply connected), but there is no
// record of this in the data structure.  The mesh may consist of
// several disconnected regions, so it may not be possible to visit
// the entire mesh by starting at a half-edge and traversing the edge
// structure.
//
// The mesh does NOT support isolated vertices; a vertex is deleted along
// with its last edge.  Similarly when two faces are merged, one of the
// faces is deleted (see __gl_meshDelete below).  For mesh operations,
// all face (loop) and vertex pointers must not be NULL.  However, once
// mesh manipulation is finished, __gl_MeshZapFace can be used to delete
// faces of the mesh, one at a time.  All external faces can be "zapped"
// before the mesh is returned to the client; then a NULL face indicates
// a region which is not part of the output polygon.


struct TessVertex;
struct TessFace;
struct TessHalfEdge;

struct TessVertex
{
	TessVertex* next; // next vertex (never NULL)
	TessVertex* prev; // previous vertex (never NULL)
	TessHalfEdge* anEdge; // a half-edge with this origin

	// Internal data (keep hidden)
	float x, y;
	float z;
	int pqHandle; // to allow deletion from priority queue
};

struct TessFace
{
  TessFace* next; // next face (never NULL)
  TessFace* prev; // previous face (never NULL)
  TessHalfEdge* anEdge; // a half edge with this left face

  // Internal data (keep hidden)
  bool marked; // flag for conversion to strips
  bool inside; // this face is in the polygon interior
};

struct TessHalfEdge
{
  TessHalfEdge* next; // doubly-linked list (prev==Sym->next)
  TessHalfEdge* sym; // same edge, opposite direction
  TessHalfEdge* orgNext; // next edge CCW around origin
  TessHalfEdge* leftNext; // next edge CCW around left face
  TessVertex* org; // origin vertex (Overtex too long)
  TessFace* leftFace; // left face

	inline TessVertex*& Org() { return org; }
	inline TessVertex*& Dst() { return sym->org; }

	inline TessHalfEdge*& Sym() { return sym; }

	inline TessHalfEdge*& ONext() { return orgNext; }
	inline TessHalfEdge*& OPrev() { return sym->leftNext; }

	inline TessHalfEdge*& LNext() { return leftNext; }
	inline TessHalfEdge*& LPrev() { return orgNext->sym; }

	inline TessHalfEdge*& RNext() { return sym->leftNext->sym; }
	inline TessHalfEdge*& RPrev() { return sym->orgNext; }

	inline TessHalfEdge*& DNext() { return sym->orgNext->sym; }
	inline TessHalfEdge*& DPrev() { return leftNext->sym; }

	inline TessFace*& LFace() { return leftFace; }
	inline TessFace*& RFace() { return sym->leftFace; }

  // Internal data (keep hidden)
  void* activeRegion; // a region with this upper edge (sweep.c)
  int winding; // change in winding number when crossing from the right face to the left face
};

// When we merge two edges into one, we need to compute the combined
// winding of the new edge.
inline void AddWinding(TessHalfEdge* eDst, TessHalfEdge* eSrc)
{
	eDst->winding += eSrc->winding;
	eDst->Sym()->winding += eSrc->Sym()->winding;
}



// The mesh operations below have three motivations: completeness,
// convenience, and efficiency.  The basic mesh operations are MakeEdge,
// Splice, and Delete.  All the other edge operations can be implemented
// in terms of these.  The other operations are provided for convenience
// and/or efficiency.
//
// When a face is split or a vertex is added, they are inserted into the
// global list *before* the existing vertex or face (ie. e->Org or e->Lface).
// This makes it easier to process all vertices or faces in the global lists
// without worrying about processing the same data twice.  As a convenience,
// when a face is split, the "inside" flag is copied from the old face.
// Other internal data (v->data, v->activeRegion, f->data, f->marked,
// f->trail, e->winding) is set to zero.


class TessMesh
{
public:
	TessMesh();
	~TessMesh();
	
	// ********************** Basic Edge Operations **************************

	// __gl_meshMakeEdge( mesh ) creates one edge, two vertices, and a loop.
	// The loop (face) consists of the two new half-edges.
	TessHalfEdge* CreateEdge();

	// __gl_meshSplice( eOrg, eDst ) is the basic operation for changing the
	// mesh connectivity and topology.  It changes the mesh so that
	//	eOrg->Onext <- OLD( eDst->Onext )
	//	eDst->Onext <- OLD( eOrg->Onext )
	// where OLD(...) means the value before the meshSplice operation.
	//
	// This can have two effects on the vertex structure:
	//  - if eOrg->Org != eDst->Org, the two vertices are merged together
	//  - if eOrg->Org == eDst->Org, the origin is split into two vertices
	// In both cases, eDst->Org is changed and eOrg->Org is untouched.
	//
	// Similarly (and independently) for the face structure,
	//  - if eOrg->Lface == eDst->Lface, one loop is split into two
	//  - if eOrg->Lface != eDst->Lface, two distinct loops are joined into one
	// In both cases, eDst->Lface is changed and eOrg->Lface is unaffected.
	bool SpliceEdge(TessHalfEdge* eOrg, TessHalfEdge* eDst);

	// __gl_meshDelete( eDel ) removes the edge eDel.  There are several cases:
	// if (eDel->Lface != eDel->Rface), we join two loops into one; the loop
	// eDel->Lface is deleted.  Otherwise, we are splitting one loop into two;
	// the newly created loop will contain eDel->Dst.  If the deletion of eDel
	// would create isolated vertices, those are deleted as well.
	bool DeleteEdge(TessHalfEdge* eDel);


	// ********************** Other Edge Operations **************************

	// __gl_meshAddEdgeVertex( eOrg ) creates a new edge eNew such that
	// eNew == eOrg->Lnext, and eNew->Dst is a newly created vertex.
	// eOrg and eNew will have the same left face.
	TessHalfEdge* AddEdgeVertex(TessHalfEdge* eOrg);

	// __gl_meshSplitEdge( eOrg ) splits eOrg into two edges eOrg and eNew,
	// such that eNew == eOrg->Lnext.  The new vertex is eOrg->Dst == eNew->Org.
	// eOrg and eNew will have the same left face.
	TessHalfEdge* SplitEdge(TessHalfEdge* eOrg);

	// __gl_meshConnect( eOrg, eDst ) creates a new edge from eOrg->Dst
	// to eDst->Org, and returns the corresponding half-edge eNew.
	// If eOrg->Lface == eDst->Lface, this splits one loop into two,
	// and the newly created loop is eNew->Lface.  Otherwise, two disjoint
	// loops are merged into one, and the loop eDst->Lface is destroyed.
	TessHalfEdge* ConnectEdge(TessHalfEdge* eOrg, TessHalfEdge* eDst);

	// ************************ Other Operations *****************************

	// __gl_meshZapFace( fZap ) destroys a face and removes it from the
	// global face list.  All edges of fZap will have a NULL pointer as their
	// left face.  Any edges which also have a NULL pointer as their right face
	// are deleted entirely (along with any isolated vertices this produces).
	// An entire mesh can be deleted by zapping its faces, one at a time,
	// in any order.  Zapped faces cannot be used in further mesh operations!
	void ZapFace(TessFace* fZap);

	// __gl_meshUnion( mesh1, mesh2 ) forms the union of all structures in
	// both meshes, and returns the new mesh (the old meshes are destroyed).
	void Union(TessMesh* mesh2);

	// __gl_meshCheckMesh( mesh ) checks a mesh for self-consistency.
	void CheckMesh();

	//Remove zero-length edges, and contours with fewer than 3 vertices.
	bool RemoveDegenerateEdges();

	// Delete any degenerate faces with only two edges.
	bool RemoveDegenerateFaces();

	// Merges neighbour triangles into convex faces.
	bool MergeConvexFaces(int maxVerts);

	// Tries to optimize the mesh by flipping edges.
	bool OptimizeTriangles();

	// __gl_meshTessellateInterior( mesh ) tessellates each region of
	// the mesh which is marked "inside" the polygon.  Each such region
	// must be monotone.
	bool TessellateInterior();

	// __gl_meshDiscardExterior( mesh ) zaps (ie. sets to NULL) all faces
	// which are not marked "inside" the polygon.  Since further mesh operations
	// on NULL faces are not allowed, the main purpose is to clean up the
	// mesh so that exterior loops are not represented in the data structure.
	void DiscardExterior();

	// __gl_meshSetWindingNumber( mesh, value, keepOnlyBoundary ) resets the
	// winding numbers on all edges so that regions marked "inside" the
	// polygon have a winding number of "value", and regions outside
	// have a winding number of 0.
	//
	// If keepOnlyBoundary is TRUE, it also deletes all edges which do not
	// separate an interior region from an exterior one.
	bool SetWindingNumber(int value, bool keepOnlyBoundary);

	// Return vertex linked list head.
	inline TessVertex* GetVHead() { return &m_vHead; }
	// Return halfedge linked list head.
	inline TessHalfEdge* GetEHead() { return &m_eHead; }
	// Return face linked list head.
	inline TessFace* GetFHead() { return &m_fHead; }

private:

	// __gl_meshTessellateMonoRegion( face ) tessellates a monotone region
	// (what else would it do?)  The region must consist of a single
	// loop of half-edges (see mesh.h) oriented CCW.  "Monotone" in this
	// case means that any vertical line intersects the interior of the
	// region in a single interval.  
	//
	// Tessellation consists of adding interior edges (actually pairs of
	// half-edges), to split the region into non-overlapping triangles.
	bool TessellateMonoRegion(TessFace* face);

	void Splice(TessHalfEdge* a, TessHalfEdge* b);
	void MakeVertex(TessVertex* newVertex, TessHalfEdge* eOrig, TessVertex* vNext);
	TessHalfEdge* MakeEdge(TessHalfEdge* eNext);
	void MakeFace(TessFace* newFace, TessHalfEdge* eOrig, TessFace* fNext);
	void KillEdge(TessHalfEdge* eDel);
	void KillVertex(TessVertex* vDel, TessVertex* newOrg);
	void KillFace(TessFace* fDel, TessFace* newLface);

  TessVertex m_vHead; //dummy header for vertex list
	TessFace m_fHead; // dummy header for face list
  TessHalfEdge m_eHead; // dummy header for edge list
  TessHalfEdge m_eHeadSym; //and its symmetric counterpart

	struct EdgePair
	{
		TessHalfEdge e, eSym;
	};

	LayeredNavMesh::PoolAllocator<TessVertex, 512> m_vertPool;
	LayeredNavMesh::PoolAllocator<TessFace, 512> m_facePool;
	LayeredNavMesh::PoolAllocator<EdgePair, 512> m_edgePool;	
};

#endif
