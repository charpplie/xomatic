//
//  NavContourGenerator.h - Generates outline contours from nav surface.
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

#ifndef NAVCONTOURGENERATOR_H
#define NAVCONTOURGENERATOR_H

#include "Vec3.h"
#include "DynArray.h"
#include "NavMesh.h"
#include "NavSurfaceGenerator.h"
#include "ContourOptimizer.h"

namespace LayeredNavMesh {

struct Contour
{
	inline void CalcBounds()
	{
		if (points.Empty())
		{
			minx = maxx = 0;
			miny = maxy = 0;
			minz = maxz = 0;
			return;
		}
		
		minx = maxx = points.First().x;
		miny = maxy = points.First().y;
		minz = maxz = points.First().z;
		for (DynArray<Pointi>::ConstIter p = points.Begin(); p != points.End(); ++p)
		{
			if (p->x < minx) minx = p->x;
			if (p->y < miny) miny = p->y;
			if (p->z < minz) minz = p->z;
			if (p->x > maxx) maxx = p->x;
			if (p->y > maxy) maxy = p->y;
			if (p->z > maxz) maxz = p->z;
		}
	}
	
	unsigned int region;
	DynArray<Pointi> points;
	short minx, miny, minz;
	short maxx, maxy, maxz;
};

class NavContourGenerator
{
public:
	// NOTE Jan 19, 2010: <pvl> the last two params are a numerator and a denominator,
	// together building a fraction that controls tolerance in contour optimisation
	NavContourGenerator(const BuildSettings& settings, int tolNumer=3, int tolDenom=4);
	~NavContourGenerator();
	
	void BuildContours(NavSurfaceGenerator* pSurface);

	inline unsigned GetContourCount() const { return m_contours.Size(); }
	inline const Contour* GetContour(unsigned i) const { return m_contours[i]; }

#ifdef DEBUG_DRAW
	void DebugDraw(const BuildSettings& settings);
	void DebugDrawContours (
			const BuildSettings & settings,
			const LayeredNavMesh::DynArray<Contour*> & contours,
			float lineWidth
	);

	static const unsigned DEBUG_ITEM_COUNT = 5;
	DebugItem m_debugItems[DEBUG_ITEM_COUNT];
#endif

private:

	void ExtractContours(NavSurfaceGenerator* pSurface);
	void ExtractPortals(NavSurfaceGenerator* pSurface);
	void SimplifyContours();
	void SplitContours();
	bool FindContourContainingPoint(const LayeredNavMesh::DynArray<Contour*> & contours, const Pointi& pt, Contour*& cont, unsigned& );
	void SplitContour(Contour* cont, unsigned startNum, unsigned endNum,
											unsigned regA, unsigned regB, const DynArray<Pointi>& portal);
	void MergeContours(Contour* startCont, unsigned startNum, Contour* endCont, unsigned endNum,
											unsigned regA, unsigned regB, const DynArray<Pointi>& portal);
	void DebugDumpContours ();

	struct Portal
	{
		Portal(const Vec3i& pos, Cell* cell, unsigned regA, unsigned regB, unsigned char dir) :
			pos(pos), cell(cell), regA(regA), regB(regB), dir(dir), error(false) {}
		Vec3i pos;
		Cell* cell;
		unsigned regA, regB;
		unsigned char dir;
		DynArray<Pointi> points;
		bool error;
	};
	
	DynArray<Portal*> m_portals;
	DynArray<Contour*> m_rawContours;
	DynArray<Contour*> m_contours;

	DynArray<Cell*> m_temp;

	int m_optimTolNumer;
	int m_optimTolDenom;
	
	const BuildSettings m_settings;
};

} // namespace LayeredNavMesh

#endif
