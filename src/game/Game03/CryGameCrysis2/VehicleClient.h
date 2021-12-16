/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Implements a class which handle client actions on vehicles.

-------------------------------------------------------------------------
History:
- 17:10:2006: Created by Mathieu Pinard

*************************************************************************/
#ifndef __VEHICLECLIENT_H__
#define __VEHICLECLIENT_H__

#include <map>
#include <vector>
#include <IActionMapManager.h>

class CPlayer;

enum EVehicleActionExtIds
{
	eVAI_ActionsExt = eVAI_Others,
	eVAI_RipoffWeapon,
};

class CVehicleClient
	: public IVehicleClient
{
public:

	virtual bool Init();
	virtual void Reset();
	virtual void Release() { delete this; }

	virtual void OnAction(IVehicle* pVehicle, EntityId actorId, const ActionId& actionId, int activationMode, float value);
	virtual void PreUpdate(IVehicle* pVehicle, EntityId actorId);
	virtual void OnEnterVehicleSeat(IVehicleSeat* pSeat);
	virtual void OnExitVehicleSeat(IVehicleSeat* pSeat);
	virtual void OnEnterVehicle(IVehicle* pVehicle, const EntityId passengerId);
	virtual void OnExitVehicle(IVehicle* pVehicle, const EntityId passengerId);

protected:

	typedef std::map<ActionId, int> TActionNameIdMap;
	TActionNameIdMap m_actionNameIds;

	struct SVehicleSeatClientInfo
	{
		TVehicleSeatId seatId;
		TVehicleViewId viewId;
	};

	typedef std::vector <SVehicleSeatClientInfo> TVehicleSeatClientInfoVector;

	struct SVehicleClientInfo
	{
		TVehicleSeatClientInfoVector seats;
	};

	typedef std::map <IEntityClass*, SVehicleClientInfo> TVehicleClientInfoMap;
	TVehicleClientInfoMap m_vehiclesInfo;

	SVehicleClientInfo& GetVehicleClientInfo(IVehicle* pVehicle);
	SVehicleSeatClientInfo& GetVehicleSeatClientInfo(SVehicleClientInfo& vehicleClientInfo, TVehicleSeatId seatId);

private:
	Ang3 m_xiRotation;
	Ang3 m_xiMovement;
  float m_fLeftRight;
  float m_fForwardBackward;
	bool m_bMovementFlagForward;
	bool m_bMovementFlagBack;
	bool m_bMovementFlagRight;
	bool m_bMovementFlagLeft;

  bool m_tp;

	bool m_cloaked;
	EntityId m_playerId;
	EntityId m_vehicleId;

};

#endif
