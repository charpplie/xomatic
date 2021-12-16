////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2012.
// -------------------------------------------------------------------------
//  File name:   IAssetSearcher.h
//  Version:     v1.00
//  Created:     11/12/2012 by Paul Reindell.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __IAssetSearcher_H__
#define __IAssetSearcher_H__

struct IAssetSearcher : public _i_reference_target_t
{
public:
	typedef uint64 TAssetSearchId;
	static const TAssetSearchId INVALID_ID = 0;
	static const TAssetSearchId FIRST_VALID_ID = 1;

	virtual ~IAssetSearcher() {}

	virtual const char* GetName() const = 0;

	virtual bool Accept(const char* asset, int& assetTypeId) const = 0;
	virtual bool Accept(const char varType, int& assetTypeId) const = 0;

	virtual const char* GetAssetTypeName(int assetTypeId) = 0;

	virtual bool Exists(const char* asset, int assetTypeId) const = 0;
	virtual bool GetReplacement(CString& replacement, int assetTypeId) = 0;

	virtual void StartSearcher() = 0;
	virtual void StopSearcher() = 0;

	virtual TAssetSearchId AddSearch(const char* asset, int assetTypeId) = 0;
	virtual void CancelSearch(TAssetSearchId id) = 0;
	virtual bool GetResult(TAssetSearchId id, std::vector<CString>& result, bool& doneSearching) = 0;
};

#endif //#ifndef __IAssetSearcher_H__