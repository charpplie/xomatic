////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2009.
// -------------------------------------------------------------------------
//  File name:   VisualBudgetTool.h
//  Created:     6/10/2009 by Paulo Zaffari.
//  Description: This class implements the tool which gathers the position
//               of the terrain around which we want to analyze the budget.
//							 One of the uses for this is selecting which region we want
//							 to analyze in terms of memory by having a detailed report.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef VisualBudgetTool_h__
#define VisualBudgetTool_h__

#pragma once


// {FE07E5E0-D83D-41d1-88DE-C351B88FCED8}
DEFINE_GUID(VISUAL_BUDGET_MODE_GUID,	0xfe07e5e0, 0xd83d, 0x41d1, 0x88, 0xde, 0xc3, 0x51, 0xb8, 0x8f, 0xce, 0xd8);

#include "..\EditTool.h"

class CVisualBudgetMode : public CEditTool
{
public:
	//////////////////////////////////////////////////////////////////////////
	// Types & typedefs
	//////////////////////////////////////////////////////////////////////////
	class CVisualBudgetModeClassDesc : public CRefCountClassDesc
	{
		//! This method returns an Editor defined GUID describing the class this plugin class is associated with.
		virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_EDITTOOL; }

		//! Return the GUID of the class created by plugin.
		virtual REFGUID ClassID() 
		{
			return VISUAL_BUDGET_MODE_GUID;
		}

		//! This method returns the human readable name of the class.
		virtual const char* ClassName() { return "EditTool.VisualBudgetMode"; };

		//! This method returns Category of this class, Category is specifing where this plugin class fits best in
		//! create panel.
		virtual const char* Category() { return "Select"; };
		virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CVisualBudgetMode); }
		//////////////////////////////////////////////////////////////////////////
	};
	//////////////////////////////////////////////////////////////////////////

	DECLARE_DYNCREATE(CVisualBudgetMode);

	CVisualBudgetMode();
	virtual ~CVisualBudgetMode();

	// Registration function.
	static void RegisterTool( CRegistrationContext &rc );

	//////////////////////////////////////////////////////////////////////////
	// CEditTool implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual void BeginEditParams( IEditor *ie,int flags ) {};
	virtual void EndEditParams() {};
	virtual void Display( struct DisplayContext &dc );

	virtual bool MouseCallback( CViewport *view,EMouseEvent event,CPoint &point,int flags );
	virtual bool OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags );
	virtual bool OnKeyUp( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags );
	virtual bool OnSetCursor( CViewport *vp ) { return false; };

protected:
	//////////////////////////////////////////////////////////////////////////
	// IEditorNotifyListener
	//////////////////////////////////////////////////////////////////////////
	void OnEditorNotifyEvent( EEditorNotifyEvent event );
	//////////////////////////////////////////////////////////////////////////

	enum ECommandMode
	{
		NothingMode = 0,
		SelectMode,
	};

	bool OnLButtonDown( CViewport *view,int nFlags, CPoint point );
	bool OnLButtonDblClk( CViewport *view,int nFlags, CPoint point);
	bool OnLButtonUp( CViewport *view,int nFlags, CPoint point );
	bool OnRButtonDown( CViewport *view,int nFlags, CPoint point );
	bool OnRButtonUp( CViewport *view,int nFlags, CPoint point );
	bool OnMButtonDown( CViewport *view,int nFlags, CPoint point );
	bool OnMouseMove( CViewport *view,int nFlags, CPoint point );
	bool CheckVirtualKey( int virtualKey );
	void SetCommandMode( ECommandMode mode ) { m_commandMode = mode; }
	ECommandMode GetCommandMode() const { return m_commandMode; }

	//! Ctrl-Click in move mode to move selected objects to given pos.
	void MoveSelectionToPos( CViewport *view,Vec3 &pos );

	virtual void DeleteThis() { delete this; };

	void UpdateStatusText();

	bool CalculateHeightmapPointFromDisplayPoint(const CPoint& rstDisplayPoint,Vec3& rstHeightmapPoint);
private:
	CPoint					m_cMouseDownPos;
	CPoint					m_cCurrentMousePos;
	ECommandMode		m_commandMode;

	bool						m_openContext;
	
	AABB						m_stSelectedAABB;
	bool						m_boAABBSelectionStart;
	bool						m_boAABBDragSelection;
	std::vector<AABB>	m_regionsBackup;
};

#endif // VisualBudgetTool_h__
