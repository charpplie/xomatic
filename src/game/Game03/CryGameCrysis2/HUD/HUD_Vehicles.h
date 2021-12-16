#ifndef ___HUD_VEHICLES___
#define ___HUD_VEHICLES___

#include "HUD/HUDObject.h"

#include <IVehicleSystem.h>

// TODO : Move listeners in to HUD Translator if needed else where.
class CHUD_Vehicles : public CHUDObject
                           , IVehicleUsageEventListener
													 , IVehicleEventListener
{
public :

	 CHUD_Vehicles();
	~CHUD_Vehicles();

	void Init( void );

	void Draw( void );

	void OnHUDEvent( const SHUDEvent& event );

	//IVehicleUsageEventListener
	virtual void OnStartUse( const EntityId playerId, IVehicle* pVehicle );
	virtual void OnEndUse( const EntityId playerId, IVehicle* pVehicle );
	//~IVehicleUsageEventListener

	//IVehicleEventListener
	virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params);
	//~IVehicleEventListener

private :
	IHUDAsset* m_vehicleHUD;

	IVehicle*  m_vehicle;

	EntityId m_localActorId;

	bool m_usingVehicle;

	int m_forceShow;
};

#endif // ___HUD_VEHICLES___