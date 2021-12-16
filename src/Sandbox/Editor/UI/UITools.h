////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UITools.h
//  Version:     v1.00
//  Created:     11/10/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////

#ifndef __UITools_H__
#define __UITools_H__

#include "Dialogs/ButtonsPanel.h"
#include "UIRollupView.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
class CUIToolButtons : public CButtonsPanel
{
public:
	CUIToolButtons();

	enum { IDD = IDD_UITOOLS };
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
class CUIBaseTool
{
public:
	virtual ~CUIBaseTool() {}

	virtual void OpenTool() = 0;
	virtual void CloseTool() = 0;
	virtual void Update() = 0;

protected:
	void AddPanel( const char* category, CDialog* panel, bool bExpanded = true );
	void ClearPanels();

	virtual CUIRollupControl* GetRollupControl() = 0;

private:
	typedef std::map<int, CDialog*> TPanelMap;
	TPanelMap m_panels;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
class CUITool : public CEditTool
{
public:
	DECLARE_DYNCREATE(CUITool)

	CUITool();
	~CUITool();

	//////////////////////////////////////////////////////////////////////////
	// Ovverides from CEditTool
	virtual void SetUserData( const char *key,void *userData );
	bool MouseCallback( CViewport *view,EMouseEvent event,CPoint &point,int flags ) { return true; };
	bool Activate( CEditTool *pPreviousTool ) { return true; };

	virtual void BeginEditParams( IEditor *ie,int flags );
	virtual void EndEditParams();

	virtual void Display( DisplayContext &dc ) {};
	virtual bool OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags );
	virtual bool OnKeyUp( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags ) { return false; };
	virtual bool OnSetCursor( CViewport *vp );
	//////////////////////////////////////////////////////////////////////////

	void CloseTools();
	void UpdateTools();
protected:
	// Delete itself.
	void DeleteThis() { delete this; };

private:
	void SelectCategory( const CString &category );

private:
	typedef std::map<CString, CUIBaseTool*> TToolMap;
	TToolMap m_Tools;
};

#endif // __UITools_H__
