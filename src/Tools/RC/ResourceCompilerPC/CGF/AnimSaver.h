////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   AnimSaver.h
//  Version:     v1.00
//  Created:     27/9/2007 by Norbert
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __AnimSaver_h__
#define __AnimSaver_h__
#pragma once

#include <StringUtils.h>
#include "CryPath.h"
#include <CryTypeInfo.h>
#include "ChunkFile.h"
#include "CGFContent.h"
#include "..\CGA\Controller.h"
#include "..\CGA\ControllerPQ.h"
#include "..\CGA\ControllerPQLog.h"
#include "LoaderCAF.h"

class CSaverAnim
{
public:
	CSaverAnim(const char* filename,CChunkFile& chunkFile);
	virtual void Save(CContentCGF *pCGF,CInternalSkinningInfo* pSkinningInfo) = 0;

	static int SaveTCB3Track(CChunkFile *pChunkFile, CInternalSkinningInfo* pSkinningInfo, int trackIndex);
	static int SaveTCBQTrack(CChunkFile *pChunkFile, CInternalSkinningInfo* pSkinningInfo, int trackIndex);
	static int SaveTiming(CChunkFile *pChunkFile, CInternalSkinningInfo* pSkinningInfo);

protected:
	int SaveExportFlags(CContentCGF *pCGF);
	int SaveTiming(CInternalSkinningInfo* pSkinningInfo);

	string m_filename;
	CChunkFile* m_pChunkFile;
};

#endif //__AnimSaver_h__