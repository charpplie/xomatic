#include "StdAfx.h"
#include "VehicleXMLHelper.h"

#include <algorithm>


namespace VehicleXml
{
	bool IsNodeTypeEqual( const XmlNodeRef& node, const char* nodeType );

	XmlNodeRef GetXmlNodeDefinitionByIdRec( const XmlNodeRef& node, const char* id );
	bool GetVariableHierarchyRec( IVariable* pCurrentVar, IVariable* pSearchedVar, std::vector< IVariable* >& varHierarchyOut );

	XmlNodeRef FindChildDefinitionByName( const XmlNodeRef& node, const char* name );

	void CopyUseNodeImportantProperties( const XmlNodeRef& useNode, XmlNodeRef useNodeDefintionOut );

	XmlNodeRef GetRootXmlNode( const XmlNodeRef& node );

	bool PropertyValueEquals( const char* a, const char* b );


	IVariable* CreateSimpleVar( const XmlNodeRef& definitionRoot, const XmlNodeRef& definition );
	IVariable* CreateSimpleArrayVar( const XmlNodeRef& definitionRoot, const XmlNodeRef& definition );
	IVariable* CreateTableVar( const XmlNodeRef& definitionRoot, const XmlNodeRef& definition );
}

//////////////////////////////////////////////////////////////////////////
bool VehicleXml::PropertyValueEquals( const char* a, const char* b )
{
	if ( a == NULL || b == NULL )
	{
		return false;
	}

	return ( strcmpi( a, b ) == 0 );
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef VehicleXml::GetRootXmlNode( const XmlNodeRef& node )
{
	assert( node );
	XmlNodeRef currentNode = node;
	do
	{
		if ( ! currentNode->getParent() )
		{
			return currentNode ;
		}

		currentNode = currentNode->getParent();

	} while( true );
}


//////////////////////////////////////////////////////////////////////////
bool VehicleXml::IsNodeTypeEqual( const XmlNodeRef& node, const char* nodeType )
{
	assert( node );
	if ( ! node )
	{
		return false;
	}

	assert( nodeType != NULL );
	if ( nodeType == NULL )
	{
		return false;
	}

	bool isNodeTypeEqual = ( stricmp( node->getTag(), nodeType ) == 0 );
	return isNodeTypeEqual;
}


//////////////////////////////////////////////////////////////////////////
bool VehicleXml::IsUseNode( const XmlNodeRef& node )
{
	return IsNodeTypeEqual( node, "Use" );
}


//////////////////////////////////////////////////////////////////////////
bool VehicleXml::IsArrayNode( const XmlNodeRef& node )
{
	return IsNodeTypeEqual( node, "Array" );
}


//////////////////////////////////////////////////////////////////////////
bool VehicleXml::IsArrayElementNode( const XmlNodeRef& node, const char* name )
{
	assert( name != NULL );
	if ( name == NULL )
	{
		return false;
	}

	return ( IsArrayNode( node ) && HasElementNameEqualTo( node, name ) );
}


//////////////////////////////////////////////////////////////////////////
bool VehicleXml::IsArrayParentNode( const XmlNodeRef& node, const char* name )
{
	assert( name != NULL );
	if ( name == NULL )
	{
		return false;
	}

	return ( IsArrayNode( node ) && HasNodeNameEqualTo( node, name ) );
}


//////////////////////////////////////////////////////////////////////////
bool VehicleXml::IsPropertyNode( const XmlNodeRef& node )
{
	return IsNodeTypeEqual( node, "Property" );
}


//////////////////////////////////////////////////////////////////////////
bool VehicleXml::IsOptionalNode( const XmlNodeRef& node )
{
	assert( node );
	if ( ! node )
	{
		return NULL;
	}

	bool isOptional = false;
	if ( node->getAttr( "optional", isOptional ) )
	{
		return isOptional;
	}

	return false;
}


//////////////////////////////////////////////////////////////////////////
bool VehicleXml::IsDeprecatedNode( const XmlNodeRef& node )
{
	assert( node );
	if ( ! node )
	{
		return NULL;
	}

	bool isDeprecated = false;
	if ( node->getAttr( "deprecated", isDeprecated ) )
	{
		return isDeprecated;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
const char* VehicleXml::GetNodeElementName( const XmlNodeRef& node )
{
	assert( node );
	if ( ! node )
	{
		return NULL;
	}

	return node->getAttr( "elementName" );
}


//////////////////////////////////////////////////////////////////////////
const char* VehicleXml::GetNodeId( const XmlNodeRef& node )
{
	assert( node );
	if ( ! node )
	{
		return NULL;
	}

	return node->getAttr( "id" );
}


//////////////////////////////////////////////////////////////////////////
const char* VehicleXml::GetNodeName( const XmlNodeRef& node )
{
	assert( node );
	if ( ! node )
	{
		return NULL;
	}

	return node->getAttr( "name" );
}


//////////////////////////////////////////////////////////////////////////
bool VehicleXml::HasNodeIdEqualTo( const XmlNodeRef& node, const char* searchedNodeId )
{
	return PropertyValueEquals( GetNodeId( node ), searchedNodeId );
}


//////////////////////////////////////////////////////////////////////////
bool VehicleXml::HasElementNameEqualTo( const XmlNodeRef& node, const char* searchedName )
{
	return PropertyValueEquals( GetNodeElementName( node ), searchedName );
}


//////////////////////////////////////////////////////////////////////////
bool VehicleXml::HasNodeNameEqualTo( const XmlNodeRef& node, const char* searchedName )
{
	return PropertyValueEquals( GetNodeName( node ), searchedName );
}


//////////////////////////////////////////////////////////////////////////
void VehicleXml::CopyUseNodeImportantProperties( const XmlNodeRef& useNode, XmlNodeRef useNodeDefintionOut )
{
	assert( useNode );
	assert( useNodeDefintionOut );

	bool isUseNodeOptional;
	if ( useNode->getAttr( "optional", isUseNodeOptional ) )
	{
		useNodeDefintionOut->setAttr( "optional", isUseNodeOptional );
	}
}


//////////////////////////////////////////////////////////////////////////
XmlNodeRef VehicleXml::GetXmlNodeDefinitionByIdRec( const XmlNodeRef& node, const char* id )
{
	assert( id != NULL );
	assert( node );

	if ( IsUseNode( node ) )
	{
		// 'Use' nodes are just references to another node, so they don't contain the definition we're interested in.
		return XmlNodeRef();
	}

	if ( HasNodeIdEqualTo( node, id ) )
	{
		return node;
	}

	for ( int i = 0; i < node->getChildCount(); ++i )
	{
		XmlNodeRef childNode = node->getChild( i );
		XmlNodeRef nodeFound = GetXmlNodeDefinitionByIdRec( childNode, id );
		if ( nodeFound )
		{
			return nodeFound;
		}
	}

	return XmlNodeRef();
}


XmlNodeRef VehicleXml::GetXmlNodeDefinitionById( const XmlNodeRef& definitionRoot, const char* id )
{
	if ( ! definitionRoot )
	{
		return XmlNodeRef();
	}

	if ( id == NULL )
	{
		return XmlNodeRef();
	}

	return GetXmlNodeDefinitionByIdRec( definitionRoot, id );	
}


//////////////////////////////////////////////////////////////////////////
bool VehicleXml::GetVariableHierarchyRec( IVariable* pCurrentVar, IVariable* pSearchedVar, std::vector< IVariable* >& varHierarchyOut )
{
	assert( pCurrentVar != NULL );
	assert( pSearchedVar != NULL );

	if ( pSearchedVar == pCurrentVar )
	{
		varHierarchyOut.push_back( pCurrentVar );
		return true;
	}

	for ( int i = 0; i < pCurrentVar->NumChildVars(); ++i )
	{
		IVariable* pChildVar = pCurrentVar->GetChildVar( i );
		bool found = GetVariableHierarchyRec( pChildVar, pSearchedVar, varHierarchyOut );
		if ( found )
		{
			varHierarchyOut.push_back( pCurrentVar );
			return true;
		}
	}

	return false;
}


//////////////////////////////////////////////////////////////////////////
bool VehicleXml::GetVariableHierarchy( IVariable* pRootVar, IVariable* pVar, std::vector< IVariable* >& varHierarchyOut )
{
	if ( pRootVar == NULL )
	{
		return false;
	}

	if ( pVar == NULL )
	{
		return false;
	}

	bool found = GetVariableHierarchyRec( pRootVar, pVar, varHierarchyOut );
	std::reverse( varHierarchyOut.begin(), varHierarchyOut.end() );

	return found;
}

 
//////////////////////////////////////////////////////////////////////////
XmlNodeRef VehicleXml::FindChildDefinitionByName( const XmlNodeRef& node, const char* name )
{
	if ( IsArrayNode( node ) )
	{
		if ( HasElementNameEqualTo( node, name ) )
		{
			// We're searching for an element of the array.
			// Return the array definition as the child definition is defined there.
			return node;
		}
	}

	for ( int i = 0; i < node->getChildCount(); ++i )
	{
		XmlNodeRef childNode = node->getChild( i );
		if ( HasNodeNameEqualTo( childNode, name ) )
		{
			return childNode;
		}

		if ( IsUseNode( childNode ) )
		{
			const char* id = GetNodeId( childNode );
			XmlNodeRef rootNode = GetRootXmlNode( node );
			XmlNodeRef useNodeDefintion = GetXmlNodeDefinitionById( rootNode, id );
			if ( useNodeDefintion )
			{
				if ( HasNodeNameEqualTo( useNodeDefintion, name ) )
				{
					XmlNodeRef useNodeDefintionCopy = useNodeDefintion->clone();
					CopyUseNodeImportantProperties( childNode, useNodeDefintionCopy );
					return useNodeDefintionCopy;
				}
			}
		}
	}

	return XmlNodeRef();
}


//////////////////////////////////////////////////////////////////////////
void VehicleXml::GetXmlNodeChildDefinitions( const XmlNodeRef& definitionRoot, const XmlNodeRef& definition, std::vector< XmlNodeRef >& childDefinitionsOut )
{
	if ( ! definitionRoot )
	{
		return;
	}

	if ( ! definition )
	{
		return;
	}

	for ( int i = 0; i < definition->getChildCount(); ++i )
	{
		XmlNodeRef childNode = definition->getChild( i );

		if ( IsUseNode( childNode ) )
		{
			// Use nodes will only reference a definition by id, so we must obtain the real definition.
			const char* childNodeId = GetNodeId( childNode );
			childNode = GetXmlNodeDefinitionById( definitionRoot, childNodeId );
		}

		if ( childNode )
		{
			childDefinitionsOut.push_back( childNode );
		}
	}
}


//////////////////////////////////////////////////////////////////////////
XmlNodeRef VehicleXml::GetXmlNodeDefinitionByVariable( const XmlNodeRef& definitionRoot, IVariable* pRootVar, IVariable* pVar )
{
	if ( ! definitionRoot )
	{
		return XmlNodeRef();
	}

	if ( pRootVar == NULL )
	{
		return XmlNodeRef();
	}

	if ( pVar == NULL )
	{
		return XmlNodeRef();
	}

	
	std::vector< IVariable* > varHirerarchy;
	bool varFoundInHirerarchy = GetVariableHierarchy( pRootVar, pVar, varHirerarchy );
	if ( ! varFoundInHirerarchy )
	{
		return XmlNodeRef();
	}

	XmlNodeRef currentNode = definitionRoot;
	for ( int i = 1; i < varHirerarchy.size(); ++i )
	{
		IVariable* pCurrentVar = varHirerarchy[ i ];
		const char* nameToSearch = pCurrentVar->GetName();
		
		currentNode = FindChildDefinitionByName( currentNode, nameToSearch );
		if ( ! currentNode )
		{
			return XmlNodeRef();
		}
	}

	return currentNode;
}


//////////////////////////////////////////////////////////////////////////
void VehicleXml::GetXmlNodeChildDefinitionsByVariable( const XmlNodeRef& definitionRoot, IVariable* pRootVar, IVariable* pVar, std::vector< XmlNodeRef >& childDefinitionsOut )
{
	assert( definitionRoot );
	if ( ! definitionRoot )
	{
		return;
	}

	assert( pRootVar != NULL );
	if ( pRootVar == NULL )
	{
		return;
	}

	assert( pVar != NULL );
	if ( pVar == NULL )
	{
		return;
	}

	XmlNodeRef varDefinitionNode = GetXmlNodeDefinitionByVariable( definitionRoot, pRootVar, pVar );
	if ( ! varDefinitionNode )
	{
		return;
	}

	if ( IsArrayNode( varDefinitionNode ) )
	{
		// Array parent properties cannot have child property definitions but they store the definitions of the
		// elements they can have so we have to check if we're looking at the elementNode or the parent...
		bool isArrayParent = HasNodeNameEqualTo( varDefinitionNode, pVar->GetName() );
		if ( isArrayParent )
		{
			return;
		}
	}

	if ( IsPropertyNode( varDefinitionNode ) )
	{
		// Property nodes shouldn't have child property definitions.
		return;
	}

	for ( int i = 0; i < varDefinitionNode->getChildCount(); ++i )
	{
		XmlNodeRef childNode = varDefinitionNode->getChild( i );

		if ( IsUseNode( childNode ) )
		{
			// Use nodes will only reference a definition by id, so we must obtain the real definition.
			const char* childNodeId = GetNodeId( childNode );
			childNode = GetXmlNodeDefinitionById( definitionRoot, childNodeId );
		}

		if ( childNode )
		{
			childDefinitionsOut.push_back( childNode );
		}
	}
}


//////////////////////////////////////////////////////////////////////////
XmlNodeRef VehicleXml::GetXmlNodeChildDefinitionsByName( const XmlNodeRef& definitionRoot, const char* childName, IVariable* pRootVar, IVariable* pVar )
{
	assert( childName != NULL );
	if ( childName == NULL )
	{
		return XmlNodeRef();
	}

	std::vector< XmlNodeRef > childDefinitions;
	GetXmlNodeChildDefinitionsByVariable( definitionRoot, pRootVar, pVar, childDefinitions );

	for ( int i = 0; i < childDefinitions.size(); ++i )
	{
		XmlNodeRef childNode = childDefinitions[ i ];
		if ( HasNodeNameEqualTo( childNode, childName ) )
		{
			return childNode;
		}
	}

	return XmlNodeRef();
}


//////////////////////////////////////////////////////////////////////////
IVariable* VehicleXml::CreateVar( const char* value, const XmlNodeRef& definition )
{
	XmlNodeRef enumNode = definition->findChild( "Enum" );

	if ( ! enumNode && ! definition->haveAttr( "list" ) )
	{
		return new CVariable< CString >();
	}

	CVariableEnum< CString >* pVarEnum = new CVariableEnum< CString >();

	if ( enumNode )
	{      
		for ( int i = 0; i < enumNode->getChildCount(); ++i )
		{
			const char* itemName = enumNode->getChild( i )->getContent();
			pVarEnum->AddEnumItem( itemName, itemName );
		}      
	}
	else 
	{
		pVarEnum->AddEnumItem( "", "" );
		pVarEnum->AddEnumItem( value, value );

		const char* listType = definition->getAttr( "list" );
		if ( strcmpi( listType, "Helper" ) == 0 )
		{
			pVarEnum->SetDataType(IVariable::DT_VEEDHELPER);
		}
		else if ( strcmpi( listType, "Part" ) == 0 )
		{
			pVarEnum->SetDataType(IVariable::DT_VEEDPART);
		}
		else if ( strcmpi( listType, "Component" ) == 0 )
		{
			pVarEnum->SetDataType(IVariable::DT_VEEDCOMP);
		}
	}

	return pVarEnum;
}


//////////////////////////////////////////////////////////////////////////
IVariable* VehicleXml::CreateSimpleVar( const XmlNodeRef& definitionRoot, const XmlNodeRef& definition )
{
	const char* type = "";
	if ( type = definition->getAttr( "type" ) )
	{
		if ( strcmpi( type, "float" ) == 0 )
		{
			return CreateVar< float >( 0.f, definition );
		}
		else if ( strcmpi( type, "bool" ) == 0 )
		{
			return CreateVar< bool >( true, definition );
		}
		else if ( strcmpi( type, "Vec3" ) == 0 )
		{
			return CreateVar< Vec3 >( Vec3( 0, 0, 0 ), definition );
		}
		else if ( strcmpi( type, "int" ) == 0 )
		{
			return CreateVar< int >( 0, definition );
		}
		else
		{
			return CreateVar( "", definition );
		}
	}
	else
	{
		return CreateVar( "", definition );
	}
}


//////////////////////////////////////////////////////////////////////////
IVariable* VehicleXml::CreateSimpleArrayVar( const XmlNodeRef& definitionRoot, const XmlNodeRef& definition )
{
	return new CVariableArray();
}


//////////////////////////////////////////////////////////////////////////
IVariable* VehicleXml::CreateTableVar( const XmlNodeRef& definitionRoot, const XmlNodeRef& definition )
{
	IVariable* pVar = new CVariableArray();

	std::vector< XmlNodeRef > childDefinitions;
	GetXmlNodeChildDefinitions( definitionRoot, definition, childDefinitions );
	for ( int i = 0; i < childDefinitions.size(); ++i )
	{
		XmlNodeRef childDefinition = childDefinitions[ i ];
		
		bool create = false;
		create |= ! IsOptionalNode( childDefinition );

		if ( create )
		{
			IVariablePtr pChildVar = CreateDefaultVar( definitionRoot, childDefinition, GetNodeName( childDefinition ) );
			pVar->AddChildVar( pChildVar );
		}
	}
	return pVar;
}


//////////////////////////////////////////////////////////////////////////
IVariable* VehicleXml::CreateDefaultVar( const XmlNodeRef& definitionRoot, const XmlNodeRef& definition, const char* varName )
{
	assert( definitionRoot );
	if ( ! definitionRoot )
	{
		return NULL;
	}

	assert( definition );
	if ( ! definition )
	{
		return NULL;
	}

	assert( varName != NULL );
	if ( varName == NULL )
	{
		return NULL;
	}

	IVariable* pVar = NULL;
	if ( IsPropertyNode( definition ) )
	{
		pVar = CreateSimpleVar( definitionRoot, definition );
	}
	else if ( IsArrayNode( definition ) )
	{
		if ( IsArrayParentNode( definition, varName ) )
		{
			pVar = CreateSimpleArrayVar( definitionRoot, definition );
		}
		else if ( IsArrayElementNode( definition, varName ) )
		{
			bool isSimpleArrayType = ( definition->getChildCount() == 0 );
			if ( isSimpleArrayType )
			{
				pVar = CreateSimpleVar( definitionRoot, definition );
			}
			else
			{
				pVar = CreateTableVar( definitionRoot, definition );
			}
		}
	}
	else
	{
		pVar = CreateTableVar( definitionRoot, definition );
	}

	if ( pVar )
	{
		pVar->SetName( varName );
		SetExtendedVarProperties( pVar, definition );
		return pVar;
	}
	
	return NULL;
}

//////////////////////////////////////////////////////////////////////////
void VehicleXml::SetExtendedVarProperties( IVariable* pVar, const XmlNodeRef& node )
{
	assert( pVar != NULL );
	if ( pVar == NULL )
	{
		return;
	}

	assert( node );
	if ( ! node )
	{
		return;
	}

	pVar->SetUserData( NULL );

	{
		float min;
		float max;
		pVar->GetLimits( min, max );

		node->getAttr( "min", min );
		node->getAttr( "max", max );    
		pVar->SetLimits( min, max );
	}

	{
		CString description;
		if ( node->getAttr( "desc", description ) )
		{
			pVar->SetDescription( description );    
		}
	}

	{
		int deprecated = 0;
		node->getAttr( "deprecated", deprecated );
		if ( deprecated == 1 )
		{
			pVar->SetFlags( IVariable::UI_DISABLED );
		}
	}

	const char* varName = pVar->GetName();
	if ( IsArrayParentNode( node, varName ) )
	{
		int extendable = 0;
		node->getAttr( "extendable", extendable );

		if ( extendable == 1 )
		{
			pVar->SetDataType( IVariable::DT_EXTARRAY );  

			XmlNodeRef rootNode = GetRootXmlNode( node );
			const char* elementName = GetNodeElementName( node );
			IVariable* pChild = VehicleXml::CreateDefaultVar( rootNode, node, elementName );
			pChild->AddRef(); // For this reason we must clean up later...
			pVar->SetUserData( pChild );
		}
	}
	else if ( strcmpi( varName, "filename" ) == 0 )
	{
		pVar->SetDataType( IVariable::DT_FILE );
	}
	else if ( strcmpi( varName, "sound" ) == 0 )
	{
		pVar->SetDataType( IVariable::DT_SOUND );
	}
}

void VehicleXml::CleanUp( IVariable* pVar )
{
	if ( pVar == NULL )
	{
		return;
	}

	for ( int i = 0; i < pVar->NumChildVars(); ++i )
	{
		CleanUp( pVar->GetChildVar( i ) );
	}

	if ( pVar->GetDataType() == IVariable::DT_EXTARRAY )
	{
		IVariable* pChild = static_cast< IVariable* >( pVar->GetUserData() );
		pChild->Release();
		pVar->SetUserData( NULL );
	}
}