////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2006.
// -------------------------------------------------------------------------
//  File name:   AnimationInfoLoader.h
//  Version:     v1.00
//  Created:     22/6/2006 by Alexey Medvedev.
//  Compilers:   Visual Studio.NET 2005
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef _SKELETON_INFO
#define _SKELETON_INFO

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CGFContent.h"

class ILoaderCGFListener;



class CSkeletonInfo
{
public:
	CSkeletonInfo(void);
	~CSkeletonInfo(void);


	bool LoadCHRModel(const char * name, ILoaderCGFListener * pListener);


	CSkinningInfo m_SkinningInfo;


};

#endif