//
//  NavMesh.h - Common header for the navmesh libarary
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

#ifndef NAVMESH_H
#define NAVMESH_H

#include "Vec3.h"

// TODO Feb 20, 2009: <pvl> too bad we need to inline this code just to fwd-declare
// engine version of Vec3.  It should have been in a light-weight (in terms of
// dependencies) osfwd.h-style file.
template <typename T> struct Vec3_tpl;
typedef float f32;
typedef Vec3_tpl<f32> Vec3;

#include <vector>

namespace LayeredNavMesh {

// Compile option to turn on and off the debugdraw methods.
#define DEBUG_DRAW

#ifdef DEBUG_DRAW
//#define DEBUG_DRAW_GL
#define DEBUG_DRAW_CRYENGINE
#else
#undef DEBUG_DRAW_GL
#undef DEBUG_DRAW_CRYENGINE
#endif


class NavModifSceneBounds;

class SceneBounds {
public:
	class AABB {
		Vec3 m_min, m_max;
	public:
		AABB (const Vec3 & min, const Vec3 & max);
		const Vec3 & Min () const;
		const Vec3 & Max () const;
	};
private:
	const NavModifSceneBounds * m_impl;
public:
	SceneBounds (const NavModifSceneBounds * ); 
	const AABB & GetAABB () const;

	bool Overlaps (const Vec3 & pt) const;
	bool Overlaps (const Vec3 & a, const Vec3 & b, const Vec3 & c) const;
	bool Overlaps (const std::vector < ::Vec3> & polygon) const;
};

struct BuildSettings
{
	// Scene bounds do _not_ need to contain all the geometry they just specify
	// which volume to generate the mesh for. 
	// NOTE Feb 20, 2009: <pvl> store just a pointer to enable setting bounds
	// only in LayeredMeshGenerator::Generate()
	const SceneBounds * sceneBounds;
	std::vector <const SceneBounds *> exclusionVolumes;

	float voxelSizeHorizontal;
	float voxelSizeVertical;
	int agentHeight;
	int agentRadius;
	int agentClimb;

	BuildSettings () : sceneBounds (0) { }
	// ATTN Feb 20, 2009: <pvl> 'sceneBounds' memory is owned by LayeredMeshGenerator!
	//~BuildSettings () { delete sceneBounds; }
};

struct Pointi
{
	inline Pointi() {}
	inline Pointi(int x, int y, int z, int pinned) : x(x), y(y), z(z), pinned(pinned) {}
	inline bool operator==(const Pointi& rhs) const { return x==rhs.x && y==rhs.y && z==rhs.z; }
	short x, y, z, pinned;
//	int x; //:10;
//	int y; //:10;
//	int z; //:10;
//	int pinned:2;
};

#ifdef DEBUG_DRAW
struct DebugItem
{
	bool state;
	const char* name;
};
#endif

} // namespace LayeredNavMesh

#endif
