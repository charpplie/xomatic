/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc add action dialog.
-------------------------------------------------------------------------
History:
- 23:04:2014: Created by Paul Slinger
*************************************************************************/

#ifndef __SCHEMATYC_ADDACTIONDLG_H__
#define __SCHEMATYC_ADDACTIONDLG_H__

#include <Schematyc/Schematyc_IDoc.h>
#include <Schematyc/Schematyc_IEnvRegistry.h>

struct IPropertyTree;

namespace Schematyc
{
	class CAddActionDlg : public CDialog
	{
		DECLARE_MESSAGE_MAP()

	public:

		CAddActionDlg(CWnd* pParent, CPoint pos, IDoc& doc, const SGUID& scopeGUID);

	protected:

		virtual BOOL OnInitDialog();
		virtual void DoDataExchange(CDataExchange* pDX);
		virtual void OnActionCtrlSelChange();
		virtual void OnNameCtrlSelChange();
		virtual void OnOK();

	private:

		struct SAction
		{
			SAction();

			SAction(const SGUID& _guid, const SGUID& _componentInstanceGUID, const char* _name, const char* _fullName, const char* _textDesc);

			SGUID		guid;
			SGUID		componentInstanceGUID;
			string	name;
			string	fullName;
			string	textDesc;
		};

		typedef std::vector<SAction> TActionVector;

		VisitStatus::EValue VisitActionFactory(const IActionFactoryConstPtr& pActionFactory);
		bool ValidateName(const stack_string& name);
		SAction GetSelection() const;

		CPoint				m_pos;
		IDoc&					m_doc;
		SGUID					m_scopeGUID;
		TActionVector	m_actions;
		CListBox			m_actionsCtrl;
		CStatic				m_descriptionCtrl;
		CEdit					m_nameCtrl;
		bool					m_nameModified;
	};
}

#endif __SCHEMATYC_ADDACTIONDLG_H__