//
//  SpanBuffer.cpp - Run length encoded voxel map. 
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
#include "SpanBuffer.h"

using namespace LayeredNavMesh;

SpanBuffer::SpanBuffer(int width, int height) :
	m_width(width),
	m_height(height),
	m_spanMin(0),
	m_spanMax(0),
//	m_freeList(0),
	m_spans(0)
{
	m_spans = new Span*[m_width * m_height];
	for (int i = 0, ni = m_width*m_height; i < ni; ++i)
		m_spans[i] = 0;
}

SpanBuffer::~SpanBuffer()
{
/*
	// Delete spans
	for (int i = 0, ni = m_width*m_height; i < ni; ++i)
	{
		Span* s = m_spans[i];
		while (s)
		{
			Span* next = s->next;
			delete s;
			s = next;
		}
		m_spans[i] = 0;
	}
	delete [] m_spans;
	m_spans = 0;
	// Delete freelist
	Span* s = m_freeList;
	while (s)
	{
		Span* next = s->next;
		delete s;
		s = next;
	}
	m_freeList = 0;
*/
}


void SpanBuffer::Compact()
{
	// Delete freelist
/*	Span* s = m_freeList;
	while (s)
	{
		Span* next = s->next;
		delete s;
		s = next;
	}
	m_freeList = 0;*/
}

int SpanBuffer::GetSpanCount() const
{
	// Delete spans
	int spans = 0;
	for (int i = 0, ni = m_width*m_height; i < ni; ++i)
	{
		Span* s = m_spans[i];
		while (s)
		{
			spans++;
			s = s->next;
		}
	}
	return spans;
}

int SpanBuffer::GetMemSize()
{
	int size = sizeof(*this);
	size += sizeof(Span*)*m_width*m_height;

	// Delete spans
/*	int spans = 0;
	for (int i = 0, ni = m_width*m_height; i < ni; ++i)
	{
		Span* s = m_spans[i];
		while (s)
		{
			spans++;
			s = s->next;
		}
	}

	// Delete freelist
	int free = 0;
	Span* s = m_freeList;
	while (s)
	{
		free++;
		s = s->next;
	}
	size += (spans+free) * sizeof(Span);*/
	
	size += m_pool.GetMemSize();

	return size;
}

#ifdef DEBUG_DRAW_GL

#include <SDL.h>
#include <SDL_OpenGL.h>


static void DrawBox(float ax, float ay, float az,
										float bx, float by, float bz)
{
	Vec3 verts[8];
	
	verts[0].Set(ax, ay, az);
	verts[1].Set(bx, ay, az);
	verts[2].Set(bx, ay, bz);
	verts[3].Set(ax, ay, bz);
	
	verts[4].Set(ax, by, az);
	verts[5].Set(bx, by, az);
	verts[6].Set(bx, by, bz);
	verts[7].Set(ax, by, bz);

	// Top
	glNormal3f(0, 1, 0);
	glVertex3fv(verts[7]);
	glVertex3fv(verts[6]);
	glVertex3fv(verts[5]);
	glVertex3fv(verts[4]);

	// Bottom
/*	glNormal3f(0, -1, 0);
	glVertex3fv(verts[0]);
	glVertex3fv(verts[1]);
	glVertex3fv(verts[2]);
	glVertex3fv(verts[3]);

	// Side0
	glNormal3f(1, 0, 0);
	glVertex3fv(verts[1]);
	glVertex3fv(verts[5]);
	glVertex3fv(verts[6]);
	glVertex3fv(verts[2]);

	// Side1
	glNormal3f(-1, 0, 0);
	glVertex3fv(verts[3]);
	glVertex3fv(verts[7]);
	glVertex3fv(verts[4]);
	glVertex3fv(verts[0]);

	// Side2
	glNormal3f(0, 0, 1);
	glVertex3fv(verts[2]);
	glVertex3fv(verts[6]);
	glVertex3fv(verts[7]);
	glVertex3fv(verts[3]);

	// Side3
	glNormal3f(0, 0, -1);
	glVertex3fv(verts[0]);
	glVertex3fv(verts[4]);
	glVertex3fv(verts[5]);
	glVertex3fv(verts[1]);*/
}

void SpanBuffer::DebugDraw(const BuildSettings& settings)
{
	const Vec3& bmin = settings.bmin;
	const float cs = settings.voxelSize;

	glBegin(GL_QUADS);
	for (int y = 0; y < m_height; ++y)
	{
		for (int x = 0; x < m_width; ++x)
		{
			Span* s = m_spans[x+y*m_width];
			while (s)
			{
				float fx = bmin.x + x*cs;
				float fy = bmin.y + (s->smin+0.5f)*cs;
				float fz = bmin.z + y*cs;
				float fh = (s->smax-s->smin-0.5f)*cs;

				if (s->flags)
					glColor3ub(255,255,255);
				else
					glColor3ub(255,0,0);

				DrawBox(fx, fy, fz,
								fx+cs, fy+fh, fz+cs);
				s = s->next;
			}
		}
	}
	glEnd();
}

#endif
