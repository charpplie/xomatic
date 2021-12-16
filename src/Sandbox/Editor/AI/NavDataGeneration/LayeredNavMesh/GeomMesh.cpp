//
//  GeomMesh.cpp - Simple 3D mesh representation.
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

#include "DynArray.h"
#include "NavMesh.h"
#include "Vec3.h"
#include "GeomMesh.h"
#include <math.h>

using namespace LayeredNavMesh;

GeomMesh::GeomMesh(const DynArray<LayeredNavMesh::Vec3>& verts, const DynArray<unsigned int>& indices) :
		m_flags (GENERATE_CAP_MESH)
{
	m_verts.Resize(verts.Size());
	for (unsigned i = 0, ni = verts.Size(); i < ni; ++i)
		m_verts[i] = verts[i];
	
	// Build faces
	unsigned nfaces = indices.Size() / 3;
	m_faces.Resize(nfaces);
	const unsigned int* ind = &indices.First();
	for (unsigned i = 0; i < nfaces; ++i)
	{
		Face& f = m_faces[i];
		f.va = *ind++;
		f.vb = *ind++;
		f.vc = *ind++;
		Vec3 ab = verts[f.vb] - verts[f.va];
		Vec3 ac = verts[f.vc] - verts[f.va];
		f.norm = Cross(ab, ac);
		f.norm.Normalize();
	}
}

GeomMesh::~GeomMesh()
{
	// Empty
}


#ifdef DEBUG_DRAW_GL

#include <SDL.h>
#include <SDL_OpenGL.h>

void GeomMesh::DebugDraw(const Matrix4& tm) const
{
	glPushMatrix();
	glMultMatrixf(tm);

	const float thr = cosf(60.0f/180.0f*M_PI); //tanf(35.0f/180.0f*M_PI);

	glBegin(GL_TRIANGLES);
	for (DynArray<GeomMesh::Face>::ConstIter f = m_faces.Begin(); f != m_faces.End(); ++f)
	{
		glNormal3fv(f->norm);

		Vec3 n = tm.TransformVector(f->norm);
		if (fabsf(n.y) > thr)
			glColor3ub(255,255,255);
		else
			glColor3ub(255,230,128);

		glVertex3fv(m_verts[f->va]);
		glVertex3fv(m_verts[f->vb]);
		glVertex3fv(m_verts[f->vc]);
	}
	glEnd();

	glPopMatrix();
}

#endif
