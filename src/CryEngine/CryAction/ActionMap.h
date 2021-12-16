/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Action Map implementation. Maps Actions to Keys.
  
 -------------------------------------------------------------------------
  History:
  - 7:9:2004   17:47 : Created by Márcio Martins

*************************************************************************/
#ifndef __ACTIONMAP_H__
#define __ACTIONMAP_H__

#if _MSC_VER > 1000
# pragma once
#endif


#include "IActionMapManager.h"


#define KEYS_PER_ACTION (3)


// Blocking type - Used to suppress some actions from firing due to higher-priority ones also being fired
enum EActionBlockingType
{
	eABT_Ignore = 0,	// This action shouldn't be considered
	eABT_Normal,		// This action fired, low priority
	eABT_TimerHeld,		// This action fired because of the hold timer
};


struct SAction
{
	SAction()
	{
		activationMode=eAAM_OnPress;
		currentState=eIS_Unknown;
		fPressedTime = 0.0f;
	}

	uint32			crcKey[KEYS_PER_ACTION];
	int				activationMode;
	float			fPressedTime;
	EInputState		currentState;
	CCryName		key[KEYS_PER_ACTION];
	CCryName		defaultKey[KEYS_PER_ACTION];
};


class CActionMapManager;

class CActionMap :
	public IActionMap
{
public:
	CActionMap(CActionMapManager *pActionMapManager, const char* name);
	virtual ~CActionMap();

	// IActionMap
	VIRTUAL void GetMemoryStatistics(ICrySizer * s);
	virtual void Release() { delete this; };
	VIRTUAL void CreateAction(const char *name, int activationMode, const char*defaultKey0, const char*defaultKey1);
	VIRTUAL void BindAction(const char *name, const char *keyName, int keyNumber);
	VIRTUAL void SetActionListener(EntityId id);
	VIRTUAL void Reset();
	VIRTUAL bool SerializeXML(const XmlNodeRef& root, bool bLoading);
	VIRTUAL IActionMapBindInfoIteratorPtr CreateBindInfoIterator();
	VIRTUAL const char* GetName() { return m_name.c_str(); }
	VIRTUAL bool GetBindInfo(const ActionId& actionName, SActionMapBindInfo& outInfo, int maxKeys);
	VIRTUAL void Enable(bool enable);
	VIRTUAL bool Enabled() { return m_enabled; };
	// ~IActionMap
	
	bool GetAction(const ActionId& actionId, SAction const*& pOutAction) const;

	//! Returns how long the action button must be held down (if using hold logic)
	float GetActionHoldTime(const ActionId& actionId) const;
	float GetActionHoldTime(SAction const* pAction) const;

	//! Returns TRUE if this action can be processed (conditions are met)
	bool CanProcessInput(EActionBlockingType &eActionBlockingType, const ActionId& actionId, const SInputEvent &inputEvent, uint32 keyNumber, uint32 inputCrc);

	//! Processes the action (notifies the handlers)
	void DoProcessInput(const ActionId& actionId, const SInputEvent &inputEvent);

	void InputProcessed();
	void ReleaseActionIfActive(const ActionId& actionId);
	void ReleaseFilteredActions();

private:
	void AddBind(uint32 crc, const ActionId& actionId, uint32 keyNumber);
	void RemoveBind(uint32 crc, const ActionId& actionId, uint32 keyNumber);
	void ReleaseActionIfActiveInternal(const ActionId& actionId, SAction &action);

	bool GetAction(const ActionId& actionId, SAction *& pOutAction);

	//! Determines if the action was triggered or not
	bool IsActionTriggered(SAction *pAction, const SInputEvent &inputEvent, EActionBlockingType &eActionBlockingType) const;

	typedef std::map<ActionId, SAction> TActionMap;

	bool								m_enabled;
	CActionMapManager*	m_pActionMapManager;
	TActionMap					m_actions;
	EntityId						m_listenerId;
	string							m_name;
};


#endif //__ACTIONMAP_H__
