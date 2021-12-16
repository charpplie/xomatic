#ifndef __OPTION_H__
#define __OPTION_H__

struct IPlayerProfile;

//////////////////////////////////////////////////////////////////////////


class COption
{

public:

	COption();
	COption(const char* name, const char* value);

	virtual ~COption();

	void						SetPlayerProfile(IPlayerProfile* profile) { m_currentProfile = profile; }
	IPlayerProfile* GetPlayerProfile() {return m_currentProfile; }

	const string		GetName() { return currentName; }

	virtual void		InitializeFromProfile();
	virtual void		InitializeFromCVar();
	virtual void		ResetDefault() {}

	virtual void		Set(const char* param);
	virtual string	Get();

protected:

		string				currentName;
		string				currentValue;

private:

	IPlayerProfile* m_currentProfile;

};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


class CCVarOption : public COption
{

public:

	CCVarOption();
	CCVarOption(const char* name, const char* cvarName, const char* defaultValue);

	virtual ~CCVarOption();

	virtual void		InitializeFromProfile();
	virtual void		InitializeFromCVar();
	virtual void		ResetDefault();

	virtual void		Set(const char* param);
	virtual string	Get();

private:

	string		cVar;
	string		defaultValue;

};

//////////////////////////////////////////////////////////////////////////

#endif

