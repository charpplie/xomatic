#include "StdAfx.h"

#include "HitDeathReactionsSystem.h"

#include "Actor.h"
#include "GameCVars.h"
#include "GameRules.h"

// Unnamed namespace for constants
namespace
{
	const char REACTIONS_DATA_FILE_PROPERTY[] = "fileHitDeathReactionsParamsDataFile";
	const char ACTOR_PROPERTIES_TABLE[] = "Properties";

	const char HIT_DEATH_REACTIONS_SCRIPT_FILE[] = "Scripts/GameRules/HitDeathReactions.lua";

	const char ACTOR_HIT_DEATH_REACTIONS_PARAMS[] = "hitDeathReactionsParams";

	const char HIT_REACTIONS_PARAMS[] = "HitReactionParams";
	const char DEATH_REACTIONS_PARAMS[] = "DeathReactionParams";
	const char COLLISION_REACTIONS_PARAMS[] = "CollisionReactionParams";

	const char VALIDATION_FUNC_PROPERTY[] = "validationFunc";
	const char REACTION_FUNC_PROPERTY[] = "reactionFunc";
	const char AISIGNAL_PROPERTY[] = "AISignal";
	const char MINIMUM_SPEED_PROPERTY[] = "minimumSpeed";
	const char MAXIMUM_SPEED_PROPERTY[] = "maximumSpeed";
	const char ALLOWED_PARTS_ARRAY[] = "AllowedParts";
	const char MOVEMENT_DIRECTION_PROPERTY[] = "movementDirection";
	const char SHOT_ORIGIN_PROPERTY[] = "shotOrigin";
	const char PROBABILITY_PERCENT_PROPERTY[] = "probabilityPercent";
	const char ALLOWED_STANCES_ARRAY[] = "AllowedStances";
	const char ALLOWED_HIT_TYPES_ARRAY[] = "AllowedHitTypes";
	const char ALLOWED_PROJECTILES_ARRAY[] = "AllowedProjectiles";
	const char ALLOWED_WEAPONS_ARRAY[] = "AllowedWeapons";
	const char SNAP_ORIENTATION_ANGLE[] = "snapOrientationAngle";
	const char MINIMUM_DAMAGE_PROPERTY[] = "minimumDamage";
	const char MAXIMUM_DAMAGE_PROPERTY[] = "maximumDamage";
	const char RAGDOLL_ON_COLLISION_PROPERTY[] = "ragdollOnCollision";  // Does exactly the same that endReactionOnCollision, but is more descriptive for death reactions
	const char REACTION_ON_COLLISION_PROPERTY[] = "reactionOnCollision";
	const char PAUSE_AI_PROPERTY[] = "pauseAI";
	const char ONLY_IF_USING_MOUNTED_ITEM_PROPERTY[] = "onlyIfUsingMountedItem";
	const char DESTRUCTIBLE_EVENT_PROPERTY[] = "destructibleEvent";

	const char AG_REACTION_TABLE[] = "AnimGraphReaction";
	const char AG_INPUT_VALUE_PROPERTY[] = "inputValue";
	const char VARIATIONS_ARRAY[] = "Variations";
	const char VARIATION_NAME[] = "name";
	const char VARIATION_VALUE[] = "value";

	const char REACTION_ANIM_NAME_PROPERTY[] = "animName";
	const char REACTION_ANIM_PROPERTY[] = "ReactionAnim";
	const char REACTION_ANIM_PARTIAL_ANIM[] = "partial";
	const char REACTION_ANIM_ADDITIVE_ANIM[] = "additive";
	const char REACTION_ANIM_LAYER[] = "layer";
	const char ANIM_NAME_ARRAY[] = "AnimNames";
	const char ANIM_NAME_PROPERTY[] = "name";
	const char ANIM_VARIANTS_PROPERTY[] = "variants";
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void SReactionParams::Reset()
{
	sCustomValidationFunc.clear();
	sCustomExecutionFunc.clear();
	sCustomAISignal.clear();
	fMinimumSpeedAllowed = 0.0f;
	fMaximumSpeedAllowed = std::numeric_limits<float>::max();
	fMinimumDamageAllowed = 0.0f;
	fMaximumDamageAllowed = std::numeric_limits<float>::max();
	allowedPartIds.clear();
	shotOrigin = eCD_Invalid; 
	movementDir = eCD_Invalid;
	orientationSnapAngle = 0;
	reactionOnCollision = NO_COLLISION_REACTION;
	flags = 0;
	fProbability = 1.0f;
	allowedStances.clear();
	allowedHitTypes.clear();
	allowedProjectiles.clear();
	allowedWeapons.clear();
	bPauseAI = true;
	bAllowOnlyWhenUsingMountedItems = false;
	destructibleEvent = 0;

	agReaction.Reset();
	reactionAnim.Reset();

	reactionScriptTable = ScriptTablePtr();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void SReactionParams::SReactionAnim::Reset()
{
	animIDs.clear();
	iLayer = 0;
	animFlags = DEFAULT_REACTION_ANIM_FLAGS;
	bAdditive = false;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void SReactionParams::SAnimGraphReaction::Reset()
{
	sAGInputValue.clear();
	variations.clear();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
inline void AnimIDError(const char *animName)
{
	if (g_pGameCVars->g_animatorDebug)
	{
		static const ColorF col (1.0f, 0.0f, 0.0f, 1.0f);
		g_pGame->GetIGameFramework()->GetIPersistantDebug()->Add2DText(string().Format("Missing %s", animName).c_str(), 1.0f, col, 10.0f);
	}

	CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "[CHitDeathReactions] Missing anim: %s", animName);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactionsSystem::Warning(const char* szFormat, ...)
{
	if (!gEnv || !gEnv->pSystem || !szFormat)
		return;

	va_list	args;
	va_start(args, szFormat);
	GetISystem()->WarningV(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, 0, 0, (string("[CHitDeathReactions] ") + szFormat).c_str(), args);
	va_end(args);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CHitDeathReactionsSystem::CHitDeathReactionsSystem()
{
	// Execute scripts
	ExecuteHitDeathReactionsScripts(false);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactionsSystem::GetReactionParamsForActor(const CActor& actor, ReactionsContainerConstPtr& pHitReactions, ReactionsContainerConstPtr& pDeathReactions, ReactionsContainerConstPtr& pCollisionReactions)
{
	bool bSuccess = false;

	ReactionsParamsKey key = GetKey(actor);
	if (key != INVALID_REACTION_PARAMS_ID)
	{
		ReactionsParamsContainers::const_iterator itFind = m_reactionParamsPool.find(key);
		if (itFind != m_reactionParamsPool.end())
		{
			const SSharedReactions& sharedReactions = itFind->second;
			// Delegate the table on the reactionParams property of this actor
			// (we probably don't need this, but it won't hurt)
			// [*DavidR | 22/Feb/2010] CHECK: Does this duplicate the table or create a reference?
			actor.GetEntity()->GetScriptTable()->SetValue(ACTOR_HIT_DEATH_REACTIONS_PARAMS, sharedReactions.pHitAndDeathReactionsTable);

			pHitReactions = sharedReactions.pHitReactions;
			pDeathReactions = sharedReactions.pDeathReactions;
			pCollisionReactions = sharedReactions.pCollisionReactions;

			bSuccess = true;
		}
		else
		{
			// Instantiate the shared reaction containers
			ReactionsContainerPtr pNewHitReactions(new ReactionsContainer);
			ReactionsContainerPtr pNewDeathReactions(new ReactionsContainer);
			ReactionsContainerPtr pNewCollisionReactions(new ReactionsContainer);

			// Fill death and hit reactions params script table 
			ScriptTablePtr hitAndDeathReactions = LoadReactionsScriptTable(actor);
			if (hitAndDeathReactions)
			{
				// Parse and create hit and death reactions params
				LoadHitDeathReactionsParams(actor, hitAndDeathReactions, pNewHitReactions, pNewDeathReactions, pNewCollisionReactions);

				// Insert it on the pool
				ReactionsParamsContainersItem newPoolItem(key, SSharedReactions(pNewHitReactions, pNewDeathReactions, pNewCollisionReactions, hitAndDeathReactions));
				bSuccess = m_reactionParamsPool.insert(newPoolItem).second;
				CRY_ASSERT(bSuccess);
			}
			else
				Warning("Couldn't load the reactions table for actor %s", actor.GetEntity()->GetName());

			// if the process failed these will be empty
			pHitReactions = pNewHitReactions;
			pDeathReactions = pNewDeathReactions;
			pCollisionReactions = pNewCollisionReactions;
		}
	}
	else
	{
		Warning("Couldn't get unique key for actor %s's reactions. This actor won't have any hit/death reactions", actor.GetEntity()->GetName());
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
// Reload the data structure and scripts
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactionsSystem::Reload()
{
	ExecuteHitDeathReactionsScripts(true);

	m_reactionParamsPool.clear();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactionsSystem::ExecuteHitDeathReactionsScripts(bool bForceReload)
{
	if(!gEnv->pScriptSystem->ExecuteFile(HIT_DEATH_REACTIONS_SCRIPT_FILE, true, bForceReload))
		Warning("Error executing HitDeathReactions script file");
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CHitDeathReactionsSystem::ReactionsParamsKey CHitDeathReactionsSystem::GetKey(const CActor& actor) const
{
	// Reaction params are dependent on:
	// - Character file (because animIDs and Bone Ids)
	// - Reaction params data file
	ReactionsParamsKey key = INVALID_REACTION_PARAMS_ID;

	ICharacterInstance* pMainChar = actor.GetEntity()->GetCharacter(0);
	if (pMainChar)
	{
		const char* szFilePath = pMainChar->GetFilePath();

		ScriptTablePtr pActorScriptTable = actor.GetEntity()->GetScriptTable();
		CRY_ASSERT(pActorScriptTable);

		ScriptAnyValue propertiesTable;
		pActorScriptTable->GetValueAny(ACTOR_PROPERTIES_TABLE, propertiesTable);

		const char* szReactionsDataFilePath = NULL;
		if ((propertiesTable.type == ANY_TTABLE) && propertiesTable.table->GetValue(REACTIONS_DATA_FILE_PROPERTY, szReactionsDataFilePath))
		{
			key = gEnv->pSystem->GetCrc32Gen()->GetCRC32(string(szFilePath) + szReactionsDataFilePath);
		}
		else
		{
			Warning("Couldn't find %s field on %s properties table", REACTIONS_DATA_FILE_PROPERTY, actor.GetEntity()->GetName());
		}
	}

	return key;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
ScriptTablePtr CHitDeathReactionsSystem::LoadReactionsScriptTable(const CActor& actor) const
{
	ScriptTablePtr reactionParamsTable;

	ScriptTablePtr pActorScriptTable = actor.GetEntity()->GetScriptTable();
	CRY_ASSERT(pActorScriptTable);

	if (Script::CallMethod(pActorScriptTable, "LoadXMLData"))
	{
		pActorScriptTable->GetValue(ACTOR_HIT_DEATH_REACTIONS_PARAMS, reactionParamsTable);
	}

	return reactionParamsTable;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool CHitDeathReactionsSystem::LoadHitDeathReactionsParams(const CActor& actor, ScriptTablePtr pHitDeathReactionsTable, ReactionsContainerPtr pHitReactions, ReactionsContainerPtr pDeathReactions, ReactionsContainerPtr pCollisionReactions)
{
	CRY_ASSERT(pHitDeathReactionsTable);
	CRY_ASSERT(pHitReactions);
	CRY_ASSERT(pDeathReactions);
	CRY_ASSERT(pCollisionReactions);

	bool bSuccess = false;


	// [*DavidR | 23/Feb/2010] CryShared pointer doesn't have and overload for unary operator *
	LoadReactionsParams(actor, pHitDeathReactionsTable, DEATH_REACTIONS_PARAMS, true, 0, *(pDeathReactions.get()));
	LoadReactionsParams(actor, pHitDeathReactionsTable, COLLISION_REACTIONS_PARAMS, true, pDeathReactions->size(), *(pCollisionReactions.get()));
	LoadReactionsParams(actor, pHitDeathReactionsTable, HIT_REACTIONS_PARAMS, false, 0, *(pHitReactions.get()));
	
	bSuccess = true;

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactionsSystem::LoadReactionsParams(const CActor& actor, IScriptTable* pHitDeathReactionsTable, const char* szReactionParamsName, bool bDeathReactions, ReactionId baseReactionId, ReactionsContainer& reactions)
{
	// Store list of reaction descriptions
	ScriptTablePtr pReactionsTable;
	if (pHitDeathReactionsTable->GetValue(szReactionParamsName, pReactionsTable))
	{
		for (IScriptTable::Iterator it = pReactionsTable->BeginIteration(); pReactionsTable->MoveNext(it); )
		{
			CRY_ASSERT(it.value.type == ANY_TTABLE);

			PreProcessReactionParams(it.value.table);

			SReactionParams reactionParams;
			GetReactionParamsFromScript(actor, it.value.table, reactionParams);

			// On load, write the reactionId on the reaction script table
			// [*DavidR | 23/Feb/2010] Hacky: reactionId is the index (in the range [1..size]) of the reaction 
			// on the container, negative if is a death reaction container, positive if is a hit reaction container, 0 is invalid
			// Collision reactions ids follow death reactions ids (I hate me)
			it.value.table->SetValue(REACTION_ID, (ReactionId(reactions.size() + 1) + baseReactionId) * (bDeathReactions ? -1 : 1));

			reactions.push_back(reactionParams);
		}

		// Shrink capacity excess
		ReactionsContainer(reactions).swap(reactions);
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactionsSystem::PreProcessReactionParams(SmartScriptTable pReactionTable)
{
	// Parse the stance array strings to the value hold by the lua global variable with that name. 
	// Transform the every stance array item to hold the value of that global variable (or -1 if not found, not likely to happen)
	ScriptTablePtr pAllowedStancesArray;
	pReactionTable->GetValue(ALLOWED_STANCES_ARRAY, pAllowedStancesArray);

	int iCount = pAllowedStancesArray ? pAllowedStancesArray->Count() : 0;
	for (int i = 0; i < iCount; ++i)
	{
		CRY_ASSERT(pAllowedStancesArray->GetAtType(i + 1) == svtString);

		int iStance = -1;
		const char* szStanceGlobalValue = NULL;
		pAllowedStancesArray->GetAt(i + 1, szStanceGlobalValue);
		gEnv->pScriptSystem->GetGlobalValue(szStanceGlobalValue, iStance);
		CRY_ASSERT(iStance != -1);

		pAllowedStancesArray->SetAt(i + 1, iStance);
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactionsSystem::GetReactionParamsFromScript(const CActor& actor, const ScriptTablePtr pScriptTable, SReactionParams& reactionParams) const
{
	CRY_ASSERT(pScriptTable);

	reactionParams.Reset();

	// Cache scriptTablePtr
	reactionParams.reactionScriptTable = pScriptTable;

	// Cache validation properties
	if (pScriptTable->HaveValue(VALIDATION_FUNC_PROPERTY))
	{
		const char* szValidationFunc = NULL;
		if (pScriptTable->GetValue(VALIDATION_FUNC_PROPERTY, szValidationFunc) && szValidationFunc && (szValidationFunc[0] != '\0'))
			reactionParams.sCustomValidationFunc = szValidationFunc;
	}
	
	if (pScriptTable->HaveValue(AISIGNAL_PROPERTY))
	{
		const char* szSignal = NULL;
		if (pScriptTable->GetValue(AISIGNAL_PROPERTY, szSignal) && szSignal && (szSignal[0] != '\0'))
			reactionParams.sCustomAISignal = szSignal;
	}

	if (pScriptTable->HaveValue(MINIMUM_SPEED_PROPERTY)) 
		pScriptTable->GetValue(MINIMUM_SPEED_PROPERTY, reactionParams.fMinimumSpeedAllowed);

	if (pScriptTable->HaveValue(MAXIMUM_SPEED_PROPERTY)) 
		pScriptTable->GetValue(MAXIMUM_SPEED_PROPERTY, reactionParams.fMaximumSpeedAllowed);

	if (pScriptTable->HaveValue(MINIMUM_DAMAGE_PROPERTY)) 
		pScriptTable->GetValue(MINIMUM_DAMAGE_PROPERTY, reactionParams.fMinimumDamageAllowed);

	if (pScriptTable->HaveValue(MAXIMUM_DAMAGE_PROPERTY)) 
		pScriptTable->GetValue(MAXIMUM_DAMAGE_PROPERTY, reactionParams.fMaximumDamageAllowed);

	if (pScriptTable->HaveValue(ALLOWED_PARTS_ARRAY))
		FillAllowedPartIds(actor, pScriptTable, reactionParams);

	if (pScriptTable->HaveValue(MOVEMENT_DIRECTION_PROPERTY))
	{
		const char* szMovementDirection = NULL;
		pScriptTable->GetValue(MOVEMENT_DIRECTION_PROPERTY, szMovementDirection);

		reactionParams.movementDir = GetCardinalDirectionFromString(szMovementDirection);
	}

	if (pScriptTable->HaveValue(SHOT_ORIGIN_PROPERTY))
	{
		const char* szShotOrigin = NULL;
		pScriptTable->GetValue(SHOT_ORIGIN_PROPERTY, szShotOrigin);

		reactionParams.shotOrigin = GetCardinalDirectionFromString(szShotOrigin);
	}

	if (pScriptTable->HaveValue(PROBABILITY_PERCENT_PROPERTY))
	{
		pScriptTable->GetValue(PROBABILITY_PERCENT_PROPERTY, reactionParams.fProbability);
		Limit(reactionParams.fProbability, 0.0f, 1.0f);
	}

	if (pScriptTable->HaveValue(ALLOWED_STANCES_ARRAY))
	{
		ScriptTablePtr pAllowedStancesArray;
		pScriptTable->GetValue(ALLOWED_STANCES_ARRAY, pAllowedStancesArray);

		int iCount = pAllowedStancesArray->Count();
		for (int i = 0; i < iCount; ++i)
		{
			int iStance = -1;
			pAllowedStancesArray->GetAt(i + 1, iStance);

			reactionParams.allowedStances.insert(static_cast<EStance>(iStance));
		}
	}

	if (pScriptTable->HaveValue(ALLOWED_HIT_TYPES_ARRAY))
	{
		ScriptTablePtr pAllowedHitTypesArray;
		pScriptTable->GetValue(ALLOWED_HIT_TYPES_ARRAY, pAllowedHitTypesArray);

		int iCount = pAllowedHitTypesArray->Count();
		for (int i = 0; i < iCount; ++i)
		{
			const char* szHitType = NULL;
			pAllowedHitTypesArray->GetAt(i + 1, szHitType);

			reactionParams.allowedHitTypes.insert(g_pGame->GetGameRules()->GetHitTypeId(szHitType));
		}
	}

	if (pScriptTable->HaveValue(ALLOWED_PROJECTILES_ARRAY))
	{
		ScriptTablePtr pAllowedProjectilesArray;
		pScriptTable->GetValue(ALLOWED_PROJECTILES_ARRAY, pAllowedProjectilesArray);

		int iCount = pAllowedProjectilesArray->Count();
		for (int i = 0; i < iCount; ++i)
		{
			const char* szProjClass = NULL;
			pAllowedProjectilesArray->GetAt(i + 1, szProjClass);

			uint16 uProjClassId = 0;
			if (g_pGame->GetIGameFramework()->GetNetworkSafeClassId(uProjClassId, szProjClass))
				reactionParams.allowedProjectiles.insert(uProjClassId);
		}
	}

	if (pScriptTable->HaveValue(ALLOWED_WEAPONS_ARRAY))
	{
		ScriptTablePtr pAllowedWeapons;
		pScriptTable->GetValue(ALLOWED_WEAPONS_ARRAY, pAllowedWeapons);

		int iCount = pAllowedWeapons->Count();
		for (int i = 0; i < iCount; ++i)
		{
			const char* szWeaponClass = NULL;
			pAllowedWeapons->GetAt(i + 1, szWeaponClass);

			uint16 uProjClassId = 0;
			if (g_pGame->GetIGameFramework()->GetNetworkSafeClassId(uProjClassId, szWeaponClass))
				reactionParams.allowedWeapons.insert(uProjClassId);
		}
	}

	if (pScriptTable->HaveValue(ONLY_IF_USING_MOUNTED_ITEM_PROPERTY))
	{
		pScriptTable->GetValue(ONLY_IF_USING_MOUNTED_ITEM_PROPERTY, reactionParams.bAllowOnlyWhenUsingMountedItems);
	}

	if (pScriptTable->HaveValue(DESTRUCTIBLE_EVENT_PROPERTY))
	{
		const char* szDestructibleEvent = NULL;
		if (pScriptTable->GetValue(DESTRUCTIBLE_EVENT_PROPERTY, szDestructibleEvent) && szDestructibleEvent && (szDestructibleEvent[0] != '\0'))
		{
			const Crc32Gen* pCRC32 = gEnv->pSystem->GetCrc32Gen();
			reactionParams.destructibleEvent = pCRC32->GetCRC32Lowercase(szDestructibleEvent);
		}
	}

	// Cache default execution properties
	if (pScriptTable->HaveValue(REACTION_FUNC_PROPERTY))
	{
		const char* szExecutionFunc = NULL;
		if (pScriptTable->GetValue(REACTION_FUNC_PROPERTY, szExecutionFunc) && szExecutionFunc && (szExecutionFunc[0] != '\0'))
			reactionParams.sCustomExecutionFunc = szExecutionFunc;
	}

	if (pScriptTable->HaveValue(REACTION_ON_COLLISION_PROPERTY) || 
		pScriptTable->HaveValue(RAGDOLL_ON_COLLISION_PROPERTY))
	{
		unsigned int reactionOnCollision = NO_COLLISION_REACTION;
		bool bRagdollOnCollision = false;

		if (!pScriptTable->GetValue(REACTION_ON_COLLISION_PROPERTY, reactionOnCollision))
			pScriptTable->GetValue(RAGDOLL_ON_COLLISION_PROPERTY, bRagdollOnCollision);

		reactionParams.reactionOnCollision = bRagdollOnCollision ? 1 : reactionOnCollision;
	}

	if (pScriptTable->HaveValue(PAUSE_AI_PROPERTY))
	{
		pScriptTable->GetValue(PAUSE_AI_PROPERTY, reactionParams.bPauseAI);
	}

	if (pScriptTable->HaveValue(AG_REACTION_TABLE)) 
	{
		ScriptAnyValue agReactionTable;
		if (pScriptTable->GetValueAny(AG_REACTION_TABLE, agReactionTable) && (agReactionTable.type == ANY_TTABLE))
		{
			ScriptTablePtr pReactionProperty(agReactionTable.table);

			if (pReactionProperty->HaveValue(AG_INPUT_VALUE_PROPERTY))
			{
				const char* szAGInputValue = NULL;
				if (pReactionProperty->GetValue(AG_INPUT_VALUE_PROPERTY, szAGInputValue) && szAGInputValue && (szAGInputValue[0] != '\0'))
					reactionParams.agReaction.sAGInputValue = szAGInputValue;
			}

			if (pReactionProperty->HaveValue(VARIATIONS_ARRAY))
			{
				if (!reactionParams.agReaction.sAGInputValue.empty())
				{
					ScriptTablePtr pVariationsArray;
					pReactionProperty->GetValue(VARIATIONS_ARRAY, pVariationsArray);

					int iCount = pVariationsArray->Count();
					for (int i = 0; i < iCount; ++i)
					{
						ScriptTablePtr pVariation;
						if (pVariationsArray->GetAt(i + 1, pVariation))
						{
							const char* szVariationName = NULL;
							const char* szVariationValue = NULL;
							if (pVariation->GetValue(VARIATION_NAME, szVariationName) && pVariation->GetValue(VARIATION_VALUE, szVariationValue))
							{
								SReactionParams::SAnimGraphReaction::SVariationData variationData(szVariationName, szVariationValue);
								reactionParams.agReaction.variations.push_back(variationData);
							}
						}
					}
				}
				else
					Warning("Anim graph variations found, but no input value!");
			}
		}
		else
			Warning("Error reading %s property. Expected a table", AG_REACTION_TABLE);
	}

	GetReactionAnimParamsFromScript(actor, pScriptTable, reactionParams.reactionAnim);

	if (pScriptTable->HaveValue(SNAP_ORIENTATION_ANGLE))
	{
		int angle = 0;
		pScriptTable->GetValue(SNAP_ORIENTATION_ANGLE, angle);
		reactionParams.orientationSnapAngle = DEG2RAD(static_cast<float>(angle));
		reactionParams.flags |= SReactionParams::OrientateToHitDir;
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactionsSystem::GetReactionAnimParamsFromScript(const CActor& actor, ScriptTablePtr pScriptTable, SReactionParams::SReactionAnim& reactionAnim) const
{
	if (pScriptTable->HaveValue(REACTION_ANIM_NAME_PROPERTY))
	{
		// Kept for backwards compatibility and to help keeping the reactions as simple as possible
		const char* szAnimName = NULL;
		if (pScriptTable->GetValue(REACTION_ANIM_NAME_PROPERTY, szAnimName))
		{
			ICharacterInstance* pMainChar = actor.GetEntity()->GetCharacter(0);
			CRY_ASSERT(pMainChar);
			IAnimationSet *pAnimSet = pMainChar ? pMainChar->GetIAnimationSet() : NULL;
			if (pAnimSet)
			{
				int animID = pAnimSet->GetAnimIDByName(szAnimName);
				if (animID >= 0)
				{
					reactionAnim.animIDs.push_back(animID);
				}
				else
				{
					AnimIDError(szAnimName);
				}
			}
		}
	}

	if (pScriptTable->HaveValue(REACTION_ANIM_PROPERTY)) 
	{
		ScriptAnyValue reactionAnimTable;
		if (pScriptTable->GetValueAny(REACTION_ANIM_PROPERTY, reactionAnimTable) && (reactionAnimTable.type == ANY_TTABLE))
		{
			// Partial anim flag
			ScriptTablePtr pReactionAnimTable(reactionAnimTable.table);
			if (pReactionAnimTable->HaveValue(REACTION_ANIM_PARTIAL_ANIM))
			{
				bool bPartialAnim = false;
				pReactionAnimTable->GetValue(REACTION_ANIM_PARTIAL_ANIM, bPartialAnim);

				if (bPartialAnim)
					reactionAnim.animFlags |= CA_PARTIAL_SKELETON_UPDATE;
				else
					reactionAnim.animFlags &= ~CA_PARTIAL_SKELETON_UPDATE;
			}

			// Additive anim?
			if (pReactionAnimTable->HaveValue(REACTION_ANIM_ADDITIVE_ANIM))
			{
				pReactionAnimTable->GetValue(REACTION_ANIM_ADDITIVE_ANIM, reactionAnim.bAdditive);
			}

			// Animation layer
			if (pReactionAnimTable->HaveValue(REACTION_ANIM_LAYER))
			{
				pReactionAnimTable->GetValue(REACTION_ANIM_LAYER, reactionAnim.iLayer);
			}

			// List of animations and their variation
			if (pReactionAnimTable->HaveValue(ANIM_NAME_ARRAY)) 
			{
				ICharacterInstance *pMainChar = actor.GetEntity()->GetCharacter(0);
				CRY_ASSERT(pMainChar);
				IAnimationSet *pAnimSet = pMainChar ? pMainChar->GetIAnimationSet() : NULL;
				if (pAnimSet)
				{
					ScriptTablePtr pAnimationArray;
					pReactionAnimTable->GetValue(ANIM_NAME_ARRAY, pAnimationArray);

					int iCount = pAnimationArray->Count();
					for (int i = 0; i < iCount; ++i)
					{
						ScriptTablePtr pAnimation;
						if (pAnimationArray->GetAt(i + 1, pAnimation))
						{
							const char* szReactionAnim = NULL;
							if (pAnimation->GetValue(ANIM_NAME_PROPERTY, szReactionAnim) && szReactionAnim && (szReactionAnim[0] != '\0'))
							{
								int variants = 0;
								if (pAnimation->GetValue(ANIM_VARIANTS_PROPERTY, variants))
								{
									//--- Load in all variants
									CryPathString variantName;

									for (int k = 0; k < variants; k++)
									{
										variantName.FormatFast("%s%d", szReactionAnim, k + 1);

										int animID = pAnimSet->GetAnimIDByName(variantName);
										if (animID >= 0)
										{
											reactionAnim.animIDs.push_back(animID);
										}
										else
										{
											AnimIDError(variantName);
										}
									}
								}
								else
								{
									//--- Load in the single animation
									int animID = pAnimSet->GetAnimIDByName(szReactionAnim);
									if (animID >= 0)
									{
										reactionAnim.animIDs.push_back(animID);
									}
									else
									{
										AnimIDError(szReactionAnim);
									}
								}
							}
						}
					}
				}

				// Shrink capacity excess
				SReactionParams::AnimIDContainer(reactionAnim.animIDs).swap(reactionAnim.animIDs);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CHitDeathReactionsSystem::FillAllowedPartIds(const CActor& actor, const ScriptTablePtr pScriptTable, SReactionParams& reactionParams) const
{
	ScriptTablePtr pAllowedPartArray;
	pScriptTable->GetValue(ALLOWED_PARTS_ARRAY, pAllowedPartArray);

	ICharacterInstance* pMainChar = actor.GetEntity()->GetCharacter(0);
	CRY_ASSERT(pMainChar);
	ISkeletonPose* pSkeletonPose = pMainChar ? pMainChar->GetISkeletonPose() : NULL;
	if (pSkeletonPose)
	{
		int iCount = pAllowedPartArray->Count();
		for (int i = 0; i < iCount; ++i)
		{
			const char* szPartName = NULL;
			pAllowedPartArray->GetAt(i + 1, szPartName);

			int iPartId = pSkeletonPose->GetJointIDByName(szPartName);

			// [*DavidR | 12/Nov/2009] ToDo: Log iPartId == -1 without spamming
			if (iPartId != -1)
				reactionParams.allowedPartIds.insert(iPartId);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
ECardinalDirection CHitDeathReactionsSystem::GetCardinalDirectionFromString(const char* szCardinalDirection) const
{
	ECardinalDirection cardinalDirection = eCD_Invalid;

	if (szCardinalDirection)
	{
		if (strcmp(szCardinalDirection, "left") == 0)
			cardinalDirection = eCD_Left;
		else if (strcmp(szCardinalDirection, "right") == 0)
			cardinalDirection = eCD_Right;
		else if (strcmp(szCardinalDirection, "forward") == 0)
			cardinalDirection = eCD_Forward;
		else if (strcmp(szCardinalDirection, "back") == 0)
			cardinalDirection = eCD_Back;

		else if (strcmp(szCardinalDirection, "leftSide") == 0)
			cardinalDirection = eCD_LeftSide;
		else if (strcmp(szCardinalDirection, "rightSide") == 0)
			cardinalDirection = eCD_RightSide;
		else if (strcmp(szCardinalDirection, "ahead") == 0)
			cardinalDirection = eCD_Ahead;
		else if (strcmp(szCardinalDirection, "behind") == 0)
			cardinalDirection = eCD_Behind;
	}

	return cardinalDirection;
}
