////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:   ResourceCollector.h
//  Created:     09/10/2009 by Paulo Zaffari.
//  Description: This class is the resource collector for the CrySizer
//							 element used in GetResourcememoryUsage.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef ResourceCollector_h__
#define ResourceCollector_h__

#pragma once

#include "IResourceCollector.h"
#include "ILog.h"

class CResourceCollector :public IResourceCollector
{	
	struct SInstanceEntry
	{
		//		AABB			m_AABB;
		uint32		m_dwFileNameId;				// use with m_FilenameToId,  m_IdToFilename
	};

	struct SAssetEntry
	{
		SAssetEntry() :m_dwInstanceCnt(0), m_dwDependencyCnt(0), m_dwMemSize(0xffffffff), m_dwFileSize(0xffffffff)
		{
		}

		string		m_sFileName;
		uint32		m_dwInstanceCnt;			// 1=this asset is used only once in the level, 2, 3, ...
		uint32		m_dwDependencyCnt;		// 1=this asset is only used by one asset, 2, 3, ...
		uint32		m_dwMemSize;					// 0xffffffff if unknown (only needed to verify disk file size)
		uint32		m_dwFileSize;					// 0xffffffff if unknown
	};

public:	// -----------------------------------------------------------------------------

	CResourceCollector();

	// compute m_dwDependencyCnt
	void ComputeDependencyCnt();

	// watch out: this function modifies internal data
	void LogData( ILog &rLog );

	//////////////////////////////////////////////////////////////////////////
	// interface IResourceCollector ------------------------------------------
	bool AddResource( const char *szFileName, const uint32 dwSize=0xffffffff );
	void AddInstance( const char *_szFileName, void *pInstance );
	void OpenDependencies( const char *_szFileName );
	void Reset();
	void CloseDependencies();
	//////////////////////////////////////////////////////////////////////////

	size_t	GetNumberOfAssets();
	bool	GetAssetName(const size_t nAssetIndex, string& rstrAssetName);
	bool    GetAssetSize(const size_t nAssetIndex, size_t& rnAssetSize);
private: 
	//////////////////////////////////////////////////////////////////////////
	string UnifyFilename( const char *_szFileName ) const;

	// Returns:
	//   0xffffffff if asset was already known (m_dwInstanceCnt will be increased), AssetId otherwise
	uint32 _AddResource( const char *_szFileName, const uint32 dwSize=0xffffffff );

	void AddDependencies( const uint32 dwPushedAssetId );

	void PrintDependencySet( ILog &rLog, const std::set<uint32> &Dep );

	// find all dependencies to it and move to localDependencies
	void RecursiveMove( const uint32 dwCurrentAssetID, std::set<uint32> &localDependencies );


	struct SDependencyPair
	{
		SDependencyPair( const uint32 idAsset, const uint32 idDependsOnAsset ) :m_idAsset(idAsset), m_idDependsOnAsset(idDependsOnAsset)
		{
		}

		uint32			m_idAsset;							// AssetID
		uint32			m_idDependsOnAsset;			// AssetID

		bool operator<( const SDependencyPair &rhs ) const
		{
			if(m_idAsset<rhs.m_idAsset) return true;
			if(m_idAsset>rhs.m_idAsset) return false;

			return m_idDependsOnAsset<rhs.m_idDependsOnAsset;
		}
	};

	std::vector<uint32>							m_OpenedAssetId;			// to track for dependencies
	std::map<string,uint32>					m_FilenameToId;				// could be done more efficiently
	std::vector<SAssetEntry>				m_Assets;							// could be done more efficiently
	std::vector<SInstanceEntry>			m_ResourceEntries;		//
	std::set<SDependencyPair>				m_Dependencies;				//
	std::set<void *>								m_ReportedInstances;	// to avoid counting them twice
};

#endif // ResourceCollector_h__
