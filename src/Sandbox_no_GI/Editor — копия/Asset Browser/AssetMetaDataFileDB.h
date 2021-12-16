////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:	AssetMetaDataFileDB.h
//  Version:	v1.00
//  Created:	18/05/2010 15:43 by Jaewon Jung
//  Description:	This class manages persistent file DBs for cached asset meta data
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 

#ifndef __AssetMetaDataFileDB_h__
#define __AssetMetaDataFileDB_h__
#pragma once

struct IAssetDisplay;
class CAssetTags;

class CAssetMetaDataFileDB
{
public:
	
	static void StartLoadAndUpdate();
	static void StartCachingInfo( bool bOnThread = true );
	static bool IsLoadAndUpdateDone()
	{
		return s_bDbLoadingDone > 0; 
	}
	
	static bool IsCachingInfoDone()
	{
		return s_bCachingDone; 
	}

	static IAssetDisplay* GetCurrentNonThreadedAssetToCacheInfo();
	static void AbortLoadAndUpdate();
	static void AbortCachingInfo();
	static int GetCachingInfoCurrentCount()
	{
		return GetCachingInfoThread().m_currentCount;
	}

	static int GetCachingInfoTotalCount()
	{
		return GetCachingInfoThread().m_totalCount;
	}

	static bool IsCachingInfoOnThread()
	{
		return s_bIsCacheInfoOnThread;
	}

	static CAssetTags& GetTagManager();

private:

	// Actual method for loading the meta data from XML file DB and applying them
	static bool LoadAndUpdate(const CString& fileName);
	static bool CacheAssetsInfo();
	static XmlNodeRef ReadAndVerifyMainDB( const CString& fileName );
	static void ReadTransactionsAndUpdateDB( const CString& fullPath, XmlNodeRef& root );
	static void ClearTransactionsAndApplyMetaData( const CString& fullPath, XmlNodeRef& root );

	// Callback for appending newly cached meta data to the transaction DB
	static bool OnNewTransaction(const IAssetDisplay *pAssetItem);

	// A done flag for the background meta data loading and updating 
	static volatile TIntAtomic s_bDbLoadingDone;
	// A done flag for the background asset info caching and updating 
	static volatile bool s_bCachingDone;
	// A done flag for the background asset info caching and updating 
	static bool s_bIsCacheInfoOnThread;

	// A thread for the background meta data loading and updating 
	class CLoadAndUpdateThread : public CryThread<CLoadAndUpdateThread>
	{
		public:

			~CLoadAndUpdateThread()
			{
				Abort();
			}

			void Abort()
			{
				WaitForThread();
				Stop();
			}

			void Run();
	};

	// A thread for the background meta data caching and updating 
	class CCachingInfoThread : public CryThread<CCachingInfoThread>
	{
		public:

			// true if the thread or caching operation must end
			volatile bool m_bStopped;
			// the current asset processed count
			volatile int	m_currentCount,
			// total asset count
										m_totalCount;
			// this member is used when the current cached asset cannot get its info on a thread
			// the thread will set this member with that asset and it will wait until it will become NULL,
			// proof that the main thread took care of it and cached it in a Windows timer
			IAssetDisplay* m_pNonThreadedCache;

			CCachingInfoThread()
			{
				m_bStopped = true;
				m_currentCount = m_totalCount = 0;
				m_pNonThreadedCache = NULL;
			}

			~CCachingInfoThread()
			{
				Abort();
			}

			void Abort()
			{
				m_bStopped = true;
				WaitForThread();
				Stop();
			}

			void Run();
	};

	static CLoadAndUpdateThread& GetLoadAndUpdateThread()
	{
		static CLoadAndUpdateThread s_loadAndUpdateThread;
		return s_loadAndUpdateThread;
	}

	static CCachingInfoThread& GetCachingInfoThread()
	{
		static CCachingInfoThread s_cachingInfoThread;
		return s_cachingInfoThread;
	}

	static CAssetTags s_tagManager;
};

#endif //__AssetMetaDataFileDB_h__
