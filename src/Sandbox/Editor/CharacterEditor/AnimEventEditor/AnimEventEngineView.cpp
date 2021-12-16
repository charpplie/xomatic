//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include "stdafx.h"
#include <ICryAnimation.h>
#include "AnimEventEngineView.h"
#include "IAnimEventDoc.h"
#include "IAnimEventVisitable.h"
#include "IAnimEventDocVisitor.h"
#include "IAnimEventDocChangeDescription.h"
#include <map>

class AnimEventEngineDocVisitor : public IAnimEventDocVisitor
{
public:
	AnimEventEngineDocVisitor();
	virtual ~AnimEventEngineDocVisitor();

	virtual void VisitEvent(int nID, const char* szAnimPath, const char* szName, float fTime, const char* szParameter, const char* szBone, const Vec3& vOffset, const Vec3& vDir, const char* szModel, bool bSelected);

private:
	typedef std::map<string, int> AnimationAssetMap;
	AnimationAssetMap animationAssetMap;
	IAnimEvents* pAnimEventsManager;
};

AnimEventEngineView::AnimEventEngineView(CModelViewportCE* pModelViewportCE)
:	pDoc(0),
	pModelViewportCE(pModelViewportCE)
{
}

AnimEventEngineView::~AnimEventEngineView()
{
}

void AnimEventEngineView::ReferenceDoc(IAnimEventDoc* pDoc)
{
	this->pDoc = pDoc;
}

void AnimEventEngineView::Refresh(IAnimEventDocChangeDescription* pDescription)
{
	// Check whether it is necessary to update the events.
	bool bEventsMayHaveChanged = true;
	if (pDescription)
		bEventsMayHaveChanged = pDescription->HasEventListChanged() || pDescription->HaveEventsChanged() || pDescription->HasTimeChanged();

	// Create a visitor and visit all the animation events in the document.
	if (bEventsMayHaveChanged)
	{
		AnimEventEngineDocVisitor visitor;
		this->pDoc->GetVisitable()->Visit(&visitor);
	}
}

AnimEventEngineDocVisitor::AnimEventEngineDocVisitor()
:	pAnimEventsManager(GetISystem()->GetIAnimationSystem()->GetIAnimEvents())
{
}

AnimEventEngineDocVisitor::~AnimEventEngineDocVisitor()
{
}

void AnimEventEngineDocVisitor::VisitEvent(int nID, const char* szAnimPath, const char* szName, float fTime, const char* szParameter, const char* szBone, const Vec3& vOffset, const Vec3& vDir, const char* szModel, bool bSelected)
{
	int32 nAnim_CAFID=-1;
	int32 nAnim_LMGID=-1;
	int32 nAnim_PMGID=-1;
	// Look for this animation asset in our asset map. If it is not there, then this is the first event for this
	// animation that we have encountered in the document.
	AnimationAssetMap::iterator itAssetEntry = this->animationAssetMap.find(szAnimPath);
	if (itAssetEntry == this->animationAssetMap.end())
	{
		// This is the first time we have encountered this animation. We should clear all global events registered for this
		// animation, so that we can re-add them with the updated information. First we need to find the global ID.
		nAnim_CAFID = this->pAnimEventsManager->GetGlobalAnimID_CAF(szAnimPath);
		if (nAnim_CAFID>=0)
		{
			// Delete all the events for this animation.
			this->pAnimEventsManager->DeleteAllEventsForCAF(nAnim_CAFID);
			// Add the item to the map.
			itAssetEntry = this->animationAssetMap.insert(std::make_pair(szAnimPath, nAnim_CAFID)).first;
		}
		nAnim_LMGID = this->pAnimEventsManager->GetGlobalAnimID_LMG(szAnimPath);
		if (nAnim_LMGID>=0)
		{
			// Delete all the events for this animation.
			this->pAnimEventsManager->DeleteAllEventsForLMG(nAnim_LMGID);
			// Add the item to the map.
			itAssetEntry = this->animationAssetMap.insert(std::make_pair(szAnimPath, nAnim_LMGID)).first;
		}
		nAnim_PMGID = this->pAnimEventsManager->GetGlobalAnimID_PMG(szAnimPath);
		if (nAnim_PMGID>=0)
		{
			// Delete all the events for this animation.
			this->pAnimEventsManager->DeleteAllEventsForPMG(nAnim_PMGID);
			// Add the item to the map.
			itAssetEntry = this->animationAssetMap.insert(std::make_pair(szAnimPath, nAnim_PMGID)).first;
		}
	}


	nAnim_CAFID = this->pAnimEventsManager->GetGlobalAnimID_CAF(szAnimPath);
	if (nAnim_CAFID>=0)
	{
		// Add this event to the global event list.
		int animationID = (*itAssetEntry).second;
		this->pAnimEventsManager->AddAnimEvent_CAF(animationID, szName, szParameter, szBone, fTime, vOffset, vDir,szModel);
	}
	nAnim_LMGID = this->pAnimEventsManager->GetGlobalAnimID_LMG(szAnimPath);
	if (nAnim_LMGID>=0)
	{
		// Add this event to the global event list.
		int animationID = (*itAssetEntry).second;
		this->pAnimEventsManager->AddAnimEvent_LMG(animationID, szName, szParameter, szBone, fTime, vOffset, vDir,szModel);
	}
	nAnim_PMGID = this->pAnimEventsManager->GetGlobalAnimID_PMG(szAnimPath);
	if (nAnim_PMGID>=0)
	{
		// Add this event to the global event list.
		int animationID = (*itAssetEntry).second;
		this->pAnimEventsManager->AddAnimEvent_PMG(animationID, szName, szParameter, szBone, fTime, vOffset, vDir,szModel);
	}
}
