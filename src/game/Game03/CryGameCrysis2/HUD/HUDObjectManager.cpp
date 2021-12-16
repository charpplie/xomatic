#include "StdAfx.h"

#include "HUDObjectManager.h"

#include "HUD/HUD.h"
#include "HUD/HUDObject.h"
#include "HUD/HUDObjectFactory.h"
#include "IItemSystem.h"
#include "HUD/HUD_UnifiedAsset.h"

#include "Game.h"

struct SHOM_XMLFileInfo{
	CryFixedStringT<255> fname;
	IItemParamsNode*     pnode;
};
typedef std::vector<SHOM_XMLFileInfo> THOM_Files;

CHUDObjectManager::CHUDObjectManager( )
{
	m_objects.clear();

	m_pObjectFactory = new CHUDObjectFactory();
}

CHUDObjectManager::~CHUDObjectManager( )
{
	ClearObjects( );
	
	SAFE_DELETE(CHUD_UnifiedAsset::s_pHUDAssetUnified2D);
	SAFE_DELETE(CHUD_UnifiedAsset::s_pHUDAssetUnified3D);
	delete m_pObjectFactory;
}

void CHUDObjectManager::LoadObject( const char* xml_path, const IItemParamsNode * xml_bit )
{
	if( stricmp( xml_bit->GetName(), "HUDObject" ) )
	{
		CryHUDWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, string().Format("HUD: '%s' Tag is not a HUDObject! ('%s')", xml_bit->GetName(), xml_path).c_str() );
		return;
	}

	string class_name = xml_bit->GetAttribute( "class" );
	if( class_name.size() == 0 )
	{
		CryHUDWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, string().Format("HUD: No class attribute found for HUDObject tag! ('%s')", xml_path).c_str());
		return;
	}

	CryFixedStringT<32> name = xml_bit->GetAttribute( "name" );
	if( name.size() == 0 )
	{
		CryHUDWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, string().Format("HUD: No name found for HUD Object of '%s' Tag is not a CHUDObject! ('%s')", class_name.c_str(), xml_path).c_str());
		return;
	}

	if( FindObjectByName( name.c_str() ) )
	{
		CryHUDWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, string().Format("HUD: HUDObject names must be unique! Duplicate found for '%s' in '%s'.", name.c_str(), xml_path).c_str());
		return;
	}

	CHUDObject* pObject = GetHUDObjectInstance( class_name.c_str() );

	if(!pObject)
	{
		return;
	}

	pObject->Initialize(xml_bit);

	m_objects.push_back(pObject);
	m_currentLoadedObjects.push_back(pObject);

	m_objectsNameMap[name.MakeLower()] = pObject;
}

void CHUDObjectManager::UnloadObject( const CHUDObject* objectToUnload )
{
	THUDObjectList::iterator it = m_objects.begin();
	while(it!=m_objects.end())
	{
		if( *it == objectToUnload )
		{
			SAFE_DELETE(objectToUnload);
			m_objects.erase( it );
			return;
		}
	}
}

IItemParamsNode* CHUDObjectManager::LoadObjectsFile( const char* xml_path ) const
{
	XmlNodeRef hud_objects = GetISystem()->LoadXmlFile(xml_path);

	if(!hud_objects)
	{
		CryHUDWarning( VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, string().Format( "HUD: FAILED to load file '%s'", xml_path ) );
		return 0;
	}

	IItemParamsNode *paramNode = g_pGame->GetIGameFramework()->GetIItemSystem()->CreateParams();
	paramNode->ConvertFromXML(hud_objects);

	return paramNode;
}

int CHUDObjectManager::GetObjectsInNode( const IItemParamsNode* hud_objects )
{
	if(!hud_objects)
	{
		return 0;
	}
	return hud_objects->GetChildCount();
}

void CHUDObjectManager::LoadObjects( const char* xml_path, const IItemParamsNode* hud_objects )
{
	if(!hud_objects)
	{
		CryHUDWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, string().Format("HUD: Failed to load HUDObjects xml file '%s'", xml_path).c_str());
		return;
	}

	const int number_objects = hud_objects->GetChildCount();
	m_currentLoadedObjects.clear();
	m_currentLoadedObjects.reserve(number_objects);

	for(int objId = 0; objId < number_objects; ++objId)
	{
		const IItemParamsNode * child = hud_objects->GetChild(objId);
		LoadObject( xml_path, child );
	}// for objId

	if( CHUD_UnifiedAsset::s_pHUDAssetUnified3D == CHUD::s_pHUDAssetNULL )
	{
		CryHUDWarning( VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "HUD: calling post Init when CHUD_UnifiedAsset::s_pHUDAssetUnified3D == CHUD::s_pHUDAssetNULL" );
	}
	else if( CHUD_UnifiedAsset::s_pHUDAssetUnified2D == CHUD::s_pHUDAssetNULL )
	{
		CryHUDWarning( VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "HUD: calling post Init when CHUD_UnifiedAsset::s_pHUDAssetUnified2D == CHUD::s_pHUDAssetNULL" );
	}

	THUDObjectList::iterator it = m_currentLoadedObjects.begin();
	while(it!=m_currentLoadedObjects.end())
	{
		(*it)->Init();
		++it;
	}
}

void CHUDObjectManager::ClearObjects( void )
{
	THUDObjectList::iterator it = m_objects.begin();
	while(it!=m_objects.end())
	{
		(*it)->PreDelete();
		++it;
	}

	it = m_objects.begin();
	while(it!=m_objects.end())
	{
		SAFE_DELETE(*it);
		it ++;
	}

	m_objects.clear();
	m_objectsNameMap.clear();
}

void CHUDObjectManager::LoadObjectsFromFileList( char* xml_path )
{
	XmlNodeRef xml = GetISystem()->LoadXmlFile(xml_path);
	if(!xml)
	{
		CryHUDWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, string().Format("HUD: Failed to load HUD Objects xml file '%s'", xml_path).c_str());
		return;
	}

	IItemParamsNode *paramNode = g_pGame->GetIGameFramework()->GetIItemSystem()->CreateParams();
	paramNode->ConvertFromXML(xml);

	int numberObjectsToLoad = 0;
	const int childXmlNodeCount = paramNode->GetChildCount();
	THOM_Files files;
	for(int i = 0; i < childXmlNodeCount; ++i)
	{
		const IItemParamsNode * child = paramNode->GetChild(i);
		// Only interested in 'File' tags
		if( 0 == stricmp("File", child->GetName() ) )
		{
			SHOM_XMLFileInfo fi;
			fi.fname = child->GetAttribute("path");
			fi.pnode = LoadObjectsFile( fi.fname.c_str() );
			if( fi.pnode )
			{
				files.push_back(fi);
				numberObjectsToLoad += GetObjectsInNode(fi.pnode);
			}
		}
	}

	m_objects.reserve( numberObjectsToLoad );

	THOM_Files::iterator file_it = files.begin();
	THOM_Files::iterator file_end = files.end();
	for(; file_it != file_end; ++file_it )
	{
		const char* fname = file_it->fname.c_str();
		const IItemParamsNode* pParamNode = file_it->pnode;

		// Get the info about the file from the xml.
		LoadObjects( fname, pParamNode ); // Conditional loading for mp vs single player is handled by the GAME="MP" attribute and filtered by FilterXMLNode().
		pParamNode->Release();
	}
	paramNode->Release();
}

CHUDObject* CHUDObjectManager::FindObjectByName( const char* object_ref_name )
{
	CryFixedStringT<32> object_name( object_ref_name );
	object_name.MakeLower();
	THUDObjectNameMap::iterator it = m_objectsNameMap.find(object_name);
	if( it == m_objectsNameMap.end() )
	{
		return NULL;
	}

	return it->second;
}


const char* CHUDObjectManager::FindObjectName( const CHUDObject* pObject ) const
{
	THUDObjectNameMap::const_iterator it = m_objectsNameMap.begin();
	THUDObjectNameMap::const_iterator end = m_objectsNameMap.end();
	for(; it!=end; ++it )
	{
		if( it->second == pObject )
		{
			return it->first.c_str();
		}
	}

	CRY_ASSERT_MESSAGE(0, "HUD: Object not found in map!");
	return "";
}


CHUDObject* CHUDObjectManager::GetHUDObjectInstance(const char* className)
{
	return m_pObjectFactory->GetClassInstance(className);
}

void CHUDObjectManager::GetMemoryUsage( ICrySizer *pSizer ) const
{
	pSizer->AddObject(this, sizeof(*this));
	pSizer->AddObject(m_pObjectFactory);
	pSizer->AddObject(m_objectsNameMap);
	pSizer->AddObject(m_objects);	
	pSizer->AddObject(m_currentLoadedObjects);	
}