////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2006.
// -------------------------------------------------------------------------
//  File name:   BoneKey.h
//  Version:     v1.00
//  Created:     30/10/2006 by MichaelS.
//  Compilers:   Visual Studio.NET 2005
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __BONEKEY_H__
#define __BONEKEY_H__

struct BoneKey
{
	int time;
	Vec3	abspos;
	CryQuat	absquat;
	Vec3	relpos;
	CryQuat	relquat;
};

#endif //__BONEKEY_H__
