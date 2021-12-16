#ifndef __USERPROPERTYHELPERS_H__
#define __USERPROPERTYHELPERS_H__

#include <xsi_x3dobject.h>

namespace UserPropertyHelpers
{
	class UserPropertyReader
	{
	public:
		UserPropertyReader(const XSI::X3DObject& object);

		string GetUserPropertyString() const;

		template <typename T> T GetProperty(const string& key) const;
		template <typename T> bool GetPropertyByRef(const string& key, T& value) const;

	private:
		static void ConvertValue(string& out, const string& in);
		static void ConvertValue(float& out, const string& in);
		typedef std::map<string, string, stl::less_stricmp<string> > KeyValueMap;
		KeyValueMap m_keyValueMap;
		string m_userPropertyString;
	};
}

template <typename T> inline T UserPropertyHelpers::UserPropertyReader::GetProperty(const string& key) const
{
	KeyValueMap::const_iterator keyValuePos = m_keyValueMap.find(key);
	T value = T();
	if (keyValuePos != m_keyValueMap.end())
		ConvertValue(value, (*keyValuePos).second);
	return value;
}

template <typename T> inline bool UserPropertyHelpers::UserPropertyReader::GetPropertyByRef(const string& key, T& value) const
{
	KeyValueMap::const_iterator keyValuePos = m_keyValueMap.find(key);
	if (keyValuePos != m_keyValueMap.end())
	{
		ConvertValue(value, (*keyValuePos).second);
		return true;
	}
	return false;
}

#endif //__USERPROPERTYHELPERS_H__
