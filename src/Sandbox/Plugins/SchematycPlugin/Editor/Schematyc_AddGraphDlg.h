/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc add graph dialog.
-------------------------------------------------------------------------
History:
- 01:01:2013: Created by Paul Slinger
*************************************************************************/

#ifndef __SCHEMATYC_ADDGRAPHDLG_H__
#define __SCHEMATYC_ADDGRAPHDLG_H__

#include <Schematyc/Schematyc_IEnvRegistry.h>

namespace Schematyc
{
	class CAddGraphDlg : public CDialog
	{
		DECLARE_MESSAGE_MAP()

	public:

		CAddGraphDlg(CWnd* pParent, CPoint pos, IDoc& doc, const SGUID& scopeGUID, DocGraphType::EValue type);

	protected:

		virtual BOOL OnInitDialog();
		virtual void DoDataExchange(CDataExchange* pDX);
		virtual void OnNameCtrlSelChange();
		virtual void OnContextsCtrlSelChange();
		virtual void OnOK();

	private:

		typedef std::pair<SGUID, string>			TGUIDStringPair;
		typedef std::vector<TGUIDStringPair>	TGUIDStringPairVector;

		VisitStatus::EValue VisitEnvSignal(const ISignalConstPtr& pSignal);
		VisitStatus::EValue VisitDocSignal(const IDocSignalConstPtr& pSignal);
		VisitStatus::EValue VisitDocTimer(const IDocTimerConstPtr& timer);
		bool ValidateName(const stack_string& name);
		SGUID GetSelectionGUID() const;
		const char* GetSelectionString() const;

		CPoint									m_pos;
		IDoc&										m_doc;
		SGUID										m_scopeGUID;
		DocGraphType::EValue		m_type;
		CEdit										m_nameCtrl;
		bool										m_nameModified;
		TGUIDStringPairVector		m_contexts;
		CListBox								m_contextsCtrl;
	};
}

#endif __SCHEMATYC_ADDGRAPHDLG_H__