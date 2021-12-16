#include "StdAfx.h"
#include "HUD_Vehicles.h"

#include "HUD/HUD.h"
#include "IVehicleSystem.h"
#include "StringUtils.h"


CHUD_Vehicles::CHUD_Vehicles()
: m_vehicleHUD(NULL)
, m_vehicle(NULL)
, m_localActorId(0)
, m_usingVehicle(false)
, m_forceShow(0)
{	
	CHUD::AddHUDEventListener(this,"OnInitPlayer");

	gEnv->pConsole->Register("hud_forceShowVehicleHUD", &m_forceShow, m_forceShow, 0, "Draws the names of the active states etc. on screen");
}

CHUD_Vehicles::~CHUD_Vehicles()
{
	gEnv->pConsole->UnregisterVariable("hud_forceShowVehicleHUD", true);

	g_pGame->GetIGameFramework()->GetIVehicleSystem()->UnregisterVehicleUsageEventListener( m_localActorId, this );
	OnEndUse( m_localActorId, m_vehicle );
	CHUD::RemoveHUDEventListener(this);
}

void CHUD_Vehicles::Init() 
{
	m_vehicleHUD = GetAsset("Vehicles");
}

void CHUD_Vehicles::Draw() 
{
	assert(m_vehicleHUD);
	if(m_usingVehicle || m_forceShow)
	{
		m_vehicleHUD->Draw();
	}
}

void CHUD_Vehicles::OnHUDEvent(const SHUDEvent& hudVehEvent) 
{

	assert(m_vehicleHUD);
	switch( hudVehEvent.eventType )
	{
	case eHUDEvent_OnHUDUnload:
		g_pGame->GetIGameFramework()->GetIVehicleSystem()->UnregisterVehicleUsageEventListener( m_localActorId, this );
		OnEndUse( m_localActorId, m_vehicle );
		m_localActorId = 0;
		break;
	case eHUDEvent_OnHUDReload:
	case eHUDEvent_OnInitPlayer:
		const EntityId localActorId = g_pGame->GetIGameFramework()->GetClientActorId();
		if (m_localActorId && localActorId != m_localActorId)
		{
			g_pGame->GetIGameFramework()->GetIVehicleSystem()->UnregisterVehicleUsageEventListener( m_localActorId, this );
		}
		m_localActorId = localActorId;
		g_pGame->GetIGameFramework()->GetIVehicleSystem()->RegisterVehicleUsageEventListener( m_localActorId, this );
		break;
	}
}

void CHUD_Vehicles::OnStartUse( const EntityId playerId, IVehicle* pVehicle )
{
	// OnStartUse is called again when the player changes seat. In that case don't register listener again.
	if(m_vehicle != pVehicle)
	{
		assert(m_vehicle==NULL);
		pVehicle->RegisterVehicleEventListener(static_cast<IVehicleEventListener*>(this), "player_hud");
		m_vehicle = pVehicle;
	}
	const char* vehicleClass = m_vehicle->GetEntity()->GetClass()->GetName();
	CryLog("HUD: Player is now using a vehicle of class '%s'", vehicleClass);
	m_usingVehicle = true;
}

void CHUD_Vehicles::OnEndUse( const EntityId playerId, IVehicle* pVehicle )
{
	if(!m_vehicle)
		return;

	assert(m_vehicle==pVehicle);
	pVehicle->UnregisterVehicleEventListener(static_cast<IVehicleEventListener*>(this));
	m_vehicle = NULL;
	m_usingVehicle = false;
}

void CHUD_Vehicles::OnVehicleEvent(EVehicleEvent eventId, const SVehicleEventParams& params)
{
	assert( m_vehicle );
	assert( m_localActorId );
	assert( m_usingVehicle );
	switch( eventId )
	{
	// Most frequently called events first to reduce branches.
	case eVE_Collision :
	case eVE_Damaged:
	case eVE_Hit:
		return;
	case eVE_PassengerEnter :
		{
			const EntityId playerId = params.entityId;
			const TVehicleSeatId seatId = params.iParam;

			char* whosSeatRunsSeatWhosSeatRunsSeatThatsWho = "Teammate";
			if( playerId == m_localActorId )
			{
				// current player is getting into vehicle.
				const char* className = m_vehicle->GetEntity()->GetClass()->GetName();
				const int classId = CryStringUtils::CalculateHash(className);
				m_vehicleHUD->Invoke( "gotoAndStop", className);
				whosSeatRunsSeatWhosSeatRunsSeatThatsWho = "Playerpos";
				
				// iterate over all seats setting their state.
				const int seatsCount = m_vehicle->GetSeatCount();
				for( int i=0; i<seatsCount; i++ )
				{
					if( seatId == i )
						continue;

					IVehicleSeat* pSeat = m_vehicle->GetSeatById( i );
					if( pSeat == NULL )
						continue;

					EntityId passengerId = pSeat->GetPassenger();
					const char* seatName = pSeat->GetSeatName();
					if( passengerId )
					{
						m_vehicleHUD->Invoke( string().Format("_root.vehicle.%s.gotoAndStop", seatName).c_str(), "Teammate" );
					}
					else
					{
						m_vehicleHUD->Invoke( string().Format("_root.vehicle.%s.gotoAndStop", seatName).c_str(), "empty" );
					}
				}
			}

			// a player (local or other) is getting into vehicle,
			// mark seat occupied.
			// may be redundant, see eVE_PassengerChangeSeat.
			IVehicleSeat* pSeat = m_vehicle->GetSeatById( seatId );
			const char* seatName = pSeat->GetSeatName();
			m_vehicleHUD->Invoke( string().Format("_root.vehicle.%s.gotoAndStop", seatName).c_str(), whosSeatRunsSeatWhosSeatRunsSeatThatsWho );

			CryLog( "HUD: Vehicles eVE_PassengerEnter : %s, setting to %s", seatName, whosSeatRunsSeatWhosSeatRunsSeatThatsWho );

			break;
		}
	case eVE_PassengerExit:
		{
			// May be redundant, see eVE_SeatFreed;
			const EntityId playerId = params.entityId;
			const TVehicleSeatId seatId = params.iParam;
			const EntityId localActorId = g_pGame->GetIGameFramework()->GetClientActorId();
			if( playerId == m_localActorId ) // Player is getting out of vehicle, no need to reset states
			{
				break;
			}

			// a player (other) is getting out of vehicle,
			// mark seat empty.
			IVehicleSeat* pSeat = m_vehicle->GetSeatById( seatId );
			const char* seatName = pSeat->GetSeatName();
			m_vehicleHUD->Invoke( string().Format("_root.vehicle.%s.gotoAndStop", seatName).c_str(), "Empty" );

			CryLog( "HUD: Vehicles eVE_PassengerExit : %s ", seatName );

			break;
		}
	case eVE_PassengerChangeSeat:
		{
			// An Actor enters a new seat.
			const EntityId playerId = params.entityId;
			const TVehicleSeatId newSeatId = params.iParam;
			IVehicleSeat* pSeat = m_vehicle->GetSeatById( newSeatId );
			const char* seatName = pSeat->GetSeatName();
			char* seatStateFrame = "Teammate";
			if( playerId != m_localActorId ) 
			{
				seatStateFrame = "Playerpos";
			}
			m_vehicleHUD->Invoke( string().Format("_root.vehicle.%s.gotoAndStop", seatName).c_str(), seatStateFrame );

			CryLog( "HUD: Vehicles eVE_PassengerChangeSeat : %s ", seatName );

			break;
		}
	case eVE_SeatFreed: // and Roy.
		{
			// An Actor exits a seat.
			const EntityId playerId = params.entityId;
			const TVehicleSeatId oldSeatId = params.iParam;
			IVehicleSeat* pSeat = m_vehicle->GetSeatById( oldSeatId );
			const char* seatName = pSeat->GetSeatName();
			const char* seatStateFrame = "Empty";
			m_vehicleHUD->Invoke( string().Format("_root.vehicle.%s.gotoAndStop", seatName).c_str(), seatStateFrame );

			CryLog( "HUD: Vehicles eVE_SeatFreed : %s ", seatName );
			break;
		}
	}
}
