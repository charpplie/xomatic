////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   ProjectDefines.h
//  Version:     v1.00
//  Created:     3/30/2004 by MartinM.
//  Compilers:   Visual Studio.NET
//  Description: to get some defines available in every CryEngine project 
// -------------------------------------------------------------------------
//  History:
//    July 20th 2004 - Mathieu Pinard
//    Updated the structure to handle more easily different configurations
//
////////////////////////////////////////////////////////////////////////////

#ifndef PROJECTDEFINES_H
#define PROJECTDEFINES_H

#if !defined GAME_IS_TECHDEMO
#	if defined(LINUX)
#		define EXCLUDE_SCALEFORM_SDK
#		define EXCLUDE_CRI_SDK
#		define EXCLUDE_GPU_PARTICLE_PHYSICS















# elif defined(WIN32) && defined(WIN64)
//#		define EXCLUDE_SCALEFORM_SDK
//#		define EXCLUDE_CRI_SDK
#		define EXCLUDE_GPU_PARTICLE_PHYSICS
# else
//#		define EXCLUDE_SCALEFORM_SDK
//#		define EXCLUDE_CRI_SDK
#		define EXCLUDE_GPU_PARTICLE_PHYSICS
#	endif
#endif

// For the Tech Demo, many third party library are disabled and we also disable DATAPROBE
#if defined GAME_IS_TECHDEMO
#	define EXCLUDE_SCALEFORM_SDK
#	define EXCLUDE_BINK_SDK
#	define EXCLUDE_CRI_SDK
#	define EXCLUDE_GPU_PARTICLE_PHYSICS
#endif

// see http://wiki/bin/view/CryEngine/TerrainTexCompression for more details on this
// 0=off, 1=on
#define TERRAIN_USE_CIE_COLORSPACE 0

// for consoles every bit of memory is important so files for documentation purpose are excluded
// they are part of regular compiling to verify the interface









#define EXCLUDE_UNIT_TESTS	0	
#ifdef RELEASE
#undef EXCLUDE_UNIT_TESTS
#define EXCLUDE_UNIT_TESTS	1
#endif

#if !defined(RESOURCE_COMPILER) && (!defined(_RELEASE) && (defined(PS3) && !defined(SUPP_MTRACE) && !defined(JOB_LIB_COMP) && !defined(__SPU__)) || (defined(XENON) && !defined(_LIB)) || (defined(WIN32) && !defined(_RELEASE)) && !defined(NOT_USE_CRY_MEMORY_MANAGER))
#define CAPTURE_REPLAY_LOG 1
#else
#define CAPTURE_REPLAY_LOG 0
#endif

#define OLD_VOICE_SYSTEM_DEPRECATED
//#define INCLUDE_PS3PAD
#define EXCLUDE_SCALEFORM_SDK
#define EXCLUDE_CRI_SDK







#if (defined(XENON) || defined(PS3)) && defined(USING_LICENSE_PROTECTION)
#undef USING_LICENSE_PROTECTION
#undef USING_TAGES_SECURITY
#endif




#	define TAGES_EXPORT



// test -------------------------------------
//#define EXCLUDE_CVARHELP

#define _DATAPROBE

#include "ProjectDefinesInclude.h"

#endif // PROJECTDEFINES_H
