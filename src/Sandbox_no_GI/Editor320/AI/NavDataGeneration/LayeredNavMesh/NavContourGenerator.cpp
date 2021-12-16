//
//  NavContourGenerator.cpp - Generates outline contours from nav surface.
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
#include "NavSurfaceGenerator.h"
#include "NavContourGenerator.h"
#include "ContourOptimizer.h"

using namespace LayeredNavMesh;

static const int MAX_EXTRACTION_LOOPS = 1000000;


static short GetCornerHeight(const Cell* c, unsigned short dir)
{
	short y = c->y;
	unsigned short dirp = (dir+1) & 0x3;
	if (const Cell* con = c->con[dir])
	{
		y = std::max(y, con->y);
		if (const Cell* con2 = con->con[dirp])
			y = std::max(y, con2->y);
	}
	if (const Cell* con = c->con[dirp])
	{
		y = std::max(y, con->y);
		if (const Cell* con2 = con->con[dir])
			y = std::max(y, con2->y);
	}

	return y;
}



NavContourGenerator::NavContourGenerator(const BuildSettings& settings, int tolNumer, int tolDenom) :
	m_optimTolNumer (tolNumer), m_optimTolDenom (tolDenom), m_settings(settings)
{
#ifdef DEBUG_DRAW
	m_debugItems[0].state = true;
	m_debugItems[0].name = "Draw Contours";

	m_debugItems[1].state = false;
	m_debugItems[1].name = "Draw Raw Controus";

	m_debugItems[2].state = true;
	m_debugItems[2].name = "Draw Portals";

	m_debugItems[3].state = false;
	m_debugItems[3].name = "Add Offset";

	m_debugItems[4].state = false;
	m_debugItems[4].name = "Dark Color";
#endif
}

NavContourGenerator::~NavContourGenerator()
{
	for (unsigned i = 0, ni = m_rawContours.Size(); i < ni; ++i)
		delete m_rawContours[i];
	for (unsigned i = 0, ni = m_contours.Size(); i < ni; ++i)
		delete m_contours[i];
	for (unsigned i = 0, ni = m_portals.Size(); i < ni; ++i)
		delete m_portals[i];
}
	
void NavContourGenerator::BuildContours(NavSurfaceGenerator* pSurface)
{
	printf("* Process contours...\n");
	Stopwatch total;
	total.Start ();

	ExtractContours(pSurface);
	ExtractPortals(pSurface);
	SimplifyContours();
	SplitContours();

	//DebugDumpContours ();

	total.Stop ();
	printf("  Total: %.3fms\n\n", 1000.0 * total.GetElapsed ());
}

void NavContourGenerator::ExtractContours(NavSurfaceGenerator* pSurface)
{
	printf("  - Building boundaries... ");
	Stopwatch watch;
	watch.Start ();

	DynArray<Cell*> edges(pSurface->GetCellCount()/4);
	edges.Clear();

	// Find and collect edge cells.
	for (unsigned i = 0, ni = pSurface->GetCellCount(); i < ni; ++i)
	{
		Cell& c = pSurface->GetCell(i);
		
		unsigned short res = 0;
		for (unsigned j = 0; j < 4; ++j)
		{
			if (c.con[j]) // && c.region == c.con[j]->region)
				res |= (1 << j);
		}

		c.data = res ^ 0xf;	// Inverse, mark non connected edges.

		if (c.data && c.data != 0x0f)
			edges.Push(&c);
	}
	watch.Stop ();
	printf("%.3fms\n", 1000.0 * watch.GetElapsed ());


	printf("  - Extract contours... ");
	watch.Start ();
	
	DynArray<unsigned> regs(512);
	DynArray<unsigned char> dirs(512);
	DynArray<Cell*> cells(512);
	DynArray<Pointi> points(512);

	// Walk the boundaries to find the shapes.
	bool cancel = false;
	while (!edges.Empty() && !cancel)
	{
		Cell* cell = edges.Last();
		edges.Pop();
		if (cell->data == 0) continue;

		// Choose the first non-connected edge
		unsigned char dir = 0;
//		while (cell->data & (1 << dir))
		while ((cell->data & (1 << dir)) == 0) // this might be better...
			dir++;
			
		unsigned char startDir = dir;
		Cell* startCell = cell;

		Contour* contour = new Contour;
		contour->region = cell->region;

		regs.Clear();
		dirs.Clear();
		cells.Clear();
		points.Clear();

		int iter = 0;
		while (!cancel)
		{
			if (cell->data & (1 << dir))
			{
				// Choose the edge corner
				Vec3i pos(cell->x, GetCornerHeight(cell, dir), cell->z);
				switch(dir)
				{
				case 0: pos.z++; break;
				case 1: pos.x++; pos.z++; break;
				case 2: pos.x++; break;
				}

				m_temp.Push(cell);

				points.Push(Pointi(pos.x,pos.y,pos.z,0));
				cells.Push(cell);				
				regs.Push(cell->region);
				dirs.Push(dir);

				cell->data &= ~(1 << dir); // Remove visited edges
				dir = (dir+1) & 0x3;  // Rotate CW
			}
			else
			{
				if (!cell->con[dir])
				{
					printf("\nBad dir %d\n", dir);
					cancel = true;
					break;
				}
				cell = cell->con[dir];
				dir = (dir+3) & 0x3;	// Rotate CCW
			}
		
			if (startCell == cell && startDir == dir)
			{
				break;
			}

			iter++;
			if (iter > MAX_EXTRACTION_LOOPS)
			{
				printf("\nExtractContours(): Too many iterations!\n");
				cancel = true;
				break;
			}
		}

		// Mark transitions
		if (!points.Empty())
		{
			for (unsigned j = 0, nj = regs.Size(); j < nj; ++j)
			{
				unsigned ra = regs[j];
				unsigned rb = regs[(j+1) % nj];
				if (ra != rb)
				{
					unsigned char d = dirs[j];
					Cell* c = cells[j];
					Vec3i pos(c->x, GetCornerHeight(c, d), c->z);
					switch(d)
					{
					case 0: pos.z++; break;
					case 1: pos.x++; pos.z++; break;
					case 2: pos.x++; break;
					}
					
					m_portals.Push(new Portal(pos, c, ra, rb, d));
				}
			}

		
			unsigned reg = regs.Last();
			for (unsigned j = 0, nj = regs.Size(); j < nj; ++j)
			{
				unsigned r = regs[j];
				points[j].pinned = r;
				if (r != reg)
				{
					points[j].pinned |= 0x8000;
					reg = r;
				}
			}
			
			unsigned npts = points.Size();
			contour->points.Resize(npts);
			memcpy(&contour->points.First(), &points.First(), npts*sizeof(Pointi));
		
			m_rawContours.Push(contour);
		}
		else
		{
			printf("EMPTY\n");
			delete contour;
		}
	}
	watch.Stop ();
	printf("%.3fms\n", 1000.0 * watch.GetElapsed ());
}


void BuildCellMaskForPortals(Cell* c, unsigned reg)
{
	unsigned char res = 0;
	for (unsigned j = 0; j < 4; ++j)
	{
		if (!c->con[j])
		{
			res |= (1 << j);
		}
		// NOTE Feb 16, 2009: <pvl> the original version uses equality here.
		// In that case 'reg' should be 'ps->regB' and the semantics is "mark
		// any neighbour that's in 'regB' unconnected".  The current version
		// with inequality expects 'reg' to be 'ps->regA' at the call site and
		// the semantics changes to "mark any neighbour that's not in 'regA'
		// unconnected".
		// The reason for making this check broader is a hard-to-fix
		// cock-up in earlier phases of preprocessing (in region generation).
		// Ideally the invariant of a portal should be that it always separates
		// exactly two regions.  If that were the case both versions of this
		// code would give the same results (with the original one being cleaner
		// since it expresses the invariant directly).  However, the way region
		// generation works ATM makes it possible to bump into cells of a third region
		// while tracing a portal between regA and regB.  In such cases the new
		// version makes more effort towards repairing as much of the cock-up as
		// possible and is thus more robust.
		else if (c->con[j]->region != reg)
//		else if (c->con[j]->region == reg)
			res |= (1 << j);
	}
	c->data = res;
}

void NavContourGenerator::ExtractPortals(NavSurfaceGenerator* pSurface)
{
	printf("  - Extract portals... ");
	Stopwatch watch;
	watch.Start ();

	bool cancel = false;
	
	DynArray<Pointi> points(512);
	DynArray<int> optimized(64);
	
	for (unsigned i = 0; i < m_portals.Size(); )
	{
		Portal* ps = m_portals[i];
	
		Cell* cell = ps->cell;

		points.Clear();

		// Choose the first non-connected edge
		unsigned char dir = ps->dir;

		BuildCellMaskForPortals(cell, ps->regA);
//		BuildCellMaskForPortals(cell, ps->regB);
		if (cell->data)
		{
			int iter = 0;
			while (!cancel)
			{
				if (!cell)
					break;
				if (cell->data & (1 << dir))
				{
					// Choose the edge corner
					Vec3i pos(cell->x, GetCornerHeight(cell, dir), cell->z);
					switch(dir)
					{
					case 0: pos.z++; break;
					case 1: pos.x++; pos.z++; break;
					case 2: pos.x++; break;
					}
					points.Push(Pointi(pos.x, pos.y, pos.z, 0));
					cell->data &= ~(1 << dir); // Remove visited edges
					dir = (dir+1) & 0x3;  // Rotate CW
				}
				else
				{
					if (!cell->con[dir])
					{
						printf("\nBad dir %d\n", dir);
						break;
					}
					cell = cell->con[dir];
					dir = (dir+3) & 0x3;	// Rotate CCW
					BuildCellMaskForPortals(cell, ps->regA);
//					BuildCellMaskForPortals(cell, ps->regB);
				}
				
				if (cell->region != ps->regA)
					break;

				if (!cell->con[dir])
				{
					break;
				}


				iter++;
				if (iter > MAX_EXTRACTION_LOOPS)
				{
					printf("\nExtractPortals(): Too many iterations!\n");
					cancel = true;
					break;
				}
			}
		}

		if (points.Size() < 2)
		{
			printf("portal %d too few pts %d!\n", i, points.Size());
			delete ps;
			m_portals[i] = m_portals.Last();
			m_portals.Pop();
		}
		else if (points.First() == points.Last())
		{
			printf("portal %d first==last!\n", i);
			delete ps;
			m_portals[i] = m_portals.Last();
			m_portals.Pop();
		}
		else
		{
/*
			ps->points.Resize(points.Size());
			for (unsigned j = 0; j < points.Size(); ++j)
				ps->points[j] = points[j];
			++i;
*/

			unsigned npts = points.Size();
			optimized.Resize(npts);
			unsigned nopt = OptimizeContour(&points.First(), npts, &optimized.First(), m_optimTolNumer, m_optimTolDenom);
			if (nopt > 1)
			{
				ps->points.Resize(nopt);
				for (unsigned j = 0; j < nopt; ++j)
					ps->points[j] = points[optimized[j]];
				++i;
			}
			else
			{
				delete ps;
				m_portals[i] = m_portals.Last();
				m_portals.Pop();
			}

		}
	}
	
	// Remove duplicate portals.
	// TODO: Keep the better portal (less points, more straight).
	if (!m_portals.Empty())
	{
		for (unsigned i = 0; i < m_portals.Size()-1; ++i)
		{
			Portal* psi = m_portals[i];
			for (unsigned j = i+1; j < m_portals.Size(); )
			{
				Portal* psj = m_portals[j];
				if (psi->points.First() == psj->points.Last() && 
						psi->points.Last() == psj->points.First())
				{
					// Remove j
					delete psj;
					m_portals[j] = m_portals.Last();
					m_portals.Pop();
				}
				else
					++j;
			}
		}
	}
	watch.Stop ();
	printf("%.3fms\n", 1000.0 * watch.GetElapsed ());

/*
	// DEBUG dump portals
	FILE* fp = fopen("portals.txt", "w");
	if (!fp)
	{
		printf("Cannot open file!\n");
		return;
	}
		
	fprintf(fp, "%d  // # portals\n", m_portals.Size());
	for (unsigned i = 0; i < m_portals.Size(); ++i)
	{
		Portal* ps = m_portals[i];
		fprintf(fp, "%d  // # pts\n", ps->points.Size());
		for (unsigned j = 0, nj = ps->points.Size(); j < nj; ++j)
		{
			const Pointi& p = ps->points[j];
			fprintf(fp, "%d %d %d\n", p.x, p.y, p.z);
		}
	}
	fclose(fp);
*/
}


void NavContourGenerator::SimplifyContours()
{
	printf("  - Simplify %d contours... ", m_rawContours.Size());
	Stopwatch watch;
	watch.Start ();
	
	DynArray<int> optimized;
	for (unsigned i = 0, ni = m_rawContours.Size(); i < ni; ++i)
	{
		Contour* cur = m_rawContours[i];

		// make sure first and last point is the same.
		if (!(cur->points.First() == cur->points.Last()))
		{
			Pointi pt = cur->points.First();
			cur->points.Push(pt);
		}

		unsigned npts = cur->points.Size();
		optimized.Resize(npts);
		unsigned nopt = OptimizeContour(&cur->points.First(), npts, &optimized.First(), m_optimTolNumer, m_optimTolDenom);

		if (nopt > 3)
		{
			nopt--; // Discard the last duplicated point.
			Contour* opt = new Contour;
			opt->region = cur->region;
			opt->points.Resize(nopt);
			for (unsigned j = 0; j < nopt; ++j)
			{
				opt->points[j] = cur->points[optimized[j]];
			}
				
			opt->CalcBounds();
			m_contours.Push(opt);
		}
	}
	watch.Stop ();
	printf("%.3fms\n", 1000.0 * watch.GetElapsed ());
}

void NavContourGenerator::SplitContours()
{
	if (m_portals.Empty()) return;
	
	printf("  - Split contours... ");
	Stopwatch	watch;
	watch.Start ();

	// Reset point flags
	for (unsigned i = 0, ni = m_contours.Size(); i < ni; ++i)
	{
		Contour* c = m_contours[i];
		for (DynArray<Pointi>::Iter p = c->points.Begin(); p != c->points.End(); ++p)
			p->pinned &= ~0x8000;
	}
	
	for (unsigned i = 0; i < m_portals.Size(); ++i)
	{
		Portal* p = m_portals[i];
		// Find the shape that contains starting point.
		Contour* startCont;
		unsigned startContNum;
		Contour* endCont;
		unsigned endContNum;
		if (!FindContourContainingPoint(m_contours, p->points.First(), startCont, startContNum))
		{
			printf("\nPortal %d - Cannot find portal start!\n", i);
			p->error = true;
			continue;
		}
		if (!FindContourContainingPoint(m_contours, p->points.Last(), endCont, endContNum))
		{
			printf("\nPortal %d - Cannot find portal end!\n", i);
			p->error = true;
			continue;
		}

		if (startCont == endCont)
			SplitContour(startCont, startContNum, endContNum, p->regB, p->regA, p->points);
		else
			MergeContours(startCont, startContNum, endCont, endContNum, p->regB, p->regA, p->points);
	}
	watch.Stop ();
	printf("%.3fms\n", 1000.0 * watch.GetElapsed ());
}

// ATTN Feb 6, 2009: <pvl> if you use this with m_rawContours don't forget to
// calculate their bounds first!  (They aren't calculated by default.)
bool NavContourGenerator::FindContourContainingPoint(const LayeredNavMesh::DynArray<Contour*> & contours, const Pointi& pt, Contour*& cont, unsigned& pti)
{
	for (unsigned i = 0, ni = contours.Size(); i < ni; ++i)
	{
		Contour* c = contours[i];

		// Check bounds first
		if (pt.x < c->minx || pt.x > c->maxx ||
				pt.z < c->minz || pt.z > c->maxz ||
				pt.y < c->miny || pt.y > c->maxy)
				continue;

		// Find point in shape
		unsigned j = 0;
		for (DynArray<Pointi>::Iter p = c->points.Begin(); p != c->points.End(); ++p, ++j)
		{
			if (*p == pt)
			{
				cont = c;
				pti = j;
				return true;
			}
		}
	}
	
	// Not found on any shape.
	return false;
}

void NavContourGenerator::SplitContour(Contour* cont, unsigned startNum, unsigned endNum,
																				unsigned regA, unsigned regB,
																				const LayeredNavMesh::DynArray<Pointi>& portal)
{
	int n = cont->points.Size();
	int i;

	// Build new shape A
	Contour* newContA = new Contour;
	newContA->points.Resize(n/2);
	newContA->points.Clear();
	
	for (unsigned j = 0, nj = portal.Size(); j < nj; ++j)
	{
		Pointi& p = newContA->points.Push();
		p = portal[nj-1-j];
		p.pinned = regA; //regB;
	}

	i = (int)startNum+1;
	if (i >= n) i = 0;
	if (i != endNum)
	{
		do
		{
			newContA->points.Push(cont->points[i]);
			i++;
			if (i >= n) i = 0;
		}
		while (i != endNum);
	}
	newContA->CalcBounds();
	newContA->region = regA;

	// Build new shape B
	Contour* newContB = new Contour;
	newContB->points.Resize(n/2);
	newContB->points.Clear();
	
	for (unsigned j = 0, nj = portal.Size(); j < nj; ++j)
	{
		Pointi& p = newContB->points.Push();
		p = portal[j];
		p.pinned = regB; //regA;
	}
		
	i = (int)endNum+1;
	if (i >= n) i = 0;
	if (i != startNum)
	{
		do
		{
			newContB->points.Push(cont->points[i]);
			i++;
			if (i >= n) i = 0;
		}
		while (i != startNum);
	}
	newContB->CalcBounds();
	newContB->region = regB;

	// Remove cont
	int found = 0;
	for (unsigned j = 0; j < m_contours.Size(); )
	{
		if (m_contours[j] == cont)
		{
			found++;
			m_contours[j] = m_contours.Last();
			m_contours.Pop();
		}
		else
			++j;
	}
	delete cont;

	if (found != 1)
		printf("\nSplitContour: Did not delete one of the sources (found=%d).\n", found);

	if (newContA->points.Size() < 3)
		printf("\nSplitContour: newContA too few points %d\n", newContA->points.Size());
	else
		m_contours.Push(newContA);

	if (newContB->points.Size() < 3)
		printf("\nSplitContour: newContB too few points %d\n", newContB->points.Size());
	else
		m_contours.Push(newContB);
}

void NavContourGenerator::MergeContours(Contour* startCont, unsigned startNum,
																				Contour* endCont, unsigned endNum,
																				unsigned regA, unsigned regB,
																				const LayeredNavMesh::DynArray<Pointi>& portal)
{
	int ns = startCont->points.Size();
	int ne = endCont->points.Size();
	int i;

	// Build new shape A
	Contour* newCont = new Contour;
	newCont->points.Resize(ns+ne+portal.Size()-4);
	newCont->points.Clear();
	
	// Append portal points
	for (unsigned j = 0, nj = portal.Size(); j < nj; ++j)
	{
		Pointi& p = newCont->points.Push();
		p = portal[nj-1-j];
		p.pinned = regA;
	}

	// Appens start contour
	i = (int)startNum+1;
	if (i >= ns) i = 0;
	if (i != startNum)
	{
		do
		{
			newCont->points.Push(startCont->points[i]);
			i++;
			if (i >= ns) i = 0;
		}
		while (i != startNum);
	}

	// Append portal points
	for (unsigned j = 0, nj = portal.Size(); j < nj; ++j)
	{
		Pointi& p = newCont->points.Push();
		p = portal[j];
		p.pinned = regB;
	}

	// Append end contour
	i = (int)endNum+1;
	if (i >= ne) i = 0;
	if (i != endNum)
	{
		do
		{
			newCont->points.Push(endCont->points[i]);
			i++;
			if (i >= ne) i = 0;
		}
		while (i != endNum);
	}

	newCont->CalcBounds();
	newCont->region = regA;

	// Remove startCont and end Cont
	int found = 0;
	for (unsigned j = 0; j < m_contours.Size(); )
	{
		if (m_contours[j] == startCont || m_contours[j] == endCont)
		{
			found++;
			m_contours[j] = m_contours.Last();
			m_contours.Pop();
		}
		else
			++j;
	}
	delete startCont;
	delete endCont;

	if (found != 2)
		printf("\nMergeContours: Did not delete one of the sources (found=%d).\n", found);

	if (newCont->points.Size() < 3)
		printf("newCont->points.Empty()\n");

	m_contours.Push(newCont);
}

void NavContourGenerator::DebugDumpContours ()
{
	FILE* fp = fopen("contours.txt", "w");
	if (!fp)
	{
		printf("Cannot open file!\n");
		return;
	}
		
	for (unsigned i = 0; i < m_contours.Size(); ++i)
	{
		Contour * c = m_contours[i];
		for (unsigned j = 0, nj = c->points.Size(); j < nj; ++j)
		{
			const Pointi& p = c->points[j];
			fprintf(fp, "%d %d %d\n", p.x, p.y, p.z);
		}
		fprintf (fp, "\n");
	}
	fclose(fp);
}



#ifdef DEBUG_DRAW_GL

#include <SDL.h>
#include <SDL_OpenGL.h>

inline int bit(int a, int b)
{
	return (a & (1 << b)) >> b;
}

inline void IntToCol(int i, unsigned char a)
{
	int	r = bit(i, 0) + bit(i, 3) * 2 + 1;
	int	g = bit(i, 1) + bit(i, 4) * 2 + 1;
	int	b = bit(i, 2) + bit(i, 5) * 2 + 1;
	glColor4ub(255 - 63 * r, 255 - 63 * g, 255 - 63 * b, a);
}

void NavContourGenerator::DebugDraw(const BuildSettings& settings)
{
	const Vec3& bmin = settings.bmin;
	const float cs = settings.voxelSize;

	glDisable(GL_LIGHTING);
//	glDepthMask(GL_FALSE);

	glPointSize(3.0f);

	glLineWidth(1.0f);

	bool drawRaw = m_debugItems[1].state;
	bool drawOffset = m_debugItems[3].state;
	bool simpleColor = m_debugItems[4].state;

	float offScl = drawOffset ? 0.2f : 0.0f;

	if (drawRaw)
	{
		for (unsigned i = 0, ni = m_rawContours.Size(); i < ni; ++i)
		{
			Contour* contour = m_rawContours[i];

			const float off = 0.1f + i*offScl;

	/*		glBegin(GL_LINE_LOOP);
			for (unsigned j = 0, nj = contour->points.size(); j < nj; ++j)
			{
				IntToCol(contour->cells[j]->region, 196);
				Pointi& p = contour->points[j];
				Vec3 pos(orig);
				pos.x += p.x*cs;
				pos.y += (p.y+off)*cs;
				pos.z += p.z*cs;
				glVertex3fv(pos);
			}
			glEnd();*/

/*			glColor3ub(255,255,255);
			glBegin(GL_POINTS);
			for (unsigned j = 0, nj = contour->points.Size(); j < nj; ++j)
			{
				Pointi& p = contour->points[j];
				if (!p.pinned) continue;
				Vec3 pos(bmin);
				pos.x += p.x*cs;
				pos.y += (p.y+off)*cs;
				pos.z += p.z*cs;
				glVertex3fv(pos);
			}
			glEnd();*/
			
			if (simpleColor)
				glColor4ub(0,0,0,128);
			else
				IntToCol(contour->region, 255);

			glBegin(GL_LINE_LOOP);
			for (unsigned j = 0, nj = contour->points.Size(); j < nj; ++j)
			{
				Pointi& p0 = contour->points[j];
				Vec3 pos0(bmin);
				pos0.x += p0.x*cs;
				pos0.y += (p0.y+off)*cs;
				pos0.z += p0.z*cs;
				glVertex3fv(pos0);
			}
			glEnd();
		}
	}
	
/*	static double tstart = GetTimeUsec();
	double dt = GetTimeUsec() - tstart;
	unsigned omit = (unsigned)(dt) % m_contours.size();
	printf("omit=%d\n", omit);*/

	bool drawContours = m_debugItems[0].state;
	if (drawContours)
	{
		glLineWidth(2.0f);
		for (unsigned i = 0, ni = m_contours.Size(); i < ni; ++i)
		{
			Contour* contour = m_contours[i];
			const float off = 0.1f + i*offScl;

/*			if (!contour->points.Empty())
			{
				IntToCol(contour->region, 255);
				glBegin(GL_LINES);
				unsigned prev = contour->points.Size()-1;
				for (unsigned j = 0, nj = contour->points.Size(); j < nj; ++j)
				{
					Pointi& p0 = contour->points[prev];
					Pointi& p1 = contour->points[j];
					Vec3 pos0(bmin);
					Vec3 pos1(bmin);
					pos0.x += p0.x*cs;
					pos0.y += (p0.y+off)*cs;
					pos0.z += p0.z*cs;
					pos1.x += p1.x*cs;
					pos1.y += (p1.y+off)*cs;
					pos1.z += p1.z*cs;
	//				IntToCol(contour->regs[j], 255);
					glVertex3fv(pos0);
					glVertex3fv(pos1);
					prev = j;
				}
				glEnd();
			}*/

			if (simpleColor)
				glColor4ub(0,0,0,128);
			else
				IntToCol(contour->region, 255);

			glBegin(GL_LINE_LOOP);
			for (unsigned j = 0, nj = contour->points.Size(); j < nj; ++j)
			{
				Pointi& p0 = contour->points[j];
				Vec3 pos0(bmin);
				pos0.x += p0.x*cs;
				pos0.y += (p0.y+off)*cs;
				pos0.z += p0.z*cs;
				glVertex3fv(pos0);
			}
			glEnd();
		}
		glLineWidth(1.0f);

/*		glPointSize(4.0f);
		glBegin(GL_POINTS);
		for (unsigned i = 0, ni = m_contours.Size(); i < ni; ++i)
		{
			Contour* contour = m_contours[i];
			const float off = 0.1f; //1.5f + i/10.0f;
			
			for (unsigned j = 0, nj = contour->points.Size(); j < nj; ++j)
			{
				Pointi& p = contour->points[j];
				if (p.pinned)
					IntToCol(p.pinned, 196);
				else
					IntToCol(contour->region, 196);
				
				Vec3 pos(bmin);
				pos.x += p.x*cs;
				pos.y += (p.y+off)*cs;
				pos.z += p.z*cs;
				glVertex3fv(pos);
			}
		}
		glEnd();
		glPointSize(1.0f);*/

/*		glBegin(GL_POINTS);
		for (unsigned i = 0, ni = m_contours.size(); i < ni; ++i)
		{
			Contour* contour = m_contours[i];
			const float off = 1.5f + i/10.0f;
			for (unsigned j = 0, nj = contour->points.size(); j < nj; ++j)
			{
				Vec3i& p = contour->points[j];
				Vec3 pos(orig);
				pos.x += p.x*cs;
				pos.y += (p.y+off)*cs;
				pos.z += p.z*cs;
				glVertex3fv(pos);
			}
		}
		glEnd();*/
	}


/*	glBegin(GL_POINTS);
	for (unsigned i = 0, ni = m_contours.size(); i < ni; ++i)
	{
		Contour* contour = m_contours[i];
		const float off = 1.5f + i/10.0f;
		for (unsigned j = 0, nj = contour->points.size(); j < nj; ++j)
		{
			if (contour->cells[(j+nj-1)%nj]->region != contour->cells[j]->region)
				glColor3ub(255,255,255);
			else
				IntToCol(contour->regs[j], 196);

			Pointi& p = contour->points[j];
			Vec3 pos(orig);
			pos.x += p.x*cs;
			pos.y += (p.y+off)*cs;
			pos.z += p.z*cs;
			glVertex3fv(pos);
		}
	}
	glEnd();*/

	bool drawPortals = m_debugItems[2].state;
	if (drawPortals)
	{

		for (unsigned i = 0, ni = m_portals.Size(); i < ni; ++i)
		{
			Portal* ps = m_portals[i];
			
			const float off = 0.1f + i*offScl;
			
			IntToCol(i, 196);
			
			glBegin(GL_POINTS);
			{
				Vec3i& p = ps->pos;
				Vec3 pos(bmin);
				pos.x += p.x*cs;
				pos.y += (p.y+off)*cs;
				pos.z += p.z*cs;
				glVertex3fv(pos);

				pos = bmin;
				pos.x += (ps->cell->x+0.5f)*cs;
				pos.y += (ps->cell->y+off)*cs;
				pos.z += (ps->cell->z+0.5f)*cs;
				glVertex3fv(pos);
			}
			glEnd();

/*			glLineWidth(1.0f);
			glBegin(GL_LINE_STRIP);
			for (unsigned j = 0, nj = ps->points.size(); j < nj; ++j)
			{
				Pointi& p = ps->points[j];
				Vec3 pos(bmin);
				pos.x += p.x*cs;
				pos.y += (p.y+off)*cs;
				pos.z += p.z*cs;
				glVertex3fv(pos);
			}		
			glEnd();*/

			glLineWidth(2.0f);

			if (ps->error)
				glColor3ub(255,0,0);
			else
				glColor3ub(255,255,255);

			if (!ps->points.Empty())
			{
				glBegin(GL_LINES);

				{
					Pointi& p = ps->points.First();
					Vec3 pos(bmin);
					pos.x += p.x*cs;
					pos.y += (p.y+off)*cs;
					pos.z += p.z*cs;
					glVertex3f(pos.x,pos.y-0.3f,pos.z);
					glVertex3f(pos.x,pos.y+0.3f,pos.z);
				}
				{
					Pointi& p = ps->points.Last();
					Vec3 pos(bmin);
					pos.x += p.x*cs;
					pos.y += (p.y+off)*cs;
					pos.z += p.z*cs;
					glVertex3f(pos.x,pos.y-0.3f,pos.z);
					glVertex3f(pos.x,pos.y+0.3f,pos.z);
				}

				glEnd();
			}


			IntToCol(i, 255);

			glBegin(GL_LINE_STRIP);
			for (unsigned j = 0, nj = ps->points.Size(); j < nj; ++j)
			{
				Pointi& p = ps->points[j];
				Vec3 pos(bmin);
				pos.x += p.x*cs;
				pos.y += (p.y+off)*cs;
				pos.z += p.z*cs;
				glVertex3fv(pos);
			}		
			glEnd();
		}	

	}

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
	
/*
	glBegin(GL_QUADS);
	glColor3ub(255,255,255);
	for (unsigned i = 0, ni = m_temp.Size(); i < ni; ++i)
	{
		Cell* c = m_temp[i];

		Vec3 pos(bmin);
		pos.x += (c->x)*cs;
		pos.y += (c->y+1)*cs; //(c.region*0.001f)*cs;
		pos.z += (c->z)*cs;
		glVertex3f(pos.x,pos.y,pos.z);
		glVertex3f(pos.x,pos.y,pos.z+cs);
		glVertex3f(pos.x+cs,pos.y,pos.z+cs);
		glVertex3f(pos.x+cs,pos.y,pos.z);
	}
	glEnd();
*/
	glPointSize(1);
	glLineWidth(1);
	glEnable(GL_CULL_FACE);


	glEnable(GL_LIGHTING);
}

#endif
