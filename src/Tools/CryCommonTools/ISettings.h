#ifndef __ISETTINGS_H__
#define __ISETTINGS_H__

class ISettings
{
public:
	virtual bool GetSettingString(TCHAR* buffer, int bufferSize, const TCHAR* key) = 0;
	virtual bool GetSettingInt(int& value, const TCHAR* key) = 0;
};

inline bool GetSettingByRef(ISettings* settings, const tstring& key, tstring& value)
{
	TCHAR buffer[1024];
	bool success = false;
	if (settings)
		success = settings->GetSettingString(buffer, sizeof(buffer), key.c_str());
	if (success)
		value = buffer;
	return success;
}

inline bool GetSettingByRef(ISettings* settings, const tstring& key, int& value)
{
	return settings->GetSettingInt(value, key.c_str());
}

template <typename T> inline T GetSetting(ISettings* settings, const tstring& key, const T& dflt)
{
	T value;
	if (!GetSettingByRef(settings, key, value))
		value = dflt;
	return value;
}

#endif //__ISETTINGS_H__
