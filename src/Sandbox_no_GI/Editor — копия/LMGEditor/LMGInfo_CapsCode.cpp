#include "stdafx.h"
#include "LMGEditor/LMGInfo_CapsCode.h"

CLMGInfo_CapsCode::CLMGInfo_CapsCode()
{

}

CLMGInfo_CapsCode::~CLMGInfo_CapsCode()
{

}

const CString& CLMGInfo_CapsCode::GetId() const
{
	return m_id;
}

const CString& CLMGInfo_CapsCode::GetName() const
{
	return m_name;
}

const CString& CLMGInfo_CapsCode::GetCode() const
{
	return m_code;
}

bool CLMGInfo_CapsCode::ParseFromXmlNode( XmlNodeRef node )
{
	const char* nodeName = node->getTag();
	if ( strcmpi( nodeName, "caps_code" ) != 0 )
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

	bool hasCode = node->getAttr( "code", m_code );
	if ( ! hasCode )
	{
		return false;
	}

	return true;
}