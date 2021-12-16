/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc create document dialog.
-------------------------------------------------------------------------
History:
- 01:01:2013: Created by Paul Slinger
*************************************************************************/

#ifndef __SCHEMATYC_CREATEDOCDLG_H__
#define __SCHEMATYC_CREATEDOCDLG_H__

namespace Schematyc
{
	class CCreateDocDlg : public CDialog
	{
	public:

		CCreateDocDlg(CWnd* pParent, CPoint pos, const char* path);

	protected:

		virtual BOOL OnInitDialog();
		virtual void DoDataExchange(CDataExchange* pDX);
		virtual void OnOK();

	private:

		bool ValidateFilePath(const stack_string& filePath);

		CPoint	m_pos;
		string	m_path;
		CEdit		m_nameCtrl;
	};
}

#endif //__SCHEMATYC_CREATEDOCDLG_H__