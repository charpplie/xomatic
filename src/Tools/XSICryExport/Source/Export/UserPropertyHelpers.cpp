#include "StdAfx.h"
#include "UserPropertyHelpers.h"
#include "StringHelpers.h"

UserPropertyHelpers::UserPropertyReader::UserPropertyReader(const XSI::X3DObject& node)
{
	XSI::CustomProperty l_ObjectProperties(node.GetProperties().GetItem(L"ObjectProperties"));
	string text = (l_ObjectProperties.IsValid() ? XSI::CString(l_ObjectProperties.GetParameterValue(L"Props")).GetAsciiString() : "");
	const char* properties = text.c_str();

	m_userPropertyString = properties;

	for (const char* p = properties; *p;)
	{
		const char* lineStart = p;
		size_t lineLength = strcspn(lineStart, "\n");
		size_t equalPosition = strcspn(lineStart, "=");
		equalPosition = (equalPosition < lineLength ? equalPosition : lineLength);
		if (equalPosition < lineLength)
		{
			string key(lineStart, equalPosition);
			string value(lineStart + equalPosition + 1, lineLength - equalPosition - 1);
			m_keyValueMap.insert(std::make_pair(StringHelpers::Trim(key), StringHelpers::Trim(value)));
		}

		p += lineLength + 1;
	}
}

//////////////////////////////////////////////////////////////////////////
string UserPropertyHelpers::UserPropertyReader::GetUserPropertyString() const
{
	return m_userPropertyString;
}

void UserPropertyHelpers::UserPropertyReader::ConvertValue(string& out, const string& in)
{
	out = in;
}

void UserPropertyHelpers::UserPropertyReader::ConvertValue(float& out, const string& in)
{
	out = strtod(in.c_str(), 0);
}
