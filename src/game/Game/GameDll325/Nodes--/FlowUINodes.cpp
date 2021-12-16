#include "StdAfx.h"
#include "Nodes/G2FlowBaseNode.h"

#include <IGame.h>
#include <IFlashUI.h>
#include <IGameFramework.h>
#include "IVehicleSystem.h"

#include "Player.h"
#include "Weapon.h"

//---------------------------------------------------------------------------------------
//------------------------------------- Minimap -----------------------------------------
//---------------------------------------------------------------------------------------
class CMiniMapInfo
{
public:
	static CMiniMapInfo* GetInstance()
	{
		static CMiniMapInfo inst;
		return &inst;
	}

	struct LevelMapInfo {
		LevelMapInfo() : fStartX(0), fStartY(0), fEndX(1), fEndY(1), fDimX(1), fDimY(1), iWidth(1024), iHeight(1024) {}
		string sMinimapName;
		int iWidth;
		int iHeight;
		float fStartX;
		float fStartY;
		float fEndX;
		float fEndY;
		float fDimX;
		float fDimY;
	};

	const LevelMapInfo& GetLevelInfo() { UpdateLevelInfo(); return m_LevelMapInfo; }

	void GetPlayerData( IEntity* pEntity, float& px, float &py, int& rot ) const
	{
		if (pEntity)
		{
			px = clamp( ( pEntity->GetWorldPos().x - m_LevelMapInfo.fStartX ) / m_LevelMapInfo.fDimX, 0, 1 );
			py = clamp( ( pEntity->GetWorldPos().y - m_LevelMapInfo.fStartY ) / m_LevelMapInfo.fDimY, 0, 1 );
			rot = (int) ( pEntity->GetWorldAngles().z * 180.0f/gf_PI - 90.0f );
		}
	}

private:
	CMiniMapInfo() {}
	CMiniMapInfo( const CMiniMapInfo& ) {}
	CMiniMapInfo& operator=( const CMiniMapInfo& ) {}
	~CMiniMapInfo() {}

	LevelMapInfo m_LevelMapInfo;

	void UpdateLevelInfo()
	{
		ILevel* pLevel = gEnv->pGame->GetIGameFramework()->GetILevelSystem()->GetCurrentLevel();
		if ( pLevel != NULL && pLevel->GetLevelInfo() != NULL )
		{
			IXmlParser*	pxml = gEnv->pGame->GetIGameFramework()->GetISystem()->GetXmlUtils()->CreateXmlParser();
			if( !pxml ) return;

			char tmp[255];
			sprintf( tmp,"%s/%s.xml", pLevel->GetLevelInfo()->GetPath(), pLevel->GetLevelInfo()->GetName() );
			XmlNodeRef node = GetISystem()->LoadXmlFile( tmp );
			if( !node ) return;

			node = node->findChild( "Minimap" );
			if ( node ) 
			{
				const char* minimap_dds;
				node->getAttr( "Filename", &minimap_dds );
				sprintf( tmp,"%s/%s", pLevel->GetLevelInfo()->GetPath(), minimap_dds );

				node->getAttr( "startX", m_LevelMapInfo.fStartX );
				node->getAttr( "startY", m_LevelMapInfo.fStartY );
				node->getAttr( "endX", m_LevelMapInfo.fEndX );
				node->getAttr( "endY", m_LevelMapInfo.fEndY );
				node->getAttr( "width", m_LevelMapInfo.iWidth );
				node->getAttr( "height", m_LevelMapInfo.iHeight );
				m_LevelMapInfo.fDimX = m_LevelMapInfo.fEndX - m_LevelMapInfo.fStartX;
				m_LevelMapInfo.fDimY = m_LevelMapInfo.fEndY - m_LevelMapInfo.fStartY;
				m_LevelMapInfo.fDimX = m_LevelMapInfo.fDimX > 0 ? m_LevelMapInfo.fDimX : 1;
				m_LevelMapInfo.fDimY = m_LevelMapInfo.fDimY > 0 ? m_LevelMapInfo.fDimY : 1;
				m_LevelMapInfo.sMinimapName = tmp;
			}
		}
	}
};

//---------------------------------------------------------------------------------------
//------------------------------------- Player ------------------------------------------
//---------------------------------------------------------------------------------------
struct IPlayerInfoListener
{
	virtual void OnMove( float px, float py, int rot ) {}
	virtual void OnHealthChange( int health ) {}
	virtual void OnShoot( int currammo ) {}
	virtual void OnWeaponChange( EntityId weaponId, const char* weaponname, int ammoType, const char* ammoName, int maxammo, int currammo ) {}
	virtual void OnWeaponZoom ( bool zoomed ) {}
	virtual void OnCameraChange( bool isThirdPerson ) {}
	virtual void OnOutOfAmmo() {}
	virtual void OnReloadEnd( int currammo ) {}
};

class CPlayerInfo 
	: public IWeaponEventListener
	, public IItemSystemListener
	, public IGameFrameworkListener
	, public ILevelSystemListener
{
public:
	static CPlayerInfo* GetInstance()
	{
		static CPlayerInfo inst;
		return &inst;
	}

	void addListener( IPlayerInfoListener* pListener )
	{
		stl::push_back_unique( m_Listener, pListener );
		m_bForceNotify = true;
	}

	void removeListener( IPlayerInfoListener* pListener )
	{
		m_Listener.remove( pListener );
	}

	// IItemSystemListener
	virtual void OnSetActorItem(IActor *pActor, IItem *pItem )
	{
		if (getPlayer() == pActor)
		{
			CWeapon* pOldWeapon = getPlayer()->GetWeapon( m_currentWeaponId );
			if ( pOldWeapon )
			{
				pOldWeapon->RemoveEventListener( this );
			}

			m_currentWeaponId = 0;

			CWeapon* pCurrentWeapon = NULL;
			if ( pItem )
				pCurrentWeapon = getPlayer()->GetWeapon( pItem->GetEntityId() );

			if ( pCurrentWeapon )
			{
				m_currentWeaponId = pCurrentWeapon->GetEntityId();
				pCurrentWeapon->AddEventListener( this, "CPlayerInfo" );
			}

			NotifyWeaponChange( pCurrentWeapon );
		}
	}
	virtual void OnDropActorItem(IActor *pActor, IItem *pItem ) {}
	virtual void OnSetActorAccessory(IActor *pActor, IItem *pItem ) {}
	virtual void OnDropActorAccessory(IActor *pActor, IItem *pItem ) {}
	// ~IItemSystemListener

	// IWeaponEventListener
	virtual void OnStartFire(IWeapon *pWeapon, EntityId shooterId) {}
	virtual void OnStopFire(IWeapon *pWeapon, EntityId shooterId)  {}
	virtual void OnStartReload(IWeapon *pWeapon, EntityId shooterId, IEntityClass* pAmmoType) {}
	virtual void OnSetAmmoCount(IWeapon *pWeapon, EntityId shooterId) {}
	virtual void OnReadyToFire(IWeapon *pWeapon) {}
	virtual void OnPickedUp(IWeapon *pWeapon, EntityId actorId, bool destroyed) {}
	virtual void OnDropped(IWeapon *pWeapon, EntityId actorId) {}
	virtual void OnMelee(IWeapon* pWeapon, EntityId shooterId)  {}
	virtual void OnSelected(IWeapon *pWeapon, bool selected) {}
	virtual void OnStartTargetting(IWeapon *pWeapon) {}
	virtual void OnStopTargetting(IWeapon *pWeapon) {} 

	virtual void OnEndReload(IWeapon *pWeapon, EntityId shooterId, IEntityClass* pAmmoType)
	{
		IFireMode* pFireMode = pWeapon->GetFireMode( pWeapon->GetCurrentFireMode() );
		if ( pFireMode )
		{
			for ( std::list< IPlayerInfoListener* >::iterator it = m_Listener.begin(); it != m_Listener.end(); ++it )
				(*it)->OnReloadEnd( pFireMode->GetClipSize() + (pFireMode->GetAmmoCount() > 0 ? 1 : 0) );
		}
	}

	virtual void OnShoot(IWeapon *pWeapon, EntityId shooterId, EntityId ammoId, IEntityClass* pAmmoType,
		const Vec3 &pos, const Vec3 &dir, const Vec3 &vel) 
	{		
		IFireMode* pFireMode = pWeapon->GetFireMode(pWeapon->GetCurrentFireMode());
		if ( pFireMode )
		{
			for ( std::list< IPlayerInfoListener* >::iterator it = m_Listener.begin(); it != m_Listener.end(); ++it )
				(*it)->OnShoot( pFireMode->GetAmmoCount() - 1 );
		}
	}

	virtual void OnFireModeChanged(IWeapon *pWeapon, int currentFireMode)
	{
		NotifyWeaponChange( static_cast< CWeapon* > ( pWeapon ) );
	}

	virtual void OnOutOfAmmo(IWeapon *pWeapon, IEntityClass* pAmmoType)
	{
		for ( std::list< IPlayerInfoListener* >::iterator it = m_Listener.begin(); it != m_Listener.end(); ++it )
			(*it)->OnOutOfAmmo();
	}
	// ~IWeaponEventListener

	// ILevelSystemListener
	virtual void OnLevelNotFound(const char *levelName) {}
	virtual void OnLoadingStart(ILevelInfo *pLevel)
	{
		m_currentWeaponId = 0;
	}
	virtual void OnLoadingComplete(ILevel *pLevel)
	{
		CPlayer* pPlayer = getPlayer();
		if (pPlayer)
		{
			IItem* pItem = pPlayer->GetCurrentItem();
			OnSetActorItem( pPlayer, pItem );
		}
	}
	virtual void OnLoadingError(ILevelInfo *pLevel, const char *error) {}
	virtual void OnLoadingProgress(ILevelInfo *pLevel, int progressAmount) {}
	// ~ILevelSystemListener


	// IGameFrameworkListener
	virtual void OnSaveGame(ISaveGame* pSaveGame) {}
	virtual void OnLoadGame(ILoadGame* pLoadGame) {}
	virtual void OnLevelEnd(const char* nextLevel) {}
	virtual void OnActionEvent(const SActionEvent& event)
	{
		if ( event.m_event == eAE_inGame )
		{
			IActor* pPlayer = getPlayer();
			if ( pPlayer )
			{
				OnSetActorItem( pPlayer, pPlayer->GetCurrentItem() );
			}
		}
	}

	virtual void OnPostUpdate(float fDeltaTime)
	{
		CPlayer* pPlayer = getPlayer();
		if (pPlayer)
		{
			float x = .5f;
			float y = .5f;
			int r = 0;
			CMiniMapInfo::GetInstance()->GetPlayerData(pPlayer->GetEntity(), x, y, r);
			if (x != m_fXPos || y != m_fYPos || r != m_iRot || m_bForceNotify )
			{
				for ( std::list< IPlayerInfoListener* >::iterator it = m_Listener.begin(); it != m_Listener.end(); ++it )
					(*it)->OnMove(x, y, r);

				m_fXPos = x;
				m_fYPos = y;
				m_iRot = r;
			}
			if (pPlayer->GetHealth() != m_iHealth || m_bForceNotify)
			{
				m_iHealth = pPlayer->GetHealth();
				for ( std::list< IPlayerInfoListener* >::iterator it = m_Listener.begin(); it != m_Listener.end(); ++it )
					(*it)->OnHealthChange(m_iHealth > 0 ? m_iHealth : 0);
			}

			CWeapon* pWeapon = pPlayer->GetWeapon( m_currentWeaponId );
			if ( pWeapon && (pWeapon->IsZoomed() != m_bZoomed || m_bForceNotify) )
			{
				m_bZoomed = pWeapon->IsZoomed();
				for ( std::list< IPlayerInfoListener* >::iterator it = m_Listener.begin(); it != m_Listener.end(); ++it )
					(*it)->OnWeaponZoom( m_bZoomed );
			}
			if ( pPlayer->IsThirdPerson() != m_bIsThirdPerson || m_bForceNotify)
			{
				m_bIsThirdPerson = pPlayer->IsThirdPerson();
				for ( std::list< IPlayerInfoListener* >::iterator it = m_Listener.begin(); it != m_Listener.end(); ++it )
					(*it)->OnCameraChange( m_bIsThirdPerson );
			}
			if ( m_bForceNotify )
				m_bForceNotify = false;
		}
	}
	// ~IGameFrameworkListener

private:
	CPlayerInfo() 
	{
		m_fXPos = -1.f;
		m_fYPos = -1.f;
		m_iRot = 0;
		m_iHealth = -1;
		m_bZoomed = false;
		m_bForceNotify = true;
		m_bIsThirdPerson = getPlayer() ? !getPlayer()->IsThirdPerson() : true;
		m_currentWeaponId = 0;

		IItemSystem* pItemSys = g_pGame->GetIGameFramework()->GetIItemSystem();
		if ( pItemSys )
			pItemSys->RegisterListener(this);

		if ( gEnv->pGame && gEnv->pGame->GetIGameFramework() && gEnv->pGame->GetIGameFramework()->GetILevelSystem() )
			gEnv->pGame->GetIGameFramework()->GetILevelSystem()->AddListener( this );

		if ( gEnv->pGame && gEnv->pGame->GetIGameFramework() )
			gEnv->pGame->GetIGameFramework()->RegisterListener(this, "HUD", FRAMEWORKLISTENERPRIORITY_HUD);
	}

	CPlayerInfo( const CMiniMapInfo& ) {}
	CPlayerInfo& operator=( const CMiniMapInfo& ) {}
	~CPlayerInfo() {}

	EntityId m_currentWeaponId;
	float m_fXPos;
	float m_fYPos;
	int m_iRot;
	int m_iHealth;
	bool m_bZoomed;
	bool m_bIsThirdPerson;
	bool m_bForceNotify;
	std::list< IPlayerInfoListener* > m_Listener;

	CPlayer* getPlayer()
	{
		return static_cast< CPlayer* > ( gEnv->pGame->GetIGameFramework()->GetClientActor() );
	}


	void NotifyWeaponChange( CWeapon * pWeapon )
	{
		if ( pWeapon )
		{
			IEntity* pWeaponEntity = pWeapon->GetEntity();
			IFireMode* pFireMode = pWeapon->GetFireMode( pWeapon->GetCurrentFireMode() );

			if ( pFireMode )
			{
				for ( std::list< IPlayerInfoListener* >::iterator it = m_Listener.begin(); it != m_Listener.end(); ++it )
					(*it)->OnWeaponChange(  pWeaponEntity->GetId(), 
					pWeaponEntity->GetClass() ? pWeaponEntity->GetClass()->GetName() : "UNDEFINED",
					pWeapon->GetCurrentFireMode(),
					pFireMode->GetName(),
					pFireMode->GetClipSize(), 
					pFireMode->GetAmmoCount() );
			}
		}
	}


};

//---------------------------------------------------------------------------------------
//------------------------------------- Nodes -------------------------------------------
//---------------------------------------------------------------------------------------


//----------------------------------- MiniMap info --------------------------------------
class CFlowMiniMapInfoNode 
	: public CFlowBaseNode
{
public:
	CFlowMiniMapInfoNode( SActivationInfo * pActInfo )
	{
	}

	virtual void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig_Void  ( "Get", _HELP("Get minimap info") ),
			{0}
		};
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig_Void		( "OnGet",	_HELP( "Tirggers of port <Get> is activeated" ) ),
			OutputPortConfig<string>	( "MapFile",	_HELP( "Name of minimap dds file" ) ),
			OutputPortConfig<int>		( "Width",		_HELP( "Minimap width" ) ),
			OutputPortConfig<int>		( "Height",		_HELP( "Minimap height" ) ),
			{0}
		};
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP( "Info about minimap" );
		config.SetCategory( EFLN_ADVANCED );
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if (event == eFE_Activate && IsPortActive( pActInfo, eI_Get ))
		{
			const CMiniMapInfo::LevelMapInfo mapInfo = CMiniMapInfo::GetInstance()->GetLevelInfo();
			ActivateOutput( pActInfo, eO_OnGet,   true );
			ActivateOutput( pActInfo, eO_MapName,   mapInfo.sMinimapName );
			ActivateOutput( pActInfo, eO_MapWidth,  mapInfo.iWidth );
			ActivateOutput( pActInfo, eO_MapHeight, mapInfo.iHeight );
		}
	}

	virtual void GetMemoryUsage( ICrySizer * s ) const
	{
		s->Add( *this );
	}

private:
	enum EInputPorts
	{
		eI_Get,
	};

	enum EOutputPorts
	{
		eO_OnGet = 0,
		eO_MapName,
		eO_MapWidth,
		eO_MapHeight,
	};

};

//----------------------------------- Player pos info --------------------------------------
class CFlowMiniMapPlayerPosInfo
	: public CFlowBaseNode
	, public IPlayerInfoListener
{
public:
	CFlowMiniMapPlayerPosInfo( SActivationInfo * pActInfo )
	{
	}

	virtual ~CFlowMiniMapPlayerPosInfo()
	{
		CPlayerInfo::GetInstance()->removeListener( this );
	}

	virtual void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig_Void	( "OnPosChange",_HELP( "Triggers if position has changed" ) ),
			OutputPortConfig<float>	( "PosX",		_HELP( "Player x pos on minimap" ) ),
			OutputPortConfig<float>	( "PosY",		_HELP( "Player y pos on minimap" ) ),
			OutputPortConfig<int>	( "Rotation",	_HELP( "Minimap rotation" ) ),
			{0}
		};

		config.pInputPorts = 0;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP( "Info about player position on minimap" );
		config.SetCategory( EFLN_ADVANCED );
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if ( event == eFE_Initialize )
		{
			m_ActInfo = *pActInfo;
			CPlayerInfo::GetInstance()->addListener( this );
		}
	}

	virtual void GetMemoryUsage( ICrySizer * s ) const
	{
		s->Add( *this );
	}

	virtual void OnMove( float px, float py, int rot )
	{
		ActivateOutput( &m_ActInfo, eO_PosUpdate,		true);
		ActivateOutput( &m_ActInfo, eO_PosX,		px );
		ActivateOutput( &m_ActInfo, eO_PosY,		py );
		ActivateOutput( &m_ActInfo, eO_Rotation,	rot );
	}

private:
	SActivationInfo m_ActInfo;

	enum EOutputPorts
	{
		eO_PosUpdate = 0,
		eO_PosX,
		eO_PosY,
		eO_Rotation,
	};
};

class CFlowMiniMapPlayerHitInfo
	: public CFlowBaseNode
	, public IPlayerInfoListener
{
public:
	CFlowMiniMapPlayerHitInfo( SActivationInfo * pActInfo )
	{
	}

	virtual ~CFlowMiniMapPlayerHitInfo()
	{
		CPlayerInfo::GetInstance()->removeListener( this );
	}

	virtual void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig_Void	( "OnHit",		_HELP( "Triggers if player is hit" ) ),
			OutputPortConfig<int>	( "Health",		_HELP( "Player health" ) ),
			{0}
		};

		config.pInputPorts = 0;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP( "Listener about player hit" );
		config.SetCategory( EFLN_ADVANCED );
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if ( event == eFE_Initialize )
		{
			m_ActInfo = *pActInfo;
			CPlayerInfo::GetInstance()->addListener( this );
		}
	}

	virtual void GetMemoryUsage( ICrySizer * s ) const
	{
		s->Add( *this );
	}

	virtual void OnHealthChange( int health )
	{
		ActivateOutput( &m_ActInfo, eO_Hit,		true);
		ActivateOutput( &m_ActInfo, eO_Health,	health );
	}

private:
	SActivationInfo m_ActInfo;

	enum EOutputPorts
	{
		eO_Hit = 0,
		eO_Health,
	};
};

//----------------------------------- Weapon info --------------------------------------
class CFlowMiniMapWeaponInfo
	: public CFlowBaseNode
	, public IPlayerInfoListener
{
public:
	CFlowMiniMapWeaponInfo( SActivationInfo * pActInfo )
	{
	}

	virtual ~CFlowMiniMapWeaponInfo()
	{
		CPlayerInfo::GetInstance()->removeListener( this );
	}

	virtual void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig_Void		( "OnChange",	_HELP( "Triggers if weapon changed" ) ),

			OutputPortConfig<EntityId>	( "WeaponId",	_HELP( "Weapon id" ) ),
			OutputPortConfig<string>	( "WeaponName",	_HELP( "Weapon name" ) ),
			OutputPortConfig<int>		( "AmmoType",	_HELP( "Ammo type" ) ),
			OutputPortConfig<string>	( "AmmoName",	_HELP( "Ammo name" ) ),
			OutputPortConfig<int>		( "MaxAmmo",	_HELP( "Max ammo of current weapon" ) ),
			OutputPortConfig<bool>		( "IsMelee",	_HELP( "Is melee weapon" ) ),
			{0}
		};

		config.pInputPorts = 0;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP( "Info about players current weapon" );
		config.SetCategory( EFLN_ADVANCED );
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if ( event == eFE_Initialize )
		{
			m_ActInfo = *pActInfo;
			CPlayerInfo::GetInstance()->addListener( this );
		}
	}

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

	virtual void OnWeaponChange( EntityId weaponId, const char* weaponname, int ammoType, const char* ammoName, int maxammo, int currammo )
	{
		ActivateOutput( &m_ActInfo, eO_OnChange,			true);
		ActivateOutput( &m_ActInfo, eO_CurrWeaponId,	weaponId );
		ActivateOutput( &m_ActInfo, eO_CurrWeaponName,	string( weaponname ) );
		ActivateOutput( &m_ActInfo, eO_CurrAmmoId,		ammoType );
		ActivateOutput( &m_ActInfo, eO_CurrAmmoName,	string( ammoName ) );
		ActivateOutput( &m_ActInfo, eO_MaxAmmo,			maxammo );
		ActivateOutput( &m_ActInfo, eO_IsMelee,			maxammo == 0 );
	}

private:
	SActivationInfo m_ActInfo;

	enum EOutputPorts
	{
		eO_OnChange = 0,
		eO_CurrWeaponId,
		eO_CurrWeaponName,
		eO_CurrAmmoId,
		eO_CurrAmmoName,
		eO_MaxAmmo,
		eO_IsMelee,
	};
};

class CFlowMiniMapWeaponListener
	: public CFlowBaseNode
	, public IPlayerInfoListener
{
public:
	CFlowMiniMapWeaponListener( SActivationInfo * pActInfo )
	{
	}

	virtual ~CFlowMiniMapWeaponListener()
	{
		CPlayerInfo::GetInstance()->removeListener( this );
	}

	virtual void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig_Void		( "OnShoot",	_HELP( "Triggers if weapon shoot" ) ),
			OutputPortConfig_Void		( "OnZoom",	_HELP( "Triggers if weapon zoom is chage" ) ),
			OutputPortConfig_Void		( "OnReloaded",	_HELP( "Triggers if weapon was reloaded" ) ),

			OutputPortConfig<int>		( "Ammo",	_HELP( "Curreent ammo count" ) ),
			OutputPortConfig<bool>		( "IsZoomed",	_HELP( "Is weapon in zoom mode" ) ),
			{0}
		};

		config.pInputPorts = 0;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP( "Info about players current weapon" );
		config.SetCategory( EFLN_ADVANCED );
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if ( event == eFE_Initialize )
		{
			m_ActInfo = *pActInfo;
			CPlayerInfo::GetInstance()->addListener( this );
		}
	}

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

	virtual void OnWeaponChange( EntityId weaponId, const char* weaponname, int ammoType, const char* ammoName, int maxammo, int currammo )
	{
		ActivateOutput( &m_ActInfo, eO_Ammo,			currammo );
	}

	virtual void OnShoot( int currammo )
	{
		ActivateOutput( &m_ActInfo, eO_OnShoot,			true);
		ActivateOutput( &m_ActInfo, eO_Ammo,			currammo );
	}

	virtual void OnWeaponZoom( bool zoomed )
	{
		ActivateOutput( &m_ActInfo, eO_OnZoom,			true);
		ActivateOutput( &m_ActInfo, eO_IsZoomed,		zoomed );
	}

	virtual void OnReloadEnd( int currammo )
	{
		ActivateOutput( &m_ActInfo, eO_OnReloaded,		true);
		ActivateOutput( &m_ActInfo, eO_Ammo,			currammo );
	}

private:
	SActivationInfo m_ActInfo;

	enum EOutputPorts
	{
		eO_OnShoot = 0,
		eO_OnZoom,
		eO_OnReloaded,

		eO_Ammo,
		eO_IsZoomed,
	};
};

//----------------------------------- Camera info --------------------------------------
class CFlowMiniMapCameraInfo
	: public CFlowBaseNode
	, public IPlayerInfoListener
{
public:
	CFlowMiniMapCameraInfo( SActivationInfo * pActInfo )
	{
	}

	virtual ~CFlowMiniMapCameraInfo()
	{
		CPlayerInfo::GetInstance()->removeListener( this );
	}

	virtual void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig_Void	( "OnChange",		_HELP( "Triggers if camera changed" ) ),
			OutputPortConfig<bool>	( "IsThirdPerson",	_HELP( "True if camera is third person" ) ),
			{0}
		};

		config.pInputPorts = 0;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP( "Info about current camera state" );
		config.SetCategory( EFLN_ADVANCED );
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		if ( event == eFE_Initialize )
		{
			m_ActInfo = *pActInfo;
			CPlayerInfo::GetInstance()->addListener( this );
		}
	}

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

	virtual void OnCameraChange( bool isThirdPerson )
	{
		ActivateOutput( &m_ActInfo, eO_Update,	true);
		ActivateOutput( &m_ActInfo, eO_ThirdPerson,	isThirdPerson );
	}

private:
	SActivationInfo m_ActInfo;

	enum EOutputPorts
	{
		eO_Update = 0,
		eO_ThirdPerson,
	};
};

REGISTER_FLOW_NODE_SINGLETON("Minimap:MapInfo", CFlowMiniMapInfoNode);
REGISTER_FLOW_NODE("Minimap:PlayerPos", CFlowMiniMapPlayerPosInfo);

REGISTER_FLOW_NODE("Player:HitListener", CFlowMiniMapPlayerHitInfo);
REGISTER_FLOW_NODE("Player:CurrentWeapon", CFlowMiniMapWeaponInfo);
REGISTER_FLOW_NODE("Player:WeaponListener", CFlowMiniMapWeaponListener);

REGISTER_FLOW_NODE("Camera:ThirdPerson", CFlowMiniMapCameraInfo);