//
//  NavPolygonGenerator.cpp - Generates polygonized navmesh from nav contours.
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

// NOTE Nov 17, 2008: <pvl> env.h needs to be included first otherwise it
// breaks compilation in presence of STLPort debug iterators
#include "env.h"

#include <stdio.h>

#include <float.h>
#include "NavMesh.h"
#include "NavContourGenerator.h"
#include "NavPolygonGenerator.h"

#include "Tess/Tess.h"

#include "BitArray.h"

using namespace LayeredNavMesh;


LayeredNavMesh::Vec3 NavPolygonGenerator::NavPolyPtr::GetCentroid () const
{
	// FIXME Jun 6, 2008: <pvl> 'sum' could overflow or lose precision for large
	// numbers of edges or large values of vertex coordinates - reformulate!

	Vec3 sum (0.0, 0.0, 0.0);

	for (unsigned i=0, numEdges = m_poly->numEdges; i < numEdges; ++i)
	{
		sum += m_owner->m_verts [m_poly->edges[i].v];
	}
	return sum / m_poly->numEdges;
}


static const int BUCKET_COUNT = (1<<12);

static inline unsigned ComputeHashBucketIndex(const LayeredNavMesh::Vec3& v)
{
	const unsigned int h1 = 0x8da6b343; // Large multiplicative constants;
	const unsigned int h2 = 0xd8163841; // here arbitrarily chosen primes
	const unsigned int h3 = 0xcb1ab31f;
	const unsigned int* vi = (const unsigned int*)&v;
	unsigned int n = h1 * vi[0] + h2 * vi[1] + h3 * vi[2];
	n = n & (BUCKET_COUNT-1);
	if (n < 0) n += BUCKET_COUNT;
	return n;
}


NavPolygonGenerator::NavPolygonGenerator(const BuildSettings& settings) :
	m_settings(settings),
	m_vertBuckets(0), m_reachable(0)
{
#ifdef DEBUG_DRAW
	m_debugItems[0].state = true;
	m_debugItems[0].name = "Draw Polygons";
	m_debugItems[1].state = true;//false;
	m_debugItems[1].name = "Color Polygons";
	m_debugItems[2].state = true;
	m_debugItems[2].name = "Transparent";
#endif
}

NavPolygonGenerator::~NavPolygonGenerator()
{
	for (DynArray<NavPoly*>::Iter p = m_polys.Begin(); p != m_polys.End(); ++p)
	{
		(*p)->Free();
		delete *p;
	}
	delete [] m_vertBuckets;
	delete m_reachable;
}
	
void NavPolygonGenerator::BuildPolygons(NavContourGenerator* pContour)
{
	m_vertBuckets = new int[BUCKET_COUNT];
	for (unsigned i = 0; i < BUCKET_COUNT; ++i)
		m_vertBuckets[i] = -1;

	printf("* Build polygons...\n");
	Stopwatch total;
	total.Start ();

	printf("  - Tesselating regions... ");
	Stopwatch watch;
	watch.Start ();

	DynArray<const Contour*> contours(pContour->GetContourCount());
	DynArray<unsigned> regs;

	for (unsigned i = 0, ni = pContour->GetContourCount(); i < ni; ++i)
	{
		const Contour* c = pContour->GetContour(i);
		bool found = false;
		for (DynArray<unsigned>::Iter r = regs.Begin(); r != regs.End(); ++r)
		{
			if (*r == c->region)
			{
				found = true;
				break;
			}
		}
		if (!found)
			regs.Push(c->region);
	}

	for (DynArray<unsigned>::Iter r = regs.Begin(); r != regs.End(); ++r)
	{
		unsigned reg = *r;
		contours.Clear();
		for (unsigned i = 0, ni = pContour->GetContourCount(); i < ni; ++i)
		{
			const Contour* c = pContour->GetContour(i);
			if (c->region == reg)
				contours.Push(c);
		}
		if (!contours.Empty())
			TessellateContours(contours, reg);
	}
	watch.Stop ();
	printf("%.3fms\n", 1000.0 * watch.GetElapsed ());

	// Transform the vertices to world space.
	for (Vec3* v = m_verts.Begin(); v != m_verts.End(); ++v)
	{
		v->x *= m_settings.voxelSizeHorizontal;
		v->y *= m_settings.voxelSizeVertical;
		v->z *= m_settings.voxelSizeHorizontal;
		*v += m_settings.sceneBounds->GetAABB().Min();
	}

	BuildAdjacency();

	printf("  - polygons: %d\n", m_polys.Size());
	printf("  - verts: %d\n", m_verts.Size());

	unsigned mem = 0;
	mem += m_verts.Size() * sizeof(Vec3);
	mem += m_polys.Size() * sizeof(NavPoly);
	for (unsigned i = 0, ni = m_polys.Size(); i < ni; ++i)
		mem += m_polys[i]->numEdges * sizeof(NavEdge);

	printf("  - mem: %d kB\n", (mem+1023)/1024);

	total.Stop ();
	printf("  Total: %.3fms\n", 1000.0 * total.GetElapsed ());
}

void NavPolygonGenerator::TessellateContours(
		const LayeredNavMesh::DynArray<const Contour*>& contours, unsigned region)
{
	Tess tess(TESS_WINDING_ODD, TESS_PROCESS_CONVEX_POLYGONS, 256);

	DynArray<float> verts(100*3);

	const float thr = 20/m_settings.voxelSizeHorizontal;

	for (DynArray<const Contour*>::ConstIter ci = contours.Begin(); ci != contours.End(); ++ci)
	{
		const Contour* c = *ci;
		unsigned npts = c->points.Size();

		verts.Clear();
		Vec3 prev(c->points.Last().x, c->points.Last().z, c->points.Last().y);

		for (unsigned j = 0; j < npts; ++j)
		{
			Vec3 cur(c->points[j].x, c->points[j].z, c->points[j].y);
			Vec3 delta = cur-prev; 
			float d = delta.Length();
			if (d < thr)
			{
				verts.Push(cur.x);
				verts.Push(cur.y);
				verts.Push(cur.z);
			}
			else
			{
				int n = 1 + (int)floorf(d/thr);
				for (int k = 0; k < n; ++k)
				{
					float u = (k+1) / (float)n;
					verts.Push(prev.x + delta.x*u);
					verts.Push(prev.y + delta.y*u);
					verts.Push(prev.z + delta.z*u);
				}
			}
			prev = cur;
		}
		
		tess.AddContour(verts.Begin(), verts.Size()/3);
	}
	if (!tess.Process())
	{
		printf("process failed!\n");
	}

	// Collect polys
	TessMesh* mesh = tess.GetMesh();
	TessFace* fHead = mesh->GetFHead();

	unsigned npts = 0;
	Vec3 pts[256];

	for (TessFace* f = fHead->next; f != fHead; f = f->next)
	{
		if (!f->inside)
			continue;

		npts = 0;
	
		TessHalfEdge* e = f->anEdge;
		do
		{
			if (npts < 256)
			{
				pts[npts].x = e->Org()->x;
				pts[npts].y = e->Org()->z;
				pts[npts].z = e->Org()->y;
				++npts;
			}
			e = e->LNext();
		}
		while (e != f->anEdge);
	
		// Add polygon.
		AddPolygon(pts, npts, region);
	}
}

void NavPolygonGenerator::AddPolygon(const LayeredNavMesh::Vec3* pts, unsigned npts, unsigned region)
{
	NavPoly* poly = new NavPoly;
	poly->region = region;

	poly->numEdges = npts;
	poly->edges = new NavEdge[npts];
	for (unsigned i = 0; i < npts; ++i)
	{
		poly->edges[i].v = AddVertex(pts[npts-1-i]);
		poly->edges[i].p = NavEdge::INVALID;
	}

	m_polys.Push(poly);
}

unsigned NavPolygonGenerator::AddVertex(const LayeredNavMesh::Vec3& v)
{
	unsigned bucket = ComputeHashBucketIndex(v);
	int i = m_vertBuckets[bucket];
	while (i != -1)
	{
		if (m_verts[i] == v)
			return (unsigned )i;
		i = m_vertNext[i];
	}

	// Could not find, create new.
	int n = (int)m_verts.Size();
	m_verts.Push(v);
	m_vertNext.Push(m_vertBuckets[bucket]);
	m_vertBuckets[bucket] = n;

	return (unsigned )n;
}


struct Edge
{
	unsigned vert[2];
	unsigned polyEdge[2];
	unsigned short poly[2];
};

void NavPolygonGenerator::BuildAdjacency()
{
	// Based on code by Eric Lengyel from:
	// http://www.terathon.com/code/edges.php

	printf("  - Build adjacency... ");
	Stopwatch watch;
	watch.Start ();

	unsigned maxEdgeCount = 0;
	for (DynArray<NavPoly*>::Iter p = m_polys.Begin(); p != m_polys.End(); ++p)
		maxEdgeCount += (*p)->numEdges;

	const unsigned vertCount = m_verts.Size();
	unsigned * const firstEdge = new unsigned [vertCount + maxEdgeCount];
	unsigned * const nextEdge = firstEdge + vertCount;

	Edge * const edges = new Edge[maxEdgeCount];

	for (unsigned a = 0; a < vertCount; a++) firstEdge[a] = 0xffff;

	// First pass over all triangles. This finds all the edges satisfying the
	// condition that the first vertex index is less than the second vertex index
	// when the direction from the first vertex to the second vertex represents
	// a counterclockwise winding around the triangle to which the edge belongs.
	// For each edge found, the edge index is stored in a linked list of edges
	// belonging to the lower-numbered vertex index i. This allows us to quickly
	// find an edge in the second pass whose higher-numbered vertex index is i.

	unsigned edgeCount = 0;

	for (unsigned i = 0, ni = m_polys.Size(); i < ni; ++i)
	{
		NavPoly* poly = m_polys[i];
		unsigned i1 = poly->edges[poly->numEdges-1].v;
		for (unsigned b = 0; b < poly->numEdges; b++)
		{
			unsigned i2 = poly->edges[b].v;
			if (i1 < i2)
			{
				Edge *edge = &edges[edgeCount];

				edge->vert[0] = (unsigned )i1;
				edge->vert[1] = (unsigned )i2;
				edge->poly[0] = (unsigned short)i;
				edge->polyEdge[0] = (unsigned )b;
				edge->poly[1] = (unsigned short)i;
				edge->polyEdge[1] = 0;

				unsigned edgeIndex = firstEdge[i1];
				if (edgeIndex == 0xffff)
				{
					firstEdge[i1] = edgeCount;
				}
				else
				{
					for (;;)
					{
						long index = nextEdge[edgeIndex];
						if (index == 0xffff)
						{
							nextEdge[edgeIndex] = edgeCount;
							break;
						}
						edgeIndex = index;
					}
				}

				nextEdge[edgeCount] = 0xffff;
				edgeCount++;
			}

			i1 = i2;
		}
	}

	// Second pass over all triangles. This finds all the edges satisfying the
	// condition that the first vertex index is greater than the second vertex index
	// when the direction from the first vertex to the second vertex represents
	// a counterclockwise winding around the triangle to which the edge belongs.
	// For each of these edges, the same edge should have already been found in
	// the first pass for a different triangle. So we search the list of edges
	// for the higher-numbered vertex index for the matching edge and fill in the
	// second triangle index. The maximum number of comparisons in this search for
	// any vertex is the number of edges having that vertex as an endpoint.

	for (unsigned i = 0, ni = m_polys.Size(); i < ni; ++i)
	{
		NavPoly* poly = m_polys[i];
		long i1 = poly->edges[poly->numEdges-1].v;
		for (unsigned b = 0; b < poly->numEdges; b++)
		{
			long i2 = poly->edges[b].v;
			if (i1 > i2)
			{
				for (unsigned edgeIndex = firstEdge[i2]; edgeIndex != 0xffff; edgeIndex = nextEdge[edgeIndex])
				{
					Edge* edge = &edges[edgeIndex];
					if (edge->vert[1] == i1 && edge->poly[0] == edge->poly[1])
					{
						edge->poly[1] = (unsigned short)i;
						edge->polyEdge[1] = (unsigned )b;
						break;
					}
				}
			}
			i1 = i2;
		}
	}

	// Store adjacency
	for (unsigned i = 0; i < edgeCount; ++i)
	{
		const Edge& e = edges[i];
		if (e.poly[0] != e.poly[1])
		{
			m_polys[e.poly[0]]->edges[e.polyEdge[0]].p = e.poly[1];
			m_polys[e.poly[1]]->edges[e.polyEdge[1]].p = e.poly[0];
		}
	}

	delete [] firstEdge;
	delete [] edges;

	watch.Stop ();
	printf("%.3fms\n", 1000.0 * watch.GetElapsed ());
}


void NavPolygonGenerator::SetPolygonReachability (const BitArray * reachability)
{
	m_reachable = reachability;
}

bool NavPolygonGenerator::IsPolyReachable (unsigned polyIdx) const
{
	if (m_reachable == 0) return true;

	return m_reachable->IsMarked (polyIdx);
}



#ifdef DEBUG_DRAW_GL

#include <SDL.h>
#include <SDL_OpenGL.h>

inline int bit(int a, int b)
{
	return (a & (1 << b)) >> b;
}

inline void IntToCol(int i, unsigned char a, bool grey = false)
{
	int	r = bit(i, 0) + bit(i, 3) * 2 + 1;
	int	g = bit(i, 1) + bit(i, 4) * 2 + 1;
	int	b = bit(i, 2) + bit(i, 5) * 2 + 1;

	r = 255 - 63*r;
	g = 255 - 63*g;
	b = 255 - 63*b;

	if (grey)
	{
		r = (240+r)/2;
		g = (240+g)/2;
		b = (240+b)/2;
	}

	glColor4ub(r, g, b, a);
}

void NavPolygonGenerator::DebugDraw(const BuildSettings& settings)
{
	glDisable(GL_LIGHTING);
//	glDepthMask(GL_FALSE);

	bool drawPolys = m_debugItems[0].state;
	bool colorPolys = m_debugItems[1].state;
	bool transparent = m_debugItems[2].state;

	int a = transparent ? 128 : 255;

	if (drawPolys)
	{
		glBegin(GL_TRIANGLES);
		glColor4ub(255,255,255,255);
		int i = 0;
		for (DynArray<NavPoly*>::Iter p = m_polys.Begin(); p != m_polys.End(); ++p)
		{
			NavPoly* poly = *p;
			
			if (colorPolys)
			{
				IntToCol(i,a,!transparent);
				i++;
			}
			else
			{
				IntToCol(poly->region,a,!transparent);
			}
			
			for (unsigned j = 1, nj = poly->numEdges-1; j < nj; ++j)
			{
				glVertex3fv(m_verts[poly->edges[0].v]);
				glVertex3fv(m_verts[poly->edges[j].v]);
				glVertex3fv(m_verts[poly->edges[j+1].v]);
			}
		}
		glEnd();
	}

	glEnable(GL_LIGHTING);
//	glDepthMask(GL_TRUE);
}

#endif
