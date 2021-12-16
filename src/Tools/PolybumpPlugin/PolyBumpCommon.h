#ifndef _POLYBUMP_COMMON_H
#define _POLYBUMP_COMMON_H

#pragma once


#define fxopen		fopen

#include "platform.h"


//#define USE_COPYPROTECTION							// otherwise the tool appears to be free

//#define USE_REDUCEDFORPUBLIC						// removed stuff we don't want to show currently in public (e.g. occlusion direction)


#define USE_RASTERCUBE_ACCELERATOR_MAGNIFIER 0.5f	// this magnifier could speed up the raytracing calculations (higher values =>more memory consumption)



#define TRIALVERSION_MAXHIGHPOLYCOUNT			35000
#define TRIALVERSION_MAXTEXTURESIZE				256


#ifdef PLUGIN_FOR_3DSTUDIO
//		#define USE_SMOOTH_TANGENTSPACE_BY_SMOOTHINGGROUPS		// otherwise the tangentspace is smoothed by the normals (SG do not exist in Maya)
	#pragma comment(lib, "maxutil.lib")
	#pragma comment(lib, "bmm.lib")
	#pragma comment(lib, "core.lib")
	#pragma comment(lib, "geom.lib")
	#pragma comment(lib, "mesh.lib")
	#pragma comment(lib, "winmm.lib")
#endif


#ifdef PLUGIN_FOR_MAYA
		#define USE_SMOOTH_TANGENTSPACE_BY_SMOOTHINGGROUPS		// otherwise the tangentspace is smoothed by the normals (SG do not exist in Maya)
#endif









#endif // _POLYBUMP_COMMON_H


