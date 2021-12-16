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

#ifndef TESSGEOM_H
#define TESSGEOM_H

#include <assert.h>
#include "TessMesh.h"

inline bool VertEq(const TessVertex*u, const TessVertex*v)
{
	return u->x == v->x && u->y == v->y;
}

// Returns TRUE if u is lexicographically <= v.
inline bool VertLeq(const TessVertex* u, const TessVertex*v)
{
	return (u->x < v->x) || (u->x == v->x && u->y <= v->y);
}

// Versions of VertLeq, EdgeSign, EdgeEval with s and t transposed.
inline bool TransLeq(const TessVertex* u, const TessVertex* v)
{
	return (u->y < v->y) || (u->y == v->y && u->x <= v->x);
}

// Given three vertices u,v,w such that VertLeq(u,v) && VertLeq(v,w),
// evaluates the t-coord of the edge uw at the s-coord of the vertex v.
// Returns v->t - (uw)(v->s), ie. the signed distance from uw to v.
// If uw is vertical (and thus passes thru v), the result is zero.
//
// The calculation is extremely accurate and stable, even when v
// is very close to u or w.  In particular if we set v->t = 0 and
// let r be the negated result (this evaluates (uw)(v->s)), then
// r is guaranteed to satisfy MIN(u->t,w->t) <= r <= MAX(u->t,w->t).
inline float EdgeEval(const TessVertex* u, const TessVertex* v, const TessVertex* w)
{
	assert(VertLeq(u, v) && VertLeq(v, w));
	float gapL = v->x - u->x;
	float gapR = w->x - v->x;
	if (gapL + gapR > 0)
	{
		if (gapL < gapR )
			return (v->y - u->y) + (u->y - w->y) * (gapL / (gapL + gapR));
		else
			return (v->y - w->y) + (w->y - u->y) * (gapR / (gapL + gapR));
	}
	// vertical line
	return 0;
}

// Returns a number whose sign matches EdgeEval(u,v,w) but which
// is cheaper to evaluate.  Returns > 0, == 0 , or < 0
// as v is above, on, or below the edge uw.
inline float EdgeSign(const TessVertex* u, const TessVertex* v, const TessVertex* w)
{
	assert(VertLeq(u, v) && VertLeq(v, w));
	float gapL = v->x - u->x;
	float gapR = w->x - v->x;
	if (gapL + gapR > 0)
		return (v->y - w->y) * gapL + (v->y - u->y) * gapR;
	// vertical line
	return 0;
}


// Define versions of EdgeSign, EdgeEval with s and t transposed.


// Given three vertices u,v,w such that TransLeq(u,v) && TransLeq(v,w),
// evaluates the t-coord of the edge uw at the s-coord of the vertex v.
// Returns v->s - (uw)(v->t), ie. the signed distance from uw to v.
// If uw is vertical (and thus passes thru v), the result is zero.
//
// The calculation is extremely accurate and stable, even when v
// is very close to u or w.  In particular if we set v->s = 0 and
// let r be the negated result (this evaluates (uw)(v->t)), then
// r is guaranteed to satisfy MIN(u->s,w->s) <= r <= MAX(u->s,w->s).
inline float TransEval(const TessVertex* u, const TessVertex* v, const TessVertex* w)
{
	assert(TransLeq(u, v) && TransLeq(v, w));
	float gapL = v->y - u->y;
	float gapR = w->y - v->y;
	if (gapL + gapR > 0)
	{
		if (gapL < gapR)
			return (v->x - u->x) + (u->x - w->x) * (gapL / (gapL + gapR));
		else
			return (v->x - w->x) + (w->x - u->x) * (gapR / (gapL + gapR));
	}
	// vertical line
	return 0;
}

// Returns a number whose sign matches TransEval(u,v,w) but which
// is cheaper to evaluate.  Returns > 0, == 0 , or < 0
// as v is above, on, or below the edge uw.
inline float TransSign(const TessVertex* u, const TessVertex* v, const TessVertex* w)
{
  assert(TransLeq(u, v) && TransLeq(v, w));
	float gapL = v->y - u->y;
	float gapR = w->y - v->y;
	if (gapL + gapR > 0)
		return (v->x - w->x) * gapL + (v->x - u->x) * gapR;
	// vertical line 
	return 0;
}

// For almost-degenerate situations, the results are not reliable.
// Unless the floating-point arithmetic can be performed without
// rounding errors, *any* implementation will give incorrect results
// on some degenerate inputs, so the client must have some way to
// handle this situation.
inline bool VertCCW(const TessVertex* u, const TessVertex* v, const TessVertex* w)
{
  return (u->x*(v->y - w->y) + v->x*(w->y - u->y) + w->x*(u->y - v->y)) >= 0;
}

template <typename T>
inline void Swap(T& a, T& b)
{
	T t = a;
	a = b;
	b = t;
}

// Given parameters a,x,b,y returns the value (b*x+a*y)/(a+b),
// or (x+y)/2 if a==b==0.  It requires that a,b >= 0, and enforces
// this in the rare case that one argument is slightly negative.
// The implementation is extremely stable numerically.
// In particular it guarantees that the result r satisfies
// MIN(x,y) <= r <= MAX(x,y), and the results are very accurate
// even when a and b differ greatly in magnitude.
inline float Interpolate(float a, float x, float b, float y)
{
	return (a = (a < 0) ? 0 : a, b = (b < 0) ? 0 : b, ((a <= b) ? ((b == 0) ? ((x+y) / 2) : (x + (y-x) * (a/(a+b)))) : (y + (x-y) * (b/(a+b)))));
}


// Given edges (o1,d1) and (o2,d2), compute their point of intersection.
// The computed point is guaranteed to lie in the intersection of the
// bounding rectangles defined by each edge.
inline void EdgeIntersect(const TessVertex* o1, const TessVertex* d1, const TessVertex* o2, const TessVertex* d2, TessVertex* v)
{
  // This is certainly not the most efficient way to find the intersection
  // of two line segments, but it is very numerically stable.
  //
  // Strategy: find the two middle vertices in the VertLeq ordering,
  // and interpolate the intersection s-value from these.  Then repeat
  // using the TransLeq ordering to find the intersection t-value.

  if (!VertLeq(o1, d1)) { Swap(o1, d1); }
  if (!VertLeq(o2, d2)) { Swap(o2, d2); }
  if (!VertLeq(o1, o2)) { Swap(o1, o2); Swap(d1, d2); }

  if (!VertLeq(o2, d1))
	{
    // Technically, no intersection -- do our best
    v->x = (o2->x + d1->x) / 2;
  }
	else if (VertLeq(d1, d2))
	{
    // Interpolate between o2 and d1
    float z1 = EdgeEval(o1, o2, d1);
    float z2 = EdgeEval(o2, d1, d2);
    if (z1+z2 < 0) { z1 = -z1; z2 = -z2; }
    v->x = Interpolate(z1, o2->x, z2, d1->x);
  }
	else
	{
    // Interpolate between o2 and d2
    float z1 = EdgeSign(o1, o2, d1);
    float z2 = -EdgeSign(o1, d2, d1);
    if (z1+z2 < 0) { z1 = -z1; z2 = -z2; }
    v->x = Interpolate(z1, o2->x, z2, d2->x);
  }

  // Now repeat the process for t
  if (!TransLeq(o1, d1)) { Swap(o1, d1); }
  if (!TransLeq(o2, d2)) { Swap(o2, d2); }
  if (!TransLeq(o1, o2)) { Swap(o1, o2); Swap(d1, d2); }

  if (!TransLeq(o2, d1))
	{
    // Technically, no intersection -- do our best
    v->y = (o2->y + d1->y) / 2;
  }
	else if (TransLeq(d1, d2))
	{
    // Interpolate between o2 and d1
    float z1 = TransEval(o1, o2, d1);
    float z2 = TransEval(o2, d1, d2);
    if (z1+z2 < 0) { z1 = -z1; z2 = -z2; }
    v->y = Interpolate(z1, o2->y, z2, d1->y);
  }
	else
	{
    // Interpolate between o2 and d2
    float z1 = TransSign(o1, o2, d1);
    float z2 = -TransSign(o1, d2, d1);
    if (z1+z2 < 0) { z1 = -z1; z2 = -z2; }
    v->y = Interpolate(z1, o2->y, z2, d2->y);
  }
}


inline bool EdgeGoesLeft(TessHalfEdge* e)
{
	return VertLeq(e->Dst(), e->org);
}

inline bool EdgeGoesRight(TessHalfEdge* e)
{
	return VertLeq(e->org, e->Dst());
}

#endif
