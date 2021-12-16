/********************************************************************
	Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2005.
 -------------------------------------------------------------------------
  File name:   FlowNodeAIAction.h
	$Id$
  Description: place for miscellaneous AI related flow graph nodes
  
 -------------------------------------------------------------------------
  History:
  - 15:6:2005   15:24 : Created by Kirill Bulatsev

*********************************************************************/

#ifndef __FlowNodeAIAction_H__
#define __FlowNodeAIAction_H__
#pragma once

#include "FlowBaseNode.h"
#include "IAnimationGraph.h"


//////////////////////////////////////////////////////////////////////////
// base AI Flow node 
//////////////////////////////////////////////////////////////////////////
template < bool TBlocking >
class CFlowNode_AIBase : public CFlowBaseNode, public IEntityEventListener, public IGoalPipeListener
{
public:
	static const bool m_bBlocking = TBlocking;

	CFlowNode_AIBase( SActivationInfo* pActInfo );
	virtual ~CFlowNode_AIBase();

	// Derived classes must re-implement this method!!!
	virtual void GetConfiguration( SFlowNodeConfig& config ) = 0;
	
	// Derived classes may re-implement this method!!!
	// Default implementation creates new instance for each cloned node
	// must be replicated to avoid derived template argument for this class
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo ) = 0;
	virtual void Serialize(SActivationInfo *, TSerialize ser);
	// Derived classes normally shouldn't override this method!!!
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo* pActInfo );
	// Derived classes must implement this method!!!
	virtual void DoProcessEvent( EFlowEvent event, SActivationInfo* pActInfo ) = 0;
	// Derived classes may override this method to be notified when the action was restored
	virtual void OnResume( SActivationInfo* pActInfo = NULL ) {}
	// Derived classes may override this method to be notified when the action was canceled
	virtual void OnCancel() {}
	virtual void OnFinish() {};
	// Derived classes may override this method to be updated each frame
	virtual bool OnUpdate( SActivationInfo* pActInfo ) { return false; }

	//////////////////////////////////////////////////////////////////////////
	// IGoalPipeListener
	virtual void OnGoalPipeEvent( IPipeUser* pPipeUser, EGoalPipeEvent event, int goalPipeId );

	//////////////////////////////////////////////////////////////////////////
	// IEntityEventListener
	virtual void OnEntityEvent( IEntity* pEntity, SEntityEvent& event );

	void RegisterEvents();
	virtual void UnregisterEvents();

	IEntity* GetEntity( SActivationInfo* pActInfo );

	bool Execute( SActivationInfo* pActInfo, const char* pSignalText, IAISignalExtraData* pData = NULL, int senderId = 0 );

protected:
	virtual void OnCancelPortActivated( IPipeUser* pPipeUser, SActivationInfo* pActInfo );

	virtual void Cancel();
	virtual void Finish();

	//TODO: should not store this - must be read from inPort
	IFlowGraph* m_pGraph;
	TFlowNodeId m_nodeID;

	int m_GoalPipeId;
	int m_UnregisterGoalPipeId;
	EntityId m_EntityId;
	EntityId m_UnregisterEntityId;
	bool m_bExecuted;
	bool m_bSynchronized;
	bool m_bNeedsExec;
	bool m_bNeedsSink;
	bool m_bNeedsReset;

	// you might want to override this method
	virtual bool ExecuteOnAI( SActivationInfo* pActInfo, const char* pSignalText,
		IAISignalExtraData* pData, IEntity* pEntity, IEntity* pSender );

	// you might want to override this method
	virtual bool ExecuteOnEntity( SActivationInfo* pActInfo, const char* pSignalText,
		IAISignalExtraData* pData, IEntity* pEntity, IEntity* pSender );

	// Utility function to set an AI's speed. Used by CFlowNode_AISpeed and CFlowNode_AIGotoSpeedStance
	void SetSpeed(IAIObject* pAI, int iSpeed);

	// Utility function to set an AI's stance. Used by CFlowNode_AIStance and CFlowNode_AIGotoSpeedStance
	void SetStance(IAIObject* pAI, int stance);

	// Utility function to set an AI's a. Used by CFlowNode_AIStance and CFlowNode_AIGotoSpeedStance
	void SetAllowStrafing(IAIObject* pAI,bool bAllowStrafing);

	// should call DoProcessEvent if owner is not too much alerted
	virtual void TryExecute( IAIObject* pAI, IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo );

	// this will be called when the node is activated, one update before calling ExecuteIfNotTooMuchAlerted
	// use this if there's some data that needs to be initialized before execution
	virtual void PreExecute( SActivationInfo* pActInfo ) {}
};



//---------------------------------------------------------------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////
// base AI forceable
//////////////////////////////////////////////////////////////////////////
template < bool TBlocking >
class CFlowNode_AIForceableBase
	: public CFlowNode_AIBase< TBlocking >
{
	typedef CFlowNode_AIBase< TBlocking > TBase;
public:
	CFlowNode_AIForceableBase( IFlowNode::SActivationInfo* pActInfo)
		: CFlowNode_AIBase< TBlocking >( pActInfo )
		, m_LastForceMethod(eNoForce) {}

	virtual void ProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo );

protected:
	enum EForceMethod
	{
		eNoForce = 0,
		eKeepPerception,
		eIgnoreAll,
	};

	virtual void OnCancel();
	virtual void OnFinish();
	virtual void TryExecute( IAIObject* pAI, IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo );
	virtual EForceMethod GetForceMethod( IFlowNode::SActivationInfo* pActInfo ) const
	{
		assert(!"Must implement for derived classes!");
		return eNoForce;
	}
	virtual void SetForceMethod( IAIObject* pAI, EForceMethod method );

	EForceMethod m_LastForceMethod;
};



//---------------------------------------------------------------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////
// base AI Signal order
//////////////////////////////////////////////////////////////////////////
template < class TDerivedFromSignalBase >
class CFlowNode_AISignalBase
	: public CFlowNode_AIBase< false >
{
	typedef CFlowNode_AIBase< false > TBase;
public:
	CFlowNode_AISignalBase( IFlowNode::SActivationInfo* pActInfo)
		: CFlowNode_AIBase< false >( pActInfo ){}

	virtual void GetConfiguration( SFlowNodeConfig& config );
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo *pActInfo );
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );

protected:
	virtual void Cancel();
	virtual void Finish();

	virtual void TryExecute( IAIObject* pAI, IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo );
	virtual bool ShouldForce( IFlowNode::SActivationInfo* pActInfo ) const;

	const char*	m_SignalText;

	virtual IAISignalExtraData* GetExtraData( IFlowNode::SActivationInfo *pActInfo ) const { return NULL; }
};

//---------------------------------------------------------------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////
// base AI Signal order
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AISignalAlerted
	: public CFlowNode_AISignalBase< CFlowNode_AISignalAlerted >
{
public:
	CFlowNode_AISignalAlerted( IFlowNode::SActivationInfo* pActInfo)
		: CFlowNode_AISignalBase< CFlowNode_AISignalAlerted >(pActInfo) { m_SignalText = "ACT_ALERTED"; }

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};



//////////////////////////////////////////////////////////////////////////
// prototyping AI orders
//////////////////////////////////////////////////////////////////////////
//---------------------------------------------------------------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////
// generic AI order
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AISignal
	: public CFlowNode_AISignalBase< CFlowNode_AISignal >
{
	typedef CFlowNode_AISignalBase< CFlowNode_AISignal > TBase;
public:
	CFlowNode_AISignal( IFlowNode::SActivationInfo* pActInfo ) : TBase( pActInfo ) {}
	virtual void GetConfiguration( SFlowNodeConfig& config );
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo *pActInfo );
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );

	virtual void OnCancel();
	virtual void OnFinish();

	virtual bool ShouldForce( IFlowNode::SActivationInfo *pActInfo ) const;

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// Executes an Action
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIExecute
	: public CFlowNode_AIForceableBase< true >
{
	typedef CFlowNode_AIForceableBase< true > TBase;
public:
	CFlowNode_AIExecute( IFlowNode::SActivationInfo * pActInfo ) : TBase( pActInfo ), m_bCancel(false) {}
	virtual void GetConfiguration( SFlowNodeConfig& config );
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo );
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );
	virtual EForceMethod GetForceMethod( IFlowNode::SActivationInfo* pActInfo ) const;

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}

protected:
	// should call DoProcessEvent if owner is not too much alerted
	virtual void TryExecute( IAIObject* pAI, EFlowEvent event, SActivationInfo* pActInfo );

	bool m_bCancel;
	virtual void OnCancelPortActivated( IPipeUser* pPipeUser, SActivationInfo* pActInfo );

	virtual void Cancel();
	virtual void Finish();
};

//////////////////////////////////////////////////////////////////////////
// Set Character
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AISetCharacter
	: public CFlowNode_AIBase< false >
{
	typedef CFlowNode_AIBase< false > TBase;
public:
	CFlowNode_AISetCharacter( IFlowNode::SActivationInfo * pActInfo ) : TBase( pActInfo ) {}
	virtual void GetConfiguration( SFlowNodeConfig& config );
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo );
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// Set State
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AISetState
	: public CFlowNode_AIBase< false >
{
	typedef CFlowNode_AIBase< false > TBase;
public:
	CFlowNode_AISetState( IFlowNode::SActivationInfo * pActInfo ) : TBase( pActInfo ) {}
	virtual void GetConfiguration( SFlowNodeConfig& config );
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo );
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// Modify States
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIModifyStates
	: public CFlowNode_AIBase< false >
{
	typedef CFlowNode_AIBase< false > TBase;
public:
	CFlowNode_AIModifyStates( IFlowNode::SActivationInfo * pActInfo ) : TBase( pActInfo ) {}
	virtual void GetConfiguration( SFlowNodeConfig& config );
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo );
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// Check States
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AICheckStates
	: public CFlowNode_AIBase< false >
{
	typedef CFlowNode_AIBase< false > TBase;
public:
	CFlowNode_AICheckStates( IFlowNode::SActivationInfo * pActInfo ) : TBase( pActInfo ) {}
	virtual void GetConfiguration( SFlowNodeConfig& config );
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo );
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// Follow Path 
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIFollowPath
	: public CFlowNode_AIBase< true >
{
	typedef CFlowNode_AIBase< true > TBase;
public:
	CFlowNode_AIFollowPath( IFlowNode::SActivationInfo * pActInfo ) : TBase( pActInfo ) {}
	virtual void GetConfiguration( SFlowNodeConfig &config );
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo );
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// Follow Path Speed Stance
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIFollowPathSpeedStance
	: public CFlowNode_AIForceableBase< true >
{
	typedef CFlowNode_AIForceableBase< true > TBase;
public:
	CFlowNode_AIFollowPathSpeedStance( IFlowNode::SActivationInfo * pActInfo ) : TBase( pActInfo ) {}
	virtual void GetConfiguration( SFlowNodeConfig &config );
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo );

	virtual EForceMethod GetForceMethod( IFlowNode::SActivationInfo* pActInfo ) const;

	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// GOTO 
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIGoto
	: public CFlowNode_AIBase< true >
{
	typedef CFlowNode_AIBase< true > TBase;
	Vec3 m_vDestination;
public:
	CFlowNode_AIGoto( IFlowNode::SActivationInfo * pActInfo ) : TBase( pActInfo ), m_vDestination(ZERO) {}
	virtual void GetConfiguration( SFlowNodeConfig &config );
	virtual void ProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo );
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo );
	virtual void OnResume( IFlowNode::SActivationInfo* pActInfo = NULL );
	virtual void Serialize(SActivationInfo *, TSerialize ser);
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// GOTO - Also sets speed and stance
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIGotoSpeedStance
	: public CFlowNode_AIForceableBase< true >
{
	typedef CFlowNode_AIForceableBase< true > TBase;
	Vec3 m_vDestination;
	int  m_iStance;
	int  m_iSpeed;
	bool m_bAllowStrafing;
public:
	CFlowNode_AIGotoSpeedStance( IFlowNode::SActivationInfo * pActInfo ) : TBase( pActInfo ), m_vDestination(ZERO), m_iStance(4), m_iSpeed(1) {}
	virtual void ProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo );
	virtual void GetConfiguration( SFlowNodeConfig &config );
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo );
	virtual EForceMethod GetForceMethod( IFlowNode::SActivationInfo* pActInfo ) const;
	virtual void OnResume( IFlowNode::SActivationInfo* pActInfo = NULL );
	virtual void Serialize(SActivationInfo *, TSerialize ser);
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// Look At 
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AILookAt
	: public CFlowNode_AIForceableBase< true >
{
	typedef CFlowNode_AIForceableBase< true > TBase;
	CTimeValue m_startTime;
	bool m_bExecuted;
public:
	CFlowNode_AILookAt( IFlowNode::SActivationInfo * pActInfo ) : TBase( pActInfo ), m_bExecuted( false ) {}
	virtual void GetConfiguration( SFlowNodeConfig &config );
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo );
	virtual EForceMethod GetForceMethod( IFlowNode::SActivationInfo* pActInfo ) const;
	virtual void OnCancel();
	virtual bool OnUpdate( IFlowNode::SActivationInfo* pActInfo );
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}

private:
	virtual void PreExecute( SActivationInfo* pActInfo ) { m_bExecuted = false; }
};

//////////////////////////////////////////////////////////////////////////
// body stance controller
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIStance
	: public CFlowNode_AIBase< false >
{
	typedef CFlowNode_AIBase< false > TBase;
public:
	CFlowNode_AIStance( IFlowNode::SActivationInfo * pActInfo ) : TBase( pActInfo ) {}
	virtual void GetConfiguration( SFlowNodeConfig &config );
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo *pActInfo );
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// unload vehicle
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIUnload
	: public CFlowNode_AIBase< true >
{
	typedef CFlowNode_AIBase< true > TBase;
	// must implement this abstract function, not called
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo );

protected:
	int m_numPassengers;
	std::map< int, EntityId > m_mapPassengers;

	void UnloadSeat( IVehicle* pVehicle, int seatId );
	virtual void OnGoalPipeEvent( IPipeUser* pPipeUser, EGoalPipeEvent event, int goalPipeId );
	virtual void UnregisterEvents();

public:
	CFlowNode_AIUnload( IFlowNode::SActivationInfo * pActInfo ) : TBase( pActInfo ) {}
	virtual ~CFlowNode_AIUnload() { UnregisterEvents(); }

	virtual void GetConfiguration( SFlowNodeConfig &config );
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo* pActInfo );
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};


//////////////////////////////////////////////////////////////////////////
// speed controller
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AISpeed
	: public CFlowNode_AIBase< false >
{
	typedef CFlowNode_AIBase< false > TBase;
public:
	CFlowNode_AISpeed( IFlowNode::SActivationInfo * pActInfo ) : TBase( pActInfo ) {}
	virtual void GetConfiguration( SFlowNodeConfig &config );
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo *pActInfo );
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};


class CFlowNode_DebugAISpeed
	: public CFlowNode_AIBase< false >
{
	typedef CFlowNode_AIBase< false > TBase;
public:
	CFlowNode_DebugAISpeed( IFlowNode::SActivationInfo * pActInfo ) : TBase( pActInfo ) {}
	virtual void GetConfiguration( SFlowNodeConfig &config );
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo *pActInfo );
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};


//////////////////////////////////////////////////////////////////////////
// vehicles convoys linker
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIFollow
	: public CFlowNode_AIBase< true >
{
	typedef CFlowNode_AIBase< true > TBase;
public:
	CFlowNode_AIFollow( IFlowNode::SActivationInfo * pActInfo ) : TBase( pActInfo ) {}
	virtual void GetConfiguration( SFlowNodeConfig &config );
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo *pActInfo );
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// create formation
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIFormation
	: public CFlowNode_AIBase< false >
{
	typedef CFlowNode_AIBase< false > TBase;
public:
	CFlowNode_AIFormation( IFlowNode::SActivationInfo * pActInfo ) : TBase( pActInfo ) {}
	virtual void GetConfiguration( SFlowNodeConfig &config );
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo *pActInfo );
	virtual bool ExecuteOnAI(IFlowNode::SActivationInfo* pActInfo, const char* pSignalText,
		IAISignalExtraData* pData, IEntity* pEntity, IEntity* pSender );
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// join formation
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIFormationJoin
	: public CFlowNode_AIBase< true >
{
	typedef CFlowNode_AIBase< true > TBase;
public:
	CFlowNode_AIFormationJoin( IFlowNode::SActivationInfo * pActInfo ) : TBase( pActInfo ) {}
	virtual void GetConfiguration( SFlowNodeConfig &config );
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo *pActInfo );
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};



//////////////////////////////////////////////////////////////////////////
// animation controller
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIAnim
	: public CFlowNode_AIBase< true >
	, IAnimationGraphStateListener
{
	typedef CFlowNode_AIBase< true > TBase;
protected:
	IAnimationGraphState* m_pAGState;
	TAnimationGraphQueryID m_queryID;
	int m_iMethod;
	bool m_bStarted;

public:
	CFlowNode_AIAnim( IFlowNode::SActivationInfo * pActInfo ) : TBase( pActInfo ), m_pAGState(0), m_queryID(0), m_bStarted( false ) {}
	~CFlowNode_AIAnim();

	//////////////////////////////////////////////////////////////////////////
	// IEntityEventListener
	virtual void OnEntityEvent( IEntity* pEntity, SEntityEvent& event );

protected:
	virtual void OnCancelPortActivated( IPipeUser* pPipeUser, SActivationInfo* pActInfo );

	// override this method to unregister listener if the action is canceled
	virtual void OnCancel();

	// from IAnimationGraphStateListener
	virtual void SetOutput( const char * output, const char * value ) {}
	virtual void QueryComplete( TAnimationGraphQueryID queryID, bool succeeded );
	virtual void DestroyedState(IAnimationGraphState*);

public:
	virtual void GetConfiguration( SFlowNodeConfig &config );
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo );
	virtual void Serialize( SActivationInfo* pActInfo, TSerialize ser );
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// AnimEx
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIAnimEx
	: public CFlowNode_AIBase< true >
{
	typedef CFlowNode_AIBase< true > TBase;
	Vec3 m_vPos;
	Vec3 m_vDir;
	int  m_iStance;
	int  m_iSpeed;
public:
	CFlowNode_AIAnimEx( IFlowNode::SActivationInfo * pActInfo ) : TBase( pActInfo ) {}
	virtual void GetConfiguration( SFlowNodeConfig &config );
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo );
	virtual void OnResume( IFlowNode::SActivationInfo* pActInfo = NULL );
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );

	// IGoalPipeListener
	virtual void OnGoalPipeEvent( IPipeUser* pPipeUser, EGoalPipeEvent event, int goalPipeId );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// grab object
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIGrabObject
	: public CFlowNode_AIForceableBase< true >
{
	typedef CFlowNode_AIForceableBase< true > TBase;
public:
	CFlowNode_AIGrabObject( IFlowNode::SActivationInfo * pActInfo ) : TBase( pActInfo ) {}
	virtual void GetConfiguration( SFlowNodeConfig &config );
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo );
	virtual EForceMethod GetForceMethod( IFlowNode::SActivationInfo* pActInfo ) const;
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// drop object
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIDropObject
	: public CFlowNode_AIForceableBase< true >
{
	typedef CFlowNode_AIForceableBase< true > TBase;
public:
	CFlowNode_AIDropObject( IFlowNode::SActivationInfo * pActInfo ) : TBase( pActInfo ) {}
	virtual void GetConfiguration( SFlowNodeConfig &config );
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo );
	virtual EForceMethod GetForceMethod( IFlowNode::SActivationInfo* pActInfo ) const;
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// draw weapon
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIWeaponDraw
	: public CFlowNode_AIForceableBase< true >
{
	typedef CFlowNode_AIForceableBase< true > TBase;
public:
	CFlowNode_AIWeaponDraw( IFlowNode::SActivationInfo* pActInfo ) : TBase( pActInfo ) {}
	virtual void GetConfiguration( SFlowNodeConfig& config );
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo *pActInfo );
	virtual EForceMethod GetForceMethod( IFlowNode::SActivationInfo* pActInfo ) const;
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// holster weapon
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIWeaponHolster
	: public CFlowNode_AIForceableBase< true >
{
	typedef CFlowNode_AIForceableBase< true > TBase;
public:
	CFlowNode_AIWeaponHolster( IFlowNode::SActivationInfo* pActInfo ) : TBase( pActInfo ) {}
	virtual void GetConfiguration( SFlowNodeConfig& config );
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, SActivationInfo *pActInfo );
	virtual EForceMethod GetForceMethod( IFlowNode::SActivationInfo* pActInfo ) const;
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// Shoot At 
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIShootAt
	: public CFlowNode_AIForceableBase< true >
	, public IWeaponEventListener
{
	typedef CFlowNode_AIForceableBase< true > TBase;

	int			m_ShotsNumber;
	EntityId	m_weaponId;

public:
	CFlowNode_AIShootAt( IFlowNode::SActivationInfo * pActInfo ) : TBase( pActInfo ), m_weaponId(0), m_isShootingAtEntity(false) {};
	virtual ~CFlowNode_AIShootAt() { UnregisterEvents(); };

	virtual void GetConfiguration( SFlowNodeConfig &config );
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo );
	virtual void ProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo );
	virtual void OnCancel();
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );
	virtual EForceMethod GetForceMethod( IFlowNode::SActivationInfo* pActInfo ) const;
	virtual bool OnUpdate( IFlowNode::SActivationInfo* pActInfo );
	virtual void OnFinish();
	virtual void Serialize(SActivationInfo *, TSerialize ser);

	//------------------  IWeaponEventListener
	virtual void OnShoot(IWeapon *pWeapon, EntityId shooterId, EntityId ammoId, IEntityClass* pAmmoType,
		const Vec3 &pos, const Vec3 &dir, const Vec3 &vel);
	virtual void OnStartFire(IWeapon *pWeapon, EntityId shooterId) {};
	virtual void OnStopFire(IWeapon *pWeapon, EntityId shooterId) {};
	virtual void OnFireModeChanged(IWeapon *pWeapon, int currentFireMode) {};
	virtual void OnStartReload(IWeapon *pWeapon, EntityId shooterId, IEntityClass* pAmmoType) {};
	virtual void OnEndReload(IWeapon *pWeapon, EntityId shooterId, IEntityClass* pAmmoType) {};
	virtual void OnSetAmmoCount(IWeapon *pWeapon, EntityId shooterId) {};
	virtual void OnOutOfAmmo(IWeapon *pWeapon, IEntityClass* pAmmoType) {};
	virtual void OnReadyToFire(IWeapon *pWeapon) {};
	virtual void OnPickedUp(IWeapon *pWeapon, EntityId actorId, bool destroyed){};
	virtual void OnDropped(IWeapon *pWeapon, EntityId actorId);
	virtual void OnMelee(IWeapon* pWeapon, EntityId shooterId) {}
	virtual void OnStartTargetting(IWeapon *pWeapon) {}
	virtual void OnStopTargetting(IWeapon *pWeapon) {}
	virtual void OnSelected(IWeapon *pWeapon, bool select) {}
	//------------------  ~IWeaponEventListener

	virtual	void UnregisterEvents();
	virtual void UnregisterWithWeapon();

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
	
private:
	void UpdateShootFromInputs( SActivationInfo* pActInfo );
	void OnStopAction();
	
	bool m_isShootingAtEntity;
};

//////////////////////////////////////////////////////////////////////////
// Uses an object
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIUseObject
	: public CFlowNode_AIForceableBase< true >
{
	typedef CFlowNode_AIForceableBase< true > TBase;
public:
	CFlowNode_AIUseObject( IFlowNode::SActivationInfo * pActInfo ) : TBase( pActInfo ) {}
	virtual void GetConfiguration( SFlowNodeConfig& config );
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo );
	virtual EForceMethod GetForceMethod( IFlowNode::SActivationInfo* pActInfo ) const;
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// Selects specific weapon
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIWeaponSelect
	: public CFlowNode_AIForceableBase< true >
{
	typedef CFlowNode_AIForceableBase< true > TBase;
public:
	CFlowNode_AIWeaponSelect( IFlowNode::SActivationInfo * pActInfo ) : TBase( pActInfo ) {}
	virtual void GetConfiguration( SFlowNodeConfig& config );
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo* pActInfo );
	virtual EForceMethod GetForceMethod( IFlowNode::SActivationInfo* pActInfo ) const;
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// Makes ai enter specifyed seat of specifyed vehicle
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIEnterVehicle : public CFlowNode_AIForceableBase< true >
{
	typedef CFlowNode_AIForceableBase< true > TBase;
public:
	enum EInputs {
		IN_SINK = 0,
		IN_CANCEL,
		IN_VEHICLEID,
		IN_SEAT,
		IN_FAST,
		IN_FORCEMETHOD
	};
	CFlowNode_AIEnterVehicle( IFlowNode::SActivationInfo * pActInfo ) : CFlowNode_AIForceableBase<true>( pActInfo ) {}
	virtual void GetConfiguration( SFlowNodeConfig &config );
	virtual void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo *pActInfo );
	virtual EForceMethod GetForceMethod( IFlowNode::SActivationInfo* pActInfo ) const;
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
// Makes ai exit a vehicle
//////////////////////////////////////////////////////////////////////////
class CFlowNode_AIExitVehicle : public CFlowNode_AIForceableBase< true >
{
	typedef CFlowNode_AIForceableBase< true > TBase;
public:
	CFlowNode_AIExitVehicle( IFlowNode::SActivationInfo * pActInfo ) : TBase( pActInfo ) {}
	virtual void GetConfiguration( SFlowNodeConfig &config );
	void DoProcessEvent( IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo *pActInfo );
	virtual EForceMethod GetForceMethod( IFlowNode::SActivationInfo* pActInfo ) const;
	virtual IFlowNodePtr Clone( SActivationInfo* pActInfo );

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}
};


#endif __FlowNodeAIAction_H__
