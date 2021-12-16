// GoalPipe.cpp: implementation of the CGoalPipe class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "GoalPipe.h"

#include "AILog.h"
#include "CAISystem.h"
#include "GameSpecific/GoalOp_Crysis2.h"
#include "GameSpecific/GoalOp_G02.h"
#include "GameSpecific/GoalOp_G04.h" 		//TODO move these out of AISystem
#include "GoalOp.h"
#include "GoalOpFactory.h"
#include "ObjectTracker.h"
#include "PipeUser.h"

#include "ISystem.h"
#include "ISerialize.h"


namespace
{
struct SAIGoalOpName
{
	EGoalOperations op;
	const char* szName;
};

	SAIGoalOpName g_GoalOpNames[] =
{
		{ eGO_ACQUIRETARGET, "acqtarget" },
		{ eGO_ADJUSTAIM,     "adjustaim" },
		{ eGO_ANIMATION,     "animation" },
		{ eGO_ANIMTARGET,    "animtarget" },
		{ eGO_APPROACH,      "approach" },
		{ eGO_BACKOFF,       "backoff" },
		{ eGO_BODYPOS,       "bodypos" },	// Marcio: kept here for retro-compatibility
		{ eGO_BRANCH,        "branch" },
		{ eGO_CHARGE,        "charge" },
		{ eGO_CLEAR,         "clear" },
		{ eGO_COMMUNICATE,	 "communicate" },
		{ eGO_COMPANIONSTICK,"companionstick" },
		{ eGO_CONTINUOUS,    "continuous" },
		{ eGO_DEVALUE,       "devalue" },
		{ eGO_DODGE,         "dodge" },
		{ eGO_FIRECMD,       "firecmd" },
		{ eGO_FOLLOWPATH,    "followpath" },
		{ eGO_FORGET,        "forget" },
		{ eGO_FORM,          "form" },
		{ eGO_G4APPROACH,    "G4approach" },
		{ eGO_HIDE,          "hide" },
		{ eGO_IGNOREALL,     "ignoreall" },
		{ eGO_LOCATE,        "locate" },
		{ eGO_LOOK,          "look" },
		{ eGO_LOOKAROUND,    "lookaround" },
		{ eGO_LOOKAT,        "lookat" },
		{ eGO_MOVETOWARDS,   "movetowards" },
		{ eGO_PATHFIND,      "pathfind" },
		{ eGO_PROXIMITY,     "proximity" },
		{ eGO_RANDOM,        "random" },
		{ eGO_RUN,           "run" },			// Marcio: kept here for retro-compatibility
		{ eGO_SCRIPT,        "script" },
		{ eGO_SEEKCOVER,     "seekcover" },
		{ eGO_SIGNAL,        "signal" },
		{ eGO_SPEED,         "speed" },
		{ eGO_STANCE,        "stance" },
		{ eGO_STEER,         "steer" },
		{ eGO_STICK,         "stick" },
		{ eGO_STICKMINIMUMDISTANCE, "stickminimumdistance" },
		{ eGO_STRAFE,        "strafe" },
		{ eGO_TACTICALPOS,   "tacticalpos" },
		{ eGO_TIMEOUT,       "timeout" },
		{ eGO_TRACE,         "trace" },
		{ eGO_USECOVER,      "usecover" },
		{ eGO_WAIT,          "wait" },
		{ eGO_WAITSIGNAL,    "waitsignal" },

		{ eGO_LAST,          "" },						// Sentinel value - must end the array!
};
}


QGoal QGoal::Clone()
{
	QGoal goal;
	
	goal.op = op;

	if (pGoalOp)
	{
		switch (op)
		{
			case eGO_ACQUIRETARGET:        goal.pGoalOp = new COPAcqTarget         (static_cast<COPAcqTarget&>         (*pGoalOp)); break;
			case eGO_ADJUSTAIM:            goal.pGoalOp = new COPCrysis2AdjustAim  (static_cast<COPCrysis2AdjustAim&>  (*pGoalOp)); break;
			case eGO_ANIMATION:            goal.pGoalOp = new COPAnimation         (static_cast<COPAnimation&>         (*pGoalOp)); break;
			case eGO_ANIMTARGET:           goal.pGoalOp = new COPAnimTarget        (static_cast<COPAnimTarget&>        (*pGoalOp)); break;
			case eGO_APPROACH:             goal.pGoalOp = new COPApproach          (static_cast<COPApproach&>          (*pGoalOp)); break;
			case eGO_BRANCH:
				goal.pGoalOp = (gAIEnv.configuration.eCompatibilityMode == ECCM_CRYSIS2)
					? static_cast<CGoalOp*>(new COPDummy)
					: static_cast<CGoalOp*>(new COPDeValue);
				break;
			case eGO_BACKOFF:              goal.pGoalOp = new COPBackoff           (static_cast<COPBackoff&>           (*pGoalOp)); break;
			case eGO_BODYPOS:
				case eGO_STANCE:             goal.pGoalOp = new COPBodyCmd           (static_cast<COPBodyCmd&>           (*pGoalOp)); break;
			case eGO_CLEAR:                goal.pGoalOp = new COPClear;
			case eGO_COMMUNICATE:          goal.pGoalOp = new COPCrysis2Communicate(static_cast<COPCrysis2Communicate&>(*pGoalOp)); break;
			case eGO_COMPANIONSTICK:       goal.pGoalOp = new COPCompanionStick    (static_cast<COPCompanionStick&>    (*pGoalOp)); break;
			case eGO_CONTINUOUS:           goal.pGoalOp = new COPContinuous        (static_cast<COPContinuous&>        (*pGoalOp)); break;
			case eGO_DEVALUE:              goal.pGoalOp = new COPDeValue           (static_cast<COPDeValue&>           (*pGoalOp)); break;
			case eGO_DODGE:                goal.pGoalOp = new COPDodge             (static_cast<COPDodge&>             (*pGoalOp)); break;
			case eGO_FIRECMD:              goal.pGoalOp = new COPFireCmd           (static_cast<COPFireCmd&>           (*pGoalOp)); break;
			case eGO_FOLLOWPATH:           goal.pGoalOp = new COPFollowPath        (static_cast<COPFollowPath&>        (*pGoalOp)); break;
			case eGO_FORM:                 goal.pGoalOp = new COPForm              (static_cast<COPForm&>              (*pGoalOp)); break;
			case eGO_G4APPROACH:	         goal.pGoalOp = new COPG4Approach        (static_cast<COPG4Approach&>        (*pGoalOp)); break;
			case eGO_HIDE:                 goal.pGoalOp = new COPCrysis2Hide       (static_cast<COPCrysis2Hide&>       (*pGoalOp)); break;
			case eGO_IGNOREALL:            goal.pGoalOp = new COPIgnoreAll         (static_cast<COPIgnoreAll&>         (*pGoalOp)); break;
			case eGO_LOCATE:               goal.pGoalOp = new COPLocate            (static_cast<COPLocate&>            (*pGoalOp)); break;
			case eGO_LOOK:		             goal.pGoalOp = new COPLook              (static_cast<COPLook&>              (*pGoalOp)); break;
			case eGO_LOOKAROUND:           goal.pGoalOp = new COPLookAround        (static_cast<COPLookAround&>        (*pGoalOp)); break;
			case eGO_LOOKAT:               goal.pGoalOp = new COPLookAt            (static_cast<COPLookAt&>            (*pGoalOp)); break;
			case eGO_MOVETOWARDS:          goal.pGoalOp = new COPMoveTowards       (static_cast<COPMoveTowards&>       (*pGoalOp)); break;
			case eGO_PATHFIND:             goal.pGoalOp = new COPPathFind          (static_cast<COPPathFind&>          (*pGoalOp)); break;
			case eGO_PROXIMITY:            goal.pGoalOp = new COPProximity         (static_cast<COPProximity&>         (*pGoalOp)); break;
			case eGO_RUN:
				case eGO_SPEED:              goal.pGoalOp = new COPRunCmd            (static_cast<COPRunCmd&>            (*pGoalOp)); break;
			case eGO_SCRIPT:               goal.pGoalOp = new COPScript            (static_cast<COPScript&>            (*pGoalOp)); break;
			case eGO_SEEKCOVER:            goal.pGoalOp = new COPSeekCover         (static_cast<COPSeekCover&>         (*pGoalOp)); break;
			case eGO_SIGNAL:               goal.pGoalOp = new COPSignal            (static_cast<COPSignal&>            (*pGoalOp)); break;
			case eGO_STEER:                goal.pGoalOp = new COPSteer             (static_cast<COPSteer&>             (*pGoalOp)); break;
			case eGO_STICK:
				case eGO_STICKMINIMUMDISTANCE: goal.pGoalOp = new COPStick           (static_cast<COPStick&>             (*pGoalOp)); break;
			case eGO_STRAFE:               goal.pGoalOp = new COPStrafe            (static_cast<COPStrafe&>            (*pGoalOp)); break;
			case eGO_TACTICALPOS:          goal.pGoalOp = new COPTacticalPos       (static_cast<COPTacticalPos&>       (*pGoalOp)); break;
			case eGO_TIMEOUT:              goal.pGoalOp = new COPTimeout           (static_cast<COPTimeout&>           (*pGoalOp)); break;
			case eGO_TRACE:                goal.pGoalOp = new COPTrace             (static_cast<COPTrace&>             (*pGoalOp)); break;
			case eGO_USECOVER:             goal.pGoalOp = new COPCrysis2UseCover   (static_cast<COPCrysis2UseCover&>   (*pGoalOp)); break;
			case eGO_WAIT:                 goal.pGoalOp = new COPWait              (static_cast<COPWait&>              (*pGoalOp)); break;
			case eGO_WAITSIGNAL:           goal.pGoalOp = new COPWaitSignal        (static_cast<COPWaitSignal&>        (*pGoalOp)); break;
			
			default:                       assert(!!!"What?!!");
		}
	}
			
	goal.sPipeName = sPipeName; // The name of the possible pipe.
	
	//TODO evgeny Should go away when we have finally switch to XML
	goal.params = params;
	
	goal.bBlocking = bBlocking;
	goal.eGrouping = eGrouping;
	
	return goal;
}


void QGoal::Serialize(TSerialize ser, CObjectTracker& objectTracker)
{
	ser.EnumValue("op", op, eGO_FIRST, eGO_LAST);
	ser.Value("sPipeName", sPipeName);
	ser.Value("bBlocking", bBlocking);
	ser.EnumValue("eGrouping", eGrouping, IGoalPipe::eGT_NOGROUP, IGoalPipe::eGT_LAST);
	pGoalOp->Serialize(ser, objectTracker);
}


CGoalPipe::CGoalPipe(const string& sName, bool bDynamic)
: m_sName(sName),
  m_bDynamic(bDynamic),
m_nPosition( 0 ),
m_pSubPipe( 0 ), 
m_bLoop( true ),
m_nEventId( 0 ),
m_bHighPriority( false ),
m_bKeepOnTop( false ),
m_nCurrentBlockCounter(0)
{
	PREFAST_SUPPRESS_WARNING(6326) COMPILE_TIME_ASSERT(sizeof(g_GoalOpNames) / sizeof(g_GoalOpNames[0]) == eGO_LAST + 1);
}

CGoalPipe::~CGoalPipe()
{
	if (!m_qGoalPipe.empty())
	{
		m_qGoalPipe.clear();
	}

	SAFE_DELETE(m_pSubPipe);
}

CGoalPipe* CGoalPipe::Clone()
	{
	CGoalPipe* pClone = new CGoalPipe(m_sName, true);

	// copy the labels 
	if (!m_Labels.empty())
	{
		// convert labels to offsets before cloning
		for ( VectorOGoals::iterator gi = m_qGoalPipe.begin(); gi != m_qGoalPipe.end(); ++gi )
		{
			if ( !gi->params.str.empty() && (gi->op == eGO_BRANCH || gi->op == eGO_RANDOM) )
			{
				LabelsMap::iterator it = m_Labels.find( gi->params.str );
				if ( it != m_Labels.end() )
				{
					int absolute = it->second;
					int current = (int)(gi - m_qGoalPipe.begin());
					if ( current < absolute )
						gi->params.nValueAux = absolute - current - 1;
					else if ( current > absolute )
						gi->params.nValueAux = absolute - current;
					else
					{
						gi->params.nValueAux = 0;
						AIWarning("Empty loop at label %s ignored in goal pipe %s.", gi->params.str.c_str(), m_sName.c_str());
					}
				}
				else
				{
					gi->params.nValueAux = 0;
					AIWarning("Label %s not found in goal pipe %s.", gi->params.str.c_str(), m_sName.c_str());
				}
				gi->params.str.clear();
			}
		}

		// we don't need the labels anymore
		m_Labels.clear();
	}

	for (VectorOGoals::iterator gi = m_qGoalPipe.begin(); gi != m_qGoalPipe.end(); ++gi)
	{
		QGoal& goal = (*gi);
		if (goal.op != eGO_LAST)
			pClone->m_qGoalPipe.push_back(goal.Clone());
		else
			pClone->PushPipe(goal.sPipeName.c_str(), goal.bBlocking, goal.eGrouping, goal.params);
}

	return pClone;
}

void CGoalPipe::PushGoal(const XmlNodeRef& goalOpNode, EGroupType eGrouping)
{
	const char* szGoalOpName = goalOpNode->getTag();
	
	EGoalOperations op = CGoalPipe::GetGoalOpEnum(szGoalOpName);
	if (op == eGO_LAST)
	{
		AIError("Goal pipe '%s': Tag '%s' is not recognized as a goalop name; skipping.",
			GetName(), szGoalOpName);
		return;
	}

	CGoalOp* pGoalOp = CreateGoalOp(op, goalOpNode);
	if (pGoalOp)
	{
		QGoal newgoal;
		newgoal.pGoalOp = pGoalOp;
		newgoal.op = op;
		newgoal.sPipeName = CGoalPipe::GetGoalOpName(op); // Goals should know their own names soon
		newgoal.bBlocking = (stricmp(goalOpNode->getAttr("blocking"), "false") != 0);
		newgoal.eGrouping = eGrouping;

		m_qGoalPipe.push_back(newgoal);	
	}
}

CGoalOp* CGoalPipe::CreateGoalOp(EGoalOperations op, const XmlNodeRef& goalOpNode)
{
	switch (op)
	{
	case eGO_ACQUIRETARGET:        return new COPAcqTarget(goalOpNode);
	case eGO_ADJUSTAIM:            return new COPCrysis2AdjustAim(goalOpNode);
	case eGO_ANIMATION:            return new COPAnimation(goalOpNode);
	case eGO_ANIMTARGET:           return new COPAnimTarget(goalOpNode);
	case eGO_APPROACH:             return new COPApproach(goalOpNode);
	case eGO_BACKOFF:              return new COPBackoff(goalOpNode);
	case eGO_BODYPOS:
		case eGO_STANCE:             return new COPBodyCmd(goalOpNode);
	case eGO_BRANCH:               return (gAIEnv.configuration.eCompatibilityMode == ECCM_CRYSIS2)
		? static_cast<CGoalOp*>(new COPDummy)
		: static_cast<CGoalOp*>(new COPDeValue);
	case eGO_CLEAR:                return new COPClear;
	case eGO_COMMUNICATE:          return new COPCrysis2Communicate(goalOpNode);
	case eGO_COMPANIONSTICK:       return new COPCompanionStick(goalOpNode);
	case eGO_CONTINUOUS:           return new COPContinuous(goalOpNode);
	case eGO_DEVALUE:              return new COPDeValue(goalOpNode);
	case eGO_DODGE:                return new COPDodge(goalOpNode);
	case eGO_FIRECMD:              return new COPFireCmd(goalOpNode);
	case eGO_FOLLOWPATH:           return new COPFollowPath(goalOpNode);
	case eGO_FORM:                 return new COPForm(goalOpNode);
	case eGO_G4APPROACH:	         return new COPG4Approach(goalOpNode);
	case eGO_HIDE:                 return new COPCrysis2Hide(goalOpNode);
	case eGO_IGNOREALL:            return new COPIgnoreAll(goalOpNode);
	case eGO_LOCATE:               return new COPLocate(goalOpNode);
	case eGO_LOOK:		             return new COPLook(goalOpNode);
	case eGO_LOOKAROUND:           return new COPLookAround(goalOpNode);
	case eGO_LOOKAT:               return new COPLookAt(goalOpNode);
	case eGO_MOVETOWARDS:          return new COPMoveTowards(goalOpNode);
	case eGO_PATHFIND:             return new COPPathFind(goalOpNode);
	case eGO_PROXIMITY:            return new COPProximity(goalOpNode);
	case eGO_RUN:
		case eGO_SPEED:              return new COPRunCmd(goalOpNode);
	case eGO_SCRIPT:               return new COPScript(goalOpNode);
	case eGO_SEEKCOVER:            return new COPSeekCover(goalOpNode);
	case eGO_SIGNAL:               return new COPSignal(goalOpNode);
	case eGO_STEER:                return new COPSteer(goalOpNode);
	case eGO_STICK:                return new COPStick(goalOpNode);
	case eGO_STICKMINIMUMDISTANCE: return new COPStick(goalOpNode);
	case eGO_STRAFE:               return new COPStrafe(goalOpNode);
	case eGO_TACTICALPOS:          return new COPTacticalPos(goalOpNode);
	case eGO_TIMEOUT:              return new COPTimeout(goalOpNode);
	case eGO_TRACE:                return new COPTrace(goalOpNode);
	case eGO_USECOVER:             return new COPCrysis2UseCover(goalOpNode);
	case eGO_WAIT:                 return new COPWait(goalOpNode);
	case eGO_WAITSIGNAL:           return new COPWaitSignal(goalOpNode);

	default:
		AIError("Unrecognized goalop code: %d.", (int)(op));
		assert(!!!"Unrecognized goalop code.");
		return 0;
	}
}

void CGoalPipe::SetDebugName(const char* name)
{
	m_sDebugName = name;
}

void CGoalPipe::PushLabel(const char* label)
{
	LabelsMap::iterator it = m_Labels.find(CONST_TEMP_STRING( label ));
	if (it != m_Labels.end())
		AIWarning("Label %s already exists in goal pipe %s. Overriding previous label!", label, m_sName.c_str());

	m_Labels[label] = m_qGoalPipe.size();
}

EGoalOperations CGoalPipe::GetGoalOpEnum(const char* szName)
{
	for (SAIGoalOpName* opName = g_GoalOpNames; opName->op != eGO_LAST; ++opName)
	{
		// [2/10/2010 evgeny] Changed "strcmp" to "stricmp" to accommodate names like "SeekCoverPos"
		if (!stricmp(opName->szName, szName))	// Compare the op names
		{
			return opName->op;
	}
	}

	return eGO_LAST;
}

const char* CGoalPipe::GetGoalOpName(EGoalOperations op)
{
	return (op < eGO_LAST) ? g_GoalOpNames[op].szName : 0;
}

void CGoalPipe::PushGoal(EGoalOperations op, bool bBlocking, EGroupType eGrouping, GoalParameters &params)
{
	// Note that very few of the individual goalops manipulate the blocking or grouping params
	QGoal newgoal;

	if (op != eGO_WAIT)
	{
		if (eGrouping == IGoalPipe::eGT_GROUPED)
		{
		++m_nCurrentBlockCounter;
		}
		else
		{
		m_nCurrentBlockCounter = 0;	// continuous group counter.
		}
	}

	// Try the factories first
	newgoal.pGoalOp = static_cast<CGoalOp*>(gAIEnv.pGoalOpFactory->GetGoalOp( op, params ));
	if (newgoal.pGoalOp)
	{
		// Could adjust the ops below to avoid this redundancy
		newgoal.op = op;
		newgoal.params = params;
		newgoal.bBlocking = bBlocking;
		newgoal.eGrouping = eGrouping;
		m_qGoalPipe.push_back(newgoal);
		return;
	}

	switch(op)
	{
	case eGO_ACQUIRETARGET:
		{
			newgoal.pGoalOp = new COPAcqTarget(params.str);
		}
		break;
	case eGO_ADJUSTAIM:
		{
			newgoal.pGoalOp = new COPAdjustAim((params.nValue & 0x01) != 0, (params.nValue & 0x02) != 0, (params.nValue & 0x04) != 0);
		}
		break;
	case eGO_APPROACH:
		{
			float duration, endDistance;
			if (params.nValue & AI_USE_TIME)
			{
				duration = fabsf(params.fValue);
				endDistance = 0.0f;
			}
			else
			{
				duration = 0.0f;
				endDistance = params.fValue;
			}

			// Random variation on the end distance.
			if (params.vPos.x > 0.01f)
			{
				float u = (ai_rand() % 10) / 9.0f;
				if (endDistance > 0.0f)
					endDistance = max(0.0f, endDistance - u * params.vPos.x);
				else
					endDistance = min(0.0f, endDistance + u * params.vPos.x);
			}

			newgoal.pGoalOp = new COPApproach(endDistance, params.fValueAux, duration,
				(params.nValue & AILASTOPRES_USE) != 0, (params.nValue & AILASTOPRES_LOOKAT) != 0,
				(params.nValue & AI_REQUEST_PARTIAL_PATH)!=0, (params.nValue & AI_STOP_ON_ANIMATION_START)!=0, params.str);
		}
		break;
	case eGO_G4APPROACH:
		assert(false);
		break; 
	case eGO_STEER:
		{
			newgoal.pGoalOp = new COPSteer(params.fValue, params.fValueAux);
		}
		break; 
	case eGO_FOLLOWPATH:
		{
			newgoal.pGoalOp = new COPFollowPath((params.nValue & 1) != 0, (params.nValue & 2) != 0, (params.nValue & 4) != 0, params.nValueAux, params.fValueAux, (params.nValue & 8) != 0);
		}
		break;
	case eGO_BACKOFF:
		{
			newgoal.pGoalOp = new COPBackoff(params.fValue, params.fValueAux, params.nValue);
		}
		break;
	case eGO_MOVETOWARDS:
		{
			float duration, endDistance;
			if (params.nValue & AI_USE_TIME)
			{
				duration = fabsf(params.fValue);
				endDistance = 0.0f;
			}
			else
			{
				duration = 0.0f;
				endDistance = params.fValue;
			}

			// Random variation on the end distance.
			if (params.vPos.x > 0.01f)
			{
				float u = (ai_rand() % 10) / 9.0f;
				if (endDistance > 0.0f)
					endDistance = max(0.0f, endDistance - u * params.vPos.x);
				else
					endDistance = min(0.0f, endDistance + u * params.vPos.x);
			}

/*			newgoal.pGoalOp = new COPApproach(endDistance, params.fValueAux, duration, (params.nValue & AILASTOPRES_USE) != 0, 
				(params.nValue & AILASTOPRES_LOOKAT) != 0, (params.nValue & AI_REQUEST_PARTIAL_PATH)!=0,
				(params.nValue & AI_STOP_ON_ANIMATION_START)!=0, params.szString);*/

			newgoal.pGoalOp = new COPMoveTowards(endDistance, duration); //, params.nValue);
		}
		break;
	case eGO_FIRECMD:
		{
			newgoal.pGoalOp = new COPFireCmd(params.nValue, params.bValue, params.fValue, params.fValueAux);
		}
		break;
	case eGO_STANCE:
	case eGO_BODYPOS:
		{
			newgoal.pGoalOp = new COPBodyCmd(static_cast<EStance>(params.nValue), params.bValue);
		}
		break;
	case eGO_STRAFE:
		{
			 newgoal.pGoalOp = new COPStrafe(params.fValue, params.fValueAux, params.bValue);
		}
		break;
	case eGO_TIMEOUT:
		{	
			if (!gEnv->pSystem)
				AIError("CGoalPipe::PushGoal Pushing goals without a valid System instance [Code bug]");

			newgoal.pGoalOp = new COPTimeout(params.fValue, params.fValueAux);
		}
		break;
	case eGO_SPEED:
	case eGO_RUN:
		{
			newgoal.pGoalOp = new COPRunCmd(params.fValue, params.fValueAux, params.vPos.x);
		}
		break;
	case eGO_LOOKAROUND:
		{
			newgoal.pGoalOp = new COPLookAround(params.fValue, params.fValueAux, params.vPos.x, params.vPos.y, params.nValueAux!=0, (params.nValue & AI_BREAK_ON_LIVE_TARGET)!=0,(params.nValue & AILASTOPRES_USE)!=0, params.bValue);
		}
		break;
	case eGO_LOCATE:
		{
			newgoal.pGoalOp = new COPLocate(params.str.c_str(), params.nValue, params.fValue);
		}
		break;
	case eGO_PATHFIND:
		{
			newgoal.pGoalOp = new COPPathFind(params.str.c_str(),params.pTarget, 0.0f, 0.0f, params.bValue, (params.nValue & AI_REQUEST_PARTIAL_PATH)!=0);
		}
		break;
	case eGO_TRACE:
		{
			bool bParam = (params.nValue > 0);
			newgoal.pGoalOp = new COPTrace(bParam, params.fValue, params.nValueAux > 0);
		}
		break;
	case eGO_IGNOREALL:
		{
			newgoal.pGoalOp = new COPIgnoreAll(params.bValue);
		}
		break;
	case eGO_SIGNAL:
		{
			newgoal.pGoalOp = new COPSignal(params.nValueAux, params.str, static_cast<ESignalFilter>(params.nValue), (int)params.fValueAux);
		}
		break;
	case eGO_SCRIPT:
		{
			newgoal.pGoalOp = new COPScript(params.scriptFunction, params.scriptUserData);
		}
		break;
	case eGO_DEVALUE:
		{
			newgoal.pGoalOp = new COPDeValue((int)params.fValue,params.bValue);
		}
		break;
	case eGO_HIDE:
		{
			newgoal.pGoalOp = new COPHide(params.fValue, params.fValueAux, params.nValue,params.bValue,
				(params.nValueAux & AILASTOPRES_LOOKAT) != 0);
		}
		break;
	case eGO_STICKMINIMUMDISTANCE:
	case eGO_STICK:
		{
			float duration, endDistance;
			if (params.nValue & AI_USE_TIME)
			{
				duration = fabsf(params.fValue);
				endDistance = 0.0f;
			}
			else
			{
				duration = 0.0f;
				endDistance = params.fValue;
			}
			
			// Random variation on the end distance.
			if (params.vPos.x > 0.01f)
			{
				float u = (ai_rand() % 10) / 9.0f;
				if (endDistance > 0.0f)
					endDistance = max(0.0f, endDistance - u * params.vPos.x);
				else
					endDistance = min(0.0f, endDistance + u * params.vPos.x);
			}

			const COPTrace::EEndMode eEndMode = (params.bValue) ? COPTrace::eEM_FixedDistance : COPTrace::eEM_MinimumDistance;

			// Note: the flag 0x01 means break in a goalpipe description but we change it to continuous flag inside stick goalop.
			newgoal.pGoalOp = new COPStick(endDistance, params.fValueAux, duration,params.nValue,params.nValueAux, eEndMode);
/*			(params.nValue & AILASTOPRES_USE) != 0, (params.nValue & AILASTOPRES_LOOKAT) != 0,
				(params.nValueAux & 0x01) == 0, (params.nValueAux & 0x02) != 0, (params.nValue & AI_REQUEST_PARTIAL_PATH)!=0,
				(params.nValue & AI_STOP_ON_ANIMATION_START)!=0, (params.nValue & AI_CONSTANT_SPEED)!=0);*/

		}
		break;
	case eGO_TACTICALPOS:
	  {
		  // Create Tactical Position goalop based on a given query ID
		  newgoal.pGoalOp = new COPTacticalPos(params.nValue, static_cast<EAIRegister>(params.nValueAux));
	  }
    break;
	case eGO_LOOK:
		{
			// Create Look goalop
			newgoal.pGoalOp = new COPLook(params.nValue, params.bValue, params.nValueAux);
		}
		break;
	case eGO_FORM:
		{
			newgoal.pGoalOp = new COPForm(params.str.c_str());
		}
		break;
	case eGO_CLEAR:
		{
			if (gAIEnv.configuration.eCompatibilityMode == ECCM_CRYSIS2)
			{
				// Executing COPDeValue will remove the attention target. We don't want this for Crysis 2.
				// Instead we add a dummy goalop that does nothing except saying it's done. /Jonas
				newgoal.pGoalOp = new COPDummy();
			}
			else
			{
				newgoal.pGoalOp = new COPDeValue();
			}
		}
		break;
	case eGO_BRANCH:
		{
			if (gAIEnv.configuration.eCompatibilityMode == ECCM_CRYSIS2)
			{
				// Executing COPDeValue will remove the attention target. We don't want this for Crysis 2.
				// Instead we add a dummy goalop that does nothing except saying it's done. /Jonas
				newgoal.pGoalOp = new COPDummy();
			}
			else
			{
				newgoal.pGoalOp = new COPDeValue();
			}

			bBlocking = false;
		}
		break;
	case eGO_RANDOM:
		{
			newgoal.pGoalOp = new COPDeValue();
			bBlocking = false;
		}
		break;
	case eGO_LOOKAT:
		{
			newgoal.pGoalOp = new COPLookAt(params.fValue, params.fValueAux, params.nValue, params.nValueAux!=0, params.bValue);
		}
		break;
	case eGO_CONTINUOUS:
		{
			newgoal.pGoalOp = new COPContinuous( params.bValue );
		}
		break;
	case eGO_ANIMATION:
		{
			newgoal.pGoalOp = new COPAnimation( (EAnimationMode) params.nValue, params.str );
		}
		break;
	case eGO_ANIMTARGET:
		{
			newgoal.pGoalOp = new COPAnimTarget( (params.nValue == AIANIM_SIGNAL), params.str, params.vPos.x, params.vPos.y, params.vPos.z, params.vPosAux, params.bValue );
		}
		break;
	case eGO_USECOVER:
		{
			newgoal.pGoalOp = new COPUseCover( params.bValue, params.nValue != 0, params.fValue, params.fValueAux, params.nValueAux != 0 );
		}
		break;
	case eGO_WAITSIGNAL:
		{
			// (MATT) Note that all but form 0 appear to be possible to remove {2008/08/09}
			switch ( int(params.fValue) )
			{
			case 0:
				newgoal.pGoalOp = new COPWaitSignal( params.str, params.fValueAux );
				break;
			case 1:
				newgoal.pGoalOp = new COPWaitSignal( params.str, params.strAux, params.fValueAux );
				break;
			case 2:
				newgoal.pGoalOp = new COPWaitSignal( params.str, params.nValue, params.fValueAux );
				break;
			case 3:
				newgoal.pGoalOp = new COPWaitSignal( params.str, EntityId(params.nValue), params.fValueAux );
				break;
			default:
				return;
			}
		}
		break;
	case eGO_WAIT:
		{
			newgoal.pGoalOp = new COPWait(params.nValue, m_nCurrentBlockCounter);
			m_nCurrentBlockCounter = 0;
			bBlocking = true;	// wait always has to be blocking
			eGrouping = IGoalPipe::eGT_GROUPED; // wait always needs to be grouped
		}
		break;
	case eGO_SEEKCOVER:
		assert(false);
		break;
	case eGO_PROXIMITY:
		{
			newgoal.pGoalOp = new COPProximity(params.fValue, params.str, (params.nValue & 0x1) != 0, (params.nValue & 0x2) != 0);
		}
		break;
	case eGO_DODGE:
		{
			newgoal.pGoalOp = new COPDodge(params.fValue, params.bValue);
		}
		break;
	case eGO_COMPANIONSTICK:
		{
			newgoal.pGoalOp = new COPCompanionStick(params.fValue, params.fValueAux, params.vPos.x);
		}
		break;
	default:
		// Trying to push undefined/unimplemented goalop.
		AIAssert(0);
	}

	newgoal.op = op;
	newgoal.params = params;
	newgoal.bBlocking = bBlocking;
	newgoal.eGrouping = eGrouping;

	m_qGoalPipe.push_back(newgoal);
}

//
//-------------------------------------------------------------------------------------------------------
void CGoalPipe::PushPipe(const char* szName, bool bBlocking, EGroupType eGrouping, GoalParameters &params)
{
	if (eGrouping == IGoalPipe::eGT_GROUPED)
		++m_nCurrentBlockCounter;
	else 
		m_nCurrentBlockCounter = 0;	// continuous group counter.

	QGoal newgoal;
	newgoal.op = eGO_LAST;
	newgoal.sPipeName = szName;
	newgoal.params = params;
	newgoal.bBlocking = bBlocking;
	newgoal.eGrouping = eGrouping;

	m_qGoalPipe.push_back(newgoal);
}

//
//-------------------------------------------------------------------------------------------------------
void CGoalPipe::PushGoal(IGoalOp* pGoalOp, EGoalOperations op, bool bBlocking, EGroupType eGrouping, const GoalParameters &params)
{
	// (MATT) For now we fetch a bunch of the params back from the GoalOp, because eventually they will disappear {2008/02/22}

	// Dropped the checks for wait op - it shouldn't end up here anyway
	if (eGrouping == IGoalPipe::eGT_GROUPED)
		++m_nCurrentBlockCounter;

	QGoal newgoal;
	newgoal.op = op;
	newgoal.pGoalOp = static_cast<CGoalOp*>(pGoalOp);
	newgoal.sPipeName = CGoalPipe::GetGoalOpName(op); // Goals should know their own names soon
	newgoal.params = params;
	newgoal.bBlocking = bBlocking;
	newgoal.eGrouping = eGrouping;

	m_qGoalPipe.push_back(newgoal);
}



//
//-------------------------------------------------------------------------------------------------------
EPopGoalResult CGoalPipe::PopGoal(QGoal& theGoal, CPipeUser *pOperand)
{
	// if we are processing a subpipe
	if (m_pSubPipe)
	{
		EPopGoalResult result = m_pSubPipe->PopGoal(theGoal, pOperand);
		if ( result == ePGR_AtEnd )
		{
			CCCPOINT(CGoalPipe_PopGoal_A);

			bool dontProcessParentPipe = m_pSubPipe->m_nEventId != 0;

			// this subpipe is finished
			SAFE_DELETE(m_pSubPipe); 

			if (!m_refArgument.IsNil())
				pOperand->SetLastOpResult(m_refArgument);

			pOperand->NotifyListeners( this, ePN_Resumed );
			if (dontProcessParentPipe)
				return ePGR_BreakLoop;
		}
		else
			return result;
	}

	CCCPOINT(CGoalPipe_PopGoal_B);

	if (m_nPosition < m_qGoalPipe.size() )
	{
		QGoal& current = m_qGoalPipe[m_nPosition++];
		if (current.op == eGO_LAST)
		{
			if (CGoalPipe* pPipe = gAIEnv.pPipeManager->IsGoalPipe(current.sPipeName))
			{
				CCCPOINT(CGoalPipe_PopGoal_C);

				// this goal is a subpipe of goals, get that one until it is finished
				m_pSubPipe = pPipe;
				if(pOperand->GetAttentionTarget())
					pPipe->m_vAttTargetPosAtStart = pOperand->GetAttentionTarget()->GetPos();
				else
					pPipe->m_vAttTargetPosAtStart.zero();
				return m_pSubPipe->PopGoal(theGoal, pOperand);
			}
		}
		else
		{
			CCCPOINT(CGoalPipe_PopGoal_D);

			// this is an atomic goal, just return it
			theGoal = current;
			// goal successfully retrieved
			if (0 != theGoal.pGoalOp)
				return ePGR_Succeed;
			else
			{
				// MERGE :  (MATT) We should hammer this down in Crysis and make this a full error {2008/01/07:12:38:43}
				AIWarning("CGoalPipe::PushGoal - Attempting to push goalpipe that does not exist: \"%s\"", current.sPipeName.c_str());
				return ePGR_AtEnd;
			}
		}
	}


	CCCPOINT(CGoalPipe_PopGoal_E);

	// we have reached the end of this goal pipe
	// reset position and let the world know we are done
	pOperand->NotifyListeners( this, ePN_Finished );
	Reset();
	return ePGR_AtEnd;
}


void CGoalPipe::Reset()
{
	m_nPosition = 0;
	SAFE_DELETE(m_pSubPipe);
	m_lastResult = eGOR_NONE;
}

// Makes the IP of this pipe jump to the desired position
bool CGoalPipe::Jump(int position)
{
	// if we are processing a subpipe
	if (m_pSubPipe)
	{
		return m_pSubPipe->Jump( position );
	}

	if (position<0)
		--position;
		
	if (m_nPosition)
		m_nPosition+=position;

	return position<0;
}

// obsolete - string labels are converted to integer relative offsets
// TODO: cut this version of Jump in a few weeks
bool CGoalPipe::Jump(const char* label)
{
	// if we are processing a subpipe
	if (m_pSubPipe)
	{
		return m_pSubPipe->Jump( label );
		//return;
	}
	int step = 0;
	LabelsMap::iterator it = m_Labels.find(CONST_TEMP_STRING( label ));
	if (it != m_Labels.end())
	{
		step = it->second - m_nPosition;
		m_nPosition = it->second;
	}
	else
		AIWarning("Label %s not found in goal pipe %s.", label, m_sName.c_str());

	return step<0;
}

void CGoalPipe::ReExecuteGroup()
{
	// this must be the last inserted goal pipe
	AIAssert(!m_pSubPipe);
/* this is how it was before......
	while (	m_nPosition && m_qGoalPipe[--m_nPosition].eGrouping==IGoalPipe::GT_GROUPWITHPREV)
		; // nothing to do, just keep decreasing m_nPosition until we found the beginning of this group
*/

	if(m_nPosition==0)	// if already in the beginning of the pipe - no need to move the position
		return;
	if(m_qGoalPipe[m_nPosition-1].eGrouping==IGoalPipe::eGT_GROUPED)
	{
		if(m_nPosition)
			--m_nPosition;
		while (	m_nPosition 
			&& m_qGoalPipe[m_nPosition].eGrouping==IGoalPipe::eGT_GROUPED
			&& m_qGoalPipe[m_nPosition-1].eGrouping==IGoalPipe::eGT_GROUPED)
			--m_nPosition; 
	}
	else
	{
		while (	m_nPosition && m_qGoalPipe[--m_nPosition].eGrouping==IGoalPipe::eGT_GROUPWITHPREV)
			; // nothing to do, just keep decreasing m_nPosition until we found the beginning of this group
	}
}

void CGoalPipe::SetSubpipe(CGoalPipe * pPipe)
{
	if ( m_pSubPipe && pPipe )
		pPipe->SetSubpipe( m_pSubPipe );
	m_pSubPipe = pPipe;
}

//
//------------------------------------------------------------------------------------------------------
bool CGoalPipe::RemoveSubpipe( int& goalPipeId, bool keepInserted, bool keepHigherPriority )
{
	if ( m_bHighPriority )
		keepHigherPriority = false;
	if ( !m_pSubPipe )
		return false;
	if ( !goalPipeId && !m_pSubPipe->m_pSubPipe )
		goalPipeId = m_pSubPipe->m_nEventId;
	if ( m_pSubPipe->m_nEventId == goalPipeId )
	{
		if ( keepInserted )
		{
			CGoalPipe* temp = m_pSubPipe;
			m_pSubPipe = m_pSubPipe->m_pSubPipe;
			temp->m_pSubPipe = NULL;
			delete temp;
		}
		else
		{
			if ( keepHigherPriority )
			{
				CGoalPipe* temp = m_pSubPipe;
				while ( temp && (!temp->m_pSubPipe || !temp->m_pSubPipe->m_bHighPriority) )
					temp = temp->m_pSubPipe;
				if ( temp )
				{
					CGoalPipe* subPipe = m_pSubPipe;
					m_pSubPipe = temp->m_pSubPipe;
					temp->m_pSubPipe = NULL;
					delete subPipe;
					return true;
				}
			}
			// there's no higher priority pipe
			delete m_pSubPipe;
			m_pSubPipe = NULL;
		}
		return true;
	}
	else
		return m_pSubPipe->RemoveSubpipe( goalPipeId, keepInserted, keepHigherPriority );
}

void CGoalPipe::Serialize(TSerialize ser, class CObjectTracker& objectTracker, VectorOGoals &activeGoals)
{
	ser.Value( "GoalPipePos", m_nPosition);
	ser.Value( "nEventId", m_nEventId);
	ser.Value( "bHighPriority", m_bHighPriority);
	ser.EnumValue("m_lastResult",m_lastResult,eGOR_NONE,eGOR_LAST);

	m_refArgument.Serialize(ser, "m_refArgument");

	// take care of active goals
	ser.BeginGroup("ActiveGoals");
	int counter = 0;
	int goalIdx;
	char groupName[256];
	if(ser.IsWriting())
	{
		for(uint32 activeIdx=0; activeIdx<activeGoals.size(); ++activeIdx)
		{
			VectorOGoals::iterator actGoal = std::find(m_qGoalPipe.begin(),m_qGoalPipe.end(),activeGoals[activeIdx]);
			if(actGoal!=m_qGoalPipe.end())
			{
				sprintf(groupName, "ActiveGoal-%d", counter);
				ser.BeginGroup(groupName);
				goalIdx = (int)(actGoal - m_qGoalPipe.begin());
				ser.Value("goalIdx", goalIdx);
				m_qGoalPipe[goalIdx].Serialize(ser, objectTracker);
				ser.EndGroup();
				++counter;
			}
		}
		ser.Value("activeGoalGounter", counter);
	}
	else
	{
		ser.Value("activeGoalGounter", counter);
		activeGoals.resize(counter);
		for (int i=0; i<counter; ++i)
		{
			sprintf(groupName, "ActiveGoal-%d", i);
			ser.BeginGroup(groupName);
			ser.Value("goalIdx", goalIdx);
			activeGoals[i] = m_qGoalPipe[goalIdx];
			m_qGoalPipe[goalIdx].Serialize(ser, objectTracker);
			ser.EndGroup();
		}
	}
	ser.EndGroup();

	if(ser.IsReading())
	{
    AIAssert(!IsInSubpipe()); // Danny not completely sure this is a correct assertion to make
		if(ser.BeginOptionalGroup( "SubPipe", true ))
		{
			string subPipeName;
			ser.Value( "GoalSubPipe", subPipeName);
			CGoalPipe* pSubPipe = new CGoalPipe(subPipeName, true);
			ser.BeginGroup( "DynamicPipe" );
				pSubPipe->SerializeDynamic( ser );
			ser.EndGroup();
			SetSubpipe(pSubPipe);
			GetSubpipe()->Serialize(ser, objectTracker, activeGoals);
			ser.EndGroup();
		}
	}
	else
	{
		if (ser.BeginOptionalGroup( "SubPipe", IsInSubpipe() ))
		{
			ser.Value( "GoalSubPipe", GetSubpipe()->m_sName );
			ser.BeginGroup( "DynamicPipe" );
				GetSubpipe()->SerializeDynamic( ser );
			ser.EndGroup();
			GetSubpipe()->Serialize(ser, objectTracker, activeGoals);
			ser.EndGroup();
		}
	}
}


//
//------------------------------------------------------------------------------------------------------
void CGoalPipe::SerializeDynamic(TSerialize ser)
{
	if(!m_bDynamic)
		return;
	uint32	count(m_qGoalPipe.size());
	char	buffer[16];
	ser.Value("count", count);
	for (uint32 curIdx(0); curIdx<count; ++curIdx)
	{
		sprintf(buffer, "goal_%d", curIdx);
		ser.BeginGroup(buffer);
		{
			QGoal gl;
			if(ser.IsWriting())
				gl = m_qGoalPipe[curIdx];
			ser.EnumValue("op", gl.op, eGO_FIRST, eGO_LAST);
			ser.Value("sPipeName" , gl.sPipeName);
			ser.Value("bBlocking" , gl.bBlocking);
			ser.EnumValue("Grouping",gl.eGrouping,IGoalPipe::eGT_NOGROUP,IGoalPipe::eGT_LAST);
			gl.params.Serialize(ser);
			if(ser.IsReading())
			{
				if (gl.op != eGO_LAST)
					PushGoal(gl.op,gl.bBlocking,gl.eGrouping,gl.params);
				else
					PushPipe(gl.sPipeName, gl.bBlocking, gl.eGrouping, gl.params);
			}
		}
		ser.EndGroup();
	}
	ser.Value("Labels",m_Labels);
}

void GoalParameters::Serialize(TSerialize ser)
{
	ser.Value("vPos",      vPos);
	ser.Value("vPosAux",   vPosAux);

	ser.Value("fValue", fValue);
	ser.Value("fValueAux", fValueAux);
	
	ser.Value("nValue", nValue);
	ser.Value("nValueAux", nValueAux);
	
	ser.Value("bValue", bValue);
	
	ser.Value("str",       str);
	ser.Value("strAux",    strAux);
}

#include UNIQUE_VIRTUAL_WRAPPER(IGoalPipe)
