#pragma once
////////////////////////////////////////////////////////////////////////////
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2012.
////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <set>
#include "IAssetItem.h"
#include "IAssetItemDatabase.h"
#include "IAssetTagging.h"

// Asset items vector
typedef std::vector<IAssetItem*> TAssetItems;
// Asset database vector
typedef std::vector<IAssetItemDatabase*> TAssetDatabases;

class CAssetBrowserManager
	: public IEditorNotifyListener
	, public CryThread<CAssetBrowserManager>
{
public:
	typedef std::vector<CString> StrVector;
	typedef bool (*TPfnOnUpdateCacheProgress)(int totalProgressPercent, const char* pMessage);

	static CAssetBrowserManager* Instance();
	static void DeleteInstance();
	static bool IsInstanceExist();

	CAssetBrowserManager();
	~CAssetBrowserManager();
	
	//////////////////////////////////////////////////////////////////////////
	// Initialize/shutdown and caching
	//////////////////////////////////////////////////////////////////////////
	void Initialize();
	void Shutdown();
	bool LoadCache();
	void CreateThumbsFolderPath();
	bool CacheAssets(TAssetDatabases dbsToCache, bool bForceCache = false, TPfnOnUpdateCacheProgress pProgressCallback = NULL);
	void MarkUsedInLevelAssets();

	//////////////////////////////////////////////////////////////////////////
	// Asset tagging
	//////////////////////////////////////////////////////////////////////////
	void InitializeTagging();
	int CreateTag(const CString& tag, const CString& category);
	int CreateAsset(const CString& asset, const CString& project);
	int CreateProject(const CString& project);
	void AddAssetsToTag(const CString& tag, const CString& category, const StrVector& assets);
	void AddTagsToAsset(const CString& asset, const CString& category, const CString& tags);
	void RemoveAssetsFromTag(const CString& tag, const CString& category, const StrVector& assets);
	void RemoveTagFromAsset(const CString& tag, const CString& category, const CString& asset);
	void DestroyTag(const CString& tag);
	int TagExists(const CString& tag, const CString& category);
	int AssetExists(const CString& relpath);
	int ProjectExists(const CString& project);
	CString GetProjectName();
	bool GetAssetDescription(const CString& relpath, CString& description);
	void SetAssetDescription(const CString& relpath, const CString& description);
	int GetAllTags(StrVector& tags);
	int GetAllTagCategories(StrVector& categories);
	int GetTagsForCategory(const CString& category, StrVector& tags);
	int GetTagsForAsset(StrVector& tags, const CString& asset);
	int GetTagForAssetInCategory(CString& tag, const CString& asset, const CString& category);
	int GetAssetsForTag(StrVector& assets, const CString& tag);
	int GetAssetCountForTag(const CString& tag);
	int GetAssetsWithDescription(StrVector& assets, const CString& description);
	bool GetAutocompleteDescription(const CString& partDesc, CString& description);

	//////////////////////////////////////////////////////////////////////////
	// Misc.
	//////////////////////////////////////////////////////////////////////////
	TAssetDatabases& GetAssetDatabases()
	{
		return m_assetDatabases;
	}
	
	IAssetItemDatabase* GetDatabaseByName(const char* pName);

	void EnqueueAssetForThumbLoad(IAssetItem* pAsset);
	
	void OnObjectEvent(CBaseObject* pObject, int nEvent);
	void OnEditorNotifyEvent(EEditorNotifyEvent event);

	void Run();

private:
	void CreateAssetDatabases();
	void FreeAssetDatabases();
	XmlNodeRef ReadAndVerifyMainDB(const CString& fileName);
	void ReadTransactionsAndUpdateDB(const CString& fullPath, XmlNodeRef& root);
	void ClearTransactionsAndApplyMetaData(const CString& fullPath, XmlNodeRef& root);
	// Callback for appending newly cached meta data to the transaction DB
	static bool OnNewTransaction(const IAssetItem* pAssetItem);

	std::set<IAssetItem*> m_thumbLoadQueue;
	TAssetDatabases m_assetDatabases;
	bool m_bLevelLoading;
	uint32 m_cachedAssetCount;
};

namespace AssetBrowser
{
unsigned int HashStringSbdm(const char *pStr);
}
