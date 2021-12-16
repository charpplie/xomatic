#include "StdAfx.h"
#include "HUD/UIElementManager.h"
#include "Utility/CryWatch.h"

#include <IItemSystem.h>

#if defined(PS3)
#define LOAD_PS3_CONTROLLER_ICONS		1
#else
#define LOAD_PS3_CONTROLLER_ICONS		0
#endif

CUIElementManager::CUIElementManager()
{
	//...
}

CUIElementManager::~CUIElementManager()
{
	Clear();
}

void CUIElementManager::Clear( void )
{
	ClearTextures();
	ClearElements();
}

//******************************************************************************
void CUIElementManager::ClearTextures( void )
{
	m_textures.clear();
}

//******************************************************************************
void CUIElementManager::ClearElements()
{
	std::list<CUIElement*>::iterator it = m_elements.begin();
	std::list<CUIElement*>::iterator end = m_elements.end();
	while(it!=end)
	{
		SAFE_DELETE( (*it) );
		++it;
	}

	m_elements.clear();
	assert(!m_elements.size());
}


void CUIElementManager::LoadTextures( const string& filename )
{
	XmlNodeRef xmlUITextures = GetISystem()->LoadXmlFile( filename.c_str() );
	CRY_ASSERT_MESSAGE(xmlUITextures, string().Format("[HUD_Impl] Failed to load textures from '%s'!", filename.c_str()) );
	if(!xmlUITextures)
		return;

	IItemParamsNode *paramNode = g_pGame->GetIGameFramework()->GetIItemSystem()->CreateParams();
	paramNode->ConvertFromXML(xmlUITextures);

	int iTextures = paramNode->GetChildCount();

	for(int i = 0; i < iTextures; ++i)
	{
		const IItemParamsNode * xmlTexNode = paramNode->GetChild(i);

		const char * texName = xmlTexNode->GetAttribute("name");
		const char * texPath = xmlTexNode->GetAttribute("path");

		if(texName && texPath)
		{
			ITexture *texture = gEnv->pRenderer->EF_LoadTexture(texPath, FT_DONT_STREAM|FT_DONT_RESIZE,eTT_2D);
			if(texture)
				m_textures[texName] = texture;
			else
				CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "Failed to load UITexture : %s / %s", texName, texPath);
		}
		else
			CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "UITexture not set up correctly : %s / %s", texName, texPath);
	}

	paramNode->Release();
}

//******************************************************************************
XmlNodeRef CUIElementManager::LoadElementXML( const string& filename )
{
	XmlNodeRef xmlUI = GetISystem()->LoadXmlFile( filename.c_str() );

	if(!xmlUI)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "Warning: HUD File %s not found!", filename.c_str() );
	}

	return xmlUI;
}

//******************************************************************************
void CUIElementManager::LoadElements( const string& filename, const CUIElement::UI2AnimationInfo* ainfo /*= NULL*/, std::vector<string> *elementsLoaded /* = NULL */ )
{
	XmlNodeRef xmlUI = LoadElementXML( filename );
	LoadElements(xmlUI, ainfo, elementsLoaded );
}

//******************************************************************************
void CUIElementManager::LoadElements( const XmlNodeRef& xmlUI , const CUIElement::UI2AnimationInfo* ainfo /*= NULL*/, std::vector<string> *elementsLoaded /* = NULL */ )
{
	CRY_ASSERT_MESSAGE( xmlUI, "UI2 : LoadElements() : Invalid XMLNode." );

	if( !xmlUI ) // failed xml should report in console (with line errors) and not cause a fatal crash, yet.
		return;

	IItemParamsNode *paramNode = g_pGame->GetIGameFramework()->GetIItemSystem()->CreateParams();
	paramNode->ConvertFromXML(xmlUI);

	const int iNumElements = xmlUI->getChildCount();
	for(int i = 0; i < iNumElements; ++i)
	{
		const IItemParamsNode * xmlElement = paramNode->GetChild(i);
		CUIElement *pElement = new CUIElement();
		pElement->Initialize(xmlElement, NULL);

#if INCLUDE_UI_ELEMENT_ANIMATION_CODE
		if( ainfo )
		{
			pElement->SetAnimation( *ainfo );
		}
#endif

		m_elements.push_back(pElement);
		if (elementsLoaded)
		{
			elementsLoaded->push_back(pElement->GetName());
		}
	}

	paramNode->Release();
}

void CUIElementManager::Load( void )
{
	// Load _ALL_ textures first.
	LoadTextures("Scripts/UI/UI2Textures.xml");
	if(gEnv->bMultiplayer)
	{
		LoadTextures("Scripts/UI/UI2Textures_cw2.xml");
	}

	// Then Load elements.
	LoadElements("Scripts/UI/UI2.xml");
	if(gEnv->bMultiplayer)
	{
		LoadElements("Scripts/UI/UI2_cw2_elements.xml");
		LoadElements("Scripts/UI/UI2_cw2_notification.xml");
		LoadElements("Scripts/UI/UI2_cw2_spectator.xml");
	}

#if LOAD_PS3_CONTROLLER_ICONS
	LoadTextures("Scripts/UI/UI2Textures_ps3pad.xml");
#else
	LoadTextures("Scripts/UI/UI2Textures_360pad.xml");
#endif
}

//---------------------------------------------------
IUIElement* CUIElementManager::FindElement(const char *pName) const
{
	std::list<CUIElement*>::const_iterator it = m_elements.begin();
	std::list<CUIElement*>::const_iterator end = m_elements.end();
	while(it!=end)
	{
		const char* currentElementName = (*it)->GetName();
		if (!stricmp(currentElementName, pName))
			return *it;
		++it;
	}

	return NULL;
}

//---------------------------------------------------
IUIElement* CUIElementManager::FindSubElement(const char *pName)
{
	std::list<CUIElement*>::iterator it = m_elements.begin();
	std::list<CUIElement*>::iterator end = m_elements.end();
	while(it!=end)
	{
		const char* currentElementName = (*it)->GetName();
		if (!stricmp(currentElementName, pName))
			return *it;

		IUIElement* searchElement = (*it)->GetSubElement(pName);
		if (searchElement)
			return searchElement;

		++it;
	}

	return NULL;
}


ITexture * CUIElementManager::GetTexture(const char * textureName) const
{
#if defined(USER_timf) || defined(USER_frankh)
	if (g_pGame->GetIGameFramework()->GetClientActor())
	{
		CryWatch("$4WARNING:$o Doing run-time look up of '%s' texture!", textureName);
	}
#endif

	std::map<CryFixedStringT<32>, ITexture*>::const_iterator it = m_textures.find(textureName);
	return (it != m_textures.end()) ? it->second : NULL;
}
