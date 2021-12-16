//
//  ContourOptimizer.h - Functions to optimize contours created using boundary walk.
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

#include "ContourOptimizer.h"
#include <math.h>
#include <limits.h>
#include <float.h>

#include <stdio.h>

using namespace LayeredNavMesh;

struct Vec2i
{
	int x, y;
};

inline int Cross2D(const Vec2i& p1, const Vec2i& p2)
{
	return p1.x*p2.y - p1.y*p2.x;
}	

inline int Abs(int x) { return x >= 0 ? x : -x; }


unsigned LayeredNavMesh::OptimizeContour(const Pointi* pts, unsigned npts, int* optimized, int tolNumer, int tolDenom)
{
	// Based on potrace. http://potrace.sourceforge.net/

	int n = npts;
	int ct, dir;
	Vec2i constraint[2];
	Vec2i cur, off;

	int* corner = new int[n];
	
	const int one = tolDenom;
	const int thr = tolNumer;

	for (int i = 0; i < n-1; ++i)
	{
		ct = 0;
		
		// keep track of "directions" that have occurred
		dir = (3+3*(pts[i+1].x - pts[i].x) + (pts[i+1].z - pts[i].z)) / 2;
		ct |= (1<<dir);
		
		cur.x = (pts[i+1].x - pts[i].x)*one;
		cur.y = (pts[i+1].z - pts[i].z)*one;

		constraint[0].x = cur.x + ((cur.y >= 0 && (cur.y > 0 || cur.x < 0)) ? thr : -thr);
		constraint[0].y = cur.y + ((cur.x <= 0 && (cur.x < 0 || cur.y < 0)) ? thr : -thr);
		constraint[1].x = cur.x + ((cur.y <= 0 && (cur.y < 0 || cur.x < 0)) ? thr : -thr);
		constraint[1].y = cur.y + ((cur.x >= 0 && (cur.x > 0 || cur.y < 0)) ? thr : -thr);

		// find the next k such that no straight line from i to k
		int k = i+2;
		for (k = i+2; k < n; ++k)
		{
			int j = k-1;

			// Choose pinned vertices.
			if (pts[k].pinned & 0x8000)
			{
				break;
			}
			
			dir = (3+3*(pts[k].x - pts[j].x) + (pts[k].z - pts[j].z)) / 2;
			ct |= (1<<dir);

			// if all four "directions" have occurred, cut this path
			if (ct == 0xf)
			{
				// Found k
				break;
			}
			
			cur.x = (pts[k].x - pts[i].x)*one;
			cur.y = (pts[k].z - pts[i].z)*one;
			
			// see if current constraint is violated
			if (Cross2D(constraint[0], cur) < 0 || Cross2D(constraint[1], cur) > 0)
			{
				// Found k
				break;
			}
			
			// else, update constraint
			off.x = cur.x + ((cur.y >= 0 && (cur.y > 0 || cur.x < 0)) ? thr : -thr);
			off.y = cur.y + ((cur.x <= 0 && (cur.x < 0 || cur.y < 0)) ? thr : -thr);
			if (Cross2D(constraint[0], off) >= 0)
				constraint[0] = off;
			off.x = cur.x + ((cur.y <= 0 && (cur.y < 0 || cur.x < 0)) ? thr : -thr);
			off.y = cur.y + ((cur.x >= 0 && (cur.x > 0 || cur.y < 0)) ? thr : -thr);
			if (Cross2D(constraint[1], off) <= 0)
				constraint[1] = off;
		}
		corner[i] = k-1;
	}
	corner[n-1] = n-1;

	// Naive best path selector.
	// For each valid point on the current segment
	// find next segment that goes the furthest.
	// TODO: Implement full potrace algorithm.
	int nopt = 0;
	optimized[nopt++] = 0;
	
	for (int i = 0; i < n-1; )
	{
		int j;
		int m = corner[i];
		int mj = corner[i];
		for (j = i+1; j < corner[i]; ++j)
		{
			if (corner[j] > m)
			{
				m = corner[j];
				mj = j;
			}
		}
		optimized[nopt++] = mj;
		i = mj;
	}

	if (optimized[nopt-1] != n-1)
		optimized[nopt++] = n-1;

	return nopt;
}
