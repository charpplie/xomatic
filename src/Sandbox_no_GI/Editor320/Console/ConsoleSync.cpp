////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   ConsoleSync.cpp
//  Created:     21/1/2009 by Timur.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include <StdAfx.h>
#include "ConsoleSync.h"
#include "ConsoleConnection.h"

#include "VegetationMap.h"

#include "INotificationNetwork.h"
#include "LivePreview\RealtimeRemoteUpdate.h"

#include "CryEditDoc.h"
#include ".\Terrain\SurfaceType.h"
#include "Mission.h"

#include "Objects/Entity.h"
#include "Objects/BrushObject.h"
#include "Brush/SolidBrushObject.h"

#include "IGame.h"

#include "Entityprototypemanager.h"

#include "Objects/Group.h"

#include "Particles/ParticleManager.h"

#include "Terrain/TerrainManager.h"

#include "Particles/ParticleItem.h"
#include "CryAction.h"

// As there is no console variable for the default port and there is no 
// mean to query if from the interface
// we can only code it here.
// In the future this code WILL be wrong and it will be needed to change 
// this define to a proper console variable.
#define DEFAULT_PORT	9432

//////////////////////////////////////////////////////////////////////////
CConsoleSynchronization::CLayerInfo::CLayerInfo()
{
	m_nPosX=0;
	m_nPosY=0;
	m_nw=0;
	m_nh=0;
	m_nFormat=0;
	m_nSize=0;
	m_data=NULL;
}
//////////////////////////////////////////////////////////////////////////
CConsoleSynchronization::CLayerInfo::~CLayerInfo()
{
	SAFE_DELETE(m_data);
	m_nSize=0;
}
//////////////////////////////////////////////////////////////////////////
CConsoleSynchronization::CLayerInfo::CLayerInfo(int nPosX,int nPosY, int nw,int nh,int nFormat,unsigned char* szData, int nDataSize)
{
	m_nPosX=nPosX;
	m_nPosY=nPosY;
	m_nw=nw;
	m_nh=nh;
	m_nFormat=nFormat;
	m_nSize=0;
	m_data=NULL;

	SetData(szData,nDataSize);
}
//////////////////////////////////////////////////////////////////////////
CConsoleSynchronization::CLayerInfo::CLayerInfo(const CLayerInfo& crOperand)
{
	*this=crOperand;
}
//////////////////////////////////////////////////////////////////////////
CConsoleSynchronization::CLayerInfo&	CConsoleSynchronization::CLayerInfo::operator=(const CLayerInfo& crOperand)
{
	if (this!=&crOperand)
	{
		m_nPosX=crOperand.m_nPosX;
		m_nPosY=crOperand.m_nPosY;
		m_nw=crOperand.m_nw;
		m_nh=crOperand.m_nh;
		m_nFormat=crOperand.m_nFormat;

		SetData(crOperand.m_data,crOperand.m_nSize);
	}
	return *this;
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::CLayerInfo::SetData(uint8* uchData,int nSize)
{
	if (!(nSize&&uchData))
	{
		return;
	}

	if (m_nSize!=0)
	{
		assert(m_nSize==nSize);
	}

	if (m_nSize!=nSize)
	{
		SAFE_DELETE(m_data);
		m_data=new uint8[nSize];
		m_nSize=nSize;
	}

	memcpy(m_data,uchData,nSize);
}
//////////////////////////////////////////////////////////////////////////
















//////////////////////////////////////////////////////////////////////////
CConsoleSynchronization::CConsoleSynchronization()
{
	m_boMustEndThread=false;
	m_boConsoleOptionsChanged=false;
	m_boMustStartEnablingRealtimeSync=false;
	m_boMustStartDisablingRealtimeSync=false;
	Start();

	GetIEditor()->RegisterNotifyListener( this );
	
	m_bScheduleMovieSystemSync = false;
	m_bScheduleTimeOfDaySync = false;
	m_bBrushesModified = false;
	m_bHeightmapModified = false;
	m_bVegetationModified = false;
	m_bRoadModified = false;
	m_bRiverModified = false;
	m_bDecalModified = false;
	m_bMustInterruptUpdates = false;

	m_bHasSyncedEntityIDs=false;

	m_bCameraModified=false;
	m_bIsSyncingCamera=false;
	m_oCameraPosition=Vec3(0,0,0);
	m_oCameraDirection=Vec3(0,1,0);

	m_modifiedBrushesBounds.Reset();
	m_modifiedVegetationBounds.Reset();
	m_modifiedRoadBounds.Reset();
	m_modifiedRiverBounds.Reset();
	m_modifiedDecalBounds.Reset();
	m_modifiedHeightmapBounds.Reset();

	m_lastDataFlushTime.SetValue(0);
	m_lastDelayedInfoTime.SetValue(0);
	m_lastKeepAliveMessageTime.SetValue(0);

	m_nPassThrough=0;
	m_nTerrainBlockClusteringSize=8;

	m_multiThreaded = 0;
	m_godMode = 0;

	m_bRealtimeSyncOn=gSettings.oHotUpdateSystemSettings.boRealtimeSyncEnabled;

	REGISTER_CVAR2("ed_remoteupdatepassthrough", &m_nPassThrough,0 , VF_NULL,"Enables (1)/ Disables (0) the pass-through mechanism for the remote update system");
	REGISTER_CVAR2("ed_terrainblockclusteringsize", &m_nTerrainBlockClusteringSize,8 , VF_NULL,"Controls the HxV number of blocks to be clustered during a full sync. Default: 4. 0 makes a Full Sync at once.");
}

//////////////////////////////////////////////////////////////////////////
CConsoleSynchronization::~CConsoleSynchronization()
{
	if (GetIEditor())
		GetIEditor()->UnregisterNotifyListener( this );

	m_boMustEndThread=true;
	ResumeThread();
	WaitForThread();
	Stop(); // This is here only to avoid log warnings, as this is not necessary.
}

//////////////////////////////////////////////////////////////////////////
CConsoleSynchronization& CConsoleSynchronization::GetCConsoleSynchronization()
{
	static CConsoleSynchronization oConsoleSynchronization;
	return oConsoleSynchronization;
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::SendMessage( SSyncMessage& msg )
{
	SSyncMessage*	poSyncMessage=new SSyncMessage(msg);
	SendMessage(poSyncMessage);
}

//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::FullSync()
{
	if (m_bMustInterruptUpdates)
	{
		return;
	}

	SyncTerrain();

	m_boStartedFullSync=true;
}

//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::SyncTerrain()
{
	int nSectorSize(gEnv->p3DEngine->GetTerrainSectorSize());
	int nTerrainSize(gEnv->p3DEngine->GetTerrainSize());

	int nNumberOfSectors(0);
	int nHorizontalSector(0);
	int nVerticalSector(0); //ok maybe sectors are not truly vertical, but I 
	// guess this is better than saying y sectors.

	// Heuristic: we are currently using a console variable, with default value 4
	// to control the clustering size of the terrain.
	nSectorSize*=abs(m_nTerrainBlockClusteringSize);

	if (nSectorSize>0)
	{
		nNumberOfSectors=nTerrainSize/nSectorSize;
	}

	if (nNumberOfSectors==0)
	{
		Sync3DEngineData( SYNC_TERRAIN|SYNC_OBJECTS,AABB() );
	}
	else
	{
		// Here we cancel the remaining updates for the terrain, forcing them to
		// start all over.
		m_cTerrainBoundingBoxes.resize(0);
		m_cTerrainBoundingBoxes.reserve(nNumberOfSectors*nNumberOfSectors);

		AABB stBoundingBox;
		for (nVerticalSector=0;nVerticalSector<nNumberOfSectors;++nVerticalSector)
		{
			for (nHorizontalSector=0;nHorizontalSector<nNumberOfSectors;++nHorizontalSector)
			{
				stBoundingBox.min=Vec3(nHorizontalSector*nSectorSize,nVerticalSector*nSectorSize,-32000);
				stBoundingBox.max=Vec3((nHorizontalSector+1)*nSectorSize,(nVerticalSector+1)*nSectorSize,+32000);
				m_cTerrainBoundingBoxes.push_back(stBoundingBox);
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::SyncCamera(int nOperarion)
{
	SSyncMessage*				poSyncMessage(NULL);
	Vec3								oPosition(m_oCameraPosition);
	Vec3								oViewDirection(m_oCameraDirection);

	// If the camera has not been modified, we are getting the info about it
	// from an alternate source.
	if (!m_bCameraModified)
	{
		IEditor*			piEditor=GetIEditor();
		IRenderer*		piRenderer(NULL);

		if (!piEditor)
		{
			return;
		}

		piRenderer=piEditor->GetRenderer();
		if (!piRenderer)
		{
			return;
		}

		const CCamera&			roCamera=piRenderer->GetCamera();

		oPosition=roCamera.GetPosition();
		oViewDirection=roCamera.GetViewdir();
	}

	poSyncMessage=new SSyncMessage();
	poSyncMessage->node=CreateXmlNode("SyncMessage");
	poSyncMessage->node->setAttr("Type","Camera");

	poSyncMessage->node->setAttr("Position",oPosition);

	poSyncMessage->node->setAttr("Direction",oViewDirection);

	switch (nOperarion)
	{
		case eStartSync:
			{
				m_bIsSyncingCamera=true;
				poSyncMessage->node->setAttr("StartSync",1);
			}
		break;

		case eEndSync:
			{
				poSyncMessage->node->setAttr("StartSync",0);
				m_bIsSyncingCamera=false;
			}
		break;

		case eNoOperarion:
			{
				// Nothing to do here.
			}
		break;

		default:
			{
				assert(false);
			}
		break;
	}
	
	SendMessage(poSyncMessage);
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::NotifyOptionsChanged()
{
	// If the real time sync was toggled from off to on...
	if (
			(!m_bRealtimeSyncOn)
			&&
			(gSettings.oHotUpdateSystemSettings.boRealtimeSyncEnabled)
		)
	{
		m_boMustStartEnablingRealtimeSync=true;
		if (m_boMustStartDisablingRealtimeSync)
		{
			m_boMustStartDisablingRealtimeSync=false;
		}
	}
	else if 
		(
			(!gSettings.oHotUpdateSystemSettings.boRealtimeSyncEnabled)
			&&
			(m_bRealtimeSyncOn)
		)
	{
		m_boMustStartDisablingRealtimeSync=true;
		if (m_boMustStartEnablingRealtimeSync)
		{
			m_boMustStartEnablingRealtimeSync=false;
		}
	}


	m_bRealtimeSyncOn=gSettings.oHotUpdateSystemSettings.boRealtimeSyncEnabled;

	m_bHasSyncedEntityIDs=false;

	m_boConsoleOptionsChanged = true;
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::Run()
{
	CryThreadSetName(-1, "Console Synchronization");

	TDBuffer				cuchSendBuffer;	

	while (!m_boMustEndThread)
	{
		while (!m_boMustEndThread && (m_cUpdateSchedule.empty())) 
		{
			m_oUpdateListConditionVariable.Wait(m_oUpdateListConditionMutex);
		}

		if (m_boMustEndThread)
		{
			break;
		}

		int																nCount(0);
		SSyncMessage*											pstCurrentMessage(NULL);
		INotificationNetworkClient				*pClient[CHotUpdateSystem::eNumberOfPlatforms];
		CConsoleConnection&								roConsoleConnection(CConsoleConnection::Instance());


		size_t														anCurrentMessage[CHotUpdateSystem::eNumberOfPlatforms];

		memset(&anCurrentMessage,0,sizeof(size_t)*CHotUpdateSystem::eNumberOfPlatforms);

		std::vector<SSyncMessage*>				cstAnalyzedSyncMessage;

		CHotUpdateSystem	oLocalSettings(gSettings.oHotUpdateSystemSettings);
		

		//////////////////////////////////////////////////////////////////////////
		// Probably not needed anymore, but...
		m_oConsoleOptionsChangedMutex.Lock();
		// If the console options changed...
		if (m_boConsoleOptionsChanged)
		{
			// We discard all the messages currently in the queue.
			ClearMessageQueue();
			// ...and reset the flag...
			m_boConsoleOptionsChanged=false;

			// .. and reset connections...
			for (unsigned int i=CHotUpdateSystem::eFirstPlatform;
				i<CHotUpdateSystem::eNumberOfPlatforms; ++i)
			{
				CConsoleConnection::Instance().Instance().NotificationReset(
					(CHotUpdateSystem::eConsolePlatforms)i);
			}

			// And if we started syncing in realtime...
			if (m_boMustStartEnablingRealtimeSync)
			{
				m_boMustStartEnablingRealtimeSync=false;
				// We send the default first sync messages...
				SyncTimeOfDay();
				SyncMovieSystem();
				StoreConsoleVariableOriginalCallbacks();
				SetLivePreviewConsoleVariables();
			}
			else if (m_boMustStartDisablingRealtimeSync)
			{
				m_boMustStartDisablingRealtimeSync=false;
				// If we stopped it, we unbind the console variables...
				RestoreConsoleVariableOriginalCallbacks();
				RestoreLivePreviewConsoleVariables();
				m_oConsoleOptionsChangedMutex.Unlock();
				continue;
			}			
			else
			{
				// Nothing to do, just unlock the messages.
				m_oConsoleOptionsChangedMutex.Unlock();
				continue;
			}
		}
		m_oConsoleOptionsChangedMutex.Unlock();
		//////////////////////////////////////////////////////////////////////////


		//////////////////////////////////////////////////////////////////////////
		// If the passthrough mechanism is not enabled...
		if (!m_nPassThrough)
		{

			// We start by requesting a connection.
			for (nCount=CHotUpdateSystem::eFirstPlatform;nCount<CHotUpdateSystem::eNumberOfPlatforms;++nCount)
			{
				pClient[nCount]=NULL;
				if (!oLocalSettings.boPlatformEnabled[nCount])
				{
					continue;
				}
				pClient[nCount] = roConsoleConnection.NotificationBegin((CHotUpdateSystem::eConsolePlatforms)nCount);
			}

		}
		//////////////////////////////////////////////////////////////////////////


		//////////////////////////////////////////////////////////////////////////
		if (!m_nPassThrough)
		{
			bool boMustTrySendingMessages(false);


			do
			{
				boMustTrySendingMessages=false;

				//////////////////////////////////////////////////////////////////////////
				// While we have messages scheduled for updates...
				while (!m_cUpdateSchedule.empty())
				{
					// We remove the message from the update schedule...
					pstCurrentMessage=m_cUpdateSchedule.pop();
					if (!pstCurrentMessage)
					{
						continue;
					}

					// The node must be valid, as we need to already know the message type.
					if (!pstCurrentMessage->node)
					{
						assert("CConsoleSynchronization::Run()"&&pstCurrentMessage->node);
						SAFE_DELETE(pstCurrentMessage);
						continue;
					}

					// And we add the message to the analyzed messages vector...
					cstAnalyzedSyncMessage.push_back(pstCurrentMessage);
				}
				//////////////////////////////////////////////////////////////////////////



			// For each platform...
			for (nCount=CHotUpdateSystem::eFirstPlatform;nCount<CHotUpdateSystem::eNumberOfPlatforms;++nCount)
			{
				// If it is not enabled...
				if (!oLocalSettings.boPlatformEnabled[nCount])
				{
					// We ignore the platform...
					// We move the index of the currently sent messages to the last
					// and move the current message index for this platform to the
					// end of the message vector.
					anCurrentMessage[nCount]=cstAnalyzedSyncMessage.size();
					continue;
				}


				//////////////////////////////////////////////////////////////////////////
				roConsoleConnection.BeginStatusCheck((CHotUpdateSystem::eConsolePlatforms)nCount);

				if (roConsoleConnection.IsConnected((CHotUpdateSystem::eConsolePlatforms)nCount)) // If we succeeded to connect...(or we were already connected)
				{
					// We send for this platform all the messages which we didn't send yet...
					while (anCurrentMessage[nCount]<cstAnalyzedSyncMessage.size())
					{
						// We prepare the message...
						PrepareMessage(cstAnalyzedSyncMessage[(anCurrentMessage[nCount])], cuchSendBuffer);
						// And send it.
						pClient[nCount]->Send("RealtimeUpdate",&cuchSendBuffer.front(),cuchSendBuffer.size());
						// And advance the current message index for this platform.
						++(anCurrentMessage[nCount]);
					}
				}
				else if (roConsoleConnection.IsFailedToConnect((CHotUpdateSystem::eConsolePlatforms)nCount)) 				// If we failed to connect to the platform...
				{
					// We ignore the platform...
					// We move the index of the currently sent messages to the last
					// and move the current message index for this platform to the
					// end of the message vector.
					anCurrentMessage[nCount]=cstAnalyzedSyncMessage.size();
					roConsoleConnection.EndStatusCheck((CHotUpdateSystem::eConsolePlatforms)nCount);
					continue;
				}			
				else
				{
					if (!roConsoleConnection.IsConnecting((CHotUpdateSystem::eConsolePlatforms)nCount))
					{
						roConsoleConnection.Connect((CHotUpdateSystem::eConsolePlatforms)nCount);
					}
				}

				roConsoleConnection.EndStatusCheck((CHotUpdateSystem::eConsolePlatforms)nCount);
				//////////////////////////////////////////////////////////////////////////


			}




			// For each platform...
			for (nCount=CHotUpdateSystem::eFirstPlatform;nCount<CHotUpdateSystem::eNumberOfPlatforms;++nCount)
			{
				// We check if we still have messages to send...
				if (anCurrentMessage[nCount]<cstAnalyzedSyncMessage.size())
				{
					boMustTrySendingMessages=true;
				}
			}



			} while (boMustTrySendingMessages);


			// We start by requesting a connection.
			for (nCount=CHotUpdateSystem::eFirstPlatform;nCount<CHotUpdateSystem::eNumberOfPlatforms;++nCount)
			{
				if (!oLocalSettings.boPlatformEnabled[nCount])
				{
					continue;
				}

				if (!pClient[nCount])
				{
					continue;
				}

				roConsoleConnection.NotificationEnd((CHotUpdateSystem::eConsolePlatforms)nCount);
			}


		}
		else
		{
			// We send all the pending messages to the pass-through mechanism
			for (size_t nCount=0,nTotal(cstAnalyzedSyncMessage.size());nCount<nTotal;++nCount)
			{
				PrepareMessage(cstAnalyzedSyncMessage[nCount], cuchSendBuffer);
				if (gEnv->pGame)
				{
					IRealtimeRemoteUpdate * pRemoteUpdate = gEnv->pGame->GetIGameFramework()->GetIRealTimeRemoteUpdate();
					if ( pRemoteUpdate )
					{
						pRemoteUpdate->OnNotificationNetworkReceive(&cuchSendBuffer.front(),cuchSendBuffer.size());
					}
				}
			}
		}
		//////////////////////////////////////////////////////////////////////////



		//////////////////////////////////////////////////////////////////////////
		for (size_t nCount(0),nTotalItems(cstAnalyzedSyncMessage.size());nCount<nTotalItems;++nCount)
		{
			SAFE_DELETE(cstAnalyzedSyncMessage[nCount]);
		}
		cstAnalyzedSyncMessage.clear();
		//////////////////////////////////////////////////////////////////////////



	}
}

//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::ResumeThread()
{
	m_oUpdateListConditionVariable.Notify();
	m_oUpdateListConditionMutex.Unlock();	
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::SendMessage( SSyncMessage* msg )
{
	m_cUpdateSchedule.push(msg);
	ResumeThread();
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::Sync3DEngineData( int nSyncFlags,AABB &bounds )
{
	IEditor*															piIEditor=GetIEditor();
	I3DEngine*														pi3DEngine=piIEditor->Get3DEngine();

	SSyncMessage*													poSyncMessage(NULL);

	int																		nCompiledDataSize(0);
	ITerrain*															piTerrain=pi3DEngine->GetITerrain();
	IVisAreaManager*											piIVisAreaManager=pi3DEngine->GetIVisAreaManager();

	std::vector<struct IStatObj*>*				pTempBrushTable = NULL;
	std::vector<struct IMaterial*>*				pTempMatsTable = NULL;

	if (!piTerrain)
	{
		return;
	}

  // set export parameters
	SHotUpdateInfo info;
	info.nHeigtmap = (nSyncFlags & SYNC_TERRAIN) != 0;
  info.nObjTypeMask = (nSyncFlags & SYNC_OBJECTS) ? ~0 : 0; // by default export all types (~0), for example in case of vegetation use (1<<eERType_Vegetation)
	if (nSyncFlags & SYNC_WITH_BBOX)
	{
		info.areaBox = bounds;
		//CryLog( "bbox=(%.0f,%.0f,%.0f),(%.0f,%.0f,%.0f)",bounds.min.x,bounds.min.y,bounds.min.z,bounds.max.x,bounds.max.y,bounds.max.z );
	}
	else
	{
		info.areaBox.Reset();
	}

	if(nCompiledDataSize = piTerrain->GetCompiledDataSize(&info))
  {
    // send terrain data
    poSyncMessage=new SSyncMessage();
    poSyncMessage->data.resize(nCompiledDataSize+sizeof(info),0);
    memcpy(&poSyncMessage->data.front(),&info,sizeof(info));
    SwapEndian(*(SHotUpdateInfo*)&poSyncMessage->data.front(), !GetPlatformEndian());
    piTerrain->GetCompiledData(&poSyncMessage->data.front()+sizeof(info),nCompiledDataSize,&pTempBrushTable,&pTempMatsTable,!GetPlatformEndian(),&info);
    poSyncMessage->node=CreateXmlNode("SyncMessage");
    poSyncMessage->node->setAttr("Type","EngineTerrainData");
    SendMessage(poSyncMessage);

    if(pTempBrushTable && pTempMatsTable)
    {
      // Vis Areas message: here for depending on pTempBrushTable and pTempMatsTable.
      if(nCompiledDataSize = piIVisAreaManager->GetCompiledDataSize(&info))
      {
        // send indoor instances
        poSyncMessage=new SSyncMessage();
        poSyncMessage->data.resize(nCompiledDataSize+sizeof(info),0);
        memcpy(&poSyncMessage->data.front(),&info,sizeof(info));
        SwapEndian(*(SHotUpdateInfo*)&poSyncMessage->data.front(), !GetPlatformEndian());
        piIVisAreaManager->GetCompiledData(&poSyncMessage->data.front()+sizeof(info),nCompiledDataSize,&pTempBrushTable,&pTempMatsTable,!GetPlatformEndian(),&info);
        poSyncMessage->node=CreateXmlNode("SyncMessage");
        poSyncMessage->node->setAttr("Type","EngineIndoorData");
        SendMessage(poSyncMessage);
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::SyncDetailLayers()
{
	// See CGameEngine::ReloadSurfaceTypes
	SSyncMessage*													poSyncMessage(NULL);
	CCryEditDoc														*doc(GetIEditor()->GetDocument());
	if (!doc)
	{
		return;
	}

	poSyncMessage=new SSyncMessage();

	poSyncMessage->node=CreateXmlNode("SyncMessage");
	poSyncMessage->node->setAttr("Type","DetailLayers");

	XmlNodeRef oCurrentNode;
	XmlNodeRef oRootNode = poSyncMessage->node->newChild( "SurfaceTypes" );
	
	// Write all surface types.
	for (int i = 0; i < CTerrainManager::GetTerrainManager().GetSurfaceTypeCount(); i++)
	{
		oCurrentNode = oRootNode->newChild( "SurfaceType" );
		CTerrainManager::GetTerrainManager().GetSurfaceTypePtr(i)->Serialize(oCurrentNode,false);
	}

	SendMessage(poSyncMessage);	
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::SyncEnvironmentData()
{
  CCryEditDoc*					poDoc(GetIEditor()->GetDocument());
	SSyncMessage*					poSyncMessage(NULL);

	if (!poDoc)
	{
		return;
	}
	poSyncMessage=new SSyncMessage();

	poSyncMessage->node=CreateXmlNode("SyncMessage");
	poSyncMessage->node->setAttr("Type","Environment");

	XmlNodeRef env = poSyncMessage->node->newChild("Environment");
	CXmlTemplate::SetValues( poDoc->GetEnvironmentTemplate(),env );

	//! Add lighting node to environment settings.
	poDoc->GetCurrentMission()->GetLighting()->Serialize( env,false );

	SendMessage(poSyncMessage);
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::SyncVoxels()
{
	IEditor*											piIEditor=GetIEditor();
	I3DEngine*										pi3DEngine=piIEditor->Get3DEngine();
	IVoxTerrain*									pVoxTerrain = pi3DEngine->GetIVoxTerrain();

	_smart_ptr<IMemoryBlock>			pMB(NULL);

	SSyncMessage*									poSyncMessage(NULL);

	int														nBinaryDataSize(0);

	if(pVoxTerrain)
	{
		pMB = pVoxTerrain->GetCompiledData(true, !GetPlatformEndian(), false, NULL);
		if (!pMB)
		{
			return;
		}

		nBinaryDataSize=pMB->GetSize();
		if(nBinaryDataSize)
		{
			poSyncMessage=new SSyncMessage();
			poSyncMessage->data.resize(nBinaryDataSize,0);
			memcpy(&poSyncMessage->data.front(),pMB->GetData(),nBinaryDataSize);
		}
	}
	else
	{
		poSyncMessage=new SSyncMessage();
	}


	poSyncMessage->node=CreateXmlNode("SyncMessage");
	poSyncMessage->node->setAttr("Type","Voxel");

	SendMessage(poSyncMessage);
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::SyncVegetation()
{
	assert(false);

	IEditor*											piIEditor=GetIEditor();

	SSyncMessage*									poSyncMessage(NULL);

	//////////////////////////////////////////////////////////////////////////
	/// Export vegetation objects.
	poSyncMessage=new SSyncMessage();
	poSyncMessage->node=CreateXmlNode("SyncMessage");
	poSyncMessage->node->setAttr("Type","Vegetation");

	XmlNodeRef vegetationNode = poSyncMessage->node->newChild( "Vegetation" );
	CVegetationMap *pVegetationMap = piIEditor->GetVegetationMap();
	for (int i = 0; i < pVegetationMap->GetObjectCount(); i++)
	{
		XmlNodeRef node = vegetationNode->newChild( "Object" );
		pVegetationMap->GetObject(i)->Serialize( node,false );
	}
	//////////////////////////////////////////////////////////////////////////

	SendMessage(poSyncMessage);
}
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::SyncEntities()
{
	//////////////////////////////////////////////////////////////////////////
	// Serialize objects.
	//////////////////////////////////////////////////////////////////////////
	XmlNodeRef xmlNode = GetISystem()->CreateXmlNode( "Entities" );
	GetIEditor()->GetObjectManager()->ExportEntities( xmlNode);

	SSyncMessage* poSyncMessage=new SSyncMessage();
	poSyncMessage->node=CreateXmlNode("SyncMessage");
	poSyncMessage->node->setAttr("Type","Entities");
	poSyncMessage->node->addChild( xmlNode );

	SendMessage( poSyncMessage );

	m_bHasSyncedEntityIDs=true;
}

//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::OnHeightmapModified(const AABB &bounds )
{
	if ((!gSettings.oHotUpdateSystemSettings.boRealtimeSyncEnabled)||(m_bMustInterruptUpdates))
		return;

	if (!m_bHeightmapModified)
		m_modifiedHeightmapBounds = bounds;
	else
		m_modifiedHeightmapBounds.Add(bounds);
	m_bHeightmapModified = true;
}

//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::OnVegetationModified(const AABB &bounds )
{
	if ((!gSettings.oHotUpdateSystemSettings.boRealtimeSyncEnabled)||(m_bMustInterruptUpdates))
		return;

	if (!m_bVegetationModified)
		m_modifiedVegetationBounds = bounds;
	else
		m_modifiedVegetationBounds.Add(bounds);
	m_bVegetationModified = true;
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::OnRoadModified(const AABB &bounds )
{
	if ((!gSettings.oHotUpdateSystemSettings.boRealtimeSyncEnabled)||(m_bMustInterruptUpdates))
		return;

	if (!m_bRoadModified)
		m_modifiedRoadBounds = bounds;
	else
		m_modifiedRoadBounds.Add(bounds);
	m_bRoadModified = true;
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::OnRiverModified(const AABB &bounds )
{
	if ((!gSettings.oHotUpdateSystemSettings.boRealtimeSyncEnabled)||(m_bMustInterruptUpdates))
		return;

	if (!m_bRiverModified)
		m_modifiedRiverBounds = bounds;
	else
		m_modifiedRiverBounds.Add(bounds);
	m_bRiverModified = true;
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::OnDecalModified(const AABB &bounds )
{
	if ((!gSettings.oHotUpdateSystemSettings.boRealtimeSyncEnabled)||(m_bMustInterruptUpdates))
		return;

	if (!m_bDecalModified)
		m_modifiedDecalBounds = bounds;
	else
		m_modifiedDecalBounds.Add(bounds);
	m_bDecalModified = true;
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::OnObjectModified( CBaseObject *pObject,bool bDelete,bool boTransformOnly )
{
	if ((!gSettings.oHotUpdateSystemSettings.boRealtimeSyncEnabled)||(m_bMustInterruptUpdates))
		return;

	if (pObject->IsKindOf( RUNTIME_CLASS(CEntity) ))
	{
		if (bDelete)
		{
			m_deletedEntities[pObject]=false;
		}
		else
		{
			m_modifiedEntities[pObject]=boTransformOnly;
		}
	}
	else if (
						pObject->IsKindOf( RUNTIME_CLASS(CBrushObject) ) 
						|| 
						pObject->IsKindOf( RUNTIME_CLASS(CSolidBrushObject) )
						||
						pObject->IsKindOf( RUNTIME_CLASS(CGroup) )
					)
	{
		AABB bounds;
		pObject->GetBoundBox( bounds );
		if (!m_bBrushesModified)
		{
			m_modifiedBrushesBounds = bounds;
		}
		else
			m_modifiedBrushesBounds.Add( bounds );

		m_bBrushesModified = true;
	}
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::OnCameraModified(Vec3& roPosition,Vec3& roDirection)
{
	if (!gSettings.oHotUpdateSystemSettings.boSyncCamera)
		return;

	m_oCameraPosition=roPosition;
	m_oCameraDirection=roDirection;

	m_bCameraModified=true;

	SyncCamera(eStartSync);

	m_bCameraModified=false;
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::SyncArchetypes()
{
	CEntityPrototypeManager*						poPrototypeManager=GetIEditor()->GetEntityProtManager();
	XmlNodeRef													xmlNode = GetISystem()->CreateXmlNode( "EntityPrototypes" );

	int																	nTotalLibraries(0);
	int																	nCurrentLibrary(0);

	int																	nTotalItems(0);
	int																	nCurrentItem(0);

	CEntityPrototype::SerializeContext	stSerializeContext;

	nTotalLibraries=poPrototypeManager->GetLibraryCount();
	for (nCurrentLibrary=0;nCurrentLibrary<nTotalLibraries;++nCurrentLibrary)
	{
		 IDataBaseLibrary* piLibrary(poPrototypeManager->GetLibrary(nCurrentLibrary));
		 nTotalItems=piLibrary->GetItemCount();
		 for (nCurrentItem=0;nCurrentItem<nTotalItems;++nCurrentItem)
		 {
			 XmlNodeRef xmlEntityNode=xmlNode->newChild("EntityPrototype");

			 stSerializeContext.bCopyPaste=false;
			 stSerializeContext.bIgnoreChilds=false;
			 stSerializeContext.bLoading=false;
			 stSerializeContext.bUndo=false;
			 stSerializeContext.bUniqName=false;
			 stSerializeContext.node=xmlEntityNode;

			 CBaseLibraryItem *pItem = (CBaseLibraryItem*)piLibrary->GetItem(nCurrentItem);
			 pItem->Serialize(stSerializeContext);
		 }
	}

	SSyncMessage* poSyncMessage=new SSyncMessage();
	poSyncMessage->node=CreateXmlNode("SyncMessage");
	poSyncMessage->node->setAttr("Type","EntityArchetype");
	poSyncMessage->node->addChild(xmlNode);

	SendMessage( poSyncMessage );	
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::SendKeepAliveMessage()
{
	SSyncMessage* poSyncMessage=new SSyncMessage();
	poSyncMessage->node=CreateXmlNode("SyncMessage");
	poSyncMessage->node->setAttr("Type","KeepAlive");
	poSyncMessage->node->setAttr("Timestamp",gEnv->pTimer->GetAsyncTime().GetSeconds(),false);

	SendMessage( poSyncMessage );	
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::OnEntityPrototypeModified(CEntityPrototype* poArchetype)
{
	if ((!gSettings.oHotUpdateSystemSettings.boRealtimeSyncEnabled)||(m_bMustInterruptUpdates))
		return;

	CEntityPrototype::SerializeContext	stSerializeContext;

	XmlNodeRef xmlNode = GetISystem()->CreateXmlNode( "EntityPrototypes" );
	XmlNodeRef xmlEntityNode=xmlNode->newChild("EntityPrototype");

	stSerializeContext.bCopyPaste=false;
	stSerializeContext.bIgnoreChilds=false;
	stSerializeContext.bLoading=false;
	stSerializeContext.bUndo=false;
	stSerializeContext.bUniqName=false;
	stSerializeContext.node=xmlEntityNode;

	poArchetype->Serialize(stSerializeContext);

	SSyncMessage* poSyncMessage=new SSyncMessage();
	poSyncMessage->node=CreateXmlNode("SyncMessage");
	poSyncMessage->node->setAttr("Type","EntityArchetype");
	poSyncMessage->node->addChild(xmlNode);

	SendMessage( poSyncMessage );
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::OnConsoleVariableChanged(ICVar*	piConsoleVariable)
{
	if ((!gSettings.oHotUpdateSystemSettings.boRealtimeSyncEnabled)||(m_bMustInterruptUpdates))
		return;

	SSyncMessage* poSyncMessage=new SSyncMessage();
	poSyncMessage->node=CreateXmlNode("SyncMessage");
	poSyncMessage->node->setAttr("Type","ConsoleVariables");
	poSyncMessage->node->setAttr(piConsoleVariable->GetName(),piConsoleVariable->GetString());

	SendMessage( poSyncMessage );
}

//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::OnTrackAnimated(const string& seqName, EAnimNodeType nodeType, SAnimContext& ac)
{
	if ((!gSettings.oHotUpdateSystemSettings.boRealtimeSyncEnabled)||(m_bMustInterruptUpdates))
		return;

	XmlNodeRef xmlNode = GetISystem()->CreateXmlNode( "AnimContext" );
	ac.Serialize( xmlNode,false );
	
	SSyncMessage* poSyncMessage=new SSyncMessage();
	poSyncMessage->node=CreateXmlNode("SyncMessage");
	poSyncMessage->node->setAttr("Type", "TrackAnimated");
	poSyncMessage->node->setAttr("nodeType", (int)nodeType);
	poSyncMessage->node->addChild(xmlNode);

	SendMessage( poSyncMessage );
}

//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::OnTerrainLayerPainted(unsigned char *newdata,int DataSize,int32 posx,int32 posy,int w,int h,ETEX_Format eTFSrc)
{
	if ((!gSettings.oHotUpdateSystemSettings.boRealtimeSyncEnabled)||(m_bMustInterruptUpdates))
		return;

	CLayerInfo	*poCurrentLayer(NULL);

	int64		nIndex(0);
	int32*	pnIndex((int32*)&nIndex);

	pnIndex[0]=posx;
	pnIndex[1]=posy;

	TDLayers::iterator	itIterator;

	itIterator=m_cLayers.find(nIndex);
	if (itIterator!=m_cLayers.end())
	{
		itIterator->second->SetData(newdata,DataSize);
	}
	else
	{
		poCurrentLayer=new CLayerInfo(posx,posy,w,h,(int)eTFSrc,newdata,DataSize);
		m_cLayers[nIndex]=poCurrentLayer;
	}

	int nTerrainSectorSize=gEnv->p3DEngine->GetTerrainSectorSize();

	// Switched Y and X for historical reasons.
	Vec3 p1((posy-1)*nTerrainSectorSize,(posx-1)*nTerrainSectorSize,-32000.0f);
	Vec3 p2((posy+1)*nTerrainSectorSize,(posx+1)*nTerrainSectorSize,+32000.0f);
	AABB	stAABB(p1,p1);


	stAABB.Add(p2);

	if (!m_bHeightmapModified)
	{
		m_modifiedHeightmapBounds = stAABB;
	}
	else
	{
		m_modifiedHeightmapBounds.Add(stAABB);
	}
	m_bHeightmapModified = true;
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::OnParticlesUpdated(CParticleItem*	poParticleItem)
{
	if ((!gSettings.oHotUpdateSystemSettings.boRealtimeSyncEnabled)||(m_bMustInterruptUpdates))
		return;

	IParticleEffect											*piEffect(NULL);
	IDataBaseLibrary										*piLibrary(NULL);
	IDataBaseItem::SerializeContext			stSerializeContext;
	XmlNodeRef													xmlNode;
	XmlNodeRef													xmlEffectNode;
	string															strEffectName;
	string															strLibraryName;

	piEffect=poParticleItem->GetEffect();
	piLibrary=poParticleItem->GetLibrary();

	strEffectName=piEffect->GetName();
	strLibraryName=piLibrary->GetName();
	xmlNode=CreateXmlNode("ParticleLibrary");
	xmlNode->setAttr("Effect",strEffectName);
	xmlNode->setAttr("Name",strLibraryName);
	xmlEffectNode=xmlNode->newChild("Effect");
	piEffect->Serialize(xmlEffectNode,false,true);

	m_cParticleEffectInfoContainer[strEffectName]=xmlNode;
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::LaunchGameLevelOnConsole()
{
	int				nCurrentPlatform(0);
	CString			strExecutableName;
	CString			strConsoleRootDirectory;
	CString			strTargetName;
	CString			strLevelFolder(GetIEditor()->GetLevelFolder());
	CString			strMap(Path::GetFileName(strLevelFolder));
	CString			strMapCommand;

	if (strMap.GetLength())
	{
		strMapCommand+="+map ";
		strMapCommand+=strMap;
	}

	for (nCurrentPlatform=CHotUpdateSystem::eFirstPlatform;nCurrentPlatform<CHotUpdateSystem::eNumberOfPlatforms;++nCurrentPlatform)
	{
		if (gSettings.oHotUpdateSystemSettings.boPlatformEnabled[nCurrentPlatform])
		{
			CConsoleConnection::Instance().Launch
				(
				(CHotUpdateSystem::eConsolePlatforms)nCurrentPlatform,
				gSettings.oHotUpdateSystemSettings.GetPlatformExecutableName(nCurrentPlatform,strExecutableName).GetBuffer(),
				gSettings.oHotUpdateSystemSettings.GetPlatformConsoleRootDirectory(nCurrentPlatform,strConsoleRootDirectory).GetBuffer(),
				strMapCommand,
				(gSettings.oHotUpdateSystemSettings.boPlatformAutodetectTargetMachine[nCurrentPlatform] ? NULL : gSettings.oHotUpdateSystemSettings.GetPlatformTargetMachine(nCurrentPlatform,strTargetName).GetBuffer())
				);
		}
	}
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::LoadLevelOnConsole()
{
	CString strLevelFolder(GetIEditor()->GetLevelFolder());
	CString strLevelName(Path::GetFileName(strLevelFolder));

	SSyncMessage* poSyncMessage=new SSyncMessage();
	poSyncMessage->node=CreateXmlNode("SyncMessage");
	poSyncMessage->node->setAttr("Type","ChangeLevel");
	poSyncMessage->node->setAttr("LevelName",strLevelName);

	SendMessage(poSyncMessage);  
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::CloseLevel()
{
	CString strLevelFolder(GetIEditor()->GetLevelFolder());

	SSyncMessage* poSyncMessage=new SSyncMessage();
	poSyncMessage->node=CreateXmlNode("SyncMessage");
	poSyncMessage->node->setAttr("Type","CloseLevel");

	SendMessage(poSyncMessage);
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::SyncTimeOfDay(bool boFullSync)
{
	if (!boFullSync)
	{
		if ((!gSettings.oHotUpdateSystemSettings.boRealtimeSyncEnabled)||(m_bMustInterruptUpdates))
			return;
	}

	XmlNodeRef xmlNode = GetISystem()->CreateXmlNode( "TimeOfDay" );
	GetIEditor()->Get3DEngine()->GetTimeOfDay()->Serialize( xmlNode,false );

	SSyncMessage* poSyncMessage=new SSyncMessage();
	poSyncMessage->node=CreateXmlNode("SyncMessage");
	poSyncMessage->node->setAttr("Type","TimeOfDay");
	poSyncMessage->node->addChild( xmlNode );

	SendMessage( poSyncMessage );
}

//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::SyncMovieSystem(bool boFullSync)
{
	if (!boFullSync)
	{
		if ((!gSettings.oHotUpdateSystemSettings.boRealtimeSyncEnabled)||(m_bMustInterruptUpdates))
			return;
	}

	XmlNodeRef xmlNode = GetISystem()->CreateXmlNode( "MovieSystem" );
	GetIEditor()->GetMovieSystem()->Serialize( xmlNode,false );

	SSyncMessage* poSyncMessage=new SSyncMessage();
	poSyncMessage->node=CreateXmlNode("SyncMessage");
	poSyncMessage->node->setAttr("Type","MovieSystem");
	poSyncMessage->node->addChild( xmlNode );

	SendMessage( poSyncMessage );
}

//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::SyncConsoleVariables()
{
	int									nNumberOfVariables(0);
	int									nCurrentVariable(0);
	IConsole*						piConsole(NULL);
	ICVar*							piVariable(NULL);
	std::vector<char*>	cszVariableNames;

	SSyncMessage* poSyncMessage=new SSyncMessage();
	poSyncMessage->node=CreateXmlNode("SyncMessage");
	poSyncMessage->node->setAttr("Type","ConsoleVariables");

	piConsole=gEnv->pConsole;
	nNumberOfVariables=piConsole->GetNumVars();
	cszVariableNames.resize(nNumberOfVariables,NULL);

	if (piConsole->GetSortedVars((const char**)&cszVariableNames.front(),nNumberOfVariables,NULL)!=nNumberOfVariables)
	{
		assert(false);
		return;
	}

	for (nCurrentVariable=0;nCurrentVariable<cszVariableNames.size();++nCurrentVariable)
	{
		if ( stricmp(cszVariableNames[nCurrentVariable],"_TestFormatMessage") == 0 )
			continue;

		piVariable=piConsole->GetCVar(cszVariableNames[nCurrentVariable]);
		if (!piVariable)
		{
			assert(false);
			continue;
		}
		poSyncMessage->node->setAttr(cszVariableNames[nCurrentVariable],piVariable->GetString());
	}

	SendMessage( poSyncMessage );
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::SyncMaterial( CMaterial *pMtl )
{
	if ((!gSettings.oHotUpdateSystemSettings.boRealtimeSyncEnabled)||(m_bMustInterruptUpdates))
		return;

	m_modifiedMaterials.insert( pMtl );
}

//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::SyncModifiedMaterials()
{
	// Send serialized material to the console.

	SSyncMessage* poSyncMessage=new SSyncMessage();
	poSyncMessage->node=CreateXmlNode("SyncMessage");
	poSyncMessage->node->setAttr("Type","Materials");

	XmlNodeRef rootNode = poSyncMessage->node->newChild( "Materials" );

	for (MaterialSet::const_iterator it = m_modifiedMaterials.begin(); it != m_modifiedMaterials.end(); ++it)
	{
		CMaterial *pMtl = *it;
		CMaterial::SerializeContext ctx;
		ctx.node = rootNode->newChild( "Material" );
		ctx.bLoading = false;
		pMtl->Serialize( ctx );

		ctx.node->setAttr( "name",pMtl->GetName() );
	}

	m_modifiedMaterials.clear();

	SendMessage( poSyncMessage );
}

//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::SyncModifiedEntities()
{
	if (!m_modifiedEntities.empty())
	{
		// Update single entity.
		XmlNodeRef xmlNode = GetISystem()->CreateXmlNode( "Entities" );
		xmlNode->setAttr( "PartialUpdate",1 );

		SSyncMessage* poSyncMessage=new SSyncMessage();
		poSyncMessage->node=CreateXmlNode("SyncMessage");
		poSyncMessage->node->setAttr("Type","Entities");
		poSyncMessage->node->addChild( xmlNode );

		for (ObjectSet::const_iterator it = m_modifiedEntities.begin(); it != m_modifiedEntities.end(); ++it)
		{
			CBaseObject *pObject = it->first;
			XmlNodeRef xmlEntityNode=pObject->Export( "",xmlNode );
			// For editor only entities, which should, therefore, not be synced in 
			// the console.
			if (!xmlEntityNode)
			{
				continue;
			}
			xmlEntityNode->setAttr("TransformOnly",it->second);
		}
		m_modifiedEntities.clear();

		SendMessage( poSyncMessage );
	}

	if (!m_deletedEntities.empty())
	{
		// Update single entity.
		XmlNodeRef xmlNode = GetISystem()->CreateXmlNode( "Entities" );
		xmlNode->setAttr( "PartialUpdate",1 );
		xmlNode->setAttr( "Delete",1 );

		SSyncMessage* poSyncMessage=new SSyncMessage();
		poSyncMessage->node=CreateXmlNode("SyncMessage");
		poSyncMessage->node->setAttr("Type","Entities");
		poSyncMessage->node->addChild( xmlNode );

		for (ObjectSet::const_iterator it = m_deletedEntities.begin(); it != m_deletedEntities.end(); ++it)
		{
			CBaseObject *pObject = it->first;
			pObject->Export( "",xmlNode );
		}
		m_deletedEntities.clear();

		SendMessage( poSyncMessage );
	}
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::SyncParticles()
{
	SSyncMessage* poSyncMessage=new SSyncMessage();
	poSyncMessage->node=CreateXmlNode("SyncMessage");
	poSyncMessage->node->setAttr("Type","Particles");

	GetIEditor()->GetParticleManager()->Export(poSyncMessage->node);

	SendMessage(poSyncMessage);
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::FlushModifiedData()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_EDITOR);

	if (m_bScheduleMovieSystemSync)
	{
		SyncMovieSystem();
		m_bScheduleMovieSystemSync = false;
	}

	if (m_bScheduleTimeOfDaySync)
	{
		SyncTimeOfDay();
		m_bScheduleTimeOfDaySync = false;
	}

	if (!m_modifiedMaterials.empty())
	{
		SyncModifiedMaterials();
	}
	if (!m_modifiedEntities.empty() || !m_deletedEntities.empty())
	{
		// We MUST sync all entities at least once or the indices
		// for the entities will most likely be wrong.
		if (!m_bHasSyncedEntityIDs)
		{
			SyncEntities();

			// Syncing all entities breaks the time of day.
			// and so we sync it again.
			SyncTimeOfDay();

			m_bScheduleTimeOfDaySync = false;
		}

		SyncModifiedEntities();
	}

	// Each type of render node (brush, vegetation, road, river, decal, etc...)
	// should be later separated as the engine is actually capable of sending
	// per type data. This will optimize data size and speed.
	if (
			m_bBrushesModified || 
			m_bVegetationModified || 
			m_bRoadModified || 
			m_bRiverModified || 
			m_bDecalModified
		 )
	{
		AABB box = m_modifiedVegetationBounds;

		if (!m_modifiedBrushesBounds.IsReset())
			box.Add(m_modifiedBrushesBounds);

		if (!m_modifiedRoadBounds.IsReset())
			box.Add(m_modifiedRoadBounds);

		if (!m_modifiedRiverBounds.IsReset())
			box.Add(m_modifiedRiverBounds);

		if (!m_modifiedDecalBounds.IsReset())
			box.Add(m_modifiedDecalBounds);

		// This flag system should be changed later to support
		// multiple render node types.
		Sync3DEngineData( SYNC_OBJECTS|SYNC_WITH_BBOX,box );

		m_bBrushesModified = false;
		m_bVegetationModified = false;
		m_bRoadModified = false;
		m_bRiverModified = false;
		m_bDecalModified = false;

		m_modifiedBrushesBounds.Reset();
		m_modifiedVegetationBounds.Reset();
		m_modifiedRoadBounds.Reset();
		m_modifiedRiverBounds.Reset();
		m_modifiedDecalBounds.Reset();
	}
	if (m_bHeightmapModified)
	{
		Sync3DEngineData( SYNC_TERRAIN|SYNC_WITH_BBOX,m_modifiedHeightmapBounds );
		m_bHeightmapModified = false;
		m_modifiedHeightmapBounds.Reset();
	}
	if (m_bCameraModified)
	{
		SyncCamera();
		m_bCameraModified=false;
	}



	{
		TDLayers::iterator	itIterator;
		TDLayers::iterator	itIteratorEnd;

		itIteratorEnd=m_cLayers.end();
		for (itIterator=m_cLayers.begin();itIterator!=itIteratorEnd;itIterator++)
		{
			_smart_ptr<CLayerInfo>& rpoLayerInfoStruct=itIterator->second;
			SSyncMessage* poSyncMessage=new SSyncMessage();
			poSyncMessage->node=CreateXmlNode("SyncMessage");
			poSyncMessage->node->setAttr("Type","LayerTexture");
			poSyncMessage->node->setAttr("Posx",rpoLayerInfoStruct->m_nPosX);
			poSyncMessage->node->setAttr("Posy",rpoLayerInfoStruct->m_nPosY);
			poSyncMessage->node->setAttr("w",rpoLayerInfoStruct->m_nw);
			poSyncMessage->node->setAttr("h",rpoLayerInfoStruct->m_nh);

//Diesel cut	
			//poSyncMessage->node->setAttr("ETEX_Format",eTF_DXT5);
//Diesel new
			poSyncMessage->node->setAttr("ETEX_Format",(int)eTF_DXT5);

			m_pSyncMessage = poSyncMessage;
			GetIEditor()->GetRenderer()->DXTCompress(rpoLayerInfoStruct->m_data,rpoLayerInfoStruct->m_nw,rpoLayerInfoStruct->m_nh,eTF_DXT5,true,false,4,Vec3(0.0f,0.0f,0.0f),DXTCompressCallback);
			m_pSyncMessage = NULL;
			SendMessage( poSyncMessage );
		}
		m_cLayers.clear();
	}


	{
		TDParticleEffectInfoContainer::iterator	itIterator;
		TDParticleEffectInfoContainer::iterator	itIteratorEnd;

		itIteratorEnd=m_cParticleEffectInfoContainer.end();
		for (itIterator=m_cParticleEffectInfoContainer.begin();itIterator!=itIteratorEnd;itIterator++)
		{
			SSyncMessage* poSyncMessage=new SSyncMessage();
			poSyncMessage->node=CreateXmlNode("SyncMessage");
			poSyncMessage->node->setAttr("Type","Particle.Library");
			poSyncMessage->node->addChild(itIterator->second);
			SendMessage( poSyncMessage );
		}
		m_cParticleEffectInfoContainer.clear();
	}
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::DXTCompressCallback( const void *data, size_t size, void *userData )
{
	SSyncMessage * pSyncMessage = CConsoleSynchronization::GetCConsoleSynchronization().m_pSyncMessage;
	pSyncMessage->data.resize(size,0);
	memcpy(&pSyncMessage->data.front(),data,size);
}

//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::FlushFullSyncData()
{
	if (!m_boStartedFullSync)
	{
		return;
	}
	// Here we schedule the sending of full-sync terrain packets.
	if (m_cTerrainBoundingBoxes.size()!=0)
	{
		Sync3DEngineData(SYNC_TERRAIN|SYNC_OBJECTS|SYNC_WITH_BBOX,m_cTerrainBoundingBoxes.back());
		m_cTerrainBoundingBoxes.pop_back();
	}
	else
	{
		SyncVoxels();
		SyncDetailLayers();
		SyncEnvironmentData();

		SyncTimeOfDay(true);
		SyncArchetypes();
		SyncEntities();
		SyncParticles();
		SyncMovieSystem(true);

		m_boStartedFullSync=false;
	}
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::OnIdleUpdate()
{
	if (m_bMustInterruptUpdates)
	{
		return;
	}

	CTimeValue t = gEnv->pTimer->GetAsyncTime();

	// This if for information we don't want to send all at once right away...
	// This information should have time to be processed, to allow memory 
	// consumption to remain lower (network buffers).
	// Currently, delayed information is processed 50 times per second, at most.
	if (fabs((t-m_lastDelayedInfoTime).GetSeconds()) > 0.02f)
	{
		FlushFullSyncData();
		m_lastDelayedInfoTime=t;
	}

	// When realtime sync is enabled, or the camera is syncing...
	if (
		(gSettings.oHotUpdateSystemSettings.boRealtimeSyncEnabled)
		||
		(gSettings.oHotUpdateSystemSettings.boSyncCamera)
		)
	{
		// For now we are sending keep alive messages every second, when either we are in a 
		// camera sync or a realtime sync.
		if (fabs((t-m_lastKeepAliveMessageTime).GetSeconds()) > 1.0f)
		{
			SendKeepAliveMessage();
			m_lastKeepAliveMessageTime=t;
		}
	}

	if ((!gSettings.oHotUpdateSystemSettings.boRealtimeSyncEnabled))
		return;

	// Flush data to console at most 10 times a second.
	if (fabs((t-m_lastDataFlushTime).GetSeconds()) > 0.1f)
	{
		FlushModifiedData();
		m_lastDataFlushTime = t;
	}
}

//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::OnEditorNotifyEvent( EEditorNotifyEvent event )
{
	switch (event)
	{
		case eNotify_OnTimeOfDayChange:
		{
			m_bScheduleTimeOfDaySync = true;
		}
		break;

		case eNotify_OnUpdateTrackView:
		case eNotify_OnUpdateTrackViewKeys:
		{
			m_bScheduleMovieSystemSync = true;
		}
		break;

		case eNotify_OnIdleUpdate:
		{
			OnIdleUpdate();
		}
		break;

		case eNotify_OnBeginNewScene:
		case eNotify_OnBeginLoad:
			{
				m_bMustInterruptUpdates=true;

				// If we are loading a level or
				m_bScheduleMovieSystemSync = false;
				m_bScheduleTimeOfDaySync=false;
				m_modifiedMaterials.clear();
				m_modifiedEntities.clear();
				m_deletedEntities.clear();
				m_cParticleEffectInfoContainer.clear();
				m_cTerrainBoundingBoxes.clear();
				m_cLayers.clear();

				m_bHasSyncedEntityIDs=false;
				m_bBrushesModified=false;
				m_bVegetationModified=false;
				m_bRoadModified = false;
				m_bRiverModified = false;
				m_bDecalModified = false;
				m_bHeightmapModified=false;
				m_boStartedFullSync=false;
			}
		break;

		case eNotify_OnBeginSceneSave:
			{
				m_bMustInterruptUpdates=true;
			}
		break;

		case eNotify_OnEndNewScene:
		case eNotify_OnEndSceneOpen:
			{
				m_bMustInterruptUpdates=false;
				m_bHasSyncedEntityIDs=false;

				// Unfortunately, the new level name will only be available here.
				OnLevelChanged();
			}
		break;

		case eNotify_OnEndSceneSave:
			{
				m_bMustInterruptUpdates=false;
			}
		break;
	}
}


//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::SetLivePreviewConsoleVariables()
{
	ICVar * pVar = GetIEditor()->GetSystem()->GetIConsole()->GetCVar("e_SyncConsoleLivePreviewMultiThreaded");
	if ( pVar )
	{
		if ( pVar->GetIVal() )
		{
			ICVar * pThreadedRenderingVar = GetIEditor()->GetSystem()->GetIConsole()->GetCVar("r_MultiThreaded");
			if ( pThreadedRenderingVar )
			{
				m_multiThreaded = pThreadedRenderingVar->GetIVal();
				pThreadedRenderingVar->Set(0);
			}
		}
	}
	ICVar * pGodModeVar = GetIEditor()->GetSystem()->GetIConsole()->GetCVar("g_GodMode");
	if ( pGodModeVar )
	{
		m_godMode = pGodModeVar->GetIVal();
		pGodModeVar->Set(1);
	}
	SyncConsoleVariables();
}

//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::RestoreLivePreviewConsoleVariables()
{
	ICVar * pVar = GetIEditor()->GetSystem()->GetIConsole()->GetCVar("e_SyncConsoleLivePreviewMultiThreaded");
	if ( pVar )
	{
		if ( pVar->GetIVal() )
		{
			ICVar * pThreadedRenderingVar = GetIEditor()->GetSystem()->GetIConsole()->GetCVar("r_MultiThreaded");
			if ( pThreadedRenderingVar )
			{
				pThreadedRenderingVar->Set(m_multiThreaded);
			}			
		}
	}

	ICVar * pGodModeVar = GetIEditor()->GetSystem()->GetIConsole()->GetCVar("g_GodMode");
	if ( pGodModeVar )
	{
		pGodModeVar->Set(m_godMode);
	}
	SyncConsoleVariables();
}

//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::StoreConsoleVariableOriginalCallbacks()
{
	int									nNumberOfVariables(0);
	int									nCurrentVariable(0);
	IConsole*						piConsole(NULL);
	ICVar*							piVariable(NULL);
	std::vector<char*>	cszVariableNames;

	m_cOriginalConsoleVariableCallbaks.clear();

	piConsole=gEnv->pConsole;
	nNumberOfVariables=piConsole->GetNumVars();
	cszVariableNames.resize(nNumberOfVariables,NULL);

	if (piConsole->GetSortedVars((const char**)&cszVariableNames.front(),nNumberOfVariables,NULL)!=nNumberOfVariables)
	{
		assert(false);
		return;
	}

	for (nCurrentVariable=0;nCurrentVariable<cszVariableNames.size();++nCurrentVariable)
	{
		piVariable=piConsole->GetCVar(cszVariableNames[nCurrentVariable]);
		if (!piVariable)
		{
			continue;
		}
		m_cOriginalConsoleVariableCallbaks[piVariable]=piVariable->GetOnChangeCallback();
		piVariable->SetOnChangeCallback(&CConsoleSynchronization::OnConsoleVariableChangedCallback);
	}
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::RestoreConsoleVariableOriginalCallbacks()
{
	TDOriginalConsoleVariableCallbaks::iterator	itIterator;
	TDOriginalConsoleVariableCallbaks::iterator	itEnd;

	itEnd=m_cOriginalConsoleVariableCallbaks.end();
	for (itIterator=m_cOriginalConsoleVariableCallbaks.begin();itIterator!=itEnd;itIterator++)
	{
		itIterator->first->SetOnChangeCallback(itIterator->second);
	}
	m_cOriginalConsoleVariableCallbaks.clear();
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::OnConsoleVariableChangedCallback(ICVar*	piConsoleVariable)
{
	GetCConsoleSynchronization().OnConsoleVariableChangedCallbackMethod(piConsoleVariable);
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::OnConsoleVariableChangedCallbackMethod(ICVar*	piConsoleVariable)
{
	TDOriginalConsoleVariableCallbaks::iterator	itIterator;

	itIterator=m_cOriginalConsoleVariableCallbaks.find(piConsoleVariable);
	if (itIterator==m_cOriginalConsoleVariableCallbaks.end())
	{
		return;
	}

	OnConsoleVariableChanged(piConsoleVariable);

	if (itIterator->second)
	{
		// Here we call the old callback.
		itIterator->second(piConsoleVariable);
	}
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::OnLevelChanged()
{
	if ((!gSettings.oHotUpdateSystemSettings.boRealtimeSyncEnabled)||(m_bMustInterruptUpdates))
		return;

	LoadLevelOnConsole();
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::ClearMessageQueue()
{
	SSyncMessage	*pstCurrentMessage(NULL);

	// Since we changed the options, and possibly the address or disabled it,
	// we must completely discard the current message queue.
	while (!m_cUpdateSchedule.empty()) 
	{
		pstCurrentMessage=m_cUpdateSchedule.pop();
		SAFE_DELETE(pstCurrentMessage);
	}
}
//////////////////////////////////////////////////////////////////////////
void CConsoleSynchronization::PrepareMessage( SSyncMessage* pstCurrentMessage, TDBuffer &cuchSendBuffer )
{
	size_t														nDataSize(0);
	size_t														nXmlLenght(0);
	size_t														nCompressedBinaryLenght(0);

	nCompressedBinaryLenght=pstCurrentMessage->data.size();
	pstCurrentMessage->node->setAttr("BinaryDataSize",(unsigned long)pstCurrentMessage->data.size());

	m_strXmlString=pstCurrentMessage->node->getXML();
	nXmlLenght=m_strXmlString.length()+1;
	if (nXmlLenght%4)
	{		
		m_strXmlString.append(4-(nXmlLenght%4),' ');
	}
	nXmlLenght=m_strXmlString.length()+1;
	assert(!(nXmlLenght%4));

	// The compressed data size and the uncompressed one are the same before compression.
	cuchSendBuffer.resize(nXmlLenght+pstCurrentMessage->data.size(),0);
	memcpy(&cuchSendBuffer.front(),m_strXmlString.c_str(),nXmlLenght);

	// Here we compress the the message data to the send buffer.
	if (nCompressedBinaryLenght != 0)
		GetISystem()->CompressDataBlock(&pstCurrentMessage->data.front(),pstCurrentMessage->data.size(),(&cuchSendBuffer.front()+nXmlLenght),nCompressedBinaryLenght);

	// Now that we know the compressed size, we resize it to the proper size.
	cuchSendBuffer.resize(nXmlLenght+nCompressedBinaryLenght);
}
//////////////////////////////////////////////////////////////////////////
