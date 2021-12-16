////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:   ResourceCollector.cpp
//  Created:     09/10/2009 by Paulo Zaffari.
//  Description: This class is the resource collector for the CrySizer
//							 element used in GetResourcememoryUsage.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ResourceCollector.h"
#include "ResourceCompilerHelper.h"

//////////////////////////////////////////////////////////////////////////
CResourceCollector::CResourceCollector()
{
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CResourceCollector::ComputeDependencyCnt()
{
	std::set<SDependencyPair>::const_iterator it, end=m_Dependencies.end();

	for(it=m_Dependencies.begin();it!=end;++it)
	{
		const SDependencyPair &rRef = *it;

		++m_Assets[rRef.m_idDependsOnAsset].m_dwDependencyCnt;
	}
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CResourceCollector::LogData( ILog &rLog )
{
	rLog.Log(" ");

	{
		rLog.Log("Assets:");

		std::vector<SAssetEntry>::const_iterator it, end=m_Assets.end();
		uint32 dwAssetID=0;

		for(it=m_Assets.begin();it!=end;++it,++dwAssetID)
		{
			const SAssetEntry &rRef = *it;

			rLog.Log(" A%d: inst:%5d dep:%d mem:%9d file:%9d name:%s",dwAssetID,rRef.m_dwInstanceCnt,rRef.m_dwDependencyCnt,rRef.m_dwMemSize,rRef.m_dwFileSize,rRef.m_sFileName.c_str());
		}
	}

	rLog.Log(" ");

	{
		rLog.Log("Dependencies:");

		std::set<SDependencyPair>::const_iterator it, end=m_Dependencies.end();

		uint32 dwCurrentAssetID=0xffffffff;
		uint32 dwSumFile=0;

		for(it=m_Dependencies.begin();it!=end;++it)
		{
			const SDependencyPair &rRef = *it;		

			if(rRef.m_idAsset!=dwCurrentAssetID)
			{
				if(dwSumFile!=0 && dwSumFile!=0xffffffff)
					rLog.Log("                                                ---> sum file: %d KB",(dwSumFile+1023)/1024);

				dwSumFile=0;

				rLog.Log(" ");
				rLog.Log(" A%d '%s' depends on", rRef.m_idAsset,m_Assets[rRef.m_idAsset].m_sFileName.c_str());
			}

			uint32 dwFileSize = m_Assets[rRef.m_idDependsOnAsset].m_dwFileSize;

			rLog.Log("        A%d file:%9d dep:%d '%s'",rRef.m_idDependsOnAsset,dwFileSize,m_Assets[rRef.m_idDependsOnAsset].m_dwDependencyCnt,m_Assets[rRef.m_idDependsOnAsset].m_sFileName.c_str());

			if(dwFileSize!=0xffffffff)
				dwSumFile += dwFileSize;

			dwCurrentAssetID=rRef.m_idAsset;
		}

		if(dwSumFile!=0 && dwSumFile!=0xffffffff)
			rLog.Log("                                                ---> sum file: %d KB",(dwSumFile+1023)/1024);
	}

	rLog.Log(" ");

	{
		rLog.Log("SourceAtoms:");

		std::set<SDependencyPair>::const_iterator it;

		while(!m_Dependencies.empty())
		{
			for(it=m_Dependencies.begin();it!=m_Dependencies.end();++it)
			{
				const SDependencyPair &rRef1 = *it;

				rLog.Log(" ");
				std::set<uint32> localDependencies;

				localDependencies.insert(rRef1.m_idAsset);

				RecursiveMove(rRef1.m_idAsset,localDependencies);

				PrintDependencySet(rLog,localDependencies);
				break;
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CResourceCollector::AddResource( const char *szFileName, const uint32 dwSize)
{
	uint32 dwNewAssetIdOrInvalid = _AddResource(szFileName,dwSize);

	return dwNewAssetIdOrInvalid!=0xffffffff;
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CResourceCollector::AddInstance( const char *_szFileName, void *pInstance )
{
	assert(pInstance);

	{
		std::set<void *>::const_iterator itInstance = m_ReportedInstances.find(pInstance);

		if(itInstance!=m_ReportedInstances.end())
			return;
	}

	string sOutputFileName = UnifyFilename(_szFileName);

	std::map<string,uint32>::const_iterator it = m_FilenameToId.find(sOutputFileName);

	if(it==m_FilenameToId.end())
	{
		OutputDebugString("ERROR: file wasn't registered with AddResource(): '");
		OutputDebugString(sOutputFileName.c_str());
		OutputDebugString("'\n");
		assert(0);		// asset wasn't registered yet AddResource() missing - unpredictable result might happen
		return;
	}

	uint32 dwAssetId = it->second;
	/*
	// debug
	char str[256];
	sprintf(str,"AddInstance: %p '",pInstance);
	OutputDebugString(str);
	OutputDebugString(sOutputFileName.c_str());
	OutputDebugString("'\n");
	*/
	++m_Assets[dwAssetId].m_dwInstanceCnt;
	m_ReportedInstances.insert(pInstance);
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CResourceCollector::OpenDependencies( const char *_szFileName )
{
	string sOutputFileName = UnifyFilename(_szFileName);

	std::map<string,uint32>::const_iterator it = m_FilenameToId.find(sOutputFileName);


	if(it==m_FilenameToId.end())
	{
		m_OpenedAssetId.push_back(0xffffffff);			// CloseDependencies() relies on that

		OutputDebugString("ERROR: file wasn't registered with AddResource(): '");
		OutputDebugString(sOutputFileName.c_str());
		OutputDebugString("'\n");
		assert(0);		// asset wasn't registered yet AddResource() missing - unpredictable result might happen
		return;
	}

	uint32 dwAssetId = it->second;

	m_OpenedAssetId.push_back(dwAssetId);
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CResourceCollector::Reset()
{
	m_Assets.resize(0);
	m_Dependencies.clear();
	m_FilenameToId.clear();
	m_OpenedAssetId.resize(0);
	m_ReportedInstances.clear();
	m_ResourceEntries.resize(0);
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CResourceCollector::CloseDependencies()
{
	assert(!m_OpenedAssetId.empty());		// internal error - OpenDependencies() should match CloseDependencies()

	m_OpenedAssetId.pop_back();
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
size_t	CResourceCollector::GetNumberOfAssets()
{
	return m_Assets.size();
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool	CResourceCollector::GetAssetName(const size_t nAssetIndex, string& rstrAssetName)
{
	if (nAssetIndex>=m_Assets.size())
	{
		return false;
	}

	rstrAssetName=m_Assets[nAssetIndex].m_sFileName;

	return true;
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool    CResourceCollector::GetAssetSize(const size_t nAssetIndex, size_t& rnAssetSize)
{
	if (nAssetIndex>=m_Assets.size())
	{
		return false;
	}

	rnAssetSize=m_Assets[nAssetIndex].m_dwMemSize;

	return false;
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
string CResourceCollector::UnifyFilename( const char *_szFileName ) const
{
	char *szFileName=(char *)_szFileName;

	// as bump and normal maps become combined during loading e.g.  blah.tif+blah_ddn.dds
	// the filename needs to be adjusted
	{
		char *pSearchForPlus = szFileName;

		while(*pSearchForPlus!=0 && *pSearchForPlus!='+')
			++pSearchForPlus;

		if(*pSearchForPlus=='+')
			szFileName=pSearchForPlus+1;
	}

	string sOutputFileName = CResourceCompilerHelper::GetOutputFilename(szFileName);

	sOutputFileName = PathUtil::ToUnixPath(sOutputFileName);
	sOutputFileName.MakeLower();

	return sOutputFileName;
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
uint32 CResourceCollector::_AddResource( const char *_szFileName, const uint32 dwSize)
{
	assert(_szFileName);

	if(_szFileName[0]==0)
		return 0xffffffff;								// no name provided - ignore this case - this often means the feature is not used

	uint32 dwNewAssetIdOrInvalid=0xffffffff;

	string sOutputFileName = UnifyFilename(_szFileName);

	std::map<string,uint32>::const_iterator it = m_FilenameToId.find(sOutputFileName);
	uint32 dwAssetId;

	if(it!=m_FilenameToId.end())
		dwAssetId = it->second;
	else
	{
		dwAssetId = m_FilenameToId.size();
		m_FilenameToId[sOutputFileName] = dwAssetId;

		SAssetEntry NewAsset;

		NewAsset.m_sFileName=sOutputFileName;

		//			if(dwSize==0xffffffff)
		{
			CCryFile file;

			if(file.Open(sOutputFileName.c_str(),"rb"))
				NewAsset.m_dwFileSize = file.GetLength();
		}

		dwNewAssetIdOrInvalid = dwAssetId;
		m_Assets.push_back(NewAsset);
	}

	SAssetEntry &rAsset = m_Assets[dwAssetId];

	if(dwSize!=0xffffffff)										// if size was specified
	{
		if(rAsset.m_dwMemSize==0xffffffff)
		{
			rAsset.m_dwMemSize=dwSize;						// store size
		}
		else
		{
			rAsset.m_dwMemSize=MAX(rAsset.m_dwMemSize,dwSize);
			//assert(rAsset.m_dwMemSize==dwSize);		// size should always be the same
		}
	}

	//		rAsset.m_dwInstanceCnt+=dwInstanceCount;

	// debugging
	//		char str[1204];
	//		sprintf(str,"_AddResource %s(size=%d cnt=%d)\n",_szFileName,rAsset.m_dwInstanceCnt,rAsset.m_dwMemSize);
	//		OutputDebugString(str);


	SInstanceEntry instance;

	instance.m_dwFileNameId=dwAssetId;

	AddDependencies(dwAssetId);

	m_ResourceEntries.push_back(instance);
	return dwNewAssetIdOrInvalid;
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CResourceCollector::AddDependencies( const uint32 dwPushedAssetId )
{
	std::vector<uint32>::const_iterator it, end=m_OpenedAssetId.end();

	for(it=m_OpenedAssetId.begin();it!=end;++it)
	{
		uint32 dwOpendedAssetId = *it;

		if(dwOpendedAssetId==0xffffffff)
			continue;		//  asset wasn't registered yet AddResource() missing

		m_Dependencies.insert(SDependencyPair(dwOpendedAssetId,dwPushedAssetId));
	}
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CResourceCollector::PrintDependencySet( ILog &rLog, const std::set<uint32> &Dep )
{
	rLog.Log("      {");
	std::set<uint32>::const_iterator it, end=Dep.end();
	uint32 dwSumFile=0;

	// iteration could be optimized
	for(it=Dep.begin();it!=end;++it)
	{
		uint32 idAsset = *it;

		uint32 dwFileSize = m_Assets[idAsset].m_dwFileSize;

		if(dwFileSize!=0xffffffff)
			dwSumFile += dwFileSize;

		rLog.Log("        A%d file:%9d dep:%d '%s'",idAsset,m_Assets[idAsset].m_dwFileSize,m_Assets[idAsset].m_dwDependencyCnt,m_Assets[idAsset].m_sFileName.c_str());
	}
	rLog.Log("      }                                           ---> sum file: %d KB",(dwSumFile+1023)/1024);
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CResourceCollector::RecursiveMove( const uint32 dwCurrentAssetID, std::set<uint32> &localDependencies )
{
	bool bProcess=true;

	// iteration could be optimized
	while(bProcess)
	{
		bProcess=false;

		std::set<SDependencyPair>::iterator it;

		for(it=m_Dependencies.begin();it!=m_Dependencies.end();++it)
		{
			SDependencyPair Pair = *it;

			if(Pair.m_idAsset==dwCurrentAssetID || Pair.m_idDependsOnAsset==dwCurrentAssetID)
			{
				uint32 idAsset = (Pair.m_idAsset==dwCurrentAssetID) ? Pair.m_idDependsOnAsset : Pair.m_idAsset;

				localDependencies.insert(idAsset);

				m_Dependencies.erase(it);

				RecursiveMove(idAsset,localDependencies);
				bProcess=true;
				break;
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////
