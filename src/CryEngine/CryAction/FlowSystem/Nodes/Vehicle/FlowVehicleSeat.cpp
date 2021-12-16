/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a flow node to vehicle seats

-------------------------------------------------------------------------
History:
- 12:12:2005: Created by Mathieu Pinard

*************************************************************************/
#include "StdAfx.h"
#include "CryAction.h"
#include "IVehicleSystem.h"
#include "VehicleSystem/Vehicle.h"
#include "VehicleSystem/VehicleSeat.h"
#include "IFlowSystem.h"
#include "FlowSystem/Nodes/FlowBaseNode.h"
#include "FlowVehicleSeat.h"

//------------------------------------------------------------------------
CFlowVehicleSeat::CFlowVehicleSeat(SActivationInfo* pActivationInfo)
{
	Init(pActivationInfo);

	m_isDriverSeatRequested = false;
	m_seatId = InvalidVehicleSeatId;
}

//------------------------------------------------------------------------
IFlowNodePtr CFlowVehicleSeat::Clone(SActivationInfo* pActivationInfo)
{
	IFlowNode* pNode = new CFlowVehicleSeat(pActivationInfo);
	return pNode;
}

//------------------------------------------------------------------------
void CFlowVehicleSeat::GetConfiguration(SFlowNodeConfig& nodeConfig)
{
	CFlowVehicleBase::GetConfiguration(nodeConfig);

	static const SInputPortConfig pInConfig[] = 
	{
    InputPortConfig<int>( "seatId", _HELP("seatId (optional, use instead of seatName)"), _HELP("Seat"), _UICONFIG("enum_int:None=0,Driver=1,Gunner=2,Seat03=3,Seat04=4,Seat05=5,Seat06=6,Seat07=7,Seat08=8,Seat09=9,Seat10=10,Seat11=11") ),
		InputPortConfig<string>("seatName", _HELP("the name of the seat which should be requested (optional, use instead of seatId)")),
		InputPortConfig<bool>("isDriverSeat", _HELP("used to select the driver seat (optional).")),
    InputPortConfig<bool>("lock", _HELP("used to lock/unlock the selected seat.")),
		{0}
	};

	static const SOutputPortConfig pOutConfig[] = 
	{
		OutputPortConfig<int>("seatId", _HELP("the seat id selected by the input values")),
		OutputPortConfig<int>("passengerId", _HELP("the entity id of the current passenger on the seat, if there's any")),
		{0}
	};

	nodeConfig.sDescription = _HELP("handle vehicle seats");
	nodeConfig.nFlags |= EFLN_TARGET_ENTITY;
	nodeConfig.pInputPorts = pInConfig;
	nodeConfig.pOutputPorts = pOutConfig;
	nodeConfig.SetCategory(EFLN_APPROVED);
}

//------------------------------------------------------------------------
void CFlowVehicleSeat::ProcessEvent(EFlowEvent flowEvent, SActivationInfo* pActivationInfo)
{
	CFlowVehicleBase::ProcessEvent(flowEvent, pActivationInfo);

	if (flowEvent == eFE_Initialize || flowEvent == eFE_Activate)
	{
    CVehicle* pVehicle = static_cast<CVehicle*>(GetVehicle());
    
    if (!pVehicle)
      return;

    if (IsPortActive(pActivationInfo, IN_SEATID))
    {
      m_seatId = GetPortInt(pActivationInfo, IN_SEATID);
    }

		if (IsPortActive(pActivationInfo, IN_SEATNAME))
		{			
      const string& seatName = GetPortString(pActivationInfo, IN_SEATNAME);				
      if (!seatName.empty())
        m_seatId = pVehicle->GetSeatId(seatName.c_str());		
		}

    if (IsPortActive(pActivationInfo, IN_DRIVERSEAT))
    {
      if (GetPortBool(pActivationInfo, IN_DRIVERSEAT))
        m_seatId = pVehicle->GetDriverSeatId();
    }

    if (IsPortActive(pActivationInfo, IN_LOCK))
    { 
      if (CVehicleSeat* pSeat = (CVehicleSeat*)pVehicle->GetSeatById(m_seatId))
      {
        bool lock = GetPortBool(pActivationInfo, IN_LOCK);
        pSeat->Lock(lock);
      }     
    }

    ActivateOutputPorts(pActivationInfo);
	}
}

//------------------------------------------------------------------------
void CFlowVehicleSeat::OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params)
{
	CFlowVehicleBase::OnVehicleEvent(event, params);

	if (event == eVE_PassengerEnter)
	{
		if (m_seatId != InvalidVehicleSeatId)
		{
			if (params.iParam == m_seatId)
			{
				if (IVehicle* pVehicle = GetVehicle())
				{
					if (IVehicleSeat* pSeat = pVehicle->GetSeatById(m_seatId))
					{
						EntityId actorId = pSeat->GetPassenger();
						SFlowAddress addr(m_nodeID, OUT_PASSENGERID, true);
						m_pGraph->ActivatePort(addr, actorId);
					}
				}
			}
		}
	}
	else if (event == eVE_PassengerExit)
	{
		if (m_seatId != InvalidVehicleSeatId)
		{
			if (params.iParam == m_seatId)
			{
				if (IVehicle* pVehicle = GetVehicle())
				{
					if (IVehicleSeat* pSeat = pVehicle->GetSeatById(m_seatId))
					{
						SFlowAddress addr(m_nodeID, OUT_PASSENGERID, true);
						m_pGraph->ActivatePort(addr, 0);
					}
				}
			}
		}
	}
}

//------------------------------------------------------------------------
void CFlowVehicleSeat::ActivateOutputPorts(SActivationInfo* pActivationInfo)
{
	IVehicle* pVehicle = GetVehicle();

	if (!pVehicle)
		return;
	
	if (m_seatId != InvalidVehicleSeatId)
	{
		ActivateOutput(pActivationInfo, OUT_SEATID, m_seatId);

		IVehicleSeat* pSeat = pVehicle->GetSeatById(m_seatId);
		if(pSeat)
		{
			TVehicleSeatId passengerId = pSeat->GetPassenger();
			ActivateOutput(pActivationInfo, OUT_PASSENGERID, passengerId);
		}
	}
}


//////////////////////////////////////////////////////////////////////////
class CFlowVehicleChangeSeat : public CFlowBaseNode
{
	enum EInputPorts
	{
		eINP_Sync = 0,
		eINP_Seat
	};

	enum EOutputPorts
	{
		eOUT_Succeed = 0,
		eOUT_Fail
	};
public:
	CFlowVehicleChangeSeat( SActivationInfo * pActInfo ) {}
	~CFlowVehicleChangeSeat() {}
	
	IFlowNodePtr Clone(SActivationInfo* pActivationInfo)
	{
		IFlowNode* pNode = new CFlowVehicleChangeSeat(pActivationInfo);
		return pNode;
	}
	
	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SInputPortConfig inp_config[] = {
			InputPortConfig_Void ("Sync", _HELP("Triggers the seat change")),
			InputPortConfig<int>( "SeatNumber", _HELP("New seat"), _HELP("Seat"), _UICONFIG("enum_int:None=0,Driver=1,Gunner=2,Seat03=3,Seat04=4,Seat05=5,Seat06=6,Seat07=7,Seat08=8,Seat09=9,Seat10=10,Seat11=11") ),
			{0}
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig_Void("Succeed"),
			OutputPortConfig_Void("Fail"),
			{0}
		};

		config.sDescription = _HELP( "Moves an actor from a seat to another one. Only works if the actor is already inside a vehicle" );
		config.pInputPorts = inp_config;
		config.pOutputPorts = out_config;
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.SetCategory(EFLN_APPROVED);
	}
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
			case eFE_Activate:
			{	
				if (IsPortActive( pActInfo, eINP_Sync ) && pActInfo->pEntity )
				{
					TVehicleSeatId newSeatId = GetPortInt( pActInfo, eINP_Seat );
					bool success = false;

					EntityId actorEntityId = pActInfo->pEntity->GetId();
					IActor* pActor = gEnv->pGame->GetIGameFramework()->GetIActorSystem()->GetActor( actorEntityId );
					if (pActor && pActor->GetLinkedVehicle())
					{
						CVehicle* pVehicle = static_cast<CVehicle*>( pActor->GetLinkedVehicle() );
						pVehicle->ChangeSeat( actorEntityId, 0, newSeatId );
						IVehicleSeat* pNewSeat = pVehicle->GetSeatForPassenger( actorEntityId );
						success = pNewSeat && pNewSeat->GetSeatId()==newSeatId;
					}
					ActivateOutput( pActInfo, success ? eOUT_Succeed : eOUT_Fail, true );
				}
				break;
			}
		}
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}

};


//////////////////////////////////////////////////////////////////////////
class CFlowVehicleLock : public CFlowBaseNode
{
	enum EInputPorts
	{
		eINP_Lock = 0,
		eINP_Unlock
	};

public:
	CFlowVehicleLock( SActivationInfo * pActInfo ) {}
	~CFlowVehicleLock() {}

	IFlowNodePtr Clone(SActivationInfo* pActivationInfo)
	{
		IFlowNode* pNode = new CFlowVehicleLock(pActivationInfo);
		return pNode;
	}

	virtual void GetConfiguration( SFlowNodeConfig &config )
	{
		static const SInputPortConfig inp_config[] = {
			InputPortConfig_Void ("Lock", _HELP("Locks the vehicle")),
			InputPortConfig_Void ("Unlock", _HELP("Unlocks the vehicle")),
			{0}
		};
		static const SOutputPortConfig out_config[] = {
			{0}
		};

		config.sDescription = _HELP( "Locks/Unlocks all seats of a vehicle" );
		config.pInputPorts = inp_config;
		config.pOutputPorts = out_config;
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.SetCategory(EFLN_APPROVED);
	}
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
			case eFE_Activate:
			{	
				IVehicle* pVehicle = pActInfo->pEntity ? CCryAction::GetCryAction()->GetIVehicleSystem()->GetVehicle( pActInfo->pEntity->GetId() ) : NULL;
				
				if (pVehicle)
				{
					if (IsPortActive( pActInfo, eINP_Lock ))
						Lock( pVehicle, true );
					if (IsPortActive( pActInfo, eINP_Unlock ))
						Lock( pVehicle, false );
				}
				break;
			}
		}
	}
	
	
	void Lock( IVehicle* pVehicle, bool lock )
	{
		for (uint32 i=0; i<pVehicle->GetSeatCount(); ++i)
		{
			CVehicleSeat* pSeat = static_cast<CVehicleSeat*>(pVehicle->GetSeatById( i ));
			if (pSeat)
				pSeat->Lock( lock );
		}
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}

};



REGISTER_FLOW_NODE( "Vehicle:VehicleSeat", CFlowVehicleSeat );
REGISTER_FLOW_NODE( "Vehicle:ChangeSeat", CFlowVehicleChangeSeat )
REGISTER_FLOW_NODE( "Vehicle:Lock", CFlowVehicleLock )
