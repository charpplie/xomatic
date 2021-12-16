//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2010.
// -------------------------------------------------------------------------
//  File Name        : AssetTags.cpp
//  Author           : Jaewon Jung
//  Time of creation : 7/16/2010   15:16
//  Compilers        : VS2008
//  Description      : This class manages tags assigned to each asset.
//                     This also deals with sharing tag files via Perforce.
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "AssetTags.h"
#include "ISourceControl.h"
#include "SourceControlDescDlg.h"

namespace
{
	const char TAGS_FOLDER_PATH[] = "\\Editor\\Tags\\";
}

void CAssetTags::Initialize()
{
	char buf[ICryPak::g_nMaxPath];
	buf[sizeof(buf) - 1] = 0;
	CryGetCurrentDirectory(ICryPak::g_nMaxPath, buf);
	CString path = buf;
	path += TAGS_FOLDER_PATH;
	CFileUtil::FileArray files;
	CFileUtil::ScanDirectory(path, "*.*", files, false);
	m_tagFolderPath = path;

	for(size_t i=0; i<files.size(); ++i)
	{
		LoadTag(files[i].filename);
		m_tagTimeStamp[files[i].filename] = files[i].time_write;
	}
}

void CAssetTags::ReloadTagFilesIfNecessary()
{
	assert(m_tagFolderPath.IsEmpty() == false);

	CFileUtil::FileArray files;
	CFileUtil::ScanDirectory(m_tagFolderPath, "*.*", files, false);

	for(size_t i=0; i<files.size(); ++i)
	{
		time_t time = 0;
		CString tag = files[i].filename;
		if(m_tagTimeStamp.find(tag) != m_tagTimeStamp.end())
			time = m_tagTimeStamp[tag];
		if(time != files[i].time_write)
		{
			LoadTag(tag);
			m_tagTimeStamp[tag] = files[i].time_write;
		}
	}
}

void CAssetTags::CreateTag(const CString& tag, bool shared)
{
	assert(m_tagFolderPath.IsEmpty() == false);
	assert(m_tagMap.find(tag) == m_tagMap.end());
	m_tagMap[tag] = StrSet();

	CString fullPath = m_tagFolderPath;
	fullPath += tag;
	assert(CFileUtil::FileExists(fullPath) == false);
	// Create a new tag file.
	FILE *f = fopen(fullPath, "wt");
	fclose(f);

	CFileUtil::FileDesc fileDesc;
	bool ok = CFileUtil::FileExists(fullPath, &fileDesc);
	assert(ok);
	m_tagTimeStamp[tag] = fileDesc.time_write;
#if !defined(IS_CRYDEV)
	if(shared)
	{
		if(gSettings.enableSourceControl == false)
		{
			AfxMessageBox("Cannot make the tag file shared.\nPlease enable Source Control in the preference.");
			return;
		}

		if(GetIEditor()->GetSourceControl() == NULL)
		{
			AfxMessageBox("Cannot make the tag file shared.\nPlease make sure if you have Perforce Plug-in for Sandbox set up properly.");
			return;
		}

		CSourceControlDescDlg dlg;
		if(dlg.DoModal() == IDOK)
		{
			bool ok = GetIEditor()->GetSourceControl()->Add(fullPath, dlg.m_sDesc);
			if(ok == false)
				AfxMessageBox("Cannot make the tag file shared.\nPlease make sure if Source Control Provider correctly setup and working directory is correct.");
		}
		else
				AfxMessageBox("The new tag will not be shared.");
	}
#endif
}

void CAssetTags::AddAssetsToTag(const CString& tag, const StrVector& assets)
{
	assert(m_tagFolderPath.IsEmpty() == false);
	assert(m_tagMap.find(tag) != m_tagMap.end());
	
	CString fullPath = m_tagFolderPath;
	fullPath += tag;
	bool isShared = IsVersionControlled(fullPath);
	if(isShared)
	{
		isShared = GetLatestCheckOutAndReload(fullPath);
	}

	StrSet& assetSet = m_tagMap[tag];
	for(size_t i=0; i<assets.size(); ++i)
		assetSet.insert(assets[i]);

	SaveTag(tag);

	if(isShared)
	{
		CheckIn(fullPath);
	}
}

void CAssetTags::RemoveAssetsFromTag(const CString& tag, const StrVector& assets)
{
	assert(m_tagFolderPath.IsEmpty() == false);
	assert(m_tagMap.find(tag) != m_tagMap.end());
	
	CString fullPath = m_tagFolderPath;
	fullPath += tag;
	bool isShared = IsVersionControlled(fullPath);
	if(isShared)
	{
		isShared = GetLatestCheckOutAndReload(fullPath);
	}

	StrSet& assetSet = m_tagMap[tag];
	for(size_t i=0; i<assets.size(); ++i)
		assetSet.erase(assets[i]);

	SaveTag(tag);

	if(isShared)
	{
		CheckIn(fullPath);
	}
}

void CAssetTags::DestroyTag(const CString& tag)
{
	assert(m_tagFolderPath.IsEmpty() == false);
	assert(m_tagMap.find(tag) != m_tagMap.end());
	m_tagMap.erase(tag);
	m_tagTimeStamp.erase(tag);

	CString fullPath = m_tagFolderPath;
	fullPath += tag;
	assert(CFileUtil::FileExists(fullPath) == true);
#if !defined(IS_CRYDEV)
	if(IsVersionControlled(fullPath))
	{
		CSourceControlDescDlg dlg;
		if(dlg.DoModal() == IDOK)
		{
			bool ok = GetIEditor()->GetSourceControl()->Delete(fullPath, dlg.m_sDesc);
			if(ok == false)
				AfxMessageBox("Cannot delete the tag file from Source Control.\nPlease make sure if Source Control Provider correctly setup and working directory is correct.");
		}
		else
			AfxMessageBox("The shared tag has been deleted only locally.");
	}
#endif
	CFileUtil::DeleteFile(fullPath);
}

int CAssetTags::GetTagsForAsset(StrVector& tags, const CString& asset) const
{
	assert(m_tagFolderPath.IsEmpty() == false);

	tags.clear();
	for(TagMap::const_iterator itr=m_tagMap.begin(), itrEnd=m_tagMap.end(); itr!=itrEnd; ++itr)
	{
		if(itr->second.find(asset) != itr->second.end())
			tags.push_back(itr->first);
	}
	return int(tags.size());
}

int CAssetTags::GetAssetsForTag(StrVector& assets, const CString& tag) const
{
	assert(m_tagFolderPath.IsEmpty() == false);

	assets.clear();
	TagMap::const_iterator itr = m_tagMap.find(tag);
	if(itr != m_tagMap.end())
	{
		const StrSet& assetSet = itr->second;
		assets.reserve(itr->second.size());
		for(StrSet::const_iterator itr=assetSet.begin(), itrEnd=assetSet.end(); itr!=itrEnd; ++itr)
			assets.push_back(*itr);
	}
	return int(assets.size());
}

int CAssetTags::GetAssetCountForTag(const CString& tag) const
{
	assert(m_tagFolderPath.IsEmpty() == false);

	TagMap::const_iterator itr = m_tagMap.find(tag);
	if(itr != m_tagMap.end())
		return int(itr->second.size());
	else
		return 0;
}

int CAssetTags::GetAllTags(StrVector& tags) const
{
	assert(m_tagFolderPath.IsEmpty() == false);

	tags.clear();
	tags.reserve(m_tagMap.size());
	for(TagMap::const_iterator itr=m_tagMap.begin(), itrEnd=m_tagMap.end(); itr!=itrEnd; ++itr)
		tags.push_back(itr->first);

	return int(tags.size());
}

bool CAssetTags::TagExists(const CString& tag) const
{
	assert(m_tagFolderPath.IsEmpty() == false);

	return m_tagMap.find(tag) != m_tagMap.end();
}

bool CAssetTags::IsTagShared(const CString& tag) const
{
	assert(m_tagFolderPath.IsEmpty() == false);

	CString fullPath = m_tagFolderPath;
	fullPath += tag;
	return IsVersionControlled(fullPath);
}

void CAssetTags::LoadTag(const CString& tag)
{
	assert(m_tagFolderPath.IsEmpty() == false);

	m_tagMap[tag] = StrSet();

	CString fullPath = m_tagFolderPath;
	fullPath += tag;

	FILE *f = fopen(fullPath, "rt");
	if(f == NULL)
		return;
	fseek(f, 0, SEEK_END);
	int fileSize = ftell(f);
	fseek(f, 0, SEEK_SET);
	CString contents = "";
	contents.GetBufferSetLength(fileSize);
	memset(contents.GetBuffer(), 0, fileSize);
	fread(contents.GetBuffer(), fileSize, 1, f);
	int count = 0;
	CString assetPath;
	while((assetPath = contents.Tokenize("\n", count)).IsEmpty() == false)
	{
		m_tagMap[tag].insert(assetPath);	
	}
	fclose(f);
}

void CAssetTags::SaveTag(const CString& tag)
{
	assert(m_tagFolderPath.IsEmpty() == false);
	assert(m_tagMap.find(tag) != m_tagMap.end());
	
	CString fullPath = m_tagFolderPath;
	fullPath += tag;

	FILE *f = fopen(fullPath, "wt");
	if(f == NULL)
		return;
	CString contents = "";
	const StrSet& assetSet = m_tagMap[tag];
	for(StrSet::const_iterator itr=assetSet.begin(), itrEnd=assetSet.end(); itr!=itrEnd; ++itr)
	{
		contents += *itr;
		contents += "\n";
	}
	fwrite(contents.GetBuffer(), contents.GetLength(), 1, f);
	fclose(f);

	CFileUtil::FileDesc fileDesc;
	bool ok = CFileUtil::FileExists(fullPath, &fileDesc);
	assert(ok);
	m_tagTimeStamp[tag] = fileDesc.time_write;
}

bool CAssetTags::IsVersionControlled(const CString& path) const
{
	if(gSettings.enableSourceControl && GetIEditor()->GetSourceControl())
	{
		uint32 fileAttr = GetIEditor()->GetSourceControl()->GetFileAttributes(path);
		bool isShared = fileAttr & SCC_FILE_ATTRIBUTE_MANAGED;
		return isShared;
	}
	
	return false;
}

bool CAssetTags::GetLatestCheckOutAndReload(const CString& path)
{
#if !defined(IS_CRYDEV)
	bool ok = true;
	ok = GetIEditor()->GetSourceControl()->GetLatestVersion(path);
	if(ok)
	{
		ok = GetIEditor()->GetSourceControl()->CheckOut(path);
	}
	if(ok == false)
	{
		AfxMessageBox("Source Control Operation failed.\nPlease make sure if Source Control Provider correctly setup and working directory is correct.");
		return false;
	}

	ReloadTagFilesIfNecessary();
#endif	
	return true;
}

void CAssetTags::CheckIn(const CString& path) const
{
#if !defined(IS_CRYDEV)
	CSourceControlDescDlg dlg;
	if(dlg.DoModal() == IDOK)
	{
		bool ok = GetIEditor()->GetSourceControl()->CheckIn(path, dlg.m_sDesc);
		if(ok == false)
			AfxMessageBox("Cannot submit the tag file.\nPlease make sure if Source Control Provider correctly setup and working directory is correct.");
	}
	else
		AfxMessageBox("You just cancelled the check-in of the shared tag.\nYou should do the manual check-in of the tag file.");
#endif
}