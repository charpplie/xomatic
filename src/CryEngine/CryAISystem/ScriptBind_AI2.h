/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2008.
---------------------------------------------------------------------
File name:   ScriptBind_AI2.h
Description: Script Bind for AI
---------------------------------------------------------------------
History:
- 08:02:2008 : Created by ricardo pillosu

*********************************************************************/
#ifndef _SCRIPTBIND_AI2_H_
	#define _SCRIPTBIND_AI2_H_

	#include <IScriptSystem.h>
	#include <ScriptHelpers.h>

class CScriptBind_AI2 : public CScriptableBase
{

	public:

		/*$1- Basics -------------------------------------------------------------*/
		CScriptBind_AI2();
		~CScriptBind_AI2();
		void		Release() { delete this; };
		IEntity*	GetEntity( IScriptTable* pEntityTable );


		/*$1- General  Functions -------------------------------------------------*/

		// <title DestroyAllTPSQueries>
		// Syntax: AI2.DestroyAllTPSQueries()
		// Description:
		//		Destroys all the tactical point system queries. 
		int DestroyAllTPSQueries( IFunctionHandler* pH );

		/*$1- Group Functions ----------------------------------------------------*/

		// <title CreateGroup>
		// Syntax: AI2.CreateGroup()
		// Description:
		//		Creates a new group.
		int CreateGroup( IFunctionHandler* pH );
		// <title DestroyGroup>
		// Syntax: AI2.DestroyGroup(nGroupId)
		// Arguments:
		//		nGroupId - Group ID.
		// Description:
		//		Destroys the specified group.
		int DestroyGroup( IFunctionHandler* pH );
		// <title SetGroupLeader>
		// Syntax: AI2.SetGroupLeader(nGroupId, entityId)
		// Arguments:
		//		nGroupId - Group ID.
		//		entityId - AI's entity ID.
		// Description:
		//		Sets the specified entity as group leader.
		int SetGroupLeader( IFunctionHandler* pH );
		// <title GetGroupLeader>
		// Syntax: AI2.GetGroupLeader()
		// Description:
		//		Gets the group leader.
		int GetGroupLeader( IFunctionHandler* pH );
		// <title GetGroup>
		// Syntax: AI2.GetGroup( entityId )
		//	Arguments:
		//		entityId - AI's entity ID.
		// Description:
		//		Gets the group of the specified entity.
		int GetGroup( IFunctionHandler* pH );
		// <title SetGroup>
		// Syntax: AI2.SetGroup( entityId,groupId )
		//	Arguments:
		//		entityId - AI's entity ID.
		//		groupId	 - Group ID.
		// Description:
		//		Assigns the specified entity to the specified group.
		int SetGroup( IFunctionHandler* pH );
		// <title AddGroupMember>
		// Syntax: AI2.AddGroupMember( groupId,entityId )
		//	Arguments:
		//		groupId	 - Group ID.
		//		entityId - AI's entity ID.
		// Description:
		//		Adds a member to the specified group.
		int AddGroupMember( IFunctionHandler* pH );
		// <title GetGroupMemberCount>
		// Syntax: AI2.GetGroupMemberCount( entityId )
		//	Arguments:
		//		entityId - AI's entity ID.
		// Description:
		//		Gets the members number of the entity group.
		int GetGroupMemberCount( IFunctionHandler* pH );
		int SetGroupMemberSpeed( IFunctionHandler* pH );
		int SetGroupFormationSpec( IFunctionHandler* pH );
		int SetGroupMatchLeaderSpeed( IFunctionHandler* pH );
		// <title ClearGroupSystem>
		// Syntax: AI2.ClearGroupSystem()
		// Description:
		//		Clears group system.
		int ClearGroupSystem( IFunctionHandler* pH );
		// <title MergeGroups>
		// Syntax: AI2.MergeGroups()
		int MergeGroups( IFunctionHandler* pH );
		// <title ClearFormationDefinitions>
		// Syntax: AI2.ClearFormationDefinitions()
		// Description:
		//		Clears all the formation definitions.
		int ClearFormationDefinitions( IFunctionHandler* pH );
		// <title RegisterFormation>
		// Syntax: AI2.RegisterFormation(specsTable)
		// Arguments:
		//		specsTable - Specification table.
		// Description:
		//		Registers a formation.
		int RegisterFormation( IFunctionHandler* pH );


		/*$1- Group Functions ----------------------------------------------------*/

		// <title ActivateEmotionalSystem>
		// Syntax: AI2.ActivateEmotionalSystem(Entity, sProfile)
		// Arguments:
		//		Entity		- Entity table.
		//		sProfile	- Profile name for the emotional system.
		// Description:
		//		Activates the emotional system for the specified entity.
		int ActivateEmotionalSystem( IFunctionHandler* pH, SmartScriptTable Entity, const char* sProfile );
		// <title DeactivateEmotionalSystem>
		// Syntax: AI2.DeactivateEmotionalSystem(Entity)
		// Arguments:
		//		Entity		- Entity table.
		// Description:
		//		Deactivates the emotional system for the specified entity.
		int DeactivateEmotionalSystem( IFunctionHandler* pH, SmartScriptTable Entity );
		// <title SetEmotionalPersonality>
		// Syntax: AI2.SetEmotionalPersonality(Entity)
		// Arguments:
		//		Entity			- Entity table.
		//		fExtraversion	- Extraversion level for the entity.
		//		fStability		- Stability level for the entity.
		//		fCalm			- Calm level for the entity,
		// Description:
		//		Sets the emotional personality for the specified entity.
		int SetEmotionalPersonality( IFunctionHandler*	 pH,
																 SmartScriptTable		 Entity,
																 float							 fExtraversion,
																 float							 fStability,
																 float							 fCalm );

	private:
		void	RegisterGlobals();
		void	RegisterMethods();
};
#endif // _SCRIPTBIND_AI2_H_
