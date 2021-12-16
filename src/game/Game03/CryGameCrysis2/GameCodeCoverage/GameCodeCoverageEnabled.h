////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   GameCodeCoverageEnabled.h
//  Created:     11/11/2009 by Tim Furnish
//  Description: Defines ENABLE_GAME_CODE_COVERAGE
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __GAME_CODE_COVERAGE_ENABLED_H_
#define __GAME_CODE_COVERAGE_ENABLED_H_

#if defined(_RELEASE)
	#define ENABLE_GAME_CODE_COVERAGE			0		// Final release - never enable!
#elif !defined(_DEBUG)
	#if defined(WIN64) || defined(WIN32)
		#define ENABLE_GAME_CODE_COVERAGE		1		// Profile builds on PC
	#else
		#define ENABLE_GAME_CODE_COVERAGE		0		// Profile builds on consoles
	#endif
#else
	#if defined(WIN64) || defined(WIN32)
		#define ENABLE_GAME_CODE_COVERAGE		1		// Debug builds on PC
	#else
		#define ENABLE_GAME_CODE_COVERAGE		1		// Debug builds on consoles
	#endif
#endif

#endif // __GAME_CODE_COVERAGE_ENABLED_H_