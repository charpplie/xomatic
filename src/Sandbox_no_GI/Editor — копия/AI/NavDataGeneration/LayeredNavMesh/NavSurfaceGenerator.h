//
//  NavSurfaceGenerator.h - Generates connected suface from voxels.
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

#ifndef NAVSURFACEGENERATOR_H
#define NAVSURFACEGENERATOR_H

#include "DynArray.h"
#include "NavMesh.h"
#include "VoxelBitmap.h"
#include "Voxelizer.h"

namespace LayeredNavMesh {

enum ConnectionIdx
{
	CON_XM = 0,
	CON_ZP = 1,
	CON_XP = 2,
	CON_ZM = 3,
};

struct Cell
{
	union
	{
		// ATTN Jul 21, 2009: <pvl> the idea behind using 'intptr_t' is getting
		// an int type that's the same size as pointer.  However, the standard
		// only seems to guarantee wide enough.  If it turns out to be wider
		// this will break.
		intptr_t cony[4];
		Cell* con[4];
	};		
	Cell* next;	// Next item in the row
	unsigned int region;
	short x, y, z;
	unsigned char flags;
	unsigned char data;
	unsigned char userData;
};

struct CellBounds
{
	int minx, miny, minz;
	int maxx, maxy, maxz;
};

class NavSurfaceGenerator
{
public:
	NavSurfaceGenerator(const BuildSettings& settings);
	~NavSurfaceGenerator();
	
	void BuildSurfaceFromVoxelBitmap(const VoxelBitmap* pVoxels);
	void BuildSurfaceFromSpanBuffer(const SpanBuffer* pSpans);

	inline unsigned GetCellCount() const { return m_cells.Size(); }
	inline const Cell& GetCell(unsigned i) const { return m_cells[i]; }
	inline Cell& GetCell(unsigned i) { return m_cells[i]; }
	inline const CellBounds& GetBounds() { return m_bounds; }

	bool CheckWalkability(Cell* a, Cell* b);
	Cell* FindCell(int x, int y, int z);

	// NOTE Feb 18, 2010: <pvl> will do Swap(), destroying its argument
	void SetCells (DynArray<Cell> & );

#ifdef DEBUG_DRAW
	void DebugDraw(const BuildSettings& settings);
	static const unsigned DEBUG_ITEM_COUNT = 2;
	DebugItem m_debugItems[DEBUG_ITEM_COUNT];
#endif

private:

	void BuildConnections();
	void DeflateBorders();
	void BuildNonOverlappingRegions(int w, int d);
	void BuildNonOverlappingRegions2(int w, int d);

	VoxelBitmap* m_pWalkable;
	
	Cell** m_cellBuckets;	// row for fast cell lookup
	DynArray<Cell> m_cells;	// cells
	// TODO Feb 17, 2010: <pvl> appears not used - delete if so
	CellBounds m_bounds;
	const BuildSettings m_settings;
};

} // namespace LayeredNavMesh

#endif
