/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc add abstract interface dialog.
-------------------------------------------------------------------------
History:
- 11:07:2014: Created by Paul Slinger
*************************************************************************/

#ifndef __SCHEMATYC_ADDBASTRACTINTERFACEDLG_H__
#define __SCHEMATYC_ADDBASTRACTINTERFACEDLG_H__

#include <Schematyc/Schematyc_IDoc.h>
#include <Schematyc/Schematyc_IAbstractInterface.h>

namespace Schematyc
{
	class CAddAbstractInterfaceDlg : public CDialog
	{
		DECLARE_MESSAGE_MAP()

	public:

		CAddAbstractInterfaceDlg(CWnd* pParent, CPoint pos, IDoc& doc, const SGUID& objectGUID);

		virtual ~CAddAbstractInterfaceDlg();

	protected:

		virtual BOOL OnInitDialog();
		virtual void DoDataExchange(CDataExchange* pDX);
		virtual void OnAbstractInterfacesCtrlSelChange();
		virtual void OnOK();

	private:

		VisitStatus::EValue VisitAbstractInterface(const IAbstractInterfaceConstPtr& pAbstractInterface);
		IAbstractInterfaceConstPtr GetSelectedAbstractInterface() const;

		typedef std::pair<SGUID, string>			TGUIDStringPair;
		typedef std::vector<TGUIDStringPair>	TGUIDStringPairVector;

		CPoint									m_pos;
		IDoc&										m_doc;
		SGUID										m_objectGUID;
		TGUIDStringPairVector		m_abstractInterfaces;
		CListBox								m_abstractInterfacesCtrl;
		CStatic									m_descriptionCtrl;
	};
}

#endif __SCHEMATYC_ADDBASTRACTINTERFACEDLG_H__