//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2010.
// -------------------------------------------------------------------------
//  File Name        : AssetTags.h
//  Author           : Jaewon Jung
//  Time of creation : 7/16/2010   15:14
//  Compilers        : VS2008
//  Description      : This class manages tags assigned to each asset.
//                     This also deals with sharing tag files via Perforce.
//  Notice           : Also refer to 'AssetMetaDataFileDB.h'.
//                     For use cases, refer to 'AssetBrowserSearchDlg.cpp'.
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////

#ifndef __ASSETTAGS_H__
#define __ASSETTAGS_H__
#pragma once

#include "STLPoolAllocator.h"

class CAssetTags
{
public:
	typedef std::vector<CString> StrVector;
	void Initialize();
	void CreateTag(const CString& tag, bool shared);
	void AddAssetsToTag(const CString& tag, const StrVector& assets);
	void RemoveAssetsFromTag(const CString& tag, const StrVector& assets);
	void DestroyTag(const CString& tag);

	bool TagExists(const CString& tag) const;
	int GetAllTags(StrVector& tags) const;
	int GetTagsForAsset(StrVector& tags, const CString& asset) const;
	int GetAssetsForTag(StrVector& assets, const CString& tag) const;
	int GetAssetCountForTag(const CString& tag) const;
	bool IsTagShared(const CString& tag) const;

private:
	typedef std::set<CString, std::less<CString>, stl::STLPoolAllocator<CString>> StrSet; 
	typedef std::map<CString, StrSet, std::less<CString>, stl::STLPoolAllocator<std::pair<CString, StrSet>>> TagMap;
	TagMap m_tagMap;
	
	typedef std::map<CString, time_t> TagTimeStamp;
	TagTimeStamp m_tagTimeStamp;
	
	CString m_tagFolderPath;

	void ReloadTagFilesIfNecessary();
	void LoadTag(const CString& tag);
	void SaveTag(const CString& tag);
	bool IsVersionControlled(const CString& path) const;
	bool GetLatestCheckOutAndReload(const CString& path);
	void CheckIn(const CString& path) const;
};

#endif//__ASSETTAGS_H__