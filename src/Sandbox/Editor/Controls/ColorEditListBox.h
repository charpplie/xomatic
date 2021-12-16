//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2010.
// -------------------------------------------------------------------------
//  File Name        : ColorEditListBox.h
//  Author           : Jaewon Jung
//  Time of creation : 7/2/2010   15:09
//  Compilers        : VS2008
//  Description      : ListBox control with text color customization support 
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////

#ifndef __COLOREDITLISTBOX_H__
#define __COLOREDITLISTBOX_H__
#pragma once



class CColorEditListBox : public CXTEditListBox
{
	DECLARE_DYNAMIC(CColorEditListBox)
public:
	CColorEditListBox();

	typedef Functor1wRet<int, COLORREF> ColorCallback;
	void SetColorCallback(ColorCallback colorCB)
	{ m_colorCB = colorCB; }

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void DrawItem(LPDRAWITEMSTRUCT /*lpDrawItemStruct*/);

private:
	ColorCallback m_colorCB;
	COLORREF m_defaultColor;
};

#endif // __COLOREDITLISTBOX_H__