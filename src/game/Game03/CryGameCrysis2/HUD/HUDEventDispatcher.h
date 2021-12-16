#ifndef __HUDEVENTDISPATCHER_H__
#define __HUDEVENTDISPATCHER_H__


//////////////////////////////////////////////////////////////////////////

typedef CryFixedStringT<32> TEventName;

enum EHUDEventType
{
	eHUDEvent_None = 0,
	// HUD State & resolutions
	eHUDEvent_OnHUDUnload,
	eHUDEvent_OnHUDReload,
	eHUDEvent_OnUpdate,
	eHUDEvent_OnPostHUDDraw,
	eHUDEvent_OnResolutionChange,
	eHUDEvent_HUDElementVisibility,
	// Local Player
	eHUDEvent_OnInitPlayer,
	eHUDEvent_OnHealthChanged,
	eHUDEvent_OnPlayerDeath,
	eHUDEvent_OnViewDistanceChanged,
	eHUDEvent_OnSpawn,
	eHUDEvent_PlayerSwitchTeams,
	eHUDEvent_ShowReviveCycle,
	eHUDEvent_OnShowHitIndicator, // local player taking damage
	eHUDEvent_OnViewUpdate,
	eHUDEvent_OnPhysicalImpulse,
	// Remote player
	eHUDEvent_OnEnterGame_RemotePlayer,
	eHUDEvent_OnLeaveGame_RemotePlayer,
	//eHUDEvent_OnSpawn_NonLocalPlayer,
	//eHUDEvent_OnLeave_NonLocalPlayer,
	// Score and progression
	eHUDEvent_OnNewScore,
	eHUDEvent_OnNewXP,
	eHUDEvent_OnPlayerPromotion,
	eHUDEvent_OnDogTagCollected,
	// SuitMode & Menu
	eHUDEvent_OnSuitModeChanged,
	eHUDEvent_OnEnergyChanged,
	eHUDEvent_OnSuitStateChanged,
	eHUDEvent_OnSuitMenuOpened,
	eHUDEvent_OnSuitMenuClosed,
	eHUDEvent_OnSuitPowerActivated,
	// Weapons, Weapon Feedback & Crosshair
	eHUDEvent_OnStartReload,
	eHUDEvent_OnReloaded,
	eHUDEvent_OnAmmoPickUp,
	eHUDEvent_OnSetAmmoCount,
	eHUDEvent_OnShoot,
	eHUDEvent_OnItemSelected,
	eHUDEvent_OnFireModeChanged,
	eHUDEvent_OnCrosshairSelected,
	eHUDEvent_OnHitTarget,
	eHUDEvent_OnHit,
	eHUDEvent_OnZoom,
	eHUDEvent_OnExplosiveSpawned,
	// Prompts (pickups and interactions)
	eHUDEvent_OnLookAtChanged,
	eHUDEvent_OnInteractionRequest,
	eHUDEvent_OnUsableChanged,
	eHUDEvent_ShowingUsablePrompt,
	eHUDEvent_HidingUsablePrompt,
	eHUDEvent_AddOnScreenMessage,
	// MP Gamemodes
	eHUDEvent_GameEnded,
	eHUDEvent_OnGamemodeChange,
	eHUDEvent_OnInitGameRules,
	eHUDEvent_OnUpdateGameStartMessage,
	eHUDEvent_OnWaitingForPlayers,
	eHUDEvent_OnGameStart,
	eHUDEvent_OnSetGameStateMessage,
	eHUDEvent_OnRoundStart,
	eHUDEvent_OnUpdateGameResumeMessage,
	// CTF Specific
	eHUDEvent_OnStartCarryingFlag,
	eHUDEvent_OnOwnFlagStatusChanged,
	eHUDEvent_OnEnemyFlagStatusChanged,
	// SP Objectives
	eHUDEvent_OnObjectiveChanged, // used by visor, objective display, nave path.
	// MP Objectives
	eHUDEvent_OnNewObjective,
	eHUDEvent_OnRemoveObjective,
	eHUDEvent_OnSiteBeingCaptured, // used by assault & power struggle light.
	eHUDEvent_OnSiteCaptured,
	eHUDEvent_OnCaptureObjectiveNumChanged, // called when the number of tracked entities changes
	eHUDEvent_OnNewCaptureObjectiveWave,
	eHUDEvent_OnOverallCaptureProgressUpdate,
	// Death cam replay
	eHUDEvent_OnDeathcamStartPlay,
	eHUDEvent_OnDeathcamStopPlay,
	// Blind
	eHUDEvent_OnBlind,
	eHUDEvent_OnEndBlind,
	// Radar & Map
	eHUDEvent_AlignToRadarAsset,
	eHUDEvent_RadarRotation,
	eHUDEvent_MapInfo,
	eHUDEvent_ToggleMap,
	eHUDEvent_TemporarilyTrackEntity,
	eHUDEvent_RescanActors,  
	eHUDEvent_AddEntity,     // TODO: Use objective events instead.
	eHUDEvent_RemoveEntity,  // TODO: Use objective events instead.
	// Battle area
	eHUDEvent_LeavingBattleArea,
	eHUDEvent_ReturningToBattleArea,
	// Host migration
	eHUDEvent_ShowHostMigrationScreen,
	eHUDEvent_HideHostMigrationScreen,
	eHUDEvent_HostMigrationOnNewPlayer,
	// Perks
	eHUDEvent_OnScanPerkChanged,
	eHUDEvent_OnAIAwarenessChanged,
	eHUDEvent_OnTeamRadarChanged,
	// Tagnames
	eHUDEvent_OnIgnoreEntity,
	eHUDEvent_OnStopIgnoringEntity,
	// Scanning/Tactical info
	eHUDEvent_OnScanningStart,
	eHUDEvent_OnEntityScanned,
	eHUDEvent_OnEntitySpotted,
	eHUDEvent_OnScanningStop,
	eHUDEvent_OnCenterEntityChanged,
	eHUDEvent_OnVisorChanged,
	eHUDEvent_OnLeanChanged,
	eHUDEvent_OnTacticalInfoChanged,
	eHUDEvent_OnNewBattleLogMessage,
	// Menus & Scoreboards
	eHUDEvent_OpenedIngameMenu,
	eHUDEvent_ClosedIngameMenu,
	eHUDEvent_ShowScoreboard,
	eHUDEvent_HideScoreboard,
	eHUDEvent_ToggleSelectMenu,
	eHUDEvent_ShowSelectMenu,
	eHUDEvent_HideSelectMenu,
	eHUDEvent_ShowSelectMenuMap,
	eHUDEvent_HideSelectMenuMap,
	// Save / Load
	eHUDEvent_OnGameSave,
	eHUDEvent_OnGameLoad,
	eHUDEvent_LAST // KEEP LAST!
};

enum HUDEventHitType
{
	eHUDEventHT_Bullet = 0,
	eHUDEventHT_Explosive = 1,
	eHUDEventHT_numTypes = 2
};

//////////////////////////////////////////////////////////////////////////


struct SHUDEventData
{
	union
	{
		int			m_int;
		float		m_float;
		bool		m_bool;
		void*		m_pointer;
	};

	SHUDEventData(int val) : m_int(val) {}
	SHUDEventData(float val) : m_float(val) {}
	SHUDEventData(bool val) : m_bool(val) {}
	SHUDEventData(void* val) : m_pointer(val) {}

	int GetInt() const { return m_int; }
	float GetFloat() const { return m_float; }
	bool GetBool() const { return m_bool; }
	void* GetPtr() const { return m_pointer; }
};


//////////////////////////////////////////////////////////////////////////


struct SHUDEvent
{
	SHUDEvent()
		: eventType(eHUDEvent_None)
		, eventPtrData(NULL)
		, eventIntData(0)
		, eventIntData2(0)
		, eventIntData3(0)
		, eventFloatData(0.0f)
		, eventFloat2Data(0.0f)
	{
		m_data.reserve(2);
	}

	SHUDEvent(EHUDEventType type)
		: eventType(type)
	{
		SHUDEvent();
	}

	~SHUDEvent() {}

	void ReserveData(uint8 size)
	{
		m_data.reserve(size);
	}

	void AddData(const SHUDEventData& data)
	{
		m_data.push_back(data);
	}

	const SHUDEventData& GetData(uint8 index) const
	{
		CRY_ASSERT_MESSAGE(index < m_data.size(), "More SHUDEventData requested than provided!");
		return m_data[index];
	}

	const unsigned int GetDataSize( void ) const
	{
		return m_data.size();
	}

	std::vector<SHUDEventData> m_data;
	EHUDEventType	eventType;

	//the following are depricated and should not be used anymore
	void* eventPtrData;
	int eventIntData;
	int eventIntData2;
	int eventIntData3;
	float eventFloatData;
	float eventFloat2Data;
};


//////////////////////////////////////////////////////////////////////////

struct SHUDEventData_String
{
	SHUDEventData_String()
	{
	}

	CryFixedStringT<32> string1;
	CryFixedStringT<32> string2;
};

//////////////////////////////////////////////////////////////////////////


struct IHUDEventListener
{
	virtual void OnHUDEvent(const SHUDEvent& event) = 0;
};


//////////////////////////////////////////////////////////////////////////

// These are static as there are time when the HUD itself may not be instantiated
// when registering unregistering listeners.
class CHUDEventDispatcher
{
	typedef std::vector<IHUDEventListener*>							THUDEventListeners;
	typedef std::map<EHUDEventType, THUDEventListeners> TEventMap;

public:

	static void												AddHUDEventListener(IHUDEventListener* listener, const char* eventName);
	static void												RemoveHUDEventListener(const IHUDEventListener* listener);
	static void												CallEvent(const SHUDEvent& event);
	static EHUDEventType							GetEvent(const char* eventName);

	static void                       CheckRegisteredEvents( void );

private:

	static TEventMap									s_eventMap;

};


//////////////////////////////////////////////////////////////////////////

#endif

