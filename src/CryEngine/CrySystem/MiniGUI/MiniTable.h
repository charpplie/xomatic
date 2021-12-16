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

#ifndef _MINITABLE_H_
#define _MINITABLE_H_

#include "MiniGUI.h"

MINIGUI_BEGIN

class CMiniTable : public CMiniCtrl
{
public:
	CMiniTable();
	
	//////////////////////////////////////////////////////////////////////////
	// CMiniCtrl interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual EMiniCtrlType GetType() const { return eCtrlType_Table; }
	virtual void OnPaint( CDrawContext &dc );
	virtual void OnEvent( float x,float y,EMiniCtrlEvent event );
	virtual void Reset();
	virtual void SaveState();
	virtual void RestoreState();
	virtual void AutoResize();
	//////////////////////////////////////////////////////////////////////////

	//Add a new column to the table, return column index
	int AddColumn(const char* name);

	//Add data to specified column (add onto the end), return row index
	int AddData(int columnIndex, const char *format, ...);

	//Clear all data from table
	void ClearTable();
	
	static const int MAX_TEXT_LENGTH = 64;

protected:

	struct SCell
	{
		char text[MAX_TEXT_LENGTH];
	};

	struct SColumn 
	{
		char name[MAX_TEXT_LENGTH];
		float width;
		std::vector<SCell> cells;		
	};

	std::vector<SColumn> m_columns;
};

MINIGUI_END

#endif// _MINITABLE_H_