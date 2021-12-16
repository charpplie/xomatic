/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  Description: Action manager and dispatcher.
  
 -------------------------------------------------------------------------
  History:
  - 7:9:2004   17:36 : Created by Márcio Martins

*************************************************************************/
#ifndef __ACTIONMAPMANAGER_H__
#define __ACTIONMAPMANAGER_H__

#if _MSC_VER > 1000
# pragma once
#endif

#include "IActionMapManager.h"
#include "IInput.h"
class Crc32Gen;

typedef std::map<string, class CActionMap *>		TActionMapMap;
typedef std::map<string, class CActionFilter *>	TActionFilterMap;

class CActionMapManager :
	public IActionMapManager,
	public IInputEventListener
{
public:
	CActionMapManager(IInput *pInput);

	void Release() { delete this; };

	// IInputEventListener
	virtual bool OnInputEvent(const SInputEvent &event);
	// ~IInputEventListener

	// IActionMapManager
	VIRTUAL void Update();
	VIRTUAL void Reset();
	VIRTUAL void Clear();

	VIRTUAL void LoadFromXML(const XmlNodeRef& node, bool bCheckVersion=false);
	VIRTUAL void SaveToXML(const XmlNodeRef& node);

	VIRTUAL void SetExtraActionListener(IActionListener* pExtraActionListener);
	VIRTUAL IActionListener* GetExtraActionListener();

	VIRTUAL IActionMap *CreateActionMap(const char *name);
	VIRTUAL IActionMap *GetActionMap(const char *name);
	VIRTUAL IActionFilter *CreateActionFilter(const char *name, EActionFilterType type=eAFT_ActionFail);
	VIRTUAL IActionFilter *GetActionFilter(const char *name);
	VIRTUAL IActionMapIteratorPtr CreateActionMapIterator();
	VIRTUAL IActionFilterIteratorPtr CreateActionFilterIterator();


	VIRTUAL void Enable(bool enable);
	VIRTUAL void EnableActionMap(const char *name, bool enable);
	VIRTUAL void EnableFilter(const char *name, bool enable);
	VIRTUAL bool IsFilterEnabled(const char *name);
	VIRTUAL void ReleaseFilteredActions();
	// ~IActionMapManager

	int GetVersion() const { return  m_version; }
	void SetVersion(int version) { m_version = version; }

	bool ActionFiltered(const ActionId& action);

	void RemoveActionMap(CActionMap *pActionMap);
	void RemoveActionFilter(CActionFilter *pActionFilter);

	void ReleaseActionIfActive(const ActionId& actionId);

	const Crc32Gen&	GetCrc32Gen() const;

	void AddBind(uint32 crc, const ActionId& actionId, uint32 keyNumber, CActionMap* pActionMap);
	bool RemoveBind(uint32 crc, const ActionId& actionId, uint32 keyNumber, CActionMap* pActionMap);

	void GetMemoryStatistics(ICrySizer * s);

protected:
	virtual ~CActionMapManager();

private:

	IActionListener* m_pExtraActionListener;

	struct SBindData
	{
		ActionId		actionId;
		uint32			keyNumber;
		CActionMap*	pActionMap;

		SBindData(ActionId id, uint32 k, CActionMap* pAM)
		{
			actionId = id;
			keyNumber = k;
			pActionMap = pAM;
		}
	};
	typedef std::multimap<uint32, SBindData> TInputCRCToBind;
	typedef std::list<const SBindData*> TBindPriorityList;


	void DispatchEvent(const SInputEvent &inputEvent);
	bool CreateEventPriorityList(const SInputEvent &inputEvent, TBindPriorityList &PriorityList);

	IInput*						m_pInput;
	TActionMapMap			m_actionMaps;
	TActionFilterMap	m_actionFilters;
	TInputCRCToBind		m_inputCRCToBind;
	int               m_version;
	bool							m_enabled;
};


#endif //__ACTIONMAPMANAGER_H__
