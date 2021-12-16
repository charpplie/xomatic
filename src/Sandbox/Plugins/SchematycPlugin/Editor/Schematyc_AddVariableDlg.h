/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc add variable dialog.
-------------------------------------------------------------------------
History:
- 01:01:2013: Created by Paul Slinger
*************************************************************************/

#ifndef __SCHEMATYC_ADDVARIABLEDLG_H__
#define __SCHEMATYC_ADDVARIABLEDLG_H__

#include <Schematyc/Schematyc_IDoc.h>
#include <Schematyc/Schematyc_IEnvRegistry.h>

struct IPropertyTree;

namespace Schematyc
{
	class CAddVariableDlg : public CDialog
	{
	public:

		CAddVariableDlg(CWnd* pParent, CPoint pos, IDoc& doc, const SGUID& scopeGUID);

	protected:

		virtual BOOL OnInitDialog();
		virtual void DoDataExchange(CDataExchange* pDX);
		virtual void OnOK();

	private:

		typedef std::pair<SGUID, string>			TGUIDStringPair;
		typedef std::vector<TGUIDStringPair>	TGUIDStringPairVector;

		VisitStatus::EValue VisitTypeDesc(const ITypeDescConstPtr& pTypeDesc);
		VisitStatus::EValue VisitEnumeration(const IDocEnumerationConstPtr& pEnumeration);
		VisitStatus::EValue VisitStructure(const IDocStructureConstPtr& pStructure);
		bool ValidateName(const stack_string& name);
		SGUID GetSelection() const;

		CPoint								m_pos;
		IDoc&									m_doc;
		SGUID									m_scopeGUID;
		TGUIDStringPairVector	m_types;
		CComboBox							m_typesCtrl;
		CEdit									m_nameCtrl;
	};
}

#endif __SCHEMATYC_ADDVARIABLEDLG_H__