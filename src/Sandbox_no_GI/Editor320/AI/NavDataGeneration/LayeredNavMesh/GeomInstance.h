//
//  GeomInstance.h - Instance of a GeomMesh.
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

#ifndef GEOMINSTANCE_H
#define GEOMINSTANCE_H

#include <vector>
#include "Vec3.h"
#include "Matrix4.h"

namespace LayeredNavMesh {

class GeomMesh;

class GeomInstance
{
public:
	GeomInstance(const GeomMesh* pMesh, const Matrix4& tm);
	~GeomInstance();

	inline const GeomMesh* GetMesh() const { return m_pMesh; }
	inline const Matrix4& GetTM() const { return m_tm; }

#ifdef DEBUG_DRAW
	void DebugDraw() const;
#endif

private:
	const GeomMesh* m_pMesh;
	Matrix4 m_tm;
};

} // namespace LayeredNavMesh

#endif
