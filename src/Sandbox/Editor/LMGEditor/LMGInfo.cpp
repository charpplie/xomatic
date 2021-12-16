#include "stdafx.h"
#include "..\LMGEditor/LMGInfo.h"

CLMGInfo::CLMGInfo()
{
	XmlParser parser;
	XmlNodeRef root = parser.parse( "Editor/LmgEditor.xml" );
	if ( ! root )
	{
		return;
	}

	for ( int i = 0; i < root->getChildCount(); i++ )
	{
		XmlNodeRef childNode = root->getChild( i );

		const char* childName = childNode->getTag();
		if ( strcmpi( childName, "blend_types" ) == 0 )
		{
			ParseBlendTypesXml( childNode );
		}
		else if ( strcmpi( childName, "slots" ) == 0 )
		{
			ParseSlotsXml( childNode );
		}
		else if ( strcmpi( childName, "caps_codes" ) == 0 )
		{
			ParseCapsCodesXml( childNode );
		}
		else if ( strcmpi( childName, "tests" ) == 0 )
		{
			ParseTestsXml( childNode );
		}
	}
}

CLMGInfo::~CLMGInfo()
{
	for ( size_t i = 0; i < m_blendTypes.size(); i++ )
	{
		delete m_blendTypes[ i ];
	}

	for ( size_t i = 0; i < m_slots.size(); i++ )
	{
		delete m_slots[ i ];
	}

	for ( size_t i = 0; i < m_tests.size(); i++ )
	{
		delete m_tests[ i ];
	}
}

void CLMGInfo::ParseSlotsXml( XmlNodeRef node )
{
	for ( int i = 0; i < node->getChildCount(); i++ )
	{
		XmlNodeRef slotNode = node->getChild( i );

		CLMGInfo_Slot* pSlot = new CLMGInfo_Slot();
		bool parseSuccess = pSlot->ParseFromXmlNode( slotNode );
		if ( ! parseSuccess )
		{
			CryLog( "LmgEditor: Unable to parse slot %d from xml at line %d", i, slotNode->getLine() );
			delete pSlot;
		}
		else
		{
			InsertSlot( pSlot );
		}
	}
}

void CLMGInfo::ParseBlendTypesXml( XmlNodeRef node )
{
	for ( int i = 0; i < node->getChildCount(); i++ )
	{
		XmlNodeRef blendTypeNode = node->getChild( i );

		CLMGInfo_BlendType* pBlendType = new CLMGInfo_BlendType();
		bool parseSuccess = pBlendType->ParseFromXmlNode( blendTypeNode );
		if ( ! parseSuccess )
		{
			CryLog( "LmgEditor: Unable to parse blend type from xml at line %d", blendTypeNode->getLine() );
			delete pBlendType;
		}
		else
		{
			InsertBlendType( pBlendType );
		}
	}
}

void CLMGInfo::ParseTestsXml( XmlNodeRef node )
{
	for ( int i = 0; i < node->getChildCount(); i++ )
	{
		XmlNodeRef testNode = node->getChild( i );

		CLMGInfo_Test* pTest = new CLMGInfo_Test();
		bool parseSuccess = pTest->ParseFromXmlNode( testNode );
		if ( ! parseSuccess )
		{
			CryLog( "LmgEditor: Unable to parse test %d from xml at line %d", i, testNode->getLine() );
			delete pTest;
		}
		else
		{
			InsertTest( pTest );
		}
	}
}

void CLMGInfo::ParseCapsCodesXml( XmlNodeRef node )
{
	for ( int i = 0; i < node->getChildCount(); i++ )
	{
		XmlNodeRef capsCodeNode = node->getChild( i );

		CLMGInfo_CapsCode* pCapsCode = new CLMGInfo_CapsCode();
		bool parseSuccess = pCapsCode->ParseFromXmlNode( capsCodeNode );
		if ( ! parseSuccess )
		{
			CryLog( "LmgEditor: Unable to parse caps code from xml at line %d", capsCodeNode->getLine() );
			delete pCapsCode;
		}
		else
		{
			InsertCapsCode( pCapsCode );
		}
	}
}

void CLMGInfo::InsertBlendType( CLMGInfo_BlendType* pBlendType )
{
	assert( pBlendType != NULL );

	size_t blendTypeIndex = m_blendTypes.size();

	m_blendTypes.push_back( pBlendType );
	m_blendTypeCodeToIndex[ pBlendType->GetCode() ] = blendTypeIndex;
}

void CLMGInfo::InsertSlot( CLMGInfo_Slot* pSlot )
{
	assert( pSlot != NULL );

	size_t slotIndex = m_slots.size();

	m_slots.push_back( pSlot );
	m_slotIdToIndex[ pSlot->GetId() ] = slotIndex;
}

void CLMGInfo::InsertTest( CLMGInfo_Test* pTest )
{
	assert( pTest != NULL );

	size_t testIndex = m_tests.size();

	m_tests.push_back( pTest );
	m_testIdToIndex[ pTest->GetId() ] = testIndex;
}

void CLMGInfo::InsertCapsCode( CLMGInfo_CapsCode* pCapsCode )
{
	assert( pCapsCode != NULL );

	size_t capsCodeIndex = m_capsCodes.size();

	m_capsCodes.push_back( pCapsCode );
	m_capsCodeIdToIndex[ pCapsCode->GetId() ] = capsCodeIndex;
}

size_t CLMGInfo::GetBlendTypeCount() const
{
	return m_blendTypes.size();
}

CLMGInfo_BlendType* CLMGInfo::GetBlendType( size_t index )
{
	if ( m_blendTypes.size() <= index )
	{
		return NULL;
	}

	return m_blendTypes[ index ];
}

size_t CLMGInfo::GetBlendTypeIndexByCode( const CString& code ) const
{
	const size_t INVALID_INDEX = m_blendTypes.size();
	size_t index = stl::find_in_map( m_blendTypeCodeToIndex, code, INVALID_INDEX );

	return index;
}

CLMGInfo_BlendType* CLMGInfo::GetBlendTypeByCode( const CString& code )
{
	size_t index = GetBlendTypeIndexByCode( code );
	return GetBlendType( index );
}

CLMGInfo_Slot* CLMGInfo::GetSlotById( const CString& id )
{
	const size_t INVALID_INDEX = m_slots.size();
	size_t index = stl::find_in_map( m_slotIdToIndex, id, INVALID_INDEX );

	if ( m_slots.size() <= index )
	{
		return NULL;
	}

	return m_slots[ index ];
}

CLMGInfo_Test* CLMGInfo::GetTestById( const CString& id )
{
	const size_t INVALID_INDEX = m_tests.size();
	size_t index = stl::find_in_map( m_testIdToIndex, id, INVALID_INDEX );

	if ( m_tests.size() <= index )
	{
		return NULL;
	}

	return m_tests[ index ];
}

CLMGInfo_CapsCode* CLMGInfo::GetCapsCodeById( const CString& id )
{
	const size_t INVALID_INDEX = m_capsCodes.size();
	size_t index = stl::find_in_map( m_capsCodeIdToIndex, id, INVALID_INDEX );

	if ( m_capsCodes.size() <= index )
	{
		return NULL;
	}

	return m_capsCodes[ index ];
}

CLMGInfo_CapsCode* CLMGInfo::GetCapsCodeByCode( const CString& code )
{
	for ( size_t i = 0; i < m_capsCodes.size(); i++ )
	{
		CLMGInfo_CapsCode* pCurrentCapsCode = m_capsCodes[ i ];
		if ( pCurrentCapsCode->GetCode().CompareNoCase( code ) == 0 )
		{
			return pCurrentCapsCode;
		}
	}

	return NULL;
}