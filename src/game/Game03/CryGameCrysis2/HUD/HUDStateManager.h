/************************************************************************/
/* HUD State management system for Crysis2, 2009									*/
/************************************************************************/

#ifndef __HUDSTATEMANAGER_H__
#define __HUDSTATEMANAGER_H__

#include "HUD/HUD.h"
#include "GameRulesTypes.h"

struct IHUDState;
class CHUDObjectManager;
class CUIElementManager;

#define HUD_STATEMGR_GAMEMODEADDONS_LEN  1
#define HUD_STATEMGR_MODEADDONSET_OBJS_LEN  1
#define HUD_STATEMGR_GAMEMODEADDONS_TYPENAME_LEN  16

struct SHUDModeAddonSet
{
public:
	void Init(const IItemParamsNode* pnode, CHUDObjectManager* pObjectManager, CUIElementManager* pElementManager, CHUDStateManager* pStateManager);
	void Update(float ftime);
	void Draw();
private:
	CHUDObject*  m_objs[HUD_STATEMGR_MODEADDONSET_OBJS_LEN];
	int  m_numObjs;
};

struct SHUDStateGameModeAddon
{
public:
	void Init(const IItemParamsNode* pnode, CHUDObjectManager* pObjectManager, CUIElementManager* pElementManager, CHUDStateManager* pStateManager);
	void Update(const EGameMode gameModeIdx, float ftime);
	void Draw(const EGameMode gameModeIdx);
	const char* GetTypeName() { return m_typeName; }
private:
	SHUDModeAddonSet  m_modeSets[eGM_NUM_GAMEMODES];
	char  m_typeName[HUD_STATEMGR_GAMEMODEADDONS_TYPENAME_LEN];
};

class CHUDStateManager{
public : // Types

	typedef CryFixedStringT<32> TStateName;
	typedef std::map<TStateName, IHUDState*> TStates;

public : // Interface

	 CHUDStateManager(CHUDObjectManager* pObjectManager, CUIElementManager* pElementManager);
	~CHUDStateManager();

	void LoadStatesFromFileList( char* xml_path );

	void LoadStates( const char* filename );

	void DeactivateAllStates( void );

	void ActivateState( const char* name );
	void ActivateState( const TStateName& name );

	void ClearStates( void );

	void UpdateStates( float frameTime );

	IHUDState* FindStateByName( const char* name );
	IHUDState* FindStateByName( const TStateName& name );
	const char* FindStateName( const IHUDState* pState ) const;

	SHUDStateGameModeAddon* FindGameModeAddonByTypeName(const char* typeName);

	const TStates* GetStates( void ) const { return &m_hudStates; }
	IHUDState*     GetActiveState( void ){ return m_activeState; }

#if ENABLE_HUD_EXTRA_DEBUG
	void PrintStateInfo( void ) const;
	void PrintStateObjectInfo( const IHUDState* pState ) const;
#endif

	void GetMemoryUsage(ICrySizer *pSizer) const
	{	
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_hudStates);
	}
public : // Data members
	static IHUDState*  s_pHUDStateNULL;

private : // Data members
	SHUDStateGameModeAddon  m_gameModeAddons[HUD_STATEMGR_GAMEMODEADDONS_LEN];

	TStates  m_hudStates;

	int  m_numGameModeAddons;

	IHUDState*         m_activeState;
	CHUDObjectManager* m_objectManager;
	CUIElementManager*  m_elementManager;
};


#endif // __HUDSTATEMANAGER_H__
