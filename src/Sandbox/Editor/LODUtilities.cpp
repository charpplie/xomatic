/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2013.
*************************************************************************/

#include "stdafx.h"
#include "LODUtilities.h"
#include "LODInterface.h"

//////////////////////////////////////////////////////////////////////////
// Texture display control
//////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CMeshBakerTextureCtrl, CWnd)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_MOUSEMOVE()
	ON_MESSAGE(WM_MOUSEHOVER, OnMouseHover)
	ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
END_MESSAGE_MAP()

CMeshBakerTextureCtrl::CMeshBakerTextureCtrl(bool bTooltip)
{
	m_bHovering=false;
	m_bTooltip=bTooltip;
	m_pToolTip=NULL;
}

CMeshBakerTextureCtrl::~CMeshBakerTextureCtrl()
{
	delete m_pToolTip;
}

BOOL CMeshBakerTextureCtrl::Create( CWnd *pWndParent,const CRect &rc,DWORD dwStyle )
{
	BOOL bReturn = CreateEx( NULL, AfxRegisterWndClass(CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW|CS_OWNDC, 
		AfxGetApp()->LoadStandardCursor(IDC_ARROW), NULL, NULL), NULL,dwStyle,
		rc, pWndParent, NULL);

	return bReturn;
}

void CMeshBakerTextureCtrl::OnPaint() 
{
	CPaintDC dc(this);
	CRect rc;
	GetClientRect(&rc);
	CDC bitmapDC;
	bitmapDC.CreateCompatibleDC(&dc);
	bitmapDC.SelectObject(&m_bitmap);
	dc.BitBlt(rc.left,rc.top,rc.Width(),rc.Height(),&bitmapDC,0,0,SRCCOPY);
}

void CMeshBakerTextureCtrl::UpdateBitmap()
{
	if (m_pCurrentTex)
	{
		int width=m_pCurrentTex->GetWidth();
		int height=m_pCurrentTex->GetHeight();
		std::vector<byte> pStorage;
		pStorage.resize(width*height*4);
		bool bSaved=false;
		byte *pData=m_pCurrentTex->GetData32(0,0,&(*pStorage.begin()));
		if (pData)
		{
			// Copy to temporary bitmap
			CBitmap tempBitmap;
			if (m_bShowAlpha)
			{
				for (int idx=0; idx<height*width*4; idx+=4)
				{
					pData[idx+0]=pData[idx+1]=pData[idx+2]=pData[idx+3];
					pData[idx+3]=255;
				}
			}
			else
			{
				for (int idx=0; idx<height*width*4; idx+=4)
				{
					byte red=pData[idx+0];
					pData[idx+0]=pData[idx+2];
					pData[idx+2]=red;
					pData[idx+3]=255;
				}
			}
			tempBitmap.CreateBitmap(width, height, 1, 32, pData);

			// Scale this into a compatible bitmap
			CRect rc;
			CPaintDC dc(this);
			GetClientRect(&rc);

			if (m_bitmap.GetSafeHandle() != NULL)
				m_bitmap.DeleteObject();

			m_bitmap.CreateCompatibleBitmap(&dc, rc.Width(), rc.Height());
			CDC srcDC, dstDC;
			srcDC.CreateCompatibleDC(&dc);
			srcDC.SelectObject(&tempBitmap);
			dstDC.CreateCompatibleDC(&dc);
			dstDC.SelectObject(&m_bitmap);
			dstDC.SetStretchBltMode(HALFTONE);
			dstDC.SetBrushOrg(0, 0);
			dstDC.StretchBlt(0, 0, rc.Width(), rc.Height(), &srcDC, 0, 0, width, height, SRCCOPY);
		}
	}
}

void CMeshBakerTextureCtrl::SetTexture(ITexture *pTex, bool bShowAlpha)
{
	m_pCurrentTex=pTex;
	m_bShowAlpha=bShowAlpha;
	UpdateBitmap();
	Invalidate();
}

void CMeshBakerTextureCtrl::OnSize(UINT nType, int cx, int cy)
{
	UpdateBitmap();
	Invalidate();
}

void CMeshBakerTextureCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bTooltip)
	{
		TRACKMOUSEEVENT me;
		me.cbSize=sizeof(me);
		me.dwFlags=TME_HOVER|TME_LEAVE;
		me.hwndTrack=m_hWnd;
		me.dwHoverTime=HOVER_DEFAULT;
		TrackMouseEvent(&me);
	}
}

LRESULT CMeshBakerTextureCtrl::OnMouseHover(WPARAM wParam, LPARAM lParam)
{
	if (!m_bHovering && m_pCurrentTex)
	{
		CRect rc;
		GetClientRect(&rc);
		ClientToScreen(&rc);

		int height=m_pCurrentTex->GetHeight()/2;
		int width=m_pCurrentTex->GetWidth()/2;

		POINT pos;
		pos.x = rc.left;
		pos.y = rc.top;
		HMONITOR hMon = MonitorFromPoint(pos, MONITOR_DEFAULTTONULL);
		if(!hMon)
		{
			GetCursorPos(&pos);
			hMon = MonitorFromPoint(pos, MONITOR_DEFAULTTOPRIMARY);
		}
		MONITORINFO mi = {0};
		mi.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(hMon, &mi);
		if (height>mi.rcMonitor.bottom-mi.rcMonitor.top)
		{
			float scale=(mi.rcMonitor.bottom-mi.rcMonitor.top)/(float)height;
			height=mi.rcMonitor.bottom-mi.rcMonitor.top;
			width=(int)floorf(width*scale+0.5f);
		}

		int gap=10;
		rc.top=(rc.top+rc.bottom-height)/2;
		int leftOverlap=mi.rcMonitor.left-(rc.left-gap-width);
		int rightOverlap=mi.rcMonitor.right-(rc.right+gap+width);
		if (leftOverlap<=0 || leftOverlap<rightOverlap)
		{
			rc.left=rc.left-gap-width;
			rc.right=rc.left+width;
		}
		else
		{
			rc.left=rc.right+gap;
			rc.right=rc.left+width;
		}
		rc.bottom=rc.top+height;

		if (rc.top<mi.rcMonitor.top)
		{
			rc.top=mi.rcMonitor.top;
			rc.bottom=rc.top+height;
		}
		else if (rc.bottom>mi.rcMonitor.bottom)
		{
			rc.bottom=mi.rcMonitor.bottom;
			rc.top=rc.bottom-height;
		}

		m_bHovering=true;
		m_pToolTip=new CMeshBakerTextureCtrl(false);
		m_pToolTip->Create(GetDesktopWindow(), rc, WS_VISIBLE|WS_POPUP|WS_BORDER);
		m_pToolTip->SetTexture(m_pCurrentTex, m_bShowAlpha);
		m_pToolTip->MoveWindow(rc, TRUE);
	}
	return 0;
}

LRESULT CMeshBakerTextureCtrl::OnMouseLeave(WPARAM wParam, LPARAM lParam)
{
	if (m_bHovering)
	{
		delete m_pToolTip;
		m_pToolTip=NULL;
		m_bHovering=false;
	}
	return 0;
}


//////////////////////////////////////////////////////////////////////////
// Popup preview
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CMeshBakerPopupPreview,CWnd)

BEGIN_MESSAGE_MAP(CMeshBakerPopupPreview, CWnd)
	ON_WM_SIZE()
END_MESSAGE_MAP()

CMeshBakerPopupPreview::CMeshBakerPopupPreview()
{
}

BOOL CMeshBakerPopupPreview::Create( CWnd *pWndParent,const CRect &rc,DWORD dwStyle)
{
	BOOL bReturn = CreateEx( NULL, AfxRegisterWndClass(CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW|CS_OWNDC, 
		AfxGetApp()->LoadStandardCursor(IDC_ARROW), NULL, NULL), NULL,dwStyle,
		rc, pWndParent, NULL);

	CRect lrc;
	GetClientRect(lrc);
	m_modelCtrl.Create(this,lrc,WS_CHILD|WS_VISIBLE);
	m_modelCtrl.ReleaseObject();
	m_modelCtrl.SetGrid(true);
	m_modelCtrl.EnableMaterialPrecaching(true);
	m_modelCtrl.EnableUpdate(true);
	m_modelCtrl.SetAmbient(ColorF(16,16,16,1));

	return bReturn;
}

void CMeshBakerPopupPreview::OnSize(UINT nType, int cx, int cy)
{
	if ( m_modelCtrl.GetSafeHwnd() )
	{
		CRect rc;
		GetClientRect(rc);
		m_modelCtrl.MoveWindow(rc, TRUE);
	}
}

void CMeshBakerPopupPreview::SetModel(IStatObj * pObj)
{
	m_modelCtrl.SetObject(pObj);
	m_modelCtrl.FitToScreen();
}

void CMeshBakerPopupPreview::SetMaterial(CMaterial* pMat)
{
	m_modelCtrl.SetMaterial(pMat);
}

void CMeshBakerPopupPreview::SetRotate(bool rotate)
{
	m_modelCtrl.SetRotation(rotate);
}

void CMeshBakerPopupPreview::SetWireframe(bool wireframe)
{
	m_modelCtrl.EnableWireframeRendering(wireframe);
}

void CMeshBakerPopupPreview::SetGrid(bool grid)
{
	m_modelCtrl.SetGrid(grid);
}

void CMeshBakerPopupPreview::Reset()
{
	m_modelCtrl.SetObject(NULL);
	m_modelCtrl.SetMaterial(NULL);
}

//////////////////////////////////////////////////////////////////////////
// Error Graph
//////////////////////////////////////////////////////////////////////////

CLODGeneratorErrorGraphRamp::CLODGeneratorErrorGraphRamp() : CRampControl()
{
	SetDrawPercentage(false);
	m_selectedError=-1.0f;
	m_totalError = 0.0f;
}

void CLODGeneratorErrorGraphRamp::DrawBackground(CDC &dc)
{
	CLodGeneratorInteractionManager* pInstance = CLodGeneratorInteractionManager::Instance();
	const int nNumMoves = pInstance->GetNumMoves();

	CBrush cbrush;
	CRect rc;
	GetClientRect(&rc);

	cbrush.CreateSolidBrush(RGB(128,128,128));

	dc.SetBkMode(OPAQUE);
	dc.SelectObject(&cbrush);
	dc.FillRect(&rc, &cbrush);

	if (IsWindowEnabled())
	{
		m_selectedError=-1.0f;
		m_totalError = 0.0f;
		CBrush brush;
		CBrush bbrush;
		brush.CreateSolidBrush(RGB(255,255,255));
		bbrush.CreateSolidBrush(RGB(64,64,64));

		for (int i=0; i<nNumMoves; i++)
		{
			m_totalError+=pInstance->GetErrorAtMove(i);
		}

		if (m_totalError>0.0f)
		{
			m_totalError=logf(m_totalError);
		}
		else
		{
			m_totalError=1.0f;
		}

		dc.SelectObject(&bbrush);

		float bar=floorf(m_totalError);
		while (bar>0.0f)
		{
			CRect lrc=rc;
			lrc.bottom-=(int)(rc.Height()*bar/m_totalError);
			lrc.top=lrc.bottom-1;
			dc.FillRect(&lrc, &bbrush);
			bar-=2;
		}

		dc.SelectObject(&brush);
		float currentError=0.0f;
		int lastX=-1;

		float yScale=(nNumMoves>1)?(1.0f/(float)(nNumMoves-1)):1.0f;
		for (int i=0; i<nNumMoves; i++)
		{
			CRect lrc;
			lrc.left=rc.left+(int)floorf(rc.Width()*(1.0f-i*yScale));
			if (lrc.left!=lastX)
			{
				lrc.right=lrc.left+1;
				lrc.bottom=rc.bottom-(int)(rc.Height()*((currentError==0.0f)?0.0f:logf(currentError))/m_totalError);
				lrc.top=lrc.bottom-1;
				dc.FillRect(&lrc, &brush);
				lastX=lrc.left;
			}

			HandleData handleData;
			if ( GetSelectedData(handleData) )
			{
				float selectedPercentage=handleData.percentage;
				int selected=(int)floorf(nNumMoves*(1.0f-selectedPercentage/100.0f)+0.5f);
				if (i==selected)
				{
					m_selectedError=currentError;
				}
			}

			currentError+=pInstance->GetErrorAtMove(i);
		}
	}
}

void CLODGeneratorErrorGraphRamp::DrawForeground(CDC &dc)
{
	HandleData handleData;
	if ( GetSelectedData(handleData) )
	{
		IStatObj::SStatistics loadedStats = CLodGeneratorInteractionManager::Instance()->GetLoadedStatistics();
		CRect rc;
		GetClientRect(&rc);
		CString str;
		int tris=m_currentStats.nIndices/3;
		int verts=m_currentStats.nVertices;
		if ( tris != 0 && verts != 0 )
		{
			dc.SetTextColor(RGB(255,255,255));
			dc.SetBkMode(TRANSPARENT);
			str.Format("Tris:%d (%d%%)\n", tris, (int)(((float)tris / (float)(loadedStats.nIndices/3)) * 100.0f));
			dc.DrawText(str, rc, DT_RIGHT|DT_SINGLELINE|DT_END_ELLIPSIS); 
			rc.top+=dc.GetTextExtent(str).cy;
			str.Format("Verts:%d (%d%%)\n", verts, (int)(((float)verts / (float)loadedStats.nVertices) * 100.0f));
			dc.DrawText(str, rc, DT_RIGHT|DT_SINGLELINE|DT_END_ELLIPSIS); 
			rc.top+=dc.GetTextExtent(str).cy;
			str.Format("Error:%.1f\n", m_selectedError);
			dc.DrawText(str, rc, DT_RIGHT|DT_SINGLELINE|DT_END_ELLIPSIS); 
		}
	}
}

void CLODGeneratorErrorGraphRamp::AddCustomMenuOptions(CMenu * menu)
{
	const int nIdx = GetHotHandleIndex();
	if (nIdx != -1)
	{
		menu->AppendMenu(MF_SEPARATOR,0,"");
		menu->AppendMenu(MF_STRING, ID_MENU_CUSTOM_BEGIN+1, "Show in Explorer");
	}
}

void CLODGeneratorErrorGraphRamp::SetStats(IStatObj::SStatistics stats)
{
	m_currentStats = stats;
}


void CLODGeneratorErrorGraphRamp::OnMenuCustom(UINT nID)
{
	if ( nID == ID_MENU_CUSTOM_BEGIN+1)
	{
		CLodGeneratorInteractionManager* pInstance = CLodGeneratorInteractionManager::Instance();
		const int idx = GetHotHandleIndex();
		const int nSourceLod = pInstance->GetGeometryOption<int>("nSourceLod");
		const int nHandleCount = GetHandleCount();

		int lodIndex = (nHandleCount-idx) + nSourceLod;
		pInstance->ShowLodInExplorer(lodIndex);
	}
}