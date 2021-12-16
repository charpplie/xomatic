#include "StdAfx.h"
#include "GridMapRenderWnd.h"
#include "GridMapTool.h"
#include "ViewManager.h"
#include "DisplaySettings.h"
#include ".\Terrain\Heightmap.h"
#include "ViewStates.h"
#include "SegmentedWorldManager.h"
#include "SegmentedWorldMiniMapUpdater.h"

#define MIN_ZOOM 0.01f
#define MAX_ZOOM 460.0f

using namespace sw;

//////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CGridMapRenderWnd, C2DViewport)

//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CGridMapRenderWnd, C2DViewport)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
CGridMapRenderWnd::CGridMapRenderWnd()
{
	m_bContentsUpdated = false;
	m_bFirstViewportUpdate = true;

	m_gridAlpha = 0.3f;
	m_pSegmentTool = NULL;

	SetZoomRange(0.02f, 1.0f);
	SetShowObjectsInfo(false);
	SetGridLines(false, true);
	SetGridLineNumbers(false);
	SetAutoAdjust(false);
}

//////////////////////////////////////////////////////////////////////////
CGridMapRenderWnd::~CGridMapRenderWnd()
{
	UpdateViewSettings(false);
}

//////////////////////////////////////////////////////////////////////////
void CGridMapRenderWnd::DeleteLayer()
{
	SegmentTextureMap::iterator it = m_layer.textures.begin();
	SegmentTextureMap::iterator end = m_layer.textures.end();
	for( ; it != end; ++it)
	{
		SSegmentTexture &segTexture = it->second;
		m_renderer->RemoveTexture(segTexture.textureId);
		segTexture.textureId = 0;
	}
}

//////////////////////////////////////////////////////////////////////////
void CGridMapRenderWnd::SetLayerImage(int segmentId, CImageEx *pImage, const Recti &rect)
{
	SSegmentTexture *pTexture;
	SegmentTextureMap::iterator it = m_layer.textures.find(segmentId);
	if(it != m_layer.textures.end())
		pTexture = &it->second;
	else
		pTexture = &m_layer.textures[segmentId];

	pTexture->bbox = rect;
	if(pImage->IsValid() && m_renderer)
	{
		if(!pTexture->textureId)
			pTexture->textureId = m_renderer->DownLoadToVideoMemory((unsigned char*)pImage->GetData(), pImage->GetWidth(), pImage->GetHeight(), eTF_A8R8G8B8, eTF_A8R8G8B8, 0, 0, 0);
		else
			m_renderer->UpdateTextureInVideoMemory(pTexture->textureId, (unsigned char*)pImage->GetData(), 0, 0, pImage->GetWidth(), pImage->GetHeight(), eTF_A8R8G8B8);
	}
}

//////////////////////////////////////////////////////////////////////////
void CGridMapRenderWnd::UpdateViewSettings(bool bLoading)
{
	ISegmentedWorldDoc& swdoc = GetIEditor()->GetSegmentedWorldDoc();
	if(!swdoc.IsOk())
		return;
	
	TViewStates &viewStates = swdoc.GetViewStates();

	if(bLoading)
	{
		m_origin2D.x = viewStates.m_vWorldMapOrig.x;
		m_origin2D.y = viewStates.m_vWorldMapOrig.y;
		m_fZoomFactor = viewStates.m_nWorldMapZoom;
	}
	else
	{
		viewStates.m_vWorldMapOrig.x = m_origin2D.x;
		viewStates.m_vWorldMapOrig.y = m_origin2D.y;
		viewStates.m_nWorldMapZoom = m_fZoomFactor;
	}
}

//////////////////////////////////////////////////////////////////////////
void CGridMapRenderWnd::UpdateView()
{
	m_bFirstViewportUpdate = true;
	UpdateContent(0xFFFFFFFF);
}

//////////////////////////////////////////////////////////////////////////
void CGridMapRenderWnd::UpdateContent(int flags)
{
	if (!GetIEditor()->GetDocument())
		return;

	C2DViewport::UpdateContent(flags);

	m_bContentsUpdated = true;

	if(m_bFirstViewportUpdate)
	{
		CEditTool *pTool = GetEditTool();
		if(pTool && pTool->IsKindOf(RUNTIME_CLASS(CSegmentSelectTool)))
		{
			m_pSegmentTool = (CSegmentSelectTool *)pTool;

			Recti worldBoundary = m_pSegmentTool->GetWorldBoundary();

			SetZoom(0.95f*m_rcClient.Width()/worldBoundary.GetWidth(), CPoint(m_rcClient.Width()/2,m_rcClient.Height()/2));
			SetScrollOffset(-10,-10);
		}
	}
	m_bFirstViewportUpdate = false;
}

//////////////////////////////////////////////////////////////////////////
void CGridMapRenderWnd::Draw(DisplayContext &dc)
{
	bool bDisplayHelpers = dc.settings->IsDisplayHelpers();
	dc.settings->DisplayHelpers(false);

	if(!m_bContentsUpdated)
		UpdateContent(0xFFFFFFFF);

	Recti scrRect(m_rcClient.left, m_rcClient.top, m_rcClient.right, m_rcClient.bottom);

	m_renderer->ResetToDefault();
	dc.DepthTestOff();

	Matrix34 tm = GetScreenTM();
	
	float s[4],t[4];
	s[0]=0;	t[0]=0;
	s[1]=0;	t[1]=1;
	s[2]=1;	t[2]=1;
	s[3]=1;	t[3]=0;

	// fill world blocks before applying mini maps
	Recti worldBoundary = m_pSegmentTool->GetWorldBoundary();
	m_renderer->DrawImageWithUV(tm.m03+tm.m00*worldBoundary.Min.x,tm.m13+tm.m11*worldBoundary.Min.y,0,tm.m00*worldBoundary.GetWidth(),tm.m11*worldBoundary.GetHeight(),0,s,t,0,0,0);

	// draw mini map for each segment
	Recti scrBox;
	SegmentTextureMap::iterator it = m_layer.textures.begin();
	SegmentTextureMap::iterator end = m_layer.textures.end();
	for( ; it != end; ++it)
	{
		SSegmentTexture &segTexture = it->second;
		scrBox.Min.x = tm.m03+tm.m00*segTexture.bbox.Min.x;
		scrBox.Max.y = tm.m13+tm.m11*segTexture.bbox.Min.y;
		scrBox.Max.x = tm.m03+tm.m00*segTexture.bbox.Max.x;
		scrBox.Min.y = tm.m13+tm.m11*segTexture.bbox.Max.y;
		if(scrRect.Intersects(scrBox))
			m_renderer->DrawImageWithUV(scrBox.Min.x,scrBox.Max.y,0,tm.m00*segTexture.bbox.GetWidth(),tm.m11*segTexture.bbox.GetHeight(),segTexture.textureId,s,t);
	}
	
	dc.DepthTestOn();

	// update grid size
	CGrid *pGrid = GetIEditor()->GetViewManager()->GetGrid();
	pGrid->size = m_pSegmentTool->GetPerSegmentSize() / pGrid->majorLine;
	C2DViewport::Draw(dc);

	dc.settings->DisplayHelpers(bDisplayHelpers);
}

//////////////////////////////////////////////////////////////////////////
void CGridMapRenderWnd::SetScrollOffset(float x,float y,bool bLimits)
{
	C2DViewport::SetScrollOffset(x, y, bLimits);

	if(CSWMiniMapUpdater *pmmu = CSWMiniMapUpdater::Get())
	{
		pmmu->OnWorldRectChange(m_pSegmentTool->GetSegmentBoundary());
	}
}

//////////////////////////////////////////////////////////////////////////
void CGridMapRenderWnd::SetZoom(float fZoomFactor, CPoint center)
{
	if (fZoomFactor < m_minZoomFactor)
		fZoomFactor = m_minZoomFactor;
	if (fZoomFactor > m_maxZoomFactor)
		fZoomFactor = m_maxZoomFactor;

	C2DViewport::SetZoom(fZoomFactor, center);

	if(CSWMiniMapUpdater *pmmu = CSWMiniMapUpdater::Get())
	{
		float fScale = GetZoomFactor();
		float pixelsPerGrid = m_pSegmentTool->GetPerSegmentSize() * fScale;
		pmmu->OnSizeChange(pixelsPerGrid, false);
		pmmu->OnWorldRectChange(m_pSegmentTool->GetSegmentBoundary());
	}
}

//////////////////////////////////////////////////////////////////////////
void CGridMapRenderWnd::SetZoomRange(float minZoomFactor, float maxZoomFactor)
{
	if(minZoomFactor > maxZoomFactor)
		return;

	m_minZoomFactor = clamp_tpl(minZoomFactor, MIN_ZOOM, MAX_ZOOM);
	m_maxZoomFactor = clamp_tpl(maxZoomFactor, MIN_ZOOM, MAX_ZOOM);
}
