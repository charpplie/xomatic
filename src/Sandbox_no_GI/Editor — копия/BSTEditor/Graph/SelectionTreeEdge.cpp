////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:   SelectionTreeEdge.cpp
//  Version:     v1.00
//  Created:     20/12/2010 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SelectionTreeEdge.h"

#include "BSTEditor/Graph/Nodes/SelectionTree_BaseNode.h"

CSelectionTreeEdge::CSelectionTreeEdge()
: m_pConditionProvider( NULL )
{

}

CSelectionTreeEdge::~CSelectionTreeEdge()
{

}

int	CSelectionTreeEdge::GetCustomSelectionMode()
{
	// TODO: Remove magic number!
	return 7;
}

void CSelectionTreeEdge::DrawSpecial( Gdiplus::Graphics* pGraphics, Gdiplus::PointF point )
{
	if ( m_pConditionProvider == NULL )
	{
		return;
	}

	Gdiplus::Font font( L"Tahoma", 9.0f );

	Gdiplus::SolidBrush brush( Gdiplus::Color( 0, 0, 0 ) );

	CStringW condition;
	//condition = CString( m_pConditionProvider->GetCondition() );
	pGraphics->DrawString( condition, -1, &font, point, &brush );
}

void CSelectionTreeEdge::SetConditionProvider( CSelectionTree_BaseNode* pConditionProvider )
{
	m_pConditionProvider = pConditionProvider;
}