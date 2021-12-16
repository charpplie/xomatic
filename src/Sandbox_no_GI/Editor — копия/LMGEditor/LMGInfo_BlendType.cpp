#include "stdafx.h"
#include "LMGEditor/LMGInfo_BlendType.h"

CLMGInfo_BlendType::CLMGInfo_BlendType()
{

}

CLMGInfo_BlendType::~CLMGInfo_BlendType()
{

}


const CString& CLMGInfo_BlendType::GetCode() const
{
	return m_code;
}

const CString& CLMGInfo_BlendType::GetName() const
{
	return m_name;
}

const CString& CLMGInfo_BlendType::GetDescription() const
{
	return m_description;
}


size_t CLMGInfo_BlendType::GetCapabilityIdCount() const
{
	return m_capabilityIds.size();
}

const CString& CLMGInfo_BlendType::GetCapabilityId( size_t i ) const
{
	assert( i < m_capabilityIds.size() );
	return m_capabilityIds[ i ];
}


size_t CLMGInfo_BlendType::GetSlotIdCount() const
{
	return m_slotIds.size();
}

const CString& CLMGInfo_BlendType::GetSlotId( size_t i ) const
{
	assert( i < m_slotIds.size() );
	return m_slotIds[ i ];
}

size_t CLMGInfo_BlendType::GetTestIdCount() const
{
	return m_testIds.size();
}

const CString& CLMGInfo_BlendType::GetTestId( size_t i ) const
{
	assert( i < m_testIds.size() );
	return m_testIds[ i ];
}

size_t CLMGInfo_BlendType::GetCapsCodeCount() const
{
	return m_capsCodeIds.size();
}

const CString& CLMGInfo_BlendType::GetCapsCodeId( size_t i ) const
{
	assert( i < m_capsCodeIds.size() );
	return m_capsCodeIds[ i ];
}


bool CLMGInfo_BlendType::ParseFromXmlNode( XmlNodeRef node )
{
	const char* nodeName = node->getTag();
	if ( strcmpi( nodeName, "blend_type" ) != 0 )
	{
		return false;
	}

	bool hasCode = node->getAttr( "code", m_code );
	if ( ! hasCode )
	{
		return false;
	}

	bool hasName = node->getAttr( "name", m_name );
	if ( ! hasName )
	{
		m_name = m_code;
	}

	node->getAttr( "description", m_description );

	for ( int i = 0; i < node->getChildCount(); i++ )
	{
		XmlNodeRef childNode = node->getChild( i );

		const char* childName = childNode->getTag();
		if ( strcmpi( childName, "slot_ids" ) == 0 )
		{
			ParseSlotIdsFromXmlNode( childNode );
		}
		else if ( strcmpi( childName, "capability_ids" ) == 0 )
		{
			ParseCapabilityIdsFromXmlNode( childNode );
		}
		else if ( strcmpi( childName, "test_ids" ) == 0 )
		{
			ParseTestIdsFromXmlNode( childNode );
		}
		else if ( strcmpi( childName, "caps_code_ids" ) == 0 )
		{
			ParseCapsCodeIdsFromXmlNode( childNode );
		}
	}

	return true;
}




bool CLMGInfo_BlendType::ParseSlotIdsFromXmlNode( XmlNodeRef node )
{
	for ( int i = 0; i < node->getChildCount(); i++ )
	{
		XmlNodeRef slotIdNode = node->getChild( i );

		const char* slotId = slotIdNode->getContent();
		if ( slotId != NULL )
		{
			m_slotIds.push_back( slotId );
		}
	}

	return true;
}

bool CLMGInfo_BlendType::ParseCapabilityIdsFromXmlNode( XmlNodeRef node )
{
	for ( int i = 0; i < node->getChildCount(); i++ )
	{
		XmlNodeRef capabilityIdNode = node->getChild( i );

		const char* capabilityId = capabilityIdNode->getContent();
		if ( capabilityId != NULL )
		{
			m_capabilityIds.push_back( capabilityId );
		}
	}

	return true;
}

bool CLMGInfo_BlendType::ParseTestIdsFromXmlNode( XmlNodeRef node )
{
	for ( int i = 0; i < node->getChildCount(); i++ )
	{
		XmlNodeRef testIdNode = node->getChild( i );

		const char* testId = testIdNode->getContent();
		if ( testId != NULL )
		{
			m_testIds.push_back( testId );
		}
	}

	return true;
}

bool CLMGInfo_BlendType::ParseCapsCodeIdsFromXmlNode( XmlNodeRef node )
{
	for ( int i = 0; i < node->getChildCount(); i++ )
	{
		XmlNodeRef capsCodeIdNode = node->getChild( i );

		const char* capsCodeId = capsCodeIdNode->getContent();
		if ( capsCodeId != NULL )
		{
			m_capsCodeIds.push_back( capsCodeId );
		}
	}

	return true;
}