////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   TimeOfDayEdit.h
//  Version:     v1.00
//  Created:     28/3/2012 by Achim Lang.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History: Based on the previous TimeOfDayEdit.h, but now splitted into .h and .cpp
//
////////////////////////////////////////////////////////////////////////////

#ifndef __TimeOfDayEdit_h__
#define __TimeOfDayEdit_h__

#define TIMEOFDAYN_CHANGE  0x0800

class CTimeOfDayEdit : public CXTTimeEdit
{
public:
	CTimeOfDayEdit();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual bool ProcessMask(UINT& nChar,int nEndPos);

protected:
	afx_msg void OnKillFocus(CWnd* pNewWnd);

	DECLARE_MESSAGE_MAP()

private:
	bool m_msgSent;
};

#endif // __TimeOfDayEdit_h__