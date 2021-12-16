/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------

Description: LTAG Implementation

-------------------------------------------------------------------------
History:
- 16:09:09	: Created by Benito Gangoso Rodriguez

*************************************************************************/
#pragma once

#ifndef _LTAG_H_
#define _LTAG_H_

#include "Weapon.h"

class CLTag :	public CWeapon
{
public:

	CLTag();
	virtual ~CLTag();

	virtual void Reset();
	virtual void FullSerialize( TSerialize ser );
	virtual void OnSelected(bool selected);

	virtual void OnAction(EntityId actorId, const ActionId& actionId, int activationMode, float value);

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->AddObject(this, sizeof(*this));
		CWeapon::GetInternalMemoryUsage(s); // collect memory of parent class
	}
	virtual bool CanModify() const;
	virtual void ProcessEvent(SEntityEvent& event);
	//virtual bool NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags );
	//virtual void Select(bool select);

protected:

	virtual void AnimationEvent(ICharacterInstance *pCharacter, const AnimEventInstance &event);

private:

	void HideGrenade(ICharacterInstance* pWeaponCharacter);
	void ShowGrenades(ICharacterInstance* pWeaponCharacter);
	void HideGrenadeAttachment(ICharacterInstance* pWeaponCharacter, const char* attachmentName, bool hide);
	void SetupGrenadeAttachments(int numberOfVisibleGrenades);

	typedef CWeapon inherited;
	static TActionHandler<CLTag> s_actionHandler;

	bool OnActionSwitchFireMode(EntityId actorId, const ActionId& actionId, int activationMode, float value);

	int m_visibleGrenades;	
};

#endif // _LTAG_H_