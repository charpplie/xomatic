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

#include <stdio.h>

#include <assert.h>
#include "TessMesh.h"
#include "TessGeom.h"

TessMesh::TessMesh()
{
  TessVertex* v = &m_vHead;
  TessFace* f = &m_fHead;
  TessHalfEdge* e = &m_eHead;
  TessHalfEdge* eSym = &m_eHeadSym;

  v->next = v->prev = v;
  v->anEdge = 0;

  f->next = f->prev = f;
  f->anEdge = 0;
  f->marked = false;
  f->inside = false;

  e->next = e;
  e->sym = eSym;
  e->orgNext = 0;
  e->leftNext = 0;
  e->org = 0;
  e->leftFace = 0;
  e->winding = 0;
  e->activeRegion = 0;

  eSym->next = eSym;
  eSym->sym = e;
  eSym->orgNext = 0;
  eSym->leftNext = 0;
  eSym->org = 0;
  eSym->leftFace = 0;
  eSym->winding = 0;
  eSym->activeRegion = 0;
}

TessMesh::~TessMesh()
{
	// No need to free memory, the pools will free all geom.
}

// MakeEdge creates a new pair of half-edges which form their own loop.
// No vertex or face structures are allocated, but these must be assigned
// before the current edge operation is completed.
TessHalfEdge* TessMesh::MakeEdge(TessHalfEdge* eNext)
{
	EdgePair* pair = m_edgePool.Alloc(); // new EdgePair;
	if (!pair) return 0;
  TessHalfEdge* e = &pair->e;
  TessHalfEdge* eSym = &pair->eSym;
  TessHalfEdge* ePrev = 0;

  // Make sure eNext points to the first edge of the edge pair
  if (eNext->sym < eNext) { eNext = eNext->sym; }

  // Insert in circular doubly-linked list before eNext.
  // Note that the prev pointer is stored in Sym->next.
  ePrev = eNext->Sym()->next;
  eSym->next = ePrev;
  ePrev->Sym()->next = e;
  e->next = eNext;
  eNext->Sym()->next = eSym;

  e->sym = eSym;
  e->orgNext = e;
  e->leftNext = eSym;
  e->org = 0;
  e->leftFace = 0;
  e->winding = 0;
  e->activeRegion = 0;

  eSym->sym = e;
  eSym->orgNext = eSym;
  eSym->leftNext = e;
  eSym->org = 0;
  eSym->leftFace = 0;
  eSym->winding = 0;
  eSym->activeRegion = 0;

  return e;
}

// Splice( a, b ) is best described by the Guibas/Stolfi paper or the
// CS348a notes (see mesh.h).  Basically it modifies the mesh so that
// a->Onext and b->Onext are exchanged.  This can have various effects
// depending on whether a and b belong to different face or vertex rings.
// For more explanation see __gl_meshSplice() below.
void TessMesh::Splice(TessHalfEdge* a, TessHalfEdge* b)
{
	TessHalfEdge* aOnext = a->ONext();
	TessHalfEdge* bOnext = b->ONext();
	aOnext->Sym()->LNext() = b;
	bOnext->Sym()->LNext() = a;
	a->ONext() = bOnext;
	b->ONext() = aOnext;
}

// MakeVertex( newVertex, eOrig, vNext ) attaches a new vertex and makes it the
// origin of all edges in the vertex loop to which eOrig belongs. "vNext" gives
// a place to insert the new vertex in the global vertex list.  We insert
// the new vertex *before* vNext so that algorithms which walk the vertex
// list will not see the newly created vertices.
void TessMesh::MakeVertex(TessVertex* newVertex, TessHalfEdge* eOrig, TessVertex* vNext)
{
	TessVertex* vNew = newVertex;
	assert(vNew != 0);
	
	// insert in circular doubly-linked list before vNext
	TessVertex* vPrev = vNext->prev;
	vNew->prev = vPrev;
	vPrev->next = vNew;
	vNew->next = vNext;
	vNext->prev = vNew;
	
	vNew->anEdge = eOrig;
	// leave coords, undefined
	
	// fix other edges on this vertex loop
	TessHalfEdge* e = eOrig;
	do
	{
		e->org = vNew;
		e = e->ONext();
	}
	while (e != eOrig);
}

// MakeFace( newFace, eOrig, fNext ) attaches a new face and makes it the left
// face of all edges in the face loop to which eOrig belongs.  "fNext" gives
// a place to insert the new face in the global face list.  We insert
// the new face *before* fNext so that algorithms which walk the face
// list will not see the newly created faces.
void TessMesh::MakeFace(TessFace* newFace, TessHalfEdge* eOrig, TessFace* fNext)
{
	TessHalfEdge* e;
	TessFace* fPrev;
	TessFace* fNew = newFace;
	
	assert(fNew != NULL); 
	
	// insert in circular doubly-linked list before fNext
	fPrev = fNext->prev;
	fNew->prev = fPrev;
	fPrev->next = fNew;
	fNew->next = fNext;
	fNext->prev = fNew;
	
	fNew->anEdge = eOrig;
	fNew->marked = false;
	
	// The new face is marked "inside" if the old one was.  This is a
	// convenience for the common case where a face has been split in two.
	fNew->inside = fNext->inside;
	
	// fix other edges on this face loop
	e = eOrig;
	do
	{
		e->leftFace = fNew;
		e = e->LNext();
	}
	while (e != eOrig);
}

// KillEdge( eDel ) destroys an edge (the half-edges eDel and eDel->Sym),
// and removes from the global edge list.
void TessMesh::KillEdge(TessHalfEdge* eDel)
{
	// Half-edges are allocated in pairs, see EdgePair above
	if (eDel->sym < eDel) { eDel = eDel->sym; }

	// delete from circular doubly-linked list
	TessHalfEdge* eNext = eDel->next;
	TessHalfEdge*ePrev = eDel->Sym()->next;
	eNext->Sym()->next = ePrev;
	ePrev->Sym()->next = eNext;

	EdgePair* pair = (EdgePair*)eDel;
	m_edgePool.Free(pair);
//	delete pair;
}

// KillVertex( vDel ) destroys a vertex and removes it from the global
// vertex list.  It updates the vertex loop to point to a given new vertex.
void TessMesh::KillVertex(TessVertex* vDel, TessVertex* newOrg)
{
  TessHalfEdge* eStart = vDel->anEdge;

  // change the origin of all affected edges
  TessHalfEdge* e = eStart;
  do
	{
    e->org = newOrg;
    e = e->ONext();
  }
	while (e != eStart);

  // delete from circular doubly-linked list
  TessVertex* vPrev = vDel->prev;
  TessVertex* vNext = vDel->next;
  vNext->prev = vPrev;
  vPrev->next = vNext;

	m_vertPool.Free(vDel);
//	delete vDel;
}

// KillFace( fDel ) destroys a face and removes it from the global face
// list.  It updates the face loop to point to a given new face.
void TessMesh::KillFace(TessFace* fDel, TessFace* newLface)
{
  TessHalfEdge* eStart = fDel->anEdge;

  // change the left face of all affected edges
  TessHalfEdge* e = eStart;
  do
	{
    e->leftFace = newLface;
    e = e->LNext();
  }
	while (e != eStart);

  // delete from circular doubly-linked list
  TessFace* fPrev = fDel->prev;
  TessFace* fNext = fDel->next;
  fNext->prev = fPrev;
  fPrev->next = fNext;

	m_facePool.Free(fDel);
//	delete fDel;
}

// ***************** Basic Edge Operations **********************

// __gl_meshMakeEdge creates one edge, two vertices, and a loop (face).
// The loop consists of the two new half-edges.
TessHalfEdge* TessMesh::CreateEdge()
{
  TessHalfEdge* e = MakeEdge(&m_eHead);
	if (!e) return 0;
	
  TessVertex* newVertex1 = m_vertPool.Alloc(); //new Vertex; //allocVertex();
  TessVertex* newVertex2 = m_vertPool.Alloc(); //new Vertex; // allocVertex();
  TessFace* newFace = m_facePool.Alloc(); //new Face; // allocFace();

  // if any one is null then all get freed
  if (!newVertex1 || !newVertex2 || !newFace)
	{
//		delete newVertex1;
//		delete newVertex2;
//		delete newFace;
		m_vertPool.Free(newVertex1);
		m_vertPool.Free(newVertex2);
		m_facePool.Free(newFace);
		return 0;
  } 

  MakeVertex(newVertex1, e, &m_vHead);
  MakeVertex(newVertex2, e->sym, &m_vHead);
  MakeFace(newFace, e, &m_fHead);

  return e;
}


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
//
// Some special cases:
// If eDst == eOrg, the operation has no effect.
// If eDst == eOrg->Lnext, the new face will have a single edge.
// If eDst == eOrg->Lprev, the old face will have a single edge.
// If eDst == eOrg->Onext, the new vertex will have a single edge.
// If eDst == eOrg->Oprev, the old vertex will have a single edge.
bool TessMesh::SpliceEdge(TessHalfEdge* eOrg, TessHalfEdge* eDst)
{
	bool joiningLoops = false;
	bool joiningVertices = false;
	
	if (eOrg == eDst) return true;
	
	if (eDst->Org() != eOrg->Org())
	{
		// We are merging two disjoint vertices -- destroy eDst->Org
		joiningVertices = true;
		KillVertex(eDst->Org(), eOrg->Org());
	}
	if (eDst->LFace() != eOrg->LFace())
	{
		// We are connecting two disjoint loops -- destroy eDst->Lface
		joiningLoops = true;
		KillFace(eDst->LFace(), eOrg->LFace());
	}
	
	// Change the edge structure
	Splice(eDst, eOrg);
	
	if (!joiningVertices)
	{
		TessVertex* newVertex = m_vertPool.Alloc(); //new Vertex;
		if (!newVertex) return false;
		
		// We split one vertex into two -- the new vertex is eDst->Org.
		// Make sure the old vertex points to a valid half-edge.
		MakeVertex(newVertex, eDst, eOrg->Org());
		eOrg->Org()->anEdge = eOrg;
	}
	if (!joiningLoops)
	{
		TessFace* newFace = m_facePool.Alloc(); //new Face;  
		if (!newFace) return false;
		
		// We split one loop into two -- the new loop is eDst->Lface.
		// Make sure the old face points to a valid half-edge.
		MakeFace(newFace, eDst, eOrg->LFace());
		eOrg->LFace()->anEdge = eOrg;
	}
	
	return true;
}

// __gl_meshDelete( eDel ) removes the edge eDel.  There are several cases:
// if (eDel->Lface != eDel->Rface), we join two loops into one; the loop
// eDel->Lface is deleted.  Otherwise, we are splitting one loop into two;
// the newly created loop will contain eDel->Dst.  If the deletion of eDel
// would create isolated vertices, those are deleted as well.
//
// This function could be implemented as two calls to __gl_meshSplice
// plus a few calls to memFree, but this would allocate and delete
// unnecessary vertices and faces.
bool TessMesh::DeleteEdge(TessHalfEdge* eDel)
{
	TessHalfEdge* eDelSym = eDel->Sym();
	bool joiningLoops = false;
	
	// First step: disconnect the origin vertex eDel->Org.  We make all
	// changes to get a consistent mesh in this "intermediate" state.
	if (eDel->LFace() != eDel->RFace())
	{
		// We are joining two loops into one -- remove the left face
		joiningLoops = true;
		KillFace(eDel->LFace(), eDel->RFace());
	}
	
	if (eDel->ONext() == eDel)
	{
		KillVertex(eDel->Org(), 0);
	}
	else
	{
		// Make sure that eDel->Org and eDel->Rface point to valid half-edges
		eDel->RFace()->anEdge = eDel->OPrev();
		eDel->Org()->anEdge = eDel->ONext();
		
		Splice(eDel, eDel->OPrev());
		if (!joiningLoops)
		{
			TessFace* newFace = m_facePool.Alloc(); //new Face;
			if (!newFace) return false; 
			
			// We are splitting one loop into two -- create a new loop for eDel.
			MakeFace(newFace, eDel, eDel->LFace());
		}
	}
	
	// Claim: the mesh is now in a consistent state, except that eDel->Org
	// may have been deleted.  Now we disconnect eDel->Dst.
	if (eDelSym->ONext() == eDelSym)
	{
		KillVertex(eDelSym->Org(), 0);
		KillFace(eDelSym->LFace(), 0);
	}
	else
	{
		// Make sure that eDel->Dst and eDel->Lface point to valid half-edges
		eDel->LFace()->anEdge = eDelSym->OPrev();
		eDelSym->Org()->anEdge = eDelSym->ONext();
		Splice(eDelSym, eDelSym->OPrev());
	}
	
	// Any isolated vertices or faces have already been freed.
	KillEdge(eDel);
	
	return true;
}

// ******************* Other Edge Operations *********************
//
// All these routines can be implemented with the basic edge
// operations above.  They are provided for convenience and efficiency.


// __gl_meshAddEdgeVertex( eOrg ) creates a new edge eNew such that
// eNew == eOrg->Lnext, and eNew->Dst is a newly created vertex.
// eOrg and eNew will have the same left face.
TessHalfEdge* TessMesh::AddEdgeVertex(TessHalfEdge* eOrg)
{
	TessHalfEdge* eNew = MakeEdge(eOrg);
	if (!eNew) return 0;

	TessHalfEdge* eNewSym = eNew->Sym();

	// Connect the new edge appropriately
	Splice(eNew, eOrg->LNext());

	// Set the vertex and face information
	eNew->Org() = eOrg->Dst();

	TessVertex* newVertex = m_vertPool.Alloc(); //new Vertex;
	if (!newVertex) return 0;
	MakeVertex(newVertex, eNewSym, eNew->Org());

	eNew->LFace() = eNewSym->LFace() = eOrg->LFace();

	return eNew;
}

// __gl_meshSplitEdge( eOrg ) splits eOrg into two edges eOrg and eNew,
// such that eNew == eOrg->Lnext.  The new vertex is eOrg->Dst == eNew->Org.
// eOrg and eNew will have the same left face.
TessHalfEdge* TessMesh::SplitEdge(TessHalfEdge* eOrg)
{
	TessHalfEdge* tempHalfEdge = AddEdgeVertex(eOrg);
	if (!tempHalfEdge) return 0;

	TessHalfEdge* eNew = tempHalfEdge->Sym();

	// Disconnect eOrg from eOrg->Dst and connect it to eNew->Org
	Splice(eOrg->Sym(), eOrg->Sym()->OPrev());
	Splice(eOrg->Sym(), eNew);

	// Set the vertex and face information
	eOrg->Dst() = eNew->Org();
	eNew->Dst()->anEdge = eNew->Sym();	// may have pointed to eOrg->Sym
	eNew->RFace() = eOrg->RFace();
	eNew->winding = eOrg->winding;	// copy old winding information
	eNew->Sym()->winding = eOrg->Sym()->winding;

	return eNew;
}

// __gl_meshConnect( eOrg, eDst ) creates a new edge from eOrg->Dst
// to eDst->Org, and returns the corresponding half-edge eNew.
// If eOrg->Lface == eDst->Lface, this splits one loop into two,
// and the newly created loop is eNew->Lface.  Otherwise, two disjoint
// loops are merged into one, and the loop eDst->Lface is destroyed.
//
// If (eOrg == eDst), the new face will have only two edges.
// If (eOrg->Lnext == eDst), the old face is reduced to a single edge.
// If (eOrg->Lnext->Lnext == eDst), the old face is reduced to two edges.
TessHalfEdge* TessMesh::ConnectEdge(TessHalfEdge* eOrg, TessHalfEdge* eDst)
{
	bool joiningLoops = false;  
	TessHalfEdge* eNew = MakeEdge(eOrg);
	if (!eNew) return 0;
	
	TessHalfEdge* eNewSym = eNew->Sym();
	
	if (eDst->LFace() != eOrg->LFace())
	{
		// We are connecting two disjoint loops -- destroy eDst->Lface
		joiningLoops = true;
		KillFace(eDst->LFace(), eOrg->LFace());
	}
	
	// Connect the new edge appropriately
	Splice(eNew, eOrg->LNext());
	Splice(eNewSym, eDst);
	
	// Set the vertex and face information
	eNew->Org() = eOrg->Dst();
	eNewSym->Org() = eDst->Org();
	eNew->LFace() = eNewSym->LFace() = eOrg->LFace();
	
	// Make sure the old face points to a valid half-edge
	eOrg->LFace()->anEdge = eNewSym;
	
	if (!joiningLoops)
	{
		TessFace* newFace = m_facePool.Alloc(); //new Face;;
		if (!newFace) return 0;
		
		// We split one loop into two -- the new loop is eNew->Lface
		MakeFace(newFace, eNew, eOrg->LFace());
	}
	
	return eNew;
}


// ******************* Other Operations *********************

// __gl_meshZapFace( fZap ) destroys a face and removes it from the
// global face list.  All edges of fZap will have a NULL pointer as their
// left face.  Any edges which also have a NULL pointer as their right face
// are deleted entirely (along with any isolated vertices this produces).
// An entire mesh can be deleted by zapping its faces, one at a time,
// in any order.  Zapped faces cannot be used in further mesh operations!
void TessMesh::ZapFace(TessFace* fZap)
{
	TessHalfEdge* eStart = fZap->anEdge;
	
	// walk around face, deleting edges whose right face is also NULL
	TessHalfEdge* e = eStart;
	TessHalfEdge* eNext = eStart->LNext();
	do
	{
		e = eNext;
		eNext = e->LNext();
		
		e->LFace() = 0;
		if (!e->RFace())
		{
			// delete the edge -- see __gl_MeshDelete above
			if (e->ONext() == e)
			{
				KillVertex(e->Org(), 0);
			}
			else
			{
				// Make sure that e->Org points to a valid half-edge
				e->Org()->anEdge = e->ONext();
				Splice(e, e->OPrev());
			}
			TessHalfEdge* eSym = e->Sym();
			if (eSym->ONext() == eSym)
			{
				KillVertex(eSym->Org(), 0);
			}
			else
			{
				// Make sure that eSym->Org points to a valid half-edge
				eSym->Org()->anEdge = eSym->ONext();
				Splice(eSym, eSym->OPrev());
			}
			KillEdge(e);
		}
	}
	while (e != eStart);
	
	// delete from circular doubly-linked list
	TessFace* fPrev = fZap->prev;
	TessFace* fNext = fZap->next;
	fNext->prev = fPrev;
	fPrev->next = fNext;

	m_facePool.Free(fZap);
//	delete fZap;
}
	
// __gl_meshUnion( mesh1, mesh2 ) forms the union of all structures in
// both meshes, and returns the new mesh (the old meshes are destroyed).
void TessMesh::Union(TessMesh* mesh2)
{
  TessFace* f1 = &m_fHead;
  TessVertex* v1 = &m_vHead;
  TessHalfEdge* e1 = &m_eHead;
  TessFace* f2 = &mesh2->m_fHead;
  TessVertex* v2 = &mesh2->m_vHead;
  TessHalfEdge* e2 = &mesh2->m_eHead;

  // Add the faces, vertices, and edges of mesh2 to those of mesh1
  if (f2->next != f2)
	{
    f1->prev->next = f2->next;
    f2->next->prev = f1->prev;
    f2->prev->next = f1;
    f1->prev = f2->prev;
  }

  if (v2->next != v2)
	{
    v1->prev->next = v2->next;
    v2->next->prev = v1->prev;
    v2->prev->next = v1;
    v1->prev = v2->prev;
  }

  if (e2->next != e2)
	{
    e1->sym->next->sym->next = e2->next;
    e2->next->sym->next = e1->sym->next;
    e2->sym->next->sym->next = e1;
    e1->sym->next = e2->sym->next;
  }

	// Unlink mesh2.
  TessVertex* v = &mesh2->m_vHead;
  TessFace* f = &mesh2->m_fHead;
  TessHalfEdge* e = &mesh2->m_eHead;
  TessHalfEdge* eSym = &mesh2->m_eHeadSym;

  v->next = v->prev = v;
  v->anEdge = 0;

  f->next = f->prev = f;
  f->anEdge = 0;

  e->next = e;
  e->sym = eSym;
  e->orgNext = 0;
  e->leftNext = 0;
  e->org = 0;
  e->leftFace = 0;

  eSym->next = eSym;
  eSym->sym = e;
  eSym->orgNext = 0;
  eSym->leftNext = 0;
  eSym->org = 0;
  eSym->leftFace = 0;

	delete mesh2;
}

// __gl_meshCheckMesh( mesh ) checks a mesh for self-consistency.
void TessMesh::CheckMesh()
{
	TessFace* fPrev = &m_fHead;
	TessFace* f;
	for (fPrev = &m_fHead ; (f = fPrev->next) != &m_fHead; fPrev = f)
	{
		assert(f->prev == fPrev);
		TessHalfEdge* e = f->anEdge;
		do
		{
			assert(e->Sym() != e);
			assert(e->Sym()->Sym() == e);
			assert(e->LNext()->ONext()->Sym() == e);
			assert(e->ONext()->Sym()->LNext() == e);
			assert(e->LFace() == f);
			e = e->LNext();
		}
		while (e != f->anEdge);
	}
	assert(f->prev == fPrev && f->anEdge == 0); // && f->data == 0);
	
	TessVertex* vPrev = &m_vHead;
	TessVertex* v;
	for (vPrev = &m_vHead ; (v = vPrev->next) != &m_vHead; vPrev = v)
	{
		assert(v->prev == vPrev);
		TessHalfEdge* e = v->anEdge;
		do
		{
			assert(e->Sym() != e );
			assert(e->Sym()->Sym() == e );
			assert(e->LNext()->ONext()->Sym() == e );
			assert(e->ONext()->Sym()->LNext() == e);
			assert(e->Org() == v);
			e = e->ONext();
		} while (e != v->anEdge);
	}
	assert(v->prev == vPrev && v->anEdge == 0); // && v->data == 0);
	
	TessHalfEdge* ePrev = &m_eHead;
	TessHalfEdge* e;
	for (ePrev = &m_eHead ; (e = ePrev->next) != &m_eHead; ePrev = e)
	{
		assert(e->Sym()->next == ePrev->Sym());
		assert(e->Sym() != e);
		assert(e->Sym()->Sym() == e);
		assert(e->Org() != 0);
		assert(e->Dst() != 0);
		assert(e->LNext()->ONext()->Sym() == e);
		assert(e->ONext()->Sym()->LNext() == e);
	}
	assert(e->Sym()->next == ePrev->Sym()
					&& e->Sym() == &m_eHeadSym
					&& e->Sym()->Sym() == e
					&& e->Org() == 0 && e->Dst() == 0
					&& e->LFace() == 0 && e->RFace() == 0);
}

// Delete any degenerate faces with only two edges.  WalkDirtyRegions()
// will catch almost all of these, but it won't catch degenerate faces
// produced by splice operations on already-processed edges.
// The two places this can happen are in FinishLeftRegions(), when
// we splice in a "temporary" edge produced by ConnectRightVertex(),
// and in CheckForLeftSplice(), where we splice already-processed
// edges to ensure that our dictionary invariants are not violated
// by numerical errors.
//
// In both these cases it is *very* dangerous to delete the offending
// edge at the time, since one of the routines further up the stack
// will sometimes be keeping a pointer to that edge.
bool TessMesh::RemoveDegenerateFaces()
{
	TessFace* fNext = 0;
	for (TessFace* f = m_fHead.next; f != &m_fHead; f = fNext)
	{
		fNext = f->next;
		TessHalfEdge* e = f->anEdge;
		assert(e->LNext() != e);

		if (e->LNext()->LNext() == e)
		{
			// A face with only two edges
			AddWinding(e->ONext(), e);
			if (!DeleteEdge(e)) return false;
		}
	}
  return true;
}

// Remove zero-length edges, and contours with fewer than 3 vertices.
bool TessMesh::RemoveDegenerateEdges()
{
	TessHalfEdge* eNext = 0;
	for (TessHalfEdge* e = m_eHead.next; e != &m_eHead; e = eNext)
	{
		eNext = e->next;
		TessHalfEdge* eLnext = e->LNext();
		
		if (VertEq(e->Org(), e->Dst()) && e->LNext()->LNext() != e)
		{
			// Zero-length edge, contour has at least 3 edges
			//SpliceMergeVertices(eLnext, e)
			SpliceEdge(eLnext, e); // deletes e->org
			if (!DeleteEdge(e)) return false; // e is a self-loop
			e = eLnext;
			eLnext = e->LNext();
		}
		if (eLnext->LNext() == e)
		{
			// Degenerate contour (one or two edges)
			if (eLnext != e)
			{
				if (eLnext == eNext || eLnext == eNext->Sym()) { eNext = eNext->next; }
				if (!DeleteEdge(eLnext)) return false;
			}
			if (e == eNext || e == eNext->Sym()) { eNext = eNext->next; }
			if (!DeleteEdge(e)) return false;
		}
	}
	return true;
}


int CountFaceVerts(TessFace* f)
{
	TessHalfEdge* eCur = f->anEdge;
	int n = 0;
	do
	{
		n++;
		eCur = eCur->LNext();
	}
	while (eCur != f->anEdge);
	return n;
}

bool TessMesh::MergeConvexFaces(int maxVerts)
{
	int fc = 0;
	for (TessFace* f = m_fHead.next; f != &m_fHead; f = f->next)
	{
		f->marked = false;
		fc++;
	}

	for (TessFace* f = m_fHead.next; f != &m_fHead; f = f->next)
	{
		if (f->inside) // && !f->marked)
		{
			TessHalfEdge* eCur = f->anEdge;
			TessVertex* vStart = eCur->Org();

			while (true)
			{
				TessHalfEdge* eNext = eCur->LNext();
				TessHalfEdge* eSym = eCur->Sym();

				if (eSym && eSym->LFace() && eSym->LFace()->inside) // && !eSym->LFace()->marked)
				{
					int curNv = CountFaceVerts(f);
					int symNv = CountFaceVerts(eSym->LFace());
				
					if ((curNv+symNv-1) < maxVerts)
					{
						if (VertCCW(eCur->LPrev()->Org(), eCur->Org(), eSym->LNext()->LNext()->Org()) &&
								VertCCW(eSym->LPrev()->Org(), eSym->Org(), eCur->LNext()->LNext()->Org()))
						{
							eNext = eSym->LNext();
							if (!DeleteEdge(eSym))
								return false;
							eCur = 0;
						}
					}
				}
				
				if (eCur)
				{
					if (eCur->LNext()->Org() == vStart)
						break;
				}
				eCur = eNext;
			}
			
			eCur->LFace()->marked = true;
		}
	}

	return true;
}

inline float DistVertVertSq(TessVertex* u, TessVertex* v)
{
	float dx = v->x - u->x;
	float dy = v->y - u->y;
	return dx*dx + dy*dy;
}

bool TessMesh::OptimizeTriangles()
{
	int fc = 0;
	for (TessFace* f = m_fHead.next; f != &m_fHead; f = f->next)
	{
		f->marked = false;
		fc++;
	}
	
	for (int i = 0; i < 3; ++i)
	{
	
	for (TessFace* f = m_fHead.next; f != &m_fHead; f = f->next)
	{
		if (f->inside) // && !f->marked)
		{
			TessHalfEdge* eCur = f->anEdge;
			TessVertex* vStart = eCur->Org();
			
			while (true)
			{
				TessHalfEdge* eNext = eCur->LNext();
				TessHalfEdge* eSym = eCur->Sym();

				if (eSym && eSym->LFace() && eSym->LFace()->inside) // && !eSym->LFace()->marked)
				{
					if (VertCCW(eCur->LPrev()->Org(), eCur->Org(), eSym->LNext()->LNext()->Org()) &&
							VertCCW(eSym->LPrev()->Org(), eSym->Org(), eCur->LNext()->LNext()->Org()))
					{
						float distOld = DistVertVertSq(eCur->Org(), eCur->Dst());
						float distNew = DistVertVertSq(eNext->Dst(), eSym->LPrev()->Org());
						if (distNew < distOld)
						{
							TessHalfEdge* c0 = eNext;
							TessHalfEdge* c1 = eSym->LPrev();
							eNext = eSym->LNext();
							if (!DeleteEdge(eSym))
								return false;
							if (!ConnectEdge(c0, c1))
								return false;
							break;
							eCur = 0;
						}
					}
				}
				
				if (eCur)
				{
					if (eCur->LNext()->Org() == vStart)
						break;
				}
				eCur = eNext;
			}
			
			eCur->LFace()->marked = true;
		}
	}
	
	}
	
	return true;
}


// __gl_meshTessellateMonoRegion( face ) tessellates a monotone region
// (what else would it do?)  The region must consist of a single
// loop of half-edges (see mesh.h) oriented CCW.  "Monotone" in this
// case means that any vertical line intersects the interior of the
// region in a single interval.  
//
// Tessellation consists of adding interior edges (actually pairs of
// half-edges), to split the region into non-overlapping triangles.
//
// The basic idea is explained in Preparata and Shamos (which I don''t
// have handy right now), although their implementation is more
// complicated than this one.  The are two edge chains, an upper chain
// and a lower chain.  We process all vertices from both chains in order,
// from right to left.
//
// The algorithm ensures that the following invariant holds after each
// vertex is processed: the untessellated region consists of two
// chains, where one chain (say the upper) is a single edge, and
// the other chain is concave.  The left vertex of the single edge
// is always to the left of all vertices in the concave chain.
//
// Each step consists of adding the rightmost unprocessed vertex to one
// of the two chains, and forming a fan of triangles from the rightmost
// of two chain endpoints.  Determining whether we can add each triangle
// to the fan is a simple orientation test.  By making the fan as large
// as possible, we restore the invariant (check it yourself).
bool TessMesh::TessellateMonoRegion(TessFace* face)
{
	// All edges are oriented CCW around the boundary of the region.
	// First, find the half-edge whose origin vertex is rightmost.
	// Since the sweep goes from left to right, face->anEdge should
	// be close to the edge we want.
	TessHalfEdge* up = face->anEdge;
	assert(up->LNext() != up && up->LNext()->LNext() != up);
	
	while (VertLeq(up->Dst(), up->Org()))
		up = up->LPrev();
	while (VertLeq(up->Org(), up->Dst()))
		up = up->LNext();
	
	TessHalfEdge* lo = up->LPrev();
	
	while (up->LNext() != lo)
	{
		if (VertLeq(up->Dst(), lo->Org()))
		{
			// up->Dst is on the left.  It is safe to form triangles from lo->Org.
			// The EdgeGoesLeft test guarantees progress even when some triangles
			// are CW, given that the upper and lower chains are truly monotone.
			while (lo->LNext() != up &&
						(EdgeGoesLeft(lo->LNext()) || EdgeSign(lo->Org(), lo->Dst(), lo->LNext()->Dst()) <= 0))
			{
				TessHalfEdge* tempHalfEdge = ConnectEdge(lo->LNext(), lo);
				if (!tempHalfEdge) return false;
				lo = tempHalfEdge->Sym();
			}
			lo = lo->LPrev();
		}
		else
		{
			// lo->Org is on the left.  We can make CCW triangles from up->Dst.
			while (lo->LNext() != up &&
						(EdgeGoesRight(up->LPrev()) || EdgeSign(up->Dst(), up->Org(), up->LPrev()->Org()) >= 0))
			{
				TessHalfEdge* tempHalfEdge = ConnectEdge(up, up->LPrev());
				if (!tempHalfEdge) return false;
				up = tempHalfEdge->Sym();
			}
			up = up->LNext();
		}
	}
	
	// Now lo->Org == up->Dst == the leftmost vertex.  The remaining region
	// can be tessellated in a fan from this leftmost vertex.
	assert(lo->LNext() != up);
	while (lo->LNext()->LNext() != up)
	{
		TessHalfEdge* tempHalfEdge = ConnectEdge(lo->LNext(), lo);
		if (!tempHalfEdge) return false;
		lo = tempHalfEdge->Sym();
	}
	
	return true;
}

// __gl_meshTessellateInterior( mesh ) tessellates each region of
// the mesh which is marked "inside" the polygon.  Each such region
// must be monotone.
bool TessMesh::TessellateInterior()
{
  TessFace* next = 0;
  for (TessFace* f = m_fHead.next; f != &m_fHead; f = next)
	{
    // Make sure we don't try to tessellate the new triangles.
    next = f->next;
    if (f->inside)
      if (!TessellateMonoRegion(f)) return false;
  }
  return true;
}

// __gl_meshDiscardExterior( mesh ) zaps (ie. sets to NULL) all faces
// which are not marked "inside" the polygon.  Since further mesh operations
// on NULL faces are not allowed, the main purpose is to clean up the
// mesh so that exterior loops are not represented in the data structure.
void TessMesh::DiscardExterior()
{
	TessFace* next = 0;
	for (TessFace* f = m_fHead.next; f != &m_fHead; f = next)
	{
		// Since f will be destroyed, save its next pointer.
		next = f->next;
		if (!f->inside)
			ZapFace(f);
	}
}

// __gl_meshSetWindingNumber( mesh, value, keepOnlyBoundary ) resets the
// winding numbers on all edges so that regions marked "inside" the
// polygon have a winding number of "value", and regions outside
// have a winding number of 0.
//
// If keepOnlyBoundary is TRUE, it also deletes all edges which do not
// separate an interior region from an exterior one.
bool TessMesh::SetWindingNumber(int value, bool keepOnlyBoundary)
{
	TessHalfEdge* eNext = 0;
	for (TessHalfEdge*e = m_eHead.next; e != &m_eHead; e = eNext)
	{
		eNext = e->next;
		if (e->RFace()->inside != e->LFace()->inside)
		{
			// This is a boundary edge (one side is interior, one is exterior).
			e->winding = (e->LFace()->inside) ? value : -value;
		}
		else
		{
			// Both regions are interior, or both are exterior.
			if (!keepOnlyBoundary)
				e->winding = 0;
			else
				if (!DeleteEdge(e)) return false;
		}
	}
	return true;
}
