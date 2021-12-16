
#include "StdAfx.h"
#include "IRenderAuxGeom.h"

#include "DebugDraw.h"
#include "SpanBuffer.h"
#include "NavSurfaceGenerator.h"
#include "NavContourGenerator.h"
#include "NavPolygonGenerator.h"
#include "PickingAccelerator.h"
#include "GeomMesh.h"

#define _USE_MATH_DEFINES
#include <math.h>

// NOTE Apr 25, 2008: <pvl> the purpose is that this whole file goes away when
// DEBUG_DRAW_CRYENGINE is #undef'd
#ifdef DEBUG_DRAW_CRYENGINE

//#pragma optimize ("", off)
//#pragma inline_depth (0)

// NOTE Mai 8, 2008: <pvl> keep in mind that the layered navmesh preprocessor
// uses y (as opposed to z) for vertical direction, and code in this file
// needs to perform conversion back (i.e. (z,x,y)->(x,y,z), assuming that the
// first two coordinates are horizontal and the third one is vertical).


// NOTE Mai 30, 2008: <pvl> just a glorified function with four return values
struct DrawBoundaries {
	int zmin;
	int zmax;
	int xmin;
	int xmax;
	int ymin;
	int ymax;

	// NOTE Mai 30, 2008: <pvl> we compute the area to show voxelisation in as
	// an axis-aligned square with edge length of 2*range centered at a point
	// that's 'range' metres away from camera along camera view direction.
	//
	// Caller is still expected to clamp the boundaries against dimensions of
	// the voxelised area since we don't have enough information to do it here.

	// NOTE Mai 30, 2008: <pvl> range is given in world units (metres)
	DrawBoundaries (const LayeredNavMesh::BuildSettings & settings, float range)
	{
		const LayeredNavMesh::Vec3& bmin = settings.sceneBounds->GetAABB().Min();
		const float voxelSizeHorz = settings.voxelSizeHorizontal;
		const float voxelSizeVert = settings.voxelSizeVertical;

		const ::Vec3 camPos = GetIEditor()->GetRenderer()->GetCamera().GetPosition();
		const ::Vec3 camViewDir = GetIEditor()->GetRenderer()->GetCamera().GetViewdir();

		// view direction projected to horizontal plane
		::Vec3 viewDir (camViewDir.x, camViewDir.y, 0.0);
		viewDir.Normalize();

		const float centreX = camPos.x + (range * viewDir).x;
		const float centreY = camPos.y + (range * viewDir).y;
		const float centreZ = camPos.z + (range * viewDir).z;

		// loop boundaries need to be expressed in 'voxel' units (as opposed to world
		// units), hence division by voxel size
		zmin = int ((centreX - range - bmin.z) / voxelSizeHorz);
		zmax = int ((centreX + range - bmin.z) / voxelSizeHorz);
		xmin = int ((centreY - range - bmin.x) / voxelSizeHorz);
		xmax = int ((centreY + range - bmin.x) / voxelSizeHorz);
		ymin = int ((centreZ - range - bmin.y) / voxelSizeVert);
		ymax = int ((centreZ + range - bmin.y) / voxelSizeVert);
	}
};

void LayeredNavMesh::SpanBuffer::DebugDraw(const BuildSettings& settings)
{
	const Vec3& bmin = settings.sceneBounds->GetAABB().Min();
	const float voxelSizeHorz = settings.voxelSizeHorizontal;
	const float voxelSizeVert = settings.voxelSizeVertical;

	// FIXME Mai 30, 2008: <pvl> make range a cvar once it's clear how to access
	// cvars from here
	const float range = 20.0f;
	const DrawBoundaries bounds (settings, range);
	const int zmin = clamp_tpl (bounds.zmin, 0, m_height);
	const int zmax = clamp_tpl (bounds.zmax, 0, m_height);
	const int xmin = clamp_tpl (bounds.xmin, 0, m_width);
	const int xmax = clamp_tpl (bounds.xmax, 0, m_width);

	for (int z = zmin; z < zmax; ++z)
	{
		for (int x = xmin; x < xmax; ++x)
		{
			const Span* s = m_spans[x+z*m_width];
			while (s)
			{
				const float fx = bmin.z + z*voxelSizeHorz;
				const float fy = bmin.x + x*voxelSizeHorz;
				const float fz = bmin.y + (s->smin+0.5f)*voxelSizeVert;
				const float fh = (s->smax-s->smin-0.5f)*voxelSizeVert;

				ColorB col;
				if ( ! s->WalkableSurfaceOnTop())
					col = ColorB (0,255,255);
				else if (s->flags & 0x1)
					col = ColorB (255,255,255);
				else
					col = ColorB (255,0,0);

				gEnv->pRenderer->GetIRenderAuxGeom()->DrawAABB (AABB (::Vec3 (fx,fy,fz), ::Vec3 (fx+voxelSizeHorz,fy+voxelSizeHorz,fz+fh)), true, col, eBBD_Faceted);

				s = s->next;
			}
		}
	}
}



void LayeredNavMesh::NavSurfaceGenerator::DebugDraw(const BuildSettings& settings)
{
	const Vec3& bmin = settings.sceneBounds->GetAABB().Min();
	const float voxelSizeHorz = settings.voxelSizeHorizontal;
	const float voxelSizeVert = settings.voxelSizeVertical;

	// NOTE Feb 11, 2009: <pvl> to prevent z-fighting when both voxels and
	// surfaces are drawn
	const float vertOffset = 0.01f;

	bool drawSurface = m_debugItems[0].state;
	if (drawSurface)
	{
		bool drawAllCells = m_debugItems[1].state;

		const DrawBoundaries bounds (settings, 3.0f);

		for (unsigned i = 0, ni = m_cells.Size(); i < ni; ++i)
		{
			Cell& c = m_cells[i];

			if (c.x < bounds.xmin || c.x > bounds.xmax || c.z < bounds.zmin || c.z > bounds.zmax
				|| c.y < bounds.ymin || c.y > bounds.ymax)
			{
				continue;
			}

			ColorB col (0,0,0,64);
			if (c.region)
				col = IntToCol(c.region, 255/*128*/);
			else
			{
				if (!drawAllCells)
					continue;
			}

			Vec3 pos(bmin);
			pos.x += (c.x)*voxelSizeHorz;
			pos.y += (c.y)*voxelSizeVert; //(c.region*0.001f)*cs;
			pos.z += (c.z)*voxelSizeHorz;

			::Vec3 v0 (pos.z,pos.x,pos.y+vertOffset);
			::Vec3 v1 (pos.z+voxelSizeHorz,pos.x,pos.y+vertOffset);
			::Vec3 v2 (pos.z+voxelSizeHorz,pos.x+voxelSizeHorz,pos.y+vertOffset);
			::Vec3 v3 (pos.z,pos.x+voxelSizeHorz,pos.y+vertOffset);

			gEnv->pRenderer->GetIRenderAuxGeom()->DrawTriangle (v0, col, v1, col, v2, col);
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawTriangle (v0, col, v2, col, v3, col);

			// NOTE Feb 6, 2009: <pvl> annotate with region number and coordinates
			//float textCol[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			//gEnv->pRenderer->DrawLabelEx ( /*v0*/(v0+v2)/2.0f, 0.3f, & textCol[0], false, true, "%d\n(%d,%d,%d)", c.region, c.z, c.x, c.y);
		}
	}
}


/**
 * Takes a point in voxel grid coordinates and transforms it to the world.
 *
 * Watch out for the axis rotation ( (x,y,z)->(z,x,y) ) due to the up direction
 * being y (instead of z) in the layered mesh preprocessing.
 */
template <typename VoxelSpacePt>
::Vec3 ConvertToWorld (const VoxelSpacePt & pt, const LayeredNavMesh::BuildSettings & settings, float heightOffset)
{
	const LayeredNavMesh::Vec3 & bmin = settings.sceneBounds->GetAABB().Min();
	const float voxelSizeHorz = settings.voxelSizeHorizontal;
	const float voxelSizeVert = settings.voxelSizeVertical;
	::Vec3 v (bmin.z, bmin.x, bmin.y);
	v.y += pt.x * voxelSizeHorz;
	v.z += (pt.y+heightOffset) * voxelSizeVert;
	v.x += pt.z * voxelSizeHorz;
	return v;
}

void LayeredNavMesh::NavContourGenerator::DebugDrawContours (
			const BuildSettings & settings,
			const LayeredNavMesh::DynArray<Contour*> & contours,
			float lineWidth)
{
	bool drawOffset = m_debugItems[3].state;
	bool simpleColor = m_debugItems[4].state;

	float offScl = drawOffset ? 0.2f : 0.0f;

	for (unsigned i = 0, ni = contours.Size(); i < ni; ++i)
	{
		Contour* contour = contours[i];

		const float off = 0.1f + i*offScl;

		ColorB col (0,0,0,128);
		if ( ! simpleColor)
			col = IntToCol(contour->region, 255);

		if (contour->points.Empty ()) continue;

		::Vec3 v0 = ConvertToWorld (contour->points[0], settings, off);
		::Vec3 first = v0;
		::Vec3 v1;
		for (unsigned j = 0, nj = contour->points.Size(); j < nj; ++j)
		{
			v1 = ConvertToWorld (contour->points[j], settings, off);

			// NOTE Feb 6, 2009: <pvl> draw vertex index along the contour
//			float textCol[] = { 0.5f, 0.5f, 1.0f, 1.0f };
//			gEnv->pRenderer->DrawLabelEx (v1, 5.0f, textCol, false, true, "%d", j);

			gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine (v0, col, v1, col, 5.0f);

			v0 = v1;
		}
		gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine (v1, col, first, col, 5.0f);
	}
}

void LayeredNavMesh::NavContourGenerator::DebugDraw(const BuildSettings& settings)
{
	const Vec3& bmin = settings.sceneBounds->GetAABB().Min();
	const float voxelSizeHorz = settings.voxelSizeHorizontal;
	const float voxelSizeVert = settings.voxelSizeVertical;

	bool drawRaw = m_debugItems[1].state;
	if (drawRaw)
	{
		DebugDrawContours (settings, m_rawContours, 1.0f);
	}

	bool drawContours = m_debugItems[0].state;
	if (drawContours)
	{
		DebugDrawContours (settings, m_contours, 5.0f);
	}


	bool drawOffset = m_debugItems[3].state;
	float offScl = drawOffset ? 0.2f : 0.0f;

	bool drawPortals = m_debugItems[2].state;
	if (drawPortals)
	{

		for (unsigned i = 0, ni = m_portals.Size(); i < ni; ++i)
		{
			Portal* ps = m_portals[i];

			const float off = 0.1f + i*offScl;

			ColorB col = IntToCol(i, 196);

			{
				::Vec3 pos = ConvertToWorld (ps->pos, settings, off);
				gEnv->pRenderer->GetIRenderAuxGeom()->DrawPoint (pos, col, 5.0f);
			}
			{
				::Vec3 pos (bmin.z, bmin.x, bmin.y);
				pos.y += (ps->cell->x+0.5f)*voxelSizeHorz;
				pos.z += (ps->cell->y+off)*voxelSizeVert;
				pos.x += (ps->cell->z+0.5f)*voxelSizeHorz;
				gEnv->pRenderer->GetIRenderAuxGeom()->DrawPoint (pos, col, 5.0f);
			}

			if (ps->error)
				col = ColorB (255,0,0);
			else
				col = ColorB (255,255,255);

			if (!ps->points.Empty())
			{
				{
					::Vec3 pos = ConvertToWorld (ps->points.First(), settings, off);
					::Vec3 v0 (pos.x,pos.y,pos.z-0.3f);
					::Vec3 v1 (pos.x,pos.y,pos.z+0.3f);
					gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine (v0, col, v1, col, 2.0f);
					float textCol[] = { 1.0f, 0.5f, 0.5f, 1.0f };
					gEnv->pRenderer->DrawLabelEx (pos, 1.0f, & textCol[0], false, true, "%d\n%d", ps->regA, ps->regB);
				}
				{
					::Vec3 pos = ConvertToWorld (ps->points.Last(), settings, off);
					::Vec3 v0 (pos.x,pos.y,pos.z-0.3f);
					::Vec3 v1 (pos.x,pos.y,pos.z+0.3f);
					gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine (v0, col, v1, col, 2.0f);

					// NOTE Feb 6, 2009: <pvl> draw numbers of regions this portal separates
					//float textCol[] = { 0.5f, 0.5f, 1.0f, 1.0f };
					//gEnv->pRenderer->DrawLabelEx (pos, 0.5f, & textCol[0], false, true, "%d\n%d", ps->regA, ps->regB);
				}
			}


			IntToCol(i, 255);

			assert ( ! ps->points.Empty ());
			::Vec3 v0 = ConvertToWorld (ps->points[0], settings, off);
			for (unsigned j = 1, nj = ps->points.Size(); j < nj; ++j)
			{
				::Vec3 v1 = ConvertToWorld (ps->points[j], settings, off);

				gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine (v0, col, v1, col, 5.0f);

				v0 = v1;
			}		
		}	
	}

/*
	glDepthMask(GL_TRUE);
	glDisable(GL_CULL_FACE);

	bool drawPortalsProjs = false;

	if (drawPortalsProjs)
	{
		for (unsigned i = 0, ni = m_portals.Size(); i < ni; ++i)
		{
			Portal* ps = m_portals[i];

			const float off = 0.1f; //1.5f+i/10.0f;

			IntToCol(i, 255);

			if (!ps->points.Empty())
			{
				glBegin(GL_QUADS);

				Vec3 prev;

				for (unsigned j = 0, nj = ps->points.Size(); j < nj; ++j)
				{
					Pointi& p = ps->points[j];
					Vec3 pos(bmin);
					pos.x += p.x*cs;
					pos.y += (p.y+off)*cs;
					pos.z += p.z*cs;

					if (j)
					{
						glVertex3f(prev.x,prev.y-100,prev.z);
						glVertex3f(prev.x,prev.y+100,prev.z);
						glVertex3f(pos.x,pos.y+100,pos.z);
						glVertex3f(pos.x,pos.y-100,pos.z);
					}

					prev = pos;
				}

				glEnd();
			}
		}
	}

	glPointSize(1);
	glLineWidth(1);
	glEnable(GL_CULL_FACE);


	glEnable(GL_LIGHTING);
*/
}

void LayeredNavMesh::NavPolygonGenerator::DebugDraw(const BuildSettings& settings)
{
	bool drawPolys = m_debugItems[0].state;
	bool colorPolys = m_debugItems[1].state;
	bool transparent = m_debugItems[2].state;

	int a = transparent ? 128 : 255;

	if (drawPolys)
	{
		ColorB col (255,255,255,255);
		int i = 0;
		unsigned int polyIndex = 0;
		for (DynArray<NavPoly*>::Iter p = m_polys.Begin(); p != m_polys.End(); ++p, ++polyIndex)
		{
			NavPoly* poly = *p;

			if (colorPolys)
			{
				col = IntToCol(i,a,!transparent);
				i++;
			}
			else
			{
				col = IntToCol(poly->region,a,!transparent);
			}

			if (m_reachable)
			{
				if ( ! IsPolyReachable (polyIndex) )
				col = ColorB (255,0,0,255);
			}

			for (unsigned j = 1, nj = poly->numEdges-1; j < nj; ++j)
			{
				Vec3 lnm_v0 = m_verts[poly->edges[0].v];
				Vec3 lnm_v1 = m_verts[poly->edges[j].v];
				Vec3 lnm_v2 = m_verts[poly->edges[j+1].v];

				::Vec3 v0 (lnm_v0.z, lnm_v0.x, lnm_v0.y);
				::Vec3 v1 (lnm_v1.z, lnm_v1.x, lnm_v1.y);
				::Vec3 v2 (lnm_v2.z, lnm_v2.x, lnm_v2.y);

				gEnv->pRenderer->GetIRenderAuxGeom()->DrawTriangle (v0, col, v1, col, v2, col);
			}
		}
	}
}

#include "env.h"

void LayeredNavMesh::GeomMesh::DebugDraw(const LayeredNavMesh::Matrix4& tm) const
{
	//gEnv->pRenderer->GetIRenderAuxGeom()->SetRenderFlags( e_Mode3D|e_AlphaBlended|e_FillModeSolid|e_CullModeBack|e_DepthTestOn);

	const ::Vec3 camPos = GetIEditor()->GetRenderer()->GetCamera().GetPosition();
	const ::Vec3 camViewDir = GetIEditor()->GetRenderer()->GetCamera().GetViewdir();
	::Vec3 lightDir (camViewDir.x, camViewDir.y, 0.0f);
	lightDir.Normalize();
	lightDir.z = -10.0f;
	lightDir.Normalize();

	const float third = 1.0f/3.0f;
	IRenderAuxGeom * renderer = gEnv->pRenderer->GetIRenderAuxGeom();
	const ColorB baseCol (255,230,128);

	for (DynArray<GeomMesh::Face>::ConstIter f = m_faces.Begin(); f != m_faces.End(); ++f)
	{
		::Vec3 n (ConvertVec3 (tm.TransformVector(f->norm)));
		n.Normalize();	// NOTE Feb 15, 2010: <pvl> might need to do this if 'tm' has scaling in it

		const ::Vec3 v0 (ConvertVec3 (tm.TransformPoint (m_verts[f->va])));
		const ::Vec3 v1 (ConvertVec3 (tm.TransformPoint (m_verts[f->vb])));
		const ::Vec3 v2 (ConvertVec3 (tm.TransformPoint (m_verts[f->vc])));

		const ::Vec3 triCentroid ((third*v0 + third*v1 + third*v2));
		// NOTE Feb 17, 2010: <pvl> don't let the lit colour get any darker than
		// say 15% of the base colour
		const float lightCoef = std::max ((camPos-triCentroid).GetNormalized()*n, 0.15f);

		const ColorB litCol (lightCoef*baseCol.r, lightCoef*baseCol.g, lightCoef*baseCol.b);

		renderer->DrawTriangle (v0, litCol, v1, litCol, v2, litCol);
		//renderer->DrawLine (v0, baseCol, v1, baseCol, 1.0f);
		//renderer->DrawLine (v1, baseCol, v2, baseCol, 1.0f);
		//renderer->DrawLine (v2, baseCol, v0, baseCol, 1.0f);
	}
}


/**
* Draw AABB's of tree nodes.
*/
void QuadtreePickingAcceleratorBase::Node::DebugDraw (int level) const
{
	ColorB col (level*32, level*32, level*32);

	// NOTE Jul 16, 2008: <pvl> basically, draw node AABB but put the root below
	// the scene (even below zero - there still might be something drawn at 0)
	// and offset each level a bit up from the previous one (at least to prevent
	// z-fighting).  Also give the boxes some thickness so that they don't
	// disappear when looked at from the side.
	AABB aabb (Vec3 (m_aabb.min), Vec3 (m_aabb.max));
	aabb.Move (Vec3 (0.0f, 0.0f, -30.0f + level*1.5f));
	aabb.max.z += 0.1f;

	gEnv->pRenderer->GetIRenderAuxGeom()->DrawAABB (aabb, true, col, eBBD_Faceted);

	for (int i=0; i<4; ++i)
		if (m_children[i])
			m_children[i]->DebugDraw (level+1);
}

#endif // DEBUG_DRAW_CRYENGINE
