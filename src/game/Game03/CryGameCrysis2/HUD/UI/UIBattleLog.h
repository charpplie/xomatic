/************************************************************************/
/* UI BattleLog for Component HUD, Tim Furnish, 2009  									*/
/************************************************************************/

#ifndef UI_BATTLE_LOG_H
#define UI_BATTLE_LOG_H

#include "HUD/UI/UIElement.h"
#include "HUD/HUDEventDispatcher.h"

struct SBattleLogMessageInfo;

typedef CUIElement TBattleLogParent;

#define MessageListIDs(f)    \
	f(kMessageList_suicide)    \
	f(kMessageList_kill)       \

class CUIBattleLog : public TBattleLogParent, public IHUDEventListener
{
private :

	struct SBattleLogInfo
	{
		string msg;
		float timeLeft;
		float timeUntilRevealNextCharacter;
		int numCharactersRevealed;
	};

	struct SMessageDef
	{
		char msg[48];
		char type[16];
	};

	AUTOENUM_BUILDENUMWITHTYPE_WITHNUM(EMessageListID, MessageListIDs, kMessageList_num);

public:

	CUIBattleLog();
	virtual ~CUIBattleLog();

	// IUIElement
	void Initialize( const IItemParamsNode* params, IUIElement* parent );
	// ~IUIElement

	// IHUDEventListener
	virtual void OnHUDEvent(const SHUDEvent& event);
	// ~IHUDEventListener


private:

	void     NewBattleLogMessage(const SBattleLogMessageInfo & info);
	void     NewJoinMessage(EntityId eid);
	void     NewLeaveMessage(EntityId eid);
	void     NewPromotedMessage(EntityId eid, uint8 rank, const char* name);

	void     AppendGeneratedMessageToLog(const char * message);
	int      GetLocalPlayerTeam();
	char     GetInkColourForEntity(EntityId entityID, int localPlayerTeam);
	IEntity* GetEntityForMessage(EntityId eid);

	void Update                       (float frameTime);
	void Draw                         (void) const;
	void CreateMessageDefFromXML      (const IItemParamsNode *xmlElement, EMessageListID listID);
	void CreateMessageDefListFromXML  (const IItemParamsNode *xmlElement, EMessageListID listID);
	const SMessageDef * PickMessageDef(EMessageListID listID, const char * hitType);
	const SMessageDef * PickMessageDefIncludeDefault(EMessageListID listID, const char * hitType);

	static void CmdTestBattleLog      (IConsoleCmdArgs* pCmdArgs);

	std::vector<SBattleLogInfo> m_player_battle_log;
	std::vector<SMessageDef> m_messageDefinitions[kMessageList_num];
};

#endif	// UI_BATTLE_LOG_H
