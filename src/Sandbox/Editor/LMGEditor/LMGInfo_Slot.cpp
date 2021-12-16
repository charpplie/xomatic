#include "stdafx.h"
#include "..\LMGEditor/LMGInfo_Slot.h"

CLMGInfo_Slot::CLMGInfo_Slot()
{

}

CLMGInfo_Slot::~CLMGInfo_Slot()
{

}


const CString& CLMGInfo_Slot::GetId() const
{
	return m_id;
}

const CString& CLMGInfo_Slot::GetName() const
{
	return m_name;
}

const CString& CLMGInfo_Slot::GetDescription() const
{
	return m_description;
}

const CString& CLMGInfo_Slot::GetPosition() const
{
	return m_position;
}


bool CLMGInfo_Slot::ParseFromXmlNode( XmlNodeRef node )
{
	const char* nodeName = node->getTag();
	if ( strcmpi( nodeName, "slot" ) != 0 )
	{
		return false;
	}

	bool hasId = node->getAttr( "id", m_id );
	if ( ! hasId )
	{
		return false;
	}

	bool hasName = node->getAttr( "name", m_name );
	if ( ! hasName )
	{
		m_name = m_id;
	}

	node->getAttr( "description", m_description );
	node->getAttr( "position", m_position );

	return true;
}
