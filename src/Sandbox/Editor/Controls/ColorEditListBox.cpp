//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2010.
// -------------------------------------------------------------------------
//  File Name        : ColorEditListBox.cpp
//  Author           : Jaewon Jung
//  Time of creation : 7/2/2010   15:09
//  Compilers        : VS2008
//  Description      : ListBox control with text color customization support
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ColorEditListBox.h"

IMPLEMENT_DYNAMIC(CColorEditListBox, CXTEditListBox)

BEGIN_MESSAGE_MAP(CColorEditListBox, CXTEditListBox)
	ON_WM_DRAWITEM_REFLECT()
END_MESSAGE_MAP()

CColorEditListBox::CColorEditListBox()
					: m_colorCB(NULL), m_defaultColor(RGB(0,0,0)), CXTEditListBox()
{
}

void CColorEditListBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	//CRect members to store the position of the items
	CRect rItem;
	CDC* dc = CDC::FromHandle(lpDrawItemStruct->hDC);

	if ((int)lpDrawItemStruct->itemID < 0)
	{
		// If there are no elements in the CListBox
		// based on whether the list box has Focus  or not
		// draw the Focus Rect or Erase it,
		if ((lpDrawItemStruct->itemAction & ODA_FOCUS) && (lpDrawItemStruct->itemState & ODS_FOCUS))
		{
			dc->DrawFocusRect(&lpDrawItemStruct->rcItem);
		}
		else if ((lpDrawItemStruct->itemAction & ODA_FOCUS) && !(lpDrawItemStruct->itemState & ODS_FOCUS))
		{
			dc->DrawFocusRect(&lpDrawItemStruct->rcItem);
		}
		return;
	}


	// String to store the text
	CString strText;

	// Get the item text.
	GetText(lpDrawItemStruct->itemID, strText);

	//Initialize the CListBox Item's row size
	rItem = lpDrawItemStruct->rcItem;

	UINT nFormat = DT_LEFT | DT_SINGLELINE | DT_VCENTER;
	if (GetStyle() & LBS_USETABSTOPS)
		nFormat |= DT_EXPANDTABS;


	// If CListBox item selected, draw the highlight rectangle.
	// Or if CListBox item deselected, draw the rectangle using the window color.
	if ((lpDrawItemStruct->itemState & ODS_SELECTED) &&
		(lpDrawItemStruct->itemAction & (ODA_SELECT | ODA_DRAWENTIRE)))
	{
		CBrush br(::GetSysColor(COLOR_HIGHLIGHT));
		dc->FillRect(&rItem, &br);
	}
	else if (!(lpDrawItemStruct->itemState & ODS_SELECTED) && (lpDrawItemStruct->itemAction & ODA_SELECT))
	{
		CBrush br(::GetSysColor(COLOR_WINDOW));
		dc->FillRect(&rItem, &br);
	}

	// If the CListBox item has focus, draw the focus rect.
	// If the item does not have focus, erase the focus rect.
	if ((lpDrawItemStruct->itemAction & ODA_FOCUS) && (lpDrawItemStruct->itemState & ODS_FOCUS))
	{
		dc->DrawFocusRect(&rItem);
	}
	else if ((lpDrawItemStruct->itemAction & ODA_FOCUS) && !(lpDrawItemStruct->itemState & ODS_FOCUS))
	{
		dc->DrawFocusRect(&rItem);
	}

	// To draw the Text in the CListBox set the background mode to Transparent.
	int iBkMode = dc->SetBkMode(TRANSPARENT);

	//COLORREF crText;
	//CFont font;
	//font.CreatePointFont(100,"Times New Roman");

	COLORREF textColor = m_defaultColor;
	if(m_colorCB)
		textColor = m_colorCB(int(lpDrawItemStruct->itemID));
	dc->SetTextColor(textColor);
	//dc->SelectObject(&font);

	//Draw the Text
	dc->TextOut(rItem.left,rItem.top,strText);
}