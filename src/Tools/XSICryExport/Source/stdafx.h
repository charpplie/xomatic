//***************************************************************************************
//
// File supervisor: Softimage Rendering & Pipeline team
//
// (c) Copyright 2001-2005 Avid Technology, Inc. . All rights reserved.
//
//***************************************************************************************

/****************************************************************************************
THIS CODE IS PUBLISHED AS A SAMPLE ONLY AND IS PROVIDED "AS IS".
IN NO EVENT SHALL SOFTIMAGE, AVID TECHNOLOGY, INC. AND/OR THEIR RESPECTIVE
SUPPLIERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
CONNECTION WITH THE USE OR PERFORMANCE OF THIS CODE .

COPYRIGHT NOTICE. Copyright © 1999-2005 Avid Technology Inc. . All rights reserved. 

SOFTIMAGE is a registered trademark of Avid Technology Inc. or its subsidiaries 
or divisions. Windows NT is a registered trademark of Microsoft Corp. All other
trademarks contained herein are the property of their respective owners. 
****************************************************************************************/

/*! \file stdafx.h   
    include file for standard system include files
*/

#pragma once

//////////////////////////////////////////////////////////////////////////
#define _WIN32_WINNT 0x501
// Turn off debug stl stuff.
#define _HAS_ITERATOR_DEBUGGING 0
#define _SECURE_SCL 0

#define NOT_USE_CRY_MEMORY_MANAGER
#define NOT_USE_CRY_STRING

#include <wchar.h>

#ifdef unix
# include <sys/time.h>
# include <sys/param.h>
# define _XSI_EXTERN_ extern "C"
# define XSIEXPORT
#else
# define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
# ifdef _WIN64
#  define _AMD64_
# endif
struct _CONTEXT;
typedef struct _CONTEXT CONTEXT;
typedef _CONTEXT* PCONTEXT;
#ifdef _WIN32
#include <intrin.h>
#endif
#include <windows.h>
#include <commdlg.h>
#include <tchar.h>
# include <time.h>
# include <mmsystem.h>
# define _XSI_EXTERN_ extern "C" __declspec(dllexport)

#ifndef XSIEXPORT
# define XSIEXPORT __declspec(dllexport)
#endif

#endif



#ifdef unix
#define _wcsdup wcsdup
#define stricmp strcasecmp
#define si_getusername(struser, length) cuserid(struser)
#define MAX_PATH MAXPATHLEN

inline unsigned int timeGetTime() 
{
	struct timeval tv;
	struct timezone tz;

	gettimeofday(&tv, &tz);
	return tv.tv_sec*1000 + (tv.tv_usec/1000);
}

inline wchar_t* si_wcstok(wchar_t* wcs, wchar_t* delim)
{
	wchar_t* l_state;
	return wcstok(wcs, delim, &l_state);
}


#else
#define si_wcstok wcstok
#define si_getusername GetUserNameA
#endif


// TODO: reference additional headers your program requires here
#include <sicppsdk.h>

#include <string>
using std::string;
typedef std::wstring tstring;

// Define this to prevent including CryAssert (there is no proper hook for turning this off, like the above).
#define __CRYASSERT_H__

#define NO_XENON_INTRINSICS // Make sure we don't end up using xenon code.
//#define NOT_USE_CRY_STRING // defined in project settings

#include <assert.h>

#define CRY_ASSERT_TRACE assert
#define CRY_ASSERT assert

#include <platform.h>

// Turn of _DEBUG around stl headers - this is to stop the classes having a first iterator pointer,
// which breaks some binary compatibility. This is only really needed because the Max morpher modifier
// exposes a std::vector as part of the interface to the dll and for some reason expects that to work.
#if defined(_DEBUG)
#	undef _DEBUG
#endif //defined(_DEBUG)

#include <string>
using std::string;
typedef std::wstring tstring;
#define TSTRING_DEFINED

#include <map>
#include <set>
#include <vector>
#include "StlUtils.h"

#include <Windows.h>
#include <smartptr.h>

// Main defines of the CryExport project
#include "CryExportDefines.h"
#include "Export/ExportFileType.h"


//////////////////////////////////////////////////////////////////////////
// Include all major XSI headers.
//////////////////////////////////////////////////////////////////////////
#include <xsi_application.h>
#include <xsi_scene.h>
#include <xsi_project.h>
#include <xsi_context.h>
#include <xsi_menu.h>
#include <xsi_property.h>
#include <xsi_model.h>
#include <xsi_command.h>
#include <xsi_argument.h>
#include <xsi_ppglayout.h>
#include <xsi_time.h>
#include <xsi_parameter.h>
#include <xsi_ppgeventcontext.h>
#include <xsi_selection.h>
#include <xsi_uitoolkit.h>
#include <xsi_griddata.h>
#include <xsi_color.h> 

#include <xsi_material.h>
#include <xsi_shader.h>
#include <xsi_kinematics.h>
#include <xsi_imageclip.h>
#include <xsi_actionsource.h>
#include <xsi_mixer.h>
#include <xsi_materiallibrary.h>
#include <xsi_preferences.h>
#include <xsi_null.h>
#include <xsi_geometry.h>
#include <xsi_primitive.h>
#include <xsi_polygonmesh.h>
#include <xsi_geometryaccessor.h>
#include <xsi_ref.h>
#include <xsi_graphicsequencercontext.h>

#include <xsi_animationsourceitem.h>
#include <xsi_fcurve.h>
#include <xsi_staticsource.h>

//! log text to XSI script panel, final text size must be < 2048 bytes ( chars + \0 )
extern void XSILog( const char* pFormat, ... );

//////////////////////////////////////////////////////////////////////////
// CrossWalk headers.
//////////////////////////////////////////////////////////////////////////
/*
// Modules
#include "Crosswalk/Modules/cnv_dotXSI_IO.h"
#include "Crosswalk/Modules/cnv_COLLADA_IO.h"
#include "Crosswalk/Modules/cnv_info.h"
#include "Crosswalk/Modules/cnv_environment.h"
#include "Crosswalk/Modules/cnv_hierarchytraversal.h"
#include "Crosswalk/Modules/cnv_model.h"
#include "Crosswalk/Modules/cnv_null.h"
#include "Crosswalk/Modules/cnv_modelinfo.h"
#include "Crosswalk/Modules/cnv_camera.h"
#include "Crosswalk/Modules/cnv_custompset.h"
#include "Crosswalk/Modules/cnv_material.h"
#include "Crosswalk/Modules/cnv_image.h"
#include "Crosswalk/Modules/cnv_instance.h"
#include "Crosswalk/Modules/cnv_mesh.h"
#include "Crosswalk/Modules/cnv_mesh_clusterprop.h"
#include "Crosswalk/Modules/cnv_mesh_cluster.h"
#include "Crosswalk/Modules/cnv_light.h"
#include "Crosswalk/Modules/cnv_camera_anim.h"
#include "Crosswalk/Modules/cnv_custompset_anim.h"
#include "Crosswalk/Modules/cnv_environment_anim.h"
#include "Crosswalk/Modules/cnv_material_anim.h"
#include "Crosswalk/Modules/cnv_light_anim.h"
#include "Crosswalk/Modules/cnv_modelinfo_anim.h"
#include "Crosswalk/Modules/cnv_image_anim.h"
#include "Crosswalk/Modules/cnv_mixer.h"
#include "Crosswalk/Modules/cnv_userdatablob.h"
#include "Crosswalk/Modules/cnv_plottedanimation.h"
#include "Crosswalk/Modules/cnv_envelope.h"
#include "Crosswalk/Modules/cnv_shapeanimation.h"
#include "Crosswalk/Modules/cnv_hierarchyelementinfo.h"
#include "Crosswalk/Modules/cnv_ik.h"
*/
//////////////////////////////////////////////////////////////////////////