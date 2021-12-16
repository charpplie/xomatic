#ifndef __MENUDATA_H__
#define __MENUDATA_H__


//////////////////////////////////////////////////////////////////////////
enum EFlashFrontEndScreens
{
	eFFES_unknown,
	eFFES_main,
	eFFES_profile_login,
	eFFES_play_online,
	eFFES_play_lan,
	eFFES_lobby,
	eFFES_reputation,
	eFFES_leaderboards,
};

enum EFlashMenuScreenFlags
{
	eFMSF_None = 0,
	eFMSF_RememberHighlight		= BIT(1),			/* Store the previously highlighted menu option and when going back to that page highlight it. */
	eFMSF_KeepInStack					= BIT(2),			/* Keeps the screen in the menu stack to go back to. */
};

#define DEFAULT_MENUSCREEN_FLAGS (eFMSF_RememberHighlight|eFMSF_KeepInStack)

struct IFlashPlayer;

//////////////////////////////////////////////////////////////////////////

struct IMenuObject
{
	virtual void Initialize(const IItemParamsNode* paramNode) = 0;
	virtual void Announce(IFlashPlayer* pPlayer) const = 0;
};

//////////////////////////////////////////////////////////////////////////

class CMenuButton: public IMenuObject
{
public:

	CMenuButton();
	virtual ~CMenuButton();

	virtual void Initialize(const IItemParamsNode* paramNode);
	virtual void Announce(IFlashPlayer* pPlayer) const;

private:

	struct SAction
	{
		CryFixedStringT<32> m_action;
		CryFixedStringT<32> m_param;
	};

	typedef std::vector<CMenuButton::SAction> TActions;

	TActions            m_actions;

	CryFixedStringT<64> m_caption;
	CryFixedStringT<64> m_tooltip;

	bool								m_disabled;
};

//////////////////////////////////////////////////////////////////////////

class CMenuSlider: public IMenuObject
{
public:

	CMenuSlider();
	virtual ~CMenuSlider();

	virtual void Initialize(const IItemParamsNode* paramNode);
	virtual void Announce(IFlashPlayer* pPlayer) const;

private:

	CryFixedStringT<64> m_caption;
	CryFixedStringT<64> m_tooltip;
	CryFixedStringT<32> m_action;
	float								m_min;
	float								m_max;
	float								m_step;
	bool								m_disabled;
};

//////////////////////////////////////////////////////////////////////////

class CMenuSwitch: public IMenuObject
{
public:

	CMenuSwitch();
	virtual ~CMenuSwitch();

	virtual void Initialize(const IItemParamsNode* paramNode);
	virtual void Announce(IFlashPlayer* pPlayer) const;

private:

	class COption
	{
	public:
		CryFixedStringT<64> m_caption;
		CryFixedStringT<64> m_value;
	};

	typedef std::vector<CMenuSwitch::COption> TOptions;

	CryFixedStringT<64> m_caption;
	CryFixedStringT<64> m_tooltip;
	CryFixedStringT<32> m_action;
	TOptions						m_options;
	bool								m_disabled;
};

//////////////////////////////////////////////////////////////////////////

class CMenuSubScreen: public IMenuObject
{
public:

	CMenuSubScreen();
	virtual ~CMenuSubScreen();

	virtual void Initialize(const IItemParamsNode* paramNode);
	virtual void Announce(IFlashPlayer* pPlayer) const;

private:

	CryFixedStringT<128> m_path;
};

//////////////////////////////////////////////////////////////////////////

class CMenuScreen
{
public:

	CMenuScreen();
	virtual ~CMenuScreen();

	void Initialize(const IItemParamsNode* paramNode);
	void Announce(IFlashPlayer* pPlayer) const;

	const char* GetName() const { return m_name.c_str(); }
	EFlashFrontEndScreens GetScreenType() const { return m_screen; }

	const int GetCurrentHighlight() const { return m_currentHighlight; }
	void SetCurrentHighlight(int highlight) { if (m_flags&eFMSF_RememberHighlight) { m_currentHighlight = highlight; } }
	bool ShouldKeepInStack() const { return ((m_flags&eFMSF_KeepInStack)!=0); }

private:

	void ReadFlag(const IItemParamsNode* paramNode, const char* name, EFlashMenuScreenFlags flag);

	typedef std::vector<IMenuObject*> TObjects;
	TObjects m_objects;

	EFlashFrontEndScreens m_screen;
	CryFixedStringT<32> m_name;
	int m_currentHighlight;
	uint8 m_flags;
};

//////////////////////////////////////////////////////////////////////////

class CMenuData
{
public:

	CMenuData() {}
	virtual ~CMenuData() {}

	void AddScreen(CMenuScreen* pScreen)
	{
		m_screens.push_back(pScreen);
	}

	CMenuScreen* GetScreen(const int num) const
	{
		return m_screens[num];
	}

	const int GetScreen(const char* name) const
	{
		int length = m_screens.size();
		for(int i=0; i<length; ++i)
		{
			if(!stricmp(m_screens[i]->GetName(), name))
			{
				return i;
			}
		}
		return -1;
	}

	void ReserveScreens(int num)
	{
		m_screens.reserve(num);
	}

	void Clear()
	{
		m_screens.clear();
	}

	const char* GetPath() const
	{
		return m_path.c_str();
	}

	void SetPath(const char* path)
	{
		m_path = path;
	}

private:

	typedef std::vector<CMenuScreen*> TScreens;
	TScreens m_screens;
	CryFixedStringT<64> m_path;
};


//////////////////////////////////////////////////////////////////////////

#endif
