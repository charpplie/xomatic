////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  Copyright (C), Crytek GmbH, 2011.
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "LiveCreate/EditorLiveCreateHostInfo.h"
#include "LiveCreate/EditorLiveCreateManager.h"
#include "LiveCreate/EditorLiveCreateAreaSync.h"
#include "LiveCreate/EditorLiveCreate.h"
#include "Objects/ObjectManager.h"
#include "Objects/ObjectLayer.h"
#include "Objects/RoadObject.h"
#include "Objects/EntityObject.h"
#include "Objects/ObjectLayerManager.h"

#ifndef NO_LIVECREATE

#define NO_LIVECREATE_COMMAND_IMPLEMENTATION
#include "../../CryEngine/CryLiveCreate/LiveCreateCommands.h"
#include "../../CryEngine/CryLiveCreate/LiveCreate_Objects.h"
#undef NO_LIVECREATE_COMMAND_IMPLEMENTATION

namespace LiveCreate
{

CObjectSync::CObjectSync(CEditorManager* pManager)
	: m_pManager(pManager)
{
}

CObjectSync::~CObjectSync()
{
}

void CObjectSync::FlushChanges()
{
	// don't bother updating if it's not possible
	if (!m_pManager->CanSend())
	{
		return;
	}

	// process each object type separately
	bool bLayerIdsAssigned = false;
	const float kSyncTime = m_pManager->GetAdvancedSettings().fObjectsSyncTime;
	for (uint32 i=0; i<eLiveCreateObjectType_MAX; ++i)
	{
		SyncType& syncType = m_types[i];

		// no dirty object
		if (!syncType.m_bHasDirtyObjectsArea)
		{
			continue;
		}

		// do not sync to often
		if (gEnv->pTimer->GetAsyncTime().GetDifferenceInSeconds(syncType.m_lastObjectSyncSentTime) <= kSyncTime)
		{
			continue;
		}

		// Make sure the LayerID in the render nodes are up to date
		if (!bLayerIdsAssigned)
		{
			bLayerIdsAssigned = true;
			GetIEditor()->GetObjectManager()->AssignLayerIDsToRenderNodes();
		}

		// create object mask
		uint32 objectMask = 0;
		const bool bTerrain = (i == eLiveCreateObjectType_Terrain);
		switch (i)
		{
			case eLiveCreateObjectType_Brushes: objectMask |= 1<<eERType_Brush; break;
			case eLiveCreateObjectType_Decals: objectMask |= 1<<eERType_Decal; break;
			case eLiveCreateObjectType_Roads: objectMask |= 1<<eERType_Road; break;
			case eLiveCreateObjectType_Vegetation: objectMask |= 1<<eERType_Vegetation; break;
		}

		// prepare sync data
		IDataWriteStream* pWriter = gEnv->pServiceNetwork->CreateMessageWriter();
		gEnv->p3DEngine->SaveInternalState(*pWriter, syncType.m_dirtyObjectArea, bTerrain, objectMask);

		// create a command with 3D engine data
		if (pWriter->GetSize() > 0 )
		{
			CLiveCreateCmd_ObjectAreaUpdate command;
			command.m_data.resize(pWriter->GetSize());
			pWriter->CopyToBuffer(&command.m_data[0]);	

			// Get layer mapping tables
			{
				std::vector<CObjectLayer*> layers;
				GetIEditor()->GetObjectManager()->GetLayersManager()->GetLayers(layers);

				for (uint32 i=0; i<layers.size(); ++i)
				{
					CObjectLayer* pLayer = layers[i];
					if (NULL != pLayer)
					{
						command.m_layerNames.push_back((const char*)pLayer->GetName());
						command.m_layerIds.push_back(pLayer->GetLayerID());
					}
				}
			}

			// send the command
			m_pManager->SendCommand(command);
		}

		// cleanup
		pWriter->Delete();

		// reset
		syncType.m_lastObjectSyncSentTime = gEnv->pTimer->GetAsyncTime();
		syncType.m_bHasDirtyObjectsArea = false;
		syncType.m_dirtyObjectArea = AABB();
	}
}

void CObjectSync::AddDirtyObjectArea(const AABB& area, ELiveCreateObjectType objectType)
{
	if (area.IsNonZero())
	{
		SyncType& sync = m_types[objectType];

		if (sync.m_bHasDirtyObjectsArea)
		{
			sync.m_dirtyObjectArea.Add(area);
		}
		else
		{
			sync.m_dirtyObjectArea = area;
			sync.m_bHasDirtyObjectsArea = true;
		}
	}
}

} // namespace LiveCreate

#endif
