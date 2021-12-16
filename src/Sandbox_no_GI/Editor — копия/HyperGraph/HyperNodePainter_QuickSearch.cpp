////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File name:   HyperNodePainter_QuickSearch.cpp
//  Version:     v1.00
//  Created:     19/2/2011 by Sascha Hoba.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "HyperNodePainter_QuickSearch.h"
#include "QuickSearchNode.h"

namespace
{

	struct SAssets
	{
		SAssets() :
			font(L"Tahoma", 10.0f, Gdiplus::FontStyleBold),
			brushBackground( Gdiplus::Color(145,145,145) ),
			brushBackgroundSelected( Gdiplus::Color(145,145,145) ),
			brushTextWhite( Gdiplus::Color(255,255,255) ),
			brushTextBlack( Gdiplus::Color(0,0,0) ),
			penBorder( Gdiplus::Color(0,0,0) )
		{
			sf.SetAlignment( Gdiplus::StringAlignmentCenter );
			sf.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);
			sf.SetTrimming(Gdiplus::StringTrimmingEllipsisPath);
		}
		~SAssets()
		{
		}

		Gdiplus::Font font;
		Gdiplus::SolidBrush brushBackground;
		Gdiplus::SolidBrush brushBackgroundSelected;
		Gdiplus::SolidBrush brushTextWhite;
		Gdiplus::SolidBrush brushTextBlack;
		Gdiplus::StringFormat sf;
		Gdiplus::Pen penBorder;
	};

}

void CHyperNodePainter_QuickSearch::Paint( CHyperNode * pNode, CDisplayList * pList )
{
	static SAssets * pAssets = 0;
	if (!pAssets)
		pAssets = new SAssets();

	CDisplayRectangle * pBackground = pList->Add<CDisplayRectangle>()
		->SetHitEvent(eSOID_ShiftFirstOutputPort)
		->SetStroked(&pAssets->penBorder);

	CString text = pNode->GetName();
	text.Replace('_', ' ');
	text.Replace( "\\n","\r\n" );

	CDisplayString * pString = pList->Add<CDisplayString>()
		->SetHitEvent(eSOID_ShiftFirstOutputPort)
		->SetText( text )
		->SetBrush( &pAssets->brushTextWhite )
		->SetFont( &pAssets->font )
		->SetStringFormat( &pAssets->sf );

	pBackground->SetFilled(&pAssets->brushBackgroundSelected);
	
	CQuickSearchNode* pQuickSearchNode = static_cast<CQuickSearchNode*>(pNode);
	CString resultCount = "";
	resultCount.Format("%d / %d", pQuickSearchNode->GetIndex(), pQuickSearchNode->GetSearchResultCount());
	CDisplayString * pSearchCountString = pList->Add<CDisplayString>()
		->SetHitEvent(eSOID_ShiftFirstOutputPort)
		->SetText( resultCount )
		->SetBrush( &pAssets->brushTextBlack )
		->SetFont( &pAssets->font )
		->SetStringFormat( &pAssets->sf );

	Gdiplus::RectF rect;
	rect.Width = 200.0f;
	rect.Height = 50.0f;
	rect.X = rect.Y = 0.0f;
	pBackground->SetRect(rect);
	rect.Y = 13.0f;

	Gdiplus::RectF rectString = pString->GetBounds(pList->GetGraphics());
	
	if(rectString.Width > rect.Width)
	{
		text.Delete(0, text.ReverseFind(':'));
		CString trimmedText = "...";
		trimmedText.Append(text);
		pString->SetText(trimmedText);
	}

	pString->SetRect(rect);
	pString->SetLocation( Gdiplus::PointF( 100.0f, 32.0f ) );
	pSearchCountString->SetLocation( Gdiplus::PointF( 100.0f, 32.0f ) );
	pBackground->SetHitEvent( eSOID_Title );
	pNode->SetRect(rect);

	Gdiplus::PointF start(0.0f, 32.0f);
	Gdiplus::PointF end(200.0f, 32.0f);

	pList->AddLine(start, end,  &pAssets->penBorder);
}
