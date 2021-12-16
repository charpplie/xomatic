#include "StdAfx.h"

#include "HUDStateManager.h"

#include "HUD/HUDState.h"
#include "HUD/HUDObjectManager.h"
#include "HUD/HUDObject.h"

#include "Game.h"
#include "GameRules.h"
#include "IItemSystem.h"

#include "Utility/CryWatch.h"

#if defined(_RELEASE) || defined(PROFILE)
#define DO_STATE_MGR_DBG_CODE			0
#else
#define DO_STATE_MGR_DBG_CODE			1
#endif

IHUDState* CHUDStateManager::s_pHUDStateNULL = new CHUDStateNull();

CHUDStateManager::CHUDStateManager( CHUDObjectManager* pObjectManager, CUIElementManager* pElementManager )
 : m_objectManager( pObjectManager )
 , m_elementManager( pElementManager )
 , m_activeState( s_pHUDStateNULL )
{
	memset(m_gameModeAddons, 0, sizeof(m_gameModeAddons));
	m_numGameModeAddons = 0;
}

CHUDStateManager::~CHUDStateManager()
{
	ClearStates();

	SAFE_DELETE(s_pHUDStateNULL);
}

void CHUDStateManager::ClearStates( void )
{
	TStates::iterator it = m_hudStates.begin();
	while( it != m_hudStates.end() )
	{
		IHUDState* pState = it->second;
		SAFE_DELETE(pState);
		it ++;
	}

	m_hudStates.clear();
	m_activeState = s_pHUDStateNULL;

	memset(m_gameModeAddons, 0, sizeof(m_gameModeAddons));
	m_numGameModeAddons = 0;
}

void CHUDStateManager::LoadStatesFromFileList( char* xml_path )
{
	XmlNodeRef xml = GetISystem()->LoadXmlFile(xml_path);
	if(!xml)
	{
		CryHUDWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, string().Format("HUD: Failed to load States file list'%s'", xml_path).c_str());
		return;
	}

	IItemParamsNode *paramNode = g_pGame->GetIGameFramework()->GetIItemSystem()->CreateParams();
	paramNode->ConvertFromXML(xml);

	const int numChildren = paramNode->GetChildCount();

	for(int i = 0; i < numChildren; ++i)
	{
		const IItemParamsNode * child = paramNode->GetChild(i);
		// Only interested in 'File' tags
		if( 0 == stricmp("File", child->GetName() ) )
		{
			// Get the info about the file from the xml.
			LoadStates( child->GetAttribute("path") ); // Conditional loading for mp vs single player is handled by the GAME="MP" attribute and filtered by FilterXMLNode().
		}
	}

	paramNode->Release();
}

void CHUDStateManager::LoadStates( const char* filename )
{
	XmlNodeRef xml = GetISystem()->LoadXmlFile(filename);
	if(!xml)
	{
		CryHUDWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, string().Format("HUD: Failed to load State xml file '%s'", filename).c_str());
		return;
	}

	IItemParamsNode *paramNode = g_pGame->GetIGameFramework()->GetIItemSystem()->CreateParams();
	paramNode->ConvertFromXML(xml);

	const int numChildren = paramNode->GetChildCount();

	for(int i = 0; i < numChildren; ++i)
	{
		const IItemParamsNode * child = paramNode->GetChild(i);

		if (!stricmp(child->GetName(), "State"))
		{
			TStateName name = "";
			bool enabled = true;

			for(int attr = 0; attr < child->GetAttributeCount(); ++attr)		
			{
				const char* key = child->GetAttributeName(attr);
				const char* value = child->GetAttribute(attr);
				if(key && value)
				{
					if(!stricmp(key, "name"))
					{
						name = value;
					}
					else if(!stricmp(key, "enabled"))
					{
						enabled = value && !stricmp(value, "1");
					}
				}
			}

			if(name.empty())
				continue;

			std::pair<TStates::iterator, bool> result = m_hudStates.insert(TStates::value_type(name, new CHUDState()));

			if(result.second)
			{
				IHUDState* pState = result.first->second;

				pState->Initialize(child,m_objectManager,m_elementManager,this);
				pState->SetActive(enabled);
			}
			else
			{
				CryHUDWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, string().Format("HUD: Ignoring duplicate state name '%s' in file '%s'", name.c_str(), filename).c_str());
			}
		}
		else if (!stricmp(child->GetName(), "StateGameModeAddon"))
		{
			CryFixedStringT<HUD_STATEMGR_GAMEMODEADDONS_TYPENAME_LEN>  typeName = "";

			for (int attr = 0; attr < child->GetAttributeCount(); ++attr)		
			{
				const char* key = child->GetAttributeName(attr);
				const char* value = child->GetAttribute(attr);
				if(key && value)
				{
					if(!stricmp(key, "type"))
					{
						typeName = value;
					}
				}
			}

			if (typeName.empty())
				continue;

			// [tlh] TODO debug code to check for duplicates

			if (m_numGameModeAddons < HUD_STATEMGR_GAMEMODEADDONS_LEN)
			{
				SHUDStateGameModeAddon*  a = &m_gameModeAddons[m_numGameModeAddons];
				m_numGameModeAddons++;
				a->Init(child, m_objectManager, m_elementManager, this);
			}
			else CRY_ASSERT_MESSAGE(0, "HUD: Whilst loading states, no more space in game mode addons array for new addons.");
		}
	}

	paramNode->Release();
}

void CHUDStateManager::DeactivateAllStates( void )
{
	TStates::iterator it  = m_hudStates.begin();
	TStates::iterator end = m_hudStates.end();
	for(;it!=end;++it)
	{
		IHUDState* hudState = (it->second);
		hudState->SetActive( false );
	}
}

IHUDState* CHUDStateManager::FindStateByName( const char* name )
{
	return FindStateByName( TStateName(name) );
}

IHUDState* CHUDStateManager::FindStateByName( const TStateName& statename )
{
	TStates::iterator it = m_hudStates.begin();

#if DO_STATE_MGR_DBG_CODE
	CryFixedStringT<128> listOfStates = "Valid HUD State are :";
#endif

	for(; it!=m_hudStates.end(); ++it )
	{
		if( it->first == statename )
		{
			return it->second;
		}

#if DO_STATE_MGR_DBG_CODE
		listOfStates.append("\n\t");
		listOfStates.append(it->first);
#endif
	}

	CryHUDWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, string().Format("HUD: Invalid HUD state '%s' requested", statename.c_str()).c_str());

#if DO_STATE_MGR_DBG_CODE
	CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "(%s)", listOfStates.c_str());
#endif

	return s_pHUDStateNULL;
}

const char* CHUDStateManager::FindStateName( const IHUDState* pState ) const
{
	if( pState == s_pHUDStateNULL)
	{
		return "<NULLHUDSTATE>";
	}

	TStates::const_iterator it = m_hudStates.begin();
	TStates::const_iterator end = m_hudStates.end();
	for(; it!=end; ++it )
	{
		if( it->second == pState )
		{
			return it->first.c_str();
		}
	}

	CRY_ASSERT_MESSAGE(0, "HUD: State not found in map!");
	return "";
}

SHUDStateGameModeAddon* CHUDStateManager::FindGameModeAddonByTypeName(const char* typeName)
{
	SHUDStateGameModeAddon*  a = NULL;
	CRY_ASSERT(m_numGameModeAddons <= HUD_STATEMGR_GAMEMODEADDONS_LEN);
	for (int i=0; i<m_numGameModeAddons; i++)
	{
		if (!stricmp(typeName, m_gameModeAddons[i].GetTypeName()))
		{
			a = &m_gameModeAddons[i];
			break;
		}
	}
	return a;
}

//------------------------------------------------------------------------------------------------------

void CHUDStateManager::ActivateState( const char* name )
{
	ActivateState( TStateName(name) );
}

void CHUDStateManager::ActivateState( const TStateName& name )
{  
#if DO_STATE_MGR_DBG_CODE
	CryLog( "HUD: Changing HUD State to '%s' (from '%s')", name.c_str(), FindStateName(m_activeState) );
#endif

	if(IHUDState* pState = FindStateByName(name) )
	{
		if( pState != m_activeState )
		{
			DeactivateAllStates( ); // Turn everything currently on, off.
			pState->SetActive(true);
			m_activeState = pState;
		}
	}
}

void CHUDStateManager::UpdateStates( float frameTime )
{
	m_activeState->Update(frameTime); 
}

void SHUDModeAddonSet::Init(const IItemParamsNode* pnode, CHUDObjectManager* pObjectManager, CUIElementManager* pElementManager, CHUDStateManager* pStateManager)
{
	memset(m_objs, 0, sizeof(m_objs));
	m_numObjs = 0;

	const int  numChildren = pnode->GetChildCount();

	for (int i=0; i<numChildren; i++)
	{
		const IItemParamsNode*  child = pnode->GetChild(i);

		if (!stricmp(child->GetName(), "HUDObjectRef"))
		{	
			if (const char* objName=child->GetAttribute("name"))
			{
				if (CHUDObject* pObject=pObjectManager->FindObjectByName(objName))
				{
					if (m_numObjs < HUD_STATEMGR_MODEADDONSET_OBJS_LEN)
					{
						m_objs[m_numObjs] = pObject;
						m_numObjs++;
					}
					else CRY_ASSERT_MESSAGE(0, string().Format( "HUD: Whilst initializing ModeAddonSet, no more space in objects array for object reference '%s'.", objName));
				}
				else CRY_ASSERT_MESSAGE(0, string().Format( "HUD: Whilst initializing ModeAddonSet could not find object reference '%s'.", objName));
			}
			else CRY_ASSERT_MESSAGE(0, "HUD: No \"name\" attribute found in HUDObjectRef child node.");
		}
		else CRY_ASSERT_MESSAGE(0, string().Format("HUD: Wasn't expecting a child node called '%s'!", child->GetName()));
	}
}

void SHUDModeAddonSet::Update(float ftime)
{
	for (int i=0; i<m_numObjs; i++)
	{
		CRY_ASSERT(m_objs[i]);
		m_objs[i]->Update(ftime);
	}
}

void SHUDModeAddonSet::Draw()
{
	for (int i=0; i<m_numObjs; i++)
	{
		CRY_ASSERT(m_objs[i]);
		m_objs[i]->Draw();
	}
}

//------------------------------------------------------------------------------------------------------

void SHUDStateGameModeAddon::Init(const IItemParamsNode* pnode, CHUDObjectManager* pObjectManager, CUIElementManager* pElementManager, CHUDStateManager* pStateManager)
{
	memset(m_modeSets, 0, sizeof(m_modeSets));
	memset(m_typeName, 0, sizeof(m_typeName));

	if (const char* typeName=pnode->GetAttribute("type"))
	{
		strncpy(m_typeName, typeName, sizeof(m_typeName));
		CRY_ASSERT(m_typeName[sizeof(m_typeName)-1] == '\0');
		m_typeName[sizeof(m_typeName)-1] = '\0';
	}
	else CRY_ASSERT_MESSAGE(0, "HUD: No \"type\" attribute found for this StateGameModeAddon.");

	const int  numChildren = pnode->GetChildCount();

	for (int i=0; i<numChildren; i++)
	{
		const IItemParamsNode*  child = pnode->GetChild(i);

		if (!stricmp(child->GetName(), "Mode"))
		{
			if (const char* modeName=child->GetAttribute("name"))
			{
				int  modeidx;
				bool  modeOk = AutoEnum_GetEnumValFromString(modeName, CGameRules::S_GetGameModeNamesArray(), eGM_NUM_GAMEMODES, &modeidx);
				if (modeOk && ((modeidx >= 0) && (modeidx < eGM_NUM_GAMEMODES)))
				{
					m_modeSets[modeidx].Init(child, pObjectManager, pElementManager, pStateManager);
				}
				else CRY_ASSERT_MESSAGE(0, string().Format( "HUD: Whilst initializing SHUDStateGameModeAddon type '%s', mode name '%s' did not translate into a valid array idx.", m_typeName, modeName ));
			}
			else CRY_ASSERT_MESSAGE(0, string().Format( "HUD: Whilst initializing SHUDStateGameModeAddon type '%s', no \"name\" attribute found for this ModeAddonSet.", m_typeName ));
		}
	}
}

void SHUDStateGameModeAddon::Update(const EGameMode gameModeIdx, float ftime)
{
	if ((gameModeIdx >= 0) && (gameModeIdx < eGM_NUM_GAMEMODES))
	{
		m_modeSets[gameModeIdx].Update(ftime);
	}
	else CRY_ASSERT_MESSAGE(0, string().Format( "HUD: Current game mode (%d) is not a valid modeSets array idx.", gameModeIdx ));
}

void SHUDStateGameModeAddon::Draw(const EGameMode gameModeIdx)
{
	if ((gameModeIdx >= 0) && (gameModeIdx < eGM_NUM_GAMEMODES))
	{
		m_modeSets[gameModeIdx].Draw();
	}
	else CRY_ASSERT_MESSAGE(0, string().Format( "HUD: Current game mode (%d) is not a valid modeSets array idx.", gameModeIdx ));
}

#if ENABLE_HUD_EXTRA_DEBUG
void CHUDStateManager::PrintStateInfo( void ) const 
{
	TStates::const_iterator it = m_hudStates.begin();
	TStates::const_iterator end = m_hudStates.end();
	for(; it!=end; ++it)
	{
		const string& stateName = it->first;
		IHUDState* pState = it->second;
		CryWatch( "%s : %s", stateName.c_str(), pState->IsActive() ? "Active" : "Inactive" );		
		PrintStateObjectInfo(pState);
	}
}
#endif

#if ENABLE_HUD_EXTRA_DEBUG
void CHUDStateManager::PrintStateObjectInfo( const IHUDState* pState ) const
{
	if( pState == s_pHUDStateNULL )
	{
		CryWatch( "TRYING TO SHOW INFO FOR NULLSTATE!" );
		return;
	}
	
	const THUDObjects* pObjects = pState->GetObjects();
	THUDObjects::const_iterator objects_it = pObjects->begin();
	THUDObjects::const_iterator objects_end = pObjects->end();
	CryWatch( "%s :", FindStateName(pState) );
	for(; objects_it!=objects_end; ++objects_it)
	{
		const CHUDObject* pObject = *objects_it;
		const char* objName = m_objectManager->FindObjectName( pObject );
		CryWatch( "\t %s (HUDOBJ)", objName );
	}

	const TUIElements* pElements = pState->GetElements();
	TUIElements::const_iterator elements_it = pElements->begin();
	TUIElements::const_iterator elements_end = pElements->end();
	for(; elements_it!=elements_end; ++elements_it)
	{
		const IUIElement* pUIElement = *elements_it;
		CryWatch( "\t %s (UI)", pUIElement->GetName() );
	}

	CryWatch( "State Refs :" );
	const TRequiredStates* pDepStates = pState->GetDepStates();
	TRequiredStates::const_iterator it_state = pDepStates->begin();
	TRequiredStates::const_iterator end_state = pDepStates->end();
	for(; it_state!=end_state; ++it_state)
	{
		CryWatch( "SUBSTATE: %s", FindStateName(*it_state) );
		PrintStateObjectInfo( *it_state );
	}
}
#endif
