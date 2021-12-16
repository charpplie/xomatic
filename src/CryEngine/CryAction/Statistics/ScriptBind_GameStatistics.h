#ifndef		__SCRIPTBIND_GAMESTATISTICS_H__
#define		__SCRIPTBIND_GAMESTATISTICS_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include <IScriptSystem.h>
#include <ScriptHelpers.h>

#define STATISTICS_CHECK_BINDING 1

struct IStatsTracker;
class CGameStatistics;

class CScriptBind_GameStatistics : public CScriptableBase
{
public:
	CScriptBind_GameStatistics(CGameStatistics* pGS);
	virtual ~CScriptBind_GameStatistics ();

	// <title PushGameScope>
	// Syntax: GameStatistics.PushGameScope()
	// Description:
	//		Pushes a scope on top of the stack.
	virtual int PushGameScope(IFunctionHandler *pH, int scopeID);
	// <title PopGameScope>
	// Syntax: GameStatistics.PopGameScope(int checkScopeID /*optional*/ )
	// Description:
	//		Removes the scope from the top of the stack.
	virtual int PopGameScope(IFunctionHandler *pH);


	// <title AddGameElement>
	// Syntax: GameStatistics.AddGameElement( scopeID, elementID, locatorID, locatorValue, table /*optional*/ )
	// Description:
	//		Adds a game element to specified scope.
	virtual int AddGameElement(IFunctionHandler *pH);
	// <title RemoveGameElement>
	// Syntax: GameStatistics.RemoveGameElement( scopeID, elementID, locatorID, locatorValue )
	// Description:
	//		Removes element from the specified scope if data parameters match.
	virtual int RemoveGameElement(IFunctionHandler *pH);

	// <title Event>
	// Syntax: GameStatistics.Event()
	virtual int Event(IFunctionHandler* pH);
	// <title StateValue>
	// Syntax: GameStatistics.StateValue( )
	virtual int StateValue(IFunctionHandler* pH);

	bool BindTracker(IScriptTable* pT, const char* name, IStatsTracker* tracker);
	bool UnbindTracker(IScriptTable* pT, const char* name, IStatsTracker* tracker);

private:
	void RegisterMethods();
	void RegisterGlobals();
	static IStatsTracker* GetTracker(IFunctionHandler* pH);
	static IGameStatistics* GetStats(IFunctionHandler* pH);

	//CScriptableBase m_StatsTracker; //[FINDME][SergeyR]: ctor is protected

	CGameStatistics *m_pGS;
	IScriptSystem	*m_pSS;

#if STATISTICS_CHECK_BINDING
	std::set<IStatsTracker*> m_boundTrackers;
#endif
};

#endif //__SCRIPTBIND_GAMESTATISTICS_H__
