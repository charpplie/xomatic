////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name: LayerNodeAnimator.cpp
//  Version:   v1.00
//  Created:   22-03-2010 by Dongjoon Kim
//  Description:
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 

#include "StdAfx.h"
#include "LayerNodeAnimator.h"
#include "TrackViewTrack.h"
#include "Objects/ObjectLayerManager.h"

//-----------------------------------------------------------------------------
void CLayerNodeAnimator::Animate(CTrackViewAnimNode *pNode, const SAnimContext& ac)
{
	if (GetIEditor()->IsInGameMode())
	{
		return;
	}

	CTrackViewTrack *pTrack = pNode->GetTrackForParameter(eAnimParamType_Visibility);
	if(pTrack)
	{
		bool visible = true;
		pTrack->GetValue( ac.time,visible );

		CObjectLayerManager* pLayerManager = 
			GetIEditor()->GetObjectManager()->GetLayersManager();

		if(pLayerManager)
		{
			CObjectLayer* pLayer = pLayerManager->FindLayerByName(pNode->GetName());
			if(pLayer)
				pLayer->SetVisible(visible);
		}
	}
}