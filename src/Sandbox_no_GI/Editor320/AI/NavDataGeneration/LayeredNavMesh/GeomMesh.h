//
//  GeomMesh.h - Simple 3D mesh representation.
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

#ifndef GEOMMESH_H
#define GEOMMESH_H

#include "NavMesh.h"
#include "DynArray.h"
#include "Vec3.h"
#include "Matrix4.h"

namespace LayeredNavMesh {

class GeomMesh
{
public:
	struct Face
	{
		Vec3 norm;
		union
		{
			struct { unsigned int va, vb, vc; };
			unsigned int v[3];
		};
	};
	
	GeomMesh(const DynArray<Vec3>& verts, const DynArray<unsigned int>& indices);
	~GeomMesh();

	inline const DynArray<Vec3>& GetVerts() const { return m_verts; }
	inline const DynArray<Face>& GetFaces() const { return m_faces; }

	void GenerateCapMesh (bool val);
	bool GenerateCapMesh () const;

#ifdef DEBUG_DRAW
	void DebugDraw(const Matrix4& tm) const;
#endif

private:
	DynArray<Vec3> m_verts;
	DynArray<Face> m_faces;

	enum {
		// NOTE Jan 26, 2010: <pvl> turn this off to generate no cap mesh
		// (i.e. no navigable area on top) for this piece of input geometry
		GENERATE_CAP_MESH = 1 << 0,
	};
	unsigned m_flags;
};

inline void GeomMesh::GenerateCapMesh (bool val)
{
	if (val)
		m_flags |= GENERATE_CAP_MESH;
	else
		m_flags &= ~GENERATE_CAP_MESH;
}

inline bool GeomMesh::GenerateCapMesh () const
{
	return m_flags & GENERATE_CAP_MESH;
}

} // namespace LayeredNavMesh

#endif
