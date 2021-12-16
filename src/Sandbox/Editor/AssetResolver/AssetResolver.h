////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2012.
// -------------------------------------------------------------------------
//  File name:   AssetResolver.h
//  Version:     v1.00
//  Created:     11/12/2012 by Paul Reindell.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __AssetResolver_H__
#define __AssetResolver_H__

#include <functor.h>
#include "Util/Variable.h"

class CMissingAssetReport;
struct IAssetSearcher;

class CMissingAssetResolver : public IEditorNotifyListener
{
public:
	CMissingAssetResolver();
	~CMissingAssetResolver();

	void Shutdown();
	void StartResolver();
	void StopResolver();
	void PumpEvents();

	uint32 AddResolveRequest(const char* assetStr, const TMissingAssetResolveCallback& request, char varType = IVariable::DT_SIMPLE, bool onlyIfNotExist = true);
	void CancelRequest(const TMissingAssetResolveCallback& request, uint32 reportId = 0);

	// IEditorNotifyListener
	void OnEditorNotifyEvent(EEditorNotifyEvent ev) override;
	// ~IEditorNotifyListener

private: // those functions should be only accessible to the dialog!
	friend class CMissingAssetDialog; 
	friend class CMissingAssetMessage;
	void AcceptRequest(uint32 reportId, const char* filename);
	void CancelRequest(uint32 reportId);

	CMissingAssetReport* GetReport() { return m_pReport; }

	IAssetSearcher* GetAssetSearcherById(int id) const;

private:
	IAssetSearcher* GetSearcherAndAssetId(int& searcherId, int& assetId, const char* filename, char varType);

private:
	_smart_ptr<CMissingAssetReport> m_pReport;
	CryCriticalSection m_mutex;
	typedef std::map<int, _smart_ptr<IAssetSearcher>> TSearcher;
	TSearcher m_searcher;
	int cv_popupMissingAssetResolver;
	int ed_MissingAssetResolver;
};

#endif //#ifndef __AssetResolver_H__