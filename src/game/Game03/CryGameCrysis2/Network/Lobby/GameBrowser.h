#ifndef ___GAME_BROWSER_H___
#define ___GAME_BROWSER_H___

#include "ICryLobby.h"
#include "ICryMatchMaking.h"
#include "GameMechanismManager/GameMechanismBase.h"

class CGameBrowser : public CGameMechanismBase
{
public :

	CGameBrowser();
	void Init( void );
	virtual void Update( float dt );
	void StartSearchingForServers();
	void CancelSearching();

	const char * GetNatTypeString() const;

protected:
	ENatType m_NatType;
	CryLobbyTaskID m_searchingTask;
	void SetNatType(ENatType natType) { m_NatType = natType; }

	// Callbacks
	static void GetNatTypeCallback(UCryLobbyEventData eventData, void *userParam);
	static void InitialiseCallback(ECryLobbyService service, ECryLobbyError error, void* arg);
	static void ConfigurationCallback(ECryLobbyService service, SConfigurationParams *requestedParams, uint32 paramCount);
	static void MatchmakingSessionSearchCallback(CryLobbyTaskID taskID, ECryLobbyError error, SCrySessionSearchResult* session, void* arg);
};

#endif // ___GAME_LOBBY_H___
