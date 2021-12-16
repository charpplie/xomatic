////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001.
// -------------------------------------------------------------------------
//  File name:   TerrainMoveTool.cpp
//  Version:     v1.00
//  Created:     11/1/2002 by Timur.
//  Compilers:   Visual C++ 6.0
//  Description: Terrain Modification Tool implementation.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "TerrainPanel.h"
#include "TerrainMoveTool.h"
#include "TerrainMoveToolPanel.h"
#include "Viewport.h"
#include ".\Terrain\Heightmap.h"
#include "VegetationMap.h"
#include ".\Terrain\TerrainGrid.h"
#include "Objects\ObjectManager.h"

#include "ITransformManipulator.h"

#include "I3DEngine.h"

#define PI 3.141592653589793f

class CUndoTerrainMoveTool : public IUndoObject
{
public:
	CUndoTerrainMoveTool( CTerrainMoveTool *pMoveTool )
	{
		m_posSourceUndo = pMoveTool->m_source.pos;
		m_posTargetUndo = pMoveTool->m_target.pos;
	}
protected:
	virtual int GetSize() { return sizeof(*this); }
	virtual const char* GetDescription() { return ""; };

	virtual void Undo( bool bUndo )
	{
		CEditTool *pTool = GetIEditor()->GetEditTool();
		if (!pTool || !pTool->IsKindOf(RUNTIME_CLASS(CTerrainMoveTool)))
			return;
		CTerrainMoveTool *pMoveTool = (CTerrainMoveTool*)pTool;
		if (bUndo)
		{
			m_posSourceRedo = pMoveTool->m_source.pos;
			m_posTargetRedo = pMoveTool->m_target.pos;
		}
		pMoveTool->m_source.pos = m_posSourceUndo;
		pMoveTool->m_target.pos = m_posTargetUndo;
		if(pMoveTool->m_source.isSelected)
			pMoveTool->Select(1);
		if(pMoveTool->m_target.isSelected)
			pMoveTool->Select(2);
	}
	virtual void Redo()
	{
		CEditTool *pTool = GetIEditor()->GetEditTool();
		if (!pTool || !pTool->IsKindOf(RUNTIME_CLASS(CTerrainMoveTool)))
			return;
		CTerrainMoveTool *pMoveTool = (CTerrainMoveTool*)pTool;
		pMoveTool->m_source.pos = m_posSourceRedo;
		pMoveTool->m_target.pos = m_posTargetRedo;
		if(pMoveTool->m_source.isSelected)
			pMoveTool->Select(1);
		if(pMoveTool->m_target.isSelected)
			pMoveTool->Select(2);
	}
private:
	Vec3 m_posSourceRedo;
	Vec3 m_posTargetRedo;
	Vec3 m_posSourceUndo;
	Vec3 m_posTargetUndo;
};


//////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CTerrainMoveTool,CEditTool)

Vec3 CTerrainMoveTool::m_dym(512,512,1024);
int CTerrainMoveTool::m_targetRot = 0.0f;
//SMTBox CTerrainMoveTool::m_source;
//SMTBox CTerrainMoveTool::m_target;

//////////////////////////////////////////////////////////////////////////
CTerrainMoveTool::CTerrainMoveTool()
{
	m_pointerPos(0,0,0);
	m_archive = 0;

	m_panelId = 0;
	m_panel = 0;
}

//////////////////////////////////////////////////////////////////////////
CTerrainMoveTool::~CTerrainMoveTool()
{
	m_pointerPos(0,0,0);
	if (m_archive)
		delete m_archive;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMoveTool::BeginEditParams( IEditor *ie,int flags )
{
	m_ie = ie;

	CUndo undo( "Unselect All" );
	GetIEditor()->ClearSelection();

	if (!m_panelId)
	{
		m_panel = new CTerrainMoveToolPanel(this,AfxGetMainWnd());
		m_panelId = m_ie->AddRollUpPage( ROLLUP_TERRAIN,"Terrain Move Tool",m_panel );
		AfxGetMainWnd()->SetFocus();
	}

	if(m_source.isCreated)
		m_source.isShow = true;
	if(m_target.isCreated)
		m_target.isShow = true;

	//CPoint hmapSrcMin,hmapSrcMax;
	//hmapSrcMin = GetIEditor()->GetHeightmap()->WorldToHmap(srcBox.min);
	//hmapSrcMax = GetIEditor()->GetHeightmap()->WorldToHmap(srcBox.max);
	//m_srcRect.SetRect( hmapSrcMin,hmapSrcMax );
			
	//GetIEditor()->GetHeightmap()->ExportBlock( m_srcRect,*m_archive );
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMoveTool::EndEditParams()
{
	if (m_panelId)
	{
		m_ie->RemoveRollUpPage(ROLLUP_TERRAIN,m_panelId);
		m_panelId = 0;
		m_panel = 0;
	}
	Select(0);
}

//////////////////////////////////////////////////////////////////////////
bool CTerrainMoveTool::MouseCallback( CViewport *view,EMouseEvent event,CPoint &point,int flags )
{
	m_pointerPos = view->ViewToWorld( point,0,true );

	if (event == eMouseMove)
	{
		if(m_source.isSelected && !m_source.isCreated)
		{
			m_source.pos = view->ViewToWorld( point,0,true );
			Select(1);
		}
		if(m_target.isSelected && !m_target.isCreated)
		{
			m_target.pos = view->ViewToWorld( point,0,true );
			Select(2);
		}
	}
	else if (event == eMouseLDown)
	{
		// Close tool.
		if(!m_source.isSelected && !m_target.isSelected)
			GetIEditor()->SetEditTool(0);
	}
	else if (event == eMouseLUp)
	{
		if(m_source.isSelected && !m_source.isCreated)
		{
			m_source.isCreated = true;
			m_panel->UpdateButtons();
		}
		else if(m_target.isSelected && !m_target.isCreated)
		{
			m_target.isCreated = true;
			m_panel->UpdateButtons();
		}
		else
			GetIEditor()->AcceptUndo( "Move Box" );
		CorrectPosition();
	}
	else
	{
		
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMoveTool::Display( DisplayContext &dc )
{
	if(m_source.isShow)
	{
		dc.SetColor( RGB(255,128,128),1 );
		if(m_source.isSelected)
			dc.SetColor(dc.GetSelectedColor());
		dc.DrawWireBox(m_source.pos - m_dym/2, m_source.pos + m_dym/2);
	}

	if(m_target.isShow)
	{
		dc.SetColor( RGB(128,128,255),1 );
		if(m_target.isSelected)
			dc.SetColor(dc.GetSelectedColor());

		Matrix34 tm;
		tm.SetIdentity();
		if(m_targetRot)
		{
			float a=PI/2 * m_targetRot;
			tm.SetRotationZ(a);
		}
		tm.SetTranslation(m_target.pos);
		dc.PushMatrix( tm );
		dc.DrawWireBox( -m_dym/2, m_dym/2);
		dc.PopMatrix();
	}

	/*
	BBox box;
	GetIEditor()->GetSelectedRegion(box);

	Vec3 p1 = GetIEditor()->GetHeightmap()->HmapToWorld( CPoint(m_srcRect.left,m_srcRect.top) );
	Vec3 p2 = GetIEditor()->GetHeightmap()->HmapToWorld( CPoint(m_srcRect.right,m_srcRect.bottom) );

	Vec3 ofs = m_pointerPos - p1;
	p1 += ofs;
	p2 += ofs;

	dc.SetColor( RGB(0,0,255) );
	dc.DrawTerrainRect( p1.x,p1.y,p2.x,p2.y,0.2f );
	*/
}

//////////////////////////////////////////////////////////////////////////
bool CTerrainMoveTool::OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{
	bool bProcessed = false;
	return bProcessed;
}

bool CTerrainMoveTool::OnKeyUp( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMoveTool::Move(bool isCopy, bool bOnlyVegetation, bool bOnlyTerrain)
{
	// Move terrain area.
	CUndo undo( "Copy Area" );
	CWaitCursor wait;

	CHeightmap *pHeightmap = GetIEditor()->GetHeightmap();
	if(!pHeightmap || !pHeightmap->GetHeight() || !pHeightmap->GetWidth())
		return;

	AABB srcBox(m_source.pos-m_dym/2, m_source.pos+m_dym/2);

	LONG bx1 = pHeightmap->WorldToHmap(srcBox.min).x;
	LONG by1 = pHeightmap->WorldToHmap(srcBox.min).y;
	LONG bx2 = pHeightmap->WorldToHmap(srcBox.max).x;
	LONG by2 = pHeightmap->WorldToHmap(srcBox.max).y;

	LONG xc = (bx1+bx2)/2;
	LONG yc = (by1+by2)/2;

	LONG cx1 = bx1;
	LONG cy1 = by1;

	CPoint hmapPosStart = pHeightmap->WorldToHmap(m_target.pos-m_dym/2) - CPoint(cx1,cy1);

	if(m_targetRot==1 || m_targetRot==3)
	{
		cx1 = xc - (by2-by1)/2;
		cy1 = yc - (bx2-bx1)/2;
		hmapPosStart = pHeightmap->WorldToHmap(m_target.pos-Vec3(m_dym.y/2, m_dym.x/2, m_dym.z)) - CPoint(cx1,cy1);
	}

	LONG blocksize = 512;

	for(LONG by = by1; by<by2; by+=blocksize)
	for(LONG bx = bx1; bx<bx2; bx+=blocksize)
	{
		LONG cx = bx;
		LONG cy = by;

		if(m_targetRot==1)
		{
			cx = xc - (yc - by);
			cy = yc + (xc - bx) - blocksize;
			if(cy < cy1) cy = cy1;
		}
		else if(m_targetRot==3)
		{
			cx = xc + (yc - by) - blocksize;
			cy = yc - (xc - bx);
			if(cx < cx1) cx = cx1;
		}
		else if(m_targetRot==2)
		{
			cx = bx2-1 + bx1 - bx - blocksize;
			if(cx < bx1) cx = bx1;

			cy = by2-1 + by1 - by - blocksize;
			if(cy < by1) cy = by1;
		}

		// Move terrain heightmap block.
		CPoint hmapPos = hmapPosStart + CPoint(cx, cy);

		CRect rc (bx, by, min(bx + blocksize+1, bx2), min(by + blocksize+1, by2));
		CRect rcc(hmapPos.x, hmapPos.y, hmapPos.x+rc.Width(), hmapPos.y+rc.Height());
		if(m_targetRot==1 || m_targetRot==3)
		{
			rcc.right = hmapPos.x+rc.Height();
			rcc.bottom = hmapPos.y+rc.Width();
		}

		if (m_archive)
			delete m_archive;
		m_archive = new CXmlArchive("Root");
		// Switch archive to saving.
		m_archive->bLoading = false;

		pHeightmap->ExportBlock( rc, *m_archive, isCopy && (bOnlyVegetation || (!bOnlyVegetation && !bOnlyTerrain)));

		// Switch archive to loading.
		m_archive->bLoading = true;

		pHeightmap->ImportBlock( *m_archive, hmapPos, true, (m_target.pos-m_source.pos).z, bOnlyVegetation && !bOnlyTerrain, m_targetRot);

		if(!bOnlyVegetation || bOnlyTerrain)
		{
			CRGBLayer * pLayer = &pHeightmap->m_TerrainRGBTexture;
			uint32 nRes = pLayer->CalcMaxLocalResolution((float)bx/pHeightmap->GetWidth(), (float)by/pHeightmap->GetHeight(), (float)rc.right/pHeightmap->GetWidth(), (float)rc.bottom/pHeightmap->GetHeight());
			float fTerrainSize = (float)GetIEditor()->Get3DEngine()->GetTerrainSize();

			{
				CImage image;
				CImage tmpImage;
				image.Allocate((rc.right-rc.left)*nRes/pHeightmap->GetWidth(), (rc.bottom-rc.top)*nRes/pHeightmap->GetHeight());
				pLayer->GetSubImageStretched((float)bx/pHeightmap->GetWidth(), (float)by/pHeightmap->GetHeight(), (float)rc.right/pHeightmap->GetWidth(), (float)rc.bottom/pHeightmap->GetHeight(), image);

				if(m_targetRot)
					tmpImage.RotateOrt(image, m_targetRot);

				pLayer->SetSubImageStretched(
					((float)rcc.left)/pHeightmap->GetWidth(), ((float)rcc.top)/pHeightmap->GetHeight(), 
					((float)rcc.right)/pHeightmap->GetWidth(), ((float)rcc.bottom)/pHeightmap->GetHeight(), m_targetRot ? tmpImage : image);
			}

			for(int x=0; x<=rc.Width(); x++)
				for(int y=0; y<=rc.Height(); y++)
				{
					uint32 lid = pHeightmap->GetLayerIdAt(x+rc.left, y+rc.top);
					if(m_targetRot==1)
						pHeightmap->SetLayerIdAt(rcc.right-y, x, lid);
					else if(m_targetRot==2)
						pHeightmap->SetLayerIdAt(rcc.right-x, rcc.bottom-y, lid);
					else if(m_targetRot==3)
						pHeightmap->SetLayerIdAt(y, rcc.bottom-x, lid);
					else
						pHeightmap->SetLayerIdAt(x+rcc.left, y+rcc.top, lid);
				}
			
			pHeightmap->UpdateLayerTexture(rcc);
		}
		if (m_archive)
		{
			delete m_archive;
			m_archive = 0;
		}
	}


	if(!isCopy && (bOnlyVegetation || (!bOnlyVegetation && !bOnlyTerrain)))
	{
		pHeightmap->GetVegetationMap()->RepositionArea(srcBox, m_target.pos-m_source.pos, m_targetRot);
	}

	if(!isCopy && (!bOnlyVegetation && !bOnlyTerrain))
	{
		GetIEditor()->GetObjectManager()->MoveObjects( srcBox, m_target.pos-m_source.pos, m_targetRot, isCopy);
	}

	/*
	// Load selection from archive.
	XmlNodeRef objRoot = m_archive->root->findChild("Objects");
	if (objRoot)
	{
		GetIEditor()->ClearSelection();
		CObjectArchive ar( GetIEditor()->GetObjectManager(),objRoot,true );
		GetIEditor()->GetObjectManager()->LoadObjects( ar,false );
	}
	*/
	GetIEditor()->ClearSelection();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMoveTool::SetArchive( CXmlArchive *ar )
{
	if (m_archive)
		delete m_archive;
	m_archive = ar;

	int x1,y1,x2,y2;
	// Load rect size our of archive.
	m_archive->root->getAttr( "X1",x1 );
	m_archive->root->getAttr( "Y1",y1 );
	m_archive->root->getAttr( "X2",x2 );
	m_archive->root->getAttr( "Y2",y2 );

	m_srcRect.SetRect( x1,y1,x2,y2 );
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMoveTool::Select(int nBox)
{
	m_source.isSelected = false;
	m_target.isSelected = false;

	if(nBox==0)
	{
		GetIEditor()->ShowTransformManipulator(false);
		m_source.isShow = false;
		m_target.isShow = false;
	}

	if(nBox==1)
	{
		m_source.isSelected = true;
		m_source.isShow = true;
		ITransformManipulator *pManipulator = GetIEditor()->ShowTransformManipulator(true);
		Matrix34 tm;
		tm.SetIdentity();
		tm.SetTranslation( m_source.pos );
		pManipulator->SetTransformation( COORDS_LOCAL, tm );
	}

	if(nBox==2)
	{
		m_target.isSelected = true;
		m_target.isShow = true;
		ITransformManipulator *pManipulator = GetIEditor()->ShowTransformManipulator(true);
		Matrix34 tm;
		tm.SetIdentity();
		if(m_targetRot)
		{
			float a=PI/2 * m_targetRot;
			tm.SetRotationZ(a);
		}
		tm.SetTranslation( m_target.pos );
		pManipulator->SetTransformation( COORDS_LOCAL, tm );
	}
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMoveTool::OnManipulatorDrag( CViewport *view,ITransformManipulator *pManipulator,CPoint &p0,CPoint &p1,const Vec3 &value )
{
	int editMode = GetIEditor()->GetEditMode();
	if (editMode == eEditModeMove)
	{
		CHeightmap *pHeightmap = GetIEditor()->GetHeightmap();
		GetIEditor()->RestoreUndo();

		Vec3 pos = m_source.pos;
		Vec3 val = value;

		Vec3 max = pHeightmap->HmapToWorld(CPoint(pHeightmap->GetWidth(), pHeightmap->GetHeight()));

		uint32 wid = max.x;
		uint32 hey = max.y;
		
		if(m_target.isSelected)
			pos = m_target.pos;

		pManipulator = GetIEditor()->ShowTransformManipulator(true);
		Matrix34 tm = pManipulator->GetTransformation(COORDS_LOCAL);

		if (CUndo::IsRecording())
			CUndo::Record( new CUndoTerrainMoveTool(this) );

		if(m_source.isSelected)
		{
			Vec3 p = m_source.pos+val;

			if(p.x - m_dym.x/2 < 0)
				val.x = m_dym.x/2 - m_source.pos.x;
			if(p.y - m_dym.y/2 < 0)
				val.y = m_dym.y/2 - m_source.pos.y;

			if(p.x + m_dym.x/2 >= wid)
				val.x = wid - m_dym.x/2 - m_source.pos.x;
			if(p.y + m_dym.y/2 >= hey)
				val.y = hey - m_dym.y/2 - m_source.pos.y;

			m_source.pos+=val;
		}
		
		if(m_target.isSelected)
		{
			Vec3 p = m_target.pos+val;

			float hx = m_dym.x/2;
			float hy = m_dym.y/2;
			if(m_targetRot==1 || m_targetRot==3)
			{
				hx = m_dym.y/2;
				hy = m_dym.x/2;
			}

			if(p.x - hx < 0)
				val.x = hx - m_target.pos.x;
			if(p.y - hy < 0)
				val.y = hy - m_target.pos.y;

			if(p.x + hx >= wid)
				val.x = wid - hx - m_target.pos.x;
			if(p.y + hy >= hey)
				val.y = hey - hy - m_target.pos.y;

			m_target.pos+=val;
		}

		tm.SetTranslation( pos + val );
		pManipulator->SetTransformation( COORDS_LOCAL,tm );

	}
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMoveTool::CorrectPosition()
{
	CHeightmap *pHeightmap = GetIEditor()->GetHeightmap();
	GetIEditor()->RestoreUndo();

	Vec3 max = pHeightmap->HmapToWorld(CPoint(pHeightmap->GetWidth(), pHeightmap->GetHeight()));

	uint32 wid = max.x;
	uint32 hey = max.y;

	float hx = m_dym.x/2;
	float hy = m_dym.y/2;

	if(m_source.pos.x - hx < 0)
		m_source.pos.x = hx;
	if(m_source.pos.y - hy < 0)
		m_source.pos.y = hy;

	if(m_source.pos.x + hx >= wid)
		m_source.pos.x = wid-hx;
	if(m_source.pos.y + hy >= hey)
		m_source.pos.y = hey-hy;

	if(m_targetRot==1 || m_targetRot==3)
	{
		hx = m_dym.y/2;
		hy = m_dym.x/2;
	}

	if(m_target.pos.x - hx < 0)
		m_target.pos.x = hx;
	if(m_target.pos.y - hy < 0)
		m_target.pos.y = hy;

	if(m_target.pos.x + hx >= wid)
		m_target.pos.x = wid-hx;
	if(m_target.pos.y + hy >= hey)
		m_target.pos.y = hey-hy;

	if ( m_source.isSelected || m_target.isSelected )
	{
		ITransformManipulator * pManipulator = GetIEditor()->ShowTransformManipulator(true);
		Matrix34 tm = pManipulator->GetTransformation(COORDS_LOCAL);
		tm.SetIdentity();
		if(m_source.isSelected)
			tm.SetTranslation( m_source.pos );
		if(m_target.isSelected)
		{
			if(m_targetRot)
			{
				float a=PI/2 * m_targetRot;
				tm.SetRotationZ(a);
			}
			tm.SetTranslation( m_target.pos );
		}
		pManipulator->SetTransformation( COORDS_LOCAL, tm );
	}
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMoveTool::SetDym(Vec3 dym)
{	
	m_dym = dym;
	CorrectPosition();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainMoveTool::SetTargetRot(int targetRot)
{	
	m_targetRot = targetRot;
	CorrectPosition();
}