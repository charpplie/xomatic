////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek.
// -------------------------------------------------------------------------
//  File name:   MiniTable.cpp
//  Created:     01/12/2009 by AndyM.
//  Description: Table implementation in the MiniGUI
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include <StdAfx.h>
#include "MiniTable.h"
#include "DrawContext.h"

MINIGUI_BEGIN

CMiniTable::CMiniTable()
{
	m_fTextSize = 15.f;
}

void CMiniTable::OnPaint( CDrawContext &dc )
{
	if(m_requiresResize)
	{
		AutoResize();
	}

	ColorB borderCol;
	ColorB backgroundCol = dc.Metrics().clrBackground;

	if(m_pGUI->InFocus())
	{
		if(m_moving)
		{
			borderCol =dc.Metrics().clrFrameBorderHighlight;
		}
		else
		{
			borderCol = dc.Metrics().clrFrameBorder;
		}
	}
	else
	{
		borderCol = dc.Metrics().clrFrameBorderOutOfFocus;
		backgroundCol.a = 127;
	}
	
	dc.DrawFrame( m_rect, borderCol, backgroundCol );
	
	float fTextSize = m_fTextSize;
	if (fTextSize == 0)
		fTextSize = dc.Metrics().fTextSize;

	float indent = 4.f;

	float x = m_rect.left + indent;

	const int nColumns = m_columns.size();

	for(int i=0; i < nColumns; i++)
	{
		float y = m_rect.top + indent;

		SColumn &column = m_columns[i];

		dc.SetColor( ColorB(32,32,255,255) );

		dc.DrawString( x,y, fTextSize, eTextAlign_Left, column.name);
		y += fTextSize+indent;
		
		dc.SetColor( ColorB(255,255,255,255) );

		const int nCells = column.cells.size();

		for( int j=0; j<nCells; j++ )
		{
			dc.DrawString( x,y, fTextSize, eTextAlign_Left, column.cells[j].text);
			y += fTextSize;
		}

		x+=column.width;
	}
}

void CMiniTable::OnEvent( float x,float y,EMiniCtrlEvent event )
{
	//movement
	CMiniCtrl::OnEvent(x,y,event);
}

void CMiniTable::AutoResize()
{
	const int nColumns = m_columns.size();
	
	//must be at least the size of cross box
	float tableWidth = 30.f;

	float tableHeight = 0.f;
	const float widthScale = 0.7f;

	for(int i=0; i<nColumns; i++)
	{
		SColumn &column = m_columns[i];

		float width = strlen(column.name)*m_fTextSize;
		
		int nCells = column.cells.size();

		for(int j=0; j<nCells; j++)
		{
			width = max(width, strlen(column.cells[j].text)*m_fTextSize);
		}

		width *= widthScale;

		column.width = width;
		tableWidth+=width;
		tableHeight = max(tableHeight, nCells*m_fTextSize);
	}

	tableHeight += m_fTextSize*2;

	Rect newRect = m_rect;

	newRect.right = newRect.left+tableWidth;
	newRect.bottom = newRect.top+tableHeight;

	SetRect(newRect);

	m_requiresResize = false;
}

void CMiniTable::Reset()
{
}

void CMiniTable::SaveState()
{
}

void CMiniTable::RestoreState()
{
}

int CMiniTable::AddColumn(const char *name)
{
	assert(name);

	SColumn col;

	int length = strlen(name);
	strncpy(col.name, name, length+1);
	col.width = length * 8.f;

	m_columns.push_back(col);

	m_requiresResize=true;

	return m_columns.size()-1;
}

int CMiniTable::AddData(int columnIndex, const char *format, ...)
{
	assert(columnIndex<m_columns.size());

	SCell cell;

	va_list args;
	va_start(args, format);
	int written = vsnprintf_s(cell.text, MAX_TEXT_LENGTH, MAX_TEXT_LENGTH-1, format, args);
	va_end(args);

	if(written==-1)
	{
		cell.text[MAX_TEXT_LENGTH-1] = '\0';
	}

	m_columns[columnIndex].cells.push_back(cell);

	m_requiresResize=true;

	return m_columns[columnIndex].cells.size()-1;

}

void CMiniTable::ClearTable()
{
	const int nColumns = m_columns.size();

	for(int i=0; i<nColumns; i++)
	{
		m_columns[i].cells.clear();
	}
}

MINIGUI_END
