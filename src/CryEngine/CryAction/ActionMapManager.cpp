/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
  $Id$
  $DateTime$
  
 -------------------------------------------------------------------------
  History:
  - 7:9:2004   17:42 : Created by Márcio Martins

*************************************************************************/
#include "StdAfx.h"
#include "ActionMapManager.h"
#include "ActionMap.h"
#include "ActionFilter.h"
#include <crc32.h>

//------------------------------------------------------------------------
CActionMapManager::CActionMapManager(IInput *pInput)
: m_pInput(pInput),
	m_enabled(true),
	m_version(-1),    // the first actionmap/filter sets the version
	m_pExtraActionListener(0)
{
	if (m_pInput) m_pInput->AddEventListener(this);
}

//------------------------------------------------------------------------
CActionMapManager::~CActionMapManager()
{
	if (m_pInput) m_pInput->RemoveEventListener(this);
}

//------------------------------------------------------------------------
bool CActionMapManager::OnInputEvent(const SInputEvent &event)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_ACTION);

	if (m_enabled)
	{
		DispatchEvent(event);
	}	

	return false;
}

//------------------------------------------------------------------------
void CActionMapManager::Update()
{
	if (!m_enabled)
	{
		return;
	}

	for (TActionMapMap::iterator it = m_actionMaps.begin(); it != m_actionMaps.end(); it++)
	{
		CActionMap *pAm = it->second;
		if (pAm->Enabled())
			pAm->InputProcessed();
	}
}

//------------------------------------------------------------------------
void CActionMapManager::Reset()
{
	for (TActionMapMap::iterator it = m_actionMaps.begin(); it != m_actionMaps.end(); it++)
	{
		it->second->Reset();
	}
}

//------------------------------------------------------------------------
void CActionMapManager::Clear()
{
	// FIXME: who is responsible for deleting the actionmaps and filters?
	m_actionMaps.clear();
	m_actionFilters.clear();
}

//------------------------------------------------------------------------
void CActionMapManager::LoadFromXML(const XmlNodeRef& node, bool bCheckVersion)
{
	int version = -1;

	if(node->getAttr("version", version))
	{
		if (bCheckVersion && version != m_version)
		{
			GameWarning("CActionMapManager::LoadFromXML: Didn't load actionmaps. Version found %d -> required %d", version, m_version);
			return;
		}
		else if (!bCheckVersion && m_version == -1 && version != -1)
		{
			m_version = version;	
		}
	}
	else
	{
		GameWarning("Obsolete action map format - version info is missing");
		m_version = 0;
	}
	
	//	load action maps
	int nChildren = node->getChildCount();
	for (int i=0; i<nChildren; ++i)
	{
		XmlNodeRef child = node->getChild(i);
		if (!strcmp(child->getTag(), "actionmap"))
		{
			IActionMap* pActionMap = CreateActionMap(child->getAttr("name"));
			pActionMap->SerializeXML(child, true);
		}
	}
}

void CActionMapManager::SaveToXML(const XmlNodeRef& node)
{
	node->setAttr("version", m_version);
	TActionMapMap::iterator iter = m_actionMaps.begin();
	while (iter != m_actionMaps.end())
	{
		iter->second->SerializeXML(node, false);
		++iter;
	}
}

//------------------------------------------------------------------------

void CActionMapManager::SetExtraActionListener(IActionListener* pExtraActionListener)
{
	m_pExtraActionListener = pExtraActionListener;
}

//------------------------------------------------------------------------

IActionListener* CActionMapManager::GetExtraActionListener()
{
	return m_pExtraActionListener;
}

//------------------------------------------------------------------------
IActionMap *CActionMapManager::CreateActionMap(const char *name)
{
	IActionMap* pActionMap = GetActionMap(name);

	if (!pActionMap)
	{
		CActionMap* pMap = new CActionMap(this, name);

		m_actionMaps.insert(TActionMapMap::value_type(name, pMap));
		return pMap;
	}

	return pActionMap;
}

//------------------------------------------------------------------------
IActionMap *CActionMapManager::GetActionMap(const char *name)
{
	TActionMapMap::iterator it = m_actionMaps.find(CONST_TEMP_STRING(name));
	if (it == m_actionMaps.end())
		return 0;
	else
		return it->second;
}

//------------------------------------------------------------------------
IActionFilter *CActionMapManager::CreateActionFilter(const char *name, EActionFilterType type)
{
	IActionFilter *pActionFilter = GetActionFilter(name);

	if (!pActionFilter)
	{
		CActionFilter *pFilter = new CActionFilter(this, m_pInput, name, type);

		m_actionFilters.insert(TActionFilterMap::value_type(name, pFilter));

		return pFilter;
	}

	return pActionFilter;
}

//------------------------------------------------------------------------
IActionFilter *CActionMapManager::GetActionFilter(const char *name)
{
	TActionFilterMap::iterator it = m_actionFilters.find(CONST_TEMP_STRING(name));

	if (it != m_actionFilters.end())
	{
		return it->second;
	}

	return 0;
}

//------------------------------------------------------------------------
void CActionMapManager::Enable(bool enable)
{
	m_enabled = enable;
}

//------------------------------------------------------------------------
void CActionMapManager::EnableActionMap(const char *name, bool enable)
{
	if (name == 0 || *name == 0)
	{
		TActionMapMap::iterator it = m_actionMaps.begin();
		TActionMapMap::iterator itEnd = m_actionMaps.end();
		while (it != itEnd)
		{
			it->second->Enable(enable);
			++it;
		}
	}
	else
	{
		TActionMapMap::iterator it = m_actionMaps.find(CONST_TEMP_STRING(name));

		if (it != m_actionMaps.end())
		{
			if(it->second->Enabled() != enable)
			{
				it->second->Enable(enable);
				ReleaseFilteredActions();
			}
		}
	}
}

//------------------------------------------------------------------------
void CActionMapManager::EnableFilter(const char *name, bool enable)
{
	if (name == 0 || *name == 0)
	{
		TActionFilterMap::iterator it = m_actionFilters.begin();
		TActionFilterMap::iterator itEnd = m_actionFilters.end();
		while (it != itEnd)
		{
			it->second->Enable(enable);
			++it;
		}
	}
	else
	{
		TActionFilterMap::iterator it = m_actionFilters.find(CONST_TEMP_STRING(name));

		if (it != m_actionFilters.end())
		{
			if(it->second->Enabled() != enable)
			{
				it->second->Enable(enable);
				ReleaseFilteredActions();
			}
		}
	}
}

//------------------------------------------------------------------------
void CActionMapManager::ReleaseFilteredActions()
{
	for(TActionMapMap::iterator it = m_actionMaps.begin(), eit = m_actionMaps.end();it!=eit; ++it)
	{
		if(it->second->Enabled())
		{
			it->second->ReleaseFilteredActions();
		}
	}
}

//------------------------------------------------------------------------
bool CActionMapManager::IsFilterEnabled(const char *name)
{
	TActionFilterMap::iterator it = m_actionFilters.find(CONST_TEMP_STRING(name));

	if (it != m_actionFilters.end())
	{
		return it->second->Enabled();
	}

	return false;
}

//------------------------------------------------------------------------
bool CActionMapManager::ActionFiltered(const ActionId& action)
{
	for (TActionFilterMap::iterator it = m_actionFilters.begin(); it != m_actionFilters.end(); it++)
	{
		if (it->second->ActionFiltered(action))
		{
			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------
void CActionMapManager::DispatchEvent(const SInputEvent &inputEvent)
{
	// no actions while console is open
	if (gEnv->pConsole->IsOpened())
		return;

	if (inputEvent.keyName.key && inputEvent.keyName.key[0] == 0)
		return;

	// Process priority list based on event
	TBindPriorityList PriorityList;
	if (CreateEventPriorityList(inputEvent, PriorityList))
	{
		TBindPriorityList::iterator itBind = PriorityList.begin();
		TBindPriorityList::const_iterator itBindEnd = PriorityList.end();
		for (; itBind != itBindEnd; ++itBind)
		{
			const SBindData* pBindData = *itBind;
			CRY_ASSERT(pBindData);
			
			// Fire it
			pBindData->pActionMap->DoProcessInput(pBindData->actionId, inputEvent);
		}
	}
}

//------------------------------------------------------------------------
bool CActionMapManager::CreateEventPriorityList(const SInputEvent &inputEvent, TBindPriorityList &PriorityList)
{
	bool bRes = false;

	// Look up the actions which are associated with this
	uint32 inputCrc = GetCrc32Gen().GetCRC32Lowercase(inputEvent.keyName);
	TInputCRCToBind::iterator itCRCBound = m_inputCRCToBind.lower_bound(inputCrc);
	TInputCRCToBind::const_iterator itCRCBoundEnd = m_inputCRCToBind.end();

	// Create priority listing for which processes should be fired
	EActionBlockingType eCurrentPriority = eABT_Normal;
	float fLongestHeldTime = 0.0f;
	for (; itCRCBound != itCRCBoundEnd && itCRCBound->first == inputCrc; ++itCRCBound)
	{
		const SBindData& bindData = itCRCBound->second;

		// Check for blocking input!
		EActionBlockingType eThisPriority = eABT_Ignore;
		bool bAdd = (!ActionFiltered(bindData.actionId) && bindData.pActionMap->CanProcessInput(eThisPriority, bindData.actionId, inputEvent, bindData.keyNumber, inputCrc));

		if (eThisPriority > eCurrentPriority)
		{
			// Erase what we had before
			PriorityList.clear();
			eCurrentPriority = eThisPriority;
		}

		if (eThisPriority == eCurrentPriority)
		{
			// Handle adding based on blocking type
			if (eABT_TimerHeld == eCurrentPriority)
			{
				const float fHoldTime = bindData.pActionMap->GetActionHoldTime(bindData.actionId);
				if (fLongestHeldTime < fHoldTime)
				{
					// Start over
					fLongestHeldTime = fHoldTime;
					PriorityList.clear();
				}
				else if (fLongestHeldTime != fHoldTime)
				{
					bAdd = false;
				}
			}

			if (bAdd)
			{
				PriorityList.push_back(&bindData);
				bRes = true;
			}
		}
	}

	return bRes;
}

//------------------------------------------------------------------------
void CActionMapManager::RemoveActionMap(CActionMap *pActionMap)
{
	for(TInputCRCToBind::iterator iter=m_inputCRCToBind.begin(); iter!=m_inputCRCToBind.end(); ++iter)
	{
		const SBindData *pBindData = &((*iter).second);
		if(pBindData->pActionMap == pActionMap)
		{
			TInputCRCToBind::iterator iterNext = iter;
			iterNext++;
			m_inputCRCToBind.erase(iter);
			iter = iterNext;
		}
	}

	for (TActionMapMap::iterator it = m_actionMaps.begin(); it != m_actionMaps.end(); it++)
	{
		if (it->second == pActionMap)
		{
			m_actionMaps.erase(it);

			return;
		}
	}
}

//------------------------------------------------------------------------
void CActionMapManager::RemoveActionFilter(CActionFilter *pActionFilter)
{
	for (TActionFilterMap::iterator it = m_actionFilters.begin(); it != m_actionFilters.end(); it++)
	{
		if (it->second == pActionFilter)
		{
			m_actionFilters.erase(it);

			return;
		}
	}
}

//------------------------------------------------------------------------
void CActionMapManager::ReleaseActionIfActive(const ActionId& actionId)
{
	TActionMapMap::iterator it = m_actionMaps.begin();
	TActionMapMap::const_iterator end = m_actionMaps.end();
	for (; it != end; it++)
	{
		it->second->ReleaseActionIfActive(actionId);
	}
}
//------------------------------------------------------------------------
void CActionMapManager::AddBind(uint32 crc, const ActionId& actionId, uint32 keyNumber, CActionMap* pActionMap)
{
	SBindData bindData(actionId, keyNumber, pActionMap);

	// FIXME: This is bad ... will remove later
	while (RemoveBind(crc, actionId, keyNumber, pActionMap))
	{
		assert (false); // should really never happen!
	}

	m_inputCRCToBind.insert(std::pair<uint32, SBindData>(crc, bindData));
}

bool CActionMapManager::RemoveBind(uint32 crc, const ActionId& actionId, uint32 keyNumber, CActionMap* pActionMap)
{
	TInputCRCToBind::iterator iter = m_inputCRCToBind.lower_bound(crc);
	TInputCRCToBind::const_iterator end = m_inputCRCToBind.end();

	for (; iter != end && iter->first == crc; ++iter)
	{
		if (iter->second.actionId == actionId && iter->second.keyNumber == keyNumber && iter->second.pActionMap == pActionMap)
		{
			m_inputCRCToBind.erase(iter);
			return true;
		}
	}

	return false;
}
//------------------------------------------------------------------------

namespace
{
	template<typename T, class Derived>
	struct CGenericPtrMapIterator : public Derived
	{
		typedef typename T::mapped_type MappedType;
		typedef typename T::iterator    IterType;
		CGenericPtrMapIterator(T& theMap)
		{
			m_cur = theMap.begin();
			m_end = theMap.end();
		}
		virtual MappedType Next()
		{
			if (m_cur == m_end)
				return 0;
			MappedType& dt = m_cur->second;
			++m_cur;
			return dt;
		}
		virtual void AddRef()
		{
			++m_nRefs;
		}
		virtual void Release()
		{
			if (--m_nRefs <= 0)
				delete this;
		}
		IterType m_cur;
		IterType m_end;
		int m_nRefs;
	};
};

//------------------------------------------------------------------------
const Crc32Gen&	CActionMapManager::GetCrc32Gen() const
{	
	static Crc32Gen& rCrc32Inst = *GetISystem()->GetCrc32Gen();
	return rCrc32Inst;
}

//------------------------------------------------------------------------
IActionMapIteratorPtr CActionMapManager::CreateActionMapIterator()
{
	return new CGenericPtrMapIterator<TActionMapMap, IActionMapIterator> (m_actionMaps);
}

//------------------------------------------------------------------------
IActionFilterIteratorPtr CActionMapManager::CreateActionFilterIterator()
{
	return new CGenericPtrMapIterator<TActionFilterMap, IActionFilterIterator> (m_actionFilters);
}

void CActionMapManager::GetMemoryStatistics(ICrySizer * s)
{
	SIZER_SUBCOMPONENT_NAME(s, "ActionMapManager");
	s->Add(*this);
	s->AddContainer(m_actionMaps);
	s->AddContainer(m_actionFilters);

	for (TActionMapMap::iterator iter = m_actionMaps.begin(); iter != m_actionMaps.end(); ++iter)
	{
		s->Add(iter->first);
		iter->second->GetMemoryStatistics(s);
	}
	for (TActionFilterMap::iterator iter = m_actionFilters.begin(); iter != m_actionFilters.end(); ++iter)
	{
		s->Add(iter->first);
		iter->second->GetMemoryStatistics(s);
	}
}

#include UNIQUE_VIRTUAL_WRAPPER(IActionMapManager)