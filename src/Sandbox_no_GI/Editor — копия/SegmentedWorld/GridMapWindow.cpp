#include "stdafx.h"
#include "GridMapWindow.h"
#include "GridMapDlg.h"
#include "Controls/MemDC.h"
#include "SegmentedWorldManager.h"

using namespace sw;

CryCriticalSection CMapWnd::m_cs;

//////////////////////////////////////////////////////////////////////////
// CMapWnd
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNAMIC(CMapWnd, CWnd)

BEGIN_MESSAGE_MAP(CMapWnd, CWnd)
  ON_WM_PAINT()
  ON_WM_GETDLGCODE()
  ON_WM_KEYDOWN()
  ON_WM_KEYUP()
  ON_WM_LBUTTONDOWN()
  ON_WM_LBUTTONDBLCLK()
  ON_WM_LBUTTONUP()
  ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
  ON_WM_SETCURSOR()
  ON_WM_MOUSEWHEEL()
  ON_WM_RBUTTONDOWN()
  ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

CMapWnd::CMapWnd() : CWnd()
{
  WNDCLASSW wc;
  HINSTANCE hInstance = AfxGetResourceHandle();
  if (!GetClassInfoW(hInstance, L"CMapWnd", &wc)) {
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = ::DefWindowProcW;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = 0;
    wc.lpszMenuName  = 0;
    wc.lpszClassName = L"CMapWnd";
    RegisterClassW(&wc);
  }
}

HWND CMapWnd::CreateTrackingToolTip(int toolID, HWND hDlg, LPSTR pText)
{
	HINSTANCE hinst = AfxGetInstanceHandle();
	// Create a tooltip.
	HWND hwndTT = CreateWindowEx(WS_EX_TOPMOST,
		TOOLTIPS_CLASS, NULL,
		WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,		
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		hDlg, NULL, hinst,NULL);

	if (!hwndTT)
	{   
		return NULL;
	}

	// Set up tool information.
	// In this case, the "tool" is the entire parent window.
	m_ti.cbSize = sizeof(TOOLINFO);
	m_ti.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
	m_ti.hwnd = hDlg;
	m_ti.hinst = hinst;
	m_ti.lpszText = pText;
	m_ti.uId = (UINT_PTR)hDlg;
	::GetClientRect (hDlg, &m_ti.rect);

	::SendMessage(hwndTT, TTM_SETMAXTIPWIDTH, 0, 0);

	// Associate the tooltip with the tool window.
	::SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &m_ti);	
	return hwndTT;
}

void CMapWnd::Init() 
{
  m_bScrolling = m_bSpacePressed = m_bShiftPressed = m_bLBPressed = m_bRBPressed = m_bControlPressed = false;

  CSWManager *pSWMgr = GetIEditor()->GetSegmentedWorldManager();
  m_nSegmentSizeMt = pSWMgr->GetSegmentSizeMeters();

  m_editMode = EGEM_EDIT;
  m_lastEditMode = EGEM_EDIT;

  m_nEditSize = EDIT_1K;
  m_nCheckSize = EDIT_2K;
  m_nMergeSize = Vec2i(1, 1);

  assert(m_nSegmentSizeMt);
  m_arrEditSize[EDIT_1K] = 1024/m_nSegmentSizeMt;
  m_arrEditSize[EDIT_2K] = 2048/m_nSegmentSizeMt;
  m_arrEditSize[EDIT_4K] = 4096/m_nSegmentSizeMt;
  m_arrEditSize[EDIT_8K] = 8192/m_nSegmentSizeMt;
  m_arrEditSize[EDIT_16K] = 16384/m_nSegmentSizeMt;

  m_iEditSegX = 0;
  m_iEditSegY = 0;
  m_iEditSegW = 0;
  m_iEditSegH = 0;

  m_nZoomLevel = 0;
  m_arrMTperPIX[0] = 64;
  m_arrMTperPIX[1] = 32;
  m_arrMTperPIX[2] = 16;
  m_arrMTperPIX[3] = 8;
  m_arrMTperPIX[4] = 4;
  m_arrMTperPIX[5] = 2;

  m_ptDrawWorldPosMt.set(0,0);
  m_ptMouseWorldPosMt.set(0,0);
  m_rcWorld(0, 0, 0, 0);


  CGridMapDlg::Get()->OnLevelChanged();	// TODO: why mapwnd inits its parent????

  m_hHandCursor = LoadCursor( NULL, IDC_HAND );
  m_hCrossCursor = LoadCursor( NULL, IDC_CROSS );
  m_hOrgCursor = GetCursor();
  SetCursor(m_hCrossCursor);

  SetZoomLevel(m_nZoomLevel);

  SetSelecionSize(m_nEditSize);

  m_lastClickTime = 0;

	
	m_trackingTT = CreateTrackingToolTip(1, GetSafeHwnd(), (LPTSTR)"some text");

	m_bSegInfoUpdateSuspended = false;

  //CreateLayer(2, 1, CRect(-16*1024, -16*1024, 16*1024, 16*1024), 120);
  //SetLayerImage(2, "..\\Tools\\WorldMap\\b&beast.bmp");
}

void CMapWnd::UpdateWorld(CRect *rcWorldMt)
{
  Invalidate();
}

void CMapWnd::SetLayerImage(DWORD dwLayer, DWORD dwID, CImage *pImage, CRect *prcWorldMt, bool bOwnImage)
{
  bool bNewLayer = false;
  TLayer *player = GetLayer(dwLayer);
  if (!player) {
    player = &m_Layers[dwLayer];
    bNewLayer = true;
  }

  bool bNewImage = false;
  TImg *pimg = GetLayerImage(dwLayer, dwID);
  if (pimg)
	{
		player->RemoveImageFromHistogram(pimg->pImage);
    pimg->Destroy();
	}
  else  {
    pimg = &player->mImages[dwID];
    bNewImage = true;
  }

  pimg->pImage = pImage;
  pimg->bOwnImage = bOwnImage;

	player->AddImageToHistogram(pImage);

  if (prcWorldMt)
    pimg->rcWorldMt = *prcWorldMt;
  else if (bNewImage || bNewLayer) {
    assert(!"the rect must be given");
  }

  UpdateLayerBitmap(dwLayer, dwID);
}

void CMapWnd::SetLayerImage(DWORD dwLayer, DWORD dwID, const char *pcFileName, CRect *prcWorldMt)
{
  CImage img;
  if (!CImageUtil::LoadBmp(pcFileName, img))
    return;
  img.SwapRedAndBlue();

  SetLayerImage(dwLayer, dwID, &img, prcWorldMt, true);
}

CMapWnd::TImg *CMapWnd::GetLayerImage(DWORD dwLayer, DWORD dwID)
{
  TLayer *player = GetLayer(dwLayer);
  if (!player)
    return 0;

  TImgagesMap::iterator it = player->mImages.find(dwID);
  if (it == player->mImages.end())
    return 0;

  TImg &img = it->second;
  return &img;
}

void CMapWnd::SetLayerAlpha(DWORD dwLayer, BYTE btAlpha)
{
  TLayer *player = GetLayer(dwLayer);
  if (player)
    player->btAlpha = btAlpha;
  ((CGridMapDlg*)GetParent())->UpdateWorld();
}

CMapWnd::TLayer *CMapWnd::GetLayer(DWORD dwID)
{
  TLayersMap::iterator it = m_Layers.find(dwID);
  if (it == m_Layers.end())
    return 0;
  TLayer &layer = it->second;
  return &layer;
}

void CMapWnd::DelLayer(DWORD dwLayer)
{
  TLayersMap::iterator it = m_Layers.find(dwLayer);
  if (it == m_Layers.end())
    return;

  it->second.Destroy();
  m_Layers.erase(it);

  ((CGridMapDlg*)GetParent())->UpdateWorld();
}

void CMapWnd::UpdateLayerBitmap(DWORD dwLayer, DWORD dwBitmap)
{
  AUTO_LOCK(m_cs);

  //CString s; s.Format("%s\n", dwBitmap == (DWORD)-1 ? "Full layer" : "Specific image");
  //OutputDebugString(s);

  for (TLayersMap::iterator itl = m_Layers.begin(); itl != m_Layers.end(); ++itl) {
    TLayer &layer = itl->second;

    if (dwLayer != (DWORD)-1 && dwLayer != itl->first)
      continue;

    for (TImgagesMap::iterator it = layer.mImages.begin(); it != layer.mImages.end(); ++it) {
      TImg &img = it->second;

      if (dwBitmap != (DWORD)-1 && dwBitmap != it->first)
        continue;

      img.bDirty = true;

      if (dwBitmap != (DWORD)-1) {
        CRect rcViewMt = GetRectMt();
        CRect rcIntersectMt = rcViewMt & img.rcWorldMt;
        if (!rcIntersectMt.IsRectEmpty()) {
          rcIntersectMt = img.rcWorldMt;
          CRect rcWnd; GetClientRect(&rcWnd);
          Vec4 rcDst = MT2SCR(Vec4(rcIntersectMt.left, rcIntersectMt.top, rcIntersectMt.right, rcIntersectMt.bottom), 
            Vec2(rcWnd.Width(), rcWnd.Height()), 
            Vec4(rcViewMt.left, rcViewMt.top, rcViewMt.right, rcViewMt.bottom)); 

          InvalidateRect(CRect(rcDst.x, rcDst.y, rcDst.z, rcDst.w));
        }
        break;
      }
    }

    layer.nDirty = max(layer.nDirty, (dwBitmap == -1) ? 2 : 1);

    if (dwLayer != (DWORD)-1)
      break;

  }

  if (dwBitmap == -1)
    Invalidate();
}

void CMapWnd::RedrawLayerBitmaps()
{
  AUTO_LOCK(m_cs);

  CRect rcViewMt = GetRectMt();

  CRect rcWnd; GetClientRect(&rcWnd);
  int nScrWidth = rcWnd.Width();
  int nScrHeight = rcWnd.Height();

  for (TLayersMap::iterator itl = m_Layers.begin(); itl != m_Layers.end(); ++itl) {
    TLayer &layer = itl->second;

    if (!layer.nDirty)
      continue;

    //CString s; s.Format("=>>>>>>>>>>>>>> Layer dirty: %d\n", layer.nDirty);
    //OutputDebugString(s);

    if (!layer.pbmpDraw->GetSafeHandle() || nScrWidth != layer.nBmpWidth || nScrHeight != layer.nBmpHeight) {
      layer.pbmpDraw->DeleteObject();
      layer.pbmpDraw->CreateCompatibleBitmap(GetDC(), nScrWidth, nScrHeight);
      layer.nBmpWidth = nScrWidth;
      layer.nBmpHeight = nScrHeight;
    }

    CDC dc;
    dc.CreateCompatibleDC(GetDC());
    CBitmap *pOldBitmap = dc.SelectObject(layer.pbmpDraw);
    dc.SetStretchBltMode(HALFTONE);

    bool bFull = (layer.nDirty == 2);

    layer.nDirty = 0;

    if (bFull)
      PatBlt(dc, 0, 0, nScrWidth, nScrHeight, BLACKNESS);

    bool bEmptyBitmap = true;

    for (TImgagesMap::iterator it = layer.mImages.begin(); it != layer.mImages.end(); ++it) {
      TImg &img = it->second;

      if (!img.bDirty)
        continue;

      CRect rcIntersectMt = rcViewMt & img.rcWorldMt;
      if (!rcIntersectMt.IsRectEmpty()) {
        rcIntersectMt = img.rcWorldMt;
        Vec4 rcDst = MT2SCR(Vec4(rcIntersectMt.left, rcIntersectMt.top, rcIntersectMt.right, rcIntersectMt.bottom), 
          Vec2(nScrWidth, nScrHeight), 
          Vec4(rcViewMt.left, rcViewMt.top, rcViewMt.right, rcViewMt.bottom)); 
        int nDstWidth = rcDst.z - rcDst.x;
        int nDstHeight = rcDst.w - rcDst.y;

        if (img.pImage) {
          BITMAPINFO bmpi;
          bmpi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
          bmpi.bmiHeader.biWidth = img.pImage->GetWidth();
          bmpi.bmiHeader.biHeight = -img.pImage->GetHeight();
          bmpi.bmiHeader.biPlanes = 1;
          bmpi.bmiHeader.biBitCount = 32;
          bmpi.bmiHeader.biCompression = 0;
          bmpi.bmiHeader.biSizeImage = img.pImage->GetSize();
          bmpi.bmiHeader.biXPelsPerMeter = 0;
          bmpi.bmiHeader.biYPelsPerMeter = 0;
          bmpi.bmiHeader.biClrUsed = 0;
          bmpi.bmiHeader.biClrImportant = 0;

					uint32* pPixels = img.pImage->GetData();

					if (layer.autoLevelMin != 0 || layer.autoLevelMax != 255)
					{
						uint32* pOldPixels = pPixels;
						int size = img.pImage->GetWidth() * img.pImage->GetHeight();
						pPixels = new uint32[size];
						for (int i = 0; i < size; ++i)
						{
							uint32 pixel = pOldPixels[i];
							int r = ( pixel        & 0xff);
							int g = ((pixel >>  8) & 0xff);
							int b = ((pixel >> 16) & 0xff);
							int a = ((pixel >> 24) & 0xff);
							r = 255 * (r - layer.autoLevelMin) / (layer.autoLevelMax - layer.autoLevelMin);
							g = 255 * (g - layer.autoLevelMin) / (layer.autoLevelMax - layer.autoLevelMin);
							b = 255 * (b - layer.autoLevelMin) / (layer.autoLevelMax - layer.autoLevelMin);
							if (r < 0) r = 0; else if (r > 255) r = 255;
							if (g < 0) g = 0; else if (g > 255) g = 255;
							if (b < 0) b = 0; else if (b > 255) b = 255;
							pPixels[i] = r | (g << 8) | (b << 16) | (a << 24);
						}
					}

					StretchDIBits(dc, 
						rcDst.x, rcDst.y, nDstWidth, nDstHeight, 
						0, 0, img.pImage->GetWidth(), img.pImage->GetHeight(), 
						pPixels, &bmpi, DIB_RGB_COLORS, SRCCOPY);

					if (layer.autoLevelMin != 0 || layer.autoLevelMax != 255)
						delete[] pPixels;

          img.bDirty = false;

          bEmptyBitmap = false;
        } else
          PatBlt(dc, rcDst.x, rcDst.y, nDstWidth, nDstHeight, BLACKNESS);
      }
    }

    if (bEmptyBitmap && bFull) {
      layer.pbmpDraw->DeleteObject();
      layer.nBmpWidth = layer.nBmpHeight = 0;
    }

    dc.SelectObject(pOldBitmap);
  }

}

void CMapWnd::SetZoomLevel(int nLevel)
{
  if (nLevel < 0)
    nLevel = 0;
  if (nLevel >= ZOOM_LEVELS)
    nLevel = ZOOM_LEVELS-1;

  m_nZoomLevel = nLevel;

  m_selectionBitmap.DeleteObject();

  CSWMiniMapUpdater *pmmu = CSWMiniMapUpdater::Get();
  if (pmmu) {
    pmmu->OnSizeChange(m_arrMTperPIX[nLevel], false);
    pmmu->OnWorldRectChange(GetRectMt());
  }

  UpdateLayerBitmap();

  UpdateSelection(m_ptMousePos);

  SetFocus();
}

CMapWnd::~CMapWnd()
{
  DestroyCursor(m_hHandCursor);
  DestroyCursor(m_hCrossCursor);
  m_offscreenBitmap.DeleteObject();
  m_offscreenLayer.DeleteObject();
  m_selectionBitmap.DeleteObject();
  for (TLayersMap::iterator it = m_Layers.begin(); it != m_Layers.end(); ) {
    TLayersMap::iterator itTemp = it;
    ++it;
    itTemp->second.Destroy();
    m_Layers.erase(itTemp);
  }
}

void CMapWnd::DrawLines(CMemDC *pdc, int nWidth, int nHeight)
{
	assert(pdc);

	CRect rcWorldDrawMt = GetRectMt();

	CPen pen(PS_SOLID, 1, RGB(96, 96, 96));
	CPen penHighlight(PS_SOLID, 1, RGB(192, 192, 192));
	CPen penRed(PS_SOLID, 1, RGB(255, 0, 0));
	CPen *pOldPen =pdc->SelectObject(&pen);

	pdc->SetTextColor(RGB(255, 255, 255));
	pdc->SetBkMode(TRANSPARENT);

	char buf[256] = {0};
	int interval = m_nSegmentSizeMt;
	int radius = m_arrMTperPIX[m_nZoomLevel] * interval;
	int textInterval = 16 * interval;
	int aimSize = 5;
	Vec2i center((int)m_ptDrawWorldPosMt.x, (int)m_ptDrawWorldPosMt.y);
	center.x -= center.x % interval;
	center.y -= center.y % interval;
	CRect rcMap(center.x - radius, center.y - radius, center.x + radius, center.y + radius);

	for(int x = rcMap.left; x < rcMap.right; x += interval)
	{
		float u = MAP<float>(x, rcWorldDrawMt.left, rcWorldDrawMt.right, 0, nWidth);
		if(u < 0 || u > nWidth)
			continue;
		pdc->MoveTo(u, 0);
		pdc->LineTo(u, nHeight);
	}
	for(int y = rcMap.top; y < rcMap.bottom; y += interval)
	{
		float v = MAP<float>(y, rcWorldDrawMt.top, rcWorldDrawMt.bottom, 0, nHeight);
		if(v < 0 || v > nHeight)
			continue;
		pdc->MoveTo(0, v);
		pdc->LineTo(nWidth, v);
		if(!(y % textInterval))
		{
			sprintf(buf, "%d", y / m_nSegmentSizeMt);
			pdc->TextOut(0, v, buf);
		}
	}
	{
		int step = interval;
		int x = rcMap.left;
		while(x < rcMap.right)
	{
		float u = MAP<float>(x, rcWorldDrawMt.left, rcWorldDrawMt.right, 0, nWidth);
			if(!(x % textInterval))
			{
				step = textInterval;
				if(u > 0 && u < nWidth)
				{
		sprintf(buf, "%d", x / m_nSegmentSizeMt);
		pdc->TextOut(u, 0, buf);
	}
			}
			x += step;
		}
	}
	pdc->SelectObject(&penHighlight);
	{
		float u = MAP<float>(0, rcWorldDrawMt.left, rcWorldDrawMt.right, 0, nWidth);
		pdc->MoveTo(u, 0);
		pdc->LineTo(u, nHeight);
		float v = MAP<float>(0, rcWorldDrawMt.top, rcWorldDrawMt.bottom, 0, nHeight);
		pdc->MoveTo(0, v);
		pdc->LineTo(nWidth, v);
	}
	pdc->SelectObject(&penRed);
	{
		float u = MAP<float>(m_ptDrawWorldPosMt.x, rcWorldDrawMt.left, rcWorldDrawMt.right, 0, nWidth);
		float v = MAP<float>(m_ptDrawWorldPosMt.y, rcWorldDrawMt.top, rcWorldDrawMt.bottom, 0, nHeight);
		pdc->MoveTo(u - aimSize, v);
		pdc->LineTo(u + aimSize, v);
		pdc->MoveTo(u, v - aimSize);
		pdc->LineTo(u, v + aimSize);
	}
	pdc->SelectObject(pOldPen);
}

void CMapWnd::OnPaint()
{
  RedrawLayerBitmaps();

  CPaintDC paintDC(this);

  CRect rcWnd; GetClientRect(&rcWnd);
  INT nWidth = rcWnd.Width();
  INT nHeight = rcWnd.Height();
  Vec2 szScreen(nWidth, nHeight);

  if (!m_offscreenBitmap.GetSafeHandle())
    m_offscreenBitmap.CreateCompatibleBitmap(&paintDC, nWidth, nHeight);

  CMemDC dc(paintDC, &m_offscreenBitmap);

  dc.FillSolidRect(0, 0, nWidth, nHeight, 0);

  CRect rcWorldDrawMt = GetRectMt();
  Vec4 rcWorldMT = RectToVec4(rcWorldDrawMt);

  if (!m_offscreenLayer.GetSafeHandle())
    m_offscreenLayer.CreateCompatibleBitmap(&paintDC, nWidth, nHeight);

  CDC dcAlpha;
  dcAlpha.CreateCompatibleDC(GetDC());
  CBitmap *pOldBitmap = dcAlpha.SelectObject(&m_offscreenLayer);

  for (TLayersMap::iterator it = m_Layers.begin(); it != m_Layers.end(); ++it) {
    TLayer &layer = it->second;

    if (!layer.pbmpDraw->GetSafeHandle())
      continue;

    long bytecount = layer.nBmpWidth * layer.nBmpHeight * 4;

    BITMAPINFO bmpi;
    bmpi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpi.bmiHeader.biWidth = layer.nBmpWidth;
    bmpi.bmiHeader.biHeight = -layer.nBmpHeight;
    bmpi.bmiHeader.biPlanes = 1;
    bmpi.bmiHeader.biBitCount = 32;
    bmpi.bmiHeader.biCompression = 0;
    bmpi.bmiHeader.biSizeImage = bytecount;
    bmpi.bmiHeader.biXPelsPerMeter = 0;
    bmpi.bmiHeader.biYPelsPerMeter = 0;
    bmpi.bmiHeader.biClrUsed = 0;
    bmpi.bmiHeader.biClrImportant = 0;

    std::vector<unsigned char> bmpbits;
    bmpbits.resize(bytecount);
    layer.pbmpDraw->GetBitmapBits(bytecount, &bmpbits[0]);

    if (layer.btAlpha < 255) {
      StretchDIBits(dcAlpha, 
        0, 0, nWidth, nHeight, 
        0, 0, layer.nBmpWidth, layer.nBmpHeight,
        &bmpbits[0], &bmpi, DIB_RGB_COLORS, SRCCOPY);

      BLENDFUNCTION blend;
      blend.BlendOp = AC_SRC_OVER;
      blend.BlendFlags = 0;
      blend.SourceConstantAlpha = layer.btAlpha;
      blend.AlphaFormat = 0;

      dc.AlphaBlend(0, 0, nWidth, nHeight, &dcAlpha, 0, 0, nWidth, nHeight, blend);
    } else {
      StretchDIBits(dc, 
        0, 0, nWidth, nHeight, 
        0, 0, layer.nBmpWidth, layer.nBmpHeight,
        &bmpbits[0], &bmpi, DIB_RGB_COLORS, SRCCOPY);
    }
  }

	DrawLines(dc, nWidth, nHeight);

  if (1) {
    AUTO_LOCK(CGridMapDlg::s_cs);
    TSegmentMap &segments = m_SegmentsStatus;
			// = 0,
			//1 - SSClr_Purple, //Conflict - modified but not locked by me
			//2 - SSClr_Red, //selected is locked by other
			//3 - SSClr_Orange, //something is locked by other, nothing is locked by us
			//4 - SSClr_Cyan, //something locked by other, something locked by us
			//5 - SSClr_Blue, //selected locked and modified by me
			//6 - SSClr_Green //something locked by us

    COLORREF arrClr[] = {
      RGB(255,255,0),			// 0 - SSClr_Default - not locked (won't be shown at all)
			RGB(255,0,255),			// 1 - SSClr_Purple, //Conflict - modified but not locked by me
      RGB(255,0,0),				// 2 - SSClr_Red, //selected is locked by other
			RGB(255,128,64),		// 3 - SSClr_Orange, //something is locked by other, nothing is locked by us
			RGB(0,255,255),			// 4 - SSClr_Cyan, //something locked by other, something locked by us
			RGB(0,0,255),				// 5 - SSClr_Blue, //selected locked and modified by me
			RGB(0,255,0),				// 6 - SSClr_Green //something locked by us
			RGB(128,128,128),		// 7 - SSClr_Gray //missing
    };

    for (TSegmentMap::iterator it = segments.begin(); it != segments.end(); ++it)
	{
		CPoint ptPos;
		SegIDToWorldCoordPoint(ptPos, it->first);
		ptPos.x *= m_nSegmentSizeMt;
		ptPos.y *= m_nSegmentSizeMt;
		DWORD iClr = it->second;
		if (iClr >= sizeof(arrClr) / sizeof(arrClr[0]))
			iClr = 0;
		COLORREF color = arrClr[iClr];

		CRect rcSegMt(ptPos.x, ptPos.y, ptPos.x + m_nSegmentSizeMt, ptPos.y + m_nSegmentSizeMt);

		CRect rcIntersect = rcWorldDrawMt & rcSegMt; 

		if (!rcIntersect.IsRectEmpty()) {
			Vec4 rcScr = MT2SCR(RectToVec4(rcIntersect), szScreen, rcWorldMT);

			int drx = ftoi(rcScr.x);
			int dry = ftoi(rcScr.y);
			int drw = ftoi(rcScr.z - drx);
			int drh = ftoi(rcScr.w - dry);
			dcAlpha.FillSolidRect(drx, dry, drw, drh, color);

			BLENDFUNCTION blend;
			blend.BlendOp = AC_SRC_OVER;
			blend.BlendFlags = 0;
			blend.SourceConstantAlpha = 100;
			blend.AlphaFormat = 0;

			dc.AlphaBlend(drx, dry, drw, drh, 
				&dcAlpha, 
				drx, dry, drw, drh, 
				blend);
		}
	}
  }

  if (m_editMode == EGEM_CHECK) {
    //if (m_rcMouseScreen.right > m_rcMouseScreen.left) {
    //  CPaintDC paintSelDC(this);

    //  int drw =  m_rcMouseScreen.Width();
    //  int drh =  m_rcMouseScreen.Height();

    //  bool bFillRect = false;
    //  if(!m_selectionBitmap.GetSafeHandle()) {
    //    m_selectionBitmap.CreateCompatibleBitmap(&paintSelDC, drw+5, drh+5);
    //    bFillRect = true;
    //  }

    //  CMemDC dcSel(paintSelDC, &m_selectionBitmap);

    //  if (bFillRect)
    // dcSel.FillSolidRect(0, 0, drw+5, drh+5, RGB(255,0,0));

    //  BLENDFUNCTION blend;
    //  blend.BlendOp = AC_SRC_OVER;
    //  blend.BlendFlags = 0;
    //  blend.SourceConstantAlpha = 128;
    //  blend.AlphaFormat = 0;

    //  dc.AlphaBlend(m_rcMouseScreen.left, m_rcMouseScreen.top, drw, drh, 
    //    &dcSel, 
    //    0, 0, drw, drh, 
    //    blend);
    //}

    TSegmentMap &segments = m_SegmentsCheck;
    COLORREF color = RGB(0,0,0);

    for (TSegmentMap::iterator it = segments.begin(); it != segments.end(); ++it) 
	{
		CPoint ptPos;
		SegIDToWorldCoordPoint(ptPos, it->first);
		ptPos.x *= m_nSegmentSizeMt;
		//ptPos.x = ptPos.x < 0 ? ptPos.x * m_nSegmentSizeMt : (ptPos.x - 1) * m_nSegmentSizeMt;
		ptPos.y *= m_nSegmentSizeMt;
		//ptPos.y = ptPos.y < 0 ? ((ptPos.y + 1) * m_nSegmentSizeMt) * -1 : (ptPos.y * m_nSegmentSizeMt) * -1;

		CRect rcSegMt(ptPos.x, ptPos.y, ptPos.x + m_nSegmentSizeMt, ptPos.y + m_nSegmentSizeMt);

		CRect rcIntersect = rcWorldDrawMt & rcSegMt; 

		if (!rcIntersect.IsRectEmpty()) {
			Vec4 rcScr = MT2SCR(RectToVec4(rcIntersect), szScreen, rcWorldMT);

			int drx = ftoi(rcScr.x);
			int dry = ftoi(rcScr.y);
			int drw = ftoi(rcScr.z - drx);
			int drh = ftoi(rcScr.w - dry);
			dcAlpha.FillSolidRect(drx, dry, drw, drh, color);

			BLENDFUNCTION blend;
			blend.BlendOp = AC_SRC_OVER;
			blend.BlendFlags = 0;
			blend.SourceConstantAlpha = 100;
			blend.AlphaFormat = 0;

			dc.AlphaBlend(drx, dry, drw, drh, 
				&dcAlpha, 
				drx, dry, drw, drh, 
				blend);
		}
	}
  }

  // draw move to area
  ISegmentedWorldDoc& swdoc = GetIEditor()->GetSegmentedWorldManager()->GetDoc();
  int iEditSegX = 0, iEditSegY = 0;
  UINT iEditSegW = 0, iEditSegH = 0;
  if (swdoc.IsOk()) 
  {
	  swdoc.GetOffsetInSegments(iEditSegX, iEditSegY);
	  swdoc.GetSizeInSegments(iEditSegW, iEditSegH);
	  if (iEditSegW > 0 && iEditSegH > 0)
	  {
		  Vec4 vMoveTo(iEditSegX, iEditSegY, iEditSegX + iEditSegW, iEditSegY + iEditSegH);
		  Vec4 vScreen = MT2SCR(vMoveTo * m_nSegmentSizeMt, szScreen, rcWorldMT);
		  
		  CRect rcScreen = Vec4ToRect(vScreen);
		  rcScreen.InflateRect(1, 1);
		  dc.Draw3dRect(&rcScreen, RGB(0, 0, 0), RGB(0, 0, 0));
		  rcScreen.DeflateRect(1, 1);
		  dc.Draw3dRect(&rcScreen, RGB(255, 255, 255), RGB(255, 255, 255));
	  }
  }

  // draw selected area
  if (m_editMode == EGEM_EDIT && m_iEditSegW > 0 && m_iEditSegH > 0 &&
    (m_iEditSegW != iEditSegW || m_iEditSegH != iEditSegH ||
    m_iEditSegX != iEditSegX || m_iEditSegY != iEditSegY))
  {
      Vec4 vScreen = MT2SCR(GetEditRect() * m_nSegmentSizeMt, szScreen, rcWorldMT);

      CRect rcScreen = Vec4ToRect(vScreen);
	  rcScreen.InflateRect(1, 1);
      COLORREF clr1 = (iEditSegW > 0 && iEditSegH > 0) ? RGB(64,64,64) : RGB(0,0,0);
      dc.Draw3dRect(&rcScreen, clr1, clr1);
      COLORREF clr2 = (iEditSegW > 0 && iEditSegH > 0) ? RGB(192,192,192) : RGB(255, 255, 255);
      rcScreen.DeflateRect(1, 1);
      dc.Draw3dRect(&rcScreen, clr2, clr2);
  }

  // draw world boundary
  {
	  Vec4 vScreen = MT2SCR(RectToVec4(m_rcWorldBound) * m_nSegmentSizeMt, szScreen, rcWorldMT);

	  CRect rcScreen = Vec4ToRect(vScreen);
	  dc.Draw3dRect(&rcScreen, RGB(0, 0, 255), RGB(0, 0, 255));
  }

  // draw missing segments
  for(RectList::iterator it = m_rcMissingSeg.begin(); it != m_rcMissingSeg.end(); ++it)
  {
	  Vec4 vScreen = MT2SCR(RectToVec4(*it) * m_nSegmentSizeMt, szScreen, rcWorldMT);

	  CRect rcScreen = Vec4ToRect(vScreen);
	  dc.Draw3dRect(&rcScreen, RGB(255, 255, 0), RGB(255, 255, 0));
  }

  // draw window boundary
  dc.Draw3dRect(0,0,nWidth, nHeight, RGB(100,100,100), RGB(100,100,100));

  // draw mouse
  if (m_rcMouseScreen.right > m_rcMouseScreen.left)
    dc.FrameRect(&m_rcMouseScreen, dc.GetHalftoneBrush());

  //sprintf(chBuf, "%f,%f", m_ptMouseWorldPosMt.x, m_ptMouseWorldPosMt.y);
  //dc.DrawText(chBuf, CRect(0,50,nWidth,nHeight), DT_SINGLELINE);

  dcAlpha.SelectObject(pOldBitmap);
}

UINT CMapWnd::OnGetDlgCode()
{
  return DLGC_WANTARROWS|DLGC_WANTALLKEYS|DLGC_WANTCHARS;
}

void CMapWnd::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
  CWnd::OnKeyUp(nChar, nRepCnt, nFlags);

  switch(nChar)
  {
  case VK_SPACE:
	  m_bSpacePressed = false;
	  SetCursor(m_hCrossCursor);
	  break;

  case VK_SHIFT:
	  m_bShiftPressed = false;
	  break;

  case VK_CONTROL:
	  m_bControlPressed = false;
	  break;

  case VK_ESCAPE:
	  if(IsEditMode(EGEM_MERGE))
		  RestoreEditMode();
	  break;
  }
}

void CMapWnd::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
  CWnd::OnKeyDown(nChar, nRepCnt, nFlags);

  switch(nChar)
  {
  case VK_SPACE:
	  m_bSpacePressed = true;
	  SetCursor(m_hHandCursor);
	  break;

  case VK_SHIFT:
	  m_bShiftPressed = true;
	  break;

  case VK_CONTROL:
	  m_bControlPressed = true;
	  break;

  case VK_HOME:
	  SetPos(Vec2(0,0));
	  UpdateSelection(m_ptMousePos);
	  Invalidate();
	  break;
  }
}

void CMapWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();
	m_bLBPressed = true;

	if (m_bSpacePressed) {
		m_ptScrollPoint.SetPoint(point.x, point.y);
		m_bScrolling = true;
		SetCapture();
		return;
	}

	switch(m_editMode)
	{
	case EGEM_EDIT:
		{
			assert(m_nSegmentSizeMt);
			INT nHalfSize = (float)m_arrEditSize[m_nEditSize] / 2 * m_nSegmentSizeMt;
			CRect rc;
			rc.SetRect(m_ptMousePosSegmentAlignMt.x - nHalfSize, m_ptMousePosSegmentAlignMt.y - nHalfSize, 
			  m_ptMousePosSegmentAlignMt.x + nHalfSize, m_ptMousePosSegmentAlignMt.y + nHalfSize);
			m_iEditSegX = (int32) (rc.left / m_nSegmentSizeMt);
			m_iEditSegY = (int32) (rc.top / m_nSegmentSizeMt);
			m_iEditSegW = m_arrEditSize[m_nEditSize];
			m_iEditSegH = m_arrEditSize[m_nEditSize];
			Invalidate();
		}
		break;

	case EGEM_CHECK:
		{
			CRect rc;
			std::list<CPoint> segments;
			rc.SetRect(m_ptMousePosSegmentAlignMt.x, m_ptMousePosSegmentAlignMt.y, 
			  m_ptMousePosSegmentAlignMt.x + (m_nCheckSize * m_nSegmentSizeMt), 
			  m_ptMousePosSegmentAlignMt.y + (m_nCheckSize * m_nSegmentSizeMt));
			GetSegmentsInScreenRect(rc, segments);

			bool bChanged = false;
			for(std::list<CPoint>::iterator it = segments.begin(); it != segments.end(); ++it )
			  if (SetSegmentState(m_SegmentsCheck, *it, m_bShiftPressed ? 0 : 1))
				bChanged = true;
			if (bChanged)
			  Invalidate();
		}
		break;

	case EGEM_MERGE:
		{
			CRect rc;
			rc.SetRect(m_ptMousePosSegmentAlignMt.x, m_ptMousePosSegmentAlignMt.y, 
			  m_ptMousePosSegmentAlignMt.x + (m_nMergeSize.x * m_nSegmentSizeMt), 
			  m_ptMousePosSegmentAlignMt.y + (m_nMergeSize.y * m_nSegmentSizeMt));
			m_iEditSegX = (int32) (rc.left / m_nSegmentSizeMt);
			m_iEditSegY = (int32) (rc.top / m_nSegmentSizeMt);
			m_iEditSegW = m_nMergeSize.x;
			m_iEditSegH = m_nMergeSize.y;
			Invalidate();
		}
		break;
	}
}

//-------------------------------------------------------------------
void CMapWnd::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	::PostMessage(GetParent()->m_hWnd, WM_USER_ON_DBL_CLICK, 0, 0);
}

//-------------------------------------------------------------------
void CMapWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
	SetFocus();
  m_bLBPressed = false;
  m_bScrolling = false;
  ReleaseCapture();

  clock_t curr_time = clock();

  if ((curr_time - m_lastClickTime) < 200)
    OnLButtonDblClk(nFlags, point);
  m_lastClickTime = curr_time;
}

void CMapWnd::OnRButtonDown(UINT nFlags, CPoint point)
{
	SetFocus();
  m_bRBPressed = true;

  m_ptScrollPoint.SetPoint(point.x, point.y);
  m_bScrolling = true;
  SetCapture();
}

void CMapWnd::OnRButtonUp(UINT nFlags, CPoint point)
{
  m_bRBPressed = false;
  m_bScrolling = false;
  ReleaseCapture();
}

void CMapWnd::OnMouseMove(UINT nFlags, CPoint point)
{
  if (m_bScrolling) {
    Vec2 ptMapPosMtOld = GetPos(&m_ptScrollPoint);
    m_ptScrollPoint.SetPoint(point.x, point.y);
    Vec2 ptMapPosMtNew = GetPos(&m_ptScrollPoint);

    m_ptDrawWorldPosMt.x += (ptMapPosMtOld.x - ptMapPosMtNew.x);
    m_ptDrawWorldPosMt.y += (ptMapPosMtOld.y - ptMapPosMtNew.y);

    SetPos(m_ptDrawWorldPosMt);
  }

  m_ptMouseWorldPosMt = GetPos(&point);

  {
    UpdateSelection(point);
    if (m_bLBPressed)
      OnLButtonDown(0, point);
  }

	//tracking tooltip
	if (!m_bLBPressed && !m_bRBPressed && !m_bSegInfoUpdateSuspended)
	{
		static char chGridCenter[256], chGridTopLeft[256];
		static int oldX, oldY;
		static int oldSegX, oldSegY;
		static int oldTopLeftX, oldTopLeftY;
		int newX, newY;

		// The mouse has just entered the window.
		{
			// Request notification when the mouse leaves.
			TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT) };
			tme.hwndTrack = GetSafeHwnd();
			tme.dwFlags = TME_LEAVE;
			TrackMouseEvent(&tme);

			// Activate the tooltip.
			::SendMessage(m_trackingTT, TTM_TRACKACTIVATE, 
				(WPARAM)TRUE, (LPARAM)&m_ti);
		}

		int wposX = m_ptMouseWorldPosMt.x;
		int wposY = m_ptMouseWorldPosMt.y;

		if (m_ptMouseWorldPosMt.x < 0)
		{
			wposX -= m_nSegmentSizeMt;
		}
		if (m_ptMouseWorldPosMt.y < 0)
		{
			wposY -= m_nSegmentSizeMt;
		}
		
		int segX;
		int segY;
		segX = (int32) (wposX / m_nSegmentSizeMt);
		segY = (int32) (wposY / m_nSegmentSizeMt);
		
		newX = point.x;
		newY = point.y;

		//newX = segX;
		//newY = segY;

		// Make sure the mouse has actually moved. The presence of the tooltip 
		// causes Windows to send the message continuously.
		if ((newX != oldX) || (newY != oldY))
		{
			//AUTO_LOCK(CSWMiniMapUpdater::g_cs);
			oldX = newX;
			oldY = newY;

			assert(CGridMapDlg::Get());

			// Update the text.
			if ((segX != oldSegX) || (segY != oldSegY))
			{
				oldSegX = segX;
				oldSegY = segY;

				sprintf(chGridCenter, "Mouse Focus [%d,%d]\n", segX, segY);
				CGridMapDlg::Get()->m_statusBar.SetPaneText(1, chGridCenter);

				int nSegID;
				POINT pt = {segX, segY};
				WorldCoordPointToSegID(nSegID, pt);

				if (m_bSegInfoUpdateSuspended)
					return;
				AUTO_LOCK(CSWMiniMapUpdater::g_cs);

				string strInfo;
				char chTxt[2048];
				chTxt[0]=0;
				
				std::map<int,TSegmentState>::iterator it = m_mapSegmentStates.find(nSegID);
				if(it != m_mapSegmentStates.end())
				{
					TSegmentState &sls = it->second;
					if (sls.m_SegmentDataStatus.m_nStatus)
					{	
						sprintf(chTxt + strlen(chTxt), " - Terrain: %s(%d)\n", sls.m_SegmentDataStatus.m_strUserName.c_str(), sls.m_SegmentDataStatus.m_nLockedBy);
					}

					for(std::map<int, TSegmentState::TLayerState>::iterator it = sls.m_mapLayersStatus.begin(); it != sls.m_mapLayersStatus.end(); ++it)
					{
						TSegmentState::TLayerState &st = it->second;
						if (st.m_nLockedBy > 0)
						{
							sprintf(chTxt + strlen(chTxt), " - Layer %s(%d): ", st.m_strLayerName.c_str(), it->first);
							sprintf(chTxt + strlen(chTxt), "%s(%d)\n", st.m_strUserName.c_str(), st.m_nLockedBy);
						}
					}
				}
			}

			int topLeftX = (int)(m_rcWorld.x / m_nSegmentSizeMt);
			int topLeftY = (int)(m_rcWorld.y / m_nSegmentSizeMt);
			if((topLeftX != oldTopLeftX) || (topLeftY != oldTopLeftY))
			{
				oldTopLeftX = topLeftX;
				oldTopLeftY = topLeftY;

				sprintf(chGridTopLeft, "[%d,%d]\n", topLeftX, topLeftY);
				//CGridMapDlg::Get()->m_statusBar.SetPaneText(0, chGridTopLeft);

				m_ti.lpszText = (LPSTR)chGridTopLeft;
				::SendMessage(m_trackingTT, TTM_SETTOOLINFO, 0, (LPARAM)&m_ti);

				POINT pt = { m_rcMouseScreen.left, m_rcMouseScreen.top };
				ClientToScreen(&pt);
				::SendMessage(m_trackingTT, TTM_TRACKPOSITION,
					0, (LPARAM)MAKELONG(pt.x, pt.y - 40));
			}
		}
	}
}


void CMapWnd::OnMouseLeave()
{
	// The mouse pointer has left our window.
	// Deactivate the tooltip.
	::SendMessage(m_trackingTT, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)&m_ti);
}

BOOL CMapWnd::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
  if (m_bSpacePressed)
    SetCursor(m_hHandCursor);
  else
    SetCursor(m_hCrossCursor);

  return TRUE;
}

BOOL CMapWnd::OnMouseWheel(UINT fFlags, short zDelta, CPoint point)
{
  ScreenToClient(&point);

  if (m_bControlPressed) {
    if (m_editMode == EGEM_EDIT)
      SetSelecionSize(m_nEditSize + (zDelta > 0 ? 1 : -1));
    else if (m_editMode == EGEM_CHECK)
      SetSelecionSize(m_nCheckSize + (zDelta > 0 ? 1 : -1));
  } else {
    Vec2 ptMapPos = GetPos(&point);
    SetZoomLevel(m_nZoomLevel + (zDelta > 0 ? 1 : -1));
    SetPos(ptMapPos, &point);

    UpdateSelection(m_ptMousePos);
  }

  return FALSE;
}

void CMapWnd::SetPos(Vec2 ptPosMt, CPoint *ptScreenPos) // ptPosMt - in meters, ptScreenPos - screen pix
{
  if (!ptScreenPos)
    m_ptDrawWorldPosMt = ptPosMt;
  else {
    CRect rcWnd; GetClientRect(&rcWnd);

    float fszmx = ((float)rcWnd.Width() * m_arrMTperPIX[m_nZoomLevel]) / 2;
    float fszmy = ((float)rcWnd.Height() * m_arrMTperPIX[m_nZoomLevel]) / 2;

    Vec2 ptMapOffsetMt;
    ptMapOffsetMt.x = MAP<float>(ptScreenPos->x, 0, rcWnd.Width(), -fszmx, fszmx);
    ptMapOffsetMt.y = MAP<float>(ptScreenPos->y, 0, rcWnd.Height(), -fszmy, fszmy);

    m_ptDrawWorldPosMt = ptPosMt;
    m_ptDrawWorldPosMt.x -= ptMapOffsetMt.x;
    m_ptDrawWorldPosMt.y -= ptMapOffsetMt.y;
  }

  //if (CSWManager *psw = CSWManager::Get())
  //	psw->OnWorldMapUIMoved(m_ptDrawWorldPosMt, m_nZoomLevel);

  CSWMiniMapUpdater *pmmu = CSWMiniMapUpdater::Get();
  if (pmmu)
    pmmu->OnWorldRectChange(GetRectMt());

  UpdateLayerBitmap();

  ((CGridMapDlg*)GetParent())->UpdatePosition();
}

Vec2 CMapWnd::GetPos(CPoint *ptScreenPos) // return map pos in meters of the ptScreenPos(screen pix) default is center of the screen
{
  if (!ptScreenPos)
    return m_ptDrawWorldPosMt;

  CRect rcWnd; GetClientRect(&rcWnd);

  float fszmx = ((float)rcWnd.Width() * m_arrMTperPIX[m_nZoomLevel]) / 2;
  float fszmy = ((float)rcWnd.Height() * m_arrMTperPIX[m_nZoomLevel]) / 2;

  float x = MAP<float>(ptScreenPos->x, 0, rcWnd.Width(), m_ptDrawWorldPosMt.x - fszmx, m_ptDrawWorldPosMt.x + fszmx);
  float y = MAP<float>(ptScreenPos->y, 0, rcWnd.Height(), m_ptDrawWorldPosMt.y - fszmy, m_ptDrawWorldPosMt.y + fszmy);

  return Vec2(x, y);
}

CRect CMapWnd::GetRectMt()
{
  CRect rcWnd; GetClientRect(&rcWnd);

  float fwidthmt = (float)rcWnd.Width() * m_arrMTperPIX[m_nZoomLevel];
  float fheightmt = (float)rcWnd.Height() * m_arrMTperPIX[m_nZoomLevel];

  CRect rcmt;
  rcmt.left = m_ptDrawWorldPosMt.x - (fwidthmt / 2);
  rcmt.top = m_ptDrawWorldPosMt.y - (fheightmt / 2);
  rcmt.right = rcmt.left + fwidthmt;
  rcmt.bottom = rcmt.top + fheightmt;

  return rcmt;
}

int CMapWnd::GetSelectionSize()
{
	if(m_editMode == EGEM_EDIT)
		return m_nEditSize;
	else if(m_editMode == EGEM_CHECK)
		return m_nCheckSize;

	return EDIT_1K;
}

void CMapWnd::SetSelecionSize(int nSelSize)
{
  if (m_editMode == EGEM_EDIT)
  {
	  m_nEditSize = CLAMP(nSelSize, EDIT_1K, EDIT_SIZES - 1);
  }
  else if(m_editMode == EGEM_CHECK)
  {
	  m_nCheckSize = CLAMP(nSelSize, EDIT_2K, EDIT_SIZES - 1);
  }

  m_selectionBitmap.DeleteObject();

  UpdateSelection(m_ptMousePos);

  SetFocus();

  Invalidate();
}

void CMapWnd::SetSelecionPos(Vec2 ptPos) // in meters
{
  CRect rcWnd; GetClientRect(&rcWnd);

  assert(m_nSegmentSizeMt);
  Vec2 ptSegmentSizePx;
  ptSegmentSizePx.x = ((float)rcWnd.Width() * m_arrMTperPIX[m_nZoomLevel]) / m_nSegmentSizeMt;
  ptSegmentSizePx.y = ((float)rcWnd.Height() *m_arrMTperPIX[m_nZoomLevel]) / m_nSegmentSizeMt;

  Vec2 ptWorldPosMt = ptPos;
  float fHalfSizeMt = m_nSegmentSizeMt * 0.5f;
  switch(m_editMode)
  {
  case EGEM_EDIT:
	  ptWorldPosMt.x += (ptWorldPosMt.x < 0) ? -fHalfSizeMt : fHalfSizeMt;
	  ptWorldPosMt.y += (ptWorldPosMt.y < 0) ? -fHalfSizeMt : fHalfSizeMt;
	  break;

  case EGEM_CHECK:
	  ptWorldPosMt.x += (ptWorldPosMt.x < 0) ? -(m_nCheckSize+1) * fHalfSizeMt : -(m_nCheckSize-1) * fHalfSizeMt;
	  ptWorldPosMt.y += (ptWorldPosMt.y < 0) ? -(m_nCheckSize+1) * fHalfSizeMt : -(m_nCheckSize-1) * fHalfSizeMt;
	  break;

  case EGEM_MERGE:
	  ptWorldPosMt.x += (ptWorldPosMt.x < 0) ? -(m_nMergeSize.x+1) * fHalfSizeMt : -(m_nMergeSize.x-1) * fHalfSizeMt;
	  ptWorldPosMt.y += (ptWorldPosMt.y < 0) ? -(m_nMergeSize.y+1) * fHalfSizeMt : -(m_nMergeSize.y-1) * fHalfSizeMt;
	  break;
  }

  m_ptMousePosSegmentAlignMt.x = (int)(ptWorldPosMt.x / m_nSegmentSizeMt) * m_nSegmentSizeMt;
  m_ptMousePosSegmentAlignMt.y = (int)(ptWorldPosMt.y / m_nSegmentSizeMt) * m_nSegmentSizeMt;

  CRect rcWorldDrawMt = GetRectMt();

  float fHalfSelSizeMt;
  switch(m_editMode)
  {
  case EGEM_EDIT:
		fHalfSelSizeMt = m_arrEditSize[m_nEditSize] * fHalfSizeMt;
		m_rcWorld.x = m_ptMousePosSegmentAlignMt.x - fHalfSelSizeMt;
		m_rcWorld.y = m_ptMousePosSegmentAlignMt.y - fHalfSelSizeMt;
		m_rcWorld.z = m_ptMousePosSegmentAlignMt.x + fHalfSelSizeMt;
		m_rcWorld.w = m_ptMousePosSegmentAlignMt.y + fHalfSelSizeMt;
	  break;

  case EGEM_CHECK:
		m_rcWorld.x = m_ptMousePosSegmentAlignMt.x;
		m_rcWorld.y = m_ptMousePosSegmentAlignMt.y;
		m_rcWorld.z = m_rcWorld.x + (m_nCheckSize * m_nSegmentSizeMt);
		m_rcWorld.w = m_rcWorld.y + (m_nCheckSize * m_nSegmentSizeMt);
	  break;

  case EGEM_MERGE:
		m_rcWorld.x = m_ptMousePosSegmentAlignMt.x;
		m_rcWorld.y = m_ptMousePosSegmentAlignMt.y;
		m_rcWorld.z = m_rcWorld.x + (m_nMergeSize.x * m_nSegmentSizeMt);
		m_rcWorld.w = m_rcWorld.y + (m_nMergeSize.y * m_nSegmentSizeMt);
	  break;
  }

  Vec4 rcScreen = MT2SCR(m_rcWorld, Vec2(rcWnd.Width(), rcWnd.Height()), Vec4(rcWorldDrawMt.left, rcWorldDrawMt.top, rcWorldDrawMt.right, rcWorldDrawMt.bottom));

  CRect rcOld = m_rcMouseScreen;
  m_rcMouseScreen.SetRectEmpty();

  m_rcMouseScreen.left = rcScreen.x;
  m_rcMouseScreen.right = rcScreen.z;
  m_rcMouseScreen.bottom = rcScreen.w;
  m_rcMouseScreen.top = rcScreen.y;

  CRect rcDirty;
  UnionRect(&rcDirty, &m_rcMouseScreen, &rcOld);
  InvalidateRect(&rcDirty, true);
}

void CMapWnd::UpdateSelection(CPoint ptMouse)
{
  m_ptMousePos = ptMouse;

  Vec2 ptWorldPosMt = GetPos(&ptMouse);
  SetSelecionPos(ptWorldPosMt);
}

bool CMapWnd::SetSegmentState(TSegmentMap &Segments, CPoint ptPos, DWORD dwState)
{
  bool bChanged = false;
  int uiID;
	WorldCoordPointToSegID(uiID, ptPos);
  TSegmentMap::iterator it = Segments.find(uiID);
  if (it == Segments.end()) {
    if (dwState) {
      bChanged = true;
      DWORD &state = Segments[uiID];
      state = dwState;
    }
  } else {
    if (dwState) {
      if (it->second != dwState) {
        bChanged = true;
        it->second = dwState;
      }
    } else {
      bChanged = true;
      Segments.erase(it);
    }
  }
  return bChanged;
}

DWORD CMapWnd::GetSegmentState(TSegmentMap const&Segments, CPoint ptPos) const
{
  
	int uiID;
	WorldCoordPointToSegID(uiID, ptPos);
  TSegmentMap::const_iterator it = Segments.find(uiID);
  if (it != Segments.end())
    return it->second;
  return 0;
}

void CMapWnd::GetSegmentsInScreenRect(CRect rcMt, std::list<CPoint> &segments)
{
  assert(m_nSegmentSizeMt);
  int x1 = (int) (rcMt.left / m_nSegmentSizeMt);
  int x2 = (int) (rcMt.right / m_nSegmentSizeMt);
  int y1 = (int) (rcMt.top / m_nSegmentSizeMt);
  int y2 = (int) (rcMt.bottom / m_nSegmentSizeMt);

  for (int y = y1; y < y2; ++y)
    for (int x = x1; x < x2; ++x)
      segments.push_back(CPoint(x, y));

  //INT sx = rcMt.Width() / m_nSegmentSizeMt;
  //INT sy = rcMt.Height() / m_nSegmentSizeMt;

  //INT dx = min(rcMt.left, rcMt.right) / m_nSegmentSizeMt;
  //if (dx >= 0)
  //  dx++;
  //INT dy = -max(rcMt.top, rcMt.bottom) / m_nSegmentSizeMt;
  //if (dy >= 0)
  //  dy++;

  //INT ay = 0;
  //for (INT y = 0; y < sy; y++) {
  //  INT ax = 0;
  //  for (INT x = 0; x < sx; x++) {
  //    
  //    // skip zero coords - they are invalid
  //    if (dx + x == 0) ax = 1;
  //    if (dy + y == 0) ay = 1;

  //    CPoint pt(dx + x + ax, dy + y + ay);
  //    segments.push_back(pt);
  //  }
  //}
}

void CMapWnd::SetEditSeg(int iEditSegX, int iEditSegY, int iEditSegW, int iEditSegH)
{
	if (iEditSegX == m_iEditSegX && iEditSegY == m_iEditSegY &&
		iEditSegW == m_iEditSegW && iEditSegH == m_iEditSegH)
		return;
	m_iEditSegX = iEditSegX;
	m_iEditSegY = iEditSegY;
	m_iEditSegW = iEditSegW;
	m_iEditSegH = iEditSegH;

	switch (iEditSegW) {
	case 2:
		m_nEditSize = EDIT_1K;
		break;
	case 4:
		m_nEditSize = EDIT_2K;
		break;
	case 8:
		m_nEditSize = EDIT_4K;
		break;
	case 16:
		m_nEditSize = EDIT_8K;
		break;
	case 32:
		m_nEditSize = EDIT_16K;
		break;
	};

	Invalidate();
}

void CMapWnd::PrepareSegmentsForDelete( std::vector<CPoint> &pts )
{
	CRect rcInt, rc;
	if (m_editMode == EGEM_CHECK)
	{
		for (CMapWnd::TSegmentMap::const_iterator iter = m_SegmentsCheck.begin(); iter != m_SegmentsCheck.end(); ++iter)
		{
			if (iter->second)	// means it's checked
			{
				POINT pt;
				SegIDToWorldCoordPoint(pt, iter->first);
				pts.push_back(pt);

				rc = CRect(pt.x, pt.y, pt.x + 1, pt.y + 1);
				if(rcInt.IntersectRect(m_rcWorldBound, rc))
					m_rcMissingSeg.push_back(rc);
			}
		}
	}
	else if(m_editMode == EGEM_EDIT)
	{
		for (int dy = 0; dy < m_iEditSegH; ++dy)
		{
			for (int dx = 0; dx < m_iEditSegW; ++dx)
			{
				CPoint pt(m_iEditSegX+dx, m_iEditSegY+dy);
				pts.push_back(pt);

				rc = CRect(pt.x, pt.y, pt.x + 1, pt.y + 1);
				if(rcInt.IntersectRect(m_rcWorldBound, rc))
					m_rcMissingSeg.push_back(rc);
			}
		}
	}
}

void CMapWnd::PrepareSegmentsForMerge(CRect &rect)
{
	if(m_editMode != EGEM_MERGE)
	{
		rect.SetRectEmpty();
		return;
	}

	rect = Vec4ToRect(GetEditRect());

	CRect rcInt;
	RectList::iterator it = m_rcMissingSeg.begin();
	for( ; it != m_rcMissingSeg.end(); )
	{
		if(rcInt.IntersectRect(*it, rect))
			it = m_rcMissingSeg.erase(it);
		else
			++it;
	}
}

void CMapWnd::UpdateWorldBound(CRect &newWorldBound, const char *levelpath)
{
	// world bound keeps unchanged
	if(newWorldBound == m_rcWorldBound)
	{
		Invalidate();
		return;
	}

	CRect rcSub;
	rcSub.SubtractRect(newWorldBound, m_rcWorldBound);

	// world bound becomes smaller
	if(rcSub.IsRectEmpty())
	{
		CRect rcInt;
		RectList::iterator it = m_rcMissingSeg.begin();
		for( ; it != m_rcMissingSeg.end(); )
		{
			if(!rcInt.IntersectRect(*it, newWorldBound))
				it = m_rcMissingSeg.erase(it);
			else
				++it;
		}
	}
	else
	{
		// world bound is enlarged
		CSWManager::GetMissingSegmentsInRect(levelpath, rcSub, m_rcMissingSeg);
	}

	m_rcWorldBound = newWorldBound;

	Invalidate();
}