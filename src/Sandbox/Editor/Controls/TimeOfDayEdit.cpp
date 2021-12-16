////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2014.
// -------------------------------------------------------------------------
//  File name:   TimeOfDayDialog.cpp
//  Version:     v1.00
//  Created:     10/30/2014 by Taeyoen.
//  Compilers:   Visual C++ 2012
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "TimeOfDayEdit.h"

BEGIN_MESSAGE_MAP(CTimeOfDayEdit, CXTTimeEdit)
	ON_WM_CTLCOLOR_REFLECT()
END_MESSAGE_MAP()

CTimeOfDayEdit::CTimeOfDayEdit()
{
	m_bMilitary = true;
	m_bValidTimeValue = true;
	m_EditBrush.CreateSolidBrush( GetSysColor(COLOR_WINDOW) );
	m_fCurrentTime = -1.f;
}

BOOL CTimeOfDayEdit::PreTranslateMessage(MSG* pMsg)
{
	if ( pMsg->message == WM_KEYDOWN || pMsg->message==WM_KEYUP )
	{
		switch ( pMsg->wParam )
		{
		case VK_RETURN:	
			m_bValidTimeValue = true;

			CString str;
			GetWindowText(str);
			str = ForceMakeComleteValue(str);
			SetWindowText(str);

			if(IsCompleteTimeStr(str))
			{
				uint hour = 0;
				uint minute = 0;

				sscanf( str, _T("%02d:%02d"), &hour, &minute );
				const float fTime = (float)hour + (float)minute/60.0f;

				SetTime(fTime);
			}
			else
			{
				m_bValidTimeValue = false;
			}

			break;
		}
	}

	return __super::PreTranslateMessage(pMsg);
}

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

COLORREF CTimeOfDayEdit::GetTextColor()
{
	if(!m_bValidTimeValue)
	{
		return RGB(68, 186, 231);
	}
 
	return GetSysColor(COLOR_WINDOWTEXT);
}

bool CTimeOfDayEdit::IsCompleteTimeStr(CString szText)
{
	int mid = szText.Find(":");
	CString szHour = szText.Left(mid);
	CString szMinute = szText.Right(mid);

	if(szHour.GetLength()!=2 || szMinute.GetLength()!=2 || isdigit(szHour[0])==0 || isdigit(szHour[1])==0 || isdigit(szMinute[0])==0 || isdigit(szMinute[1])==0 )
	{
		return false;
	}

	return true;
}

CString CTimeOfDayEdit::ForceMakeComleteValue(CString szText)
{
	int mid = szText.Find(":");
	uint hour = atoi(szText.Left(mid));
	uint minute = atoi(szText.Right(mid));

	CString szCompleteValue;
	szCompleteValue.Format("%02d:%02d",hour,minute);

	return szCompleteValue;
}

void CTimeOfDayEdit::SetTime(float fTime)
{
	if(m_fCurrentTime != fTime)
	{
		m_fCurrentTime = fTime;
		::SendMessage( GetOwner()->GetSafeHwnd(),WM_COMMAND,MAKEWPARAM( GetDlgCtrlID(),TIMEOFDAYN_CHANGE ),(LPARAM)GetSafeHwnd() );

		int nHour = floor(fTime);
		int nMins = (fTime - floor(fTime)) * 60.0f;
		
		__super::SetTime(nHour, nMins);
	}
}

HBRUSH CTimeOfDayEdit::CtlColor(CDC* pDC, UINT nCtlColor)
{
	pDC->SetTextColor(GetTextColor());
	pDC->SetBkColor(GetSysColor(COLOR_WINDOW));

	m_EditBrush.DeleteObject();
	m_EditBrush.CreateSolidBrush(GetSysColor(COLOR_WINDOW));
	return m_EditBrush;
}