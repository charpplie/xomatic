//
//  NavSurfaceGenerator.cpp - Generates connected suface from voxels.
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
#include "VoxelBitmap.h"
#include "NavSurfaceGenerator.h"
#include "Voxelizer.h"

#include <queue>
#include <limits>

//#pragma optimize ("", off)
//#pragma inline_depth (0)

using namespace LayeredNavMesh;

static const int BUCKET_COUNT = (1<<16); //8192 //2048

static inline int ComputeHashBucketIndex(int x, int y, int z)
{
	const int h1 = 0x8da6b343; // Large multiplicative constants;
	const int h2 = 0xd8163841; // here arbitrarily chosen primes
	const int h3 = 0xcb1ab31f;
	int n = h1 * x + h2 * y + h3 * z;
	n = n & (BUCKET_COUNT-1);
	if (n < 0) n += BUCKET_COUNT;
	return n;
}

NavSurfaceGenerator::NavSurfaceGenerator(const BuildSettings& settings) :
	m_settings(settings),
	m_pWalkable(0),
	m_cellBuckets(0)
{
	m_bounds.minx = m_bounds.miny = m_bounds.minz = 0;
	m_bounds.maxx = m_bounds.maxy = m_bounds.maxz = 0;

#ifdef DEBUG_DRAW
	m_debugItems[0].state = true;
	m_debugItems[0].name = "Draw Surface";
	m_debugItems[1].state = true;
	m_debugItems[1].name = "Draw All Cells";
#endif
}

NavSurfaceGenerator::~NavSurfaceGenerator()
{
	delete [] m_cellBuckets;
	delete m_pWalkable;
}

void NavSurfaceGenerator::BuildSurfaceFromVoxelBitmap(const VoxelBitmap* pVoxels)
{
	// This code is currently not called - an alternative method is used
	Stopwatch watch, total;

	printf("* Process walkable surface\n");
	total.Start ();

	m_pWalkable = new VoxelBitmap(*pVoxels);
	const int stridey = m_pWalkable->GetStrideY();
	const int stridez = m_pWalkable->GetStrideZ();
	const int w = m_pWalkable->GetWidth();
	const int h = m_pWalkable->GetHeight();
	const int d = m_pWalkable->GetDepth();

	m_bounds.minx = m_bounds.miny = m_bounds.minz = 0;
	m_bounds.maxx = w;
	m_bounds.maxy = h;
	m_bounds.maxz = d;

	printf("  - smear... ");
	watch.Start ();

	// Smear down the voxels by the actor height
	for (int i = 0; i < m_settings.agentHeight; ++i)
	{
		VoxelBitmap::Type* pData = m_pWalkable->GetData();
		for (int y = 0; y < h-1; ++y)
		{
			for (int z = 0; z < d; ++z)
			{
				for (int x = 0; x < stridez; ++x)
				{
					*pData |= pData[stridey];	// merge row from above
					pData++;
				}
			}
		}
	}
	watch.Stop ();
	printf("%.3fms\n", 1000.0 * watch.GetElapsed ());

	printf("  - find walkable... ");
	watch.Start ();

	if (!m_cellBuckets)
		m_cellBuckets = new Cell*[BUCKET_COUNT];
	memset(m_cellBuckets, 0, sizeof(Cell*)*BUCKET_COUNT);

	m_cells.Clear();

	VoxelBitmap::Type* pData = m_pWalkable->GetData();
	for (int y = 0; y < h-1; ++y)
	{
		for (int z = 0; z < d; ++z)
		{
			for (int x = 0; x < w; ++x)
			{
				// If the current voxel is occupied but the one above is not.
				if (pData[0] & ~pData[stridey])
				{
					Cell& c = m_cells.Push();
					c.cony[CON_XP] = -1;
					c.cony[CON_ZP] = -1;
					c.con[CON_XM] = 0;
					c.con[CON_ZM] = 0;
					c.next = 0;
					c.x = x;
					c.y = y;
					c.z = z;
					c.data = 0;
					c.userData = 0;
					c.region = 0;
					c.flags = pData[0];

					// Find next x connection.
					if (x+1 < w)
					{
						VoxelBitmap::Type* pDataAdj = pData+1;
						if (pDataAdj[0])
						{
							// Obstructed, climb up.
							int off = stridey;
							int n = m_settings.agentClimb;
							if (y+n >= h-1) n = (h-1)-y;
							for (int i = 1; i <= n; ++i, off += stridey)
							{
								if (pData[off]) break;	// Hit ceiling
								if (pDataAdj[off] == 0)	// Found opening
								{
									c.cony[CON_XP] = y+i-1;
									break;
								}
							}
						}
						else
						{
							// Unobstructed, reach down.
							int off = -stridey;
							int n = m_settings.agentClimb;
							if (y-n < 0) n = y;
							for (int i = 1; i <= n; ++i, off -= stridey)
							{
								if (pDataAdj[off])	// Hit bottom
								{
									c.cony[CON_XP] = y-i;
									break;
								}
							}
						}
					}

					// Find next z connection.
					if (z+1 < d)
					{
						VoxelBitmap::Type* pDataAdj = pData+stridez;
						if (pDataAdj[0])
						{
							// Obstructed, climb up.
							int off = stridey;
							int n = m_settings.agentClimb;
							if (y+n >= h-1) n = (h-1)-y;
							for (int i = 1; i <= n; ++i, off += stridey)
							{
								if (pData[off]) break;	// Hit ceiling
								if (pDataAdj[off] == 0)	// Found opening
								{
									c.cony[CON_ZP] = y+i-1;
									break;
								}
							}
						}
						else
						{
							// Unobstructed, reach down.
							int off = -stridey;
							int n = m_settings.agentClimb;
							if (y-n < 0) n = y;
							for (int i = 1; i <= n; ++i, off -= stridey)
							{
								if (pDataAdj[off])	// Hit bottom
								{
									c.cony[CON_ZP] = y-i;
									break;
								}
							}
						}
					}
				
				}
				++pData;
			}
		}
	}
	watch.Stop ();
	printf("%.3fms\n", 1000.0 * watch.GetElapsed ());

	printf("  - cells: %d  %dkB\n", m_cells.Size(), (m_cells.Size()*sizeof(Cell)+1023)/1024);

	printf("  - register rows... ");
	watch.Start ();

	// Register cells to buckets
	for (unsigned i = 0, ni = m_cells.Size(); i < ni; ++i)
	{
		Cell& c = m_cells[i];
		int bucket = ComputeHashBucketIndex(c.x, c.y, c.z);
		c.next = m_cellBuckets[bucket];
		m_cellBuckets[bucket] = &c;
	}

	watch.Stop ();
	printf("%.3fms\n", 1000.0 * watch.GetElapsed ());


	BuildConnections();
	DeflateBorders();
	BuildNonOverlappingRegions(w, d);

	total.Stop ();
	printf("  Total: %.3fms\n\n", 1000.0 * watch.GetElapsed ());
}


void NavSurfaceGenerator::BuildSurfaceFromSpanBuffer(const SpanBuffer* pSpans)
{
	Stopwatch watch, total;

	printf("* Process walkable surface from spans\n");
	total.Start ();

	const int w = pSpans->GetWidth();
	const int h = pSpans->GetSpanMax();
	const int d = pSpans->GetHeight();

	m_bounds.minx = m_bounds.miny = m_bounds.minz = 0;
	m_bounds.maxx = w;
	m_bounds.maxy = h;
	m_bounds.maxz = d;

	printf("  - find walkable... ");
	watch.Start ();

	if (!m_cellBuckets)
		m_cellBuckets = new Cell*[BUCKET_COUNT];
	memset(m_cellBuckets, 0, sizeof(Cell*)*BUCKET_COUNT);

	m_cells.Resize(pSpans->GetSpanCount());

	const int agentClimb = m_settings.agentClimb;
	unsigned int cc = 0;

	for (int z = 0; z < d; ++z)
	{
		for (int x = 0; x < w; ++x)
		{
			const SpanBuffer::Span* s = pSpans->GetSpans(x, z);
			while (s)
			{
				int bot = s->smax;
				int top = s->next ? s->next->smin : h;
/*
				if (top < bot)
					__asm { int 0x3 };
*/
				int step = std::min(top, bot + agentClimb);
		
				if (cc > m_cells.Size())
				{
					printf("overflow!\n");
					break;
				}

				Cell& c = m_cells[cc];
				++cc;

				c.cony[CON_XP] = -1;
				c.cony[CON_ZP] = -1;
				c.con[CON_XM] = 0;
				c.con[CON_ZM] = 0;
				c.next = 0;
				c.x = x;
				c.y = bot;
				c.z = z;
				c.data = 0;
				c.userData = 0;
				c.region = 0;
				// NOTE Jan 26, 2010: <pvl> the usefulness of the 0x2 flag ends here -
				// don't carry it over to Cell flags as it's of no use there and
				// could even interfere with comparisons against zero
				c.flags = s->flags & ~0x2;

				if ( ! s->WalkableSurfaceOnTop ())
				{
					s = s->next;
					continue;
				}

				// Find next x connection.
				if (x+1 < w)
				{
					const SpanBuffer::Span* sadj = pSpans->GetSpans(x+1, z);
					while (sadj)
					{
						if (sadj->WalkableSurfaceOnTop ())
						{
							int abot = sadj->smax;
							int atop = sadj->next ? sadj->next->smin : h;
							int astep = std::min(atop, abot + agentClimb);
							// If the step interval overlaps with the 
							if (OverlapInterval(bot, step, abot, astep))
							{
								c.cony[CON_XP] = abot;
								break;
							}
						}
						sadj = sadj->next;
					}
				}
				// Find next z connection.
				if (z+1 < d)
				{
					const SpanBuffer::Span* sadj = pSpans->GetSpans(x, z+1);
					while (sadj)
					{
						if (sadj->WalkableSurfaceOnTop ())
						{
							int abot = sadj->smax;
							int atop = sadj->next ? sadj->next->smin : h;
							int astep = std::min(atop, abot + agentClimb);
							// If the step interval overlaps with the 
							if (OverlapInterval(bot, step, abot, astep))
							{
								c.cony[CON_ZP] = abot;
								break;
							}
						}
						sadj = sadj->next;
					}
				}

				s = s->next;
			}
		}
	}
	watch.Stop ();
	printf("%.3fms\n", 1000.0 * watch.GetElapsed ());


	printf("  - cells: %d  %dkB\n", m_cells.Size(), (m_cells.Size()*sizeof(Cell)+1023)/1024);

	printf("  - register rows... ");
	watch.Start ();

	// Register cells to buckets
	for (unsigned i = 0, ni = m_cells.Size(); i < ni; ++i)
	{
		Cell& c = m_cells[i];
		int bucket = ComputeHashBucketIndex(c.x, c.y, c.z);
		c.next = m_cellBuckets[bucket];
		m_cellBuckets[bucket] = &c;
	}

	watch.Stop ();
	printf("%.3fms\n", 1000.0 * watch.GetElapsed ());


	BuildConnections();
	DeflateBorders();
	//BuildNonOverlappingRegions(w, d);
	BuildNonOverlappingRegions2(w, d);

	total.Stop ();
	printf("  Total: %.3fms\n\n", 1000.0 * total.GetElapsed ());
}

void NavSurfaceGenerator::BuildConnections()
{
	printf("  - build connections... ");
	Stopwatch watch;
	watch.Start ();

	// Build connections
	for (unsigned i = 0, ni = m_cells.Size(); i < ni; ++i)
	{
		Cell& c = m_cells[i];

		// NOTE Aug 5, 2009: <pvl> casting since on 64bit intptr_t might be wide
		// than int, resulting in a warning.  This should be safe as we can't use
		// up even int range for our 'y' coordinate.
		int xpy = int (c.cony[CON_XP]);
		int zpy = int (c.cony[CON_ZP]);

		if (xpy != -1)
		{
			c.con[CON_XP] = FindCell(c.x+1, xpy, c.z);
			if (c.con[CON_XP])
				c.con[CON_XP]->con[CON_XM] = &c;
			else
				printf("Could not find xpy\n");
		}
		else
			c.con[CON_XP] = 0;

		if (zpy != -1)
		{
			c.con[CON_ZP] = FindCell(c.x, zpy, c.z+1);
			if (c.con[CON_ZP])
				c.con[CON_ZP]->con[CON_ZM] = &c;
			else
				printf("Could not find zpy\n");
		}
		else
			c.con[CON_ZP] = 0;
	}
	watch.Stop ();
	printf("%.3fms\n", 1000.0 * watch.GetElapsed ());
}


void NavSurfaceGenerator::DeflateBorders()
{
	printf("  - deflate borders... ");
	Stopwatch watch;
	watch.Start ();

	// Mark edges
	const unsigned char dist = m_settings.agentRadius;
	for (unsigned i = 0, ni = m_cells.Size(); i < ni; ++i)
	{
		Cell& c = m_cells[i];
		if (c.flags == 0 || !c.con[0] || !c.con[1] || !c.con[2] || !c.con[3])
			c.data = dist;
	}

	for (int i = 0; i < m_settings.agentRadius-1; ++i)
	{
		// Expand the edge distance
		for (unsigned i = 0, ni = m_cells.Size(); i < ni; ++i)
		{
			Cell& c = m_cells[i];
			for (unsigned j = 0; j < 4; ++j)
			{
				if (c.con[j])
					c.data = (unsigned char)std::max((int)c.data, (int)c.con[j]->data-1);
			}
		}
	}

	// Remove links
	for (unsigned i = 0, ni = m_cells.Size(); i < ni; ++i)
	{
		Cell& c = m_cells[i];
		if (c.data != 0)
		{
			for (unsigned j = 0; j < 4; ++j)
			{
				if (c.con[j])
				{
					c.con[j]->con[(j+2)&0x3] = 0;	// Clear Opposite
					c.con[j] = 0; // Clear self
				}
			}
		}
	}
	watch.Stop ();
	printf("%.3fms\n", 1000.0 * watch.GetElapsed ());
}

class CellHeightCmp {
public:
	bool operator() (const Cell * c0, const Cell * c1) const
	{
		return c0->y > c1->y;
	}
};

#include "BitArray.h"
using LayeredNavMesh::BitArray;

class FloodFill {
	const LayeredNavMesh::DynArray<Cell> & m_cells;
	// NOTE Feb 10, 2009: <pvl> horizontal dimensions of the area covered by 'm_cells'
	unsigned mW, mD;
	// NOTE Feb 10, 2009: <pvl> one bit for each Cell - set if cell is assigned
	// to a region already.  This is meant to be primarily just optimization by
	// avoiding touching the actuall Cell memory.
	BitArray & m_assignedCells;
	// NOTE Feb 10, 2009: <pvl> one bit for each square in horizontal plane.  There
	// can be multiple Cells stacked on top of each other and the corresponding
	// bit is set if any of these has already been assigned to the region we're filling.
	BitArray m_occupied;
	BitArray m_selfOverlapFence;

	// NOTE Feb 10, 2009: <pvl> number of cells comprising the flood-filled region
	// (valid after Run() finishes)
	unsigned m_regSize;

	unsigned GetCellIndex (const Cell * ) const;
	bool Overlaps (const Cell * c, unsigned reg) const;
	bool IsOverlapInDirection (const Cell * c, unsigned dir, unsigned reg) const;
	bool IsOverlapInDiagonalDirection (const Cell * c, unsigned dir, unsigned reg) const;
	bool IsDiagonalNeighbour (const Cell * c, unsigned dir, unsigned nextDir, unsigned reg) const;
	void AddToRegion (Cell * , unsigned reg);
	void RemoveFromRegion (Cell * );
public:
	FloodFill (const LayeredNavMesh::DynArray<Cell> & cells, unsigned w, unsigned d, BitArray & assigned);

	void Run (Cell * seed, unsigned reg);

	unsigned RegSize () const;
};

FloodFill::FloodFill (const LayeredNavMesh::DynArray<Cell> & cells, unsigned w, unsigned d, BitArray & assigned) :
		m_cells (cells), mW(w), mD(d), m_assignedCells (assigned),
		m_occupied (mW * mD), m_selfOverlapFence (mW * mD), m_regSize (0)
{
}

unsigned FloodFill::GetCellIndex (const Cell * c) const
{
	// NOTE Aug 5, 2009: <pvl> casting as pointer difference is 64 bits wide on
	// x64 and compiler warns about it
	return unsigned (c - & m_cells[0]);
}

void FloodFill::AddToRegion (Cell * c, unsigned reg)
{
	assert (c->region == 0);
	assert ( ! m_occupied.IsMarked (c->x + mW * c->z));

	m_assignedCells.Mark (GetCellIndex (c));
	c->region = reg;
	m_regSize++;

	m_occupied.Mark (c->x + mW * c->z);
}

void FloodFill::RemoveFromRegion (Cell * c)
{
	assert (c->region != 0);
	assert (m_occupied.IsMarked (c->x + mW * c->z));

	m_assignedCells.Unmark (GetCellIndex (c));
	c->region = 0;
	m_regSize--;

	m_occupied.Unmark (c->x + mW * c->z);
}

// NOTE Feb 10, 2009: <pvl> checks if there's a Cell belonging to 'reg' directly
// above or below 'c'.  Takes advantage of the way Cells are stored in 'm_cells'
// where all Cells with the same horizontal coordinates are next to each other.
bool FloodFill::Overlaps (const Cell * c, unsigned reg) const
{
	const unsigned ci = GetCellIndex (c);

	for (unsigned i = ci+1; i<m_cells.Size() && m_cells[i].x == c->x && m_cells[i].z == c->z; ++i)
		if (m_cells[i].region == reg)
			return true;

	for (int i = ci-1; i>=0 && m_cells[i].x == c->x && m_cells[i].z == c->z; --i)
		if (m_cells[i].region == reg)
			return true;

	return false;
}

struct DirectionOffset {
	int dx, dz;
};
DirectionOffset dirOffsets[] = { { -1, 0 }, { 0, 1 }, { 1, 0 }, { 0, -1 } };

bool FloodFill::IsDiagonalNeighbour (const Cell * c, unsigned dir, unsigned nextDir, unsigned reg) const
{
	const Cell * neigh4conn = c->con[dir];
	if (neigh4conn == 0)
		return false;

	const Cell * neigh8conn = neigh4conn->con[nextDir];
	if (neigh8conn == 0)
		return false;

	if (neigh4conn->x<0 || neigh4conn->x>=mW || neigh4conn->z<0 || neigh4conn->z>=mD)
		return false;

	if (neigh4conn->region == 0 && m_occupied.IsMarked (neigh4conn->x + mW * neigh4conn->z))
		return false;

	if (neigh4conn->region != 0 && neigh4conn->region != reg)
		return false;

	if (neigh8conn->x<0 || neigh8conn->x>=mW || neigh8conn->z<0 || neigh8conn->z>=mD)
		return false;

	if (neigh8conn->region == 0 && m_occupied.IsMarked (neigh8conn->x + mW * neigh8conn->z))
		return false;

	if (neigh8conn->region != 0 && neigh8conn->region != reg)
		return false;

	return true;
}

bool FloodFill::IsOverlapInDiagonalDirection (const Cell * c, unsigned dir, unsigned reg) const
{
	// NOTE Feb 10, 2009: <pvl> get horizontal coordinates of the diagonal (8-connected)
	// neighbour corresponding to 'dir' (that's the one between the 4-connected
	// neighbours at 'dir' and 'dir+1%4')
	int x = c->x + dirOffsets[dir].dx;
	int z = c->z + dirOffsets[dir].dz;
	const unsigned nextDir = (dir+1) & 3;
	x += dirOffsets[nextDir].dx;
	z += dirOffsets[nextDir].dz;

	if (x<0 || x>=mW || z<0 || z>=mD)
		return false;

	// NOTE Feb 10, 2009: <pvl> if there is no Cell belonging to our region
	// at (x,z) at all then there's no danger of self-overlap
	if ( ! m_occupied.IsMarked (x + mW * z))
		return false;

//	if (m_selfOverlapFence.IsMarked (x + mW * z))
//		return true;

	// NOTE Feb 10, 2009: <pvl> there is such a cell - but there's still no
	// self-overlap if that cell is our (8-connected) neighbour
	if (IsDiagonalNeighbour (c, dir, nextDir, reg))
		return false;
	else if (IsDiagonalNeighbour (c, nextDir, dir, reg))
		return false;

	return true;
}

bool FloodFill::IsOverlapInDirection (const Cell * c, unsigned dir, unsigned reg) const
{
	int x = c->x + dirOffsets[dir].dx;
	int z = c->z + dirOffsets[dir].dz;

	if (x<0 || x>=mW || z<0 || z>=mD)
		return false;

	if (m_occupied.IsMarked (x + mW * z))
	{
		if ( ! (c->con[dir] && c->con[dir]->region == reg) )
			return true;
	}

	return false;
}

void FloodFill::Run (Cell * seed, unsigned reg)
{
	Stopwatch floodFillWatch;
	floodFillWatch.Start ();

	BitArray closedList (m_cells.Size ());

	std::priority_queue<Cell*,std::vector<Cell*>,CellHeightCmp> openList;
	openList.push (seed);

	while (!openList.empty())
	{
		Cell* c = openList.top();
		openList.pop();

		assert ( ! m_selfOverlapFence.IsMarked (c->x + mW * c->z));

		unsigned ci = GetCellIndex (c);

		assert ( ! m_assignedCells.IsMarked (ci));
		assert (c->region == 0);

		bool assignToReg = true;

		Cell * toBePushed[4];
		unsigned toBePushedCnt = 0;

		for (unsigned j = 0; j < 4; ++j)
		{
			Cell* nc = c->con[j];
			if (!nc) continue;

			if (m_selfOverlapFence.IsMarked (nc->x + mW * nc->z))
				continue;

			// NOTE Nov 10, 2008: <pvl> make sure there isn't a Cell above or below us that
			// belonged to the same region (we're building *non-overlapping* regions here!)
			const bool overlap4conn = IsOverlapInDirection (c, j, reg);
			const bool overlap8conn = IsOverlapInDiagonalDirection (c, j, reg);
			if (overlap4conn || overlap8conn)
			{
				assignToReg = false;
				break;
			}

			if (nc->region)
			{
				assert (m_assignedCells.IsMarked (GetCellIndex (nc)));
				continue; // Skip already visited
			}

			unsigned nci = GetCellIndex (nc);

			if (closedList.IsMarked (nci)) continue;

			toBePushed[toBePushedCnt++] = nc;
		}

		if (assignToReg)
		{
			// NOTE Nov 10, 2008: <pvl> mark it assigned to a region
			AddToRegion (c, reg);
			for (unsigned i=0; i < toBePushedCnt; ++i)
			{
				closedList.Mark (GetCellIndex (toBePushed[i]));
				openList.push (toBePushed[i]);
			}

			// NOTE Feb 4, 2010: <pvl> put "overlap fence" marks in neighbouring
			// (x,z) cells that this cell has no connected neighbour surface elements in.
			// Any elements in those cells can't be part of the current region as
			// that would ultimately lead to a self-touching region contour.
			for (unsigned dir=0; dir<4; ++dir)
			{
				int x = c->x + dirOffsets[dir].dx;
				int z = c->z + dirOffsets[dir].dz;
				if ( ! c->con[dir] )
				{
					m_selfOverlapFence.Mark (x + mW * z);
				}
				const unsigned nextDir = (dir+1) & 3;
				if ( ! (c->con[dir] && c->con[dir]->con[nextDir] || c->con[nextDir] && c->con[nextDir]->con[dir]) )
				{
					x += dirOffsets[nextDir].dx;
					z += dirOffsets[nextDir].dz;
					m_selfOverlapFence.Mark (x + mW * z);
				}
			}
		}
		else
		{
			assert (c->region == 0);
			assert (closedList.IsMarked (ci));
			m_selfOverlapFence.Mark (c->x + mW * c->z);
		}
	}

	floodFillWatch.Stop ();
	//printf ("    (... region %d (%d cells) flood-filled in: %.3fms\n", reg, m_regSize, 1000.0 * floodFillWatch.GetElapsed ());
}

unsigned FloodFill::RegSize () const
{
	return m_regSize;
}

void RemoveOrphans (const LayeredNavMesh::DynArray<Cell*> & orphans)
{
	LayeredNavMesh::DynArray<Cell*> openList;
	LayeredNavMesh::DynArray<Cell*> detach;

	for (unsigned i = 0, ni = orphans.Size(); i < ni; ++i)
	{
		Cell* seed = orphans[i];

		openList.Clear();
		openList.Push(seed);

		unsigned int reg = seed->region;

		seed->region = 0;

		detach.Clear();

		while (!openList.Empty())
		{
			Cell* c = openList.Last();
			openList.Pop();

			detach.Push(c);

			for (unsigned j = 0; j < 4; ++j)
			{
				Cell* nc = c->con[j];
				if (!nc) continue;
				if (nc->region != reg) continue; // Skip already visited
				nc->region = 0;
				openList.Push(nc);
			}
		}

		for (unsigned j = 0, nj = detach.Size(); j < nj; ++j)
		{
			Cell* c = detach[j];
			c->data = 0;
			for (unsigned k = 0; k < 4; ++k)
			{
				Cell* nc = c->con[k];
				if (!nc) continue;
				nc->con[(k+2)&3] = 0;
				c->con[k] = 0;
			}
		}
	}
}

void NavSurfaceGenerator::BuildNonOverlappingRegions2(int w, int d)
{
	// Build non-overlapping regions
	printf("  - build non-overlapping regions 2 ... ");
	Stopwatch watch;
	watch.Start ();

	BitArray assignedCells (m_cells.Size ());

	DynArray<Cell*> orphans;

	unsigned reg = 1;

	int iter = 0;
	while (++iter < 100000)
	{
		// NOTE Nov 10, 2008: <pvl> find seed - the lowest cell still unassigned
		// to a region
		Stopwatch seedFindWatch;
		seedFindWatch.Start ();
		unsigned cellsChecked = 0;

		Cell * lowestUnassigned = 0;
		for (int ci=0; ci < m_cells.Size (); ++ci)
		{
			if (assignedCells.IsMarked (ci)) continue;
			Cell * c = & m_cells[ci];
			if (c->data)
			{
				assignedCells.Mark (ci);
				continue; // Skip unconnected.
			}

			++cellsChecked;

			if (lowestUnassigned == 0 || c->y < lowestUnassigned->y)
				lowestUnassigned = c;
/*
			lowestUnassigned = c;
			break;
*/
		}
		seedFindWatch.Stop ();
		//printf ("    (... %d cells checked, seed found in: %.3fms\n", cellsChecked, 1000.0 * seedFindWatch.GetElapsed ());
		if (lowestUnassigned == 0) break;

		// NOTE Nov 10, 2008: <pvl> now flood-fill from 'lowestUnassigned'
		FloodFill fill (m_cells, w, d, assignedCells);
		fill.Run (lowestUnassigned, reg);

		if (fill.RegSize () < 3)
			orphans.Push (lowestUnassigned);

		++reg;
	}

	RemoveOrphans (orphans);

	watch.Stop ();
	printf("%.3fms\n", 1000.0 * watch.GetElapsed ());
}



void NavSurfaceGenerator::BuildNonOverlappingRegions(int w, int d)
{
	// Build non-overlapping regions
	printf("  - build non-overlapping regions... ");
	Stopwatch totalWatch, memsetWatch, phase1Watch, phase2Watch, phase3Watch, phase4Watch, orphansWatch;

	totalWatch.Start();

	const unsigned csize = (w+2)*(d+2);
	Cell** visited = new Cell*[csize];

	DynArray<Cell*> openList;
	DynArray<Cell*> orphans;

	unsigned int reg = 1;

	// (MATT) Establish simple limits of how much of the array is still active.
	// This is virtually useless for a level with 2 tall towers in opposite corners, but in a more typical
	// level where gameplay is concentrated around the middle, it yields significant improvements. 
	// Can be disabled, see below. {2008/11/05}
	unsigned iMin = 0;
	unsigned iMax = m_cells.Size()-1;

	// (MATT)	Another simple optimisation: Keep track of whether spans should still be visited in the
	// first phase (because they are still not part of a walkable area). Can disable, see below {2008/11/05}
	bool *visitBuffer = new bool[m_cells.Size()];
	memset(visitBuffer, 1, m_cells.Size()*sizeof(bool));

	int iter = 0;
	while (++iter < 10000)
	{
		// Keep track of what range of the array was used in this iteration
		unsigned nextiMin = iMax;
		unsigned nextiMax = iMin;

		memsetWatch.Resume ();
		memset(visited, 0, csize*sizeof(Cell*));
		memsetWatch.Pause ();

		// Build occupied cells.
		phase1Watch.Resume ();
		int nvisited = 0;
		for (unsigned i=iMin ; i <= iMax; ++i)
		{
			if (!visitBuffer[i]) continue;  // Disable visitBuffer optimisation by skipping this line

			Cell* c = &m_cells[i];
			if (c->data || c->region) // Skip unconnected or already visited.
			{
				visitBuffer[i] = false;
				continue;
			}

			if (i < nextiMin)
				nextiMin = i;
			if (i > nextiMax)
				nextiMax = i;

			// Store lowest cells.
			const int idx = c->x+1 + (c->z+1)*w;
			if (visited[idx])
			{
				if (c->y < visited[idx]->y)
					visited[idx] = c;
			}
			else
			{
				visited[idx] = c;
				nvisited++;
			}
		}
		phase1Watch.Pause ();

		// Update the active range for next iteration
		// Disable the iMin/imax optimisation by skipping these lines
		iMin = nextiMin;
		iMax = nextiMax;

		// If no cells were visited, we're done
		if (!nvisited)
			break;

		phase2Watch.Resume ();
		// Fix boundaries.
		Cell* seed = 0;
		for (unsigned i = 0; i < csize; ++i)
		{
			Cell* c = visited[i];
			if (!c) continue;

			for (int j = 0; j < 4; ++j)
			{
				int x0 = c->x, z0 = c->z;
				switch(j)
				{
				case 0: x0--; break;
				case 1: z0++; break;
				case 2: x0++; break;
				case 3: z0--; break;
				};

				{
					const int idx = x0+1 + (z0+1)*w;
					Cell* cn = visited[idx];
					if (cn && cn != c->con[j])
					{
						if (c->y > cn->y)
						{
							visited[i] = 0;
							break;
						}
					}
				}

				int j1 = (j+1)&3;
				int x1 = x0, z1 = z0;
				switch(j1)
				{
				case 0: x1--; break;
				case 1: z1++; break;
				case 2: x1++; break;
				case 3: z1--; break;
				};

				{
					const int idx = x1+1 + (z1+1)*w;
					Cell* cn = visited[idx];
					if (cn)
					{
						Cell* cj = 0;
						if (c->con[j])
							cj = c->con[j]->con[j1];
						if (!cj && c->con[j1])
							cj = c->con[j1]->con[j];
						if (cn != cj)
						{
							if (c->y > cn->y)
							{
								visited[i] = 0;
								break;
							}
						}
					}
				}

			}

			c = visited[i];
			if (c)
			{
				if (!seed)
					seed = c;
				else if (c->y < seed->y)
					seed = c;
			}
		}
		phase2Watch.Pause ();

		if (!seed)
		{
			phase3Watch.Resume ();
			// In case seed got destroyed.
			for (unsigned i = 0; i < csize; ++i)
			{
				Cell* c = visited[i];
				if (c)
				{
					if (!seed)
						seed = c;
					else if (c->y < seed->y)
						seed = c;
				}
			}
			phase3Watch.Pause ();
		}

		phase4Watch.Resume ();
		openList.Clear();
		openList.Push(seed);

		seed->region = reg;

		int regSize = 0;
		while (!openList.Empty())
		{
			Cell* c = openList.Last();
			openList.Pop();
			regSize++;

			for (unsigned j = 0; j < 4; ++j)
			{
				Cell* nc = c->con[j];
				if (!nc) continue;
				if (nc->region) continue; // Skip already visited
				const int idx = nc->x+1 + (nc->z+1)*w;
				if (!visited[idx]) continue;
				nc->region = reg;
				openList.Push(nc);
			}
		}

		if (regSize < 3)
			orphans.Push(seed);

		reg++;

		phase4Watch.Pause ();
	
	} // while(...

	printf ("  (... %d iterations)", iter);

	DynArray<Cell*> detach;

	// Remove orphans
	orphansWatch.Start ();
	for (unsigned i = 0, ni = orphans.Size(); i < ni; ++i)
	{
		Cell* seed = orphans[i];

		openList.Clear();
		openList.Push(seed);

		unsigned int reg = seed->region;

		seed->region = 0;

		detach.Clear();

		int regSize = 0;
		while (!openList.Empty())
		{
			Cell* c = openList.Last();
			openList.Pop();
			regSize++;

			detach.Push(c);

			for (unsigned j = 0; j < 4; ++j)
			{
				Cell* nc = c->con[j];
				if (!nc) continue;
				if (nc->region != reg) continue; // Skip already visited
				nc->region = 0;
				openList.Push(nc);
			}
		}

		for (unsigned j = 0, nj = detach.Size(); j < nj; ++j)
		{
			Cell* c = detach[j];
			c->data = 0;
			for (unsigned k = 0; k < 4; ++k)
			{
				Cell* nc = c->con[k];
				if (!nc) continue;
				nc->con[(k+2)&3] = 0;
				c->con[k] = 0;
			}
		}
	}
	orphansWatch.Stop ();


	delete [] visitBuffer;
	delete [] visited;

	totalWatch.Stop ();

	// Get and print total timings for each phase
	double mw  = 1000.0 * memsetWatch.GetElapsed ();
	double p1w = 1000.0 * phase1Watch.GetElapsed ();
	double p2w = 1000.0 * phase2Watch.GetElapsed ();
	double p3w = 1000.0 * phase3Watch.GetElapsed ();
	double p4w = 1000.0 * phase4Watch.GetElapsed ();
	double ow  = 1000.0 * orphansWatch.GetElapsed ();
	printf ("  (... memset : %.3fms\n", mw);
	printf ("  (... phase 1: %.3fms\n", p1w);
	printf ("  (... phase 2: %.3fms\n", p2w);
	printf ("  (... phase 3: %.3fms\n", p3w);
	printf ("  (... phase 4: %.3fms\n", p4w);
	printf ("  (... orphans:  %.3fms\n", ow);
	// Add up the values as a sanity check
	printf ("  ...totalling %.3fms which should be similar to...\n", mw+p1w+p2w+p3w+p4w+ow );

	printf("%.3fms\n", 1000.0 * totalWatch.GetElapsed ());

/*
	// Can be useful for verifying that results have not changed
	printf("Dumping %d cells to text file", m_cells.Size());

	Stopwatch dump;
	dump.Start ();
	FILE *file = fopen("cellDump.txt","wb");
	if (file)
	{
		for (unsigned i = 0, ni = m_cells.Size(); i < ni; ++i)
		{
			Cell &c = m_cells[i];
			fprintf(file,"[%d,%d,%d] %d %d %d %d\n",c.x,c.y,c.z,c.region,c.flags,c.data,c.userData);
		}
		fclose(file);
	}
	dump.Stop();
	printf(" - Cell DEBUG dump: %.3fms\n", 1000.0 * dump.GetElapsed ());
*/
}

Cell* NavSurfaceGenerator::FindCell(int x, int y, int z)
{
	int bucket = ComputeHashBucketIndex(x, y, z);
	Cell* c = m_cellBuckets[bucket];
	while (c)
	{
		if (c->x == x && c->y == y && c->z == z)
			return c;
		c = c->next;
	}
	return 0;
}

#ifndef _WIN32
inline int abs(int a) { return a < 0 ? -a : a; }
static inline int sgn(int a) { return a < 0 ? -1 : 1; }
#endif // _WIN32

bool NavSurfaceGenerator::CheckWalkability(Cell* a, Cell* b)
{
	if (!a || !b) return false;

	int x = a->x;
	int z = a->z;
	int dx = b->x - a->x;
	int dz = b->z - a->z;

	int n, sx, sz, exz, ax, az, bx, bz;

	sx = sgn(dx);
	sz = sgn(dz);
	ax = abs(dx);
	az = abs(dz);
	bx = 2*ax;
	bz = 2*az;
	exz = az-ax;

	Cell* cur = a;

	n = ax+az;
	while (cur && n--)
	{
		if (exz < 0)
		{
			if (sx < 0)
				cur = cur->con[CON_XM];
			else
				cur = cur->con[CON_XP];
			x += sx;
			exz += bz;
		}
		else
		{
			if (sz < 0)
				cur = cur->con[CON_ZM];
			else
				cur = cur->con[CON_ZP];
			z += sz;
			exz -= bx;
		}
	}

	return cur == b;
}

void NavSurfaceGenerator::SetCells (LayeredNavMesh::DynArray<Cell> & newCells)
{
	m_cells.Swap (newCells);

	if (!m_cellBuckets)
		m_cellBuckets = new Cell*[BUCKET_COUNT];
	memset(m_cellBuckets, 0, sizeof(Cell*)*BUCKET_COUNT);

	for (unsigned i = 0, ni = m_cells.Size(); i < ni; ++i)
	{
		Cell& c = m_cells[i];
		int bucket = ComputeHashBucketIndex(c.x, c.y, c.z);
		c.next = m_cellBuckets[bucket];
		m_cellBuckets[bucket] = &c;
	}
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

void NavSurfaceGenerator::DebugDraw(const BuildSettings& settings)
{
	const Vec3& bmin = settings.bmin;
	const float cs = settings.voxelSize;	

//	if (m_pWalkable)
//		m_pWalkable->DebugDraw(orig, cs);

	glDisable(GL_LIGHTING);
//	glDepthMask(GL_FALSE);

/*	glPointSize(3);
	glBegin(GL_POINTS);
		for (unsigned i = 0, ni = m_cells.size(); i < ni; ++i)
		{
			Cell& c = m_cells[i];
			if (c.data == 0)
				IntToCol(c.region, 128);
			else
				glColor4ub(0,0,0,64);
			Vec3 pos(orig);
			pos.x += (c.x+0.5f)*cs;
			pos.y += (c.y+1.2f)*cs;
			pos.z += (c.z+0.5f)*cs;
			glVertex3fv(pos);
		}
	glEnd();
	glPointSize(1);

	glBegin(GL_LINES);
		for (unsigned i = 0, ni = m_cells.size(); i < ni; ++i)
		{
			Cell& c = m_cells[i];
			IntToCol(c.region, 128);
			Vec3 pos(orig);
			pos.x += (c.x+0.5f)*cs;
			pos.y += (c.y+1.2f)*cs;
			pos.z += (c.z+0.5f)*cs;
			if (c.con[CON_XP])
			{
				Cell* con = c.con[CON_XP];
				Vec3 pos1(orig);
				pos1.x += (con->x+0.5f)*cs;
				pos1.y += (con->y+1.2f)*cs;
				pos1.z += (con->z+0.5f)*cs;
				glVertex3fv(pos);
				glVertex3fv(pos1);
			}
			if (c.con[CON_ZP])
			{
				Cell* con = c.con[CON_ZP];
				Vec3 pos1(orig);
				pos1.x += (con->x+0.5f)*cs;
				pos1.y += (con->y+1.2f)*cs;
				pos1.z += (con->z+0.5f)*cs;
				glVertex3fv(pos);
				glVertex3fv(pos1);
			}
		}
	glEnd();
	glPointSize(1);*/

	bool drawSurface = m_debugItems[0].state;
	if (drawSurface)
	{
		glBegin(GL_QUADS);

		bool drawAllCells = m_debugItems[1].state;

		for (unsigned i = 0, ni = m_cells.Size(); i < ni; ++i)
		{
			Cell& c = m_cells[i];

			if (c.region)
				IntToCol(c.region, 128);
			else
			{
				if (!drawAllCells)
					continue;
				glColor4ub(0,0,0,64);
			}

			Vec3 pos(bmin);
			pos.x += (c.x)*cs;
			pos.y += (c.y)*cs; //(c.region*0.001f)*cs;
			pos.z += (c.z)*cs;
			glVertex3f(pos.x,pos.y,pos.z);
			glVertex3f(pos.x,pos.y,pos.z+cs);
			glVertex3f(pos.x+cs,pos.y,pos.z+cs);
			glVertex3f(pos.x+cs,pos.y,pos.z);
		}

		glEnd();
	}

	glEnable(GL_LIGHTING);
//	glDepthMask(GL_TRUE);
}

#endif
