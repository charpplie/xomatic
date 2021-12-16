/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc environment browser control.
-------------------------------------------------------------------------
History:
- 01:01:2013: Created by Paul Slinger
*************************************************************************/

#include "StdAfx.h"

#include "Schematyc_EnvBrowserCtrl.h"

#include <Schematyc/Schematyc_IGlobalFunction.h>

#include "Schematyc_BrowserIcons.h"
#include "Schematyc_PluginUtils.h"

namespace Schematyc
{
	namespace
	{
		inline void EditCPPFile(const char* fileName)
		{
			CRY_ASSERT(fileName);
			if(fileName)
			{
				STARTUPINFO startupInfo;
				ZeroMemory(&startupInfo, sizeof(startupInfo));
				startupInfo.dwFlags			= STARTF_USESHOWWINDOW;
				startupInfo.wShowWindow	= SW_HIDE;

				PROCESS_INFORMATION	processInformation;
				ZeroMemory(&processInformation, sizeof(processInformation));

				char	currentDirectory[512];
				GetCurrentDirectory(sizeof(currentDirectory) - 1, currentDirectory);

				char	commandLine[1024] = "devenv /edit ";
				strcat_s(commandLine, currentDirectory);
				strcat_s(commandLine, "\\");
				strcat_s(commandLine, fileName);

				if(CreateProcess(NULL, commandLine, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInformation))
				{
					CloseHandle(processInformation.hThread);
					CloseHandle(processInformation.hProcess);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	CEnvBrowserCtrlItem::CEnvBrowserCtrlItem(const char* text, size_t icon, const CCustomTreeCtrlItemPtr& pParent, const SGUID& guid)
		: CCustomTreeCtrlItem(text, icon, pParent)
		, m_guid(guid)
	{}

	//////////////////////////////////////////////////////////////////////////
	const SGUID& CEnvBrowserCtrlItem::GetGUID() const
	{
		return m_guid;
	}

	//////////////////////////////////////////////////////////////////////////
	BEGIN_MESSAGE_MAP(CEnvBrowserCtrl, CCustomTreeCtrl)
		ON_NOTIFY_REFLECT(TVN_BEGINDRAG, OnBeginDrag)
		ON_WM_LBUTTONDBLCLK()
	END_MESSAGE_MAP()

	//////////////////////////////////////////////////////////////////////////
	BOOL CEnvBrowserCtrl::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
	{
		if(CCustomTreeCtrl::Create(dwStyle, rect, pParentWnd, nID))
		{
			Refresh();
			return true;
		}
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	void CEnvBrowserCtrl::OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult) 
	{
		if(CEnvBrowserCtrlItemPtr pItem = boost::static_pointer_cast<CEnvBrowserCtrlItem>(CCustomTreeCtrl::FindItem(reinterpret_cast<NM_TREEVIEW*>(pNMHDR)->ptDrag)))
		{
			switch(pItem->GetIcon())
			{
			case BrowserIcon::BRANCH:
			case BrowserIcon::FOR_LOOP:
			case BrowserIcon::RETURN:
			case BrowserIcon::ENV_SIGNAL:
			case BrowserIcon::ENV_GLOBAL_FUNCTION:
			case BrowserIcon::ENV_COMPONENT_FUNCTION:
			case BrowserIcon::ENV_ACTION_FUNCTION:
				{
					PluginUtils::BeginDragAndDrop(PluginUtils::SDragAndDropData(pItem->GetIcon(), pItem->GetGUID()));
					break;
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CEnvBrowserCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
	{
		if(CEnvBrowserCtrlItemPtr pItem = boost::static_pointer_cast<CEnvBrowserCtrlItem>(CCustomTreeCtrl::FindItem(point)))
		{
			switch(pItem->GetIcon())
			{
			case BrowserIcon::ENV_SIGNAL:
				{
					ISignalConstPtr	pSignal = GetSchematycFramework().GetEnvRegistry().GetSignal(pItem->GetGUID());
					CRY_ASSERT(pSignal != NULL);
					if(pSignal != NULL)
					{
						EditCPPFile(pSignal->GetFileName());
					}
					break;
				}
			case BrowserIcon::ENV_GLOBAL_FUNCTION:
				{
					IGlobalFunctionConstPtr	pFunction = GetSchematycFramework().GetEnvRegistry().GetGlobalFunction(pItem->GetGUID());
					CRY_ASSERT(pFunction != NULL);
					if(pFunction != NULL)
					{
						EditCPPFile(pFunction->GetFileName());
					}
					break;
				}
			case BrowserIcon::ENV_ABSTRACT_INTERFACE:
				{
					IAbstractInterfaceConstPtr	pInterface = GetSchematycFramework().GetEnvRegistry().GetAbstractInterface(pItem->GetGUID());
					CRY_ASSERT(pInterface != NULL);
					if(pInterface != NULL)
					{
						EditCPPFile(pInterface->GetFileName());
					}
					break;
				}
			case BrowserIcon::ENV_INTERFACE_FUNCTION:
				{
					// TODO : Traverse back up the tree hierarchy in order to find the interface this function belongs to?
					break;
				}
			case BrowserIcon::ENV_COMPONENT:
				{
					IComponentFactoryConstPtr	pComponentFactory = GetSchematycFramework().GetEnvRegistry().GetComponentFactory(pItem->GetGUID());
					CRY_ASSERT(pComponentFactory != NULL);
					if(pComponentFactory != NULL)
					{
						EditCPPFile(pComponentFactory->GetFileName());
					}
					break;
				}
			case BrowserIcon::ENV_COMPONENT_FUNCTION:
				{
					IComponentMemberFunctionConstPtr	pFunction = GetSchematycFramework().GetEnvRegistry().GetComponentMemberFunction(pItem->GetGUID());
					CRY_ASSERT(pFunction != NULL);
					if(pFunction != NULL)
					{
						EditCPPFile(pFunction->GetFileName());
					}
					break;
				}
			case BrowserIcon::ENV_ACTION:
				{
					IActionFactoryConstPtr	pActionFactory = GetSchematycFramework().GetEnvRegistry().GetActionFactory(pItem->GetGUID());
					CRY_ASSERT(pActionFactory != NULL);
					if(pActionFactory != NULL)
					{
						EditCPPFile(pActionFactory->GetFileName());
					}
					break;
				}
			case BrowserIcon::ENV_ACTION_FUNCTION:
				{
					IActionMemberFunctionConstPtr	pFunction = GetSchematycFramework().GetEnvRegistry().GetActionMemberFunction(pItem->GetGUID());
					CRY_ASSERT(pFunction != NULL);
					if(pFunction != NULL)
					{
						EditCPPFile(pFunction->GetFileName());
					}
					break;
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CEnvBrowserCtrl::LoadImageList(CImageList& imageList)
	{
		LoadBrowserIcons(imageList);
		SetImageList(&imageList, TVSIL_NORMAL);
	}

	//////////////////////////////////////////////////////////////////////////
	void CEnvBrowserCtrl::Refresh()
	{
		CCustomTreeCtrl::Reset();
		// Create root item.
		m_pRootItem = AddItem("...", BrowserIcon::FOLDER);
		// Visit environment registry and add elements to tree control.
		// N.B. We visit signals last to ensure owner items exist.
		IEnvRegistry&	envRegistry = GetSchematycFramework().GetEnvRegistry();
		envRegistry.VisitGlobalFunctions(MAKE_MEMBER_DELEGATE(CEnvBrowserCtrl::VisitGlobalFunction, *this));
		envRegistry.VisitAbstractInterfaces(MAKE_MEMBER_DELEGATE(CEnvBrowserCtrl::VisitAbstractInterface, *this));
		envRegistry.VisitComponentFactories(MAKE_MEMBER_DELEGATE(CEnvBrowserCtrl::VisitComponentFactory, *this));
		envRegistry.VisitComponentMemberFunctions(MAKE_MEMBER_DELEGATE(CEnvBrowserCtrl::VisitComponentMemberFunction, *this));
		envRegistry.VisitActionFactories(MAKE_MEMBER_DELEGATE(CEnvBrowserCtrl::VisitActionFactory, *this));
		envRegistry.VisitActionMemberFunctions(MAKE_MEMBER_DELEGATE(CEnvBrowserCtrl::VisitActionMemberFunction, *this));
		envRegistry.VisitSignals(MAKE_MEMBER_DELEGATE(CEnvBrowserCtrl::VisitSignal, *this));
	}

	//////////////////////////////////////////////////////////////////////////
	CEnvBrowserCtrlItemPtr CEnvBrowserCtrl::AddItem(const char* text, size_t icon, const CCustomTreeCtrlItemPtr& pParentItem, const SGUID& guid)
	{
		CEnvBrowserCtrlItemPtr	pItem(new CEnvBrowserCtrlItem(text, icon, pParentItem, guid));
		CCustomTreeCtrl::AddItem(pItem);
		return pItem;
	}

	//////////////////////////////////////////////////////////////////////////
	CEnvBrowserCtrlItemPtr CEnvBrowserCtrl::FindItem(const SGUID& guid)
	{
		TCustomTreeCtrlItemPtrVector&	items = CCustomTreeCtrl::GetItems();
		for(TCustomTreeCtrlItemPtrVector::iterator iItem = items.begin(), iEndItem = items.end(); iItem != iEndItem; ++ iItem)
		{
			CEnvBrowserCtrlItemPtr	pItem = boost::static_pointer_cast<CEnvBrowserCtrlItem>(*iItem);
			if(pItem->GetGUID() == guid)
			{
				return pItem;
			}
		}
		return CEnvBrowserCtrlItemPtr();
	}

	//////////////////////////////////////////////////////////////////////////
	CEnvBrowserCtrlItemPtr CEnvBrowserCtrl::GetOrCreateParentItem(const SGUID& ownerGUID, const char* scope)
	{
		if(ownerGUID.Empty() == false)
		{
			return FindItem(ownerGUID);
		}
		else
		{
			CCustomTreeCtrlItemPtr	pParentItem = m_pRootItem;
			if((scope != NULL) && (scope[0] != '\0'))
			{
				stack_string	tokens = scope;
				if(const size_t length = tokens.length())
				{
					int	pos = 0;
					do
					{
						stack_string						token = tokens.Tokenize("::", pos);
						CCustomTreeCtrlItemPtr	pItem = CCustomTreeCtrl::FindItem(token.c_str(), pParentItem);
						if(!pItem)
						{
							pItem = AddItem(token.c_str(), BrowserIcon::FOLDER, pParentItem);
						}
						pParentItem	= pItem;
					} while(pos < length);
				}
			}
			return boost::static_pointer_cast<CEnvBrowserCtrlItem>(pParentItem);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	VisitStatus::EValue CEnvBrowserCtrl::VisitSignal(const ISignalConstPtr& pSignal)
	{
		AddItem(pSignal->GetName(), BrowserIcon::ENV_SIGNAL, GetOrCreateParentItem(pSignal->GetSenderGUID(), pSignal->GetScope()), pSignal->GetGUID());
		return VisitStatus::CONTINUE;
	}

	//////////////////////////////////////////////////////////////////////////
	VisitStatus::EValue CEnvBrowserCtrl::VisitGlobalFunction(const IGlobalFunctionConstPtr& pFunction)
	{
		AddItem(pFunction->GetName(), BrowserIcon::ENV_GLOBAL_FUNCTION, GetOrCreateParentItem(SGUID(), pFunction->GetScope()), pFunction->GetGUID());
		return VisitStatus::CONTINUE;
	}

	//////////////////////////////////////////////////////////////////////////
	VisitStatus::EValue CEnvBrowserCtrl::VisitAbstractInterface(const IAbstractInterfaceConstPtr& pAbstractInterface)
	{
		CEnvBrowserCtrlItemPtr	pInterfaceItem = AddItem(pAbstractInterface->GetName(), BrowserIcon::ENV_ABSTRACT_INTERFACE, GetOrCreateParentItem(SGUID(), pAbstractInterface->GetScope()), pAbstractInterface->GetGUID());
		for(size_t iFunction = 0, functionCount = pAbstractInterface->GetFunctionCount(); iFunction < functionCount; ++ iFunction)
		{
			IAbstractInterfaceFunctionConstPtr	pFunction = pAbstractInterface->GetFunction(iFunction);
			AddItem(pFunction->GetName(), BrowserIcon::ENV_INTERFACE_FUNCTION, pInterfaceItem, pFunction->GetGUID());
		}
		return VisitStatus::CONTINUE;
	}

	//////////////////////////////////////////////////////////////////////////
	VisitStatus::EValue CEnvBrowserCtrl::VisitComponentFactory(const IComponentFactoryConstPtr& pComponentFactory)
	{
		AddItem(pComponentFactory->GetName(), BrowserIcon::ENV_COMPONENT, GetOrCreateParentItem(SGUID(), pComponentFactory->GetScope()), pComponentFactory->GetComponentGUID());
		return VisitStatus::CONTINUE;
	}

	//////////////////////////////////////////////////////////////////////////
	VisitStatus::EValue CEnvBrowserCtrl::VisitComponentMemberFunction(const IComponentMemberFunctionConstPtr& pFunction)
	{
		AddItem(pFunction->GetName(), BrowserIcon::ENV_COMPONENT_FUNCTION, GetOrCreateParentItem(pFunction->GetComponentGUID(), pFunction->GetScope()), pFunction->GetGUID());
		return VisitStatus::CONTINUE;
	}

	//////////////////////////////////////////////////////////////////////////
	VisitStatus::EValue CEnvBrowserCtrl::VisitActionFactory(const IActionFactoryConstPtr& pActionFactory)
	{
		AddItem(pActionFactory->GetName(), BrowserIcon::ENV_ACTION, GetOrCreateParentItem(pActionFactory->GetComponentGUID(), pActionFactory->GetScope()), pActionFactory->GetActionGUID());
		return VisitStatus::CONTINUE;
	}

	//////////////////////////////////////////////////////////////////////////
	VisitStatus::EValue CEnvBrowserCtrl::VisitActionMemberFunction(const IActionMemberFunctionConstPtr& pFunction)
	{
		AddItem(pFunction->GetName(), BrowserIcon::ENV_ACTION_FUNCTION, GetOrCreateParentItem(pFunction->GetActionGUID(), pFunction->GetScope()), pFunction->GetGUID());
		return VisitStatus::CONTINUE;
	}
}