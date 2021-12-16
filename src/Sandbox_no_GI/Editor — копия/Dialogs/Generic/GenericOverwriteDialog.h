/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: GenericOverwriteDialog.h,v 1.1 2008/12/05 12:13:14 PauloZaffari Exp wwwrun $
$DateTime$
Description:  This is the header file for the general utility dialog for
overwrite confirmation. The purpose of this dialog, as one might imagine, is to 
get check if the user really wants to overwrite some item, allowing him/her to
apply the same choices to all the remaining items.

The recomended way to call this dialog is through DoModal() 
method.

Usage Hint: Use the UserOptions.h file.
-------------------------------------------------------------------------
History:
- 05:12:2008   12:13: Created by Paulo Zaffari
*************************************************************************/

#ifndef GenericOverwriteDialog_h__
#define GenericOverwriteDialog_h__

#pragma once

class CGenericOverwriteDialog: public CDialog
{
	//////////////////////////////////////////////////////////////////////////
	// Types & typedefs
public:
protected:
private:
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Methods
public:
	CGenericOverwriteDialog(const CString& strTitle,const CString& strText,bool boMultipleFiles=true);
	void DoDataExchange(CDataExchange* pDX);
	BOOL OnInitDialog();
	void OnYes();
	void OnNo();
	void OnCancel();

	// Call THIS method to start this dialog.
	// The return values can be IDYES, IDNO and IDCANCEL
	INT_PTR DoModal( );

	bool IsToAllToggled();

	DECLARE_MESSAGE_MAP();
	//}}AFX_MSG
protected:
private:
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Data
public:
protected:
	CString   m_strTitle;
	CString   m_strText;
	bool			m_boToAll;

	bool			m_boMultipleFiles;
	CStatic		m_strTextMessage;
	CButton		m_oToAll;

	bool			m_boIsModal;
private:
	//////////////////////////////////////////////////////////////////////////
};


#endif // GenericOverwriteDialog_h__