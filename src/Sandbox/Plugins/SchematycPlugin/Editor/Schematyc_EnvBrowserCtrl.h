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

#ifndef __SCHEMATYC_ENVBROWSERCTRL_H__
#define __SCHEMATYC_ENVBROWSERCTRL_H__

#include "BoostHelpers.h"

#include <Schematyc/Schematyc_IEnvRegistry.h>
#include <Schematyc/Schematyc_IAbstractInterface.h>

#include "Schematyc_TreeCtrl.h"

namespace Schematyc
{
	class CEnvBrowserCtrlItem : public CCustomTreeCtrlItem
	{
	public:

		CEnvBrowserCtrlItem(const char* text, size_t icon, const CCustomTreeCtrlItemPtr& pParent, const SGUID& guid);

		const SGUID& GetGUID() const;

	private:

		SGUID	m_guid;
	};

	DECLARE_BOOST_POINTERS(CEnvBrowserCtrlItem)

	class CEnvBrowserCtrl : public CCustomTreeCtrl
	{
		DECLARE_MESSAGE_MAP()

	public:

		// CCustomTreeCtrl
		virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
		// ~CCustomTreelCtrl

		void Refresh();

	protected:

		afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);

		// CCustomTreeCtrl
		virtual void LoadImageList(CImageList& imageList);
		// ~CCustomTreeCtrl

	private:

		CEnvBrowserCtrlItemPtr AddItem(const char* text, size_t icon, const CCustomTreeCtrlItemPtr& pParentItem = CCustomTreeCtrlItemPtr(), const SGUID& guid = SGUID());
		CEnvBrowserCtrlItemPtr FindItem(const SGUID& guid);
		CEnvBrowserCtrlItemPtr GetOrCreateParentItem(const SGUID& ownerGUID, const char* scope);
		VisitStatus::EValue VisitSignal(const ISignalConstPtr& pSignal);
		VisitStatus::EValue VisitGlobalFunction(const IGlobalFunctionConstPtr& pFunction);
		VisitStatus::EValue VisitAbstractInterface(const IAbstractInterfaceConstPtr& pAbstractInterface);
		VisitStatus::EValue VisitComponentFactory(const IComponentFactoryConstPtr& pComponentFactory);
		VisitStatus::EValue VisitComponentMemberFunction(const IComponentMemberFunctionConstPtr& pFunction);
		VisitStatus::EValue VisitActionFactory(const IActionFactoryConstPtr& pActionFactory);
		VisitStatus::EValue VisitActionMemberFunction(const IActionMemberFunctionConstPtr& pFunction);

		CEnvBrowserCtrlItemPtr	m_pRootItem;
	};
}

#endif //__SCHEMATYC_ENVBROWSERCTRL_H__
