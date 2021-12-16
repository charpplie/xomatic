////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2012.
// -------------------------------------------------------------------------
//  File name:   ParticleSearcher.h
//  Version:     v1.00
//  Created:     11/12/2012 by Paul Reindell.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __ParticleSearcher_H__
#define __ParticleSearcher_H__

#include "IAssetSearcher.h"

////////////////////////////////////////////////////////////////////////////
class CParticleSearcher : public IAssetSearcher
{
public:
	CParticleSearcher();
	~CParticleSearcher();

	virtual const char* GetName() const { return "ParticleLib"; }

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

private:
	typedef std::vector<CString> TStringVec;
	typedef std::map< IAssetSearcher::TAssetSearchId, TStringVec > TParticleSearchRequests;

	void FindReplacement(const char* particleName, TStringVec& res);
	void SplitPath(const char* name, TStringVec& path);
	int FindMatching(const TStringVec& search, const TStringVec& sub);
	TAssetSearchId GetNextFreeId() const;

private:
 	TParticleSearchRequests m_requests;
};

////////////////////////////////////////////////////////////////////////////

#endif //#ifndef __ParticleSearcher_H__