////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   TimeOfDayEdit.cpp
//  Version:     v1.00
//  Created:     28/3/2012 by Achim Lang.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History: 
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TimeOfDayEdit.h"

//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CTimeOfDayEdit, CXTTimeEdit)
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
CTimeOfDayEdit::CTimeOfDayEdit()
{
	m_msgSent = false;
	m_bMilitary = true;
}

//////////////////////////////////////////////////////////////////////////
BOOL CTimeOfDayEdit::PreTranslateMessage(MSG* pMsg)
{
	if ( pMsg->message == WM_KEYDOWN || pMsg->message==WM_KEYUP )
	{
		switch ( pMsg->wParam )
		{
		case VK_RETURN:
			m_msgSent = true;
			::SendMessage( GetOwner()->GetSafeHwnd(),WM_COMMAND,MAKEWPARAM( GetDlgCtrlID(),TIMEOFDAYN_CHANGE ),(LPARAM)GetSafeHwnd() );
			break;
		}
	}
	return __super::PreTranslateMessage(pMsg);
}

//////////////////////////////////////////////////////////////////////////
bool CTimeOfDayEdit::ProcessMask(UINT& nChar,int nEndPos)
{
	// check the key against the mask
	switch ( m_strMask.GetAt( nEndPos ) )
	{
	case '0':		// digit only //completely changed this
		{
			if ( _istdigit( (TCHAR)nChar ) )
			{
				switch (nEndPos)
				{
				case 0:
					if (m_bMilitary)
					{
						if (nChar < 48 || nChar > 50)
						{
							MessageBeep((UINT)-1);
							return false;
						}
					}
					else
					{
						if (nChar < 48 || nChar > 49)
						{
							MessageBeep((UINT)-1);
							return false;
						}
					}
					break;

				case 1:
					if (m_bMilitary)
					{
						if (nChar < 48 || nChar > 57)
						{
							MessageBeep((UINT)-1);
							return false;
						}
					}
					else
					{
						if (nChar < 48 || nChar > 50)
						{
							MessageBeep((UINT)-1);
							return false;
						}
					}
					break;

				case 3:
					if (nChar < 48 || nChar > 53)
					{
						MessageBeep((UINT)-1);
						return false;
					}
					break;

				case 4:
					if (nChar < 48 || nChar > 57)
					{
						MessageBeep((UINT)-1);
						return false;
					}
					break;
				}

				return true;
			}
			break;
		}
	}

	MessageBeep((UINT)-1);
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CTimeOfDayEdit::OnKillFocus(CWnd* pNewWnd)
{
	CEdit::OnKillFocus(pNewWnd);
	if (!m_msgSent)
		::SendMessage( GetOwner()->GetSafeHwnd(),WM_COMMAND,MAKEWPARAM( GetDlgCtrlID(),TIMEOFDAYN_CHANGE ),(LPARAM)GetSafeHwnd() );
	else
		m_msgSent = false;
}
