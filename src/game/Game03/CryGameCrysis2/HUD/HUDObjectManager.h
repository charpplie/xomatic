#ifndef ___HUDOBJECTMANAGER_H___
#define ___HUDOBJECTMANAGER_H___

class CHUDObject;
class CHUDObjectFactory;
struct IItemParamsNode;

class CHUDObjectManager
{
private : // Typedefs

	typedef std::map<CryFixedStringT<32>,CHUDObject*> THUDObjectNameMap;
	typedef std::vector<CHUDObject*> THUDObjectList ;

public : // Interface

	CHUDObjectManager( );
	~CHUDObjectManager();

	void LoadObjectsFromFileList( char* xml_path );

	void UnloadObject( const CHUDObject* );

	void ClearObjects( void );

	// TODO : Return an HUDObjectID instead.
	CHUDObject* FindObjectByName( const char* object_ref_name );

	const char* FindObjectName( const CHUDObject* ) const;

	CHUDObject* GetHUDObjectInstance(const char* className);

	void GetMemoryUsage(ICrySizer *pSizer) const;
private :

	void LoadObject( const char* xml_path, const IItemParamsNode* xml_bit );
	void LoadObjects( const char* xml_path, const IItemParamsNode* hud_objects );
	IItemParamsNode * LoadObjectsFile( const char* xml_path ) const;

	int  GetObjectsInNode( const IItemParamsNode* hud_objects );

private : // Data members
	CHUDObjectFactory* m_pObjectFactory;
	THUDObjectNameMap  m_objectsNameMap;
	THUDObjectList     m_objects;
	THUDObjectList		 m_currentLoadedObjects;

};

#endif //___HUDOBJECTMANAGER_H___
