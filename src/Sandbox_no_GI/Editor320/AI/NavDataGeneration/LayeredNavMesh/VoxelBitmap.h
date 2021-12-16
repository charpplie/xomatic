//
//  VoxelBitmap.h - Voxel map.
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

#ifndef VOXELBITMAP_H
#define VOXELBITMAP_H

#include "NavMesh.h"
#include "Vec3.h"

namespace LayeredNavMesh {

class VoxelBitmap
{
public:

	typedef unsigned char Type;

	VoxelBitmap(int width, int height, int depth);
	VoxelBitmap(const VoxelBitmap& bitmap);
	~VoxelBitmap();
	
	void Clear();
	void Fill();
	
	inline int GetWidth() const { return m_width; }
	inline int GetHeight() const { return m_height; }
	inline int GetDepth() const { return m_depth; }
	inline int GetStrideY() const { return m_strideY; }
	inline int GetStrideZ() const { return m_strideZ; }
	inline int GetDataSize() const { return m_dataSize; }
	inline Type* GetData() { return m_pData; }
	inline const Type* GetData() const { return m_pData; }

	inline void Set(int x, int y, int z, Type v)
	{
		if (x >= 0 && y >= 0 && z >= 0 && x < m_width && y < m_height && z < m_depth)
		{
			const int idx = x + y * m_strideY + z * m_strideZ;
			m_pData[idx] = v;
		}
	}

	inline Type Get(int x, int y, int z)
	{
		if (x >= 0 && y >= 0 && z >= 0 && x < m_width && y < m_height && z < m_depth)
		{
			int idx = x + y * m_strideY + z * m_strideZ;
			return m_pData[idx];
		}
		return 0;
	}

#ifdef DEBUG_DRAW
	void DebugDraw(const BuildSettings& settings);
#endif
	
private:
	int m_width, m_height, m_depth;
	int m_dataSize;
	int m_strideY, m_strideZ;
	Type* m_pData;
};

} // namespace LayeredNavMesh

#endif
