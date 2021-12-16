////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2012.
// -------------------------------------------------------------------------
//  File name:   ParticleSearcher.cpp
//  Version:     v1.00
//  Created:     11/12/2012 by Paul Reindell.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "ParticleSearcher.h"
#include "Particles/ParticleManager.h"
#include "GenericSelectItemDialog.h"
#include <IParticles.h>

////////////////////////////////////////////////////////////////////////////
CParticleSearcher::CParticleSearcher()
{
}

////////////////////////////////////////////////////////////////////////////
CParticleSearcher::~CParticleSearcher()
{
}

////////////////////////////////////////////////////////////////////////////
bool CParticleSearcher::Accept(const char* asset, int& assetTypeId) const
{
	return false;
}

////////////////////////////////////////////////////////////////////////////
bool CParticleSearcher::Accept(const char varType, int& assetTypeId) const
{
	if (varType == IVariable::DT_PARTICLE_EFFECT)
	{
		assetTypeId = 0;
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////
const char* CParticleSearcher::GetAssetTypeName(int assetTypeId)
{
	assert(assetTypeId == 0);
	return "Particle";
}

////////////////////////////////////////////////////////////////////////////
bool CParticleSearcher::Exists(const char* asset, int assetTypeId) const
{
	assert(assetTypeId == 0);
	return gEnv->pParticleManager->FindEffect(asset, "", false) != NULL;
}

////////////////////////////////////////////////////////////////////////////
bool CParticleSearcher::GetReplacement(CString& replacement, int assetTypeId)
{
	assert(assetTypeId == 0);
	IParticleEffectIteratorPtr iter = gEnv->pParticleManager->GetEffectIterator();
	if (iter->GetCount() == 0)
		return false;

	std::vector<CString> items;
	while (IParticleEffect* pEffect = iter->Next())
	{
		string name = pEffect->GetFullName();
		items.push_back(name.c_str());
	}

	CGenericSelectItemDialog gtDlg;
	gtDlg.SetTitle("Select Particle Effect");
	gtDlg.SetMode(CGenericSelectItemDialog::eMODE_TREE);
	gtDlg.SetTreeSeparator(".");
	gtDlg.SetItems(items);
	INT_PTR res = gtDlg.DoModal();
	CString resStr = gtDlg.GetSelectedItem();
	if (res==IDOK && !resStr.IsEmpty())
	{
		replacement = resStr;
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////
void CParticleSearcher::StartSearcher()
{
	gEnv->pParticleManager->LoadLibrary( "*", NULL, false );
}

////////////////////////////////////////////////////////////////////////////
void CParticleSearcher::StopSearcher()
{
}

////////////////////////////////////////////////////////////////////////////
IAssetSearcher::TAssetSearchId CParticleSearcher::AddSearch(const char* asset, int assetTypeId)
{
	assert(assetTypeId == 0);

	TAssetSearchId id = GetNextFreeId();
	FindReplacement(asset, m_requests[id]);
	return id;
}

////////////////////////////////////////////////////////////////////////////
void CParticleSearcher::CancelSearch(TAssetSearchId id)
{
	TParticleSearchRequests::iterator it = m_requests.find(id);
	assert(it != m_requests.end());
	m_requests.erase(it);
}

////////////////////////////////////////////////////////////////////////////
bool CParticleSearcher::GetResult(TAssetSearchId id, std::vector<CString>& result, bool& doneSearching)
{
	TParticleSearchRequests::iterator it = m_requests.find(id);
	assert(it != m_requests.end());
	bool res = !it->second.empty();
	for (TStringVec::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
		result.push_back(*it2);
	doneSearching = true;
	return res;
}

////////////////////////////////////////////////////////////////////////////
void CParticleSearcher::FindReplacement(const char* particleName, TStringVec& res)
{
	std::vector<CString> particlePath;
	SplitPath(particleName, particlePath);
	if (particlePath.empty())
		return;

	IParticleEffectIteratorPtr iter = gEnv->pParticleManager->GetEffectIterator();
	std::map<int, TStringVec> sortedRes;
	while (IParticleEffect* pEffect = iter->Next())
	{
		std::vector<CString> path;
		SplitPath(pEffect->GetFullName().c_str(), path);
		int r = FindMatching(particlePath, path);
		if (r > 0)
			sortedRes[r].push_back(pEffect->GetFullName().c_str());
	}

	for (std::map<int, TStringVec>::const_reverse_iterator it = sortedRes.rbegin(); it != sortedRes.rend(); ++it)
		for (TStringVec::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
		res.push_back(*it2);
}

////////////////////////////////////////////////////////////////////////////
int CParticleSearcher::FindMatching(const TStringVec& search, const TStringVec& sub)
{
	int r = 0;
	const int count = min(search.size(), sub.size());
	for (; r < count && search[r] == sub[r]; ++r);
	return r;
}

////////////////////////////////////////////////////////////////////////////
void CParticleSearcher::SplitPath(const char* name, std::vector<CString>& path)
{
	CString n = name;
	n.MakeLower();
	int pos = n.ReverseFind('.');
	while (pos >= 0)
	{
		CString part = n.Mid(pos+1);
		n = n.Left(pos);
		if (!part.IsEmpty()) path.push_back(part);
		pos = n.ReverseFind('.');
	}
	if (!n.IsEmpty()) path.push_back(n);
}

////////////////////////////////////////////////////////////////////////////
IAssetSearcher::TAssetSearchId CParticleSearcher::GetNextFreeId() const
{
	TAssetSearchId id = IAssetSearcher::FIRST_VALID_ID;
	for (TParticleSearchRequests::const_iterator it = m_requests.begin(); it != m_requests.end() && it->first == id; ++it, ++id);
	return id;
}
////////////////////////////////////////////////////////////////////////////
