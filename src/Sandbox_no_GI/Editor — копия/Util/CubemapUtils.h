////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2010.
// -------------------------------------------------------------------------
//  File name:   environemenprobeobject.h
//  Version:     v1.00
//  Created:     30/3/2010 by Johnmichael.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#if !defined __cubemaputils_h_
#define __cubemaputils_h_

namespace CubemapUtils
{
	bool GenCubemap(CString & texturename, CWnd* pParent);
	bool GenCubemapWithPathAndSize(CString &filename, const int size, const bool dds = true);
	bool GenCubemapWithObjectPathAndSize(CString &filename, CBaseObject * pObject, const int size, const bool dds);
	void GenHDRCubemapTiff( const CString& fileName, int size, Vec3& pos );
	void RegenerateAllEnvironmentProbeCubemaps();
}

#endif //__cubemaputils_h_ 
