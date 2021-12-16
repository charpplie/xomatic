////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   ConsoleSync.h
//  Created:     21/1/2009 by Timur.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __ConsoleSync_h__
#define __ConsoleSync_h__
#pragma once

class CMaterial;
struct INotificationNetworkClient;
class CEntityPrototype;

class CParticleItem;

#include <Cry_Geo.h>
#include "CryThread.h"
#include "Settings.h"

//////////////////////////////////////////////////////////////////////////
class CConsoleSynchronization : public CryThread<CConsoleSynchronization>, public IEditorNotifyListener
{
public:
	struct SSyncMessage
	{
		XmlNodeRef node;
		std::vector<uint8> data;
	};

	typedef 	CryMT::CLocklessPointerQueue<SSyncMessage>				TDMessageList;
	typedef  std::vector<unsigned char>													TDBuffer;

	class CLayerInfo: public CRefCountBase
	{
		public:
			CLayerInfo();
			~CLayerInfo();
			CLayerInfo(int nPosX,int nPosY, int nw,int nh,int nFormat,unsigned char* szData, int nDataSize);
			CLayerInfo(const CLayerInfo& crOperand);
			CLayerInfo&	operator=(const CLayerInfo& crOperand);

			void SetData(uint8* uchData,int nSize);

			int m_nPosX;
			int m_nPosY;
			int m_nw;
			int m_nh;
			int m_nFormat;
			int m_nSize;

			uint8* m_data;
	};

	typedef   std::map<int64,_smart_ptr<CLayerInfo> >					TDLayers;

	enum ECameraOperarion
	{
		eNoOperarion=0,
		eStartSync=1,
		eEndSync=2,
	};
public:
	// Singleton class... we don't want multiple background threads to be running.
	static CConsoleSynchronization& GetCConsoleSynchronization();

	// This will cause 2 copies of the data: one to a pointer variable and one to the sending buffer.
	// Take care while sending big chunks of binary data.
	virtual void SendMessage( SSyncMessage& msg );

	// Sync as much as possible from editor to the console.
	virtual void FullSync();

	//Synchronize specific things.
	virtual void SyncMaterial( CMaterial *pMtl );

	// Sync all terrain features (except for the terraintexture.pak).
	virtual void SyncTerrain();

	// Sync camera attributes
	// Use ECameraOperarion as the parameter nOperarion.
	virtual void SyncCamera(int nOperarion=eNoOperarion);

	// Called when heightmap was modified
	virtual void OnHeightmapModified(const AABB &bounds );

	// Called when vegetation was modified
	virtual void OnVegetationModified(const AABB &bounds );

	// Called when a road was modified.
	virtual void OnRoadModified(const AABB &bounds );

	// Called when a river was modified.
	virtual void OnRiverModified(const AABB &bounds );

	// Called when a Decal was modified.
	virtual void OnDecalModified(const AABB &bounds );

	// Notify the class that the conection options have been changed.
	// Should only be called by ConsoleOptionsDialog.
	void NotifyOptionsChanged();
	// Sync all game Entities.
	virtual void SyncEntities();

	// Sync time of day.
	virtual void SyncTimeOfDay(bool boFullSync=false);

	virtual void SyncMovieSystem(bool boFullSync=false);

	//
	virtual void SyncConsoleVariables();

	//////////////////////////////////////////////////////////////////////////
	// Called when a track in the TrackView played by editor.
	void OnTrackAnimated(const string& seqName, EAnimNodeType nodeType, SAnimContext& ac);

	//////////////////////////////////////////////////////////////////////////
	// Called by the object manager when object parameters change.
	void OnObjectModified( CBaseObject *pObject,bool bDelete,bool boTransformOnly );

	//////////////////////////////////////////////////////////////////////////
	// Called by GameEngine when the camera is updated.
	void OnCameraModified(Vec3& roPosition,Vec3& roDirection);

	// Syncs the archetypes.
	void SyncArchetypes();

	// Keep Alive message.
	void SendKeepAliveMessage();

	//////////////////////////////////////////////////////////////////////////
	// Called by the Entity Prototypes when we have new archetype of an entity 
	// archetype is changed.
	void OnEntityPrototypeModified(CEntityPrototype* poArchetype);

	//////////////////////////////////////////////////////////////////////////
	// Called when a given console variable has changed.
	void OnConsoleVariableChanged(ICVar*	piConsoleVariable);

	//////////////////////////////////////////////////////////////////////////
	// Called when we update the terrain texture in the editor while painting
	// terrain layers.
	void OnTerrainLayerPainted(unsigned char *newdata,int DataSize,int32 posx,int32 posy,int w,int h,ETEX_Format eTFSrc=eTF_R8G8B8);

	//////////////////////////////////////////////////////////////////////////
	// Called when a particle prototype is changed.
	void OnParticlesUpdated(CParticleItem*	poParticleItem);

	//////////////////////////////////////////////////////////////////////////
	//
	void LaunchGameLevelOnConsole();

	//////////////////////////////////////////////////////////////////////////
	//
	void LoadLevelOnConsole();

	//////////////////////////////////////////////////////////////////////////
	// tells the console to close the currently opened level 
	void CloseLevel();


	//////////////////////////////////////////////////////////////////////////
	// is realtime syncing enabled
	bool IsRealTimeSyncing() { return m_bRealtimeSyncOn; }

	//////////////////////////////////////////////////////////////////////////
	// CryThread implementation.
	//////////////////////////////////////////////////////////////////////////
	// DO NOT CALL THIS METHOD.
	// This should be a protected method, it is not one just because it can't be in order to work
	// properly with Crytek.
	void Run();

	// Not thread safe.
	void PrepareMessage( SSyncMessage* pstCurrentMessage, TDBuffer &cuchSendBuffer );
	

	//////////////////////////////////////////////////////////////////////////
	// IEditorNotifyListener implementation
	//////////////////////////////////////////////////////////////////////////
	virtual void OnEditorNotifyEvent( EEditorNotifyEvent event );
	//////////////////////////////////////////////////////////////////////////
protected:
	CConsoleSynchronization();
	virtual ~CConsoleSynchronization();

	enum E3DEngineSyncFlags
	{
		SYNC_TERRAIN				= BIT(0),
		SYNC_OBJECTS				= BIT(1),
		SYNC_WITH_BBOX			= BIT(10),
	};

	void ResumeThread();

	// To avoid massive data copies, we must use internally pointers.
	// The pointer will be deleted internally later.
	virtual void SendMessage( SSyncMessage *msg );

	virtual void SyncVoxels();
	virtual void SyncDetailLayers();
	virtual void SyncEnvironmentData();
	virtual void SyncVegetation();

	// see E3DEngineSyncFlags
	void Sync3DEngineData( int nSyncFlags,AABB &bounds );

	void SyncModifiedMaterials();
	void SyncModifiedEntities();

	void SyncParticles();

	void OnIdleUpdate();
	void FlushModifiedData();

	// Even though more than just terrain can be added to this function later,
	// for now we will be using the name terrain.
	void FlushFullSyncData();

	void					SetLivePreviewConsoleVariables();
	void					RestoreLivePreviewConsoleVariables();
	void					StoreConsoleVariableOriginalCallbacks();
	void					RestoreConsoleVariableOriginalCallbacks();
	static void		OnConsoleVariableChangedCallback(ICVar*	piConsoleVariable);
	void					OnConsoleVariableChangedCallbackMethod(ICVar*	piConsoleVariable);

	void          OnLevelChanged();

	// This function is not thread safe.
	void          ClearMessageQueue();

	//static callback for dxt5 compression
	static void DXTCompressCallback( const void *data, size_t size, void *userData );

public:
protected:
	bool													m_bRealtimeSyncOn;
	bool													m_boConsoleOptionsChanged;
	bool													m_boMustStartEnablingRealtimeSync;
	bool													m_boMustStartDisablingRealtimeSync;

	bool													m_boMustEndThread;

	bool													m_bMustInterruptUpdates;

	TDMessageList									m_cUpdateSchedule;

	CryConditionVariable					m_oUpdateListConditionVariable;
	CryMutex											m_oUpdateListConditionMutex;
	CryMutex											m_oConsoleOptionsChangedMutex;

	int														m_nPassThrough;

	//////////////////////////////////////////////////////////////////////////
	// Should be removed later.
	int														m_nCameraPaddingSize;
	//////////////////////////////////////////////////////////////////////////
private:
	CTimeValue m_lastDataFlushTime;
	CTimeValue m_lastDelayedInfoTime;
	CTimeValue m_lastKeepAliveMessageTime;

	bool m_bScheduleTimeOfDaySync;
	bool m_bScheduleMovieSystemSync;

	typedef std::set<_smart_ptr<CMaterial> > MaterialSet;
	MaterialSet m_modifiedMaterials;

	typedef std::map<_smart_ptr<CBaseObject>,bool > ObjectSet;
	ObjectSet m_modifiedEntities;
	ObjectSet m_deletedEntities;
	bool			m_bHasSyncedEntityIDs;

	typedef std::map<ICVar*,ConsoleVarFunc>		TDOriginalConsoleVariableCallbaks;
	TDOriginalConsoleVariableCallbaks					m_cOriginalConsoleVariableCallbaks;

	typedef std::map<string,XmlNodeRef>				TDParticleEffectInfoContainer;
	TDParticleEffectInfoContainer							m_cParticleEffectInfoContainer;

	typedef std::vector<AABB>									TDBoundingBoxContainer;
	TDBoundingBoxContainer										m_cTerrainBoundingBoxes;
	int																				m_nTerrainBlockClusteringSize;
	bool																			m_boStartedFullSync;

	bool m_bBrushesModified;
	AABB m_modifiedBrushesBounds;

	bool m_bVegetationModified;
	AABB m_modifiedVegetationBounds;

	bool m_bHeightmapModified;
	AABB m_modifiedHeightmapBounds;

	bool m_bRoadModified;
	AABB m_modifiedRoadBounds;

	bool m_bRiverModified;
	AABB m_modifiedRiverBounds;

	bool m_bDecalModified;
	AABB m_modifiedDecalBounds;

	bool  m_bIsSyncingCamera;
	bool	m_bCameraModified;
	Vec3	m_oCameraPosition;
	Vec3	m_oCameraDirection;

	TDLayers	m_cLayers;


	// Allocated here to promote memory re-usage.
	XmlString													m_strXmlString;
	
	// static Member to contain the current message for dxt5 compression callback
	SSyncMessage * m_pSyncMessage;
		
	// stored live preview console variables
	int m_multiThreaded;
	int m_godMode;

};

#endif //__ConsoleSync_h__
