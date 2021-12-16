////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2012.
// -------------------------------------------------------------------------
//  File name:   FileSystemSearcher.h
//  Version:     v1.00
//  Created:     11/12/2012 by Paul Reindell.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __FileSystemSearcher_H__
#define __FileSystemSearcher_H__

#include "IAssetSearcher.h"

////////////////////////////////////////////////////////////////////////////
struct SFileSystemAssetDesc
{
	SFileSystemAssetDesc();
	SFileSystemAssetDesc(const char* _typeName, char _varType = IVariable::DT_SIMPLE);

	void AddExt(const char* ext);
	const char* GetTypeName() const;
	int GetExtCount() const;
	const char* GetExt(int i) const;
	char GetVarType() const;

	bool Accept(const char* ext) const;
	bool Accept(char _varType) const;

private:
	std::vector<CString> extensions;
	CString typeName;
	char varType;
};
typedef std::map<int, SFileSystemAssetDesc> TFileSystemAssetTypes;

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
struct SFileSystemSearchFolder;
struct SFileSystemSearchRequest
{
	SFileSystemSearchRequest(const char* file);
	~SFileSystemSearchRequest();

	void AddRef();
	void Release();

	bool GetResult(std::vector<CString>& result, bool& doneSearching);
	void AddResult(const CString& filepath);

	inline int GetPrio() const { return m_currPrio; }
	static int GetMinPrio();

	inline const CString& GetFile() const { return m_file; }

	void FolderVisited(SFileSystemSearchFolder* pFolder);
	bool AlreadyVisited(SFileSystemSearchFolder* pFolder) const;

public:
	enum EState
	{
		eAdded,
		eProcessing,
		eRemoved,
	};

	volatile EState state;

	static const int MAX_PRIO_COUNT = 10;

private:
	CryCriticalSection m_lock;
	CString m_file;
	volatile int m_refCount;
	int m_currPrio;
	std::vector<CString> m_foundFiles;
	std::vector<SFileSystemSearchFolder*> m_folderVisited;

	static volatile int s_totalCount;

	static volatile int s_minPrio[MAX_PRIO_COUNT];
};

TYPEDEF_AUTOPTR(SFileSystemSearchRequest);
typedef SFileSystemSearchRequest_AutoPtr TFileSystemSearchRequestPtr;

typedef std::vector<TFileSystemSearchRequestPtr> TFileSystemReqFiles;
typedef std::map< IAssetSearcher::TAssetSearchId, std::pair<SFileSystemSearchRequest*, CString> > TFileSystemRequests;

////////////////////////////////////////////////////////////////////////////
struct SFileSystemSearchFolder
{
	SFileSystemSearchFolder(const CString& path, SFileSystemSearchFolder* pParent = NULL);
	~SFileSystemSearchFolder();

	void AddRequest(TFileSystemSearchRequestPtr req, const CString& path);
	void Process();
	bool IsSearching() const;

private:
	SFileSystemSearchFolder* GetOrCreateSubdir(const CString& subdir);
	void AddFileSearch(TFileSystemSearchRequestPtr pSearch, SFileSystemSearchFolder* pSender);
	const char* GetFolderName() const;

private:
	CString m_path;
	CString m_name;
	TFileSystemReqFiles m_filesToSearch;

	typedef std::list<SFileSystemSearchFolder*> TChildren;
	TChildren m_subFolders;
	SFileSystemSearchFolder* m_pParent;
};

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
class CFileSystemSearcher : public IAssetSearcher, public CrySimpleThread<>
{
public:
	CFileSystemSearcher();
	virtual ~CFileSystemSearcher();

	virtual const char* GetName() const { return "FileSystem"; }

	virtual bool Accept(const char* asset, int& assetTypeId) const;
	virtual bool Accept(const char varType, int& assetTypeId) const;

	virtual const char* GetAssetTypeName(int assetTypeId);

	virtual bool Exists(const char* asset, int assetTypeId) const;
	virtual bool GetReplacement(CString& replacement, int assetTypeId);

	virtual void StartSearcher();
	virtual void StopSearcher();

	virtual TAssetSearchId AddSearch(const char* asset, int assetTypeId);
	virtual void CancelSearch(TAssetSearchId id);
	virtual bool GetResult(TAssetSearchId id, std::vector<CString>& result, bool& doneSearching);

public:
	// CrySimpleThread
	virtual void Run();
	// ~CrySimpleThread

private:
	TAssetSearchId GetNextFreeId() const;

private:
	CString m_lastPath;
	TFileSystemAssetTypes m_assetTypes;
	TFileSystemRequests m_requests;
	SFileSystemSearchFolder* m_pRoot;

	CryCriticalSection m_lock;
};

////////////////////////////////////////////////////////////////////////////
#endif //#ifndef __FileSystemSearcher_H__