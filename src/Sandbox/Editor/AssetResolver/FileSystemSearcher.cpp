////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2012.
// -------------------------------------------------------------------------
//  File name:   FileSystemSearcher.cpp
//  Version:     v1.00
//  Created:     11/12/2012 by Paul Reindell.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "FileSystemSearcher.h"
#include <StringUtils.h>


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
SFileSystemAssetDesc::SFileSystemAssetDesc()
	: typeName("UNDEFINED")
	, varType(IVariable::DT_SIMPLE) 
{

}

////////////////////////////////////////////////////////////////////////////
SFileSystemAssetDesc::SFileSystemAssetDesc(const char* _typeName, char _varType) 
	: typeName(_typeName)
	, varType(_varType) 
{

}

////////////////////////////////////////////////////////////////////////////
void SFileSystemAssetDesc::AddExt(const char* ext)
{
	extensions.push_back(ext);
}

////////////////////////////////////////////////////////////////////////////
const char* SFileSystemAssetDesc::GetTypeName() const
{
	return typeName.GetString();
}

////////////////////////////////////////////////////////////////////////////
int SFileSystemAssetDesc::GetExtCount() const
{
	return extensions.size();
}

////////////////////////////////////////////////////////////////////////////
const char* SFileSystemAssetDesc::GetExt(int i) const
{
	return i>=0 && i<extensions.size() ? extensions[i].GetString() : "";
}

////////////////////////////////////////////////////////////////////////////
char SFileSystemAssetDesc::GetVarType() const
{
	return varType;
}

////////////////////////////////////////////////////////////////////////////
bool SFileSystemAssetDesc::Accept(const char* ext) const
{
	for (int i = 0; i < GetExtCount(); ++i)
	{
		if (strcmpi(ext, GetExt(i)) == 0)
			return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////
bool SFileSystemAssetDesc::Accept(char _varType) const
{
	return varType != IVariable::DT_SIMPLE && varType == _varType;
}


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
volatile int SFileSystemSearchRequest::s_totalCount = 0;
volatile int SFileSystemSearchRequest::s_minPrio[SFileSystemSearchRequest::MAX_PRIO_COUNT];

////////////////////////////////////////////////////////////////////////////
SFileSystemSearchRequest::SFileSystemSearchRequest(const char* file)
	: m_refCount(1)
	, m_file(file)
	, m_currPrio(0)
	, state(eAdded)
{
	 long refCnt = CryInterlockedIncrement(&s_totalCount);
	 CryInterlockedIncrement(&s_minPrio[m_currPrio]);
}

////////////////////////////////////////////////////////////////////////////
SFileSystemSearchRequest::~SFileSystemSearchRequest()
{
	long refCnt = CryInterlockedDecrement(&s_totalCount);
	CryInterlockedDecrement(&s_minPrio[m_currPrio]);
}

////////////////////////////////////////////////////////////////////////////
void SFileSystemSearchRequest::AddRef()
{
	CryInterlockedIncrement(&m_refCount);
}

////////////////////////////////////////////////////////////////////////////
void SFileSystemSearchRequest::Release()
{
	long refCnt = CryInterlockedDecrement(&m_refCount);
	if (refCnt <= 0)
		delete this;
}

////////////////////////////////////////////////////////////////////////////
bool SFileSystemSearchRequest::GetResult(std::vector<CString>& result, bool& doneSearching)
{
	CryAutoCriticalSection lock(m_lock);
	bool res = !m_foundFiles.empty();
	for (std::vector<CString>::iterator it = m_foundFiles.begin(); it != m_foundFiles.end(); ++it)
		result.push_back(*it);
	m_foundFiles.clear();
	doneSearching = state == eProcessing && m_refCount == 1;
	return res;
}

////////////////////////////////////////////////////////////////////////////
void SFileSystemSearchRequest::AddResult(const CString& filepath)
{
	CryAutoCriticalSection lock(m_lock);
	m_foundFiles.push_back(filepath);
	CryInterlockedDecrement(&s_minPrio[m_currPrio]);
	m_currPrio = min(m_currPrio+1, MAX_PRIO_COUNT-1);
	CryInterlockedIncrement(&s_minPrio[m_currPrio]);
}

////////////////////////////////////////////////////////////////////////////
int SFileSystemSearchRequest::GetMinPrio()
{
	for (int i = 0; i < MAX_PRIO_COUNT; ++i)
	{
		if (s_minPrio[i] > 0)
			return i;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////
void SFileSystemSearchRequest::FolderVisited(SFileSystemSearchFolder* pFolder)
{
	m_folderVisited.push_back(pFolder);
}

////////////////////////////////////////////////////////////////////////////
bool SFileSystemSearchRequest::AlreadyVisited(SFileSystemSearchFolder* pFolder) const
{
	return stl::find(m_folderVisited, pFolder);
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
SFileSystemSearchFolder::SFileSystemSearchFolder(const CString& path, SFileSystemSearchFolder* pParent)
	: m_pParent(pParent)
	, m_path(path)
{
	m_path.TrimLeft('/');
	m_path.TrimRight('/');
	m_path.MakeLower();
	m_name = GetFolderName();
}

////////////////////////////////////////////////////////////////////////////
SFileSystemSearchFolder::~SFileSystemSearchFolder()
{
	for (TChildren::iterator it = m_subFolders.begin(); it != m_subFolders.end(); ++it)
		delete *it;
}

////////////////////////////////////////////////////////////////////////////
void SFileSystemSearchFolder::AddRequest(TFileSystemSearchRequestPtr req, const CString& path)
{
	int pos = path.Find('/');
	if (pos > 0)
	{
		CString remaining = path.Mid(pos+1);
		pos = remaining.Find('/');
		CString next = pos > 0 ? remaining.Left(pos) : remaining;

		SFileSystemSearchFolder* pFolder = GetOrCreateSubdir(next);
		pFolder->AddRequest(req, remaining);
	}
	else
	{
		req->state = SFileSystemSearchRequest::eProcessing;
		m_filesToSearch.push_back(req);
	}
}

////////////////////////////////////////////////////////////////////////////
void SFileSystemSearchFolder::Process()
{
	int minPrio = SFileSystemSearchRequest::GetMinPrio();
	int folderPrio = SFileSystemSearchRequest::MAX_PRIO_COUNT;
	for (TFileSystemReqFiles::iterator it = m_filesToSearch.begin(); it != m_filesToSearch.end();)
	{
		if ((*it)->state == SFileSystemSearchRequest::eRemoved)
			it = m_filesToSearch.erase(it);
		else
			folderPrio = min((*it++)->GetPrio(), folderPrio);
	}
	
	if (folderPrio <= minPrio)
	{
		ICryPak * pCryPak = gEnv->pCryPak;
		_finddata_t fd;

		CString search = m_path + "/*.*";
		intptr_t handle = pCryPak->FindFirst( search.GetString(), &fd );
		if ( handle != -1 )
		{
			int res = 0;
			do 
			{
				if (fd.name[0] != 0 && (fd.attrib & _A_SUBDIR) != 0 && strcmpi(fd.name, ".") != 0 && strcmpi(fd.name, "..") != 0)
				{
					CString dir = fd.name;
					dir.MakeLower();
					GetOrCreateSubdir(dir);
				}
				else
				{
					for (TFileSystemReqFiles::iterator it = m_filesToSearch.begin(); it != m_filesToSearch.end(); ++it)
					{
						if (strcmpi(fd.name, (*it)->GetFile().GetString()) == 0)
						{
							CString filepath = m_path + "/";
							int p = filepath.Find('/');
							filepath = filepath.Mid(p+1); // remove the game folder!
							filepath += fd.name;
							(*it)->AddResult(filepath);
						}
					}
				}
				res = pCryPak->FindNext( handle, &fd );
			}
			while ( res >= 0 );
			pCryPak->FindClose( handle );
		}

		for (TFileSystemReqFiles::iterator it = m_filesToSearch.begin(); it != m_filesToSearch.end(); ++it)
		{
			(*it)->FolderVisited(this);
			if (m_pParent)
				m_pParent->AddFileSearch(*it, this);
			for (TChildren::iterator sub = m_subFolders.begin(); sub != m_subFolders.end(); ++sub)
				(*sub)->AddFileSearch(*it, this);
		}

		m_filesToSearch.clear();
	}

	for (TChildren::iterator it = m_subFolders.begin(); it != m_subFolders.end();)
	{
		SFileSystemSearchFolder* subfolder = (*it);

		if (subfolder->IsSearching())
		{
			subfolder->Process();
			++it;
		}
		else
		{
			SAFE_DELETE(subfolder);
			it = m_subFolders.erase(it);
		}
	}
}

////////////////////////////////////////////////////////////////////////////
SFileSystemSearchFolder* SFileSystemSearchFolder::GetOrCreateSubdir(const CString& subdir)
{
	for (TChildren::iterator it = m_subFolders.begin(); it != m_subFolders.end(); ++it)
	{
		if ((*it)->m_name == subdir)
			return *it;
	}
	CString path = m_path + "/";
	path += subdir;
	SFileSystemSearchFolder* pNewFolder = new SFileSystemSearchFolder(path, this);
	m_subFolders.push_back(pNewFolder);
	return pNewFolder;
}

////////////////////////////////////////////////////////////////////////////
void SFileSystemSearchFolder::AddFileSearch(TFileSystemSearchRequestPtr pSearch, SFileSystemSearchFolder* pSender)
{
	if (pSearch->AlreadyVisited(this))
	{
		if (m_pParent && m_pParent != pSender)
			m_pParent->AddFileSearch(pSearch, this);
		for (TChildren::iterator it = m_subFolders.begin(); it != m_subFolders.end(); ++it)
		{
			if (*it != pSender)
				(*it)->AddFileSearch(pSearch, this);
		}
		return;
	}

	stl::push_back_unique(m_filesToSearch, pSearch);
}

////////////////////////////////////////////////////////////////////////////
const char* SFileSystemSearchFolder::GetFolderName() const
{
	const char* res = m_path.GetString();
	const char* folder = res;
	while (strlen(folder) > 1)
	{
		if (*folder == '/')
			res = folder+1;
		folder++;
	}
	return res;
}

bool SFileSystemSearchFolder::IsSearching() const
{
	bool isSearching = !m_filesToSearch.empty();
	for (TChildren::const_iterator it = m_subFolders.begin(); it != m_subFolders.end(); ++it)
	{
		isSearching |= (*it)->IsSearching();
	}

	return isSearching;
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
CFileSystemSearcher::CFileSystemSearcher()
{
	m_lastPath = Path::GetGameFolder();
	m_pRoot = new SFileSystemSearchFolder(m_lastPath);

	m_assetTypes[0] = SFileSystemAssetDesc("Object", IVariable::DT_OBJECT);
	m_assetTypes[0].AddExt("cgf");

	m_assetTypes[1] = SFileSystemAssetDesc("Texture", IVariable::DT_TEXTURE);
	m_assetTypes[1].AddExt("tif");
	m_assetTypes[1].AddExt("dds");

	m_assetTypes[2] = SFileSystemAssetDesc("Animation", IVariable::DT_ANIMATION);
	m_assetTypes[2].AddExt("caf");

	m_assetTypes[3] = SFileSystemAssetDesc("SoundFile");
	m_assetTypes[3].AddExt("mp2");
	m_assetTypes[3].AddExt("mp3");
	m_assetTypes[3].AddExt("wav");

	m_assetTypes[4] = SFileSystemAssetDesc("Scripts");
	m_assetTypes[4].AddExt("lua");

	m_assetTypes[5] = SFileSystemAssetDesc("XML");
	m_assetTypes[5].AddExt("xml");

	m_assetTypes[6] = SFileSystemAssetDesc("Material", IVariable::DT_MATERIAL);
	m_assetTypes[6].AddExt("mtl");
}

////////////////////////////////////////////////////////////////////////////
CFileSystemSearcher::~CFileSystemSearcher()
{
	WaitForThread();
	SAFE_DELETE(m_pRoot);
}

////////////////////////////////////////////////////////////////////////////
bool CFileSystemSearcher::Accept(const char* asset, int& assetTypeId) const
{
	const char* pExt = CryStringUtils::FindExtension(asset);
	for (TFileSystemAssetTypes::const_iterator it = m_assetTypes.begin(); it != m_assetTypes.end(); ++it)
	{
		if (it->second.Accept(pExt))
		{
			assetTypeId = it->first;
			return true;
		}
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////
bool CFileSystemSearcher::Accept(const char varType, int& assetTypeId) const 
{
	for (TFileSystemAssetTypes::const_iterator it = m_assetTypes.begin(); it != m_assetTypes.end(); ++it)
	{
		if (it->second.Accept(varType))
		{
			assetTypeId = it->first;
			return true;
		}
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////
const char* CFileSystemSearcher::GetAssetTypeName(int assetTypeId)
{
	TFileSystemAssetTypes::const_iterator it = m_assetTypes.find(assetTypeId);
	return it != m_assetTypes.end() ? it->second.GetTypeName() : "UNDEFINED";
}

////////////////////////////////////////////////////////////////////////////
bool CFileSystemSearcher::Exists(const char* asset, int assetTypeId) const {
	CString assetName = asset;
	const char* pExt = PathUtil::GetExt(asset);

	TFileSystemAssetTypes::const_iterator it = m_assetTypes.find(assetTypeId);
	assert(it != m_assetTypes.end());

	if (strcmp(pExt, "") == 0)
	{
		assetName += ".";
		assetName += it->second.GetExt(0);
	}

	bool fileExists = gEnv->pCryPak->IsFileExist(assetName.GetString());

	if (!fileExists && (it->second.GetVarType() == IVariable::DT_TEXTURE))
	{
		// check for the dds version of a tif file or vice versa
		assert(it->second.GetExtCount() == 2);
		const int ext = (strcmp(pExt, it->second.GetExt(0)) == 0) ? 1 : 0;
		fileExists = gEnv->pCryPak->IsFileExist(PathUtil::ReplaceExtension(assetName.GetString(), it->second.GetExt(ext)));
	}

	return fileExists;
}

////////////////////////////////////////////////////////////////////////////
bool CFileSystemSearcher::GetReplacement(CString& replacement, int assetTypeId)
{
	TFileSystemAssetTypes::const_iterator it = m_assetTypes.find(assetTypeId);
	assert(it != m_assetTypes.end());

	string filter; // would like to use CString, but it crashes (I assume due to pass GetString() to itself)
	for (int i = 0; i < it->second.GetExtCount(); ++i)
	{
		filter.Format("%s%s%s (*.%s)|*.%s",
			filter.c_str(),
			filter.empty() ? "" : "|",
			it->second.GetTypeName(),
			it->second.GetExt(i),
			it->second.GetExt(i));
	}

	filter.Format("%s%sAll files (*.*)|*.*",
		filter.c_str(),
		filter.empty() ? "" : "|");

	CString fullFileName;
	if(CFileUtil::SelectFile(filter.c_str(), m_lastPath, fullFileName))
	{
		m_lastPath = PathUtil::GetPath(string(fullFileName.GetString())).c_str();
		string gamePath = Path::FullPathToGamePath(fullFileName).GetString();
		gamePath = PathUtil::ToUnixPath(gamePath);
		replacement = gamePath.c_str();
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////
void CFileSystemSearcher::StartSearcher()
{
	Start();
}

////////////////////////////////////////////////////////////////////////////
void CFileSystemSearcher::StopSearcher()
{
	Stop();

	CryAutoCriticalSection lock(m_lock);
	for (TFileSystemRequests::iterator it = m_requests.begin(); it != m_requests.end(); ++it)
		it->second.first->Release();
	m_requests.clear();
}

////////////////////////////////////////////////////////////////////////////
IAssetSearcher::TAssetSearchId CFileSystemSearcher::AddSearch(const char* asset, int assetTypeId)
{
	CString assetName = asset;
	const char* pExt = CryStringUtils::FindExtension(asset);
	if (strcmp(pExt, "") == 0)
	{
		TFileSystemAssetTypes::const_iterator it = m_assetTypes.find(assetTypeId);
		assert(it != m_assetTypes.end());
		assetName += ".";
		assetName += it->second.GetExt(0);
	}
	assetName.MakeLower();

	string file = PathUtil::GetFile(assetName.GetString());
	string path = PathUtil::GetPath(assetName.GetString());
	path = PathUtil::ToUnixPath(path);
	path.TrimLeft('/');
	path.TrimRight('/');

	CryAutoCriticalSection lock(m_lock);
	TAssetSearchId id = GetNextFreeId();
	m_requests[id] = std::make_pair(new SFileSystemSearchRequest(file.c_str()), path.c_str());
	return id;
}

////////////////////////////////////////////////////////////////////////////
void CFileSystemSearcher::CancelSearch(TAssetSearchId id)
{
	CryAutoCriticalSection lock(m_lock);
	TFileSystemRequests::iterator it = m_requests.find(id);
	assert(it != m_requests.end());
	it->second.first->state = SFileSystemSearchRequest::eRemoved;
	it->second.first->Release();
	m_requests.erase(it);
}

////////////////////////////////////////////////////////////////////////////
bool CFileSystemSearcher::GetResult(TAssetSearchId id, std::vector<CString>& result, bool& doneSearching)
{
 	CryAutoCriticalSection lock(m_lock);
	TFileSystemRequests::iterator it = m_requests.find(id);
	assert(it != m_requests.end());
	return it->second.first->GetResult(result, doneSearching);
}

////////////////////////////////////////////////////////////////////////////
void CFileSystemSearcher::Run()
{
	SetName("FileSystemSearcher");

	while (IsStarted())
	{
		m_pRoot->Process();

		CryAutoCriticalSection lock(m_lock);
		for (TFileSystemRequests::iterator it = m_requests.begin(); it != m_requests.end(); ++it)
		{
			if (it->second.first->state == SFileSystemSearchRequest::eAdded)
			{
				m_pRoot->AddRequest(it->second.first, it->second.second);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////
IAssetSearcher::TAssetSearchId CFileSystemSearcher::GetNextFreeId() const
{
	TAssetSearchId id = IAssetSearcher::FIRST_VALID_ID;
	for (TFileSystemRequests::const_iterator it = m_requests.begin(); it != m_requests.end() && it->first == id; ++it, ++id);
	return id;
}

////////////////////////////////////////////////////////////////////////////