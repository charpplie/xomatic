/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc create schema dialog.
-------------------------------------------------------------------------
History:
- 01:01:2013: Created by Paul Slinger
*************************************************************************/

#ifndef __SCHEMATYC_CREATESCHEMADLG_H__
#define __SCHEMATYC_CREATESCHEMADLG_H__

#include <Schematyc/Schematyc_IEnvRegistry.h>
#include <Schematyc/Schematyc_IDoc.h>

namespace Schematyc
{
	class CCreateSchemaDlg : public CDialog
	{
	public:

		CCreateSchemaDlg(CWnd* pParent, CPoint pos, IDoc& doc, const SGUID& scopeGUID);

	protected:

		virtual BOOL OnInitDialog();
		virtual void DoDataExchange(CDataExchange* pDX);
		virtual void OnOK();

	private:

		typedef std::pair<SGUID, string>			TGUIDStringPair;
		typedef std::vector<TGUIDStringPair>	TGUIDStringPairVector;

		void CollectFoundations();
		VisitStatus::EValue VisitFoundation(const IFoundationConstPtr& pFoundation);
		bool ValidateName(const stack_string& name);
		SGUID GetSelection() const;

		CPoint								m_pos;
		IDoc&									m_doc;
		SGUID									m_scopeGUID;
		TGUIDStringPairVector	m_foundations;
		CComboBox							m_foundationsCtrl;
		CEdit									m_nameCtrl;
	};
}

#endif __SCHEMATYC_CREATESCHEMADLG_H__