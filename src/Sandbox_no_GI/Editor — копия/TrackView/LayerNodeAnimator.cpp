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

//-----------------------------------------------------------------------------
CLayerNodeAnimator::CLayerNodeAnimator()
{

}

//-----------------------------------------------------------------------------
void CLayerNodeAnimator::Animate( IAnimNode *pNode , const SAnimContext& ac)
{
	IAnimTrack *pTrack = pNode->GetTrackForParameter(APARAM_VISIBLE);
	if(pTrack)
	{
		bool visible = true;
		pTrack->GetValue( ac.time,visible );

		CObjectLayerManager* pLayerManager = 
			GetIEditor()->GetObjectManager()->GetLayersManager();

		if(pLayerManager)
		{
			CObjectLayer* pLayer = pLayerManager->FindLayerByName_Workaround(pNode->GetName());
			if(pLayer)
				pLayer->SetVisible(visible);
		}
	}
}