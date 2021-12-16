/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc property tree dialog.
-------------------------------------------------------------------------
History:
- 25:11:2014: Created by Paul Slinger
*************************************************************************/

#ifndef __SCHEMATYC_PROPERTYTREEDLG_H__
#define __SCHEMATYC_PROPERTYTREEDLG_H__

struct IPropertyTree;

namespace Schematyc
{
	class CPropertyTreeDlg : public CDialog
	{
	public:

		CPropertyTreeDlg(CWnd* pParent, CPoint pos, const char* name, Serialization::SStruct& properties);

		virtual ~CPropertyTreeDlg();

	private:

		virtual BOOL OnInitDialog();
		virtual void DoDataExchange(CDataExchange* pDX);
		virtual void OnOK();

		CPoint									m_pos;
		string									m_name;
		Serialization::SStruct&	m_properties;
		IPropertyTree*					m_pPropertyTree;
	};
}

#endif __SCHEMATYC_PROPERTYTREEDLG_H__