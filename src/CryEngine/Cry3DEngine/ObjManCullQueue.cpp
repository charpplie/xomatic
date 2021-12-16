////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   objmancullqueue.cpp
//  Version:     v1.00
//  Created:     2/12/2009 by Michael Glueck
//  Compilers:   Visual Studio.NET
//  Description: Implementation for asynchronous obj-culling queue
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ObjManCullQueue.h"
#include "CZBufferCuller.h"






























NCullQueue::SCullQueue::SCullQueue() : curIndex(0) 
{ 
#ifdef USE_CULL_QUEUE
#ifndef __SPU__
	memset(cullItemBuf,0,sizeof(cullItemBuf)); 
#endif









#endif
}

void NCullQueue::SCullQueue::Process(uint32 mainFrameID, CCullBuffer *const pCullBuffer,const CCamera* pCam)
{
#ifdef USE_CULL_QUEUE















#endif
}

NCullQueue::SCullQueue::~SCullQueue()
{
#ifdef USE_CULL_QUEUE







#endif
}

void NCullQueue::SCullQueue::Wait()
{
#ifdef USE_CULL_QUEUE










#endif
}

#if !defined(CRCYG_CM)
SPU_ENTRY(IsBoxOccluded)
#endif
void NCullQueue::SCullQueue::ProcessInternal(uint32 mainFrameID, CZBufferCuller *const pCullBuffer, const CCamera* const pCam)
{
#ifdef USE_CULL_QUEUE












	CZBufferCuller& cullBuffer = *(CZBufferCuller*)pCullBuffer;
	#define localQueue (*this)
	#define localCam (*pCam)

	cullBuffer.BeginFrame(localCam);
	bool feedbackExec = false;//becomes true after one possible feedback loop



	{
		SPU_DOMAIN_LOCAL const SCullItem*const cEnd = &localQueue.cullItemBuf[localQueue.curIndex];
		for(uint16 a=1,b=0;b<4;a<<=1,b++)//traverse through all 4 occlusion buffers
		{
			cullBuffer.ReloadBuffer(b);
			for(SCullItem* it = localQueue.cullItemBuf; it != cEnd; ++it)
			{
				SCullItem& rItem = *it;
				IF(!(rItem.BufferID&a),1)
					continue;

				IF((rItem.BufferID&1),0)	//zbuffer
				{
					if(!cullBuffer.IsObjectVisible(rItem.objBox, eoot_OBJECT, 0.f, &rItem.pOcclTestVars->nLastOccludedMainFrameID))
						rItem.pOcclTestVars->nLastOccludedMainFrameID = mainFrameID;
					else
						rItem.pOcclTestVars->nLastVisibleMainFrameID = mainFrameID;
				}
				else	//shadow buffer
				if(rItem.pOcclTestVars->nLastNoShadowCastMainFrameID != mainFrameID)
				{
					if(cullBuffer.IsObjectVisible(rItem.objBox, eoot_OBJECT, 0.f, &rItem.pOcclTestVars->nLastOccludedMainFrameID))
						rItem.pOcclTestVars->nLastShadowCastMainFrameID = mainFrameID;
				}
			}
		}

		//if not visible, set occluded
		for(SCullItem* it = localQueue.cullItemBuf; it != cEnd; ++it)
		{
			SCullItem& rItem = *it;
			IF((rItem.BufferID&6) & (rItem.pOcclTestVars->nLastNoShadowCastMainFrameID != mainFrameID),1)
					rItem.pOcclTestVars->nLastShadowCastMainFrameID = mainFrameID;
		}

	}
















	curIndex = 0;//goes through cache
#endif
	#undef localQueue
	#undef localCam
}
