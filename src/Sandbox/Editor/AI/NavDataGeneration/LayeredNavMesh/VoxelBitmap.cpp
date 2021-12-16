//
//  VoxelBitmap.cpp - Voxel map.
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

#include <string.h>
#include "Vec3.h"
#include "VoxelBitmap.h"

using namespace LayeredNavMesh;

VoxelBitmap::VoxelBitmap(int width, int height, int depth) :
	m_width(width), m_height(height), m_depth(depth)
{
	m_dataSize = m_width * m_height * m_depth;
	m_strideY = m_width * m_depth;
	m_strideZ = m_width;
	m_pData = new Type[m_dataSize];
}

VoxelBitmap::VoxelBitmap(const VoxelBitmap& bitmap) :
	m_width(bitmap.m_width), m_height(bitmap.m_height), m_depth(bitmap.m_depth)
{
	m_dataSize = m_width * m_height * m_depth;
	m_strideY = m_width * m_depth;
	m_strideZ = m_width;
	m_pData = new Type[m_dataSize];
	memcpy(m_pData, bitmap.m_pData, m_dataSize * sizeof(Type));
}

VoxelBitmap::~VoxelBitmap()
{
	delete [] m_pData;
}

void VoxelBitmap::Clear()
{
	memset(m_pData, 0, m_dataSize * sizeof(Type));
}

void VoxelBitmap::Fill()
{
	memset(m_pData, 0xff, m_dataSize * sizeof(Type));
}


#ifdef DEBUG_DRAW_GL

#include <SDL.h>
#include <SDL_OpenGL.h>
	
void VoxelBitmap::DebugDraw(const BuildSettings& settings)
{
	const Vec3& bmin = settings.bmin;
	const float cs = settings.voxelSize;

	glBegin(GL_QUADS);
	
	for (int y = -1; y < m_height+1; ++y)
	{
		float sy = y*cs + bmin.y;

		for (int z = -1; z < m_depth+1; ++z)
		{
			float sz = z*cs + bmin.z;
			for (int x = -1; x < m_width+1; ++x)
			{
				float sx = x*cs + bmin.x;

				Type c = Get(x,y,z);
				Type cx = Get(x+1,y,z);
				Type cy = Get(x,y+1,z);
				Type cz = Get(x,y,z+1);

				if (c)
				{
					if (!cx)
					{
						glNormal3f(1,0,0);
						glVertex3f(sx+cs, sy, sz);
						glVertex3f(sx+cs, sy+cs, sz);
						glVertex3f(sx+cs, sy+cs, sz+cs);
						glVertex3f(sx+cs, sy, sz+cs);
					}
					if (!cy)
					{
						glNormal3f(0,1,0);
						glVertex3f(sx, sy+cs, sz);
						glVertex3f(sx, sy+cs, sz+cs);
						glVertex3f(sx+cs, sy+cs, sz+cs);
						glVertex3f(sx+cs, sy+cs, sz);
					}
					if (!cz)
					{
						glNormal3f(0,0,1);
						glVertex3f(sx+cs, sy, sz+cs);
						glVertex3f(sx+cs, sy+cs, sz+cs);
						glVertex3f(sx, sy+cs, sz+cs);
						glVertex3f(sx, sy, sz+cs);
					}
				}
				else
				{
					if (cx)
					{
						glNormal3f(-1,0,0);
						glVertex3f(sx+cs, sy, sz);
						glVertex3f(sx+cs, sy, sz+cs);
						glVertex3f(sx+cs, sy+cs, sz+cs);
						glVertex3f(sx+cs, sy+cs, sz);
					}
					if (cy)
					{
						glNormal3f(0,-1,0);
						glVertex3f(sx, sy+cs, sz);
						glVertex3f(sx+cs, sy+cs, sz);
						glVertex3f(sx+cs, sy+cs, sz+cs);
						glVertex3f(sx, sy+cs, sz+cs);
					}
					if (cz)
					{
						glNormal3f(0,0,-1);
						glVertex3f(sx+cs, sy, sz+cs);
						glVertex3f(sx, sy, sz+cs);
						glVertex3f(sx, sy+cs, sz+cs);
						glVertex3f(sx+cs, sy+cs, sz+cs);
					}
				}

			}
		}
	}
	glEnd();
}


#endif
