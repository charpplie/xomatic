/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   CAISystem.h
$Id$
Description: 

-------------------------------------------------------------------------
History:
Jeremy Gross Jan 2010:
Clean up work done (moving classes to own header files and such)
*********************************************************************/

#ifndef _CAISYSTEM_H_
#define _CAISYSTEM_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include "IAISystem.h"
#include "Reference.h"
#include "Formation.h"
#include "Graph.h"
#include "VertexList.h"
#include "PipeManager.h"
#include "AIObject.h"
#include "AICollision.h"
#include "AIGroup.h"
#include "AIQuadTree.h"
#include "MiniQueue.h"
#include "AIRadialOcclusion.h"
#include "AILightManager.h"
#include "AIDynHideObjectManager.h"
#include "Shape.h"
#include "ShapeContainer.h"
#include "Pathfinder.h"
#include "AIHideObject.h"
#include "Shape2.h"
#include "Navigation.h"
#include "HideSpot.h"
#include "VisionMap.h"
#include "CoordinationSystem/Coordination.h"

#ifdef CRYAISYSTEM_DEBUG
	#include "AIDbgRecorder.h"
	#include "AIRecorder.h"
#endif //CRYAISYSTEM_DEBUG

struct ICoordinationManager;
struct ITacticalPointSystem;
struct IBSSProfileManager;
struct AgentPathfindingProperties;
struct IFireCommandDesc;
struct IVisArea;
struct IAISignalExtraData;

class CAIActionManager;
class ICentralInterestManager;
class CPerceptionManager;
class CProfileDictionary;
class CEmotionalSystem;
class CCoopReadabilitiesSystem;
class CScriptBind_AI;
class CScriptBind_AI2;
class CScriptBind_AI_G02;
class CScriptBind_AI_G04;

namespace NewBehaviourTree
{
	class TreeProfileManager;
};

#define AGENT_COVER_CLEARANCE 0.35f
#define VIEW_RAY_PIERCABILITY 10
#define HIT_COVER ((geom_colltype_ray << rwi_colltype_bit) | rwi_colltype_any | (VIEW_RAY_PIERCABILITY & rwi_pierceability_mask))
#define HIT_SOFT_COVER (geom_colltype14 << rwi_colltype_bit)
#define COVER_OBJECT_TYPES (ent_terrain|ent_static|ent_rigid|ent_sleeping_rigid)

const static int NUM_ALERTNESS_COUNTERS = 4;



// Listener for path found events.
struct IAIPathFinderListerner
{
	virtual void OnPathResult(int id, const std::vector<unsigned>* pathNodes) = 0;
};



enum EGetObstaclesInRadiusFlags
{
	OBSTACLES_COVER = 0x01,
	OBSTACLES_SOFT_COVER = 0x02,
};

enum EPuppetUpdatePriority
{
	AIPUP_VERY_HIGH,
	AIPUP_HIGH,
	AIPUP_MED,
	AIPUP_LOW,
};

enum AsyncState
{
	AsyncFailed = 0,
	AsyncReady,
	AsyncInProgress,
	AsyncComplete,
};

//====================================================================
// CAISystem 
//====================================================================
class CAISystem : public IAISystem, public ISystemEventListener, public IAIObjectManager/*TEMP*/
{
public:
	//-------------------------------------------------------------

	CAISystem(ISystem *pSystem);
	~CAISystem();


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//IAISystem/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Basic////////////////////////////////////////////////////////////////////////////////////////////////////////

	VIRTUAL bool Init();

	VIRTUAL void Reload();
	VIRTUAL void Reset(EResetReason reason);//TODO this is called by lots of people including destructor, but causes NEW ALLOCATIONS! Needs big refactor!
	virtual void Release();

	//If disabled most things early out
	VIRTUAL void Enable(bool enable=true);

	//Every frame (multiple time steps per frame possible?)		//TODO find out
	//	currentTime - AI time since game start in seconds (GetCurrentTime)
	//	frameTime - since last update (GetFrameTime)
	VIRTUAL void Update(CTimeValue currentTime, float frameTime);

	VIRTUAL bool RegisterListener(IAISystemListener* pListener);
	VIRTUAL bool UnregisterListener(IAISystemListener* pListener);
	void OnAgentDeath(EntityId deadEntityID);

	// Registers AI event listener. Only events overlapping the sphere will be sent.
	// Register can be called again to update the listener position, radius and flags.
	// If pointer to the listener is specified it will be used instead of the pointer to entity.
	VIRTUAL void RegisterAIEventListener(IAIEventListener* pListener, const Vec3& pos, float rad, int flags);
	VIRTUAL void UnregisterAIEventListener(IAIEventListener* pListener);

	VIRTUAL void Event( int eventT, const char *);
	VIRTUAL IAISignalExtraData* CreateSignalExtraData() const;
	VIRTUAL void FreeSignalExtraData( IAISignalExtraData* pData ) const;
	VIRTUAL void SendSignal(unsigned char cFilter, int nSignalId,const char *szText,  IAIObject *pSenderObject, IAISignalExtraData* pData=NULL);
	VIRTUAL void SendAnonymousSignal(int nSignalId,const char *szText, const Vec3 &pos, float fRadius, IAIObject *pSenderObject,IAISignalExtraData* pData=NULL);
	VIRTUAL void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);

	//Basic////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////



	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Time/Updates/////////////////////////////////////////////////////////////////////////////////////////////////

	//Over-ride auto-disable for distant AIs
	VIRTUAL bool GetUpdateAllAlways() const;

	// Returns the current time (seconds since game began) that AI should be working with - 
	// This may be different from the system so that we can support multiple updates per
	// game loop/update.
	VIRTUAL CTimeValue GetFrameStartTime() const;

	// Time interval between this and the last update
	VIRTUAL float GetFrameDeltaTime() const;

	// returns the basic AI system update interval
	VIRTUAL float GetUpdateInterval() const;

	// profiling
	VIRTUAL int GetAITickCount();

	//Time/Updates/////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////



	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//AIObject/////////////////////////////////////////////////////////////////////////////////////////////////////

	VIRTUAL IAIObject *CreateAIObject(unsigned short type, IAIObject *pAssociation, EntityId entityID);

	VIRTUAL IAIObject *GetAIObject(tAIObjectID aiObjectID);
	VIRTUAL IAIObject *GetAIObjectByName(unsigned short type, const char *pName) const;

	// Returns AIObject iterator for first match, see EGetFirstFilter for the filter options.
	// The parameter 'n' specifies the type, group id or species based on the selected filter.
	// It is possible to iterate over all objects by setting the filter to OBJFILTER_TYPE
	// passing zero to 'n'.
	VIRTUAL IAIObjectIter* GetFirstAIObject(EGetFirstFilter filter, short n);
	// Iterates over AI objects within specified range.
	// Parameter 'pos' and 'rad' specify the enclosing sphere, for other parameters see GetFirstAIObject.
	VIRTUAL IAIObjectIter* GetFirstAIObjectInRange(EGetFirstFilter filter, short n, const Vec3& pos, float rad, bool check2D);

	VIRTUAL void RemoveObject(tAIObjectID objectID);

	//AIObject/////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////



	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//FileIO///////////////////////////////////////////////////////////////////////////////////////////////////////

	// save/load
	VIRTUAL void Serialize( TSerialize ser );

	//! Set a path for the current level as working folder for level-specific metadata
	VIRTUAL void SetLevelPath(const char *sPath);

	/// this called before loading (level load/serialization)
	VIRTUAL void FlushSystem(void);
	VIRTUAL void FlushSystemNavigation(void);

	VIRTUAL void LoadNavigationData(const char * szLevel, const char * szMission, bool demandLoadLNMs = false);
	VIRTUAL void LoadCover(const char* szLevel, const char* szMission);

	VIRTUAL bool LoadNavMesh(const char * navModifName, const char * agentTypeName);
	VIRTUAL bool UnloadNavMesh(const char * navModifName, const char * agentTypeName);

	VIRTUAL void OnMissionLoaded();

	//FileIO///////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Debugging////////////////////////////////////////////////////////////////////////////////////////////////////

	// AI DebugDraw
	VIRTUAL IAIDebugRenderer* GetAIDebugRenderer();
	VIRTUAL IAIDebugRenderer* GetAINetworkDebugRenderer() ;
	VIRTUAL void SetAIDebugRenderer				(IAIDebugRenderer* pAIDebugRenderer);
	VIRTUAL void SetAINetworkDebugRenderer(IAIDebugRenderer* pAINetworkDebugRenderer);

	// debug recorder
	VIRTUAL bool	IsRecording( const IAIObject* pTarget, IAIRecordable::e_AIDbgEvent event ) const;
	VIRTUAL void	Record( const IAIObject* pTarget, IAIRecordable::e_AIDbgEvent event, const char* pString ) const;
	VIRTUAL void	GetRecorderDebugContext(SAIRecorderDebugContext* &pContext);
	VIRTUAL void	AddDebugLine( const Vec3& start, const Vec3& end, uint8 r, uint8 g, uint8 b, float time );
	VIRTUAL void	AddDebugSphere(const Vec3& pos, float radius, uint8 r, uint8 g, uint8 b, float time);

	VIRTUAL void	DebugReportHitDamage(IEntity* pVictim, IEntity* pShooter, float damage, const char* material);
	VIRTUAL void	DebugReportDeath(IAIObject* pVictim);

	// functions to let external systems (e.g. lua) access the AI logging functions. 
	// the external system should pass in an identifier (e.g. "<Lua> ")
	VIRTUAL void Warning(const char * id, const char * format, ...)const PRINTF_PARAMS(3, 4);
	VIRTUAL void Error(const char * id, const char * format, ...) PRINTF_PARAMS(3, 4);
	VIRTUAL void LogProgress(const char * id, const char * format, ...) PRINTF_PARAMS(3, 4);
	VIRTUAL void LogEvent(const char * id, const char * format, ...) PRINTF_PARAMS(3, 4);
	VIRTUAL void LogComment(const char * id, const char * format, ...) PRINTF_PARAMS(3, 4);

	VIRTUAL bool IsAIInDevMode();

	// Draws a fake tracer around the player.
	VIRTUAL void DebugDrawFakeTracer(const Vec3& pos, const Vec3& dir);

	VIRTUAL void GetMemoryStatistics(ICrySizer *pSizer);

	// debug members ============= DO NOT USE
	VIRTUAL void DebugDraw();

	//Debugging////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////



	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Get Subsystems///////////////////////////////////////////////////////////////////////////////////////////////
	VIRTUAL IAIRecorder *GetIAIRecorder();
	VIRTUAL INavigation *GetINavigation();
	VIRTUAL IAIPathFinder *GetIAIPathFinder();
	VIRTUAL ICentralInterestManager* GetCentralInterestManager(void);
	VIRTUAL ICentralInterestManager const* GetCentralInterestManager(void) const;
	VIRTUAL ITacticalPointSystem* GetTacticalPointSystem(void);
	VIRTUAL IBSSProfileManager* GetBSSProfileManager(void);
	VIRTUAL IEmotionalSystem *			GetEmotionalSystem();
	VIRTUAL IEmotionalSystem const *	GetEmotionalSystem() const;
	VIRTUAL ICoordinationManager* GetCoordinationManager() const;
	VIRTUAL ICommunicationManager* GetCommunicationManager() const;
	VIRTUAL ICoverSystem* GetCoverSystem() const;
	VIRTUAL ISelectionTreeManager* GetSelectionTreeManager() const;
	VIRTUAL ITargetTrackManager* GetTargetTrackManager() const;

	VIRTUAL ISmartObjectManager* GetSmartObjectManager();
	VIRTUAL IAIObjectManager* GetAIObjectManager();
	//Get Subsystems///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////



	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//AI Actions///////////////////////////////////////////////////////////////////////////////////////////////////
	VIRTUAL IAIActionManager* GetAIActionManager();
	//AI Actions///////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////



	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Leader/Formations////////////////////////////////////////////////////////////////////////////////////////////
	VIRTUAL void EnumerateFormationNames(unsigned int maxNames, const char** names, unsigned int* nameCount) const;
	VIRTUAL int GetGroupCount(int nGroupID, int flags = GROUP_ALL, int type = 0);
	VIRTUAL IAIObject* GetGroupMember(int groupID, int index, int flags = GROUP_ALL, int type = 0);
	//Leader/Formations////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////



	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Goal Pipes///////////////////////////////////////////////////////////////////////////////////////////////////
	//TODO: get rid of this; => it too many confusing uses to remove just yet
	VIRTUAL int AllocGoalPipeId() const;
	//Goal Pipes///////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////



	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Navigation / Pathfinding/////////////////////////////////////////////////////////////////////////////////////
	VIRTUAL bool CreateNavigationShape(const SNavigationShapeParams &params);
	VIRTUAL void DeleteNavigationShape(const char *szPathName);
	VIRTUAL bool DoesNavigationShapeExists(const char * szName, EnumAreaType areaType, bool road = false);
	VIRTUAL const char*	GetEnclosingGenericShapeOfType(const Vec3& pos, int type, bool checkHeight);
	VIRTUAL bool IsPointInsideGenericShape(const Vec3& pos, const char* shapeName, bool checkHeight);
	VIRTUAL float DistanceToGenericShape(const Vec3& pos, const char* shapeName, bool checkHeight);
	VIRTUAL bool ConstrainInsideGenericShape(Vec3& pos, const char* shapeName, bool checkHeight);
	VIRTUAL const char*	CreateTemporaryGenericShape(Vec3* points, int npts, float height, int type);
	VIRTUAL void EnableGenericShape(const char* shapeName, bool state);

	VIRTUAL const ObstacleData GetObstacle(int nIndex);

	// Pathfinding properties
	VIRTUAL void AssignPFPropertiesToPathType(const string& sPathType, const AgentPathfindingProperties& properties);
	VIRTUAL const AgentPathfindingProperties* GetPFPropertiesOfPathType(const string& sPathType);
	VIRTUAL string GetPathTypeNames();

	/// Checks for each point in the array if it is inside path obstacles of the specified requester. The corresponding result
	/// value will be set to true if the point is inside obstacle. Returns number of points inside the obstacles.
	VIRTUAL unsigned int CheckPointsInsidePathObstacles(IAIObject* requester, const Vec3* points, bool* results, unsigned int n);

	/// Register a spherical region that causes damage (so should be avoided in pathfinding). pID is just
	/// a unique identifying - so if this is called multiple times with the same pID then the damage region
	/// will simply be moved. If radius <= 0 then the region is disabled.
	VIRTUAL void RegisterDamageRegion(const void *pID, const Sphere &sphere);


	//Navigation / Pathfinding/////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////



	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Hide spots///////////////////////////////////////////////////////////////////////////////////////////////////

	// Returns specified number of nearest hidespots. It considers the hidespots in graph and anchors.
	// Any of the pointers to return values can be null. Returns number of hidespots found.
	VIRTUAL unsigned int GetHideSpotsInRange(IAIObject* requester, const Vec3& reqPos,
		const Vec3& hideFrom, float minRange, float maxRange, bool collidableOnly, bool validatedOnly,
		unsigned int maxPts, Vec3* coverPos, Vec3* coverObjPos, Vec3* coverObjDir, float* coverRad, bool* coverCollidable);
	// This method will soon replace the one above. /Jonas
	VIRTUAL unsigned int GetHideSpots(HidespotQueryContext& queryContext) const;
	// Marcio: HAX
	VIRTUAL void SetHideSpotOccupied(const Vec3& pos, bool occupied);
	// Returns a point which is a valid distance away from a wall in front of the point.
	VIRTUAL void AdjustDirectionalCoverPosition(Vec3& pos, const Vec3& dir, float agentRadius, float testHeight);

	//Hide spots///////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////



	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Perception///////////////////////////////////////////////////////////////////////////////////////////////////

	//TODO PerceptionManager://Some stuff in this section maps to that..

	// current global AI alertness value (what's the most alerted puppet)
	VIRTUAL int GetAlertness() const;
	VIRTUAL int GetAlertness(const IAIAlertnessPredicate& alertnessPredicate);
	// Returns the exposure and threat levels as perceived by the specified AIobject.
	// If the AIobject is null, the local player values are returns
	VIRTUAL void GetDetectionLevels(IAIObject *pObject, SAIDetectionLevels& levels);
	VIRTUAL void SetAssesmentMultiplier(unsigned short type, float fMultiplier);
	VIRTUAL void SetSpeciesThreatMultiplier(int nSpeciesID, float fMultiplier);
	VIRTUAL void SetPerceptionDistLookUp( float* pLookUpTable, int tableSize );	//look up table to be used when calculating visual time-out increment
	/// Fills the array with possible dangers, returns number of dangers.
	VIRTUAL unsigned int GetDangerSpots(const IAIObject* requester, float range, Vec3* positions, unsigned int* types, unsigned int n, unsigned int flags);
	VIRTUAL const char* GetCustomOnSeenSignal(int combatClass);
	VIRTUAL void RegisterStimulus(const SAIStimulus& stim);
	VIRTUAL void IgnoreStimulusFrom(EntityId sourceId, EAIStimulusType type, float time);
	VIRTUAL void DynOmniLightEvent(const Vec3& pos, float radius, EAILightEventType type, EntityId shooterId, float time = 5.0f);
	VIRTUAL void DynSpotLightEvent(const Vec3& pos, const Vec3& dir, float radius, float fov, EAILightEventType type, EntityId shooterId, float time = 5.0f);
	VIRTUAL IVisionMap& GetVisionMap() { return m_visionMap; }

	//Perception///////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////



	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//WTF are these?///////////////////////////////////////////////////////////////////////////////////////////////
	VIRTUAL IAIObject* GetBeacon(unsigned short nGroupID);
	VIRTUAL void UpdateBeacon(unsigned short nGroupID, const Vec3 & vPos, IAIObject *pOwner = 0);

	VIRTUAL void RegisterFirecommandHandler(IFireCommandDesc* desc);
	VIRTUAL IFireCommandHandler* CreateFirecommandHandler(const char* name, IAIActor *pShooter);

	VIRTUAL void AddCombatClass(int combatClass, float* pScalesVector, int size, const char* szCustomSignal);
	VIRTUAL float ProcessBalancedDamage(IEntity* pShooterEntity, IEntity* pTargetEntity, float damage, const char* damageType);
	VIRTUAL void NotifyDeath(IAIObject* pVictim);
	VIRTUAL bool ParseTables(int firstTable, bool parseMovementAbility, IFunctionHandler* pH, AIObjectParameters& aiParams, bool& updateAlways);
	//WTF are these?///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////


	//IAISystem/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

























	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//CAISystem/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// CActiveAction needs to access action manager
	CPerceptionManager* GetPerceptionManager();
	CAILightManager* GetLightManager();
	CAIDynHideObjectManager* GetDynHideObjectManager();
	// Get BT Profiles
	CProfileDictionary *       GetBTProfileDictionary();
	CProfileDictionary const * GetBTProfileDictionary() const;
	// Get Cooperative Readabilites System. Not in IAISystem interface.
	CCoopReadabilitiesSystem*					GetCooperativeReadabilitiesSystem();
	CCoopReadabilitiesSystem const *	GetCooperativeReadabilitiesSystem() const;
	
	bool InitSmartObjects();	//TODO find way to remove this (should be done in init, !maybe! reset, nowhere else)








	typedef std::vector< std::pair<string, const SpecialArea*> > VolumeRegions;

	/// Returns true if all the links leading out of the node have radius < fRadius
	bool ExitNodeImpossible(CGraphLinkManager& linkManager, const GraphNode * pNode, float fRadius) const;

	/// Returns true if all the links leading into the node have radius < fRadius
	bool EnterNodeImpossible(CGraphNodeManager& nodeManager, CGraphLinkManager& linkManager, const GraphNode * pNode, float fRadius) const;

	void InvalidatePathsThroughArea(const ListPositions& areaShape);

	/// Returns obstacles within specified radius.
	/// This function return obstacle within euclidean distance, not distance along the navigation graph.
	/// The obstacles are returned as <position, radius> pair. This function should be faster then using
	/// the graph search to query the vertices. Triangular obstacles only. See EGetObstaclesInRadiusFlags.
	void GetObstaclePositionsInRadius(const Vec3& pos, float radius, float minHeight, int flags, std::vector<std::pair<Vec3, float> >& positions);








	typedef VectorSet < CWeakRef<CPuppet> > PuppetSet;
	bool GetNearestPunchableObjectPosition(IAIObject* pRef, const Vec3& searchPos, float searchRad, const Vec3& targetPos, float minSize, float maxSize, float minMass, float maxMass, Vec3& posOut, Vec3& dirOut, IEntity** objEntOut);
	void DumpStateOf(IAIObject * pObject);
	IAIObject *GetBehindObjectInRange(IAIObject * pRef, unsigned short nType, float fRadiusMin, float fRadiusMax);
	IAIObject *GetRandomObjectInRange(IAIObject * pRef, unsigned short nType, float fRadiusMin, float fRadiusMax, bool bFaceAttTarget=false);
	IAIObject *GetNearestObjectOfTypeInRange(const Vec3 &pos, unsigned int type, float fRadiusMin, float fRadiusMax, IAIObject* pSkip=NULL, int nOption = 0);
	IAIObject *GetNearestObjectOfTypeInRange(IAIObject *pObject , unsigned int type, float fRadiusMin, float fRadiusMax, int nOption = 0);

	//// Iterates over AI objects within specified shape.
	//// Parameter 'name' specify the enclosing shape and parameter 'checkHeight' specifies if hte height of the shape is taken into account too,
	//// for other parameters see GetFirstAIObject.
	IAIObjectIter* GetFirstAIObjectInShape(EGetFirstFilter filter, short n, const char* shapeName, bool checkHeight);
	IAIObject *GetNearestToObjectInRange( IAIObject *pRef , unsigned short nType, float fRadiusMin, float fRadiusMax, float inCone=-1, bool bFaceAttTarget=false, bool bSeesAttTarget=false, bool bDevalue=true);

	void CreateDummyObject(CStrongRef <CAIObject>& ref, const string &name="", CAIObject::ESubTypes type = CAIObject::STP_NONE);
	void CreateDummyObject(CCountedRef<CAIObject>& ref, const string &name="", CAIObject::ESubTypes type = CAIObject::STP_NONE);

	//// Devalues an AI object for the refence object only or for whole group.
	void Devalue( IAIObject* pRef, IAIObject* pObject, bool group, float fDevalueTime=20.f );
	CAIObject * GetAIObjectByName(const char *pName) const;
	CAIObject * GetPlayer() const;
	void RemoveObject(CAIObject *pObject);

	//// it removes all references to this object from all objects of the specified type
	//// (MATT) Note that is does not imply deletion - vehicles use it when they lose their driver {2009/02/05}
	void RemoveObjectFromAllOfType(int nType, CAIObject* pRemovedObject);

	void NotifyEnableState(CPuppet* pPuppet, bool state);
	EPuppetUpdatePriority CalcPuppetUpdatePriority(CPuppet* pPuppet) const;

	//// non-virtual, accessed from CAIObject.
	//// notifies that AIObject has changed its position, which is important for smart objects
	void NotifyAIObjectMoved( IEntity* pEntity, SEntityEvent event );
	void NotifyTargetDead(CAIObject* pDeadObject);

	const PuppetSet& GetEnabledPuppetSet() const;
	//// adds an object to a species
	void AddToSpecies(CAIObject * pObject, unsigned short nSpecies);



	IAIObject* GetLeaderAIObject(int iGroupId);
	IAIObject* GetLeaderAIObject(IAIObject* pObject);
	IAIGroup* GetIAIGroup(int groupid);
	void SetLeader(IAIObject* pObject);
	CLeader* GetLeader(int nGroupID);
	CLeader* GetLeader(const CAIActor* pSoldier);
	CLeader* CreateLeader(int nGroupID);
	CAIGroup*	GetAIGroup(int nGroupID);
	// removes specified object from group
	void RemoveFromGroup(int nGroupID, CAIObject * pObject);
	// adds an object to a group
	void AddToGroup(CAIActor * pObject,int nGroupId = -1);
	int GetBeaconGroupId(CAIObject* pBeacon);
	void UpdateGroupStatus(int groupId);








	bool ScaleFormation(IAIObject* pOwner, float fScale);
	bool SetFormationUpdate(IAIObject* pOwner, bool bUpdate);
	void AddFormationPoint(const char *name, const FormationNode& nodeDescriptor);
	IAIObject* GetFormationPoint(IAIObject* pObject);
	int	 GetFormationPointClass(const char* descriptorName, int position);
	bool ChangeFormation(IAIObject* pOwner, const char * szFormationName,float fScale);
	void CreateFormationDescriptor(const char *name);
	void FreeFormationPoint(CWeakRef<CAIObject> refOwner);
	bool IsFormationDescriptorExistent(const char *name);
	CFormation *CreateFormation(CWeakRef<CAIObject> refOwner, const char * szFormationName, Vec3 vTargetPos = ZERO);
	void ReleaseFormation(CWeakRef<CAIObject> refOwner, bool bDelete);
	void ReleaseFormationPoint(CAIObject * pReserved);
	// changes the formation's scale factor for the given group id
	bool SameFormation(const CPuppet* pHuman, const CAIVehicle* pVehicle);
	// Calculates a static row formation for group of puppets.
	bool CreateStaticFormation(const std::vector<CPuppet*>& groupies, const Vec3& targetPos, std::vector<Vec3>& points, std::vector<int>& order, float offsetTowardsTarget, float searchRange, float spacing);
	struct SAIStaticFormation
	{
		SAIStaticFormation()
			:debugOcclusionObjectCount(0)
			,error(ERR_NONE)
			,debugPointsOutsideTerritory(0)
			,debugPointsInsideObstacle(0)
			,debugPointsInsideForbiddenArea(0)
			,debugPointsInsidePathObstacles(0)
		{
		}

		enum EType
		{
			AISF_ROW,
			AISF_LINE,
		};
		enum ELocation
		{
			AISF_NEAREST,
			AISF_NEAREST_LEFT,
			AISF_NEAREST_RIGHT,
			AISF_BEST,
		};
		enum EError
		{
			ERR_NONE,
			ERR_GROUP_EMPTY,
			ERR_NO_HIDESPOTS,
			ERR_FORMATION_POINT_NOT_FOUND,
			ERR_ISLANDS_NOT_FOUND,
		};

		std::vector<Vec3> formationPoints;
		std::vector<Vec3> desiredFormationPoints;
		std::vector<int> order;

		struct SDebugPoint
		{
			inline void Set(const Vec3& pos_, float w_) { pos = pos_; w = w_; }
			Vec3 pos;
			float w;
		};
		struct SDebugIsland
		{
			SDebugIsland(float smin, float smax, float mid, float score) : smin(smin), smax(smax), mid(mid), score(score) {}
			float smin, smax;
			float mid;
			float score;
		};
		std::vector<SDebugPoint> debugPoints;
		std::vector<SDebugIsland> debugIslands;
		Vec3 debugForw, debugRight;
		Vec3 debugAdvanceSpot;
		Vec3 debugSearchPos;
		std::vector<float> debugHistogram;
		CAIRadialOcclusion debugOcclusion;
		int debugOcclusionObjectCount;
		int debugPointsOutsideTerritory;
		int debugPointsInsideObstacle;
		int debugPointsInsideForbiddenArea;
		int debugPointsInsidePathObstacles;
		EError error;
	};
	bool CreateStaticFormation(SAIStaticFormation::EType type, SAIStaticFormation::ELocation location,
		const std::vector<CPuppet*>& group, const Vec3& targetPos, const Vec3* pPivotPos,
		float offsetTowardsTarget, float minDistanceToTarget, float searchRange, float spacing,
		std::vector<std::pair<Vec3, float> >& otherGroups, CAIRadialOcclusionRaycast* occlusion,
		SAIStaticFormation& outFormation);








	// reads special areas from file. clears the existing special areas
	void ReadAreasFromFile(const char * fileNameAreas);
	void FlushAllAreas();






	SShape*	GetGenericShapeOfName(const char* shapeName);
	const ShapeMap &GetGenericShapes() const;









	/// Indicates if a human would be visible if placed at the specified position
	/// If fullCheck is false then only a camera frustum check will be done. If true
	/// then more precise (perhaps expensive) checking will be done.
	bool WouldHumanBeVisible(const Vec3 &footPos, bool fullCheck) const;

	const ShapeMap& GetOcclusionPlanes() const;

	bool CheckPointsVisibility(const Vec3& from, const Vec3& to, float rayLength, IPhysicalEntity* pSkipEnt=0, IPhysicalEntity* pSkipEntAux=0);
	bool CheckObjectsVisibility(const IAIObject *pObj1, const IAIObject *pObj2, float rayLength);
	float	GetVisPerceptionDistScale( float distRatio );	//distRatio (0-1) 1-at sight range
	float GetRayPerceptionModifier(const Vec3 &start, const Vec3 &end, const char *actorName);

	// returns value in range [0, 1]
	// calculate how much targetPos is obstructed by water
	// takes into account water depth and density
	float	GetWaterOcclusionValue(const Vec3& targetPos) const;

	bool CheckVisibilityToBody(CPuppet* pObserver, CAIActor* pBody, float& closestDistSq, IPhysicalEntity* pSkipEnt = 0);











	/// Returns positions of currently occupied hide point objects excluding the requesters hide spot.
	void	GetOccupiedHideObjectPositions(const CPipeUser* pRequester, std::vector<Vec3>& hideObjectPositions);

	/// Finds all hidespots (and their path range) within path range of startPos, along with the navigation graph nodes traversed.
	/// Each hidespot contains info about where it came from. If you want smart-object hidespots you need to pass in an entity
	/// so it can be checked to see if it could use the smart object. pLastNavNode/pLastHideNode are just used as a hint.
	/// Smart Objects are only considered if pRequester != 0
	MultimapRangeHideSpots &GetHideSpotsInRange(MultimapRangeHideSpots &result, MapConstNodesDistance &traversedNodes, const Vec3 & startPos, float maxDist, 
		IAISystem::tNavCapMask navCapMask, float passRadius, bool skipNavigationTest,
		IEntity* pSmartObjectUserEntity = 0, unsigned lastNavNodeIndex = 0, const class CAIObject *pRequester = 0);

	bool IsHideSpotOccupied(CPipeUser* pRequester, const Vec3& pos) const;

	void	AdjustOmniDirectionalCoverPosition(Vec3& pos, Vec3& dir, float hideRadius, float agentRadius, const Vec3& hideFrom, const bool hideBehind=true);



















	/// Adds object to a list of dead bodies to avoid.
	void RegisterDeadBody(unsigned int species, const Vec3& pos);

	//combat classes scale
	float	GetCombatClassScale(int shooterClass, int targetClass);

	typedef std::map<const void *, Sphere> TDamageRegions;
	/// Returns the regions game code has flagged as causing damage
	const TDamageRegions &GetDamageRegions() const;













	static void ReloadConsoleCommand( IConsoleCmdArgs * );
	static void StartCoordinationConsoleCommand( IConsoleCmdArgs * );
	static void CheckGoalpipes(IConsoleCmdArgs *);
	static void DumpCodeCoverageCheckpoints(IConsoleCmdArgs *pArgs);
	static void StartAIRecorder( IConsoleCmdArgs * );
	static void StopAIRecorder( IConsoleCmdArgs * );




















	// Clear out AI system for clean script reload
	void ClearForReload(void);






	void SetupAIEnvironment();
	void SetAIworkaroundsConfiguration();






	/// Walks through over object in the system that needs to be serialised, or is referenced
	/// by serialisation, and adds it to the object tracker (to generate/store an ID for it)
	void PopulateObjectTracker(class CObjectTracker& objectTracker);
	/// add a single AI object to tracker
	void PopulateObjectTracker(class CObjectTracker& objectTracker, CAIObject* pObject);

	/// Uses the object tracker to create empty objects that are ready to be filled in
	/// through subsequent load-serialisation
	void InstantiateFromObjectTracker(class CObjectTracker& objectTracker);

	/// Our own internal serialisation - just serialise our state (but not the things
	/// we own that are capable of serialising themselves)
	void Serialize( TSerialize ser, CObjectTracker& objectTracker );

	void SerializeDynamicGoalPipes( TSerialize ser );






	void SingleDryUpdate(CPuppet * pObject);
	bool SingleFullUpdate(CPuppet * pPuppet, int64 frameStartTime);
	bool SingleFullUpdate2(CPuppet * pPuppet, std::vector<CAIObject*>& priorityTargets);

	void UpdateAmbientFire();
	void UpdateExpensiveAccessoryQuota();
	void UpdateAuxSignalsMap();








	void TakeDetectionSnapshot();





	void CheckVisibilityBodies();
	int RayOcclusionPlaneIntersection(const Vec3& start, const Vec3& end);
	int RayObstructionSphereIntersection(const Vec3& start, const Vec3& end);



	// just steps through objects - for debugging
	void DebugOutputObjects(const char *txt) const;





	const bool IsEnabled() const;//TODO fix initialization so this is not needed!




























	///////////////////////////////////////////////////
	CPipeManager m_PipeManager;
	CGraph* m_pGraph;
	CPathfinder* m_pPathfinder;
	CNavigation* m_pNavigation;
	CAIActionManager* m_pAIActionManager;
	CSmartObjectManager* m_pSmartObjectManager;
	bool m_bUpdateSmartObjects;
	bool m_IsEnabled;//TODO eventually find how to axe this!
	///////////////////////////////////////////////////







	///////////////////////////////////////////////////////////////////////////////////
	typedef std::multimap<short, CCountedRef<CAIObject> > AIObjectOwners;
	AIObjectOwners m_Objects;// m_RootObjects or EntityObjects might be better names

	std::vector<short>	m_priorityObjectTypes;

	typedef std::multimap<short, CWeakRef<CAIObject> > AIObjects;
	AIObjects m_mapGroups;
	AIObjects m_mapSpecies;
	AIObjects m_mapDummyObjects;

	// This map stores the AI group info.
	typedef std::map<int, CAIGroup*> AIGroupMap;
	AIGroupMap	m_mapAIGroups;

	//AIObject Related Data structs:
	unsigned int m_nNumPuppets;
	PuppetSet m_enabledPuppetsSet;  // Set of enabled puppets.
	PuppetSet m_disabledPuppetsSet; // Set of disabled puppets.
	float m_enabledPuppetsUpdateError;
	int		m_enabledPuppetsUpdateHead;
	int		m_totalPuppetsUpdateCount;
	float m_disabledPuppetsUpdateError;
	int		m_disabledPuppetsHead;


	typedef std::map<tAIObjectID, CAIHideObject> DebugHideObjectMap;
	DebugHideObjectMap m_DebugHideObjects;


	struct BeaconStruct
	{
		CCountedRef<CAIObject> refBeacon;
		CWeakRef<CAIObject> refOwner;
	};
	typedef std::map<unsigned short, BeaconStruct> BeaconMap;
	BeaconMap			m_mapBeacons;


	typedef std::map<CWeakRef<CAIObject>, CFormation*> FormationMap;  // (MATT) Could be a pipeuser or such? {2009/03/18}
	FormationMap	m_mapActiveFormations;
	typedef std::map<string, CFormationDescriptor> FormationDescriptorMap;
	FormationDescriptorMap m_mapFormationDescriptors;

	//AIObject Related Data structs:
	///////////////////////////////////////////////////////////////////////////////////




	////////////////////////////////////////////////////////////////////
	//Subsystems
	CAILightManager m_lightManager;
	CAIDynHideObjectManager m_dynHideObjectManager;
	CEmotionalSystem* m_pEmotionalSystem;
	CProfileDictionary* m_pBTProfileDictionary;
	CCoopReadabilitiesSystem* m_pCoopReadabilitiesSystem;
	SAIRecorderDebugContext m_recorderDebugContext;
	NewBehaviourTree::TreeProfileManager *m_BehaviourTreeProfileManager;
	//Subsystems
	////////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////////
	//pathfinding data structures
	CVertexList m_VertexList;
	typedef std::map<string, AgentPathfindingProperties> PFPropertiesMap;
	PFPropertiesMap mapPFProperties;
	ShapeMap m_mapOcclusionPlanes;
	ShapeMap m_mapGenericShapes;
	// structures to cache forbidden area query results
	struct SIPOFEQuery
	{
		SIPOFEQuery(const Vec3 &pos, const float tol, bool checkAutoRegions)
			: pos(pos), tol(tol), checkAutoRegions(checkAutoRegions), normal(ZERO), pPolygon(0) {}
		SIPOFEQuery() {}
		bool operator==(const SIPOFEQuery &other) {return pos == other.pos && tol == other.tol && checkAutoRegions == other.checkAutoRegions;}

		Vec3 pos;
		float tol;
		bool checkAutoRegions;

		bool result;
		Vec3 normal;
		const ListPositions* pPolygon;
	};

	struct SIPIFRQuery
	{
		SIPIFRQuery(const Vec3 &pos, bool checkAutoRegions)
			: pos(pos), checkAutoRegions(checkAutoRegions), pPolygon(0) {}
		SIPIFRQuery() {}
		bool operator==(const SIPIFRQuery &other) {return pos == other.pos && checkAutoRegions == other.checkAutoRegions;}

		Vec3		pos;
		bool checkAutoRegions;

		bool result;
		const ListPositions* pPolygon;
	};

	enum EQueueSizes {QS_IPOFE = 8, QS_IPIFR = 8};
	typedef MiniQueue<SIPOFEQuery, QS_IPOFE> tIPOFEQueue;
	typedef MiniQueue<SIPIFRQuery, QS_IPIFR> tIPIFRQueue;
	mutable tIPOFEQueue m_isPointOnForbiddenEdgeQueue;
	mutable tIPIFRQueue m_isPointInForbiddenRegionQueue;
	//pathfinding data structures
	////////////////////////////////////////////////////////////////////



	////////////////////////////////////////////////////////////////////
	//scripting data structures
	CScriptBind_AI *m_pScriptAI;
	CScriptBind_AI2* m_pScriptBind;

	//WTF? Why are these not registered from outside?
	CScriptBind_AI_G02 *m_pScriptAI_G02;
	CScriptBind_AI_G04 *m_pScriptAI_G04;
	//scripting data structures
	////////////////////////////////////////////////////////////////////




	////////////////////////////////////////////////////////////////////
	//system listeners
	typedef VectorSet<IAISystemListener *> SystemListenerSet;
	SystemListenerSet m_setSystemListeners;
	//system listeners
	////////////////////////////////////////////////////////////////////













	//ISystem *gEnv->pSystem;
	//IPhysicalWorld *m_pWorld;


	// Marcio: HAX
	typedef std::vector<Vec3> OccupiedHideSpotPositions;
	OccupiedHideSpotPositions m_occupiedHideSpotPositions;


	typedef std::map<int, float> MapMultipliers;
	MapMultipliers	m_mapMultipliers;
	MapMultipliers	m_mapSpeciesThreatMultipliers;


	//<<FIXME>> just used for profiling:
	//typedef std::map<tAIObjectID,float> TimingMap;
	//TimingMap m_mapDEBUGTiming;

	//typedef std::map<string,float> NamedTimingMap;
	//NamedTimingMap m_mapDEBUGTimingGOALS;

	int	m_AlertnessCounters[NUM_ALERTNESS_COUNTERS];

	// (MATT) For now I bloat CAISystem with this, but it should move into some new struct of Environment.
	// Stores level path for metadata - i.e. the code coverage data files {2009/02/17}
	string m_sWorkingFolder;


	int		m_CurrentGlobalAlertness;



	float m_DEBUG_screenFlash;





	bool m_bCodeCoverageFailed;


	unsigned int m_nTickCount;
	bool m_bInitialized;





	IVisArea *m_pAreaList[100];



	float m_fFrameDeltaTime;
	CTimeValue m_fLastPuppetUpdateTime;
	CTimeValue m_fFrameStartTime;
	CTimeValue m_lastVisBroadPhaseTime;
	CTimeValue m_lastAmbientFireUpdateTime;
	CTimeValue m_lastExpensiveAccessoryUpdateTime;
	CTimeValue m_lastGroupUpdateTime;

	PerceptionModifierShapeMap m_mapPerceptionModifiers;





	std::vector<IFireCommandDesc*> m_firecommandDescriptors;

	TDamageRegions m_damageRegions;







	struct SAIDeadBody
	{
		inline SAIDeadBody() {}
		inline SAIDeadBody(unsigned int s, const Vec3& p) { Set(s, p); }
		inline void Set(unsigned int s, const Vec3& p)
		{
			species = s;
			pos = p;
			t = 20.0f;
		}
		inline void Serialize(TSerialize ser)
		{
			ser.BeginGroup("AIDeadBody");
			ser.Value("pos", pos);
			ser.Value("species", species);
			ser.Value("t", t);
			ser.EndGroup();
		}
		unsigned int species;
		Vec3 pos;
		float t;
	};

	std::vector<SAIDeadBody> m_deadBodies;



	struct SAIDelayedExpAccessoryUpdate
	{
		SAIDelayedExpAccessoryUpdate(CPuppet* pPuppet, float time, bool state)
			:pPuppet(pPuppet)
			,time(time)
			,state(state) {}
		CPuppet* pPuppet;
		float time;
		bool state;
	};
	std::vector<SAIDelayedExpAccessoryUpdate>	m_delayedExpAccessoryUpdates;


	struct AuxSignalDesc
	{
		float fTimeout;
		string strMessage;
		void Serialize( TSerialize ser, CObjectTracker& objectTracker )
		{
			ser.Value("AuxSignalDescTimeOut",fTimeout);
			ser.Value("AuxSignalDescMessage",strMessage);
		}
	};
	typedef std::multimap<short, AuxSignalDesc> MapSignalStrings;
	MapSignalStrings	m_mapAuxSignalsFired;

	// combat classes
	// vector of target selection scale multipliers
	struct SCombatClassDesc
	{
		std::vector<float> mods;
		string customSignal;
	};
	std::vector<SCombatClassDesc>	m_CombatClasses;














	class AILinearLUT
	{
		int	m_size;
		float* m_pData;

	public:
		AILinearLUT() : m_size(0), m_pData(0) {}
		~AILinearLUT()
		{
			if (m_pData)
				delete [] m_pData;
		}

		// Returns the size of the table.
		int	GetSize() const { return m_size; }
		// Returns the value is specified sample.
		float	GetSampleValue(int i) const { return m_pData[i]; }

		/// Set the lookup table from a array of floats.
		void Set(float* values, int n)
		{
			delete [] m_pData;
			m_size = n;
			m_pData = new float[n];
			for(int i = 0; i < n; ++i) m_pData[i] = values[i];
		}

		// Returns linearly interpolated value, t in range [0..1]
		float	GetValue(float t) const
		{
			const int	last = m_size - 1;

			// Convert 't' to a sample.
			t *= (float)last;
			const int	n = (int)floorf(t);

			// Check for out of range cases.
			if(n < 0) return m_pData[0];
			if(n >= last) return m_pData[last];

			// Linear interpolation between the two adjacent samples.
			const float	a = t - (float)n;
			return m_pData[n] + (m_pData[n+1] - m_pData[n]) * a;
		}
	};

	AILinearLUT	m_VisDistLookUp;






	////////////////////////////////////////////////////////////////////
	//Debugging / Logging subsystems

#ifdef CRYAISYSTEM_DEBUG
	CAIDbgRecorder m_DbgRecorder;
	CAIRecorder	m_Recorder;

	struct SPerceptionDebugLine
	{
		SPerceptionDebugLine(const char *name_, const Vec3& start_, const Vec3& end_, const ColorB& color_, float time_, float thickness_ )
			:start(start_)
			,end(end_)
			,color(color_)
			,time(time_)
			,thickness(thickness_) 
		{
			if (name_)
				_snprintf(name, sizeof(name), "%s", name_);
			else
				*name = '\0';
		}

		Vec3		start, end;
		ColorB	color;
		float		time;
		float		thickness;
		char		name[64];
	};
	std::list<SPerceptionDebugLine>	m_lstDebugPerceptionLines;


	enum EDebugPlayerDamageType
	{
		DBG_PLAYER_SHOT_AT,
		DBG_PLAYER_HIT,
	};

	struct SDebugPlayerDamage
	{
		SDebugPlayerDamage(EDebugPlayerDamageType type, const char* shooterName, float damage, const char* materialName, float t)
			:type(type)
			,shooterName(shooterName)
			,damage(damage)
			,materialName(materialName)
			,t(t){}
		EDebugPlayerDamageType	type;
		string	shooterName;
		float		damage;
		string	materialName;
		float	t;
	};
	typedef std::list<SDebugPlayerDamage>	DebugPlayerDamageList;
	DebugPlayerDamageList	m_DEBUG_playerShots;


	struct SDebugFakeTracer
	{
		SDebugFakeTracer(const Vec3& p0, const Vec3& p1, float a, float t) : p0(p0), p1(p1), a(a), t(t), tmax(t) {}
		Vec3	p0, p1;
		float a;
		float t, tmax;
	};
	std::vector<SDebugFakeTracer>	m_DEBUG_fakeTracers;

	struct SDebugFakeDamageInd
	{
		SDebugFakeDamageInd() {}
		SDebugFakeDamageInd(const Vec3& pos, float t) : p(pos), t(t), tmax(t) {}
		std::vector<Vec3> verts;
		Vec3	p;
		float	t, tmax;
	};
	std::vector<SDebugFakeDamageInd>	m_DEBUG_fakeDamageInd;

	struct SDebugFakeHitEffect
	{
		SDebugFakeHitEffect() {}
		SDebugFakeHitEffect(const Vec3& p, const Vec3& n, float r, float t, ColorB c) : p(p), n(n), r(r), t(t), tmax(t), c(c) {}
		Vec3	p, n;
		float	r, t, tmax;
		ColorB c;
	};
	std::vector<SDebugFakeHitEffect>	m_DEBUG_fakeHitEffect;
	
	void UpdateDebugStuff();

	void DebugDrawEnabledPuppets();
	void DebugDrawEnabledPlayers() const;
	void DebugDrawUpdate() const;
	void DebugDrawFakeTracers() const;
	void DebugDrawFakeHitEffects() const;
	void DebugDrawFakeDamageInd() const;
	void DebugDrawPlayerRanges() const;
	void DebugDrawPerceptionIndicators();
	void DebugDrawPerceptionModifiers();
	void DebugDrawTargetTracks() const;
	void DebugDrawCodeCoverage() const;
	void DebugDrawPerceptionManager();
	void DebugDrawNavigation() const;
	void DebugDrawGraph(int debugDrawVal) const;
	void DebugDrawDeadBodies() const;
	void DebugDrawNavModifiers() const;
	void DebugDrawLightManager();
	void DebugDrawP0AndP1() const;
	void DebugDrawPolygonSetOps() const;
	void DebugDrawPuppetPaths();
	void DebugDrawBestPositions() const;
	void DebugDrawCheckCapsules() const;
	void DebugDrawCheckRay() const;
	void DebugDrawCheckGetEnclosing() const;
	void DebugDrawCheckWalkability();
	void DebugDrawCheckWalkabilityTime() const;
	void DebugDrawCheckFloorPos() const;
	void DebugDrawCheckGravity() const;
	void DebugDrawGetTeleportPos() const;
	void DebugDrawTestNode() const;
	void DebugDrawDebugShapes();
	void DebugDrawGroupTactic();	
	void DebugDrawDamageParts() const;
	void DebugDrawStanceSize() const;
	void DebugDrawForceAGAction() const;
	void DebugDrawForceAGSignal() const;
	void DebugDrawForceStance() const;
	void DebugDrawForcePosture() const;
	void DebugDrawPlayerActions() const;
	void DebugDrawPath();
	void DebugDrawPathAdjustments() const;
	void DebugDrawPathSingle(const CPuppet *pPuppet) const;
	void DebugDrawAgents() const;
	void DebugDrawAgent(CAIObject* pAgent) const;
	void DebugDrawStatsTarget(const char *pName);
	void DebugDrawBehaviorStatsTarget(const char *pName);
	void DebugDrawBehaviorSelection(const char *agentName);
	void DebugDrawGroupSystem(int nMode);
	void DebugDrawFormations() const;
	void DebugDrawCooperativeReadabilities(const char *pName) const;
	void DebugDrawTaggedNodes() const;
	void DebugDrawNode() const;
	void DebugDrawNodeSingle(CAIObject* pTargetObject) const;
	void DebugDrawGraph(CGraph* pGraph, const std::vector<Vec3> * focusPositions = 0, float radius = 0.0f) const;
	void DebugDrawGraphErrors(CGraph* pGraph) const;
	void DebugDrawType() const;
	void DebugDrawTypeSingle(CAIObject* pAIObj) const;
	void DebugDrawPendingEvents(CPuppet* pPuppet, int xPos, int yPos) const;
	void DebugDrawTargetsList() const;
	void DebugDrawTargetUnit(CAIObject* pAIObj) const;
	void DebugDrawStatsList() const;
	void DebugDrawLocate() const;
	void DebugDrawLocateUnit(CAIObject* pAIObj) const;
	void DebugDrawBadAnchors();
	void DebugDrawSteepSlopes();
	void DebugDrawVegetationCollision();
	void DebugDrawGroups(short int groupId);
	void DebugDrawOneGroup(short grpID);
	void DebugDrawHideSpots();
	void DebugDrawDynamicHideObjects();
	void DebugDrawHashSpace();
	void DebugDrawMyHideSpot(CAIObject* pAIObj) const;
	void DebugDrawSelectedHideSpots() const;
	void DebugDrawCrowdControl();
	void DebugDrawRadar();
	void DebugDrawDistanceLUT();
	void DrawRadarPath(CPuppet* puppet, const Matrix34& world, const Matrix34& screen);
	void DebugDrawRecorderRange() const;
	void DebugDrawShooting() const;
	void DebugDrawAreas() const;
	void DebugDrawAmbientFire() const;
	void DebugDrawInterestSystem(int iLevel) const;
	void DebugDrawExpensiveAccessoryQuota() const;
	void DebugDrawDamageControlGraph() const;
	void DebugDrawAdaptiveUrgency() const;
	enum EDrawUpdateMode
	{
		DRAWUPDATE_NONE = 0,
		DRAWUPDATE_NORMAL,
		DRAWUPDATE_WARNINGS_ONLY,
	};
	bool DebugDrawUpdateUnit(CPuppet* pTargetPuppet, int row, EDrawUpdateMode mode) const;
	void DebugDrawTacticalPoints();

	void DEBUG_AddFakeDamageIndicator(CPuppet* pShooter, float t);

	struct SDebugLine
	{
		SDebugLine( const Vec3& start_, const Vec3& end_, const ColorB& color_, float time_ )
			:start(start_)
			,end(end_)
			,color(color_)
			,time(time_)
		{}
		Vec3		start, end;
		ColorB	color;
		float		time;
	};
	std::vector<SDebugLine>	m_vecDebugLines;
	Vec3 m_lastStatsTargetTrajectoryPoint;
	std::list<SDebugLine> m_lstStatsTargetTrajectory;

	struct SDebugBox
	{
		SDebugBox( const Vec3& pos_, const OBB& obb_, const ColorB& color_, float time_ )
			:pos(pos_)
			,obb(obb_)
			,color(color_)
			,time(time_)
		{}
		Vec3		pos;
		OBB			obb;
		ColorB	color;
		float		time;
	};
	std::vector<SDebugBox>	m_vecDebugBoxes;

	struct SDebugSphere
	{
		SDebugSphere (const Vec3 &pos_, float radius_, const ColorB &color_, float time_)
			:pos(pos_)
			,radius(radius_)
			,color(color_)
			,time(time_)
		{ }
		Vec3		pos;
		float		radius;
		ColorB	color;
		float		time;
	};
	std::vector<SDebugSphere> m_vecDebugSpheres;

	void DrawDebugShape (const SDebugLine & );
	void DrawDebugShape (const SDebugBox & );
	void DrawDebugShape (const SDebugSphere & );
	template <typename ShapeContainer>
	void DrawDebugShapes (ShapeContainer & shapes, float dt);

	void AddDebugBox(const Vec3& pos, const OBB& obb, uint8 r, uint8 g, uint8 b, float time);
	void AddPerceptionDebugLine( const char *tag, const Vec3& start, const Vec3& end, uint8 r, uint8 g, uint8 b, float time, float thickness );

#endif //CRYAISYSTEM_DEBUG

	//Debugging / Logging subsystems
	////////////////////////////////////////////////////////////////////


	//CAISystem/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

private:
	CVisionMap m_visionMap;
};




#endif // _CAISYSTEM_H_
