#include "StdAfx.h"
#include "LocaleChanger.h"
#include <locale.h>

LocaleChanger::LocaleChanger(int category, const char* newLocale)
{
	m_category = category;
	m_oldLocale = setlocale(category, newLocale);
}

LocaleChanger::~LocaleChanger()
{
	setlocale(m_category, m_oldLocale.c_str());
}
