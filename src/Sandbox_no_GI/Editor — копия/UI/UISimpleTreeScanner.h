////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UISimpleTreeScanner.h
//  Version:     v1.00
//  Created:     11/10/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////

#ifndef __UISimpleTreeScanner_H__
#define __UISimpleTreeScanner_H__

#include "PanelSimpleTreeBrowser.h"
#include <IFlashUI.h>

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
struct SUIBaseScanner : public ISimpleTreeBrowserScanner
{
	virtual bool HasNewFiles() const
	{
		return true;
	}

	virtual void GetScannedFiles( TSimpleTreeBrowserItems& items ) const
	{
		for (TSimpleTreeBrowserItems::const_iterator it = m_ScannedItems.begin(); it != m_ScannedItems.end(); ++it)
		{
			items.push_back(*it);
		}
		m_ScannedItems.clear();
	}

	virtual bool IsScanningForFiles() const
	{
		return false;
	}

protected:
	void AddItem(const SSimpleTreeBrowserItem& item)
	{
		m_ScannedItems.push_back(item);
	}

private:
	mutable TSimpleTreeBrowserItems m_ScannedItems;
};

//////////////////////////////////////////////////////////////////////////
struct SUIParameterTooltipHelper
{
	static void AddParamsTooltip(string& tooltip, const SUIEventDesc::SEvtParams& params, const char* header)
	{
		if (params.Params.size() > 0 || params.IsDynamic)
		{
			tooltip += string().Format("\n\n%s:", header);
			for (int k = 0; k < params.Params.size(); ++k)
			{
				string desc;
				if ( strlen(params.Params[k].sDesc) > 0)
					desc.Format(" (%s)", params.Params[k].sDesc);
				tooltip += string().Format("\n[%s]%s", params.Params[k].sDisplayName, desc);
			}
			if (params.IsDynamic)
			{
				string desc;
				if ( strlen(params.sDynamicDesc) > 0)
					desc.Format(" (%s)", params.sDynamicDesc);
				tooltip += string().Format("\n[%s]%s", params.sDynamicName, desc);
			}
			tooltip.replace("\n\n\n", "\n\n");
		}
	}
};
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
struct SUIElementScanner : public SUIBaseScanner
{
	virtual void Scan()
	{
		if (gEnv->pFlashUI)
		{
			const int count = gEnv->pFlashUI->GetUIElementCount();
			for (int i = 0; i < count; ++i)
			{
				IUIElement* pElement = gEnv->pFlashUI->GetUIElement(i);
				IUIElementIteratorPtr instances = pElement->GetInstances();

				SSimpleTreeBrowserItem item;
				item.UserData = pElement;
				item.DisplayName = pElement->GetName();

				while (IUIElement* pInstance = instances->Next())
				{
					if (pInstance == pElement) continue;

					SSimpleTreeBrowserItem instance;
					instance.UserData = pInstance;
					instance.DisplayName.Format("%s @%i%s", pElement->GetName(), pInstance->GetInstanceID(), pInstance->IsValid() ? "" : " [ERROR]");
					item.Children.push_back(instance);
				}

				AddItem(item);
			}
		}
	}
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
struct SUIElementBaseScanner : public SUIBaseScanner
{
	SUIElementBaseScanner() : m_pElement(NULL)
	{
	}

	void SetElement(IUIElement* pElement)
	{
		m_pElement = pElement;
	}

protected:
	IUIElement* m_pElement;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
struct SUIElementScannerHelperEvents
{
	SUIElementScannerHelperEvents(IUIElement* pElement) : m_pElement(pElement) {}
	int GetCount() { return m_pElement->GetEventCount(); }
	const SUIEventDesc* GetDesc( int index ) { return m_pElement->GetEventDesc(index); }
	const char* GetCommandStr() const { return "fscommand"; }
	const char* GetHeaderStr() const { return "ReturnValues"; }
private:
	IUIElement* m_pElement;
};

//////////////////////////////////////////////////////////////////////////
struct SUIElementScannerHelperFunctions
{
	SUIElementScannerHelperFunctions(IUIElement* pElement) : m_pElement(pElement) {}
	int GetCount() { return m_pElement->GetFunctionCount(); }
	const SUIEventDesc* GetDesc( int index ) { return m_pElement->GetFunctionDesc(index); }
	const char* GetCommandStr() const { return "function"; }
	const char* GetHeaderStr() const { return "Arguments"; }
private:
	IUIElement* m_pElement;
};


//////////////////////////////////////////////////////////////////////////
template< class T >
struct SUIElementEventScanner : public SUIElementBaseScanner
{
	virtual void Scan()
	{
		if (m_pElement)
		{
			T helper(m_pElement);
			const int count = helper.GetCount();
			for (int i = 0; i < count; ++i)
			{
				const SUIEventDesc* pDesc = helper.GetDesc(i);
				SSimpleTreeBrowserItem item;
				item.UserData = m_pElement;
				item.DisplayName = pDesc->sDisplayName;

				string tooltip;
				tooltip.Format("%s (%s: [%s])\n%s", pDesc->sDisplayName, helper.GetCommandStr(), pDesc->sName, pDesc->sDesc);
				SUIParameterTooltipHelper::AddParamsTooltip(tooltip, pDesc->InputParams, helper.GetHeaderStr());
				item.Tooltip = tooltip.c_str();

				AddItem(item);
			}
		}
	}
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
struct SUIElementScannerHelperVariables
{
	SUIElementScannerHelperVariables(IUIElement* pElement) : m_pElement(pElement) {}
	int GetCount() { return m_pElement->GetVariableCount(); }
	const SUIParameterDesc* GetDesc( int index ) { return m_pElement->GetVariableDesc(index); }
private:
	IUIElement* m_pElement;
};

//////////////////////////////////////////////////////////////////////////
struct SUIElementScannerHelperMovieClips
{
	SUIElementScannerHelperMovieClips(IUIElement* pElement) : m_pElement(pElement) {}
	int GetCount() { return m_pElement->GetMovieClipCount(); }
	const SUIParameterDesc* GetDesc( int index ) { return m_pElement->GetMovieClipDesc(index); }
private:
	IUIElement* m_pElement;
};

//////////////////////////////////////////////////////////////////////////
struct SUIElementScannerHelperArrays
{
	SUIElementScannerHelperArrays(IUIElement* pElement) : m_pElement(pElement) {}
	int GetCount() { return m_pElement->GetArrayCount(); }
	const SUIParameterDesc* GetDesc( int index ) { return m_pElement->GetArrayDesc(index); }
private:
	IUIElement* m_pElement;
};

//////////////////////////////////////////////////////////////////////////
template< class T >
struct SUIElementItemScanner : public SUIElementBaseScanner
{
	virtual void Scan()
	{
		if (m_pElement)
		{
			T helper(m_pElement);
			const int count = helper.GetCount();
			for (int i = 0; i < count; ++i)
			{
				const SUIParameterDesc* pDesc = helper.GetDesc(i);
				SSimpleTreeBrowserItem item;
				item.UserData = m_pElement;
				item.DisplayName = pDesc->sDisplayName;
				item.Tooltip.Format("Instance: %s\nDescription: %s", pDesc->sName, pDesc->sDesc);
				AddItem(item);
			}
		}
	}
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
struct SUIActionScanner : public SUIBaseScanner
{
	virtual void Scan()
	{
		if (gEnv->pFlashUI)
		{
			const int count = gEnv->pFlashUI->GetUIActionCount();
			for (int i = 0; i < count; ++i)
			{
				IUIAction* pAction = gEnv->pFlashUI->GetUIAction(i);
				SSimpleTreeBrowserItem item;
				item.UserData = pAction;
				item.DisplayName = pAction->GetName();
				item.Tooltip.Format("Libs/UI/UIActions/%s.xml", pAction->GetName());
				AddItem(item);
			}
		}
	}
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
struct SUIEventSystemScanner : public SUIBaseScanner
{
	SUIEventSystemScanner(IUIEventSystem::EEventSystemType type) : m_type(type)
	{
	}

	virtual void Scan()
	{
		if (gEnv->pFlashUI)
		{
			IUIEventSystemIteratorPtr eventSystems = gEnv->pFlashUI->CreateEventSystemIterator(m_type);
			string name;
			while (IUIEventSystem* pEventSystem = eventSystems->Next(name))
			{
				SSimpleTreeBrowserItem item;
				item.UserData = pEventSystem;
				item.DisplayName = name;
				AddItem(item);
			}
		}
	}
private:
	IUIEventSystem::EEventSystemType m_type;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
struct SUIEventSystemEventsScanner : public SUIBaseScanner
{
	SUIEventSystemEventsScanner() : m_pEventSystem(NULL)
	{
	}

	void SetEventSystem(IUIEventSystem* pEventSystem)
	{
		m_pEventSystem = pEventSystem;
	}

	virtual void Scan()
	{
		if (m_pEventSystem)
		{
			const int count = m_pEventSystem->GetEventCount();
			for (int i = 0; i < count; ++i)
			{
				const SUIEventDesc* pDesc = m_pEventSystem->GetEventDesc(i);
				SSimpleTreeBrowserItem item;
				item.UserData = m_pEventSystem;
				item.DisplayName = pDesc->sDisplayName;

				string tooltip;
				tooltip.Format("%s\n%s", pDesc->sDisplayName, pDesc->sDesc);
				SUIParameterTooltipHelper::AddParamsTooltip(tooltip, pDesc->InputParams, "Arguments");
				SUIParameterTooltipHelper::AddParamsTooltip(tooltip, pDesc->OutputParams, "ReturnValues");
				item.Tooltip = tooltip.c_str();

				AddItem(item);
			}
		}
	}
private:
	IUIEventSystem* m_pEventSystem;
};


#endif // __UISimpleTreeScanner_H__