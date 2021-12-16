#include "StdAfx.h"

#include "UIObjectives.h"

#include "HUD/HUD.h"
#include "HUD/UIElementManager.h"
#include "HUD/ScreenLayoutManager.h"
#include "HUD/UI/UITagNames.h"

#include "Graphics/2DRenderUtils.h"

#include "Game.h"
#include "GameRules.h"

#include "Utility/StringUtils.h"
#include "PerkDbgDisplay.h"
#include "Player.h"

static const float DEFAULT_OBJECTIVE_ICON_OFFSCREEN_SIZE = 20.0f;
static const float DEFAULT_OBJECTIVE_ICON_SIZE           = 32.0f;

static AUTOENUM_BUILDNAMEARRAY(s_missionObjectiveNames, MissionObjectiveList);
static AUTOENUM_BUILDNAMEARRAY(s_renderModeNames, ObjectiveIconRenderModeList);

struct SObjectiveIconTexCoords
{
	float s0, s1, t0, t1;

	ILINE void Set(float s0_in, float s1_in, float t0_in, float t1_in)
	{
		s0 = s0_in;
		s1 = s1_in;
		t0 = t0_in;
		t1 = t1_in;
	}
};

enum ETexCoordsType
{
	kTexCoordsPart_centre       = 0,
	kTexCoordsPart_left         = 1,
	kTexCoordsPart_right        = 2,

	kTexCoordsPart_middle       = 0,
	kTexCoordsPart_top          = 3,
	kTexCoordsPart_bottom       = 6,

	kTexCoordsType_topLeft      = kTexCoordsPart_top    + kTexCoordsPart_left,
	kTexCoordsType_topCentre    = kTexCoordsPart_top    + kTexCoordsPart_centre,
	kTexCoordsType_topRight     = kTexCoordsPart_top    + kTexCoordsPart_right,
	kTexCoordsType_middleLeft   = kTexCoordsPart_middle + kTexCoordsPart_left,
	kTexCoordsType_middleCentre = kTexCoordsPart_middle + kTexCoordsPart_centre,
	kTexCoordsType_middleRight  = kTexCoordsPart_middle + kTexCoordsPart_right,
	kTexCoordsType_bottomLeft   = kTexCoordsPart_bottom + kTexCoordsPart_left,
	kTexCoordsType_bottomCentre = kTexCoordsPart_bottom + kTexCoordsPart_centre,
	kTexCoordsType_bottomRight  = kTexCoordsPart_bottom + kTexCoordsPart_right,

	kTexCoordsType_num          = 9
};

SObjectiveIconTexCoords s_objectiveIconTexCoords[kTexCoordsType_num];

//------------------------------------------------------------------------------------
static bool ShouldShowIcon_ThreatAwarenessPerk()
{
	IActor * actor = g_pGame->GetIGameFramework()->GetClientActor();
	return actor && actor->IsPlayer() && ((CPlayer*)actor)->GetPerkData<bool>(EPD_ThreatIcons);
}

//------------------------------------------------------------------------------------
CUIMissionObjectives::CUIMissionObjectives() :
CUIElement()
{

	//REGISTER_COMMAND("hud_testBattleLog", CmdTestBattleLog, 0, "Add a test message to the battle log");

	m_layoutManager = g_pGame->GetHUD()->GetLayoutManager();

	// Arrow textures are split into 8 as follows:
	//		top left	| top	 | top right	| right
	//		------------|--------|--------------|------
	//		bottom left	| bottom | bottom right | left

	s_objectiveIconTexCoords[kTexCoordsType_topLeft].Set       (0.f,   0.25f, 1.f,  0.5f );
	s_objectiveIconTexCoords[kTexCoordsType_topCentre].Set     (0.25f, 0.5f,  1.f,  0.5f );
	s_objectiveIconTexCoords[kTexCoordsType_topRight].Set      (0.5f,  0.75f, 1.f,  0.5f );
	s_objectiveIconTexCoords[kTexCoordsType_middleLeft].Set    (0.75f, 1.f,   0.5f, 0.f  );
	s_objectiveIconTexCoords[kTexCoordsType_middleCentre].Set  (0.f,   1.f,   1.f,  0.f  );
	s_objectiveIconTexCoords[kTexCoordsType_middleRight].Set   (0.75f, 1.f,   1.f,  0.5f );
	s_objectiveIconTexCoords[kTexCoordsType_bottomLeft].Set    (0.f,   0.25f, 0.5f, 0.f  );
	s_objectiveIconTexCoords[kTexCoordsType_bottomCentre].Set  (0.25f, 0.5f,  0.5f, 0.f  );
	s_objectiveIconTexCoords[kTexCoordsType_bottomRight].Set   (0.5f,  0.75f, 0.5f, 0.f  );

	// Register for HUD Events
	CHUD::AddHUDEventListener(this, "OnNewObjective");
	CHUD::AddHUDEventListener(this, "OnRemoveObjective");
}

CUIMissionObjectives::~CUIMissionObjectives( void )
{
	// unregister at listener
	CHUD::RemoveHUDEventListener(this);

#if INCLUDE_OBJECTIVE_ICON_DEBUG_CODE
	if(gEnv && gEnv->pConsole)
	{
		gEnv->pConsole->UnregisterVariable("hud_objectiveIcons_neverUseEdgeTextures", true);
		gEnv->pConsole->UnregisterVariable("hud_objectiveIcons_showList", true);
		gEnv->pConsole->UnregisterVariable("hud_objectiveIcons_clampToOval", true);
		gEnv->pConsole->UnregisterVariable("hud_objectiveIcons_scaleSafeZone", true);
		gEnv->pConsole->UnregisterVariable("hud_objectiveIcons_brightnessWhenBehindCamera", true);
	}
#endif

	delete[] m_missionObjectives;
	delete[] m_groups;
}

static bool xmlReadBool(const IItemParamsNode * element, const char * key, bool returnValue, bool require)
{
	int gotcha = 0;
	bool valuePresent = element->GetAttribute(key, gotcha);

	if (valuePresent)
	{
		if (gotcha)
		{
			returnValue = true;
		}
		else
		{
			returnValue = false;
		}
	}
	else if (require)
	{
		CRY_ASSERT_MESSAGE(0, string().Format("Attribute '%s' is missing from XML tag '%s'!", key, element->GetName()));
	}

	return returnValue;
}

// Init
void CUIMissionObjectives::Initialize(const IItemParamsNode * element, IUIElement *parent)
{
	CUIElement::Initialize(element, parent);

	m_maxIcons = 0;
	m_iconBrightnessWhenBehindCamera = 1.f;
	m_scaleSafeZone = 0.5f;
	m_clampToOval = false;
	m_groups = NULL;

#if INCLUDE_OBJECTIVE_ICON_DEBUG_CODE
	m_numGroups = 0;
#endif

	memset (m_missionObjectiveTypes, 0, sizeof(m_missionObjectiveTypes));

	XmlNodeRef xmlUI = GetISystem()->LoadXmlFile( "Scripts/UI/UI2_cw2_missionobjectives.xml" );
	if(!xmlUI)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "[HUD_Impl] Warning: Mission objectives file not found!");
		return;
	}

	IItemParamsNode *paramNode = g_pGame->GetIGameFramework()->GetIItemSystem()->CreateParams();
	paramNode->ConvertFromXML(xmlUI);

	const int numBaseLevelElements = paramNode->GetChildCount();

	for(int baseLevelElement = 0; baseLevelElement < numBaseLevelElements; ++ baseLevelElement)
	{
		const IItemParamsNode * baseLevelChildXml = paramNode->GetChild(baseLevelElement);

		if (0 == stricmp(baseLevelChildXml->GetName(), "clampToOval"))
		{
			m_clampToOval = xmlReadBool(baseLevelChildXml, "value", false, true);
		}
		else if (0 == stricmp(baseLevelChildXml->GetName(), "scaleSafeZone"))
		{
			baseLevelChildXml->GetAttribute("value", m_scaleSafeZone);
		}
		else if (0 == stricmp(baseLevelChildXml->GetName(), "iconBrightnessWhenBehindCamera"))
		{
			baseLevelChildXml->GetAttribute("value", m_iconBrightnessWhenBehindCamera);
		}
		else if (m_groups == NULL && 0 == stricmp(baseLevelChildXml->GetName(), "IconGroups"))
		{
			int iNumGroups = baseLevelChildXml->GetChildCount();

#if IS_HUD_DEV
			CryLog ("[HUD] About to allocate enough memory for %d objective icon groups", iNumGroups);
#endif //IS_HUD_DEV

			m_groups = new SMissionObjectiveIconGroup[iNumGroups];

			for (int groupNum = 0; groupNum < iNumGroups; ++ groupNum)
			{
				// Sanity checks
				ASSERT_ARE_EQUAL(m_groups[groupNum].m_maxNum, 0);
				ASSERT_ARE_EQUAL(m_groups[groupNum].m_numUsed, 0);

				const IItemParamsNode * immediateChildXml = baseLevelChildXml->GetChild(groupNum);
				if (0 == stricmp(immediateChildXml->GetName(), "IconGroup"))
				{
					int numToAllocate = 0;
					immediateChildXml->GetAttribute("maxNum", numToAllocate);
					CRY_ASSERT_MESSAGE(numToAllocate > 0, string().Format("IconGroup specified bad maxNum=%d%s", numToAllocate, immediateChildXml->GetAttributeType("maxNum") != eIPT_None ? "" : " (actually, it didn't specify a maxNum value at all)"));

					if (numToAllocate > 0) // TODO: && is required for current game mode
					{
						m_groups[groupNum].m_maxNum = numToAllocate;

#if INCLUDE_OBJECTIVE_ICON_DEBUG_CODE
						m_groups[groupNum].m_assertWhenRunOutOfSlots = xmlReadBool(immediateChildXml, "assertWhenRunOutOfSlots", false, true);
#endif

						m_maxIcons += numToAllocate;
						int iNumInnerElements = immediateChildXml->GetChildCount();
						for(int i = 0; i < iNumInnerElements; ++i)
						{
							const IItemParamsNode * xmlElement = immediateChildXml->GetChild(i);
							if (0 == stricmp(xmlElement->GetName(), "IconDef"))
							{
								ReadIconDef(xmlElement, groupNum);
							}
							else
							{
								CRY_ASSERT_MESSAGE(false, string().Format("Found an XML '%s' tag where only expected 'IconDef' tags!", xmlElement->GetName()));
							}
						}
					}
				}
				else
				{
					CRY_ASSERT_MESSAGE(false, string().Format("Found an XML '%s' tag where only expected 'IconGroup' tags!", immediateChildXml->GetName()));
				}
			}
#if INCLUDE_OBJECTIVE_ICON_DEBUG_CODE
			m_numGroups = iNumGroups;
#endif
		}
		else
		{
			CRY_ASSERT_MESSAGE(false, string().Format("Found an XML '%s' tag where didn't expect one!", baseLevelChildXml->GetName()));
		}
	}

	for(int i = 0; i < EGRMO_Max; ++i)
	{
		CRY_ASSERT_MESSAGE(m_missionObjectiveTypes[i].m_textureOne[0], string().Format("Failed to find any info for objective type %d '%s'", i, s_missionObjectiveNames[i]));
	}

	assert (m_groups);

#if INCLUDE_OBJECTIVE_ICON_DEBUG_CODE
	m_dbgListUsedIconSlots = 0;
	m_dbgNeverUseEdgeTextures = 0;

	if(gEnv && gEnv->pConsole)
	{
		gEnv->pConsole->Register("hud_objectiveIcons_neverUseEdgeTextures", & m_dbgNeverUseEdgeTextures, 0, 0, "");
		gEnv->pConsole->Register("hud_objectiveIcons_showList", & m_dbgListUsedIconSlots, 0, 0, "");
		gEnv->pConsole->Register("hud_objectiveIcons_clampToOval", & m_clampToOval, m_clampToOval, 0, "");
		gEnv->pConsole->Register("hud_objectiveIcons_scaleSafeZone", & m_scaleSafeZone, m_scaleSafeZone, 0, "");
		gEnv->pConsole->Register("hud_objectiveIcons_brightnessWhenBehindCamera", & m_iconBrightnessWhenBehindCamera, m_iconBrightnessWhenBehindCamera, 0, "");

		// NB: If adding more variables here, make sure you also unregister them in the destructor... otherwise when the HUD is reloaded, the
		// commands will carry on modifying the OLD memory locations in the deleted class rather than the NEW memory locations here. Nasty.
	}
#endif

#if IS_HUD_DEV
	CryLog ("[HUD] About to allocate enough memory for %d simultaneous objective icons", m_maxIcons);
#endif //IS_HUD_DEV
	m_missionObjectives = new SMissionObjectiveData[m_maxIcons];

	paramNode->Release();
}

void CUIMissionObjectives::ReadIconDef(const IItemParamsNode * params, int groupNum)
{
	const char * objectiveTypeString = params->GetAttribute("type");
	EGameRulesMissionObjectives objectiveType = EGRMO_Unknown;

	for (int j = 0; j < EGRMO_Max; ++ j)
	{
		if (0 == stricmp(objectiveTypeString, s_missionObjectiveNames[j]))
		{
			objectiveType = (EGameRulesMissionObjectives)j;
			break;
		}
	}

	CRY_ASSERT_MESSAGE(objectiveType != EGRMO_Unknown, string().Format("'%s' is not a valid objective type!", objectiveTypeString));

	if (objectiveType != EGRMO_Unknown)
	{
		SMissionObjectiveType *info = & m_missionObjectiveTypes[objectiveType];

		// Set defaults...
		memset (info, 0, sizeof(SMissionObjectiveType));
		info->m_onScreenSize = DEFAULT_OBJECTIVE_ICON_SIZE;
		info->m_atEdgeSize = DEFAULT_OBJECTIVE_ICON_OFFSCREEN_SIZE;
		info->m_group = & m_groups[groupNum];

		// Read data from XML...
		params->GetAttribute("zOffset", info->zOffset);
		params->GetAttribute("size", info->m_onScreenSize);
		params->GetAttribute("edgeSize", info->m_atEdgeSize);

		const char * renderModeString = params->GetAttribute("renderMode");

		if(renderModeString)
		{
			for (int j = 0; j < kOIRM_num; ++ j)
			{
				if (0 == stricmp(renderModeString, s_renderModeNames[j]))
				{
					info->m_renderMode = (EObjectiveIconRenderMode)j;
					break;
				}
			}

		}

		CRY_ASSERT_MESSAGE(info->m_renderMode != kOIRM_singleTexture || renderModeString == NULL || (0 == stricmp(renderModeString, s_renderModeNames[kOIRM_singleTexture])),
			string().Format("'%s' is not a valid objective icon render mode! (Was found while reading '%s')", renderModeString, objectiveTypeString));

		const char * texName = params->GetAttribute("texture");

		if(texName)
		{
			cry_strncpy(info->m_textureOne, texName, sizeof(info->m_textureOne));
		}
		else
		{
			info->m_textureOne[0] = '\0';
		}

		texName = params->GetAttribute("textureAlt");

		if(texName)
		{
			cry_strncpy(info->m_textureTwo, texName, sizeof(info->m_textureTwo));
		}
		else
		{
			info->m_textureTwo[0] = '\0';
		}

		texName = params->GetAttribute("arrowsTex");

		if(texName)
		{
			cry_strncpy(info->m_arrowsTextureOne, texName, sizeof(info->m_arrowsTextureOne));
		}
		else
		{
			info->m_arrowsTextureOne[0] = '\0';
		}
		
		texName = params->GetAttribute("arrowsTexAlt");
		if(texName)
		{
			cry_strncpy(info->m_arrowsTextureTwo, texName, sizeof(info->m_arrowsTextureTwo));
		}
		else
		{
			info->m_arrowsTextureTwo[0] = '\0';
		}
		

		params->GetAttribute("opaqueWithinDistance", info->m_fadeOut.opaqueWithinDistance);
		info->m_doFadeOut = params->GetAttribute("vanishAfterDistance", info->m_fadeOut.vanishAfterDistance);
		info->m_doShrinkWithDistance = params->GetAttribute("shrinkWithDistance", info->m_onScreenShrinkWithDistanceVal);
		if (! params->GetAttribute("edgeShrinkWithDistance", info->m_atEdgeShrinkWithDistanceVal))
		{
			info->m_atEdgeShrinkWithDistanceVal = info->m_onScreenShrinkWithDistanceVal;
		}

#if IS_HUD_DEV
		CryLog ("'%s' m_doShrinkWithDistance = %d (val = %f %f)", objectiveTypeString, info->m_doShrinkWithDistance, info->m_onScreenShrinkWithDistanceVal, info->m_atEdgeShrinkWithDistanceVal);
#endif //IS_HUD_DEV

		const int numIconExtras = params->GetChildCount();
		for (int iconExtraNum = 0; iconExtraNum < numIconExtras; ++ iconExtraNum)
		{
			const IItemParamsNode * extraXml = params->GetChild(iconExtraNum);
			const char * extraName = extraXml->GetName();
			if (0 == stricmp(extraName, "Text"))
			{
				float r = 1.f, g = 1.f, b = 1.f, a = 1.f;

				extraXml->GetAttribute("x", info->m_text.x);
				extraXml->GetAttribute("y", info->m_text.y);
				extraXml->GetAttribute("width", info->m_text.width);
				extraXml->GetAttribute("height", info->m_text.height);
				extraXml->GetAttribute("r", r);
				extraXml->GetAttribute("g", g);
				extraXml->GetAttribute("b", b);
				extraXml->GetAttribute("alpha", a);
				info->m_text.colour.Set(r, g, b, a);
				info->m_doShowEntityName = xmlReadBool(extraXml,"name",true/*default if not found*/,false/*required*/);
				info->m_showDistanceTo = xmlReadBool(extraXml,"dist",false/*default if not found*/,false/*required*/);
			}
			else if (0 == stricmp(extraName, "Dist"))
			{
				float r = 1.f, g = 1.f, b = 1.f, a = 1.f;

				extraXml->GetAttribute("x", info->m_dist.x);
				extraXml->GetAttribute("y", info->m_dist.y);
				extraXml->GetAttribute("width", info->m_dist.width);
				extraXml->GetAttribute("height", info->m_dist.height);
				extraXml->GetAttribute("r", r);
				extraXml->GetAttribute("g", g);
				extraXml->GetAttribute("b", b);
				extraXml->GetAttribute("alpha", a);
				info->m_dist.colour.Set(r, g, b, a);

				info->m_hasDistElement = true;
			}
			else
			{
				CRY_ASSERT_MESSAGE (0, string().Format("Unexpected '%s' tag while reading icon def '%s'", extraName, objectiveTypeString));
			}
		}

		// Some icons only render under certain circumstances...
		const char * onlyRenderWhen = params->GetAttribute("onlyRenderWhen");
		assert(info->m_shouldRenderFunc == NULL);

		if (onlyRenderWhen)
		{
			if (0 == stricmp("threatAwareness", onlyRenderWhen))
			{
				info->m_shouldRenderFunc = ShouldShowIcon_ThreatAwarenessPerk;
			}
			else
			{
				CRY_ASSERT_MESSAGE(0, string().Format("Unexpected onlyRenderWhen value \"%s\"! The '%s' icon will always render!", onlyRenderWhen, objectiveTypeString));
			}
		}
	}
}

//------------------------------------------------------------------------------------
void CUIMissionObjectives::Update(float frameTime)
{
	m_onScreenIcons.clear();

#if INCLUDE_OBJECTIVE_ICON_DEBUG_CODE
	if (m_dbgListUsedIconSlots)
	{
		for (int j = 0; j < m_numGroups; ++ j)
		{
			CryWatch("Objective icon subgroup %d: %d/%d slots in use", j, m_groups[j].m_numUsed, m_groups[j].m_maxNum);
		}
	}
#endif

	for (int i = 0; i < m_maxIcons; ++ i)
	{
		SMissionObjectiveData *objective = &m_missionObjectives[i];
		SMissionObjectiveIconData *pData = objective->GetData();

#if INCLUDE_OBJECTIVE_ICON_DEBUG_CODE

#define ICON_INFO(n) objective->m_details[n].m_type ? objective->m_details[n].m_type->m_textureOne : "NULL"

		if (m_dbgListUsedIconSlots && (pData || objective->targetId))
		{
			COMPILE_TIME_ASSERT(SMissionObjectiveData::k_numPriorities == 2); // If this number changes, also change the following dbg text line!
			XDbgDisplayForEntityIDFunc (objective->targetId, string().Format("<ICON #%d> 0=%s 1=%s", i, ICON_INFO(0), ICON_INFO(1)));
		}

#undef ICON_INFO

#endif

		CRY_ASSERT_MESSAGE ((objective->targetId == 0) == (pData == NULL), string().Format("In slot %d, %s objective icon, but target ID is %d - either both should be set or both should be cleared!", i, pData ? "found" : "found no", objective->targetId));

		if (pData)
		{
			UpdateMissionObjective(objective);
		}
	}
}

//------------------------------------------------------------------------------------
void CUIMissionObjectives::UpdateMissionObjective(SMissionObjectiveData *objective)
{
	IEntity *pTarget = gEnv->pEntitySystem->GetEntity(objective->targetId);
	SMissionObjectiveIconData *pIconData = objective->GetData();

	ShouldRenderIconCallbackFunc func = pIconData->m_type->m_shouldRenderFunc;
	IActor * clientActor = g_pGame->GetIGameFramework()->GetClientActor();

	if (pTarget && clientActor && (func == NULL || func()))
	{
		// Dedicated builds have no active view but, at time of writing, can still have an instance of this class! [TF]
		IView*  pView = g_pGame->GetIGameFramework()->GetIViewSystem()->GetActiveView();
		if (!pView)
		{
			return;
		}

		const float VIRTUAL_SCREEN_X = m_layoutManager->GetVirtualWidth();
		const float VIRTUAL_SCREEN_Y = m_layoutManager->GetVirtualHeight();

		ScreenLayoutManager * layoutMgr = g_pGame->GetHUD()->GetLayoutManager();
		const float halfSafeZoneSizeFraction = (0.5f - layoutMgr->GetSafeAreaBorderScreenProportion().x) * m_scaleSafeZone;

		Vec3 vScreenSpace;
		Vec3 vWorldPos;
		float dist = 0;

		float xFraction = 0;
		float yFraction = 0;

		IActor* pActor =gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor(objective->targetId);
		if( pActor && pActor->IsPlayer() )
		{
			GetTagNamePositionScreen( pTarget, &vScreenSpace, &dist );
			GetTagNamePositionWorld( pTarget, &vWorldPos, &dist );
			xFraction = (vScreenSpace.x / VIRTUAL_SCREEN_X) - 0.5f;
			yFraction = (vScreenSpace.y / VIRTUAL_SCREEN_Y) - 0.5f;
		}
		else
		{
			// Project the objective entity into screen space
			vWorldPos = pTarget->GetWorldPos();
			vWorldPos.z += pIconData->m_type->zOffset;

			gEnv->pRenderer->ProjectToScreen(vWorldPos.x,vWorldPos.y,vWorldPos.z,&vScreenSpace.x,&vScreenSpace.y,&vScreenSpace.z);

			xFraction = vScreenSpace.x * 0.01f - 0.5f;
			yFraction = vScreenSpace.y * 0.01f - 0.5f;
		}

		const SViewParams*  pViewParams = pView->GetCurrentParams();

		const Vec3 &vViewPos = pViewParams->position;

		const Quat &qCurRot = pViewParams->rotation;
		const Vec3 vViewDir = qCurRot.GetColumn1();

		assert (clientActor->GetGameObject());
		const Vec3 vPlayerToTarget = (vWorldPos - vViewPos);
		const Vec3 vDirToTarget = vPlayerToTarget.normalized();

		const float fDist = vPlayerToTarget.len();

		if (pIconData->m_type->m_doFadeOut && fDist > pIconData->m_type->m_fadeOut.vanishAfterDistance)
		{
			return;
		}

		const float dot = vDirToTarget.Dot(vViewDir);
		const bool isBehind = dot < 0.f;

		float x, y;

		// If the icon is behind us, invert the position
		if (isBehind)
		{
			xFraction = 0.5f - xFraction;
			yFraction = 0.5f - yFraction;
		}

		int positionId = 0;
		const float midX = VIRTUAL_SCREEN_X * 0.5f;
		const float midY = VIRTUAL_SCREEN_Y * 0.5f;

#if INCLUDE_OBJECTIVE_ICON_DEBUG_CODE
		const bool clampIconsToEdge = (pIconData->arrowsTextureOne != NULL) && (m_dbgNeverUseEdgeTextures == 0);
#else
		const bool clampIconsToEdge = (pIconData->arrowsTextureOne != NULL);
#endif

		if (! clampIconsToEdge)
		{
			// If there's no 'arrows' (i.e. edge-of-screen) texture, do no clamping; let the icon escape
			// from the screen (and don't draw it at all if it's behind us)

			if (isBehind)
			{
				return;
			}
		}
		else if (m_clampToOval)
		{
			const float distanceSq = xFraction * xFraction + yFraction * yFraction;
			if (isBehind || distanceSq > (halfSafeZoneSizeFraction * halfSafeZoneSizeFraction))
			{
				const float scaleIt = halfSafeZoneSizeFraction / sqrtf(distanceSq);
				xFraction *= scaleIt;
				yFraction *= scaleIt;

				const float iconFrac = halfSafeZoneSizeFraction * 0.5f;

				// Calculate the texture co-ordinates
				positionId = ((xFraction <= -iconFrac) ? kTexCoordsPart_left : (xFraction >= iconFrac) ? kTexCoordsPart_right : kTexCoordsPart_centre) + 
					((yFraction <= -iconFrac) ? kTexCoordsPart_top  : (yFraction >= iconFrac) ? kTexCoordsPart_bottom : kTexCoordsPart_middle);
			}
		}
		else
		{
			const float distance = max(abs(xFraction), abs(yFraction));
			if (isBehind || distance >= halfSafeZoneSizeFraction)
			{
				const float scaleIt = halfSafeZoneSizeFraction / distance;
				xFraction *= scaleIt;
				yFraction *= scaleIt;

				const float iconFrac = halfSafeZoneSizeFraction * 0.95f;

				// Calculate the texture co-ordinates
				positionId = ((xFraction <= -iconFrac) ? kTexCoordsPart_left : (xFraction >= iconFrac) ? kTexCoordsPart_right : kTexCoordsPart_centre) + 
					((yFraction <= -iconFrac) ? kTexCoordsPart_top  : (yFraction >= iconFrac) ? kTexCoordsPart_bottom : kTexCoordsPart_middle);
			}
		}

		x = midX + VIRTUAL_SCREEN_X * xFraction;
		y = midY + VIRTUAL_SCREEN_Y * yFraction;

		ITexture *pTextureOne = pIconData->textureOne;
		ITexture *pTextureTwo = pIconData->textureTwo;

		const bool isOnScreen = (positionId == 0);

		if (! isOnScreen)
		{
			if (clampIconsToEdge)
			{
				pTextureOne = pIconData->arrowsTextureOne;
				pTextureTwo = pIconData->arrowsTextureTwo;
			}
			else
			{
				positionId = 0;
			}
		}

		float fSize = 1.0;

		if (pIconData->m_type->m_doShrinkWithDistance)
		{
			float useVal = isOnScreen ? pIconData->m_type->m_onScreenShrinkWithDistanceVal : pIconData->m_type->m_atEdgeShrinkWithDistanceVal;
			float scaleSizes = useVal / (useVal + fDist);
			fSize *= scaleSizes;
		}

		const float fTexSize = (isOnScreen ? pIconData->m_type->m_onScreenSize : pIconData->m_type->m_atEdgeSize) * fSize;
		const float textX = pIconData->m_type->m_text.x * fTexSize;
		const float textY = pIconData->m_type->m_text.y * fTexSize;
		const float textWidth = pIconData->m_type->m_text.width * fTexSize;
		const float textHeight = pIconData->m_type->m_text.height * fTexSize;
		const float distX = pIconData->m_type->m_dist.x * fTexSize;
		const float distY = pIconData->m_type->m_dist.y * fTexSize;
		const float distWidth = pIconData->m_type->m_dist.width * fTexSize;
		const float distHeight = pIconData->m_type->m_dist.height * fTexSize;

		m_onScreenIcons.push_back(SOnScreenIcon(objective, pTextureOne, pTextureTwo, x, y, fTexSize, fDist, dot, positionId, ((pIconData->m_type->m_doShowEntityName && isOnScreen) ? pTarget->GetName() : 0), textX, textY, textWidth, textHeight, (pIconData->m_type->m_hasDistElement && isOnScreen), distX, distY, distWidth, distHeight));
	}

	if (m_onScreenIcons.size())
	{
		std::sort(m_onScreenIcons.begin(), m_onScreenIcons.end());
	}
}

//---------------------------------------------------
void CUIMissionObjectives::Draw(void) const
{
	if (m_onScreenIcons.size())
	{
		C2DRenderUtils* pRenderUtils = g_pGame->GetHUD()->Get2DRenderUtils();

		ScreenLayoutStates prevLayoutState = m_layoutManager->GetState();
		m_layoutManager->SetState(eSLO_FullScreen);

		const float VIRTUAL_SCREEN_X = m_layoutManager->GetVirtualWidth();
		const float VIRTUAL_SCREEN_Y = m_layoutManager->GetVirtualHeight();
		const float fScreenWidth = (float) gEnv->pRenderer->GetWidth();
		const float fScreenHeight = (float) gEnv->pRenderer->GetHeight();
		const float widthMult = (fScreenHeight / VIRTUAL_SCREEN_Y) / (fScreenWidth / VIRTUAL_SCREEN_X);

		std::vector<SOnScreenIcon>::const_iterator it  = m_onScreenIcons.begin();
		std::vector<SOnScreenIcon>::const_iterator end = m_onScreenIcons.end();
		for (; it != end ; ++it)
		{
			const SMissionObjectiveIconData *pIconData = it->m_data->GetData();

			float size = it->m_renderSize;
			float fTexX = it->x - size * 0.5f * widthMult;
			float fTexY = it->y - size * 0.5f;

			float alpha = 1.f;
			float brightness = 1.f + ((it->m_dot - 1.f) * 0.5f * (1.f - m_iconBrightnessWhenBehindCamera));

			if (pIconData->m_type->m_doFadeOut && it->distance > pIconData->m_type->m_fadeOut.opaqueWithinDistance)
			{
				alpha = 1.f - (it->distance - pIconData->m_type->m_fadeOut.opaqueWithinDistance) / (pIconData->m_type->m_fadeOut.vanishAfterDistance - pIconData->m_type->m_fadeOut.opaqueWithinDistance);
			}
			ColorF whiteWithFadeOut(brightness, brightness, brightness, alpha);
			
			assert (it->m_positionId >= 0 && it->m_positionId < kTexCoordsType_num);
			const SObjectiveIconTexCoords * texCoords = & s_objectiveIconTexCoords[it->m_positionId];

			switch (pIconData->m_type->m_renderMode)
			{
				case kOIRM_draining:
				{
					const float progress = clamp(pIconData->m_progress, 0.f, 1.f);
					const float invProgress = 1.f - progress;
					if (invProgress > 0.f)
					{
						pRenderUtils->DrawImage(it->m_textureOne->GetTextureID(), fTexX, fTexY, size * widthMult, size * invProgress, 0, whiteWithFadeOut, texCoords->s0, texCoords->t0, texCoords->s1, texCoords->t0 * progress + texCoords->t1 * invProgress);
					}
					if (progress > 0.f)
					{
						const float reducedBrightness = brightness * 0.25f;
						pRenderUtils->DrawImage(it->m_textureTwo->GetTextureID(), fTexX, fTexY + size * invProgress, size * widthMult, size * progress, 0, ColorF(reducedBrightness, reducedBrightness, reducedBrightness, alpha), texCoords->s0, texCoords->t0 * progress + texCoords->t1 * invProgress, texCoords->s1, texCoords->t1);
					}
				}
				break; 

				case kOIRM_flashing:
				{
					if (g_pGameCVars->hud_objectiveIcons_flashTime != 0.f)
					{
						const float halfFlashTime = g_pGameCVars->hud_objectiveIcons_flashTime * 0.5f;
						if (cry_fmod(gEnv->pTimer->GetCurrTime(), g_pGameCVars->hud_objectiveIcons_flashTime) > halfFlashTime)
						{
							pRenderUtils->DrawImage(it->m_textureOne->GetTextureID(), fTexX, fTexY, size * widthMult, size, 0, whiteWithFadeOut, texCoords->s0, texCoords->t0, texCoords->s1, texCoords->t1);
						}
						else
						{
							pRenderUtils->DrawImage(it->m_textureTwo->GetTextureID(), fTexX, fTexY, size * widthMult, size, 0, whiteWithFadeOut, texCoords->s0, texCoords->t0, texCoords->s1, texCoords->t1);
						}
					}
				}
				break;

				case kOIRM_singleTexture:
					pRenderUtils->DrawImage(it->m_textureOne->GetTextureID(), fTexX, fTexY, size * widthMult, size, 0, whiteWithFadeOut, texCoords->s0, texCoords->t0, texCoords->s1, texCoords->t1);
					break;
			}

			if (it->name && pIconData->m_type->m_doShowEntityName )
			{
				float textX = it->textX + fTexX + (size * widthMult * 0.5f);
				float textY = it->textY + fTexY;

				ColorF textCol = pIconData->m_type->m_text.colour * whiteWithFadeOut;
				if( pIconData->m_type->m_showDistanceTo )
				{
					pRenderUtils->DrawText(textX, textY, it->fontWidth, it->fontHeight, string().Format("%s %.2fm", it->name, it->distance).c_str(), textCol, UIDRAWHORIZONTAL_CENTER);
				}
				else
				{
					pRenderUtils->DrawText(textX, textY, it->fontWidth, it->fontHeight, it->name, textCol, UIDRAWHORIZONTAL_CENTER);
				}
			}
			else if( pIconData->m_type->m_showDistanceTo )
			{
				float textX = it->textX + fTexX + (size * widthMult * 0.5f);
				float textY = it->textY + fTexY;

				ColorF textCol = pIconData->m_type->m_text.colour * whiteWithFadeOut;
				pRenderUtils->DrawText(textX, textY, it->fontWidth, it->fontHeight, string().Format("%.2fm", it->distance).c_str(), textCol, UIDRAWHORIZONTAL_CENTER);
			}

			if (it->m_showDistElement)
			{
				float distX = it->m_distEleX + fTexX + (size * widthMult * 0.5f);
				float distY = it->m_distEleY + fTexY;
				ColorF distCol = pIconData->m_type->m_dist.colour * whiteWithFadeOut;
				pRenderUtils->DrawText(distX, distY, it->m_distEleW, it->m_distEleH, string().Format("%.2fm", it->distance).c_str(), distCol, UIDRAWHORIZONTAL_CENTER);
			}
		}
		m_layoutManager->SetState(prevLayoutState);
	}

	return;
}


void CUIMissionObjectives::OnHUDEvent(const SHUDEvent& event)
{
	switch( event.eventType )
	{
	case eHUDEvent_OnNewObjective :
		{
			const EntityId objectiveId = static_cast<EntityId>( event.GetData(0).GetInt() );
			const EGameRulesMissionObjectives objectiveType = static_cast<EGameRulesMissionObjectives>( event.GetData(1).GetInt() );
			float progress = 0.0f;
			int priority = 0;

			if( event.GetDataSize() >= 3 )
			{
				progress = event.GetData(2).GetFloat();
			}

			if( event.GetDataSize() == 4 )
			{
				priority = event.GetData(3).GetInt();
			}

			NewMissionObjective( objectiveId, objectiveType, progress, priority );
		}
		break;
	case eHUDEvent_OnRemoveObjective :
		{
			EntityId targetId = static_cast<EntityId>( event.GetData(0).GetInt() );
			int priority = 0;
			if( event.GetDataSize() == 2 )
			{
				priority = event.GetData(1).GetInt();
			}
			RemoveMissionObjective( targetId, priority );
		}
		break;
	}
}

//---------------------------------------------------
void CUIMissionObjectives::NewMissionObjective( const EntityId targetId, const EGameRulesMissionObjectives objectiveType, float progress, int priority )
{
	// Remove any existing objective on the target entity
	RemoveMissionObjective(targetId, priority);

	CRY_ASSERT_MESSAGE (objectiveType >= 0 && objectiveType < EGRMO_Max, string().Format("Bad objective number %d - should be in range 0 to %d", objectiveType, EGRMO_Max - 1));

	const SMissionObjectiveType * it = & m_missionObjectiveTypes[objectiveType];
	if (it->m_group->m_numUsed < it->m_group->m_maxNum)
	{
		NewMissionObjectiveInternal(targetId, it, progress, priority);
	}
#if INCLUDE_OBJECTIVE_ICON_DEBUG_CODE
	else if (it->m_group->m_assertWhenRunOutOfSlots)
	{
		IEntity * theEntity = gEnv->pEntitySystem->GetEntity(targetId);
		CRY_ASSERT_MESSAGE(0, string().Format("Can't add a new \"%s\" icon to entity '%s'! Icon group has max num = %d", s_missionObjectiveNames[objectiveType], theEntity ? theEntity->GetName() : "NULL", it->m_group->m_maxNum));
	}
#endif
}

//---------------------------------------------------
SMissionObjectiveData * CUIMissionObjectives::GetObjectiveSlot( const EntityId targetId )
{
	SMissionObjectiveData *pEmptySlot = 0;

	for (int index = 0; index < m_maxIcons; ++index)
	{
		if (m_missionObjectives[index].targetId == targetId)
		{
			return & m_missionObjectives[index];
		}

		if (pEmptySlot == NULL && m_missionObjectives[index].targetId == 0)
		{
			pEmptySlot = &m_missionObjectives[index];
		}
	}

	return pEmptySlot;
}

//---------------------------------------------------
void CUIMissionObjectives::NewMissionObjectiveInternal( const EntityId targetId, const SMissionObjectiveType * theType, float progress, int priority)
{
	CRY_ASSERT_MESSAGE(priority < SMissionObjectiveData::k_numPriorities, "Invalid priority given for mission objective icon");
	SMissionObjectiveData *pObjectiveData = GetObjectiveSlot(targetId);
	if (pObjectiveData)
	{
		CUIElementManager* pElementMan = g_pGame->GetHUD()->GetElementManager();

		SMissionObjectiveIconData &iconData = pObjectiveData->m_details[priority];
		iconData.m_type = theType;
		iconData.m_progress = progress;
		theType->m_group->m_numUsed ++;
		pObjectiveData->targetId = targetId;
		iconData.textureOne = pElementMan->GetTexture(theType->m_textureOne);
		iconData.textureTwo = pElementMan->GetTexture(theType->m_textureTwo);
		CRY_ASSERT_MESSAGE(iconData.textureOne != 0, string().Format("Failed to load '%s' texture!", theType->m_textureOne));
		if (theType->m_arrowsTextureOne[0] != '\0')
		{
			iconData.arrowsTextureOne = pElementMan->GetTexture(theType->m_arrowsTextureOne);
			iconData.arrowsTextureTwo = pElementMan->GetTexture(theType->m_arrowsTextureTwo);
			CRY_ASSERT_MESSAGE(iconData.arrowsTextureOne != 0, string().Format("Failed to load '%s' arrows texture!", theType->m_arrowsTextureOne));
		}
		else
		{
			iconData.arrowsTextureOne = 0;
			iconData.arrowsTextureTwo = 0;
		}
		return;
	}

	CRY_ASSERT_MESSAGE(false, "Unable to find a free slot for a mission objective icon");
}

//---------------------------------------------------
void CUIMissionObjectives::RemoveMissionObjective( EntityId targetId, int priority )
{
	CRY_ASSERT_MESSAGE(priority < SMissionObjectiveData::k_numPriorities, "Invalid priority given for mission objective icon");
	for (int index = 0; index < m_maxIcons; ++index)
	{
		if (m_missionObjectives[index].targetId == targetId)
		{
			SMissionObjectiveIconData &iconData = m_missionObjectives[index].m_details[priority];
			if (iconData.m_type)
			{
				assert (iconData.m_type);
				assert (iconData.m_type->m_group);
				assert (iconData.m_type->m_group->m_numUsed >= 1);

				iconData.m_type->m_group->m_numUsed --;
				iconData.m_type = NULL;

				if (m_missionObjectives[index].GetData() == NULL)
				{
					m_missionObjectives[index].targetId = 0;
				}
			}
			return;
		}
	}
}

//---------------------------------------------------
void CUIMissionObjectives::ClearMissionObjectives()
{
	for (int index = 0; index < m_maxIcons; ++index)
	{
		for (int priorityIndex = 0; priorityIndex < SMissionObjectiveData::k_numPriorities; ++ priorityIndex)
		{
			SMissionObjectiveIconData &iconData = m_missionObjectives[index].m_details[priorityIndex];
			if (iconData.m_type)
			{
				assert (iconData.m_type);
				assert (iconData.m_type->m_group);
				assert (iconData.m_type->m_group->m_numUsed >= 1);

				iconData.m_type->m_group->m_numUsed --;
				iconData.m_type = NULL;
			}
		}
	}
}

//---------------------------------------------------
bool CUIMissionObjectives::IsMissionObjectiveTarget( EntityId targetId ) const
{
	for (int index = 0; index < m_maxIcons; ++index)
	{
		if ((m_missionObjectives[index].targetId == targetId) && m_missionObjectives[index].GetData())
		{
			return true;
		}
	}
	return false;
}
