/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Controls player movement when using a mounted gun

-------------------------------------------------------------------------
History:
- 2:10:2009   Created by Benito Gangoso Rodriguez

*************************************************************************/

#pragma once

#ifndef _MOUNTED_GUN_CONTROLLER_H_
#define _MOUNTED_GUN_CONTROLLER_H_

class CPlayer;
class CItem;

class CMountedGunController
{
public:
	CMountedGunController()
		: m_pControlledPlayer(NULL)
	{

	}

	ILINE void InitWithPlayer(CPlayer* pPlayer) { assert(m_pControlledPlayer == NULL); m_pControlledPlayer = pPlayer; }

	void OnEnter(EntityId mountedGunID);
	void OnLeave();
	void Update(EntityId mountedGunID, float frameTime);

private:

	void UpdateGunnerLocation(CItem* pMountedGun, IEntity* pParent, const Vec3& bodyDirection);
	void UpdateThirdPersonAnimations(ICharacterInstance* pCharacter, float aimRad, float aimUp, float aimDown, bool firstPerson = false);
	void UpdateFirstPersonAnimations(CItem* pMountedGun);
	void UpdateIKMounted(CItem* pMountedGun);
	void UpdateViewLimits(CItem* pMountedGun, IEntity* pParent);
	Vec3 GetMountDirection(CItem* pMountedGun, IEntity* pParent) const;
	Vec3 GetMountedGunPosition(CItem* pMountedGun, IEntity* pParent) const;


	CPlayer*		m_pControlledPlayer;
};

#endif
