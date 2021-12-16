//
//  Voxelizer.cpp - Generates bitmap or span buffer voxelization of GeomMeshes.
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
#define _USE_MATH_DEFINES
#include <math.h>

#include "DynArray.h"
#include "NavMesh.h"
#include "VoxelBitmap.h"
#include "Matrix4.h"
#include "Vec3.h"
#include "GeomInstance.h"
#include "GeomMesh.h"
#include "Voxelizer.h"

using namespace LayeredNavMesh;

// The voxelisation algorithm below is based on the pslim source by Steve Zelinka:
//   http://graphics.cs.uiuc.edu/~zelinka/software.html
// The source code distribution did not imply any license.

//#pragma optimize ("", off)
//#pragma inline_depth (0)

template <typename T>
inline T Sqr (T val)
{
	return val*val;
}

template <typename T>
T Clamp(T val, T min, T max)
{
	if (val < min) return min;
	if (val > max) return max;
	return val;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

Voxelizer::Voxelizer(const BuildSettings& settings) :
	m_settings(settings),
	m_pSpans(0)
{
}
	
Voxelizer::~Voxelizer()
{
	delete m_pSpans;
}

bool Voxelizer::InitSpanBuffer (int * width, int * height, int * depth)
{
	if (m_pSpans)
	{
		delete m_pSpans;
		m_pSpans = 0;
	}

	const Vec3& bmin = m_settings.sceneBounds->GetAABB().Min();
	const Vec3& bmax = m_settings.sceneBounds->GetAABB().Max();
	const float voxelSizeHorz = m_settings.voxelSizeHorizontal;
	const float voxelSizeVert = m_settings.voxelSizeVertical;

	int w = (int)ceilf((bmax.x - bmin.x) / voxelSizeHorz);
	int h = (int)ceilf((bmax.y - bmin.y) / voxelSizeVert) + 1; // Allow one extra empty row above the highest geometry.
	int d = (int)ceilf((bmax.z - bmin.z) / voxelSizeHorz); 

	if (!w || !h || !d)
	{
		printf("Zero dim\n");
		return false;
	}

	m_pSpans = new SpanBuffer(w, d);

	if (width) *width = w;
	if (height) *height = h;
	if (depth) *depth = d;

	return true;
}

void Voxelizer::VoxelizeSpanBuffer(const LayeredNavMesh::DynArray<GeomInstance*>& instances)
{
	int width;
	int height;
	int depth;

	if ( ! InitSpanBuffer (&width, &height, &depth) )
		return;

	Voxelize(instances, width, height, depth);
}

void Voxelizer::Voxelize(const LayeredNavMesh::DynArray<GeomInstance*>& instances,
			int width, int height, int depth)
{
	Stopwatch total;

	int dataSize = 0;
	if (m_pSpans)
		dataSize = (m_pSpans->GetMemSize()+1023)/1024;

	printf("* Voxelizing\n");
	printf("  - size: (%d x %d x %d) %d kB... ", width, height, depth, dataSize);
	total.Start ();

	m_testPlane = 0;
	m_testTri = 0;
	m_testPass = 0;

	unsigned numFaces = 0;

	if (m_pSpans)
	{
		for (unsigned i = 0, ni = instances.Size(); i < ni; ++i)
		{
			GeomInstance* pInst = instances[i];
			VoxelizeMeshSpans(pInst->GetTM(), pInst->GetMesh(), width, height, depth, m_pSpans);

			numFaces += pInst->GetMesh()->GetFaces().Size ();
		}
	}
	total.Stop ();
	printf("  - %d faces ... ", numFaces);
	printf("  Total: %.3fms\n\n", 1000.0 * total.GetElapsed ());

/*	unsigned ncells = width*height*depth;
	
	printf("Plane tests.. %7d %5.2f%%\n", m_testPlane, (float)m_testPlane / (float)ncells * 100.0f);
	printf("Tri tests.... %7d %5.2f%%\n", m_testTri, (float)m_testTri / (float)ncells * 100.0f);
	printf("Pass......... %7d %5.2f%%\n", m_testPass, (float)m_testPass / (float)ncells * 100.0f);
	if (m_testTri)
		printf("Plane/Tri.. %5.2f%%\n", (float)m_testTri / (float)m_testPlane * 100.0f);
	if (m_testTri)
		printf("Tri/Pass... %5.2f%%\n", (float)m_testPass / (float)m_testTri * 100.0f);
	printf("\n");*/

}

// NOTE Aug 19, 2009: <pvl> not used ATM as we don't do exclusion at the triangle level (yet?)















static bool Excluded (const BuildSettings & settings, const LayeredNavMesh::Vec3 & pt)
{
	std::vector <const SceneBounds *>::const_iterator exclIt = settings.exclusionVolumes.begin ();
	const std::vector <const SceneBounds *>::const_iterator exclEnd = settings.exclusionVolumes.end ();
	for ( ; exclIt != exclEnd; ++exclIt)
		if ( (*exclIt)->Overlaps (pt) )
			return true;
	return false;
}

void Voxelizer::VoxelizeMeshSpans(const Matrix4& tm, const GeomMesh* pMesh, int w, int h, int d, SpanBuffer* pDst)
{
	static DynArray<Vec3> tverts;

	const SceneBounds * sceneBounds = m_settings.sceneBounds;
	const int agentHeight = m_settings.agentHeight;
	const float voxelSizeHorz = m_settings.voxelSizeHorizontal;
	const float voxelSizeVert = m_settings.voxelSizeVertical;
	const float voxelSizeHorzInv = 1.0f / voxelSizeHorz;
	const float voxelSizeVertInv = 1.0f / voxelSizeVert;
	const Vec3& bmin = sceneBounds->GetAABB().Min();

	const DynArray<Vec3>& verts = pMesh->GetVerts();
	const DynArray<GeomMesh::Face>& faces = pMesh->GetFaces();

	tverts.Resize(verts.Size());

	DynArray<Vec3>::Iter dst = tverts.Begin();
	for (DynArray<Vec3>::ConstIter v = verts.Begin(); v != verts.End(); ++v)
	{
		*dst = tm.TransformPoint(*v) - bmin;
		dst->x *= voxelSizeHorzInv;
		dst->y *= voxelSizeVertInv;
		dst->z *= voxelSizeHorzInv;
		dst++;
	}

//	const int stridey = pDst->GetStrideY();
//	const int stridez = pDst->GetStrideZ();
	const int lx = w - 1;
	const int ly = h - 1;
	const int lz = d - 1;

	const float epsilon = sqrtf(2.0f)/2;//sqrtf(3.0f)/2.0f;
	const float epsilonSqr = Sqr(epsilon);

	const float thr = cosf(60.0f/180.0f * (float )M_PI); //tanf(35.0f/180.0f*M_PI);

	for (DynArray<GeomMesh::Face>::ConstIter f = faces.Begin(); f != faces.End(); ++f)
	{
		if ( ! sceneBounds->Overlaps (
							tm.TransformPoint(verts[f->va]),
							tm.TransformPoint(verts[f->vb]),
							tm.TransformPoint(verts[f->vc])))
			continue;

		// NOTE Aug 19, 2009: <pvl> don't do exclusion at the level of whole triangles
		// for the time being.  For a concave exclusion volume it's not trivial
		// to check whether a triangle is fully contained within it, and I suspect
		// the costs of this checking might mean it's not worth it.  We could maybe
		// preprocess exclusion volumes to flag the convex ones and run triangle-level
		// exclusion just on those in the future.









		const Vec3& va = tverts[f->va];
		const Vec3& vb = tverts[f->vb];
		const Vec3& vc = tverts[f->vc];

		unsigned char flags = 0; 
		Vec3 norm = tm.TransformVector(f->norm);
		// TODO Feb 16, 2010: <pvl> would it be worth it to check Len2() == 1 first?
		norm.Normalize();	// NOTE Feb 15, 2010: <pvl> might need to do this if 'tm' has scaling in it
		if (norm.y > thr)
			flags = 1;

		if (pMesh->GenerateCapMesh ())
			flags |= 0x2;

		// Calculate the normal vectors for the 7 planes used to identify
		// regions of the triangular lozenge.
		
		Vec3 vab = vb - va;
		vab /= vab.LengthSqr();
		float vabOffset = -Dot(vab, va);
		
		Vec3 vbc = vc - vb;
		vbc /= vbc.LengthSqr();
		float vbcOffset = -Dot(vbc, vb);
		
		Vec3 vca = va - vc;
		vca /= vca.LengthSqr();
		float vcaOffset = -Dot(vca, vc);
		
		Vec3 normal = Cross(vab, vbc);
		normal.Normalize();
		float normalOffset = -Dot(normal, va);
		
		Vec3 vd = Cross(normal, vbc);
		vd.Normalize();
		float vdOffset = -Dot(vd, vb);
		
		Vec3 ve = Cross(normal, vca);
		ve.Normalize();
		float veOffset = -Dot(ve, vc);
		
		Vec3 vf = Cross(normal, vab);
		vf.Normalize();
		float vfOffset = -Dot(vf, va);
		
		// Construct a bounding box for the triangle.
		Vec3 tmin(va), tmax(va);
		tmin.SetMin(vb);
		tmax.SetMax(vb);
		tmin.SetMin(vc);
		tmax.SetMax(vc);

		// Inflate the bounding box by epsilon.
		const Vec3 e(epsilon, epsilon, epsilon);
		tmin -= e;
		tmax += e;
		
		// Transform the bounding box to voxel indices.
		int minx, miny, minz;
		int maxx, maxy, maxz;
		
		minx = Clamp((int)floorf(tmin.x), 0, lx);
		miny = Clamp((int)floorf(tmin.y), 0, ly);
		minz = Clamp((int)floorf(tmin.z), 0, lz);
		
		maxx = Clamp((int)floorf(tmax.x), 0, lx);
		maxy = Clamp((int)floorf(tmax.y), 0, ly);
		maxz = Clamp((int)floorf(tmax.z), 0, lz);

		tmin.x = minx + 0.5f;
		tmin.y = miny + 0.5f;
		tmin.z = minz + 0.5f;
		
		tmax.x = maxx + 0.5f;
		tmax.y = maxy + 0.5f;
		tmax.z = maxz + 0.5f;
		
		
		// Set up DDA for scanning through the bounding box.
		// The algorithm touches every voxel in the box, maintaining
		// distances to each plane.
		
		float dnormal = Dot(normal, tmin) + normalOffset;
		float dvd = Dot(vd, tmin) + vdOffset;
		float dve = Dot(ve, tmin) + veOffset;
		float dvf = Dot(vf, tmin) + vfOffset;
		float dvab = Dot(vab, tmin) + vabOffset;
		float dvbc = Dot(vbc, tmin) + vbcOffset;
		float dvca = Dot(vca, tmin) + vcaOffset;
		
		Vec3 xyz = tmin;

		for (int z = minz; z <= maxz; ++z)
		{
			float dnormal_z = dnormal;
			float dvd_z = dvd;
			float dve_z = dve;
			float dvf_z = dvf;
			float dvab_z = dvab;
			float dvbc_z = dvbc;
			float dvca_z = dvca;

			for (int x = minx; x <= maxx; ++x)
			{
				if ( ! sceneBounds->Overlaps (bmin + Vec3(voxelSizeHorz*(x + 0.5f), voxelSizeVert*0.5f, voxelSizeHorz*(z + 0.5f))))
					continue;

				// Store the originals to avoid floating point error creep.
				float dnormal_x = dnormal;
				float dvd_x = dvd;
				float dve_x = dve;
				float dvf_x = dvf;
				float dvab_x = dvab;
				float dvbc_x = dvbc;
				float dvca_x = dvca;

				int ymin = ly+1, ymax = -1;
				
				for (int y = miny; y <= maxy; ++y)
				{ 
					if ( ! Excluded (m_settings, bmin + Vec3(voxelSizeHorz*(x + 0.5f), voxelSizeVert*(y + 0.5f), voxelSizeHorz*(z + 0.5f))))
					{
						float distanceSqr = epsilonSqr*2;
						if (dnormal < epsilon && dnormal > -epsilon)
						{
							if (dvd >= 0 && dve >= 0 && dvf >= 0)
							{
								// Closest to face.
								distanceSqr = dnormal*dnormal;
							}
							else if (dvab < 0 && dvca > 1)
							{
								// Closest to vertex a.
								distanceSqr = (va - xyz).LengthSqr();
							}
							else if (dvab > 1 && dvbc < 0)
							{
								// Closest to vertex b.
								distanceSqr = (vb - xyz).LengthSqr();
							}
							else if (dvbc > 1 && dvca < 0)
							{
								// Closest to vertex c.
								distanceSqr = (vc - xyz).LengthSqr();
							}
							else if (dvab >= 0 && dvab <= 1 && dvf < 0)
							{
								// Closest to edge ab.
								distanceSqr = dnormal*dnormal + dvf*dvf;
							}
							else if (dvbc >= 0 && dvbc <= 1 && dvd < 0)
							{
								// Closest to edge bc.
								distanceSqr = dnormal*dnormal + dvd*dvd;
							}
							else if (dvca >= 0 && dvca <= 1 && dve < 0)
							{
								// Closest to edge ca.
								distanceSqr = dnormal*dnormal + dve*dve;
							}
							
							// If we're within the proper distance, mark the voxel.
							if (distanceSqr < epsilonSqr)
							{
//								pDst->AddSpan(x, z, y+1 - agentHeight, y+1);
								if (y < ymin) ymin = y;
								if (y > ymax) ymax = y;
							}
						}
					}
					
					dnormal += normal.y;
					dvd += vd.y;
					dve += ve.y;
					dvf += vf.y;
					dvab += vab.y;
					dvbc += vbc.y;
					dvca += vca.y;
					xyz.y += 1.0f;
				}

				if (ymin <= ymax)
				{
//					pDst->GetSpans(x, z);
					pDst->AddSpan(x, z, ymin+1 - agentHeight, ymax+1, flags);
				}

				dnormal = dnormal_x + normal.x;
				dvd = dvd_x + vd.x;
				dve = dve_x + ve.x;
				dvf = dvf_x + vf.x;
				dvab = dvab_x + vab.x;
				dvbc = dvbc_x + vbc.x;
				dvca = dvca_x + vca.x;
				xyz.y = tmin.y;
				xyz.x += 1.0f;
			}
			
			dnormal = dnormal_z + normal.z;
			dvd = dvd_z + vd.z;
			dve = dve_z + ve.z;
			dvf = dvf_z + vf.z;
			dvab = dvab_z + vab.z;
			dvbc = dvbc_z + vbc.z;
			dvca = dvca_z + vca.z;
			xyz.x = tmin.x;
			xyz.y = tmin.y;
			xyz.z += 1.0f;
		}
	}
}




#ifdef DEBUG_DRAW

void Voxelizer::DebugDraw(const BuildSettings& settings)
{
	if (m_pSpans)
		m_pSpans->DebugDraw(settings);
}

#endif
