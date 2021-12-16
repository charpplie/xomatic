////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   CAFSaver.h
//  Version:     v1.00
//  Created:     27/9/2007 by Norbert
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __CAFSaver_h__
#define __CAFSaver_h__
#pragma once

#include "AnimSaver.h"

class CSaverCAF : public CSaverAnim
{
public:
	CSaverCAF(const char* filename,CChunkFile& chunkFile) : CSaverAnim(filename,chunkFile) {}
	virtual void Save(CContentCGF *pCGF,CInternalSkinningInfo* pSkinningInfo);

private:
	int SaveController(CInternalSkinningInfo* pSkinningInfo,int ctrlIndex);
	int SaveBoneNameList(CContentCGF *pCGF);
};

#endif //__CAFSaver_h__