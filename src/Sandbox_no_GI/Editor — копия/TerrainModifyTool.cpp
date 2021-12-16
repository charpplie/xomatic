////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001.
// -------------------------------------------------------------------------
//  File name:   TerrainModifyTool.cpp
//  Version:     v1.00
//  Created:     11/1/2002 by Timur.
//  Compilers:   Visual C++ 6.0
//  Description: Terrain Modification Tool implementation.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "INITGUID.H"
#include "TerrainModifyTool.h"
#include "TerrainPanel.h"
#include "Viewport.h"
#include "TerrainModifyPanel.h"
#include ".\Terrain\Heightmap.h"
#include "VegetationMap.h"
#include "Objects\DisplayContext.h"
#include "Objects\ObjectManager.h"

//////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CTerrainModifyTool,CEditTool)

CTerrainBrush CTerrainModifyTool::m_brush[eBrushTypeLast];
BrushType			CTerrainModifyTool::m_currentBrushType = eBrushFlatten;

//////////////////////////////////////////////////////////////////////////
CTerrainModifyTool::CTerrainModifyTool()
{
	m_pClassDesc = GetIEditor()->GetClassFactory()->FindClass(TERRAIN_MODIFY_TOOL_GUID);

	SetStatusText( _T("Modify Terrain Heightmap") );
	m_panelId = 0;
	m_panel = 0;
	m_prevBrush = 0;

	m_bSmoothOverride = false;
	m_bQueryHeightMode = false;
	m_bPaintingMode = false;

	m_isCtrlPressed = false;
	m_isShiftPressed = false;

	m_brush[eBrushFlatten].type = eBrushFlatten;
	m_brush[eBrushSmooth].type = eBrushSmooth;
	m_brush[eBrushRiseLower].type = eBrushRiseLower;
	m_brush[eBrushPickHeight].type = eBrushPickHeight;
	m_pBrush = &m_brush[m_currentBrushType];
	
	m_pointerPos(0,0,0);
	GetIEditor()->ClearSelection();

	if (m_pBrush->height < 0)
	{
		// Initialize brush height to water level.
		m_pBrush->height = GetIEditor()->GetHeightmap()->GetWaterLevel();
	}
	m_brush[eBrushFlatten].heightRange.y = GetIEditor()->GetHeightmap()->GetMaxHeight();
	m_brush[eBrushRiseLower].heightRange.x = -50;
	m_brush[eBrushRiseLower].heightRange.y =  50;
	m_brush[eBrushPickHeight].radius=0;
	m_brush[eBrushPickHeight].hardness=0;

	m_hPickCursor = AfxGetApp()->LoadCursor( IDC_POINTER_GET_HEIGHT );
	m_hPaintCursor = AfxGetApp()->LoadCursor( IDC_HAND_INTERNAL );
	m_hFlattenCursor = AfxGetApp()->LoadCursor( IDC_POINTER_FLATTEN );
	m_hSmoothCursor = AfxGetApp()->LoadCursor( IDC_POINTER_SMOOTH );
}

//////////////////////////////////////////////////////////////////////////
CTerrainModifyTool::~CTerrainModifyTool()
{
	m_pointerPos(0,0,0);

	if (GetIEditor()->IsUndoRecording())
		GetIEditor()->CancelUndo();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyTool::BeginEditParams( IEditor *ie,int flags )
{
	m_ie = ie;
	if (!m_panelId && !m_panel)
	{
		m_panel = new CTerrainModifyPanel(this,AfxGetMainWnd());
		m_panelId = m_ie->AddRollUpPage( ROLLUP_TERRAIN,"Modify Terrain",m_panel );
		AfxGetMainWnd()->SetFocus();

		UpdateUI();
		GetIEditor()->UpdateViews(eUpdateHeightmap);
	}
}

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyTool::EndEditParams()
{
	if (m_panelId)
	{
		m_ie->RemoveRollUpPage(ROLLUP_TERRAIN,m_panelId);
		m_panel = 0;
		m_panelId = 0;
	}
}

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyTool::SetExternalUIPanel( class CTerrainModifyPanel *pPanel )
{
	assert(pPanel);
	EndEditParams();
	m_panel = pPanel;
	UpdateUI();
}

//////////////////////////////////////////////////////////////////////////
bool CTerrainModifyTool::MouseCallback( CViewport *view,EMouseEvent event,CPoint &point,int flags )
{
	m_bSmoothOverride = false;
	if (flags & MK_SHIFT)
	{
		m_bSmoothOverride = true;
	}
	m_bPaintingMode = false;
	m_bQueryHeightMode = false;

	bool bPainting = false;
	bool bCollideWithTerrain = true;
	m_pointerPos = view->ViewToWorld( point,&bCollideWithTerrain,true );
	if ((event == eMouseLDown || (event == eMouseMove && (flags&MK_LBUTTON))) && m_pBrush->type==eBrushPickHeight)
	{
		m_pBrush->height = GetIEditor()->GetTerrainElevation(m_pointerPos.x,m_pointerPos.y);

		char str[256];
		sprintf(str,"Picked height: %f\n", m_pBrush->height);
		OutputDebugString(str);

		UpdateUI();
	}
	else if (event == eMouseLDown && bCollideWithTerrain)
	{
		if (!GetIEditor()->IsUndoRecording())
			GetIEditor()->BeginUndo();
		Paint();
		bPainting = true;
	}

	//if (event == eMouseMove && (flags&MK_LBUTTON) && !(flags&MK_CONTROL) && !(flags&MK_SHIFT))
	if (event == eMouseMove && (flags&MK_LBUTTON) && !m_bQueryHeightMode && bCollideWithTerrain)
	{
		Paint();
		bPainting = true;
	}

	if (event == eMouseMDown)
	{
		// When middle mouse button down.
		m_MButtonDownPos = point;
		m_prevRadius = m_pBrush->radius;
		m_prevRadiusInside = m_pBrush->radiusInside;
		m_prevHeight = m_pBrush->height;
	}
	if (event == eMouseMove && (flags&MK_MBUTTON) && (flags&MK_SHIFT) && !bPainting)
	{
		// Change brush radius.
		CPoint p = point - m_MButtonDownPos;
		//float dist = sqrtf(p.x*p.x + p.y*p.y);
		m_pBrush->radius = m_prevRadius + p.x * 0.1f;
		m_pBrush->radiusInside = m_prevRadiusInside - p.y * 0.1f;
		if (m_pBrush->radiusInside > m_pBrush->radius)
			m_pBrush->radiusInside = m_pBrush->radius;
		UpdateUI();
	}
	if (event == eMouseMove && (flags&MK_MBUTTON) && (flags&MK_CONTROL) && !bPainting)
	{
		// Change brush radius.
		CPoint p = point - m_MButtonDownPos;
		float dist = sqrtf(p.x*p.x + p.y*p.y);
		m_pBrush->height = m_prevHeight - p.y * 0.1f;
		if (m_pBrush->height < 0)
			m_pBrush->height = 0;
		UpdateUI();
	}

	m_bPaintingMode = bPainting;

	if (!bPainting && GetIEditor()->IsUndoRecording())
	{
		GetIEditor()->AcceptUndo( "Terrain Modify" );
	}

	// Show status help.
	if (m_pBrush->type == eBrushRiseLower)
		GetIEditor()->SetStatusText( "CTRL: Inverse Height  SHIFT: Smooth  LMB: Rise/Lower/Smooth  +-: Change Brush Radius  */: Change Height" );
	else
		GetIEditor()->SetStatusText( "CTRL: Query Height  SHIFT: Smooth  LMB: Flatten/Smooth  +-: Change Brush Radius  */: Change Height" );

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyTool::Display( DisplayContext &dc )
{
	CTerrainBrush * pBrush = m_pBrush;
	if(m_bSmoothOverride)
		pBrush = &m_brush[eBrushSmooth];

	if (dc.view)
	{
		// Check if mouse cursor is out of viewport
		CPoint cursor;
		GetCursorPos(&cursor);
		dc.view->ScreenToClient(&cursor);
		CRect rc;
		dc.view->GetClientRect(rc);
		if (cursor.x < rc.left || cursor.y < rc.top || cursor.x > rc.right || cursor.y > rc.bottom)
			return;
	}

	if (pBrush->type != eBrushSmooth)
	{
		dc.SetColor( 0.5f,1,0.5f,1 );
		dc.DrawTerrainCircle( m_pointerPos,pBrush->radiusInside,0.2f );
	}
	dc.SetColor( 0,1,0,1 );
	dc.DrawTerrainCircle( m_pointerPos,pBrush->radius,0.2f );
	if (m_pointerPos.z < pBrush->height)
	{
		if (pBrush->type != eBrushSmooth)
		{
			Vec3 p = m_pointerPos;
			p.z = pBrush->height;
			dc.SetColor( 1,1,0,1 );
			if (pBrush->type == eBrushFlatten)
				dc.DrawTerrainCircle( p,pBrush->radiusInside,pBrush->height-m_pointerPos.z );
			dc.DrawLine( m_pointerPos,p );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
bool CTerrainModifyTool::OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{
	bool bProcessed = false;
	if (nChar == VK_MULTIPLY)
	{
		BrushType eBrushType = m_pBrush->type;
		if (m_bSmoothOverride)
			eBrushType = eBrushSmooth;
		if (eBrushType == eBrushSmooth)
			m_brush[eBrushType].hardness += 0.05f;
		else
			m_brush[eBrushType].height += 1;
		bProcessed = true;
	}
	if (nChar == VK_DIVIDE)
	{
		BrushType eBrushType = m_pBrush->type;
		if (m_bSmoothOverride)
			eBrushType = eBrushSmooth;
		if (eBrushType == eBrushSmooth)
		{
			if (m_brush[eBrushType].hardness > 0)
				m_brush[eBrushType].hardness -= 0.05f;
		}
		else
		{
			if (m_brush[eBrushType].height > 0)
				m_brush[eBrushType].height -= 1;
		}
		bProcessed = true;
	}
	if (nChar == VK_ADD)
	{
		CTerrainBrush* pBrush = m_pBrush;
		if(m_bSmoothOverride)
			pBrush = &m_brush[eBrushSmooth];
		pBrush->radius += 1;
		bProcessed = true;
	}
	if (nChar == VK_SUBTRACT)
	{
		CTerrainBrush* pBrush = m_pBrush;
		if(m_bSmoothOverride)
			pBrush = &m_brush[eBrushSmooth];
		if (pBrush->radius > 1)
			pBrush->radius -= 1;
		bProcessed = true;
	}
	if (nChar == VK_SHIFT && !m_isShiftPressed)
	{
		m_isShiftPressed = true;
		SetCurBrushType(eBrushSmooth);
		bProcessed = true;
	}
	if (nChar == VK_CONTROL && !m_isCtrlPressed )
	{
		if(m_pBrush->type==eBrushFlatten)
		{
			SetCurBrushType(eBrushPickHeight);
			bProcessed = true;
		}
		else if(m_pBrush->type==eBrushRiseLower)
		{
			m_pBrush->height = -m_pBrush->height;
			bProcessed = true;
		}
		m_isCtrlPressed = true;
	}
	if (bProcessed)
	{
		UpdateUI();
	}
	return bProcessed;
}

//////////////////////////////////////////////////////////////////////////
bool CTerrainModifyTool::OnKeyUp( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{
	if (nChar == VK_SHIFT && m_prevBrush)
	{
		m_isShiftPressed = false;
		if(m_prevBrush)
			SetCurBrushType(m_prevBrush->type);
		return true;
	}

	if (nChar == VK_CONTROL)
	{
		m_isCtrlPressed = false;
		if(m_prevBrush)
		{
			if(m_pBrush->type==eBrushPickHeight && m_prevBrush->type!=eBrushPickHeight)
			{
				SetCurBrushType(m_prevBrush->type);
				return true;
			}
			else if(m_pBrush->type==eBrushRiseLower)
			{
				m_pBrush->height = -m_pBrush->height;
				UpdateUI();
				return true;
			}
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyTool::SetCurBrushParams( const CTerrainBrush &brush, bool isSyncRadius )
{
	*m_pBrush = brush;

	if(isSyncRadius)
	{
		m_brush[eBrushFlatten].radius = brush.radius;
		m_brush[eBrushSmooth].radius = brush.radius;
		m_brush[eBrushRiseLower].radius = brush.radius;
	}

	UpdateUI();
}


//////////////////////////////////////////////////////////////////////////
void CTerrainModifyTool::UpdateUI()
{
	if (m_panel)
		m_panel->SetBrush(m_pBrush);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyTool::Paint()
{
	CTerrainBrush * pBrush = m_pBrush;
	if(m_bSmoothOverride)
		pBrush = &m_brush[eBrushSmooth];

	CHeightmap *heightmap = GetIEditor()->GetHeightmap();
	int unitSize = heightmap->GetUnitSize();

	//dc.renderer->SetMaterialColor( 1,1,0,1 );
	int tx = RoundFloatToInt(m_pointerPos.y / unitSize);
	int ty = RoundFloatToInt(m_pointerPos.x / unitSize);

	float fInsideRadius = (pBrush->radiusInside / unitSize);
	int tsize = (pBrush->radius / unitSize);
	if (tsize == 0)
		tsize = 1;
	int tsize2 = tsize*2;
	int x1 = tx - tsize;
	int y1 = ty - tsize;

	if (pBrush->type == eBrushFlatten && !m_bSmoothOverride)
		heightmap->DrawSpot2( tx,ty,tsize,fInsideRadius,pBrush->height,pBrush->hardness,pBrush->bNoise,pBrush->noiseFreq/10.0f,pBrush->noiseScale/1000.0f );
	if (pBrush->type == eBrushRiseLower && !m_bSmoothOverride)
		heightmap->RiseLowerSpot( tx,ty,tsize,fInsideRadius,pBrush->height,pBrush->hardness,pBrush->bNoise,pBrush->noiseFreq/10.0f,pBrush->noiseScale/1000.0f );
	else if (pBrush->type == eBrushSmooth || m_bSmoothOverride)
		heightmap->SmoothSpot( tx,ty,tsize,pBrush->height,pBrush->hardness );//,m_pBrush->noiseFreq/10.0f,m_pBrush->noiseScale/10.0f );

	heightmap->UpdateEngineTerrain( x1,y1,tsize2,tsize2,true,false );

	AABB box;
	box.min = m_pointerPos - Vec3(pBrush->radius,pBrush->radius,0);
	box.max = m_pointerPos + Vec3(pBrush->radius,pBrush->radius,0);
	box.min.z -= 10000;
	box.max.z += 10000;

	GetIEditor()->UpdateViews(eUpdateHeightmap,&box);

	if (pBrush->bRepositionVegetation && GetIEditor()->GetVegetationMap())
		GetIEditor()->GetVegetationMap()->RepositionArea( box );

	// Make sure objects preserve height.
	if (pBrush->bRepositionObjects)
		GetIEditor()->GetObjectManager()->SendEvent( EVENT_KEEP_HEIGHT,box );
}


//////////////////////////////////////////////////////////////////////////
bool CTerrainModifyTool::OnSetCursor( CViewport *vp )
{
	if (m_pBrush->type == eBrushPickHeight)
	{
		vp->SetCursor( m_hPickCursor );
		return true;
	}
	if ((m_pBrush->type == eBrushFlatten || m_pBrush->type == eBrushRiseLower) && !m_bSmoothOverride)
	{
		vp->SetCursor( m_hFlattenCursor );
		return true;
	}
	else if (m_pBrush->type == eBrushSmooth || m_bSmoothOverride)
	{
		vp->SetCursor( m_hSmoothCursor );
		return true;
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
void CTerrainModifyTool::SetCurBrushType( BrushType type )
{
	CTerrainBrush * prevPrevBrush = m_prevBrush;
	m_prevBrush = m_pBrush;
	m_currentBrushType = type;
	m_pBrush = &m_brush[type];

	if(m_pBrush->type==eBrushPickHeight && m_prevBrush->type!=eBrushPickHeight)
	{
		m_pBrush->height = m_prevBrush->height;
	}
	if(prevPrevBrush && prevPrevBrush->type==eBrushFlatten && m_prevBrush->type==eBrushPickHeight && m_pBrush->type==eBrushFlatten)
	{
		m_pBrush->height = m_prevBrush->height;
	}

	UpdateUI();
}


//////////////////////////////////////////////////////////////////////////
void CTerrainModifyTool::Command_Activate()
{
	CEditTool *pTool = GetIEditor()->GetEditTool();
	if (pTool && pTool->IsKindOf(RUNTIME_CLASS(CTerrainModifyTool)))
	{
		// Already active.
		return;
	}
	pTool = new CTerrainModifyTool;
	GetIEditor()->SetEditTool( pTool );
	GetIEditor()->SelectRollUpBar( ROLLUP_TERRAIN );
	AfxGetMainWnd()->RedrawWindow(NULL,NULL,RDW_INVALIDATE|RDW_ALLCHILDREN);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyTool::Command_Flatten()
{
	Command_Activate();
	CEditTool *pTool = GetIEditor()->GetEditTool();
	if (pTool && pTool->IsKindOf(RUNTIME_CLASS(CTerrainModifyTool)))
	{
		CTerrainModifyTool *pModTool = (CTerrainModifyTool*)pTool;
		pModTool->SetCurBrushType(eBrushFlatten);
	}
}

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyTool::Command_Smooth()
{
	Command_Activate();
	CEditTool *pTool = GetIEditor()->GetEditTool();
	if (pTool && pTool->IsKindOf(RUNTIME_CLASS(CTerrainModifyTool)))
	{
		CTerrainModifyTool *pModTool = (CTerrainModifyTool*)pTool;
		pModTool->SetCurBrushType(eBrushSmooth);
	}
}

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyTool::Command_RiseLower()
{
	Command_Activate();
	CEditTool *pTool = GetIEditor()->GetEditTool();
	if (pTool && pTool->IsKindOf(RUNTIME_CLASS(CTerrainModifyTool)))
	{
		CTerrainModifyTool *pModTool = (CTerrainModifyTool*)pTool;
		pModTool->SetCurBrushType(eBrushRiseLower);
	}
}

//////////////////////////////////////////////////////////////////////////
// Class description.
//////////////////////////////////////////////////////////////////////////
class CTerrainModifyTool_ClassDesc : public CRefCountClassDesc
{
	//! This method returns an Editor defined GUID describing the class this plugin class is associated with.
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_EDITTOOL; }

	//! Return the GUID of the class created by plugin.
	virtual REFGUID ClassID() 
	{
		return TERRAIN_MODIFY_TOOL_GUID;
	}

	//! This method returns the human readable name of the class.
	virtual const char* ClassName() { return "EditTool.TerrainModify"; };

	//! This method returns Category of this class, Category is specifing where this plugin class fits best in
	//! create panel.
	virtual const char* Category() { return "Terrain"; };
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CTerrainModifyTool); }
	//////////////////////////////////////////////////////////////////////////
};

//////////////////////////////////////////////////////////////////////////
void CTerrainModifyTool::RegisterTool( CRegistrationContext &rc )
{
	rc.pClassFactory->RegisterClass( new CTerrainModifyTool_ClassDesc );
	
	rc.pCommandManager->RegisterCommand( "EditTool.TerrainModifyTool.Activate",functor(CTerrainModifyTool::Command_Activate) );
	rc.pCommandManager->RegisterCommand( "EditTool.TerrainModifyTool.Flatten",functor(CTerrainModifyTool::Command_Flatten) );
	rc.pCommandManager->RegisterCommand( "EditTool.TerrainModifyTool.Smooth",functor(CTerrainModifyTool::Command_Smooth) );
	rc.pCommandManager->RegisterCommand( "EditTool.TerrainModifyTool.RiseLower",functor(CTerrainModifyTool::Command_RiseLower) );
}
