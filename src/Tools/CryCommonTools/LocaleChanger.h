#ifndef __LOCALECHANGER_H__
#define __LOCALECHANGER_H__

class LocaleChanger
{
public:
	LocaleChanger(int category, const char* newLocale);
	~LocaleChanger();

private:
	int m_category;
	string m_oldLocale;
};

#endif //__LOCALECHANGER_H__
