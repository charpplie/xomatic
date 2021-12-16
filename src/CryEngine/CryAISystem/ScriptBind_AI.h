/******************************************************************** 
StalTech Source File.
Copyright (C), StalTech Studios, 2001-2008.
-------------------------------------------------------------------------
File name:   ScriptBind_AI.h
Version:     v1.00
Description: Core scriptbinds for the AI system

-------------------------------------------------------------------------
History:
- 3:11:2004   - Created by Kirill Bulatsev
-13:02:2008   - Moved into AI system by Matthew

*********************************************************************/


#ifndef __ScriptBind_AI_H__
#define __ScriptBind_AI_H__


#pragma once


#include <IScriptSystem.h>
#include <ScriptHelpers.h>

// These numerical values are deprecated; use the strings instead
enum EAIPathType
{
	AIPATH_DEFAULT,
	AIPATH_HUMAN,
	AIPATH_HUMAN_COVER,
	AIPATH_CAR,
	AIPATH_TANK,
	AIPATH_BOAT,
	AIPATH_HELI,
	AIPATH_3D,
	AIPATH_SCOUT,
	AIPATH_TROOPER,
	AIPATH_HUNTER,
};

struct IAISystem;
struct ISystem;
struct AgentMovementAbility;
class CScriptBind_AI :
	public CScriptableBase
{
public:

	CScriptBind_AI();
	virtual ~CScriptBind_AI();

	enum EEncloseDistanceCheckType
	{
		CHECKTYPE_MIN_DISTANCE,
		CHECKTYPE_MIN_ROOMSIZE
	};

	void Release() { delete this; };

	void	RunStartupScript();
	void	InitConVars();

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Log functions
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// <title LogProgress>
	// Syntax: AI.LogProgress(string szMessage )
	// Description: Used to indicate "progress markers" - e.g. during loading 
	// Arguments:
	// szMessage - message line to be displayed in log
	int LogProgress(IFunctionHandler *pH);

	// <title LogEvent>
	// Syntax: AI.LogEvent(string szMessage )
	// Description: Used to indicate event-driven info that would be useful for debugging 
	//	(may occur on a per-frame or even per-AI-update basis)
	// Arguments:
	// szMessage - message line to be displayed in log
	int LogEvent(IFunctionHandler *pH);

	// <title LogComment>
	// Syntax: AI.LogComment(string szMessage )
	// Description: Used to indicate info that would be useful for debugging, 
	//	but there's too much of it for it to be enabled all the time 
	// Arguments:
	// szMessage - message line to be displayed in log
	int LogComment(IFunctionHandler *pH);

	// <title Warning>
	// Syntax: AI.Warning(string szMessage )
	// Description: Used for warnings about data/script errors 
	// Arguments:
	// szMessage - message line to be displayed in log
	int Warning(IFunctionHandler *pH);

	// <title Error>
	// Syntax: AI.Error(string szMessage )
	// Description: Used when we really can't handle some data/situation. 
	//	The code following this should struggle on so that the original cause (e.g. data) 
	//	of the problem can be fixed in the editor, but in game this would halt execution 
	// Arguments:
	// szMessage - message line to be displayed in log
	int Error(IFunctionHandler *pH);

	// <title RecComment>
	// Syntax: AI.RecComment(string szMessage )
	// Description: Record comment with AIRecorder.
	// Arguments:
	// szMessage - message line to be displayed in recorder view
	int RecComment(IFunctionHandler *pH);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	// General AI System functions
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// <title ResetParameters>
	// Syntax: AI.ResetParameters(entityId, PropertiesTable, PropertiesInstanceTable)
	// Description: Resets all the AI parameters
	// Arguments:
	// entityId - entity id
	//	PropertiesTable - Lua table containing the entity properties like groupid, sightrange, sound range etc (editable in editor, usually defined as "Properties")
	//	PropertiesInstanceTable - another properties table, same as PropertiesTable (editable in editor, usually defined as "PropertiesInstance")
	int ResetParameters(IFunctionHandler *pH); 

	// <title ChangeParameter>
	// Syntax: AI.ChangeParameter( entityId, paramEnum, paramValue )
	// Description: Changes an enumerated AI parameter.
	// Arguments:
	// entityId - entity id
	// paramEnum - the parameter to change, should be one of the following:
	//		AIPARAM_SIGHTRANGE - sight range in (meters).
	//		AIPARAM_ATTACKRANGE	- attack range in (meters).
	//		AIPARAM_ACCURACY - firing accuracy (real [0..1]).
	//		AIPARAM_AGGRESION - aggression (real [0..1]).
	//		AIPARAM_GROUPID	- group id (integer).
	//		AIPARAM_FOV_PRIMARY - primary field of vision (degrees).
	//		AIPARAM_FOV_SECONDARY - pheripheral field of vision (degrees).
	//		AIPARAM_COMMRANGE - communication range (meters).
	//		AIPARAM_FWDSPEED - forward speed (vehicles only).
	//		AIPARAM_RESPONSIVENESS - responsiveness (real).
	//		AIPARAM_SPECIES - entity species (integer).
	//		AIPARAM_RANK - unit rank (integer).
	//		AIPARAM_TRACKPATTERN - track pattern name (string).
	//		AIPARAM_TRACKPATTERN_ADVANCE - track pattern advancing (0 = stop, 1 = advance).
	//	paramValue - new parameter value, see above for type and meaning.
	int ChangeParameter(IFunctionHandler * pH);

	// <title IsEnabled>
	// Syntax: AI.IsEnabled( entityId )
	// Description: Returns true if entity's AI is enabled
	// Arguments:
	// entityId - entity id
	int IsEnabled(IFunctionHandler * pH);

	// <title GetTypeOf>
	// Syntax: AI.GetTypeOf(entityId)
	// Description: returns the given entity's type
	// Arguments:
	// entityId - AI's entity id
	// Returns:
	// type of given entity (as defined in IAgent.h)
	int GetTypeOf(IFunctionHandler * pH);

	// <title GetSubTypeOf>
	// Syntax: AI.GetSubTypeOf(entityId)
	// Description: returns the given entity's sub type
	// Arguments:
	// entityId - AI's entity id
	// Returns:
	// sub type of given entity (as defined in IAgent.h)
	// TODO : only returns subtypes below 26/06/2006
	// AIOBJECT_CAR
	// AIOBJECT_BOAT
	// AIOBJECT_HELICOPTER
	int GetSubTypeOf(IFunctionHandler * pH);

	// <title GetParameter>
	// Syntax: AI.GetParameter( entityId, paramEnum )
	// Description: Changes an enumerated AI parameter.
	// Arguments:
	//   entityId - entity id
	//   paramEnum - index of the parameter to get, see AI.ChangeParameter() for complete list
	// Returns:
	//   parameter value
	// INTEGRATION : (MATT) GetAIParameter will appear from main branch and is duplicate {2007/08/03:11:00:39}
	// Embracing and extending their code whilst keeping the old name - wish to remain consistent with ChangeParameter above
	int GetParameter(IFunctionHandler * pH);

	// <title ChangeMovementAbility>
	// Syntax: AI.ChangeMovementAbility( entityId, paramEnum, paramValue )
	// Description: Changes an enumerated AI movement ability parameter.
	// Arguments:
	// entityId - entity id
	// paramEnum - the parameter to change, should be one of the following:
	//		AIMOVEABILITY_OPTIMALFLIGHTHEIGHT - optimal flight height while finding path (meters).
	//		AIMOVEABILITY_MINFLIGHTHEIGHT - minimum flight height while finding path (meters).
	//		AIMOVEABILITY_MAXFLIGHTHEIGHT - maximum flight height while finding path (meters).
	//	paramValue - new parameter value, see above for type and meaning.
	int ChangeMovementAbility(IFunctionHandler * pH);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Smart Object functions
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	/*
	// <title AddSmartObjectCondition>
	// Syntax: AI.AddSmartObjectCondition( smartObjectTable )
	// Description: Registers the given smartObjectTable to AI System
	// Arguments:
	// smartObjectTable - a table defining conditions on which specified Smart Object could be used
	int AddSmartObjectCondition(IFunctionHandler *pH);
	*/

	// <title ExecuteAction>
	// Syntax: AI.ExecuteAction( action, participant1 [, participant2 [, ... , participantN ] ] )
	// Description: Executes an Action on a set of Participants
	// Arguments:
	// action - Smart Object Action name or id
	// participant1 - entity id of the first Participant in the Action
	// (optional) participant2..N - entity ids of other participants
	int ExecuteAction(IFunctionHandler *pH);

	// <title AbortAction>
	// Syntax: AI.AbortAction( userId [, actionId ] )
	// Description: Aborts execution of an action if actionId is specified
	//				or aborts execution of all actions if actionId is nil or 0
	// Arguments:
	// userId - entity id of the user which AI action is aborted
	// (optional) actionId - id of action to be aborted or 0 (or nil) to abort all actions on specified entity
	int AbortAction(IFunctionHandler *pH);


	// <title SetSmartObjectState>
	// Syntax: AI.SetSmartObjectState( entityId, stateName )
	// Description: Sets only one single smart object state replacing all other states
	// Arguments:
	// entityId - entity id
	// stateName - name of the new state (i.e. "Idle")
	int SetSmartObjectState(IFunctionHandler *pH);

	// <title ModifySmartObjectStates>
	// Syntax: AI.ModifySmartObjectStates( entityId, listStates )
	// Description: Adds/Removes smart object states of a given entity
	// Arguments:
	// entityId - entity id
	// listStates - names of the states to be added and removed (i.e. "Closed, Locked - Open, Unlocked, Busy")
	int ModifySmartObjectStates(IFunctionHandler *pH);

	// <title SmartObjectEvent>
	// Syntax: AI.SmartObjectEvent( actionName, userEntityId, objectEntityId [, vRefPoint] )
	// Description: Executes a smart action
	// Arguments:
	// actionName - smart action name
	// usedEntityId - entity id of the user who needs to execute the smart action or a nil if user is unknown
	// objectEntityId - entity id of the object on which the smart action needs to be executed or a nil if objects is unknown
	// vRefPoint - (optional) point to be used instead of user's attention target position
	// Returns:
	// 0 if smart object rule wasn't found or a non-zero unique id of the goal pipe inserted to execute the action
	int SmartObjectEvent(IFunctionHandler *pH);

	// <title GetLastUsedSmartObject>
	// Syntax: AI.GetLastUsedSmartObject( userEntityId )
	// Description: Returns the last used smart object
	// Arguments:
	// usedEntityId - entity id of the user for which its last used smart object is needed
	// Returns:
	// nil if there's no last used smart object (or if an error has occurred)
	// or the script table of the entity which is the last used smart object
	int GetLastUsedSmartObject(IFunctionHandler *pH);

	// <title GetLastSmartObjectExitPoint>
	// Syntax: AI.GetLastSmartObjectExitPoint( userEntityId, pos)
	// Description: Returns the last used navigational smart object's exit point
	// Arguments:
	// usedEntityId - entity id of the user for which its last used smart object is needed
	// pos - returned position of last used navigational smartobject's exit point 
	// Returns:
	// true if the operation was successful
	int GetLastSmartObjectExitPoint(IFunctionHandler *pH);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	// GoalPipe functions
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// <title CreateGoalPipe>
	// Syntax: AI.CreateGoalPipe(string szPipeName )
	// Description: Used for warnings about data/script errors 
	// Arguments:
	// szPipeName - goal pipe name
	int CreateGoalPipe(IFunctionHandler *pH);

	// <title BeginGoalPipe>
	// Syntax: AI.BeginGoalPipe(string szPipeName )
	// Description: Creates a goal pipe and allows to start filling it.
	// Arguments:
	// szPipeName - goal pipe name
	int BeginGoalPipe(IFunctionHandler *pH);

	// <title EndGoalPipe>
	// Syntax: AI.EndGoalPipe()
	// Description: Ends creating a goal pipe
	int EndGoalPipe(IFunctionHandler *pH);

	// <title BeginGroup>
	// Syntax: AI.BeginGroup()
	// Description: to define group of goals
	// Arguments:
	int BeginGroup(IFunctionHandler *pH);

	// <title EndGroup>
	// Syntax: AI.EndGroup()
	// Description: to define end of group of goals
	int EndGroup(IFunctionHandler *pH);

	// <title PushGoal>
	// Syntax: AI.PushGoal(string szPipeName, string goalName, int blocking [,{params}] )
	// Description: Used for warnings about data/script errors 
	// Arguments:
	// szPipeName - goal pipe name
	// szGoalName - goal name - see AI Manual for a complete list of goals
	// blocking - used to mark the goal as blocking (goal pipe execution will stop here
	//	until the goal has finished) 0: not blocking, 1: blocking
	// (optional) params - set of parameters depending on the goal selected; 
	//	see the AI Manual for a complete list of the parameters for each goal
	int PushGoal(IFunctionHandler *pH);

	// <title PushLabel>
	// Syntax: AI.PushLabel(string szPipeName, string szLabelName)
	// Description: Used in combination with "branch" goal operation to identify jump destination
	// Arguments:
	// szPipeName - goalpipe name
	// szLabelName - label name
	int PushLabel(IFunctionHandler *pH);

	// <title IsGoalPipe>
	// Syntax: AI.IsGoalPipe(string szPipeName)
	// Description: Checks is a goalpipe of certain name exists already, returns true if pipe exists.
	// Arguments:
	// szPipeName - goalpipe name
	int IsGoalPipe(IFunctionHandler *pH);

	// <title ClearForReload>
	// Syntax: AI.ClearForReload(string szPipeName)
	// Description: Clears all goalpipes from the system
	// Arguments: none
	int ClearForReload(IFunctionHandler *pH);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Signal functions
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// <title Signal>
	// Syntax: AI.Signal(signalFilter,int signalType, string SignalText, senderId [, signalExtraData] )
	// Description: Sends a signal to an entity or a group of entities
	// Arguments:
	// sender Id: sender's entity id; despite the name, sender may not be necessarily the actual AI.Signal function caller
	// int signalFilter - filter which defines the subset of AIobjects which will receive the signal:
	//  SIGNALFILTER_SENDER: signal is sent to the sender only (may be not the caller itself in this case)
	//	SIGNALFILTER_GROUPONLY: signal is sent to all the AIObjects in the same sender's group, within its communication range
	//	SIGNALFILTER_GROUPONLY_EXCEPT: signal is sent to all the AIObjects in the same sender's group, within its communication range, except the sender itself
	//	SIGNALFILTER_SUPERGROUP: signal is sent signal is sent to all the AIObjects in the same sender's group
	//	SIGNALFILTER_SPECIESONLY: signal is sent to all the AIObjects in the same sender's species, in his comm range
	//	SIGNALFILTER_SUPERSPECIES: signal is sent to all AIObjects in the same sender's species
	//	SIGNALFILTER_ANYONEINCOMM: signal is sent to all the AIObjects in sender's communication range
	//	SIGNALFILTER_NEARESTGROUP: signal is sent to the nearest AIObject in the sender's group
	//	SIGNALFILTER_NEARESTINCOMM: signal is sent to the nearest AIObject, in the sender's group, within sender's communication range
	//	SIGNALFILTER_NEARESTINCOMM_SPECIES: signal is sent to the nearest AIObject, in the sender's species, within sender's communication range
	//	SIGNALFILTER_HALFOFGROUP: signal is sent to the first half of the AI Objects in the sender's group 
	//	SIGNALFILTER_LEADER: signal is sent to the sender's group CLeader Object
	//	SIGNALFILTER_LEADERENTITY: signal is sent to the sender's group leader AIObject (which has the CLeader associated)
	//  SIGNALFILTER_READIBILITY: signal is sent to the sender as a readability signal and will be processed by AIHandler for readability
	// int signalType - 
	//	-1: signal is processed by all entities, even if they're disabled/sleeping
	//	 0: signal is processed by all entities which are enabled
	//   1: signal is processed by all entities which are enabled and not sleeping
	//	10: signal is added in the sender's signal queue even if another signal with same text is present (normally it's not)
	// string signalText: signal text which will be processed by the receivers (in a Lua callback with the same name as the text, or directly by the CAIObject) 
	// (optional) signalExtraData: lua table which can be used to send extra data. It can contains the following values:
	//	 point: a vector in format {x,y,z}
	//	 point2: a vector in format {x,y,z}
	//	 ObjectName: a string
	//	 id: an entity id
	//   fValue: a float value
	//   iValue: an integer value
	//   iValue2: a second integer value
	int Signal(IFunctionHandler * pH);

	// <title FreeSignal>
	// Syntax: AI.FreeSignal(int signalType, string SignalText, position, radius [, entityID [,signalExtraData]] )
	// Description: Sends a signal to anyone in a given radius around a position
	// Arguments:
	// int signalType - see AI.Signal
	// string signalText - See AI.Signal
	// Vector position - center position {x,y,z}around which the signal is sent
	// float radius - radius inside which the signal is sent
	// (optional) entityID - signal is NOT sent to entities which groupID is same as the entity's one
	// (optional) signalExtraData - See AI.Signal
	int	FreeSignal(IFunctionHandler * pH);

	// <title SetIgnorant>
	// Syntax: AI.SetIgnorant(entityId, int ignorant)
	// Description: makes an AI ignore system signals, visual stimuli and sound stimuli
	// Arguments:
	// entityId - AI's entity id
	// ignorant - 0: not ignorant, 1: ignorant
	int SetIgnorant(IFunctionHandler * pH);


	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//// Group-Species functions
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// <title GetGroupCount>
	// Syntax: AI.GetGroupCount(entityId, flags, type)
	// Description: returns the given entity's group members count
	// Arguments:
	// entityId - AI's entity id
	// flags - combination of following flags:
	//		GROUP_ALL - Returns all agents in the group (default).
	//		GROUP_ENABLED - Returns only the count of enabled agents (exclusive with all).
	//		GROUP_MAX - Returns the maximum number of agents during the game (can be combined with all or enabled).
	// type - allows to filter to return only specific AI objects by type (cannot be used in together with GROUP_MAX flag).
	// Returns:
	// group members count
	int GetGroupCount(IFunctionHandler * pH);

	// <title GetGroupMember>
	// Syntax: AI.GetGroupMember(entityId|groupId,idx,flags,type)
	// Description: returns the idx-th entity in the given group
	// Arguments:
	// entityId|groupId - AI's entity id \ group id
	// idx - index  (1..N)
	// flags - combination of following flags:
	//		GROUP_ALL - Returns all agents in the group (default).
	//		GROUP_ENABLED - Returns only the count of enabled agents (exclusive with all).
	// type - allows to filter to return only specific AI objects by type (cannot be used in together with GROUP_MAX flag).
	// Returns:
	// script handler of idx-th entity (null if idx is out of range)
	int GetGroupMember(IFunctionHandler * pH);


	// <title GetGroupOf>
	// Syntax: AI.GetGroupOf(entityId)
	// Description: returns the given entity's group id
	// Arguments:
	// entityId - AI's entity id
	// Returns:
	// group id of given entity
	int GetGroupOf(IFunctionHandler * pH);

	// <title GetSpeciesOf>
	// Syntax: AI.GetSpeciesOf(entityId)
	// Description: returns the given entity's species id
	// Arguments:
	// entityId - AI's entity id
	// Returns:
	// species id of given entity
	int GetSpeciesOf(IFunctionHandler * pH);

	// <title GetUnitInRank>
	// Syntax: AI.GetUnitInRank(groupID [,rank] )
	// Description: Returns the entity in the group  id in the given rank position, or the highest if rank == nil or rank <=0
	// the rank is specified in entity.Properties.nRank;
	// Arguments: 
	// rank - rank position (the highest (1) if rank == nil or rank <=0)
	// Returns:
	// entity script table of the ranked unit
	int GetUnitInRank(IFunctionHandler * pH);

	// <title SetUnitProperties>
	// Syntax: AI.SetUnitProperties(entityId, unitProperties )
	// Description: Sets the leader knowledge about the units combat capabilities.
	// The leader will be found based on the group id of the entity.
	// Arguments: 
	// entityId - entity id
	// unitProperties - binary mask of unit properties type for which the attack is requested 
	//	(in the form of UPR_* + UPR* i.e. UPR_COMBAT_GROUND + UPR_COMBAT_FLIGHT)
	//	see IAgent.h for definition of unit properties UPR_*
	int SetUnitProperties(IFunctionHandler * pH);

	// <title GetUnitCount>
	// Syntax: AI.GetUnitCount(entityId, unitProperties)
	// Description: Gets the number of units the leader knows about.
	// The leader will be found based on the group id of the entity.
	// Arguments: 
	// entityId - entity id
	// unitProperties - binary mask of the returned unit properties type
	//	(in the form of UPR_* + UPR* i.e. UPR_COMBAT_GROUND + UPR_COMBAT_FLIGHT)
	//	see IAgent.h for definition of unit properties UPR_*
	int GetUnitCount(IFunctionHandler * pH);

	// <title Hostile>
	// Syntax: AI.Hostile(entityId, entity2Id|AIObjectName)
	// Description: returns true if the two entities are hostile
	// Arguments:
	// entityId - 1st AI's entity id
	// entity2Id |AIObjectName - 2nd AI's entity id | 2nd AIobject's name
	// Returns:
	// true if the entities are hostile
	int Hostile(IFunctionHandler * pH);

	// <title GetLeader>
	// Syntax: AI.GetLeader(groupID | entityID )
	// Description:
	// returns the leader's name of the given groupID / entity
	// Arguments:
	// groupID : group id
	// entityID : entity (id) of which we want to find the leader
	int GetLeader(IFunctionHandler * pH);

	// <title SetLeader>
	// Syntax: AI.SetLeader(entityID )
	// Description:
	// Set the given entity as Leader (associating a CLeader object to it and creating it if it doesn't exist)
	// Only one leader can be set per group
	// Arguments:
	// entityID : entity (id) of which to which we want to associate a leader class
	// Returns:
	// true if the leader creation/association has been successful
	int SetLeader(IFunctionHandler * pH);

	// <title GetBeaconPosition>
	// Syntax: AI.GetBeaconPosition(entityId | string AIObjectName, returnPos)
	// Description: get the beacon's position for the given entity/object's group
	// Arguments:
	// entityId | AIObjectName - AI's entity id | AI object name
	// returnPos - vector {x,y,z} where the beacon position will be set
	// Return:
	// true if the beacon has been found and the position set
	int GetBeaconPosition(IFunctionHandler * pH);

	// <title SetBeaconPosition>
	// Syntax: AI.SetBeaconPosition(entityId | string AIObjectName, pos)
	// Description: Set the beacon's position for the given entity/object's group
	// Arguments:
	// entityId | AIObjectName - AI's entity id | AI object name
	// pos - beacon position vector {x,y,z}
	int SetBeaconPosition(IFunctionHandler * pH);

	// <title RequestAttack>
	// Syntax: AI.RequestAttack(entityId,unitProperties, attackTypeList [,duration])
	// Description: in a group with leader, the entity requests for a group attack behavior against the enemy
	// The Cleader later will possibly create an attack leader action (CLeaderAction_Attack_*)
	// Arguments:
	// entityId - AI's entity id which group/leader is determined
	// unitProperties - binary mask of unit properties type for which the attack is requested 
	//	(in the form of UPR_* + UPR* i.e. UPR_COMBAT_GROUND + UPR_COMBAT_FLIGHT)
	//	see IAgent.h for definition of unit properties UPR_*
	// attackTypeList - a lua table which contains the list of preferred attack strategies (Leader action subtypes), 
	//	sorted by priority (most preferred first)
	//	the list must be in the format {LAS_*, LAS_*,..} i.e. (LAS_ATTACK_ROW,LAS_ATTACK_FLANK} means that first an Attack_row
	//	action will be tried, then an attack_flank if the first ends/fails.
	//	see IAgent.h for definition of LeaderActionSubtype (LAS_*) action types
	// duration - (optional) max duration in sec (default = 0)
	int RequestAttack(IFunctionHandler *pH);

	// <title GetGroupTarget>
	// Syntax: AI.GetGroupTarget(entityId [,bHostileOnly [,bLiveOnly]])
	// Description: Returns the most threatening attention target amongst the agents in the given entity's group
	// (see IAgent.h for definition of alert status)
	// Arguments:
	// entityId  - AI's entity id which group is determined
	// bHostileOnly (optional) - filter only hostile targets in group
	// bLiveOnly (optional) - filter only live targets in group (entities)
	int GetGroupTarget(IFunctionHandler * pH);

	// <title GetGroupTargetCount>
	// Syntax: AI.GetGroupTargetCount(entityId [,bHostileOnly [,bLiveOnly]])
	// Description: Returns the number of attention targets amongst the agents in the given entity's group
	// Arguments:
	// entityId  - AI's entity id which group is determined
	// bHostileOnly (optional) - filter only hostile targets in group
	// bLiveOnly (optional) - filter only live targets in group (entities)
	int GetGroupTargetCount(IFunctionHandler * pH);

	// <title GetGroupAveragePosition>
	// Syntax: AI.GetGroupAveragePosition(entityId, properties, returnPos)
	// Description: gets the average position of the (leader's) group members
	// Arguments:
	// entityId - AI's entity id which group/leader is determined
	// unitProperties - binary mask of unit properties type for which the attack is requested 
	//	(in the form of UPR_* + UPR* i.e. UPR_COMBAT_GROUND + UPR_COMBAT_FLIGHT)
	//	see IAgent.h for definition of unit properties UPR_*
	// returnPos - returned average position 
	int GetGroupAveragePosition(IFunctionHandler *pH);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Object finding
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// <title FindObjectOfType>
	// Syntax: AI.FindObjectOfType(entityId, radius, AIObjectType, flags [,returnPosition [,returnDirection]]) |
	//		   AI.FindObjectOfType(position, radius, AIObjectType, [,returnPosition [,returnDirection]])
	// Description: returns the closest AIObject of a given type around a given entity/position; 
	//				once an AIObject is found, it's devalued and can't be found again for some seconds (unless specified in flags)
	// Arguments:
	// entityId - AI's entity id as a center position of the search (if 1st parameter is not a vector)
	// position - center position of the search (if 1st parameter is not an entity id)
	// radius - radius inside which the object must be found
	// AIObjectType - AIObject type; see ScriptBindAI.cpp and Scripts/AIAnchor.lua for a complete list of AIObject types available
	// flags - filter for AI Objects to search, which can be the sum of one or more of the following values:
	//	AIFAF_VISIBLE_FROM_REQUESTER: Requires whoever is requesting the anchor to also have a line of sight to it
	//  AIFAF_VISIBLE_TARGET: Requires that there is a line of sight between target and anchor
	//  AIFAF_INCLUDE_DEVALUED: don't care if the object is devalued
	//	AIFAF_INCLUDE_DISABLED: don't care if the object is disabled
	// (optional) returnPosition - position of the found object
	// (optional) returnDirection - direction of the found object
	// Returns:
	// the found AIObject's name (nil if not found)
	int FindObjectOfType(IFunctionHandler * pH);

	// <title GetAnchor>
	// Syntax: AI.GetAnchor(entityId, radius, AIAnchorType, searchType [,returnPosition [,returnDirection]]) |
	// Description: returns the closest Anchor of a given type around a given entity; 
	//				once an Anchor is found, it's devalued and can't be found again for some seconds (unless specified in flags)
	// Arguments:
	// entityId - AI's entity id as a center position of the search 
	// radius - radius inside which the object must be found. Alternatively a search range can be specified {min=minRad,max=maxRad}.
	// AIAnchorType - Anchor type; see Scripts/AIAnchor.lua for a complete list of Anchor types available
	// searchType - search type filter, which can be one of the following values:
	//  AIANCHOR_NEAREST: (default) the nearest anchor of the given type
	//	AIANCHOR_NEAREST_IN_FRONT: the nearest anchor inside the front cone of entity
	//  AIANCHOR_NEAREST_FACING_AT: the nearest anchor of given type which is oriented towards entity's attention target (...Dejan? )
	//  AIANCHOR_RANDOM_IN_RANGE: a random anchor of the given type 
	//  AIANCHOR_NEAREST_TO_REFPOINT: the nearest anchor of given type to the entity's reference point
	// (optional) returnPosition - position of the found object
	// (optional) returnDirection - direction of the found object
	// Returns:
	// the found Anchor's name (nil if not found)
	int GetAnchor(IFunctionHandler * pH);

	// <title GetNearestEntitiesOfType>
	// Syntax: AI.GetNearestEntitiesOfType(entityId|objectname|position, AIObjectType, maxObjects, returnList [,objectFilter [,radius]]) |
	// Description: returns a list of the closest N entities of a given AIObjkect type associated
	//				the found objects are then devalued
	// Arguments:
	// entityId | objectname |position - AI's entity id | name| position as a center position of the search 
	// radius - radius inside which the entities must be found
	// AIObjectType - AIObject type; see ScriptBindAI.cpp and Scripts/AIAnchor.lua for a complete list of AIObject types available
	// maxObjects - maximum number of objects to find
	// return list - Lua table which will be filled up with the list of the found entities (Lua handlers)
	// (optional) objectFilter - search type filter, which can be the sum of one or more of the following values:
	//	 AIOBJECTFILTER_SAMESPECIES: AI objects of the same species of the querying object
	//	 AIOBJECTFILTER_SAMEGROUP: AI objects of the same group of the querying object or with no group
	//	 AIOBJECTFILTER_NOGROUP :AI objects with Group ID ==AI_NOGROUP
	//   AIOBJECTFILTER_INCLUDEINACTIVE :Includes objects which are inactivated. 
	// (optional) radius - serch range( default 30m )
	// Returns:
	// the number of found entities
	int GetNearestEntitiesOfType(IFunctionHandler * pH);

	// <title GetAIObjectPosition>
	// Syntax: AI.GetAIObjectPosition(entityId | string AIObjectName)
	// Description: get the given AIObject's position 
	// Arguments:
	// entityId | AIObjectName - AI's entity id | AI object name
	// Return:
	// AI Object position vector {x,y,z}
	int GetAIObjectPosition(IFunctionHandler * pH);

	// <title IgnoreCurrentHideObject>
	// Syntax: AI.IgnoreCurrentHideObject(entityId)
	// Description: Marks the current hideobject unreachable (will be omitted from future hidespot selections).
	// Arguments:
	// entityId - AI's entity id
	int IgnoreCurrentHideObject(IFunctionHandler * pH);

	// <title GetCurrentHideAnchor>
	// Syntax: AI.GetCurrentHideAnchor(entityId)
	// Description: Returns the name of the current anchor the entity is using for cover
	// Arguments:
	// entityId - AI's entity id
	int GetCurrentHideAnchor(IFunctionHandler * pH);


	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// Attention Target / perception related functions
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	int GetForwardDir(IFunctionHandler *pH);

	// <title GetPlayerThreatLevel>
	// Syntax: AI.GetPlayerThreatValue()
	// Description: Returns the value of player’s threat level (float number from 0 to 1.0)
	// Arguments: none
	int GetPlayerThreatLevel(IFunctionHandler * pH);

	// <title GetAttentionTargetOf>
	// Syntax: AI.GetAttentionTargetOf(entityId)
	// Description: returns the given entity's attention target
	// Arguments:
	// entityId - AI's entity id
	// Returns:
	// attention target's name (nil if no target)
	int GetAttentionTargetOf(IFunctionHandler * pH);

	// <title GetAttentionTargetType>
	// Syntax: AI.GetAttentionTargetType(entityId)
	// Description: returns the given entity's attention target type (AIOBJECT_*)
	// Arguments:
	// entityId - AI's entity id
	// Returns:
	// attention target's type (AIOBJECT_NONE if no target)
	int GetAttentionTargetType(IFunctionHandler * pH);

	// <title GetAttentionTargetEntity>
	// Syntax: AI.GetAttentionTargetEntity(entityId)
	// Description: returns the given entity's attention target entity (if it is an entity) or 
	// the owner entity of the attention target if it is a dummy object (if there is an owner entity)
	// Arguments:
	// entityId - AI's entity id
	// Returns:
	// attention target's entity
	int GetAttentionTargetEntity(IFunctionHandler * pH);

	// <title GetAttentionTargetPosition>
	// Syntax: AI.GetAttentionTargetPosition(entityId, returnPos)
	// Description: returns the given entity's attention target's position
	// Arguments:
	// entityId - AI's entity id
	// returnPos - vector {x,y,z} passed as a return value (attention target 's position)
	int GetAttentionTargetPosition(IFunctionHandler * pH);

	// <title GetAttentionTargetDirection>
	// Syntax: AI.GetAttentionTargetDirection(entityId, returnDir)
	// Description: returns the given entity's attention target's direction
	// Arguments:
	// entityId - AI's entity id
	// returnDir - vector {x,y,z} passed as a return value (attention target 's direction)
	int GetAttentionTargetDirection(IFunctionHandler * pH);

	// <title GetAttentionTargetViewDirection>
	// Syntax: AI.GetAttentionTargetViewDirection(entityId, returnDir)
	// Description: returns the given entity's attention target's view direction
	// Arguments:
	// entityId - AI's entity id
	// returnDir - vector {x,y,z} passed as a return value (attention target 's direction)
	int GetAttentionTargetViewDirection(IFunctionHandler * pH);

	// <title GetAttentionTargetDistance>
	// Syntax: AI.GetAttentionTargetDistance(entityId)
	// Description: returns the given entity's attention target's distance to the entity
	// Arguments:
	// entityId - AI's entity id
	// Returns: distance to the attention target (nil if no target)
	int GetAttentionTargetDistance(IFunctionHandler * pH);

	// <title UpTargetPriority>
	// Syntax: AI.UpTargetPriority(entityId, targetId, float increment)
	// Description: modifies the current entity's target priority for the given target
	//				if the given target is among the entity's target list
	// Arguments:
	// entityId - AI's entity id
	// targetId - target's entity id
	// float increment - value to modify the target's priority
	int UpTargetPriority(IFunctionHandler * pH);

	// <title DropTarget>
	// Syntax: AI.DropTarget(entityId, targetId)
	// Description: Clears the target from the AI's perception handler
	// Arguments:
	// entityId - AI's entity id
	// targetId - target's entity id
	int DropTarget(IFunctionHandler * pH);

	// <title ClearPotentialTargets>
	// Syntax: AI.ClearPotentialTargets(entityId)
	// Description: Clears all the potential targets from the AI's perception handler
	// Arguments:
	// entityId - AI's entity id
	int ClearPotentialTargets(IFunctionHandler * pH);

	// <title SetTempTargetPriority>
	// Syntax: AI.SetTempTargetPriority(entityId, priority)
	// Description: Set the selection priority of the temp target over other potential targets
	// Arguments:
	// entityId - AI's entity id
	// priority - New priority value
	// Returns: true if updated
	int SetTempTargetPriority(IFunctionHandler * pH);

	// <title AddAggressiveTarget>
	// Syntax: AI.AddAggressiveTarget(entityId, targetId)
	// Description: Add the target Id as an aggressive potential target to the entity's list
	// Arguments:
	// entityId - AI's entity id
	// targetId - Target to add to the list
	// Returns: true if updated
	int AddAggressiveTarget(IFunctionHandler * pH);

	// <title UpdateTempTarget>
	// Syntax: AI.UpdateTempTarget(entityId, vPos)
	// Description: Updates the entity's temporary potential target to the given position
	// Arguments:
	// entityId - AI's entity id
	// vPos - New position of the temporary target
	// Returns: true if updated
	int UpdateTempTarget(IFunctionHandler * pH);

	// <title ClearTempTarget>
	// Syntax: AI.ClearTempTarget(entityId,)
	// Description: Removes the entity's temporary potential target, so it is no longer considered for target selection
	// Arguments:
	// entityId - AI's entity id
	// Returns: true if cleared
	int ClearTempTarget(IFunctionHandler * pH);

	// <title SetExtraPriority>
	// Syntax: AI.SetExtraPriority(enemyEntityId, float increment)
	// Syntax: AI.GetExtraPriority(enemyEntityId)
	// Description:get/set a extra priority value to the entity which is given by enemyEntityId
	// Arguments:
	// enemyEntityId - AI's entity id
	// float increment - value to add to the target's priority
	int SetExtraPriority(IFunctionHandler * pH); 
	int GetExtraPriority(IFunctionHandler * pH);

	// <title GetTargetType>
	// Syntax: AI.GetTargetType(entityId)
	// Description: Returns the type of current entity's attention target (memory, human, none etc)
	// Arguments:
	// entityId - AI's entity id
	// Returns:
	// Target type value (AITARGET_NONE,AITARGET_MEMORY,AITARGET_BEACON,AITARGET_ENEMY etc) - 
	//	see IAgent.h for all definitions of target types
	int GetTargetType(IFunctionHandler * pH); 

	// <title RegisterTargetTrack>
	// Syntax: AI.RegisterTargetTrack(entityId, configuration, targetLimit, classThreat)
	// Description: Registers the AI object to use the given target track configuration for target selection
	// Arguments:
	// entityId - AI's entity id
	// configuration - Target stimulus configuration to use
	// targetLimit - Number of agents who can target the AI at any given time (0 for infinite)
	// classThreat - (Optional) Initial class threat value for the user
	// Returns:
	// TRUE if registration was successful. Note that this does nothing if 'ai_TargetTracking' is not set to '2'
	int RegisterTargetTrack(IFunctionHandler * pH);

	// <title UnregisterTargetTrack>
	// Syntax: AI.UnregisterTargetTrack(entityId)
	// Description: Unregisters the AI object with the target track manager
	// Arguments:
	// entityId - AI's entity id
	// Returns:
	// TRUE if unregistration was successful. Note that this does nothing if 'ai_TargetTracking' is not set to '2'
	int UnregisterTargetTrack(IFunctionHandler * pH);

	// <title SetTargetTrackClassThreat>
	// Syntax: AI.SetTargetTrackClassThreat(entityId, classThreat)
	// Description: Sets the class threat for the entity for target track usage
	// Arguments:
	// entityId - AI's entity id
	// classThreat - Class threat value for the user
	int SetTargetTrackClassThreat(IFunctionHandler * pH);

	// <title SoundEvent>
	// Syntax: AI.SoundEvent(position, radius, threat, interest, entityId) 
	// Description: Generates a sound event in the AI system with the given parameters.
	// Arguments: 
	// position - of the origin of the sound event
	// radius - in which this sound event should be heard
	// threat - value of the sound event 
	// interest - value of the sound event
	// entityId - of the entity who generated this sound event
	int SoundEvent(IFunctionHandler *pH);

	// <title VisualEvent>
	// Syntax: AI.VisualEvent(entityId, targetId) 
	// Description: Generates a visual event in the AI system with the given parameters.
	// Arguments: 
	// entityId - who receives the visual event
	// targetId - who the receiver is seeing
	int VisualEvent(IFunctionHandler *pH);

	// <title GetSoundPerceptionDescriptor>
	// Syntax: AI.GetSoundPerceptionDescriptor(entityId, soundType, descriptorTable) 
	// Description: Fills descriptorTable with info about how perception works for the entity dealing with soundType
	// Arguments: 
	// entityId - who to get the info from
	// soundType - what type of sound stimulus to get the info for
	// descriptorTable - where to store the info once retrieved
	// Returns:
	// True if info was returned
	int GetSoundPerceptionDescriptor(IFunctionHandler *pH);

	// <title SetSoundPerceptionDescriptor>
	// Syntax: AI.SetSoundPerceptionDescriptor(entityId, soundType, descriptorTable) 
	// Description: Sets data on how perception works for the entity dealing with soundType
	// Arguments: 
	// entityId - who to set the info for
	// soundType - what type of sound stimulus to set the info for
	// descriptorTable - info to be set
	// Returns:
	// True if info was saved
	int SetSoundPerceptionDescriptor(IFunctionHandler *pH);

	// <title SetAssesmentMultiplier>
	// Syntax: AI.SetAssesmentMultiplier(AIObjectType, float multiplier)
	// Description: set the assesment multiplier factor for the given AIObject type
	// Arguments:
	// AIObjectType - AIObject type; see ScriptBindAI.cpp for a complete list of AIObject types available
	// multiplier - assesment multiplication factor 
	int SetAssesmentMultiplier(IFunctionHandler * pH);

	// <title SetSpeciesThreatMultiplier>
	// Syntax: AI.SetSpeciesThreatMultiplier(nSpecies, float multiplier)
	// Description: set the threat multiplier factor for the given species (if 0, species is not hostile to any other)
	// Arguments:
	// nSpecies: species ID. nSpecies must not be 0, use SetPlayerSpeciesThreatMultiplier(mult) if this is required.
	// multiplier - Threat multiplication factor
	int SetSpeciesThreatMultiplier(IFunctionHandler * pH);

	// <title SetPlayerSpeciesThreatMultiplier>
	// Syntax: AI.SetPlayerSpeciesThreatMultiplier(float multiplier)
	// Description: set the threat multiplier factor for the player species (if 0, species is not hostile to any other)
	// Arguments:
	// multiplier - Threat multiplication factor
	int SetPlayerSpeciesThreatMultiplier(IFunctionHandler * pH);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Reference point script methods
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// <title GetRefPointPosition>
	// Syntax: AI.GetRefPointPosition(entity Ent)
	// Description:
	// Get the entity's reference point World position 
	// Arguments:
	// ent - entity id
	// Returns:
	// (script)vector (x,y,z) reference point position
	int GetRefPointPosition(IFunctionHandler * pH);

	// <title GetRefPointDirection>
	// Syntax: AI.GetRefPointDirection(entity Ent )
	// Description:
	// Get the entity's reference point direction 
	// Arguments:
	// ent - entity id
	// Returns:
	// (script)vector (x,y,z) reference point direction
	int GetRefPointDirection(IFunctionHandler * pH);

	// <title SetRefPointPosition>
	// Syntax: AI.SetRefPointPosition(entity ent, Vector& vRefPointPos )
	// Description:
	// Sets the reference point's World position of an entity
	// Arguments:
	// ent - entity
	// vRefPointPos - (script)vector (x,y,z) value
	int SetRefPointPosition(IFunctionHandler * pH);
	// <title SetRefPointDirection>
	// Syntax: AI.SetRefPointDirection( Vector& vRefPointDir )
	// Description:
	// Sets the reference point's World position of an entity
	// Arguments:
	// vRefPointDir - (script)vector (x,y,z) value
	int SetRefPointDirection(IFunctionHandler * pH);
	// <title SetRefPointRadius>
	// Syntax: AI.SetRefPointRadius(entity.id, radius)
	// Description:
	// Sets the reference point's radius.
	// Arguments:
	// entityId - the entity id of the AI.
	// radius - the radius to set.
	int SetRefPointRadius(IFunctionHandler * pH);

	// <title SetRefShapeName>
	// Syntax: AI.SetRefShapeName(entityId, name)
	// Description:
	// Sets the reference shape name.
	// Arguments:
	// entityId - the ID if the entity
	// name - the name of the reference shape.
	int SetRefShapeName(IFunctionHandler * pH);
	// <title GetRefShapeName>
	// Syntax: AI.GetRefShapeName(entityId)
	// Description:
	// Returns the reference shape name.
	// Arguments:
	// entityId - the ID if the entity
	int GetRefShapeName(IFunctionHandler * pH);

	// <title SetTerritoryShapeName>
	// Syntax: AI.SetTerritoryShapeName(entityId, shapeName)
	// Description:
	// Sets the territory of the puppet.
	// Arguments:
	// entityId - the id of the entity
	// shapeName - name of the shape to set
	int SetTerritoryShapeName(IFunctionHandler * pH);

	// <title SetCharacter>
	// Syntax: AI.SetCharacter(entityId, newCharater)
	// Description:
	// Sets the AI character of the entity.
	// Arguments:
	// entityId - AI's entity id
	// newCharacter - name of the new character to set.
	int	SetCharacter(IFunctionHandler * pH);

	// <title SetCharacter>
	// Syntax: AI.SetRefPointAtDefensePos(entityId, point2defend,distance)
	// Description:
	// Set the entity refpoint position in an intermediate distance between the entity's att target and the given point
	// entityId - AI's entity id
	// point2defend - point to defend
	// distance - max distance to keep from the point
	int SetRefPointAtDefensePos(IFunctionHandler *pH);


	// <title SetPathToFollow>
	// Syntax: AI.SetPathToFollow(entityId, pathName)
	// Description:
	// Set the name of the path to be used in 'followpath' goal operation.
	// entityId - AI's entity id
	// pathName - (string) name of the path to set to be followed.
	int SetPathToFollow(IFunctionHandler *pH);

	// <title SetPathToFollowBySpline>
	// Syntax: AI.SetPathAttributeToFollow(entityId, flg)
	// Description:
	// Set the attribute of the path to be used in 'followpath' goal operation.
	// entityId - AI's entity id
	int SetPathAttributeToFollow(IFunctionHandler *pH);

	// <title GetNearestPathOfTypeInRange>
	// Syntax: AI.GetNearestPathOfTypeInRange(entityId, pos, range, type <, devalue, useStartNode>)
	// Description:
	// Queries a nearest path of specified type. The type uses same types as anchors and is specified in the path properties.
	// The function will only return paths that match the requesters (entityId) navigation caps. The nav type is also
	// specified in the path properties.
	// entityId - AI's entity id
	// pos - a vector specifying to the point of interest. Path nearest to this position is returned.
	// range - search range. If useStartNode=1, paths whose start point are within this range are returned or
	//   if useStartNode=0 nearest distance to the path is calculated and compared against the range.
	// type - type of path to return.
	// devalue - (optional) specifies the time the returned path is marked as occupied.
	// useStartNode - (optional) if set to 1 the range check will use distance to the start node on the path,
	//   else nearest distance to the path is used.
	int GetNearestPathOfTypeInRange(IFunctionHandler *pH);

	// <GetTotalLengthOfPath>
	// Syntax: AI.GetTotalLengthOfPath( entityId, pathname )
	// Description:
	// returns a total length of the path
	// entityId - AI's entity id
	// pathname - designers path name
	int GetTotalLengthOfPath(IFunctionHandler *pH);

	// <GetNearestPointOnPath>
	// Syntax: AI.GetNearestPointOnPath( entityId, pathname , vPos )
	// Description:
	// returns a nearest point on the path from vPos
	// entityId - AI's entity id
	// pathname - designers path name
	int GetNearestPointOnPath(IFunctionHandler *pH);

	// <GetPathSegNoOnPath>
	// Syntax: AI.GetPathSegNoOnPath( entityId, pathname , vPos )
	// Description:
	// returns segment ratio ( 0.0 start point 100.0 end point )
	// entityId - AI's entity id
	// pathname - designers path name
	int GetPathSegNoOnPath(IFunctionHandler *pH);

	// <GetPointOnPathBySegNo>
	// Syntax: AI.GetPathSegNoOnPath( entityId, pathname , segNo )
	// Description:
	// returns point by segment ratio ( 0.0 start point 100.0 end point )
	// entityId - AI's entity id
	// segNo - segment ratio
	// pathname - designers path name
	int GetPointOnPathBySegNo(IFunctionHandler *pH);

	// <GetPathLoop>
	// Syntax: AI.GetPathSegNoOnPath( entityId, pathname )
	// Description:
	// returns true if the path is looped
	// entityId - AI's entity id
	// pathname - designers path name
	int GetPathLoop(IFunctionHandler *pH);

	// <GetPathLoop>
	// Syntax: AI.GetPredictedPosAlongPath( entityId, time,retPos )
	// Description:
	// Get's the agent preticted position along his path at a given time
	// entityId - AI's entity id
	// time - prediction time (sec)
	// retPos - return point value
	// return:
	// true if successful
	int GetPredictedPosAlongPath(IFunctionHandler * pH);

	// <title SetPointListToFollow>
	// Syntax: AI.SetPointListToFollow(entityId, pointlist, howmanypoints, bspline, navtype)
	// Description:
	// Set a point list for followpath goal op
	// entityId - AI's entity id
	// pointList should be like below
	//	local	vectors = { 
	//		{ x = 0.0, y = 0.0, z = 0.0 }, -- vectors[1].(x,y,z)
	//		{ x = 0.0, y = 0.0, z = 0.0 }, -- vectors[2].(x,y,z)
	//		{ x = 0.0, y = 0.0, z = 0.0 }, -- vectors[3].(x,y,z)
	//		{ x = 0.0, y = 0.0, z = 0.0 }, -- vectors[4].(x,y,z)
	//		{ x = 0.0, y = 0.0, z = 0.0 }, -- vectors[5].(x,y,z)
	//		{ x = 0.0, y = 0.0, z = 0.0 }, -- vectors[6].(x,y,z)
	//		{ x = 0.0, y = 0.0, z = 0.0 }, -- vectors[7].(x,y,z)
	//		{ x = 0.0, y = 0.0, z = 0.0 }, -- vectors[8].(x,y,z)
	//	}
	// if bspline == true, the line is recalcurated by spline interpolation.
	// navtype(optional) specify a navigation type ( default = IAISystem::NAV_FLIGHT )
	int SetPointListToFollow(IFunctionHandler *pH);

	// <title CanMoveStraightToPoint>
	// Syntax: AI.CanMoveStraightToPoint(entityId, position)
	// Description:
	// Returns true if the entity can move to the specified position in a straight line (no multiple segment path necessary)
	// Arguments:
	// entityId - AI's entity id
	// position - the position to check
	// Returns:
	// true if the position can be reached in a straight line
	int CanMoveStraightToPoint(IFunctionHandler *pH);

	// <title GetNearestHidespot>
	// Syntax: AI.GetNearestHidespot(entityId, rangeMin, rangeMax <, center>)
	// Description:
	// Returns position of a nearest hidepoint within specified range, returns nil if no hidepoint is found.
	// entityId - AI's entity id
	// rangeMin/rangeMax - specifies the min/max range where the hidepoints are looked for.
	// center - (optional) specifies the center of search. If not specified, the entity position is used.
	int GetNearestHidespot(IFunctionHandler * pH);

	// <title GetEnclosingGenericShapeOfType>
	// Syntax: AI.GetEnclosingGenericShapeOfType(position, type, <checkHeight>)
	// Description:
	// Returns the name of the first shape that is enclosing the specified point and is of specified type
	// position - the position to check
	// type - the type of the shapes to check against (uses anchor types).
	// checkHeight - (optional) Default=false, if the flag is set the height of the shape is tested too.
	//    The test will check space between the shape.aabb.min.z and shape.aabb.min.z+shape.height.
	int GetEnclosingGenericShapeOfType(IFunctionHandler * pH);

	// <title IsPointInsideGenericShape>
	// Syntax: AI.IsPointInsideGenericShape(position, shapeName, <checkHeight>)
	// Description:
	// Returns true if the point is inside the specified shape.
	// position - the position to check
	// shapeName - the name of the shape to test (returned by AI.GetEnclosingGenericShapeOfType)
	// checkHeight - (optional) Default=false, if the flag is set the height of the shape is tested too.
	//    The test will check space between the shape.aabb.min.z and shape.aabb.min.z+shape.height.
	int IsPointInsideGenericShape(IFunctionHandler * pH);

	// <title ConstrainPointInsideGenericShape>
	// Syntax: AI.ConstrainPointInsideGenericShape(position, shapeName, <checkHeight>)
	// Description:
	// Returns the nearest point inside specified shape.
	// position - the position to check
	// shapeName - the name of the shape to use as constraint.
	// checkHeight - (optional) Default=false, if the flag is set the height should be constrained too.
	int ConstrainPointInsideGenericShape(IFunctionHandler * pH);

	// <title DistanceToGenericShape>
	// Syntax: AI.DistanceToGenericShape(position, shapeName, <checkHeight>)
	// Description:
	// Returns true if the point is inside the specified shape.
	// position - the position to check
	// shapeName - the name of the shape to test (returned by AI.GetEnclosingGenericShapeOfType)
	// checkHeight - (optional) if the flag is set the height of the shape is tested too.
	//    The test will check space between the shape.aabb.min.z and shape.aabb.min.z+shape.height.
	int DistanceToGenericShape(IFunctionHandler * pH);

	// <title CreateTempGenericShapeBox>
	// Syntax: AI.CreateTempGenericShapeBox(center, radius, height, type)
	// Description:
	// Creates a temporary box shaped generic shape (will be destroyed upon AIsystem reset).
	// Returns the name of the shape.
	// center - the center of the box
	// radius - the extend of the box in x and y directions.
	// height - the height of the box.
	// type - the AIanchor type of the shape.
	int CreateTempGenericShapeBox(IFunctionHandler * pH);

	// <title GetObjectRadius>
	// Syntax: AI.GetObjectRadius(entityId)
	// Description:
	// Returns the radius of specified AI object.
	// entityId - AI's entity id
	int GetObjectRadius(IFunctionHandler * pH);

	// <title GetProbableTargetPosition>
	// Syntax: AI.GetProbableTargetPosition(entityId)
	// Description:
	// Returns the probable target position of the AI.
	// entityId - AI's entity id
	int GetProbableTargetPosition(IFunctionHandler * pH);

	int NotifyGroupTacticState(IFunctionHandler * pH);
	int GetGroupTacticState(IFunctionHandler * pH);
	int GetGroupTacticPoint(IFunctionHandler * pH);
	int NotifyReinfDone(IFunctionHandler * pH);

	int BehaviorEvent(IFunctionHandler* pH);

	int NotifySurpriseEntityAction(IFunctionHandler * pH);

	int InvalidateHidespot(IFunctionHandler *pH);
	int EvalHidespot(IFunctionHandler * pH);

	// <title EvalPeek>
	// Syntax: AI.EvalPeek(entityId, bGetOptimalSide)
	// Description:
	// Evaluates if an AI object can peek from his current position
	// entityId - AI's entity id
	// bGetOptimalSide (optional) - If TRUE, and AI object can peek from both sides, will return the 
	//	side that best fits where the attention target currently is
	// Returns:
	//  -1 - don't need to peek
	//  0 - cannot peek
	//  1 - can peek from left
	//  2 - can peek from right
	//  3 - can peek from left & right
	int EvalPeek(IFunctionHandler * pH);

	// <title GetDirectAnchorPos>
	// Syntax: AI.GetDirectAttackPos(entityId, searchRange, minAttackRange)
	// Description:
	// Returns a cover point which can be used to directly attack the attention target.
	// Useful for choosing attack position for RPGs and such. Returns nil if no attack point is available.
	// Note: Calling the function is quite heavy since it does raycasting.
	// Arguments:
	// entityId - AI's entity id
	// AIAnchorType - Anchor type; see Scripts/AIAnchor.lua for a complete list of Anchor types available
	// maxDist - search range
	int GetDirectAnchorPos(IFunctionHandler * pH);

	int	GetGroupAlertness(IFunctionHandler * pH);

	// Returns the estimated surrounding navigable space in meters.
	int	GetEnclosingSpace(IFunctionHandler * pH);

	int Event(IFunctionHandler * pH);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Formation related functions 
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// <title CreateFormation>
	// Syntax: AI.CreateFormation( string name )
	// Description:
	// Creates a formation descriptor and adds a fixed node at 0,0,0 (owner's node)
	// Arguments:
	// name - name of the new formation descriptor
	int CreateFormation(IFunctionHandler * pH);

	// <title AddFormationPointFixed>
	// Syntax: AI.AddFormationPointFixed(name,sightangle, x,y,z [,unit_class] )
	// Description:
	// Adds a node with a fixed offset to a formation descriptor
	// Arguments:
	// name - name of the formation descriptor
	// x,y,z - offset from formation owner
	// sightangle - angle of sight of the node (-180,180; 0 = the guy looks forward)
	// unit_class - class of soldier (see eSoldierClass definition in IAgent.h)
	int AddFormationPointFixed(IFunctionHandler * pH);

	// <title AddFormationPoint>
	// Syntax: AI.AddFormationPoint(name, sightangle, distance,offset, [unit_class [,distanceAlt,offsetAlt]] )
	// Description:
	// Adds a follow-type node to a formation descriptor
	// Arguments:
	// name - name of the formation descriptor
	// distance - distance from the formation's owner
	// offset - X offset along the following line (negative = left, positive = right)
	// sightangle - angle of sight of the node (-180,180; 0 = the guy looks forward)
	// unit_class - class of soldier (see eSoldierClass definition in IAgent.h)
	// distanceAlt (optional)- alternative distance from the formation owner (if 0, distanceAlt and offsetAlt will be set respectively to distance and offset)
	// offsetAlt (optional) - alternative X offset
	int AddFormationPoint(IFunctionHandler * pH);

	// <title AddFormationPoint>
	// Syntax: AI.AddFormationPoint(name, distance,offset,sightangle, [unit_class [,distanceAlt,offsetAlt]] )
	// Description:
	// Adds a follow-type node to a formation descriptor
	// Arguments:
	// name - name of the formation descriptor
	// position - point index in the formation (1..N)
	// Returns:
	// class of specified formation point (-1 if not found)
	int GetFormationPointClass(IFunctionHandler * pH);

	// <title GetFormationPointPosition>
	// Syntax: AI.GetFormationPointPosition(entityId,pos )
	// Description:
	// gets the AI's formation point position
	// Arguments:
	// entityId - AI's entity id
	// pos - return value- position of entity AI's current formation point if there is
	// Returns:
	// true if the formation point has been found
	int GetFormationPointPosition(IFunctionHandler * pH);

// <title BeginTrackPattern>
	// Syntax: AI.BeginTrackPattern( patternName, flags, validationRadius, [stateTresholdMin],
	//		[stateTresholdMax], [globalDeformTreshold], [localDeformTreshold], [exposureMod], [randomRotAng] )
	// Example: AI.BeginTrackPattern( "mypattern", AITRACKPAT_VALIDATE_SWEPTSPHERE, 1.0 );
	// Description:
	//		Begins definition of a new track pattern descriptor. The pattern is created bu calling
	//		the AI.AddPatternPoint() and AI.AddPatternBranch() functions, and finalised by calling
	//		the AI.EndTrackPattern().
	// Arguments:
	// patternName - name of the new track pattern descriptor
	// flags - The functionality flags of the track pattern.
	//	Validation:
	//		The validation method describes how the pattern is validated to fit the physical world.
	//		Should be one of: 
	//		AITRACKPAT_VALIDATE_NONE - no validation at all.
	//		AITRACKPAT_VALIDATE_SWEPTSPHERE - validate using swept sphere tests, the spehre radius is validation
	//			radius plus the entity pass radius.
	//		AITRACKPAT_VALIDATE_RAYCAST - validate using raycasting, the hit position is pulled back by the amount
	//			of validation radius plus the entity pass radius.
	//	Aligning:
	//		When the pattern is selected to be used the alignment of the patter ncan be changed.
	//		The alignment can be combination of the following. The descriptions are in order they are evaluated.
	//		AITRACKPAT_ALIGN_TO_TARGET - Align the pattern so that the y-axis will
	//			point towards the target each time it is set. If the agent does not have valid attention target
	//			at the time the pattern is set the pattern will be aligned to the world.
	//		AITRACKPAT_ALIGN_RANDOM - Align the pattern randonly each time it is set.
	//			The rotation ranges are set using SetRandomRotation().
	// validationRadius - the validation radius is added to the entity pass radius when validating
	//		the pattern along the offsets.
	// stateTresholdMin (optional) - If the state of the pattern is 'enclosed' (high deformation) and
	///		the global deformation < stateTresholdMin, the state becomes exposed. Default 0.35.
	// stateTresholdMax (optional) - If the state of the pattern is 'exposed' (low deformation) and
	//		the global deformation > stateTresholdMax, the state becomes enclosed. Default 0.4.
	// globalDeformTreshold (optional) - the deformation of the whole pattern is tracked in range [0..1].
	//		This treshold value can be used to clamp the bottom range, so that values in range [trhd..1] becomes [0..1], default 0.0.
	// localDeformTreshold (optional) - the deformation of the each node is tracked in range [0..1].
	//		This treshold value can be used to clamp the bottom range, so that values in range [trhd..1] becomes [0..1], default 0.0.
	// exposureMod (optional) - the exposure modifier allows to take the node exposure (how much it is seen by
	//		the tracked target) into account when branching. The modifier should be in range [-1..1], -1 means to
	//		favor unseen nodes, and 1 means to favor seen, exposed node. Default 0 (no effect).
	// randomRotAng (optional) - each time the pattern is set, it can be optionally rotated randomly.
	//		This parameter allows to define angles (in degrees) around each axis. The rotation is performed in XYZ order.
	int BeginTrackPattern(IFunctionHandler * pH);

	// <title AddPatternNode>
	// Syntax: AI.AddPatternNode( nodeName, offsetx, offsety, offsetz, flags, [parent], [signalValue] )
	// Example: AI.AddPatternNode( "point1", 1.0, 0, 0, AITRACKPAT_NODE_START+AITRACKPAT_NODE_SIGNAL, "root" );
	// Description:
	// Adds point to the track pattern. When validating the points test is made from the start position to the end position.
	// Start position is either the pattern origin or in case the parent is provided, the parent position. The end position
	// is either relative offset from the start position or offset from the pattern origin, this is chosen based on the node flag.
	// The offset is clamped to the physical world based on the test method.
	// The points will be evaluated in the same oder they are added to the descriptor, and hte system does not try to correct the
	// evaluation order. In case hierarchies are used (parent name is defined) it is up to the pattern creator to make sure the
	// nodes are created in such order that the parent is added before it is referenced.
	// Arguments:
	// nodeName - name of the new point, the point names are local to the pattern that is currently being specified.
	// offsetx, offsety, offsetz - The offset from the start position or from the pattern center, see AITRACKPAT_NODE_ABSOLUTE.
	//		If zero offset is used, the node will become an alias, that is it will not be validated and the parent position and deformation value is used directly.
	// flags - Defines the node evaluation flags, the flags are as follows and can be combined:
	//		AITRACKPAT_NODE_START - If this flag is set, this node can be used as the first node in the pattern. There can be multiple start nodes. In that case the closest one is chosen.
	//		AITRACKPAT_NODE_ABSOLUTE - If this flag is set, the offset is interpret as an offset from the pattern center, otherwise the offset is offset from the start position.
	//		AITRACKPAT_NODE_SIGNAL - If this flag is set, a signal "OnReachedTrackPatternNode" will be send when the node is reached.
	//		AITRACKPAT_NODE_STOP - If this flag is set, the advancing will be stopped, it can be continue by calling entity:ChangeAIParameter( AIPARAM_TRACKPATTERN_ADVANCE, 1 ).
	//		AITRACKPAT_NODE_DIRBRANCH - The default direction at each pattern node is direction from the node position to the center of the pattern
	//																		If this flag is set, the direction will be average direction to the branch nodes.
	// parent (optional) - If this parameter is set, the start position is considered to be the parent node position instead of the pattern center.
	// signalValue (optional) - If the signal flag is set, this value will be passed as signal parameter, it is accessible from the signal handler in data.iValue.
	int AddPatternNode(IFunctionHandler * pH);

	// <title AddPatternBranch>
	// Syntax: AI.AddPatternBranch( nodeName, method, branchNode1, branchNode2, ..., branchNodeN  )
	// Example: AI.AddPatternBranch( "point1", AITRACKPAT_CHOOSE_ALWAYS, "point2" );
	// Description:
	// Creates a branch pattern at the specified node. When the entity has approached the specified node (nodeName),
	// and it is time to choose a new point, the rules defined by this function will be used to select the new point.
	// This function allows to associate multiple target points and an evaluation rule.
	// Arguments:
	// nodeName - name of the node to add the branches.
	// method - The method to choose the next node when the node is reached. Should be one of the following:
	//		AITRACKPAT_CHOOSE_ALWAYS - Chooses on node from the list in linear sequence.
	//		AITRACKPAT_CHOOSE_LESS_DEFORMED - Chooses the least deformed point in the list. Each node is associated with a deformation value
	//			(percentage) which describes how much it was required to move in order to stay within the physical world. These deformation values
	//			are summed down to the parent nodes so that deformation at the end of the hierarchy will be caught down the hierarchy.
	//		AITRACKPAT_CHOOSE_RANDOM - Chooses one point in the list randomly.
	int AddPatternBranch(IFunctionHandler * pH);

	// <title EndTrackPattern>
	// Syntax: AI.EndTrackPattern()
	// Description:
	// Finalizes the track pattern definition. This function should always called to finalize the pattern.
	// Failing to do so, will cause erratic behavior.
	int EndTrackPattern(IFunctionHandler * pH);

	// <title ChangeFormation>
	// Syntax: AI.ChangeFormation(entityId, name [,scale] )
	// Description:
	// Changes the formation descriptor for the current formation of given entity's group (if there is a formation)
	// Arguments:
	// entityId - entity id of which group the formation is changed
	// name - name of the formation descriptor
	// scale (optional) - scale factor (1 = default)
	// Returns:
	// true if the formation change was successful
	int ChangeFormation(IFunctionHandler *pH);

	// <title ScaleFormation>
	// Syntax: AI.ScaleFormation(entityId,scale )
	// Description:
	// changes the scale factor of the given entity's formation (if there is)
	// Arguments:
	// entityId - entity id of which group the formation is scaled
	// scale - scale factor 
	// Returns:
	// true if the formation scaling was successful
	int ScaleFormation(IFunctionHandler *pH);

	// <title SetFormationUpdate>
	// Syntax: AI.SetFormationUpdate(entityId,update )
	// Description:
	// changes the update flag of the given entity's formation (if there is) -
	// the formation is no more updated if the flag is false
	// Arguments:
	// entityId - entity id of which group the formation is scaled
	// update - update flag (true/false) 
	// Returns:
	// true if the formation update setting was successful
	int SetFormationUpdate(IFunctionHandler *pH);

	// <title SetFormationUpdateSight>
	// Syntax: AI.SetFormationUpdateSight(entityId, range, minTime, maxTime )
	// Description:
	// sets a random angle rotation for the given entity's formation sight directions
	// Arguments:
	// entityId - entity id owner of the formation 
	// range - angle (0,360) of rotation around the default sight direction
	// minTime (optional) - minimum timespan for changing the direction (default = 2)
	// maxTime (optional) - minimum timespan for changing the direction (default = minTime)

	int SetFormationUpdateSight(IFunctionHandler *pH);

	////////////////////////////////////////////////////////////////////
	/// Navigation/pathfind related functions
	////////////////////////////////////////////////////////////////////

	// <title GetNavigationType>
	// Syntax: AI.GetNavigationType(entityId)
	// Description: returns the navigation type value at the specified entity's position, given the entity navigation properties
	// Arguments:
	// entityId - AI's entity id
	// Returns:
	// navigation type at the entity's position (NAV_TRIANGULAR,NAV_WAYPOINT_HUMAN,NAV_ROAD,NAV_VOLUME,NAV_WAYPOINT_3DSURFACE,
	//	NAV_FLIGHT,NAV_SMARTOBJECT) see IAISystem::ENavigationType definition 

	int GetNavigationType(IFunctionHandler * pH);

	// <title GetDistanceAlongPath>
	// Syntax: AI.GetDistanceAlongPath(entityId1,entityid2)
	// Description: returns the distance between entity1 and entity2, along entity1's path
	// Arguments:
	// entityId1 - AI's entity1 id
	// entityId2 - AI's entity2 id
	// Returns:
	// distance along entity1 path; distance value would be negative if the entity2 is ahead along the path

	int GetDistanceAlongPath(IFunctionHandler * pH);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	// tank/warrior related functions 
	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <title IsPointInForbiddenRegion>
	// Syntax: AI.IsPointInFlightRegion(start,end)
	// Description: check if the line is in a Forbidden Region
	// Arguments:
	// start: a vector in format {x,y,z}
	// end: a vector in format {x,y,z}
	// Returns:
	// intersected position or end( if there is no intersection )
	int IntersectsForbidden(IFunctionHandler * pH);

	// NOTE Oct 9, 2009: <pvl> demand-loading layered navigation meshes
	int LoadNavMesh(IFunctionHandler * pH);
	int UnloadNavMesh(IFunctionHandler * pH);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Helicopter/VTOL related functions 
	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Helicopter combat, should be merged with GetAlienApproachParams
	int	GetHeliAdvancePoint(IFunctionHandler * pH);
	int GetFlyingVehicleFlockingPos(IFunctionHandler * pH);
	int CheckVehicleColision(IFunctionHandler * pH);
	int IsFlightSpaceVoid(IFunctionHandler *pH);
	int IsFlightSpaceVoidByRadius(IFunctionHandler *pH);
	int SetForcedNavigation(IFunctionHandler * pH);
	int SetAdjustPath(IFunctionHandler * pH);

	// <title IsPointInFlightRegion>
	// Syntax: AI.IsPointInFlightRegion(point)
	// Description: check if the point is in the Flight Region
	// Arguments:
	// point: a vector in format {x,y,z}
	// Returns:
	// true - the point is in the Flight Region
	int IsPointInFlightRegion(IFunctionHandler * pH);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Boat related functions 
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// <title IsPointInWaterRegion>
	// Syntax: AI.IsPointInFlightRegion(point)
	// Description: check if the point is in the Flight Region
	// Arguments:
	// point: a vector in format {x,y,z}
	// Returns:
	// the water level - ground level.
	// if >0 means there is water.
	int IsPointInWaterRegion(IFunctionHandler * pH);

	////////////////////////////////////////////////////////////////////
	/// Miscellaneous
	////////////////////////////////////////////////////////////////////

	// <title SetFireMode>
	// Syntax: AI.SetFireMode(entityId,mode)
	// Description: immediately sets firemode
	// Arguments: 
	// entityId - AI's entity id
	// firemode - new firemode
	int SetFireMode(IFunctionHandler *pH);

	// <title SetMemoryFireType>
	// Syntax: AI.SetMemoryFireType(entityId,type)
	// Description: Sets how the puppet handles firing at its memory target
	// Arguments: 
	// entityId - AI's entity id
	// type - Memory fire type (see EMemoryFireType)
	int SetMemoryFireType(IFunctionHandler *pH);

	// <title GetMemoryFireType>
	// Syntax: AI.GetMemoryFireType(entityId)
	// Description: Returns how the puppet handles firing at its memory target
	// Arguments: 
	// entityId - AI's entity id
	// Returns:
	// Memory fire type (see EMemoryFireType)
	int GetMemoryFireType(IFunctionHandler *pH);

	// <title ThrowGrenade>
	// Syntax: AI.ThrowGrenade(entityId,grenadeType,regTargetType)
	// Description: throws a specified grenade at target type without interrupting fire mode
	// Arguments: 
	// entityId - AI's entity id
	// grenadeType - Requested grenade type (see ERequestedGrenadeType)
	// regTargetType - Who to throw at (see AI_REG_*)
	int ThrowGrenade(IFunctionHandler *pH);

	// <title EnableCoverFire>
	// Syntax: AI.EnableCoverFire(entityId,enable)
	// Description: enables/disables fire when the FIREMODE_COVER is selected
	// Arguments:
	// entityId - AI's entity id
	// enable - boolean 
	int EnableCoverFire(IFunctionHandler *pH);

	// <title EnableFire>
	// Syntax: AI.EnableFire(entityId,enable)
	// Description: enables/disables fire
	// Arguments:
	// entityId - AI's entity id
	// enable - boolean 
	int EnableFire(IFunctionHandler *pH);

	// <title IsFireEnabled>
	// Syntax: AI.IsFireEnabled(entityId)
	// Description: checks if ai is allowed to fire or not
	// Arguments:
	// entityId - AI's entity id
	// Returns:
	// wether AI is enabled to fire or not
	int IsFireEnabled(IFunctionHandler *pH);

	// <title CanFireInStance>
	// Syntax: AI.CanFireInStance(entityId,stance)
	// Description: Returns if AI can fire at his target in the given stance at his current position
	// Arguments:
	// entityId - AI's entity id
	// stance - Stance Id (STANCE_*)
	int CanFireInStance(IFunctionHandler *pH);

	// Assign AgentPathfindingProperties to the given path type
	//
	int  AssignPFPropertiesToPathType(IFunctionHandler *pH);
	int AssignPathTypeToSOUser(IFunctionHandler *pH);

	//	sets agent's pathfinder properties, (normal, road, cover, ....)
	//
	int SetPFProperties(IFunctionHandler *pH);

	// <title SetPFBlockerRadius>
	// Syntax: AI.SetPFBlocker(entityId, blocker, radius)
	// Description: 
	// Arguments:
	// entityId - AI's entity id
	int SetPFBlockerRadius(IFunctionHandler * pH); 

	int	SetRefPointToGrenadeAvoidTarget(IFunctionHandler * pH);

	// <title IsAgentInTargetFOV>
	// Syntax: AI.IsAgentInTargetFOV(entityId, fov)
	// Description: Checks if the entity is in the FOV of the attention target.
	// Arguments:
	// entityId - AI's entity id.
	// fov - FOV of the enemy in degrees.
	// Returns: true if in the FOV of the attention target else false.
	int	IsAgentInTargetFOV(IFunctionHandler * pH); 

  // <title AgentLookAtPos>
  // Syntax: AI.AgentLookAtPos(entityId, pos)
  // Description: Makes the entityId look at certain position
  // Arguments:
  // entityId - AI's entity id.
  // fov - vec3 to look at
  // Returns:
  // 
  int	AgentLookAtPos(IFunctionHandler * pH); 

  // <title ResetAgentLookAtPos>
  // Syntax: AI.ResetAgentLookAtPos(entityId)
  // Description: Makes the entityId resets a previous call to AgentLookAtPos()
  // Arguments:
  // entityId - AI's entity id.
  // Returns:
  // 
  int	ResetAgentLookAtPos(IFunctionHandler * pH); 

  // <title IsAgentInAgentFOV>
  // Syntax: AI.IsAgentInAgentFOV(entityId, entityId2)
  // Description: Check if the entity2 is within the entity FOV
  // Arguments:
  // entityId - AI's entity who's FOV is checked
  // entityId2 - that's entity is looking for ;)
  // Returns:
  // 1st value - TRUE if the agent is within the entity FOV
  // 2nd value - TRUE if the agent is within the entity's primary FOV or FALSE if within secondry FOV
  int IsAgentInAgentFOV(IFunctionHandler *pH);

  // <title CreateGroupFormation>
  // Syntax: AI.CreateGroupFormation(entityId, leaderId)
  // Description: Creates a group formation with leader (or updates leader)
  // Arguments:
  // entityId - AI's entity
  // leaderId - New leader
  // Returns:
  //
  int CreateGroupFormation(IFunctionHandler *pH);

  // <title SetFormationPosition>
  // Syntax: AI.SetFormationPosition(entityId, v2RelativePosition )
  // Description: Sets the Relative position inside the formation
  // Arguments:
  // entityId - AI's entity
  // v2RelativePosition - Table with format {x,y} storing the new relative position
  // Returns:
  //
  int SetFormationPosition(IFunctionHandler *pH);

  // <title SetFormationLookingPoint>
  // Syntax: AI.SetFormationLookingPoint(entityId, v3RelativePosition )
  // Description: Sets the Relative looking point position inside the formation
  // Arguments:
  // entityId - AI's entity
  // v3RelativePosition - Table with format {x,y,z} storing the new relative looking point
  // Returns:
  //
  int SetFormationLookingPoint(IFunctionHandler *pH);

  // <title SetFormationAngleThreshold>
  // Syntax: AI.SetFormationAngleThreshold(entityId, fAngleThreshold )
  // Description: Sets the Relative position inside the formation
  // Arguments:
  // entityId - AI's entity
  // fAngleThreshold - New Leader orientation angle threshold in degrees to recal position
  // Returns:
  //
  int SetFormationAngleThreshold(IFunctionHandler *pH);

  // <title GetFormationPosition>
  // Syntax: AI.GetFormationPosition(entityId)
  // Description: Gets the Relative position inside the formation
  // Arguments:
  // entityId - AI's entity
  // Returns:
  // v3 - Table with format {x,y,z} storing the relative position
  //
  int GetFormationPosition(IFunctionHandler *pH);

  // <title GetFormationLookingPoint>
  // Syntax: AI.GetFormationLookingPoint(entityId)
  // Description: Gets the looking point position inside the formation
  // Arguments:
  // entityId - AI's entity
  // Returns:
  // v3 - Table with format {x,y,z} storing the looking point position
  //
  int GetFormationLookingPoint(IFunctionHandler *pH);

  int AutoDisable(IFunctionHandler *pH);
  
	//	Creates new combat class
	int AddCombatClass(IFunctionHandler *pH);

	int Animation(IFunctionHandler * pH);

	//int GetClosestPointToOBB(IFunctionHandler *pH);

	// Syntax: AI.GetStance(entityId)
	// Description: get the given entity's stance
	// Arguments:
	// entityId - AI's entity id
	// Return: 
	// entity stance (STANCE_*)
	int GetStance(IFunctionHandler *pH);

	// Syntax: AI.SetStance(entityId,stance)
	// Description: set the given entity's stance
	// Arguments:
	// entityId - AI's entity id
	// stance - stance value (STANCE_*)
	int SetStance(IFunctionHandler *pH);

	// Syntax: AI.SetPostures(entityId, postures)
	// Description: Set the given entity's postures
	int SetPostures(IFunctionHandler *pH, ScriptHandle entityId, SmartScriptTable postures);

	// Syntax: AI.SetPosturePriority(entityId, postureName, priority)
	// Description: Set the given entity's posture priority
	int SetPosturePriority(IFunctionHandler *pH, ScriptHandle entityId, const char* postureName, float priority);

	// Syntax: AI.GetPosturePriority(entityId, postureName)
	// Description: Set the given entity's posture priority
	int GetPosturePriority(IFunctionHandler *pH, ScriptHandle entityId, const char* postureName, float priority);

	// Syntax: AI.IsMoving(entityId)
	// Description: Returns true if the agent desires to move.
	// Arguments:
	// entityId - AI's entity id
	int IsMoving(IFunctionHandler *pH);

	// Syntax: AI.RegisterDamageRegion(entityId, radius)
	// Description: Register a spherical region that causes damage (so should be avoided in pathfinding). 
	// Owner entity position is used as region center. Can be called multiple times, will just move 
	// update region position
	// Arguments:
	// entityId - owner entity id.
	// radius - If radius <= 0 then the region is disabled
	int RegisterDamageRegion(IFunctionHandler *pH);

  	// <title GetBiasedDirection>
  	// Syntax: AI.GetBiasedDirection(entityId)
  	// Description: Get biased direction of certain point
  	// Arguments:
  	// entityId - AI's entity
  	// Returns:
  	int GetBiasedDirection(IFunctionHandler* pH);

	// <title SetAttentiontarget>
	// Syntax: AI.SetAttentiontarget(entityId, targetId)
	// Description: Set a new attention target
	// Arguments:
	// Returns:
	int SetAttentiontarget(IFunctionHandler* pH);

	// AI.FindStandbySpotInShape(centerPos, targetPos, anchorType);
	int FindStandbySpotInShape(IFunctionHandler *pH);

	// AI.FindStandbySpotInShape(centerPos, targetPos, anchorType);
	int FindStandbySpotInSphere(IFunctionHandler *pH);

	// <title CanMelee>
	// Syntax: AI.CanMelee(entityId)
	// Description: returns 1 if the AI is able to do melee attack.
	// Arguments:
	// entityId - AI's entity id
	int CanMelee(IFunctionHandler * pH);

	// <title CheckMeleeDamage>
	// Syntax: AI.CheckMeleeDamage(entityId,targetId,radius,minheight,maxheight,angle)
	// Description: returns 1 if the AI performing melee is actually hitting target.
	// Arguments:
	// entityId - AI's entity id
	// targetId - AI's target entity id
	// radius - max distance in 2d to target
	// minheight - min distance in height
	// maxheight - max distance in height
	// angle - FOV to include target
	// Returns: (distance,angle) pair between entity and target (degrees) if melee is possible, nil otherwise
	int CheckMeleeDamage(IFunctionHandler * pH);

	// <title GetDirLabelToPoint>
	// Syntax: AI.GetDirLabelToPoint(entityId, point)
	// Description: Returns a direction label (front=0, back=1, left=2, right_3, above=4, -1=invalid) to the specified point.
	// Arguments:
	//	entityId - AI's entity id
	//	point - point to evaluate.
	int GetDirLabelToPoint(IFunctionHandler *pH);

	// <title DebugReportHitDamage>
	// Syntax: AI.DebugReportHitDamage(pVictimEntity, pShooterEntity)
	// Arguments:
	//		pVictimEntity	- Victim ID.
	//		pShooterEntity	- Shooter ID.
	// Description:
	//		Creates a debug report for the hit damage.
	int DebugReportHitDamage(IFunctionHandler *pH);

	// <title ProcessBalancedDamage>
	// Syntax: AI.ProcessBalancedDamage(pShooterEntity, pTargetEntity, damage, damageType)
	// Arguments:
	//		pShooterEntity	- Shooter ID.
	//		pTargetEntity	- Target ID.
	//		damage			- Hit damage.
	//		damageType		- Hit damage type.
	// Description:
	//		Processes balanced damage.
	int ProcessBalancedDamage(IFunctionHandler *pH);

	// <title SetRefpointToAnchor>
	// Syntax: AI.SetRefpointToAnchor(entityId,rangeMin,rangeMax,findType,findMethod)
	// Arguments:
	//		entityId	- AI's entity ID.
	//		rangeMin	- Minimum range.
	//		rangeMax	- Maximum range.
	//		findType	- Finding type.
	//		findMethod	- Finding method.
	// Description:
	//		Sets a reference point to an anchor.
	int SetRefpointToAnchor(IFunctionHandler *pH);

	// <title SetRefpointToPunchableObject>
	// Syntax: AI.SetRefpointToPunchableObject(entityId,range)
	// Arguments:
	//		entityId	- AI's entity ID.
	//		range		- Range for the punchable object.
	//	Description:
	//		Sets the reference point to the punchable object.
	int SetRefpointToPunchableObject(IFunctionHandler *pH);
	// <title MeleePunchableObject>
	// Syntax: AI.MeleePunchableObject(entityId,objectId,origPos)
	// Arguments:
	//		entityId - AI's entity ID.
	//		objectId - Object ID.
	//		origPos	 - Position of the melee punchable object
	int MeleePunchableObject(IFunctionHandler *pH);
	// <title IsPunchableObjectValid>
	// Syntax: AI.IsPunchableObjectValid(userId,objectId,origPos)
	// Arguments:
	//		userId		- User ID.
	//		objectId	- Object ID.
	//		origPos		- Object position in the world.
	// Description:
	//		Checks if a punchable object is valid.
	int IsPunchableObjectValid(IFunctionHandler *pH);

	// <title PlayReadabilitySound>
	// Syntax: AI.PlayReadabilitySound(entityId, soundName)
	// Description: Plays readability sound on the AI agent.
	// This call does not do any filtering like playing readability using signals.
	// Arguments:
	//	entityId - AI's entity id
	//	soundName - the name of the readability sound signal to play
	//	stopPreviousSounds (Optional) - TRUE if any currently playing readability should be stopped in favor of this one
	//	responseDelayMin (Optional) - Minimum (or exact, if no maximum) delay for the Response readability to play
	//	repsonseDelayMax (Optional) - Maximum delay for the Response readability to play
	int PlayReadabilitySound(IFunctionHandler *pH);


	// <title PlayCommunication>
	// Syntax: AI.PlayCommunication(entityId, commName, channelName[, targetId] [, targetPos])
	// Description: Plays communication on the AI agent.
	// Arguments:
	//	entityId - AI's entity id
	//	commName - The name of the communication to play
	//	channelName - The name of the channel where the communication will play
	int PlayCommunication(IFunctionHandler *pH, ScriptHandle entityId, const char* commName, const char* channelName,
		float contextExpirity);


	// <title GetEntityInRange>
	// Syntax: AI.GetEntityInRange(entityId, filter, minDistance, maxDistance)
	// Description: Retrieves a closes entity in a given range respecting a given filter
	// Arguments:
	//	entityId - AI's source entity id
	//	filter - filter applied
	//	minDistance - minimal distance from source entity to a searched one
	//	maxDistance - maximum distance from source entity to a searched one
	int GetEntityInRange(IFunctionHandler *pH);

	// <title EnableWeaponAccessory>
	// Syntax: AI.EnableWeaponAccessory(entityId, accessory, state)
	// Description: Enables or disables certain weapon accessory usage.
	// Arguments:
	//	entityId - AI's entity id
	//	accessory - enum of the accessory to enable (see EAIWeaponAccessories)
	//  state - true/false to enable/disable
	int EnableWeaponAccessory(IFunctionHandler *pH);

	// AIHandler calls this function to replace the OnPlayerSeen signal
/*	static const char* GetCustomOnSeenSignal( int iCombatClass )
	{
		if ( iCombatClass < 0 || iCombatClass >= m_CustomOnSeenSignals.size() )
			return "OnPlayerSeen";
		const char* result = m_CustomOnSeenSignals[ iCombatClass ];
		return *result ? result : "OnPlayerSeen";
	}*/

	// <title RegisterInterestingEntity>
	// Syntax: AI.RegisterInterestingEntity(entityId, baseInterest, category, aiAction)
	// Description: Registers the entity with the interest system
	// Any errors go to error log
	// Arguments: 
	// entityId - AI's entity
	// Returns:
	// true - if a valid update was performed
	// nil - if not (Interest system is disabled, parameters not valid, etc)
	int RegisterInterestingEntity(IFunctionHandler* pH, ScriptHandle entityId, float radius, float baseInterest, const char* actionName);

	// <title UnregisterInterestingEntity>
	// Syntax: AI.UnregisterInterestingEntity(entityId)
	// Description: Unregisters the entity with the interest system
	// Any errors go to error log
	// Arguments: 
	// entityId - AI's entity
	int UnregisterInterestingEntity(IFunctionHandler* pH, ScriptHandle entityId);

	// <title RegisterInterestingEntity>
	// Syntax: AI.RegisterInterestedActor(entityId, baseInterest, category, aiAction)
	// Description: Registers the interested actor with the interest system
	// Any errors go to error log
	// Arguments: 
	// entityId - AI's entity
	// Returns:
	// true - if a valid update was performed
	// nil - if not (Interest system is disabled, parameters not valid, etc)
	int RegisterInterestedActor(IFunctionHandler* pH, ScriptHandle entityId, float interestFilter, float angle );

	// <title UnregisterInterestingEntity>
	// Syntax: AI.UnregisterInterestedActor(entityId)
	// Description: Unregisters the entity with the interest system
	// Any errors go to error log
	// Arguments: 
	// entityId - AI's entity
	int UnregisterInterestedActor(IFunctionHandler* pH, ScriptHandle entityId);

	// <title IsCoverCompromised>
	// Syntax: AI.IsCoverCompromised(entityId)
	// Description: 
	// Arguments: 
	// entityId - AI's entity
	// Returns:
	// true - Cover is not good anymore
	// nil - if not 
	int IsCoverCompromised(IFunctionHandler *pH);

	// <title IsTakingCover>
	// Syntax: AI.IsTakingCover(entityId, [distanceThreshold])
	// Description: 
	// Arguments: 
	// entityId - AI's entity
	// distanceThreshold - distance over which if the agent is running to cover, he won't be considered as taking cover
	// Returns:
	// true - Agent is either in cover or running to cover
	// nil - if not 
	int IsTakingCover(IFunctionHandler *pH);


	// <title IsMovingToCover>
	// Syntax: AI.IsMovingToCover(entityId)
	// Description: 
	// Arguments: 
	// entityId - AI's entity
	// Returns:
	// true - Agent is running to cover
	// nil - if not 
	int IsMovingToCover(IFunctionHandler *pH);

	// <title GetCoverInfo>
	// Syntax: AI.GetCoverInfo(entityId, usage, weaponOffset)
	// Description: 
	// Arguments: 
	// entityId - AI's entity
	// usage - retrieve usage info?
	// weaponOffset - if usage is true, then use this as the characters weapon offset
	// Returns:
	// true - cover info successfully retrieve.
	// nil - if not 
	int GetCoverInfo(IFunctionHandler* pH);

	// <title IsInCover>
	// Syntax: AI.IsInCover(entityId)
	// Returns:
	// true - if AI is using cover
	// nil - if not 
	int IsInCover(IFunctionHandler* pH);

	// <title SetInCover>
	// Syntax: AI.SetInCover(entityId, inCover)
	// Arguments: 
	// entityId - AI's entity
	// inCover - if the AI should be set to be in cover or not
	int SetInCover(IFunctionHandler* pH);

	// <title IsOutOfAmmo>
	// Syntax: AI.IsOutOfAmmo(entityId)
	// Description: 
	// Arguments: 
	// entityId - AI's entity
	// Returns:
	// true - Entity is out of ammo
	// nil - if not 
	int IsOutOfAmmo(IFunctionHandler *pH);

	// <title IsLowOnAmmo>
	// Syntax: AI.IsLowOnAmmo(entityId)
	// Description: 
	// Arguments: 
	// entityId - AI's entity
	// threshold - the ammo percentage threshold
	int IsLowOnAmmo(IFunctionHandler *pH);

	// <title ResetAgentState>
	// Syntax: AI.ResetAgentState(entityId,stateLabel)
	// Description: Resets a particular aspect of the agent's state, such as "lean"
	//              Intended to keep these workaround concepts together.
	// Arguments: 
	// entityId - AI's entity
	// stateLabel - a string describing the state that must be reset to default
	// Returns: nil
	int ResetAgentState(IFunctionHandler *pH, ScriptHandle entityId, const char * stateLabel);


	// <title RegisterTacticalPointQuery>
	// Syntax: AI.RegisterTacticalPointQuery( querySpecTable )
	// Description: Get a query ID for the given tactical point query
	// Arguments: 
	// querySpecTable - table specifying the query (a mini-language - see Tactical Point System docs elsewhere)
	// Returns:
	// > 0 - If the query was parsed successfully
	// 0 - Otherwise
	int RegisterTacticalPointQuery(IFunctionHandler *pH);


	// <title GetTacticalPoints>
	// Syntax: AI.GetTacticalPoints( entityId, tacPointSpec, point )
	// Description: Get a point matching a description, related to an entity.
	//              Format of a point is: { x,y,z }
	// Arguments: 
	// entityId - AI's entity
	// tacPointSpec - table specifying the points required
	// point - a table put coordinates of point found
	// Returns:
	// true - if a valid point was found
	// false - Otherwise
	int GetTacticalPoints(IFunctionHandler *pH);

	// <title GetObjectBlackBoard>
	// Syntax: AI.GetObjectBlackBoard( entity )
	// Description: retrieves given object's black board (a lua table)
	// Arguments: 
	// entityId or entityName - some kind of AI entity's identifier	
	// Returns:
	// black board - if there was one
	// nil - Otherwise
	int GetObjectBlackBoard(IFunctionHandler *pH);

	// <title GetObjectBlackBoard>
	// Syntax: AI.GetBehaviorBlackBoard( entity )
	// Description: retrieves given AIActor current behavior's black board (a lua table)
	// Arguments: 
	// entityId or entityName - some kind of AIActors's identifier	
	// Returns:
	// black board - if there was one
	// nil - Otherwise
	int GetBehaviorBlackBoard(IFunctionHandler *pH);

	// <title SetEnableBehaviorTree>
	// Syntax: AI.SetEnableBehaviorTree( entityId, bEnable )
	// Description: Enables/disables (depending on bEnable value) BehaviorTree for given entity
	// Arguments: 
	// entityId - AIActors's identifier		
	// bEnable - controls if BT will be enabled or disabled as a result of this function
	int SetEnableBehaviorTree(IFunctionHandler *pH);

	// <title GetNextNodeRequested>
	// Syntax: AI.GetNextNodeRequested( entity )
	// Description: Returns next node requested by BT
	// Arguments: 
	// entityId or entityName - some kind of AIActors's identifier	
	// Returns:
	// next node - if requested
	// nil - Otherwise
	int GetNextNodeRequested(IFunctionHandler *pH);

	// <title SetReadyForNodeChange>
	// Syntax: AI.SetReadyForNodeChange( entity )
	// Description: Sets entity to be ready for a node change
	// Arguments: 
	// entityId or entityName - some kind of AIActors's identifier	
	// Returns: nil
	int SetReadyForNodeChange(IFunctionHandler *pH);

	// <title SetBehaviorTreeProfile>
	// Syntax: AI.SetBehaviorTreeProfile( entity, profile )
	// Description: retrieves given AIActor current behavior's black board (a lua table)
	// Arguments: 
	// entityId or entityName - some kind of AIActors's identifier	
	// profile - Name of BT
	// Returns:
	// true/false if profile was set
	int SetBehaviorTreeProfile(IFunctionHandler *pH);

	// <title SetBehaviorTreeProfile>
	// Syntax: AI.SetBehaviorVariable( entity, variableName, value )
	// Description: Sets a behavior variable for the specified actor
	int SetBehaviorVariable(IFunctionHandler *pH, ScriptHandle entityId, const char* variableName, bool value);

	// <title SetBehaviorTreeProfile>
	// Syntax: AI.SetBehaviorVariable( entity, variableName, value )
	// Description: Returns a behavior variable for the specified actor
	int GetBehaviorVariable(IFunctionHandler *pH, ScriptHandle entityId, const char* variableName);

	// <title ReloadBehaviorTreeProfile>
	// Syntax: AI.ReloadBehaviorTreeProfile( entity )
	// Description: Reloads given AIActor's behavior tree to its initial state
	// Arguments:
	// entityId or entityName - some kind of AIActors's identifier	
	// Returns:
	// true/false if tree was reset
	int ReloadBehaviorTreeProfile(IFunctionHandler *pH);

	// <title GetBTTacticUsers>
	// Syntax: AI.GetBTTacticUsers( entity, tactic )
	// Description: Gets vector of users used by BT in this tactic
	// Arguments: 
	// entityId or entityName - some kind of AIActors's identifier	
	// tactic - Name of the tactic
	// Returns:
	// vector of user tables or nil
	int GetBTTacticUsers(IFunctionHandler *pH);

	// <title SendBTGlobalSignal>
	// Syntax: AI.SendBTGlobalSignal( signal, data )
	// Description: Send a signal to the BT global activation conditions
	// Arguments: 
	// signal - Signal to send
	// data - Signal extra data
	int SendBTGlobalSignal(IFunctionHandler *pH);

	// <title GenerateUniqueTacticGroupId>
	// Syntax: AI.GenerateUniqueTacticGroupId( )
	// Description: Returns a unique tactic group Id to use
	int GenerateUniqueTacticGroupId(IFunctionHandler *pH);

	// <title GetEntityTacticGroupId>
	// Syntax: AI.GetEntityTacticGroupId( entity )
	// Description: Gets which tactic group Id an entity belongs to
	// Arguments: 
	// entityId or entityName - some kind of AIActors's identifier
	// Return:
	// The tactic group Id or nil on error
	int GetEntityTacticGroupId(IFunctionHandler *pH);

	// <title SetEntityTacticGroupId>
	// Syntax: AI.SetEntityTacticGroupId( entity, groupId )
	// Description: Sets which tactic group Id an entity belongs to
	// Arguments: 
	// entityId or entityName - some kind of AIActors's identifier
	// groupId - The Tactic group Id to be assigned
	int SetEntityTacticGroupId(IFunctionHandler *pH);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Cooperative Readability System
	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	// <title RegisterForReadabilitySet>
	// Syntax: AI.RegisterForReadabilitySet( sReadabilitySetName, entity )
	// Description: 
	// Arguments: 
	// entityId 
	// sReadabilitySetName
	int RegisterForReadabilitySet(IFunctionHandler *pH);

	// <title UnregisterFromReadabilitySet>
	// Syntax: AI.UnregisterFromReadabilitySet( sReadabilitySetName, entity )
	// Description: 
	// Arguments: 
	// entityId 
	// sReadabilitySetName
	int UnregisterFromReadabilitySet(IFunctionHandler *pH);

	// <title PlayReadabilityFromSet>
	// Syntax: AI.PlayReadabilityFromSet( sReadabilitySetName, entity, bStep == false )
	// Description: 
	// Arguments: 
	// sReadabilitySetName
	// entity	- optional
	// bStep - optional
	// Return:
	// 0 if failed to start readability, otherwise play session id (or 1 if no entity was specified)
	int PlayReadabilityFromSet(IFunctionHandler *pH);

	// <title PlayReadabilityNextStep>
	// Syntax: AI.PlayReadabilityFromSet( nSessionId )
	// Description: 
	// Arguments: 
	// nSessionId
	// Return:
	// true if succeeded and false otherwise
	int PlayReadabilityNextStep(IFunctionHandler *pH);

	// <title ParseTables>
	// Syntax: AI.ParseTables( firstTable,parseMovementAbility,pH,aiParams,updateAlways )
	// Arguments:
	//		firstTable				- Properties table.
	//		parseMovementAbility	- True to parse movement ability, false otherwise.
	//		aiParams				- AI parameters.
	//		updateAlways			- .
	bool ParseTables(int firstTable, bool parseMovementAbility, IFunctionHandler* pH,AIObjectParameters& aiParams, bool& updateAlways);


	// <title SetEntitySpeedRange>
	// Syntax: AI.SetEntitySpeedRange( userEntityId, urgency, defaultSpeed, minSpeed, maxSpeed, stance = all)
	// Description: This function allows the user to override the entity's speed range for the given urgency
	// Arguments:
	// usedEntityId - entity id of the user for which its last used smart object is needed
	// urgency			- integer value specifying the movement urgency (see AgentMovementSpeeds::EAgentMovementUrgency)
	// defaultSpeed	- floating point value specifying the default speed
	// minSpeed			- floating point value specifying the min speed
	// maxSpeed			- floating point value specifying the max speed
	// stance				- optional parameter specifying the stance for which the range is set. default is all (see AgentMovementSpeeds::EAgentMovementStance)
	// Returns:
	// true if the operation was successful and false otherwise
	int		SetEntitySpeedRange(IFunctionHandler *pH);


	// <title SetAlarmed>
	// Syntax: AI.SetAlarmed( entityId )
	// Description: This function sets the entity to be "perception alarmed"
	int SetAlarmed(IFunctionHandler *pH);

	// <title LoadBehaviours>
	// Syntax: AI.LoadBehaviours( folderName, tbl)
	int LoadBehaviours(IFunctionHandler *pH, const char *folderName, SmartScriptTable tbl);
	// <title LoadCharacters>
	// Syntax: AI.LoadCharacters( folderName, tbl)
	int LoadCharacters(IFunctionHandler *pH, const char *folderName, SmartScriptTable tbl);

	// <title StartCoordinationWithMembersOfGroup>
	// Syntax: AI.StartCoordinationWithMembersOfGroup( groupID, coordinationName )
	int StartCoordinationWithMembersOfGroup(IFunctionHandler* pH, int groupID, const char* coordinationName);

	// <title StopCoordination>
	// Syntax: AI.StopCoordination( coordinationID )
	int StopCoordination(IFunctionHandler* pH, ScriptHandle coordinationID);

	// <title SetCoordinationInput>
	// Syntax: AI.SetCoordinationInput( coordinationID, name, value )
	int SetCoordinationInput(IFunctionHandler* pH, ScriptHandle coordinationID, const char* name);

	// <title GetCoordinationInput>
	// Syntax: AI.GetCoordinationInput( coordinationID, name )
	int GetCoordinationInput(IFunctionHandler* pH, ScriptHandle coordinationID, const char* name);

	// <title GetCoordinationEnvironment>
	// Syntax: AI.GetCoordinationEnvironment( coordinationID )
	int GetCoordinationEnvironment(IFunctionHandler* pH, ScriptHandle coordinationID);

	// <title GetCurrentCoordination>
	// Syntax: AI.GetCurrentCoordination( entityID )
	int GetCurrentCoordination(IFunctionHandler* pH, ScriptHandle entityID);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	// XML support for goal pipes
	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	int LoadGoalPipes(IFunctionHandler* pH);

private:
	void SetParameter_PerceptionExtraData(IFunctionHandler *pH, IAIObject *pAI, int nParameter);
	
	static const char* GetPathTypeName(EAIPathType pathType);

protected:

	//====================================================================
	// Fetch entity ID from script parameter
	//====================================================================
	EntityId GetEntityIdFromParam( IFunctionHandler *pH, int i );

	//====================================================================
	// Fetch entity pointer from script parameter
	//====================================================================
	IEntity * GetEntityFromParam( IFunctionHandler *pH, int i );

	void AssignPFPropertiesToPathType(const string& sPathType, AgentPathfindingProperties& properties);

	void SetPFProperties(AgentMovementAbility& moveAbility, EAIPathType   nPathType) const;
	void SetPFProperties(AgentMovementAbility& moveAbility, const string& sPathType) const;

	bool GetSignalExtraData(IFunctionHandler * pH, int iParam, IAISignalExtraData* pEData);

	int	RayWorldIntersectionWrapper(Vec3 org,Vec3 dir, int objtypes, unsigned int flags, ray_hit *hits,int nMaxHits,
		IPhysicalEntity **pSkipEnts=0,int nSkipEnts=0, void *pForeignData=0,int iForeignData=0, int iCaller=1);

	int CreateQueryFromTacticalSpec( SmartScriptTable specTable );

	typedef std::map<int,int> VTypeChart_t;
	typedef std::multimap<int,int,std::greater<int>/**/> VTypeChartSorted_t;

	bool GetGroupSpatialProperties(IAIObject* pRequester, float& offset, Vec3& avgGroupPos, Vec3& targetPos, Vec3& dirToTarget, Vec3& normToTarget);

	int ParsePostureInfo(CPuppet* pPuppet, const SmartScriptTable& posture, const IPuppet::SPostureInfo& base, int parentId);

	std::list<Vec3>		m_lstPointsInFOVHistory;

	IGoalPipe*	m_pCurrentGoalPipe;
	bool				m_IsGroupOpen;
};

#endif __ScriptBind_AI_H__
