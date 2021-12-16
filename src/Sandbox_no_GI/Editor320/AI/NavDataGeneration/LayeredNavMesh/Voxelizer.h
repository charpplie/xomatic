//
//  Voxelizer.h - Generates bitmap or span buffer voxelization of GeomMeshes.
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

#ifndef VOXELIZER_H
#define VOXELIZER_H

#include "NavMesh.h"
#include "DynArray.h"
#include "VoxelBitmap.h"
#include "Matrix4.h"
#include "GeomInstance.h"
#include "SpanBuffer.h"


#include <stdio.h>

namespace LayeredNavMesh {

class Voxelizer
{
public:
	Voxelizer(const BuildSettings& settings);
	~Voxelizer();

	void VoxelizeVoxelBitmap(const DynArray<GeomInstance*>& instances);
	void VoxelizeSpanBuffer(const DynArray<GeomInstance*>& instances);

	inline const VoxelBitmap* GetBitmap() const { return m_pBitmap; }
	inline const SpanBuffer* GetSpans() const { return m_pSpans; }

	// NOTE Jan 29, 2010: <pvl> loading from file makes use of this
	// NOTE Feb 1, 2010: <pvl> initialises SpanBuffer to dimensions computed from
	// 'm_settings' and optionally returns the dimensions in arguments
	bool InitSpanBuffer (int * width=0, int * height=0, int * depth=0);

#ifdef DEBUG_DRAW
	void DebugDraw(const BuildSettings& settings);
#endif

private:
	void Voxelize(const DynArray<GeomInstance*>& instances, int w, int h, int d);
	void VoxelizeMeshBitmap(const Matrix4& tm, const GeomMesh* pMesh, VoxelBitmap* pDst);
	void VoxelizeMeshSpans(const Matrix4& tm, const GeomMesh* pMesh, int w, int h, int d, SpanBuffer* pDst);

	const BuildSettings m_settings;

	VoxelBitmap* m_pBitmap;
	SpanBuffer* m_pSpans;

	unsigned m_testPass;
	unsigned m_testPlane;
	unsigned m_testTri;
};

} // namespace LayeredNavMesh

#endif
