#ifndef __DialogCommon_H__
#define __DialogCommon_H__

#include "StringDlg.h"

class CDialogPositionHelper
{
public:
	static void SetToMouseCursor(CDialog& diag, bool center = false)
	{
		CRect formRect;
		diag.GetWindowRect(&formRect);
		const int width = formRect.Width();
		const int height = formRect.Height();
		CPoint clientPoint;
		::GetCursorPos( &clientPoint );
		if (center)
		{
			clientPoint.x -= width / 2;
			clientPoint.y -= height / 2;
			clientPoint.x = clientPoint.x >= 0 ? clientPoint.x : 0;
			clientPoint.y = clientPoint.y >= 0 ? clientPoint.y : 0;
		}
		formRect.TopLeft() = clientPoint;
		formRect.BottomRight() = CPoint(formRect.TopLeft().x + width, formRect.TopLeft().y + height);
		diag.MoveWindow(formRect, TRUE); 
	}
};

class CBSTStringDlg : public CStringDlg
{
public:
	CBSTStringDlg( const char *title = NULL,CWnd* pParent = NULL) : CStringDlg(title, pParent) {}
protected:
	virtual BOOL OnInitDialog()
	{
		BOOL res = CStringDlg::OnInitDialog();
		CDialogPositionHelper::SetToMouseCursor(*this, true);
		return res;
	}
};


#endif __DialogCommon_H__