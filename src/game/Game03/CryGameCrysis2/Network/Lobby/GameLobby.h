#ifndef ___GAME_LOBBY_H___
#define ___GAME_LOBBY_H___

#include "ICryLobby.h"
#include "ICryMatchMaking.h"
#include "GameMechanismManager/GameMechanismBase.h"

struct SGameStartParams;

struct SLobbyGameStartParams
{
	unsigned flags;
	uint16 port;
	int maxPlayers;

	CryFixedStringT<32> hostname;
	CryFixedStringT<32> connectionString;
	CryFixedStringT<32> levelName;
	CryFixedStringT<32> gameModeName;
	CryFixedStringT<32> demoRecorderFilename;
	CryFixedStringT<32> demoPlaybackFilename;

	SLobbyGameStartParams()
	{
		flags = 0;
		port = 0;
		maxPlayers = 16;
	}
};

enum ELobbyTaskType
{
	eLTT_Joining = 0,
	eLTT_Creating,
	eLTT_Deleting,
	eLTT_Num,
};

class CGameLobby : public CGameMechanismBase
{

public :
	CGameLobby();
	virtual void Update( float dt );

	void ClearLobbyTaskID(ELobbyTaskType taskId) { m_lobbyTasks[taskId] = CryLobbyInvalidTaskID; }
	void CancelLobbyTask(ELobbyTaskType taskId);
	void CancelAllLobbyTasks();

	void JoinServer(CrySessionID sessionId, const char *sessionName);

	void CreateSession( const SGameStartParams* pGameStartParams );
	void DeleteSession();

	CrySessionHandle GetSessionId() const { return m_currentSession; }
	bool IsSessionRunning() const { return m_currentSession != CrySessionInvalidHandle; }
private:
	SLobbyGameStartParams * GetLobbyGameStartParams() { return m_gameStartParams; }
	void CreateGameNextFrame() { assert(!m_creatingGame); m_creatingGame = true; }

	void SetCurrentSession(CrySessionHandle h) { m_currentSession = h; }

	bool m_creatingGame;	// TODO: State based if more lobby states are required. For now only creating a game requires the update.

	SLobbyGameStartParams *m_gameStartParams;
	CrySessionHandle m_currentSession;

	CryLobbyTaskID m_lobbyTasks[eLTT_Num];

	// Callbacks
	static void MatchmakingSessionCreateCallback(CryLobbyTaskID taskID, ECryLobbyError error, CrySessionHandle h, void* arg);
	static void MatchmakingSessionMigrateCallback(CryLobbyTaskID taskID, ECryLobbyError error, CrySessionHandle h, void* arg);
	static void MatchmakingSessionJoinCallback(CryLobbyTaskID taskID, ECryLobbyError error, CrySessionHandle h, uint32 ip, uint16 port, void* arg);
	static void MatchmakingSessionDeleteCallback(CryLobbyTaskID taskID, ECryLobbyError error, void* arg);
};

#endif // ___GAME_LOBBY_H___
