/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc browser control.
-------------------------------------------------------------------------
History:
- 01:01:2013: Created by Paul Slinger
*************************************************************************/

#include "StdAfx.h"

#include "Schematyc_BrowserCtrl.h"

#include <Schematyc/Schematyc_IDocManager.h>

#include "Schematyc_AddAbstractInterfaceDlg.h"
#include "Schematyc_AddActionDlg.h"
#include "Schematyc_AddComponentDlg.h"
#include "Schematyc_AddContainerDlg.h"
#include "Schematyc_AddGraphDlg.h"
#include "Schematyc_AddGroupDlg.h"
#include "Schematyc_AddStateDlg.h"
#include "Schematyc_AddStateMachineDlg.h"
#include "Schematyc_AddTimerDlg.h"
#include "Schematyc_AddVariableDlg.h"
#include "Schematyc_BrowserIcons.h"
#include "Schematyc_CreateDocDlg.h"
#include "Schematyc_CreateEnumerationDlg.h"
#include "Schematyc_CreateSchemaDlg.h"
#include "Schematyc_CreateSignalDlg.h"
#include "Schematyc_CreateStructureDlg.h"
#include "Schematyc_PluginUtils.h"

namespace Schematyc
{
	namespace
	{
		// TODO : Can/should we merge SDocVisitor with the browser control itself?
		struct SDocVisitor
		{
			inline SDocVisitor(CBrowserCtrl& _browserCtrl, const CBrowserCtrlItemPtr& _pParentItem)
				: browserCtrl(_browserCtrl)
				, pParentItem(_pParentItem)
			{}

			inline VisitStatus::EValue VisitGroup(const IDocGroupConstPtr& pGroup) const
			{
				browserCtrl.AddItem(pGroup->GetName(), BrowserIcon::USER_GROUP, pParentItem, pGroup->GetGUID());
				return VisitStatus::CONTINUE;
			}

			inline VisitStatus::EValue VisitEnumeration(const IDocEnumerationConstPtr& pEnumeration) const
			{
				browserCtrl.AddItem(pEnumeration->GetName(), BrowserIcon::ENUMERATION, pParentItem, pEnumeration->GetGUID());
				return VisitStatus::CONTINUE;
			}

			inline VisitStatus::EValue VisitStructure(const IDocStructureConstPtr& pStructure) const
			{
				browserCtrl.AddItem(pStructure->GetName(), BrowserIcon::STRUCTURE, pParentItem, pStructure->GetGUID());
				return VisitStatus::CONTINUE;
			}

			inline VisitStatus::EValue VisitSignal(const IDocSignalConstPtr& pSignal) const
			{
				browserCtrl.AddItem(pSignal->GetName(), BrowserIcon::SIGNAL, pParentItem, pSignal->GetGUID());
				return VisitStatus::CONTINUE;
			}

			inline VisitStatus::EValue VisitSchema(const IDocSchemaConstPtr& pSchema) const
			{
				browserCtrl.AddItem(pSchema->GetName(), BrowserIcon::SCHEMA, pParentItem, pSchema->GetGUID());
				return VisitStatus::CONTINUE;
			}

			inline VisitStatus::EValue VisitStateMachine(const IDocStateMachineConstPtr& pStateMachine) const
			{
				browserCtrl.AddItem(pStateMachine->GetName(), BrowserIcon::STATE_MACHINE, pParentItem, pStateMachine->GetGUID());
				return VisitStatus::CONTINUE;
			}

			inline VisitStatus::EValue VisitState(const IDocStateConstPtr& pState) const
			{
				browserCtrl.AddItem(pState->GetName(), BrowserIcon::STATE, pParentItem, pState->GetGUID());
				return VisitStatus::CONTINUE;
			}

			inline VisitStatus::EValue VisitVariable(const IDocVariableConstPtr& pVariable) const
			{
				browserCtrl.AddItem(pVariable->GetName(), BrowserIcon::VARIABLE, pParentItem, pVariable->GetGUID());
				return VisitStatus::CONTINUE;
			}

			inline VisitStatus::EValue VisitContainer(const IDocContainerConstPtr& pContainer) const
			{
				browserCtrl.AddItem(pContainer->GetName(), BrowserIcon::CONTAINER, pParentItem, pContainer->GetGUID());
				return VisitStatus::CONTINUE;
			}

			inline VisitStatus::EValue VisitTimer(const IDocTimerConstPtr& pTimer) const
			{
				browserCtrl.AddItem(pTimer->GetName(), BrowserIcon::TIMER, pParentItem, pTimer->GetGUID());
				return VisitStatus::CONTINUE;
			}

			inline VisitStatus::EValue VisitAbstractInterfaceInstance(const IDocAbstractInterfaceInstanceConstPtr& pAbstractInterfaceInstance) const
			{
				if((pAbstractInterfaceInstance->GetFlags() & DocAbstractInterfaceInstanceFlags::DEPRECATED) == 0)
				{
					IAbstractInterfaceConstPtr	pAbstractInterface = GetSchematycFramework().GetEnvRegistry().GetAbstractInterface(pAbstractInterfaceInstance->GetAbstractInterfaceGUID());
					CRY_ASSERT(pAbstractInterface != NULL);
					if(pAbstractInterface != NULL)
					{
						browserCtrl.AddItem(pAbstractInterfaceInstance->GetName(), BrowserIcon::ENV_ABSTRACT_INTERFACE, pParentItem, pAbstractInterfaceInstance->GetGUID());
					}
				}
				else
				{
					stack_string	name = pAbstractInterfaceInstance->GetName();
					name.append(" [DEPRECATED]");
					browserCtrl.AddItem(name.c_str(), BrowserIcon::ENV_ABSTRACT_INTERFACE, pParentItem, pAbstractInterfaceInstance->GetGUID());
				}
				return VisitStatus::CONTINUE;
			}

			inline VisitStatus::EValue VisitComponentInstance(const IDocComponentInstanceConstPtr& pComponentInstance) const
			{
				browserCtrl.AddItem(pComponentInstance->GetName(), BrowserIcon::COMPONENT, pParentItem, pComponentInstance->GetGUID());
				return VisitStatus::CONTINUE;
			}

			inline VisitStatus::EValue VisitActionInstance(const IDocActionInstanceConstPtr& pActionInstance) const
			{
				browserCtrl.AddItem(pActionInstance->GetName(), BrowserIcon::ACTION_INSTANCE, pParentItem, pActionInstance->GetGUID());
				return VisitStatus::CONTINUE;
			}

			inline VisitStatus::EValue VisitGraph(const IDocGraphConstPtr& pGraph) const
			{
				size_t	icon = BrowserIcon::GRAPH;
				switch(pGraph->GetType())
				{
				case DocGraphType::ABSTRACT_INTERFACE_FUNCTION:
				case DocGraphType::FUNCTION:
					{
						icon = BrowserIcon::FUNCTION_GRAPH;
						break;
					}
				case DocGraphType::CONDITION:
					{
						icon = BrowserIcon::CONDITION_GRAPH;
						break;
					}
				case DocGraphType::CONSTRUCTOR:
					{
						icon = BrowserIcon::CONSTRUCTOR;
						break;
					}
				case DocGraphType::DESTRUCTOR:
					{
						icon = BrowserIcon::DESTRUCTOR;
						break;
					}
				case DocGraphType::SIGNAL_RECEIVER:
					{
						icon = BrowserIcon::SIGNAL_RECEIVER;
						break;
					}
				}
				browserCtrl.AddItem(pGraph->GetName(), icon, pParentItem, pGraph->GetGUID());
				return VisitStatus::CONTINUE;
			}

			CBrowserCtrl&				browserCtrl;
			CBrowserCtrlItemPtr	pParentItem;
		};

		struct SSettingsVisitor
		{
			inline SSettingsVisitor(CBrowserCtrl& _browserCtrl, const CBrowserCtrlItemPtr& _pParentItem)
				: browserCtrl(_browserCtrl)
				, pParentItem(_pParentItem)
			{}

			inline VisitStatus::EValue VisitSettings(const char* name, const IEnvSettingsPtr&) const
			{
				browserCtrl.AddItem(name, BrowserIcon::SETTINGS_DOC, pParentItem, SGUID());
				return VisitStatus::CONTINUE;
			}

			CBrowserCtrl&				browserCtrl;
			CBrowserCtrlItemPtr	pParentItem;
		};

		//////////////////////////////////////////////////////////////////////////
		inline void GetItemPath(const CBrowserCtrlItem& item, stack_string& path, bool includeGameFolder)
		{
			path.clear();
			if(includeGameFolder == true)
			{
				path.append(gEnv->pCryPak->GetGameFolder());
				path.append("/");
			}
			TStringVector	folderNames;
			for(CCustomTreeCtrlItemConstPtr pParentItem = item.shared_from_this(); pParentItem != NULL; pParentItem = pParentItem->GetParent())
			{
				switch(pParentItem->GetIcon())
				{
				case BrowserIcon::FOLDER:
					{
						if(pParentItem->GetParent() != NULL)
						{
							folderNames.push_back(pParentItem->GetText());
						}
						else
						{
							folderNames.push_back(GetSchematycFramework().GetDocFolder());
						}
						break;
					}
				case BrowserIcon::SETTINGS_FOLDER:
					{
						folderNames.push_back(GetSchematycFramework().GetSettingsFolder());
						break;
					}
				default:
					{
						folderNames.push_back(pParentItem->GetText());
					}
				}
			}
			/////
			std::reverse(folderNames.begin(), folderNames.end());
			for(TStringVector::const_iterator iFolderName = folderNames.begin(), iEndFolderName = folderNames.end(); iFolderName != iEndFolderName; ++ iFolderName)
			{
				path.append(iFolderName->c_str());
				path.append("/");
			}
			path.TrimRight("/");
		}

		//////////////////////////////////////////////////////////////////////////
		inline CBrowserCtrlItemPtr GetRealParentItem(const CBrowserCtrlItemPtr& pItem)
		{
			for(CBrowserCtrlItemPtr pParentItem = pItem ? boost::static_pointer_cast<CBrowserCtrlItem>(pItem->GetParent()) : CBrowserCtrlItemPtr(); pParentItem; pParentItem = boost::static_pointer_cast<CBrowserCtrlItem>(pParentItem->GetParent()))
			{
				if(pParentItem->GetIcon() != BrowserIcon::USER_GROUP)
				{
					return pParentItem;
				}
			}
			return CBrowserCtrlItemPtr();
		}

		//////////////////////////////////////////////////////////////////////////
		inline void ShowInExplorer(const char* path)
		{
			CRY_ASSERT(path != NULL);
			if(path != NULL)
			{
				char	currentDirectory[512];
				GetCurrentDirectory(sizeof(currentDirectory) - 1, currentDirectory);

				stack_string	fullPath = currentDirectory;
				fullPath.append("\\");
				fullPath.append(path);
				fullPath.replace("/", "\\");

				ShellExecute(NULL, "open", fullPath.c_str(), NULL, NULL, SW_SHOWDEFAULT);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	CBrowserCtrlItem::CBrowserCtrlItem(const char* text, size_t icon, const CCustomTreeCtrlItemPtr& pParent, const SGUID& guid)
		: CCustomTreeCtrlItem(text, icon, pParent)
		, m_guid(guid)
	{}

	//////////////////////////////////////////////////////////////////////////
	const SGUID& CBrowserCtrlItem::GetGUID() const
	{
		return m_guid;
	}

	//////////////////////////////////////////////////////////////////////////
	BEGIN_MESSAGE_MAP(CBrowserCtrl, CCustomTreeCtrl)
		ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelChanged)
		ON_NOTIFY_REFLECT(TVN_BEGINDRAG, OnBeginDrag)
		ON_WM_LBUTTONDBLCLK()
		ON_NOTIFY_REFLECT(NM_RCLICK, OnRClick)
		ON_WM_CONTEXTMENU()
	END_MESSAGE_MAP()

	//////////////////////////////////////////////////////////////////////////
	BOOL CBrowserCtrl::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
	{
		if(CCustomTreeCtrl::Create(dwStyle, rect, pParentWnd, nID) == TRUE)
		{
			GetSchematycFramework().GetDocManager().Load();
			AddItem("Documents", BrowserIcon::FOLDER);
			AddItem("Settings", BrowserIcon::SETTINGS_FOLDER);
			return true;
		}
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	CBrowserCtrlItemPtr CBrowserCtrl::AddItem(const char* text, size_t icon, const CBrowserCtrlItemPtr& pParentItem, const SGUID& guid)
	{
		CBrowserCtrlItemPtr	pItem(new CBrowserCtrlItem(text, icon, pParentItem, guid));
		CCustomTreeCtrl::AddItem(pItem);
		RefreshItem(pItem);
		return pItem;
	}

	//////////////////////////////////////////////////////////////////////////
	CBrowserCtrlItemPtr CBrowserCtrl::GetSelectedItem()
	{
		return boost::static_pointer_cast<CBrowserCtrlItem>(CCustomTreeCtrl::FindItem(CCustomTreeCtrl::GetSelectedItem()));
	}

	//////////////////////////////////////////////////////////////////////////
	IDoc* CBrowserCtrl::GetItemDoc(const CBrowserCtrlItem& item) const
	{
		if(item.GetIcon() == BrowserIcon::DOC)
		{
			stack_string	docFileName;
			GetItemPath(item, docFileName, false);
			return GetSchematycFramework().GetDocManager().GetDoc(docFileName.c_str());
		}
		else
		{
			CBrowserCtrlItemPtr	pDocItem = boost::static_pointer_cast<CBrowserCtrlItem>(item.FindAncestor(BrowserIcon::DOC));
			CRY_ASSERT(pDocItem != NULL);
			if(pDocItem != NULL)
			{
				stack_string	docFileName;
				GetItemPath(*pDocItem, docFileName, false);
				return GetSchematycFramework().GetDocManager().GetDoc(docFileName.c_str());
			}
		}
		return NULL;
	}

	//////////////////////////////////////////////////////////////////////////
	IDocElementPtr CBrowserCtrl::GetItemDocElement(const CBrowserCtrlItem& item) const
	{
		IDoc*	pDoc = GetItemDoc(item);
		CRY_ASSERT(pDoc != NULL);
		if(pDoc != NULL)
		{
			return pDoc->GetElement(item.GetGUID());
		}
		return IDocElementPtr();
	}

	//////////////////////////////////////////////////////////////////////////
	IDocGroupPtr CBrowserCtrl::GetItemDocGroup(const CBrowserCtrlItem& item) const
	{
		IDoc*	pDoc = GetItemDoc(item);
		CRY_ASSERT(pDoc != NULL);
		if(pDoc != NULL)
		{
			return pDoc->GetGroup(item.GetGUID());
		}
		return IDocGroupPtr();
	}

	//////////////////////////////////////////////////////////////////////////
	IDocEnumerationPtr CBrowserCtrl::GetItemDocEnumeration(const CBrowserCtrlItem& item) const
	{
		IDoc*	pDoc = GetItemDoc(item);
		CRY_ASSERT(pDoc != NULL);
		if(pDoc != NULL)
		{
			return pDoc->GetEnumeration(item.GetGUID());
		}
		return IDocEnumerationPtr();
	}

	//////////////////////////////////////////////////////////////////////////
	IDocStructurePtr CBrowserCtrl::GetItemDocStructure(const CBrowserCtrlItem& item) const
	{
		IDoc*	pDoc = GetItemDoc(item);
		CRY_ASSERT(pDoc != NULL);
		if(pDoc != NULL)
		{
			return pDoc->GetStructure(item.GetGUID());
		}
		return IDocStructurePtr();
	}

	//////////////////////////////////////////////////////////////////////////
	IDocSignalPtr CBrowserCtrl::GetItemDocSignal(const CBrowserCtrlItem& item) const
	{
		IDoc*	pDoc = GetItemDoc(item);
		CRY_ASSERT(pDoc != NULL);
		if(pDoc != NULL)
		{
			return pDoc->GetSignal(item.GetGUID());
		}
		return NULL;
	}

	//////////////////////////////////////////////////////////////////////////
	IDocSchemaPtr CBrowserCtrl::GetItemDocSchema(const CBrowserCtrlItem& item) const
	{
		IDoc*	pDoc = GetItemDoc(item);
		CRY_ASSERT(pDoc != NULL);
		if(pDoc != NULL)
		{
			if(item.GetIcon() == BrowserIcon::SCHEMA)
			{
				return pDoc->GetSchema(item.GetGUID());
			}
			else
			{
				CBrowserCtrlItemPtr	pDocSchemaItem = boost::static_pointer_cast<CBrowserCtrlItem>(item.FindAncestor(BrowserIcon::SCHEMA));
				CRY_ASSERT(pDocSchemaItem != NULL);
				if(pDocSchemaItem != NULL)
				{
					return pDoc->GetSchema(pDocSchemaItem->GetGUID());
				}
			}
		}
		return NULL;
	}

	//////////////////////////////////////////////////////////////////////////
	IDocStateMachinePtr CBrowserCtrl::GetItemDocStateMachine(const CBrowserCtrlItem& item) const
	{
		IDoc*	pDoc = GetItemDoc(item);
		CRY_ASSERT(pDoc != NULL);
		if(pDoc != NULL)
		{
			return pDoc->GetStateMachine(item.GetGUID());
		}
		return NULL;
	}

	//////////////////////////////////////////////////////////////////////////
	IDocStatePtr CBrowserCtrl::GetItemDocState(const CBrowserCtrlItem& item) const
	{
		IDoc*	pDoc = GetItemDoc(item);
		CRY_ASSERT(pDoc != NULL);
		if(pDoc != NULL)
		{
			return pDoc->GetState(item.GetGUID());
		}
		return NULL;
	}

	//////////////////////////////////////////////////////////////////////////
	IDocVariablePtr CBrowserCtrl::GetItemDocVariable(const CBrowserCtrlItem& item) const
	{
		IDoc*	pDoc = GetItemDoc(item);
		CRY_ASSERT(pDoc != NULL);
		if(pDoc != NULL)
		{
			return pDoc->GetVariable(item.GetGUID());
		}
		return NULL;
	}

	//////////////////////////////////////////////////////////////////////////
	IDocContainerPtr CBrowserCtrl::GetItemDocContainer(const CBrowserCtrlItem& item) const
	{
		IDoc*	pDoc = GetItemDoc(item);
		CRY_ASSERT(pDoc != NULL);
		if(pDoc != NULL)
		{
			return pDoc->GetContainer(item.GetGUID());
		}
		return NULL;
	}

	//////////////////////////////////////////////////////////////////////////
	IDocTimerPtr CBrowserCtrl::GetItemDocTimer(const CBrowserCtrlItem& item) const
	{
		IDoc*	pDoc = GetItemDoc(item);
		CRY_ASSERT(pDoc != NULL);
		if(pDoc != NULL)
		{
			return pDoc->GetTimer(item.GetGUID());
		}
		return NULL;
	}

	//////////////////////////////////////////////////////////////////////////
	IDocAbstractInterfaceInstancePtr CBrowserCtrl::GetItemDocAbstractInterfaceInstance(const CBrowserCtrlItem& item) const
	{
		IDoc*	pDoc = GetItemDoc(item);
		CRY_ASSERT(pDoc != NULL);
		if(pDoc != NULL)
		{
			return pDoc->GetAbstractInterfaceInstance(item.GetGUID());
		}
		return NULL;
	}

	//////////////////////////////////////////////////////////////////////////
	IDocComponentInstancePtr CBrowserCtrl::GetItemDocComponentInstance(const CBrowserCtrlItem& item) const
	{
		IDoc*	pDoc = GetItemDoc(item);
		CRY_ASSERT(pDoc != NULL);
		if(pDoc != NULL)
		{
			return pDoc->GetComponentInstance(item.GetGUID());
		}
		return NULL;
	}

	//////////////////////////////////////////////////////////////////////////
	IDocActionInstancePtr CBrowserCtrl::GetItemDocActionInstance(const CBrowserCtrlItem& item) const
	{
		IDoc*	pDoc = GetItemDoc(item);
		CRY_ASSERT(pDoc != NULL);
		if(pDoc != NULL)
		{
			return pDoc->GetActionInstance(item.GetGUID());
		}
		return NULL;
	}

	//////////////////////////////////////////////////////////////////////////
	IDocGraphPtr CBrowserCtrl::GetItemDocGraph(const CBrowserCtrlItem& item) const
	{
		IDoc*	pDoc = GetItemDoc(item);
		if(pDoc != NULL)
		{
			return pDoc->GetGraph(item.GetGUID());
		}
		return NULL;
	}

	//////////////////////////////////////////////////////////////////////////
	CBrowserCtrl::TSelectionSignal& CBrowserCtrl::GetSelectionSignal()
	{
		return m_signals.selection;
	}

	//////////////////////////////////////////////////////////////////////////
	CBrowserCtrl::TDoubleClickSignal& CBrowserCtrl::GetDoubleClickSignal()
	{
		return m_signals.doubleClick;
	}

	//////////////////////////////////////////////////////////////////////////
	CBrowserCtrl::TDocModifiedSignal& CBrowserCtrl::GetDocModifiedSignal()
	{
		return m_signals.docModified;
	}

	//////////////////////////////////////////////////////////////////////////
	CBrowserCtrl::TDocGraphRemovedSignal& CBrowserCtrl::GetDocGraphRemovedSignal()
	{
		return m_signals.docGraphRemoved;
	}

	//////////////////////////////////////////////////////////////////////////
	CBrowserCtrl::CContextMenu::CContextMenu()
	{
		m_menu.CreatePopupMenu();
	}

	//////////////////////////////////////////////////////////////////////////
	CBrowserCtrl::CContextMenu::~CContextMenu()
	{
		for(TSubMenuVector::iterator iSubMenu = m_subMenus.begin(), iEndSubMenu = m_subMenus.end(); iSubMenu != iEndSubMenu; ++ iSubMenu)
		{
			delete (*iSubMenu);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	CMenu& CBrowserCtrl::CContextMenu::GetMenu()
	{
		return m_menu;
	}

	//////////////////////////////////////////////////////////////////////////
	CBrowserCtrl::CContextMenu* CBrowserCtrl::CContextMenu::CreateSubMenu(const char* name)
	{
		CContextMenu*	pSubMenu = new CContextMenu();
		m_subMenus.push_back(pSubMenu);
		m_menu.AppendMenu(MF_POPUP, UINT_PTR(pSubMenu->m_menu.GetSafeHmenu()), name);
		return pSubMenu;
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnSelChanged(NMHDR* pNMHDR, LRESULT* pResult)
	{
		CBrowserCtrlItemPtr	pItem = boost::static_pointer_cast<CBrowserCtrlItem>(CCustomTreeCtrl::FindItem(reinterpret_cast<NM_TREEVIEW*>(pNMHDR)->itemNew.hItem));
		if(pItem != NULL)
		{
			m_signals.selection.Send(pItem);
		}
		else
		{
			m_signals.selection.Send(CBrowserCtrlItemPtr());
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult) 
	{
		CBrowserCtrlItemPtr	pItem = boost::static_pointer_cast<CBrowserCtrlItem>(CCustomTreeCtrl::FindItem(reinterpret_cast<NM_TREEVIEW*>(pNMHDR)->ptDrag));
		if(pItem != NULL)
		{
			switch(pItem->GetIcon())
			{
			case BrowserIcon::SIGNAL:
			case BrowserIcon::SCHEMA:
			case BrowserIcon::STATE:
			case BrowserIcon::TIMER:
			case BrowserIcon::COMPONENT:
			case BrowserIcon::ACTION_INSTANCE:
			case BrowserIcon::GRAPH:
			case BrowserIcon::SIGNAL_RECEIVER:
			case BrowserIcon::CONSTRUCTOR:
			case BrowserIcon::DESTRUCTOR:
			case BrowserIcon::FUNCTION_GRAPH:
			case BrowserIcon::CONDITION_GRAPH:
				{
					if(pItem->FindAncestor(BrowserIcon::SIGNAL) == false)
					{
						PluginUtils::BeginDragAndDrop(PluginUtils::SDragAndDropData(pItem->GetIcon(), pItem->GetGUID()));
					}
					break;
				}
			case BrowserIcon::VARIABLE:
				{
					if(pItem->FindAncestor(BrowserIcon::SIGNAL) == false)
					{
						PluginUtils::BeginDragAndDrop(PluginUtils::SDragAndDropData(pItem->GetIcon(), pItem->GetGUID()));
					}
					break;
				}
			case BrowserIcon::CONTAINER:
				{
					if(pItem->FindAncestor(BrowserIcon::SIGNAL) == false)
					{
						PluginUtils::BeginDragAndDrop(PluginUtils::SDragAndDropData(pItem->GetIcon(), pItem->GetGUID()));
					}
					break;
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
	{
		CBrowserCtrlItemPtr	pItem = boost::static_pointer_cast<CBrowserCtrlItem>(CCustomTreeCtrl::FindItem(point));
		if(pItem != NULL)
		{
			size_t							icon = pItem->GetIcon();
			CBrowserCtrlItemPtr	pDocItem = ((icon == BrowserIcon::FOLDER) || (icon == BrowserIcon::DOC)) ? pItem : boost::static_pointer_cast<CBrowserCtrlItem>(pItem->FindAncestor(BrowserIcon::DOC));
			CRY_ASSERT(pDocItem != NULL);
			if(pDocItem != NULL)
			{
				m_signals.doubleClick.Send(pItem);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnRClick(NMHDR* pNMHDR, LRESULT* pResult)
	{
		SendMessage(WM_CONTEXTMENU, (WPARAM)m_hWnd, GetMessagePos());
		*pResult = 1;
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
	{
		CPoint	clientPoint = point;
		CCustomTreeCtrl::ScreenToClient(&clientPoint);

		CBrowserCtrlItemPtr	pItem = boost::static_pointer_cast<CBrowserCtrlItem>(CCustomTreeCtrl::FindItem(clientPoint));
		if(pItem != NULL)
		{
			SET_LOCAL_RESOURCE_SCOPE

			CContextMenu contextMenu;
			InitContextMenu(pItem, contextMenu);
			if(contextMenu.GetMenu().GetMenuItemCount() > 0)
			{
				switch(contextMenu.GetMenu().TrackPopupMenuEx(TPM_RETURNCMD, point.x, point.y, this, NULL))
				{
				case ContextMenuItem::SHOW_IN_EXPLORER:
					{
						OnShowInExplorer(point, pItem);
						break;
					}
				case ContextMenuItem::CREATE_DOC:
					{
						OnCreateDoc(point, pItem);
						break;
					}
				case ContextMenuItem::ADD_GROUP:
					{
						OnAddGroup(point, pItem);
						break;
					}
				case ContextMenuItem::REMOVE_GROUP:
					{
						OnRemoveGroup(point, pItem);
						break;
					}
				case ContextMenuItem::CREATE_ENUMERATION:
					{
						OnCreateEnumeration(point, pItem);
						break;
					}
				case ContextMenuItem::DESTROY_ENUMERATION:
					{
						OnDestroyEnumeration(point, pItem);
						break;
					}
				case ContextMenuItem::CREATE_STRUCTURE:
					{
						OnCreateStructure(point, pItem);
						break;
					}
				case ContextMenuItem::DESTROY_STRUCTURE:
					{
						OnDestroyStructure(point, pItem);
						break;
					}
				case ContextMenuItem::CREATE_SIGNAL:
					{
						OnCreateSignal(point, pItem);
						break;
					}
				case ContextMenuItem::DESTROY_SIGNAL:
					{
						OnDestroySignal(point, pItem);
						break;
					}
				case ContextMenuItem::CREATE_SCHEMA:
					{
						OnCreateSchema(point, pItem);
						break;
					}
				case ContextMenuItem::DESTROY_SCHEMA:
					{
						OnDestroySchema(point, pItem);
						break;
					}
				case ContextMenuItem::ADD_STATE_MACHINE:
					{
						OnAddStateMachine(point, pItem);
						break;
					}
				case ContextMenuItem::REMOVE_STATE_MACHINE:
					{
						OnRemoveStateMachine(point, pItem);
						break;
					}
				case ContextMenuItem::ADD_STATE:
					{
						OnAddState(point, pItem);
						break;
					}
				case ContextMenuItem::REMOVE_STATE:
					{
						OnRemoveState(point, pItem);
						break;
					}
				case ContextMenuItem::ADD_VARIABLE:
					{
						OnAddVariable(point, pItem);
						break;
					}
				case ContextMenuItem::REMOVE_VARIABLE:
					{
						OnRemoveVariable(point, pItem);
						break;
					}
				case ContextMenuItem::ADD_CONTAINER:
					{
						OnAddContainer(point, pItem);
						break;
					}
				case ContextMenuItem::REMOVE_CONTAINER:
					{
						OnRemoveContainer(point, pItem);
						break;
					}
				case ContextMenuItem::ADD_TIMER:
					{
						OnAddTimer(point, pItem);
						break;
					}
				case ContextMenuItem::REMOVE_TIMER:
					{
						OnRemoveTimer(point, pItem);
						break;
					}
				case ContextMenuItem::ADD_ABSTRACT_INTERFACE:
					{
						OnAddAbstractInterface(point, pItem);
						break;
					}
				case ContextMenuItem::REMOVE_ABSTRACT_INTERFACE:
					{
						OnRemoveAbstractInterface(point, pItem);
						break;
					}
				case ContextMenuItem::ADD_COMPONENT:
					{
						OnAddComponent(point, pItem);
						break;
					}
				case ContextMenuItem::REMOVE_COMPONENT:
					{
						OnRemoveComponent(point, pItem);
						break;
					}
				case ContextMenuItem::ADD_ACTION:
					{
						OnAddAction(point, pItem);
						break;
					}
				case ContextMenuItem::REMOVE_ACTION:
					{
						OnRemoveAction(point, pItem);
						break;
					}
				case ContextMenuItem::ADD_CONSTRUCTOR_GRAPH:
					{
						OnAddGraph(point, pItem, DocGraphType::CONSTRUCTOR);
						break;
					}
				case ContextMenuItem::ADD_DESTRUCTOR_GRAPH:
					{
						OnAddGraph(point, pItem, DocGraphType::DESTRUCTOR);
						break;
					}
				case ContextMenuItem::ADD_SIGNAL_RECEIVER_GRAPH:
					{
						OnAddGraph(point, pItem, DocGraphType::SIGNAL_RECEIVER);
						break;
					}
				case ContextMenuItem::ADD_FUNCTION_GRAPH:
					{
						OnAddGraph(point, pItem, DocGraphType::FUNCTION);
						break;
					}
				case ContextMenuItem::ADD_CONDITION_GRAPH:
					{
						OnAddGraph(point, pItem, DocGraphType::CONDITION);
						break;
					}
				case ContextMenuItem::REMOVE_GRAPH:
					{
						OnRemoveGraph(point, pItem);
						break;
					}
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::LoadImageList(CImageList& imageList)
	{
		LoadBrowserIcons(imageList);
		SetImageList(&imageList, TVSIL_NORMAL);
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::RefreshItem(const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			CCustomTreeCtrl::RemoveItemChildren(pItem);
			switch(pItem->GetIcon())
			{
			case BrowserIcon::FOLDER:
				{
					RefreshFolderItem(pItem);
					break;
				}
			case BrowserIcon::DOC:
				{
					RefreshDocItem(pItem);
					break;
				}
			case BrowserIcon::USER_GROUP:
				{
					RefreshGroupItem(pItem);
					break;
				}
			case BrowserIcon::SCHEMA:
				{
					RefreshSchemaItem(pItem);
					break;
				}
			case BrowserIcon::STATE_MACHINE:
				{
					RefreshStateMachineItem(pItem);
					break;
				}
			case BrowserIcon::STATE:
				{
					RefreshStateItem(pItem);
					break;
				}
			case BrowserIcon::ENV_ABSTRACT_INTERFACE:
				{
					RefreshAbstractInterfaceItem(pItem);
					break;
				}
			case BrowserIcon::SETTINGS_FOLDER:
				{
					RefreshSettingsFolderItem(pItem);
					break;
				}
			}
		}
		__super::Invalidate(true);
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::RefreshFolderItem(const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			stack_string	itemPath;
			GetItemPath(*pItem, itemPath, false);
			TStringVector	subFolderNames;
			TStringVector	fileNames;
			PluginUtils::GetSubFoldersAndFileNames(itemPath.c_str(), "*.*", true, subFolderNames, fileNames);
			for(TStringVector::iterator iSubFolderName = subFolderNames.begin(), iEndSubFolderName = subFolderNames.end(); iSubFolderName != iEndSubFolderName; ++ iSubFolderName)
			{
				AddItem(iSubFolderName->c_str(), BrowserIcon::FOLDER, pItem);
			}
			for(TStringVector::iterator iFileName = fileNames.begin(), iEndFileName = fileNames.end(); iFileName != iEndFileName; ++ iFileName)
			{
				string&				fileName = *iFileName;
				stack_string	docFileName = itemPath;
				docFileName.append("/");
				docFileName.append(fileName.c_str());
				const IDoc*	pDoc = GetSchematycFramework().GetDocManager().GetDoc(docFileName.c_str());
				CRY_ASSERT(pDoc != NULL);
				if(pDoc != NULL)
				{
					if((pDoc->GetFlags() & DocFlags::FROM_PAK) == 0)
					{
						AddItem(fileName.c_str(), BrowserIcon::DOC, pItem);
					}
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::RefreshDocItem(const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			IDoc*	pDoc = GetItemDoc(*pItem);
			CRY_ASSERT(pDoc != NULL);
			if(pDoc != NULL)
			{
				const SGUID	itemGUID;
				SDocVisitor	visitor(*this, pItem);
				pDoc->VisitGroups(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitGroup, visitor), false);
				pDoc->VisitEnumerations(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitEnumeration, visitor), false);
				pDoc->VisitStructures(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitStructure, visitor), false);
				pDoc->VisitSignals(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitSignal, visitor), false);
				pDoc->VisitSchemas(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitSchema, visitor), false);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::RefreshGroupItem(const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			IDoc*	pDoc = GetItemDoc(*pItem);
			CRY_ASSERT(pDoc != NULL);
			if(pDoc != NULL)
			{
				const SGUID	itemGUID = pItem->GetGUID();
				SDocVisitor	visitor(*this, pItem);
				pDoc->VisitGroups(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitGroup, visitor), false);
				pDoc->VisitSignals(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitSignal, visitor), false);
				pDoc->VisitSchemas(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitSchema, visitor), false);
				pDoc->VisitStateMachines(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitStateMachine, visitor), false);
				pDoc->VisitStates(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitState, visitor), false);
				pDoc->VisitVariables(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitVariable, visitor), false);
				pDoc->VisitContainers(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitContainer, visitor), false);
				pDoc->VisitTimers(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitTimer, visitor), false);
				pDoc->VisitComponentInstances(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitComponentInstance, visitor), false);
				pDoc->VisitActionInstances(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitActionInstance, visitor), false);
				pDoc->VisitGraphs(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitGraph, visitor), false);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::RefreshSchemaItem(const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			IDoc*	pDoc = GetItemDoc(*pItem);
			CRY_ASSERT(pDoc != NULL);
			if(pDoc != NULL)
			{
				const SGUID	itemGUID = pItem->GetGUID();
				SDocVisitor	visitor(*this, pItem);
				pDoc->VisitGroups(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitGroup, visitor), false);
				pDoc->VisitStateMachines(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitStateMachine, visitor), false);
				pDoc->VisitVariables(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitVariable, visitor), false);
				pDoc->VisitContainers(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitContainer, visitor), false);
				pDoc->VisitTimers(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitTimer, visitor), false);
				pDoc->VisitAbstractInterfaceInstances(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitAbstractInterfaceInstance, visitor), false);
				pDoc->VisitComponentInstances(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitComponentInstance, visitor), false);
				pDoc->VisitActionInstances(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitActionInstance, visitor), false);
				pDoc->VisitGraphs(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitGraph, visitor), false);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::RefreshStateMachineItem(const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			IDoc*	pDoc = GetItemDoc(*pItem);
			CRY_ASSERT(pDoc != NULL);
			if(pDoc != NULL)
			{
				const SGUID	itemGUID = pItem->GetGUID();
				SDocVisitor	visitor(*this, pItem);
				pDoc->VisitGroups(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitGroup, visitor), false);
				pDoc->VisitVariables(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitVariable, visitor), false);
				pDoc->VisitContainers(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitContainer, visitor), false);
				pDoc->VisitTimers(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitTimer, visitor), false);
				pDoc->VisitStates(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitState, visitor), false);
				pDoc->VisitActionInstances(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitActionInstance, visitor), false);
				pDoc->VisitGraphs(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitGraph, visitor), false);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::RefreshStateItem(const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			IDoc*	pDoc = GetItemDoc(*pItem);
			CRY_ASSERT(pDoc != NULL);
			if(pDoc != NULL)
			{
				const SGUID	itemGUID = pItem->GetGUID();
				SDocVisitor	visitor(*this, pItem);
				pDoc->VisitGroups(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitGroup, visitor), false);
				pDoc->VisitStates(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitState, visitor), false);
				pDoc->VisitVariables(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitVariable, visitor), false);
				pDoc->VisitContainers(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitContainer, visitor), false);
				pDoc->VisitTimers(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitTimer, visitor), false);
				pDoc->VisitActionInstances(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitActionInstance, visitor), false);
				pDoc->VisitGraphs(itemGUID, MAKE_MEMBER_DELEGATE(SDocVisitor::VisitGraph, visitor), false);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::RefreshAbstractInterfaceItem(const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			IDoc*	pDoc = GetItemDoc(*pItem);
			CRY_ASSERT(pDoc != NULL);
			if(pDoc != NULL)
			{
				SDocVisitor	visitor(*this, pItem);
				pDoc->VisitGraphs(pItem->GetGUID(), MAKE_MEMBER_DELEGATE(SDocVisitor::VisitGraph, visitor), false);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::InitContextMenu(const CBrowserCtrlItemPtr& pItem, CContextMenu& contextMenu)
	{
		switch(pItem->GetIcon())
		{
		case BrowserIcon::FOLDER:
			{
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::SHOW_IN_EXPLORER, "Show In Explorer");
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::CREATE_DOC, "Create Document");
				break;
			}
		case BrowserIcon::DOC:
			{
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::SHOW_IN_EXPLORER, "Show In Explorer");
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_GROUP, "Add Group");
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::CREATE_ENUMERATION, "Create Enumeration");
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::CREATE_STRUCTURE, "Create Structure");
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::CREATE_SIGNAL, "Create Signal");
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::CREATE_SCHEMA, "Create Schema");
				break;
			}
		case BrowserIcon::USER_GROUP:
			{
				CBrowserCtrlItemPtr	pParentItem = GetRealParentItem(pItem);
				if(pParentItem != NULL)
				{
					switch(pParentItem->GetIcon())
					{
					case BrowserIcon::DOC:
						{
							contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_GROUP, "Add Group");
							contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::CREATE_ENUMERATION, "Create Enumeration");
							contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::CREATE_STRUCTURE, "Create Structure");
							contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::CREATE_SIGNAL, "Create Signal");
							contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::CREATE_SCHEMA, "Create Schema");
							break;
						}
					case BrowserIcon::SCHEMA:
						{
							contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_GROUP, "Add Group");
							contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_STATE_MACHINE, "Add State Machine");
							contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_VARIABLE, "Add Variable");
							contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_CONTAINER, "Add Container");
							contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_TIMER, "Add Timer");
							contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_ABSTRACT_INTERFACE, "Add Abstract Interface");
							contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_COMPONENT, "Add Component");
							contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_ACTION, "Add Action");

							CContextMenu*	pGraphContextMenu = contextMenu.CreateSubMenu("Add Graph");
							pGraphContextMenu->GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_FUNCTION_GRAPH, "Function");
							pGraphContextMenu->GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_CONDITION_GRAPH, "Condition");
							pGraphContextMenu->GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_CONSTRUCTOR_GRAPH, "Constructor");
							pGraphContextMenu->GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_DESTRUCTOR_GRAPH, "Destructor");
							pGraphContextMenu->GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_SIGNAL_RECEIVER_GRAPH, "Signal Receiver");
							break;
						}
					case BrowserIcon::STATE_MACHINE:
						{
							contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_STATE, "Add State");
							break;
						}
					case BrowserIcon::STATE:
						{
							contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_STATE, "Add State");
							contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_GROUP, "Add Group");
							contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_VARIABLE, "Add Variable");
							contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_CONTAINER, "Add Container");
							contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_TIMER, "Add Timer");

							CContextMenu*	pGraphContextMenu = contextMenu.CreateSubMenu("Add Graph");
							pGraphContextMenu->GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_CONSTRUCTOR_GRAPH, "Constructor");
							pGraphContextMenu->GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_DESTRUCTOR_GRAPH, "Destructor");
							pGraphContextMenu->GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_SIGNAL_RECEIVER_GRAPH, "Signal Receiver");
							break;
						}
					}
					contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::REMOVE_GROUP, "Remove Group");
				}
				break;
			}
		case BrowserIcon::ENUMERATION:
			{
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::DESTROY_ENUMERATION, "Destroy Enumeration");
				break;
			}
		case BrowserIcon::STRUCTURE:
			{
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::DESTROY_STRUCTURE, "Destroy Structure");
				break;
			}
		case BrowserIcon::SIGNAL:
			{
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::DESTROY_SIGNAL, "Destroy Signal");
				break;
			}
		case BrowserIcon::SCHEMA:
			{
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_GROUP, "Add Group");
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_STATE_MACHINE, "Add State Machine");
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_VARIABLE, "Add Variable");
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_CONTAINER, "Add Container");
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_ABSTRACT_INTERFACE, "Add Abstract Interface");
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_COMPONENT, "Add Component");
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_ACTION, "Add Action");
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_TIMER, "Add Timer");

				CContextMenu*	pGraphContextMenu = contextMenu.CreateSubMenu("Add Graph");
				pGraphContextMenu->GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_FUNCTION_GRAPH, "Function");
				pGraphContextMenu->GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_CONDITION_GRAPH, "Condition");
				pGraphContextMenu->GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_CONSTRUCTOR_GRAPH, "Constructor");
				pGraphContextMenu->GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_DESTRUCTOR_GRAPH, "Destructor");
				pGraphContextMenu->GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_SIGNAL_RECEIVER_GRAPH, "Signal Receiver");

				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::DESTROY_SCHEMA, "Destroy Schema");
				break;
			}
		case BrowserIcon::STATE_MACHINE:
			{
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_STATE, "Add State");
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::REMOVE_STATE_MACHINE, "Remove State Machine");
				break;
			}
		case BrowserIcon::STATE:
			{
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_GROUP, "Add Group");
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_STATE, "Add State");
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_VARIABLE, "Add Variable");
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_CONTAINER, "Add Container");
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_TIMER, "Add Timer");
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_ACTION, "Add Action");

				CContextMenu*	pGraphContextMenu = contextMenu.CreateSubMenu("Add Graph");
				pGraphContextMenu->GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_CONSTRUCTOR_GRAPH, "Constructor");
				pGraphContextMenu->GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_DESTRUCTOR_GRAPH, "Destructor");
				pGraphContextMenu->GetMenu().AppendMenu(MF_STRING, ContextMenuItem::ADD_SIGNAL_RECEIVER_GRAPH, "Signal Receiver");
				
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::REMOVE_STATE, "Remove State");
				break;
			}
		case BrowserIcon::VARIABLE:
			{
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::REMOVE_VARIABLE, "Remove Variable");
				break;
			}
		case BrowserIcon::CONTAINER:
			{
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::REMOVE_CONTAINER, "Remove Container");
				break;
			}
		case BrowserIcon::TIMER:
			{
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::REMOVE_TIMER, "Remove Timer");
				break;
			}
		case BrowserIcon::ENV_ABSTRACT_INTERFACE:
			{
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::REMOVE_ABSTRACT_INTERFACE, "Remove Abstract Interface");
				break;
			}
		case BrowserIcon::COMPONENT:
			{
				IDocComponentInstanceConstPtr	pDocComponentInstance = GetItemDocComponentInstance(*pItem);
				CRY_ASSERT(pDocComponentInstance != NULL);
				if(pDocComponentInstance != NULL)
				{
					if((pDocComponentInstance->GetFlags() & DocComponentInstanceFlags::FOUNDATION) == 0)
					{
						contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::REMOVE_COMPONENT, "Remove Component");
					}
				}
				break;
			}
		case BrowserIcon::ACTION_INSTANCE:
			{
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::REMOVE_ACTION, "Remove Action");
				break;
			}
		case BrowserIcon::GRAPH:
		case BrowserIcon::SIGNAL_RECEIVER:
		case BrowserIcon::CONSTRUCTOR:
		case BrowserIcon::DESTRUCTOR:
		case BrowserIcon::FUNCTION_GRAPH:
		case BrowserIcon::CONDITION_GRAPH:
			{
				IDocGraphPtr	pDocGraph = GetItemDocGraph(*pItem);
				CRY_ASSERT(pDocGraph != NULL);
				if(pDocGraph != NULL)
				{
					switch(pDocGraph->GetType())
					{
					case DocGraphType::FUNCTION:
						{
							contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::REMOVE_GRAPH, "Remove Function");
							break;
						}
					case DocGraphType::CONDITION:
						{
							contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::REMOVE_GRAPH, "Remove Condition");
							break;
						}
					case DocGraphType::CONSTRUCTOR:
						{
							contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::REMOVE_GRAPH, "Remove Constructor");
							break;
						}
					case DocGraphType::DESTRUCTOR:
						{
							contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::REMOVE_GRAPH, "Remove Destructor");
							break;
						}
					case DocGraphType::SIGNAL_RECEIVER:
						{
							contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::REMOVE_GRAPH, "Remove Signal Receiver");
							break;
						}
					}
					break;
				}
			}
		case BrowserIcon::SETTINGS_FOLDER:
		case BrowserIcon::SETTINGS_DOC:
			{
				contextMenu.GetMenu().AppendMenu(MF_STRING, ContextMenuItem::SHOW_IN_EXPLORER, "Show In Explorer");
				break;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnShowInExplorer(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		const size_t				folderIcons[] = { BrowserIcon::FOLDER, BrowserIcon::SETTINGS_FOLDER };
		CBrowserCtrlItemPtr	pFolderItem = pItem->GetIcon() == BrowserIcon::FOLDER ? pItem : boost::static_pointer_cast<CBrowserCtrlItem>(pItem->FindAncestor(TSizeTConstArray(folderIcons)));
		CRY_ASSERT(pFolderItem != NULL);
		if(pFolderItem != NULL)
		{
			stack_string	itemPath;
			GetItemPath(*pFolderItem, itemPath, true);
			ShowInExplorer(itemPath.c_str());
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnCreateDoc(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			stack_string	path;
			GetItemPath(*pItem, path, false);
			if(CCreateDocDlg(this, point, path.c_str()).DoModal() == IDOK)
			{
				RefreshItem(pItem);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnAddGroup(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			IDoc*	pDoc = GetItemDoc(*pItem);
			CRY_ASSERT(pDoc != NULL);
			if(pDoc != NULL)
			{
				if(CAddGroupDlg(this, point, *pDoc, pItem->GetGUID()).DoModal() == IDOK)
				{
					RefreshItem(pItem);
					m_signals.docModified.Send(*pDoc);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnRemoveGroup(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			string	message = "Remove group named '";
			message.append(pItem->GetText());
			message.append("'?");
			if(MessageBox(message.c_str(), "Remove Group", MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
			{
				IDoc*	pDoc = GetItemDoc(*pItem);
				CRY_ASSERT(pDoc != NULL);
				if(pDoc != NULL)
				{
					pDoc->RemoveGroup(pItem->GetGUID(), true);
					RefreshItem(boost::static_pointer_cast<CBrowserCtrlItem>(pItem->GetParent()));
					m_signals.docModified.Send(*pDoc);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnCreateEnumeration(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			IDoc*	pDoc = GetItemDoc(*pItem);
			CRY_ASSERT(pDoc != NULL);
			if(pDoc != NULL)
			{
				if(CCreateEnumerationDlg(this, point, *pDoc, pItem->GetGUID()).DoModal() == IDOK)
				{
					RefreshItem(pItem);
					m_signals.docModified.Send(*pDoc);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnDestroyEnumeration(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem!= NULL);
		if(pItem!= NULL)
		{
			string	message = "Destroy enumeration named '";
			message.append(pItem->GetText());
			message.append("'?");
			if(MessageBox(message.c_str(), "Destroy Enumeration", MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
			{
				IDoc*	pDoc = GetItemDoc(*pItem);
				CRY_ASSERT(pDoc != NULL);
				if(pDoc != NULL)
				{
					pDoc->RemoveEnumeration(pItem->GetGUID(), true);
					RefreshItem(boost::static_pointer_cast<CBrowserCtrlItem>(pItem->GetParent()));
					m_signals.docModified.Send(*pDoc);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnCreateStructure(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			IDoc*	pDoc = GetItemDoc(*pItem);
			CRY_ASSERT(pDoc != NULL);
			if(pDoc != NULL)
			{
				if(CCreateStructureDlg(this, point, *pDoc, pItem->GetGUID()).DoModal() == IDOK)
				{
					RefreshItem(pItem);
					m_signals.docModified.Send(*pDoc);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnDestroyStructure(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			string	message = "Destroy structure named '";
			message.append(pItem->GetText());
			message.append("'?");
			if(MessageBox(message.c_str(), "Destroy Structure", MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
			{
				IDoc*	pDoc = GetItemDoc(*pItem);
				CRY_ASSERT(pDoc != NULL);
				if(pDoc != NULL)
				{
					pDoc->RemoveStructure(pItem->GetGUID(), true);
					RefreshItem(boost::static_pointer_cast<CBrowserCtrlItem>(pItem->GetParent()));
					m_signals.docModified.Send(*pDoc);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnCreateSignal(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			IDoc*	pDoc = GetItemDoc(*pItem);
			CRY_ASSERT(pDoc != NULL);
			if(pDoc != NULL)
			{
				if(CCreateSignalDlg(this, point, *pDoc, pItem->GetGUID()).DoModal() == IDOK)
				{
					RefreshItem(pItem);
					m_signals.docModified.Send(*pDoc);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnDestroySignal(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			string	message = "Destroy signal named '";
			message.append(pItem->GetText());
			message.append("'?");
			if(MessageBox(message.c_str(), "Remove Signal", MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
			{
				IDoc*	pDoc = GetItemDoc(*pItem);
				CRY_ASSERT(pDoc != NULL);
				if(pDoc != NULL)
				{
					pDoc->RemoveSignal(pItem->GetGUID(), true);
					RefreshItem(boost::static_pointer_cast<CBrowserCtrlItem>(pItem->GetParent()));
					m_signals.docModified.Send(*pDoc);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnCreateSchema(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			IDoc*	pDoc = GetItemDoc(*pItem);
			CRY_ASSERT(pDoc != NULL);
			if(pDoc != NULL)
			{
				if(CCreateSchemaDlg(this, point, *pDoc, pItem->GetGUID()).DoModal() == IDOK)
				{
					RefreshItem(pItem);
					m_signals.docModified.Send(*pDoc);
				}
			}
		}
	}
		
	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnDestroySchema(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			string	message = "Destroy schema named '";
			message.append(pItem->GetText());
			message.append("'?");
			if(MessageBox(message.c_str(), "Remove Schema", MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
			{
				IDoc*	pDoc = GetItemDoc(*pItem);
				CRY_ASSERT(pDoc != NULL);
				if(pDoc != NULL)
				{
					pDoc->RemoveSchema(pItem->GetGUID(), true);
					RefreshItem(boost::static_pointer_cast<CBrowserCtrlItem>(pItem->GetParent()));
					m_signals.docModified.Send(*pDoc);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnAddStateMachine(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			IDoc*	pDoc = GetItemDoc(*pItem);
			CRY_ASSERT(pDoc != NULL);
			if(pDoc != NULL)
			{
				if(CAddStateMachineDlg(this, point, *pDoc, pItem->GetGUID()).DoModal() == IDOK)
				{
					RefreshItem(pItem);
					m_signals.docModified.Send(*pDoc);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnRemoveStateMachine(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			string	message = "Remove state machine named '";
			message.append(pItem->GetText());
			message.append("'?");
			if(MessageBox(message.c_str(), "Remove State Machine", MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
			{
				IDoc*	pDoc = GetItemDoc(*pItem);
				CRY_ASSERT(pDoc != NULL);
				if(pDoc != NULL)
				{
					pDoc->RemoveStateMachine(pItem->GetGUID(), true);
					RefreshItem(boost::static_pointer_cast<CBrowserCtrlItem>(pItem->GetParent()));
					m_signals.docModified.Send(*pDoc);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnAddState(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			IDoc*	pDoc = GetItemDoc(*pItem);
			CRY_ASSERT(pDoc != NULL);
			if(pDoc != NULL)
			{
				if(CAddStateDlg(this, point, *pDoc, pItem->GetGUID()).DoModal() == IDOK)
				{
					RefreshItem(pItem);
					m_signals.docModified.Send(*pDoc);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnRemoveState(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			string	message = "Remove state named '";
			message.append(pItem->GetText());
			message.append("'?");
			if(MessageBox(message.c_str(), "Remove State", MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
			{
				IDoc*	pDoc = GetItemDoc(*pItem);
				CRY_ASSERT(pDoc != NULL);
				if(pDoc != NULL)
				{
					pDoc->RemoveState(pItem->GetGUID(), true);
					RefreshItem(boost::static_pointer_cast<CBrowserCtrlItem>(pItem->GetParent()));
					m_signals.docModified.Send(*pDoc);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnAddVariable(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			IDoc*	pDoc = GetItemDoc(*pItem);
			CRY_ASSERT(pDoc != NULL);
			if(pDoc != NULL)
			{
				if(CAddVariableDlg(this, point, *pDoc, pItem->GetGUID()).DoModal() == IDOK)
				{
					RefreshItem(pItem);
					m_signals.docModified.Send(*pDoc);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnRemoveVariable(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			string	message = "Remove variable named '";
			message.append(pItem->GetText());
			message.append("'?");
			if(MessageBox(message.c_str(), "Remove Variable", MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
			{
				IDoc*	pDoc = GetItemDoc(*pItem);
				CRY_ASSERT(pDoc != NULL);
				if(pDoc != NULL)
				{
					pDoc->RemoveVariable(pItem->GetGUID(), true);
					RefreshItem(boost::static_pointer_cast<CBrowserCtrlItem>(pItem->GetParent()));
					m_signals.docModified.Send(*pDoc);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnAddContainer(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			IDoc*	pDoc = GetItemDoc(*pItem);
			CRY_ASSERT(pDoc != NULL);
			if(pDoc != NULL)
			{
				if(CAddContainerDlg(this, point, *pDoc, pItem->GetGUID()).DoModal() == IDOK)
				{
					RefreshItem(pItem);
					m_signals.docModified.Send(*pDoc);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnRemoveContainer(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			string	message = "Remove container named '";
			message.append(pItem->GetText());
			message.append("'?");
			if(MessageBox(message.c_str(), "Remove Container", MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
			{
				IDoc*	pDoc = GetItemDoc(*pItem);
				CRY_ASSERT(pDoc != NULL);
				if(pDoc != NULL)
				{
					pDoc->RemoveContainer(pItem->GetGUID(), true);
					RefreshItem(boost::static_pointer_cast<CBrowserCtrlItem>(pItem->GetParent()));
					m_signals.docModified.Send(*pDoc);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnAddTimer(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			IDoc*	pDoc = GetItemDoc(*pItem);
			CRY_ASSERT(pDoc != NULL);
			if(pDoc != NULL)
			{
				if(CAddTimerDlg(this, point, *pDoc, pItem->GetGUID()).DoModal() == IDOK)
				{
					RefreshItem(pItem);
					m_signals.docModified.Send(*pDoc);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnRemoveTimer(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			string	message = "Remove timer named '";
			message.append(pItem->GetText());
			message.append("'?");
			if(MessageBox(message.c_str(), "Remove Timer", MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
			{
				IDoc*	pDoc = GetItemDoc(*pItem);
				CRY_ASSERT(pDoc != NULL);
				if(pDoc != NULL)
				{
					m_signals.docModified.Send(*pDoc);
					pDoc->RemoveTimer(pItem->GetGUID(), true);
					RefreshItem(boost::static_pointer_cast<CBrowserCtrlItem>(pItem->GetParent()));
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnAddAbstractInterface(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			IDoc*	pDoc = GetItemDoc(*pItem);
			CRY_ASSERT(pDoc != NULL);
			if(pDoc != NULL)
			{
				if(CAddAbstractInterfaceDlg(this, point, *pDoc, pItem->GetGUID()).DoModal() == IDOK)
				{
					RefreshItem(pItem);
					m_signals.docModified.Send(*pDoc);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnRemoveAbstractInterface(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			string	message = "Remove abstract interface named '";
			message.append(pItem->GetText());
			message.append("'?");
			if(MessageBox(message.c_str(), "Remove Abstract Interface", MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
			{
				IDoc*	pDoc = GetItemDoc(*pItem);
				CRY_ASSERT(pDoc != NULL);
				if(pDoc != NULL)
				{
					const SGUID												itemGUID = pItem->GetGUID();
					IDocAbstractInterfaceInstancePtr	pDocAbstractInterfaceInstance = pDoc->GetAbstractInterfaceInstance(itemGUID);
					CRY_ASSERT(pDocAbstractInterfaceInstance != NULL);
					if(pDocAbstractInterfaceInstance != NULL)
					{
						m_signals.docModified.Send(*pDoc);
						pDoc->RemoveAbstractInterfaceInstance(itemGUID, true);
						RefreshItem(boost::static_pointer_cast<CBrowserCtrlItem>(pItem->GetParent()));
					}
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnAddComponent(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			IDoc*	pDoc = GetItemDoc(*pItem);
			CRY_ASSERT(pDoc != NULL);
			if(pDoc)
			{
				if(CAddComponentDlg(this, point, *pDoc, pItem->GetGUID()).DoModal() == IDOK)
				{
					RefreshItem(pItem);
					m_signals.docModified.Send(*pDoc);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnRemoveComponent(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			string	message = "Remove component named '";
			message.append(pItem->GetText());
			message.append("'?");
			if(MessageBox(message.c_str(), "Remove Component", MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
			{
				IDoc*	pDoc = GetItemDoc(*pItem);
				CRY_ASSERT(pDoc != NULL);
				if(pDoc != NULL)
				{
					pDoc->RemoveComponentInstance(pItem->GetGUID(), true);
					RefreshItem(boost::static_pointer_cast<CBrowserCtrlItem>(pItem->GetParent()));
					m_signals.docModified.Send(*pDoc);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnAddAction(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			IDoc*	pDoc = GetItemDoc(*pItem);
			CRY_ASSERT(pDoc != NULL);
			if(pDoc != NULL)
			{
				if(CAddActionDlg(this, point, *pDoc, pItem->GetGUID()).DoModal() == IDOK)
				{
					RefreshItem(pItem);
					m_signals.docModified.Send(*pDoc);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnRemoveAction(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			string	message = "Remove action named '";
			message.append(pItem->GetText());
			message.append("'?");
			if(MessageBox(message.c_str(), "Remove Action", MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
			{
				IDoc*	pDoc = GetItemDoc(*pItem);
				CRY_ASSERT(pDoc != NULL);
				if(pDoc != NULL)
				{
					pDoc->RemoveActionInstance(pItem->GetGUID(), true);
					RefreshItem(boost::static_pointer_cast<CBrowserCtrlItem>(pItem->GetParent()));
					m_signals.docModified.Send(*pDoc);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnAddGraph(CPoint point, const CBrowserCtrlItemPtr& pItem, DocGraphType::EValue type)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			IDoc*	pDoc = GetItemDoc(*pItem);
			CRY_ASSERT(pDoc != NULL);
			if(pDoc != NULL)
			{
				if(CAddGraphDlg(this, point, *pDoc, pItem->GetGUID(), type).DoModal() == IDOK)
				{
					RefreshItem(pItem);
					m_signals.docModified.Send(*pDoc);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::OnRemoveGraph(CPoint point, const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			string	message = "Remove graph named '";
			message.append(pItem->GetText());
			message.append("'?");
			if(MessageBox(message.c_str(), "Remove Graph", MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
			{
				IDoc*	pDoc = GetItemDoc(*pItem);
				CRY_ASSERT(pDoc != NULL);
				if(pDoc != NULL)
				{
					const SGUID		itemGUID = pItem->GetGUID();
					IDocGraphPtr	pDocGraph = pDoc->GetGraph(itemGUID);
					CRY_ASSERT(pDocGraph != NULL);
					if(pDocGraph != NULL)
					{
						m_signals.docGraphRemoved.Send(*pDoc, *pDocGraph);
						m_signals.docModified.Send(*pDoc);
						pDoc->RemoveGraph(itemGUID, true);
						RefreshItem(boost::static_pointer_cast<CBrowserCtrlItem>(pItem->GetParent()));
					}
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CBrowserCtrl::RefreshSettingsFolderItem(const CBrowserCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem != NULL);
		if(pItem != NULL)
		{
			GetSchematycFramework().GetEnvRegistry().VisitSettings(MAKE_MEMBER_DELEGATE(SSettingsVisitor::VisitSettings, SSettingsVisitor(*this, pItem)));
		}
	}
}