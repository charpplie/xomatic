//file acts as Uber-file to include all required SPU files for faster scanning
//and workaround for missing cross translation unit inlining
#if !defined(__SPU__) && defined(_DEVIRTUALIZE_)
	#include <CryPhysics_devirt_defines.h>
#endif

#include "intersectionchecks.cpp"
#include "linunprojectionchecks.cpp"
#include "rotunprojectionchecks.cpp"
#include "matrixnm.cpp"
#include "aabbtree.cpp"
#include "obbtree.cpp"
#include "geoman.cpp"
#include "singleboxtree.cpp"
#include "heightfieldbv.cpp"
#include "boxgeom.cpp"
#include "capsulegeom.cpp"
#include "cylindergeom.cpp"
#include "heightfieldgeom.cpp"
#include "spheregeom.cpp"
#include "utils.cpp"
#include "boolean2d.cpp"
#include "overlapchecks.cpp"
#include "rigidbody.cpp"
#include "trimesh.cpp"		
#include "softentity.cpp"
#include "geometry.cpp"
#include "voxelgeom.cpp"
#include "voxelbv.cpp"
#include "ropeentity.cpp"
#include "raygeom.cpp"
#include "raybv.cpp"
#include "particleentity.cpp"
#include "jobq.cpp"
#include "physarea.cpp"
#include "rwi.cpp"
#include "articulatedentity.cpp"

#if !defined(__SPU__) && defined(_DEVIRTUALIZE_)
	#include <CryPhysics_wrapper_includes.h>
#endif
