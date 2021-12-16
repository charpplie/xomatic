#include "StdAfx.h"

#include "SkeletonInfo.h"
#include "CGF\CGFLoader.h"
#include "CGF\CGFSaver.h"



CSkeletonInfo::CSkeletonInfo()
{
}

CSkeletonInfo::~CSkeletonInfo()
{
}


bool CSkeletonInfo::LoadCHRModel(const char * name, ILoaderCGFListener * pListener)
{
	CLoaderCGF cgfLoader;

	std::auto_ptr<CContentCGF> pCGF;
	CChunkFile chunkFile;
	pCGF.reset(cgfLoader.LoadCGF( name,chunkFile, pListener ));
	if (!pCGF.get())
	{
		RCLog( "Failed to load geometry file %s - %s",(const char*)name,cgfLoader.GetLastError() );
		return false;
	}

	m_SkinningInfo = *pCGF->GetSkinningInfo();
	m_SkinningInfo.m_arrMorphTargets.clear();


	return true;
}