/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc add component dialog.
-------------------------------------------------------------------------
History:
- 23:04:2014: Created by Paul Slinger
*************************************************************************/

#ifndef __SCHEMATYC_ADDCOMPONENTDLG_H__
#define __SCHEMATYC_ADDCOMPONENTDLG_H__

#include <Schematyc/Schematyc_IDoc.h>
#include <Schematyc/Schematyc_IEnvRegistry.h>

struct IPropertyTree;

namespace Schematyc
{
	class CAddComponentDlg : public CDialog
	{
		DECLARE_MESSAGE_MAP()

	public:

		CAddComponentDlg(CWnd* pParent, CPoint pos, IDoc& doc, const SGUID& scopeGUID);

	protected:

		virtual BOOL OnInitDialog();
		virtual void DoDataExchange(CDataExchange* pDX);
		virtual void OnComponentsCtrlSelChange();
		virtual void OnNameCtrlSelChange();
		virtual void OnOK();

	private:

		VisitStatus::EValue VisitComponentFactory(const IComponentFactoryConstPtr& pComponentFactory);
		bool ValidateName(const stack_string& name);
		SGUID GetSelection() const;

		typedef std::pair<SGUID, string>			TGUIDStringPair;
		typedef std::vector<TGUIDStringPair>	TGUIDStringPairVector;

		CPoint								m_pos;
		IDoc&									m_doc;
		SGUID									m_scopeGUID;
		TGUIDStringPairVector	m_components;
		CListBox							m_componentsCtrl;
		CStatic								m_descriptionCtrl;
		CEdit									m_nameCtrl;
		bool									m_nameModified;
	};
}

#endif __SCHEMATYC_ADDCOMPONENTDLG_H__