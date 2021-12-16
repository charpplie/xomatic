/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc create structure dialog.
-------------------------------------------------------------------------
History:
- 01:01:2013: Created by Paul Slinger
*************************************************************************/

#ifndef __SCHEMATYC_CREATESTRUCTUREDLG_H__
#define __SCHEMATYC_CREATESTRUCTUREDLG_H__

namespace Schematyc
{
	class CCreateStructureDlg : public CDialog
	{
	public:

		CCreateStructureDlg(CWnd* pParent, CPoint pos, IDoc& doc, const SGUID& scopeGUID);

	protected:

		virtual BOOL OnInitDialog();
		virtual void DoDataExchange(CDataExchange* pDX);
		virtual void OnOK();

	private:

		bool ValidateName(const stack_string& name);

		CPoint		m_pos;
		IDoc&			m_doc;
		SGUID			m_scopeGUID;
		CEdit			m_nameCtrl;
	};
}

#endif __SCHEMATYC_CREATESTRUCTUREDLG_H__