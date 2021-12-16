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

#ifndef __SCHEMATYC_BROWSERCTRL_H__
#define __SCHEMATYC_BROWSERCTRL_H__

#include <BoostHelpers.h>

#include "Schematyc_TreeCtrl.h"

namespace Schematyc
{
	class CBrowserCtrlItem : public CCustomTreeCtrlItem, public boost::enable_shared_from_this<CBrowserCtrlItem>
	{
	public:

		CBrowserCtrlItem(const char* text, size_t icon, const CCustomTreeCtrlItemPtr& pParent, const SGUID& guid);

		const SGUID& GetGUID() const;

	private:

		SGUID	m_guid;
	};

	DECLARE_BOOST_POINTERS(CBrowserCtrlItem)

	class CBrowserCtrl : public CCustomTreeCtrl
	{
		DECLARE_MESSAGE_MAP()

	public:

		typedef TemplateUtils::CSignal<void (const CBrowserCtrlItemPtr&)>	TSelectionSignal;
		typedef TemplateUtils::CSignal<void (const CBrowserCtrlItemPtr&)>	TDoubleClickSignal;
		typedef TemplateUtils::CSignal<void (IDoc&)>											TDocModifiedSignal;
		typedef TemplateUtils::CSignal<void (IDoc&, IDocGraph&)>					TDocGraphRemovedSignal;

		// CCustomTreeCtrl
		virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
		// ~CCustomTreeCtrl

		CBrowserCtrlItemPtr AddItem(const char* text, size_t icon, const CBrowserCtrlItemPtr& pParentItem = CBrowserCtrlItemPtr(), const SGUID& guid = SGUID());
		CBrowserCtrlItemPtr GetSelectedItem();
		IDoc* GetItemDoc(const CBrowserCtrlItem& item) const;
		IDocElementPtr GetItemDocElement(const CBrowserCtrlItem& item) const;
		IDocGroupPtr GetItemDocGroup(const CBrowserCtrlItem& item) const;
		IDocEnumerationPtr GetItemDocEnumeration(const CBrowserCtrlItem& item) const;
		IDocStructurePtr GetItemDocStructure(const CBrowserCtrlItem& item) const;
		IDocSignalPtr GetItemDocSignal(const CBrowserCtrlItem& item) const;
		IDocSchemaPtr GetItemDocSchema(const CBrowserCtrlItem& item) const;
		IDocStateMachinePtr GetItemDocStateMachine(const CBrowserCtrlItem& item) const;
		IDocStatePtr GetItemDocState(const CBrowserCtrlItem& item) const;
		IDocVariablePtr GetItemDocVariable(const CBrowserCtrlItem& item) const;
		IDocContainerPtr GetItemDocContainer(const CBrowserCtrlItem& item) const;
		IDocTimerPtr GetItemDocTimer(const CBrowserCtrlItem& item) const;
		IDocAbstractInterfaceInstancePtr GetItemDocAbstractInterfaceInstance(const CBrowserCtrlItem& item) const;
		IDocComponentInstancePtr GetItemDocComponentInstance(const CBrowserCtrlItem& item) const;
		IDocActionInstancePtr GetItemDocActionInstance(const CBrowserCtrlItem& item) const;
		IDocGraphPtr GetItemDocGraph(const CBrowserCtrlItem& item) const;
		TSelectionSignal& GetSelectionSignal();
		TDoubleClickSignal& GetDoubleClickSignal();
		TDocModifiedSignal& GetDocModifiedSignal();
		TDocGraphRemovedSignal& GetDocGraphRemovedSignal();

	protected:

		class CContextMenu
		{
		public:

			CContextMenu();

			~CContextMenu();

			CMenu& GetMenu();
			CContextMenu* CreateSubMenu(const char* name);

		private:

			typedef std::vector<CContextMenu*> TSubMenuVector;

			CMenu						m_menu;
			TSubMenuVector	m_subMenus;
		};

		afx_msg void OnSelChanged(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
		afx_msg void OnRClick(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);

		// CCustomTreeCtrl
		virtual void LoadImageList(CImageList& imageList);
		// ~CCustomTreeCtrl

	private:

		struct ContextMenuItem
		{
			enum EValue
			{
				SHOW_IN_EXPLORER = 1,
				CREATE_DOC,
				ADD_GROUP,
				REMOVE_GROUP,
				CREATE_ENUMERATION,
				DESTROY_ENUMERATION,
				CREATE_STRUCTURE,
				DESTROY_STRUCTURE,
				CREATE_SIGNAL,
				DESTROY_SIGNAL,
				CREATE_SCHEMA,
				DESTROY_SCHEMA,
				ADD_STATE_MACHINE,
				REMOVE_STATE_MACHINE,
				ADD_STATE,
				REMOVE_STATE,
				ADD_VARIABLE,
				REMOVE_VARIABLE,
				ADD_CONTAINER,
				REMOVE_CONTAINER,
				ADD_TIMER,
				REMOVE_TIMER,
				ADD_ABSTRACT_INTERFACE,
				REMOVE_ABSTRACT_INTERFACE,
				ADD_COMPONENT,
				REMOVE_COMPONENT,
				ADD_ACTION,
				REMOVE_ACTION,
				ADD_CONSTRUCTOR_GRAPH,
				ADD_DESTRUCTOR_GRAPH,
				ADD_SIGNAL_RECEIVER_GRAPH,
				ADD_FUNCTION_GRAPH,
				ADD_CONDITION_GRAPH,
				REMOVE_GRAPH
			};
		};

		void RefreshItem(const CBrowserCtrlItemPtr& pItem);
		void RefreshFolderItem(const CBrowserCtrlItemPtr& pItem);
		void RefreshDocItem(const CBrowserCtrlItemPtr& pItem);
		void RefreshGroupItem(const CBrowserCtrlItemPtr& pItem);
		void RefreshSchemaItem(const CBrowserCtrlItemPtr& pItem);
		void RefreshStateMachineItem(const CBrowserCtrlItemPtr& pItem);
		void RefreshStateItem(const CBrowserCtrlItemPtr& pItem);
		void RefreshAbstractInterfaceItem(const CBrowserCtrlItemPtr& pItem);
		void RefreshActionItem(const CBrowserCtrlItemPtr& pItem);
		void InitContextMenu(const CBrowserCtrlItemPtr& pItem, CContextMenu& contextMenu);
		void OnShowInExplorer(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void OnCreateDoc(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void OnAddGroup(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void OnRemoveGroup(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void OnCreateEnumeration(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void OnDestroyEnumeration(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void OnCreateStructure(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void OnDestroyStructure(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void OnCreateSignal(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void OnDestroySignal(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void OnCreateSchema(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void OnDestroySchema(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void OnAddStateMachine(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void OnRemoveStateMachine(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void OnAddState(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void OnRemoveState(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void OnAddVariable(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void OnRemoveVariable(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void OnAddContainer(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void OnRemoveContainer(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void OnAddTimer(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void OnRemoveTimer(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void OnAddAbstractInterface(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void OnRemoveAbstractInterface(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void OnAddComponent(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void OnRemoveComponent(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void OnAddAction(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void OnRemoveAction(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void OnAddGraph(CPoint point, const CBrowserCtrlItemPtr& pItem, DocGraphType::EValue type);
		void OnRemoveGraph(CPoint point, const CBrowserCtrlItemPtr& pItem);
		void RefreshSettingsFolderItem(const CBrowserCtrlItemPtr& pItem);

		struct
		{
			TSelectionSignal				selection;
			TDoubleClickSignal			doubleClick;
			TDocModifiedSignal			docModified;
			TDocGraphRemovedSignal	docGraphRemoved;
		} m_signals;
	};
}

#endif //__SCHEMATYC_BROWSERCTRL_H__
