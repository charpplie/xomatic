////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   objmancullqueue.h
//  Version:     v1.00
//  Created:     2/12/2009 by Michael Glueck
//  Compilers:   Visual Studio.NET
//  Description: Declaration and entry point for asynchronous obj-culling queue
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef CObjManCullQueue_H
#define CObjManCullQueue_H

#include <platform.h>
#include <IEntityRenderState.h>
#include <Cry_Camera.h>

//forces usage of z-buffer based occlusion, enables queue processing of occlusion queries, results have 1 frame delay









#include <Cry_Geo.h>

class CZBufferCuller;
class CCullBuffer;

namespace NCullQueue
{
	class CCullTask;
#ifdef USE_CULL_QUEUE



		enum {MAX_CULL_QUEUE_ITEM_COUNT = 2048};//64 KB

#else
	enum {MAX_CULL_QUEUE_ITEM_COUNT = 1};
#endif

	struct SCullItem
	{
		AABB objBox;
		uint32 BufferID;
		OcclusionTestClient *pOcclTestVars;
	} _ALIGN(16);

	class SCullQueue
	{
	private:
		SCullItem cullItemBuf[MAX_CULL_QUEUE_ITEM_COUNT+1] _ALIGN(128);
		uint32 curIndex;
		Vec3 sunDir;




	public:
		void ProcessInternal(uint32 mainFrameID, CZBufferCuller *const pCullBuffer,const CCamera* const pCam);

		SCullQueue();
		~SCullQueue();

		uint32 Size(){return curIndex;}

		ILINE void AddItem(const AABB& objBox, const Vec3& lightDir, OcclusionTestClient *pOcclTestVars, uint32 mainFrameID)
		{
			if(curIndex < MAX_CULL_QUEUE_ITEM_COUNT-1)
			{
				SCullItem& RESTRICT_REFERENCE rItem = cullItemBuf[curIndex];
				rItem.objBox				= objBox;
				sunDir							=	lightDir;
				rItem.BufferID			= 14;//1<<1 1<<2 1<<3  shadows
				rItem.pOcclTestVars = pOcclTestVars;
				++curIndex;//store here to make it morte likely SCullItem has been writen
				return;
			}
			pOcclTestVars->nLastVisibleMainFrameID = mainFrameID;//not enough space to hold item
		}

		ILINE void AddItem(const AABB& objBox, float fDistance, OcclusionTestClient *pOcclTestVars, uint32 mainFrameID)
		{
			if(curIndex < MAX_CULL_QUEUE_ITEM_COUNT-1)
			{
				SCullItem& RESTRICT_REFERENCE rItem = cullItemBuf[curIndex];
				rItem.objBox				= objBox;
				rItem.BufferID			= 1;	//1<<0 zbuffer
				rItem.pOcclTestVars = pOcclTestVars;
				++curIndex;//store here to make it morte likely SCullItem has been writen
				return;
			}
			pOcclTestVars->nLastVisibleMainFrameID = mainFrameID;//not enough space to hold item
		}
		void Process(uint32 mainFrameID, CCullBuffer *const pCullBuffer,const CCamera* const pCam);
		void Wait();
	};










































};


#endif // CObjManCullQueue_H
