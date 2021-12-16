#ifndef __PROFILEOPTIONS_H__
#define __PROFILEOPTIONS_H__

#include "IGameFramework.h"
#include "Option.h"

//////////////////////////////////////////////////////////////////////////
struct IPlayerProfile;

class CProfileOptions
{
public:

					CProfileOptions();
	virtual ~CProfileOptions();

	void		UpdatePlayerProfile(IPlayerProfile *profile);

	void		InitializeFromProfile();
	void		InitializeFromCVar();

	void		SaveProfile();
	bool		IsOption(const char* option);

	string	GetOptionValue(const char* command);
	void		SetOptionValue(const char* command, const char* param);
	void		SetOptionValue(const char* command, int value);
private:

	bool IsCVar(const char* cVarName);
	void AddOption(const XmlNodeRef node);
	void AddOption(const char* name, const char* value, const char* cvar = NULL);

	std::vector<COption*> m_allOptions;
};


//////////////////////////////////////////////////////////////////////////

#endif

