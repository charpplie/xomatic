#pragma once

#include <QString>
#include <CryString.h>

namespace QtUtil
{
	// From QString to CryString
	inline CryStringT<char> ToString(const QString &str)
	{
		return str.toLocal8Bit().data();
	}

	// From CryString to QString
	inline QString ToQString(const CryStringT<char> &str)
	{
		return QString::fromLocal8Bit(str.c_str(), str.length());
	}

	// From const char * to QString
	inline QString ToQString(const char *str, size_t len = -1)
	{
		return QString::fromLocal8Bit(str, len);
	}
}
