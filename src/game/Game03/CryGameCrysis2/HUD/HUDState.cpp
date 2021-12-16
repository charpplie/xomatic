#include "StdAfx.h"
#include "HUDState.h"

#include "HUD/HUD.h"
#include "HUD/HUDStateManager.h"
#include "HUD/HUDObjectManager.h"
#include "HUD/UIElementManager.h"

#include "HUD/UI/IUIElement.h"

#include "GameRules.h"

#include "Utility/CryWatch.h"
#include "Utility/DesignerWarning.h"

#include "IItemSystem.h"

#include "StlUtils.h"

//////////////////////////////////////////////////////////////////////////


CHUDState::CHUDState() : m_active(false)
{
#if ENABLE_HUD_TESTS
	m_stateManager = NULL;
	m_objectManager = NULL;
#endif
}

CHUDState::~CHUDState()
{
	// clear the references to objects and other states required by this one.
	m_objects.clear();
	m_otherRequiredStates.clear();
	m_modeAddons.clear();
}

void CHUDState::Initialize(const IItemParamsNode * state, CHUDObjectManager* pObjectManager, CUIElementManager* pElementManager, CHUDStateManager* pStateManager)
{
#if ENABLE_HUD_TESTS
	m_stateManager = pStateManager;
	m_objectManager = pObjectManager;
#endif

	const int iNumStateChildren = state->GetChildCount();
	for(int i = 0; i < iNumStateChildren; ++i)
	{		
		const IItemParamsNode * child = state->GetChild(i);
		if( stricmp( child->GetName(), "HUDObjectRef" ) )
		{
			if( stricmp( child->GetName(), "HUDStateRef" ) )
			{
				if( stricmp( child->GetName(), "HUDUIRef" ) )
				{
					if( stricmp( child->GetName(), "HUDAddonRef" ) )
					{
						CRY_ASSERT_MESSAGE( 0, string().Format( "HUD: Unknown tag found initializing state was expecting one of \"HUDObjectRef\", \"HUDStateRef\", \"HUDUIRef\" or \"HUDAddonRef\". Instead got '%s'.", child->GetName() ) );
						continue;
					}

					const char* typeName = child->GetAttribute("type");
					SHUDStateGameModeAddon* pAddons = pStateManager->FindGameModeAddonByTypeName(typeName);
					CRY_ASSERT_MESSAGE( pAddons, string().Format( "HUD: GameModeAddon of type '%s' not found in HUD_Impl!.", typeName ) );
					if (pAddons)
					{
						m_modeAddons.push_back(pAddons);
					}
					continue;
				}
				
				IUIElement* pUiElement = pElementManager->FindSubElement(child->GetAttribute( "name" ));
				CRY_ASSERT_MESSAGE( pUiElement, string().Format( "HUD: UI Element '%s' not found in HUD_Impl!.", child->GetAttribute( "name" ) ) );
				if(pUiElement)
				{
					m_uiElements.push_back(pUiElement);
				}
				continue;
			}

			const char* depState_name = child->GetAttribute( "name" );
			IHUDState* depState = pStateManager->FindStateByName(depState_name);
			DesignerWarning( depState != CHUDStateManager::s_pHUDStateNULL, "HUD: HUD State '%s' depends on '%s' which does not exist (yet)!", pStateManager->FindStateName(this), depState_name );
			m_otherRequiredStates.push_back( depState ); // could be null state, find state should log warnings.
			continue;
		}

		// Is this object an overlay (e.g. suit-menu, cross-hair, etc.) 
		const char* objectName = "";
		bool is_overlay = false;

		const int iNumAttributes = child->GetAttributeCount();
		for(int j=0; j<iNumAttributes; j++ )
		{
			const char* attributeName = child->GetAttributeName(j);
			if( !stricmp( attributeName, "name" ) )
			{		
				objectName = child->GetAttribute( "name" );
			}
			else if( !stricmp( attributeName, "isoverlay" ) )
			{
				is_overlay = true;
			}
			else
			{
				CRY_ASSERT_MESSAGE( 0, string().Format( "HUD: Whilst initializing state could not find found unknown attribute for HUDOBject! '%s'.", child->GetAttribute( "name" ) ) );
			}
		}


		CHUDObject* pObject = pObjectManager->FindObjectByName( objectName );
		CRY_ASSERT_MESSAGE( pObject, string().Format( "HUD: Whilst initializing state could not find object reference '%s'.", objectName ) );
		if(pObject)
		{
			m_objects.push_back(pObject);

#if ENABLE_HUD_TESTS
			if( is_overlay )
			{
#if IS_HUD_DEV
				CryLog( "HUD: Registering '%s.%s' as overlay : '%s'.", pStateManager->FindStateName(this), objectName, pObjectManager->FindObjectName(pObject) );
#endif //IS_HUD_DEV
				m_overlayObjects.push_back( pObject );
			}

			// Check sub-node for Allowed Overlays - object-object-specific
			const int iNumRefChildren = child->GetChildCount();
			for(int j=0; j<iNumRefChildren; j++)
			{
				const IItemParamsNode * refChild = child->GetChild(j);
				const char* refChildName = refChild->GetName();
				if( !stricmp( refChildName, "AllowedOverlaps" ) )
				{
					const int iNumAllowedOverlaps = refChild->GetChildCount();
					THUDObjects allowedObjects;
					allowedObjects.reserve( iNumAllowedOverlaps );
					for(int k=0; k<iNumAllowedOverlaps; ++k)
					{
						const IItemParamsNode * allowedOverlayNode = refChild->GetChild(k);
						CRY_ASSERT_MESSAGE( !stricmp( "ref", allowedOverlayNode->GetName() ), "HUD: Unknown tag in allowed overlays!" );
						CHUDObject* pAllowedObject = pObjectManager->FindObjectByName( allowedOverlayNode->GetAttribute("name") );
						allowedObjects.push_back( pAllowedObject );
					}

					m_allowedOverlaps.insert(THUDObjectsOkToOverlap::value_type(pObject, allowedObjects));
				}
				else
				{
					CRY_ASSERT_MESSAGE( 0, string().Format( "HUD: Whilst initializing state found unkown tag '%s' for object '%s'.", refChildName, objectName ) );
				}
			}

			const char* activeStateName = pStateManager->FindStateName(this);

			TRequiredStates::const_iterator it_subState = m_otherRequiredStates.begin();
			TRequiredStates::const_iterator end_subState = m_otherRequiredStates.end();
			for( ; it_subState!=end_subState; ++it_subState )
			{
				const IHUDState* subState = *it_subState;
				const char* subStateName = pStateManager->FindStateName(subState);

#if IS_HUD_DEV
				CryLog( "HUD: '%s' : Copying Allowed Overlaps from State '%s'.", activeStateName, subStateName );
#endif //IS_HUD_DEV
				
				const THUDObjectsOkToOverlap* sub_allowedOverlaps = subState->GetAllowedObjectObjectOverlaps();
				
				THUDObjectsOkToOverlap::const_iterator it_subObjectAllowedOverlap = sub_allowedOverlaps->begin();
				THUDObjectsOkToOverlap::const_iterator end_subObjectAllowedOverlap = sub_allowedOverlaps->end();
				for( ; it_subObjectAllowedOverlap!=end_subObjectAllowedOverlap; ++it_subObjectAllowedOverlap )
				{
					const CHUDObject* pSubObject = it_subObjectAllowedOverlap->first;
					const THUDObjects& subObjectsAllowed = it_subObjectAllowedOverlap->second;

					THUDObjects::const_iterator it_allowedSubObject = subObjectsAllowed.begin();
					THUDObjects::const_iterator end_allowedSubObject = subObjectsAllowed.end();
					for( ; it_allowedSubObject!=end_allowedSubObject; ++it_allowedSubObject )
					{
						const CHUDObject* pOtherSubObject = *it_allowedSubObject;
#if IS_HUD_DEV
						CryLog( "HUD: \t\t'%s' & '%s'", pObjectManager->FindObjectName(pSubObject), pObjectManager->FindObjectName(pOtherSubObject) );
#endif //IS_HUD_DEV
					}

					THUDObjectsOkToOverlap::iterator it_objectAllowedInfo = m_allowedOverlaps.find( pSubObject );
					THUDObjectsOkToOverlap::const_iterator end_objectAllowedInfo = m_allowedOverlaps.end();
					
					if( it_objectAllowedInfo == end_objectAllowedInfo )
					{ 
						// not found, dump.
						m_allowedOverlaps.insert(THUDObjectsOkToOverlap::value_type(pSubObject, subObjectsAllowed));
					}
					else
					{
						THUDObjects& objectsAllowed = it_objectAllowedInfo->second;
						objectsAllowed.insert( objectsAllowed.end(), subObjectsAllowed.begin(), subObjectsAllowed.end()  );
					}
				}
			}

#endif // ENABLE_HUD_TESTS
		}
	}
}

// TODO : Recursion checks.
void CHUDState::Update(float frameTime)
{
	THUDObjects::const_iterator it = m_objects.begin();
	THUDObjects::const_iterator end = m_objects.end();
	for(; it!=end; ++it)
	{
		(*it)->Update(frameTime);
	}

	const EGameMode  gameModeIdx = g_pGame->GetGameRules()->GetGameMode();

	TModeAddonsList::const_iterator addons_it = m_modeAddons.begin();
	TModeAddonsList::const_iterator addons_end = m_modeAddons.end();
	for(; addons_it!=addons_end; ++addons_it)
	{
		(*addons_it)->Update(gameModeIdx, frameTime);
	}

	TRequiredStates::iterator state_it = m_otherRequiredStates.begin();
	TRequiredStates::iterator state_end = m_otherRequiredStates.end();
	for(;state_it!=state_end;++state_it)
	{
		IHUDState* otherState = *state_it;
		otherState->Update( frameTime );
	}

	TUIElements::const_iterator ui_it = m_uiElements.begin();
	TUIElements::const_iterator ui_end = m_uiElements.end();
	for(; ui_it!=ui_end; ++ui_it)
	{
		(*ui_it)->Update(frameTime);
	}
}

// TODO : Recursion checks.
void CHUDState::Draw()
{
	TUIElements::const_iterator ui_it = m_uiElements.begin();
	TUIElements::const_iterator ui_end = m_uiElements.end();
	for(; ui_it!=ui_end; ++ui_it)
	{
		(*ui_it)->Draw();
	}

	THUDObjects::const_iterator it = m_objects.begin();
	THUDObjects::const_iterator end = m_objects.end();
	for(; it!=end; ++it)
	{
		(*it)->Draw();
	}

	const EGameMode  gameModeIdx = g_pGame->GetGameRules()->GetGameMode();

	TModeAddonsList::const_iterator addons_it = m_modeAddons.begin();
	TModeAddonsList::const_iterator addons_end = m_modeAddons.end();
	for(; addons_it!=addons_end; ++addons_it)
	{
		(*addons_it)->Draw(gameModeIdx);
	}

	TRequiredStates::iterator state_it = m_otherRequiredStates.begin();
	TRequiredStates::iterator state_end = m_otherRequiredStates.end();
	for(;state_it!=state_end;++state_it)
	{
		IHUDState* otherState = *state_it;
		otherState->Draw( );
	}
}

void CHUDState::SetActive(bool active) 
{ 
	m_active = active; 
	TRequiredStates::iterator  it = m_otherRequiredStates.begin();
	TRequiredStates::iterator end = m_otherRequiredStates.end();
	for(;it!=end;++it)
	{
		IHUDState* otherState = *it;
		if( !otherState->IsActive() )
		{
			otherState->SetActive( true );
		}
	}
}

#if ENABLE_HUD_TESTS
TRequiredStates CHUDState::GetDepAncestralStates() const
{
	TRequiredStates ret_states = m_otherRequiredStates;

	TRequiredStates::const_iterator it_parentState = m_otherRequiredStates.begin();
	TRequiredStates::const_iterator end_parentState = m_otherRequiredStates.end();
	for(; it_parentState!=end_parentState; ++it_parentState)
	{
		IHUDState* pParentState = *it_parentState;
		TRequiredStates ancestorStates = pParentState->GetDepAncestralStates();

		TRequiredStates::iterator it_ancState = ancestorStates.begin();
		TRequiredStates::const_iterator end_ancState = ancestorStates.end();
		for( ; it_ancState!= end_ancState; ++it_ancState )
		{
			stl::push_back_unique( ret_states, *it_ancState );
		}
	}
	return ret_states;
};

bool CHUDState::IsObjectAnOverlay( const CHUDObject* pObject ) const
{
	THUDObjects::const_iterator it = m_overlayObjects.begin();
	THUDObjects::const_iterator end = m_overlayObjects.end();

	for( ; it!=end; ++it )
	{
		if( pObject == *it)
		{
			CryLog( "HUD: Object is overlay : '%s.%s'.", m_stateManager->FindStateName(this), m_objectManager->FindObjectName(pObject) );
			return true;
		}
	}
	return false;
}

bool CHUDState::IsObjectObjectOverlapAllowed( const CHUDObject* pObject, const CHUDObject* pObject2 ) const 
{
	THUDObjectsOkToOverlap::const_iterator ao1 = m_allowedOverlaps.find( pObject );

	if( ao1 != m_allowedOverlaps.end() )
	{
		const THUDObjects& o1 = ao1->second;
		THUDObjects::const_iterator it = o1.begin();
		THUDObjects::const_iterator end = o1.end();
		for( ; it!=end; ++it )
		{
			if( *it == pObject2 )
				return true;
		}
	}

	THUDObjectsOkToOverlap::const_iterator ao2 = m_allowedOverlaps.find( pObject2 );

	if( ao2 != m_allowedOverlaps.end() )
	{
		const THUDObjects& o2 = ao2->second;
		THUDObjects::const_iterator it = o2.begin();
		THUDObjects::const_iterator end = o2.end();
		for( ; it!=end; ++it )
		{
			if( *it == pObject )
				return true;
		}
	}

	return false;
}
#endif // ENABLE_HUD_TESTS

//////////////////////////////////////////////////////////////////////////