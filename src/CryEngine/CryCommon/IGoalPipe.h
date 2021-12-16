#include DEVIRTUALIZE_HEADER_FIX(IGoalPipe.h)

#ifndef __IGOALPIPE_H_
#define __IGOALPIPE_H_


#if _MSC_VER > 1000
#pragma once
#endif

struct GoalParameters
{
	Vec3 vPos;
	Vec3 vPosAux;
	
	IAIObject* pTarget;
	
	float fValue;
	float fValueAux;
	
	int nValue;
	int nValueAux;
	
	bool bValue;
	
	string str;
	string strAux;

	SmartScriptFunction scriptFunction;
	ScriptAnyValue scriptUserData;


	GoalParameters() :
		vPos(ZERO),
		vPosAux(ZERO),
		pTarget(0),
		fValue(0.f),
		fValueAux(0.f),
		nValue(0),
		nValueAux(0),
		bValue(false)
	{
	}

	~GoalParameters()
	{
		gEnv->pScriptSystem->ReleaseAny(scriptUserData);
	}

	GoalParameters& operator=(const GoalParameters& params)
	{
		if (&params != this)
		{
			vPos      = params.vPos;
			vPosAux   = params.vPosAux;
			
			pTarget   = params.pTarget; 
			
			fValue    = params.fValue;
			fValueAux = params.fValueAux;
			
			nValue    = params.nValue;
			nValueAux = params.nValueAux;
			
			bValue    = params.bValue;
			
			str       = params.str;
			strAux    = params.strAux;

			gEnv->pScriptSystem->ReleaseAny(scriptUserData);

			scriptFunction = params.scriptFunction;
			scriptUserData = gEnv->pScriptSystem->CloneAny(params.scriptUserData);
		}
		
		return *this;
	}

	void Serialize(TSerialize ser);
};


enum EGoalOperations
{
	eGO_FIRST,
	eGO_ACQUIRETARGET = eGO_FIRST,
	eGO_ADJUSTAIM,
	eGO_ANIMATION,
	eGO_ANIMTARGET,
	eGO_APPROACH,
	eGO_BACKOFF,
	eGO_BODYPOS,
	eGO_BRANCH,
	eGO_CHARGE,
	eGO_CLEAR,
	eGO_COMMUNICATE,
	eGO_COMPANIONSTICK,
	eGO_CONTINUOUS,
	eGO_DEVALUE,
	eGO_DODGE,
	eGO_FIRECMD,
	eGO_FOLLOWPATH,
	eGO_FORGET,
	eGO_FORM,
	eGO_G4APPROACH,	
	eGO_HIDE,
	eGO_IGNOREALL,
	eGO_LOCATE,
	eGO_LOOK,		
	eGO_LOOKAROUND,
	eGO_LOOKAT,
	eGO_MOVETOWARDS,
	eGO_PATHFIND,
	eGO_PROXIMITY,
	eGO_RANDOM,
	eGO_RUN,
	eGO_SCRIPT,
	eGO_SEEKCOVER,
	eGO_SIGNAL,
	eGO_SPEED,
	eGO_STANCE,
	eGO_STEER,
	eGO_STICK,
	eGO_STICKMINIMUMDISTANCE,
	eGO_STRAFE,
	eGO_TACTICALPOS,
	eGO_TIMEOUT,
	eGO_TRACE,
	eGO_USECOVER,
	eGO_WAIT,
	eGO_WAITSIGNAL,
	eGO_LAST
};


struct IGoalOp;


UNIQUE_IFACE struct IGoalPipe
{
	enum EGroupType
	{
		eGT_NOGROUP,
		eGT_GROUPWITHPREV,
		eGT_GROUPED,
		eGT_LAST
	};

	virtual ~IGoalPipe() {}

	// TODO evgeny Further clean-up from here

	virtual const char* GetName() const = 0;
	virtual void HighPriority() = 0;

	virtual void PushGoal(const XmlNodeRef& goalOpNode, EGroupType eGrouping) = 0;

	// Push an existing goalop to the pipe with given parameters
	// Note: This new PushGoal shouldn't need all these parameters but will do for a while.
	virtual void PushGoal(IGoalOp* pGoalOp, EGoalOperations op, bool bBlocking, EGroupType eGrouping, const GoalParameters& params) = 0;

	virtual void PushGoal(EGoalOperations name, bool bBlocking, EGroupType eGrouping, GoalParameters& params) = 0;
	virtual void PushLabel(const char* label) = 0;
	virtual void PushPipe(const char* szName, bool bBlocking, EGroupType eGrouping, GoalParameters& params) = 0;
	virtual void SetDebugName(const char* name) = 0;
};


#endif	// #ifndef __IGOALPIPE_H_
