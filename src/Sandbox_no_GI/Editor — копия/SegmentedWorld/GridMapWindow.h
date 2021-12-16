#ifndef __Grid_Map_Window_h__
#define __Grid_Map_Window_h__

 enum EGridEditSize
 {
	 EDIT_1K = 0,
	 EDIT_2K,
	 EDIT_4K,
	 EDIT_8K,
	 EDIT_16K,
	 EDIT_SIZES,
 };

 enum EGridEditMode
 {
	 EGEM_EDIT = 0,
	 EGEM_CHECK,
	 EGEM_MERGE,
 };

#define WM_USER_ON_DBL_CLICK	(WM_USER + 1)

class CMemDC;

class CMapWnd : public CWnd
{
  DECLARE_DYNAMIC(CMapWnd)
public:
  static CryCriticalSection m_cs;
  
  TOOLINFO m_ti;
  HWND m_trackingTT;

  INT m_nSegmentSizeMt;

  CPoint m_ptScrollPoint;
  CPoint m_ptMousePos;

  CBitmap	m_offscreenBitmap;
  CBitmap	m_offscreenLayer;
  CBitmap	m_selectionBitmap;

  HCURSOR m_hCrossCursor;
  HCURSOR m_hHandCursor;
  HCURSOR m_hOrgCursor;

  BOOL m_bControlPressed;
  BOOL m_bShiftPressed;
  BOOL m_bSpacePressed;
  BOOL m_bRBPressed;
  BOOL m_bLBPressed;
  BOOL m_bScrolling;

  Vec2 m_ptDrawWorldPosMt;
  Vec2 m_ptMouseWorldPosMt;
  Vec2 m_ptMousePosSegmentAlignMt;

  clock_t m_lastClickTime;

  CRect m_rcMouseScreen;
  Vec4 m_rcWorld;

  CRect m_rcWorldBound;
  typedef std::list<CRect> RectList;
  std::list<CRect> m_rcMissingSeg;

  struct TImg {
    CImage *pImage;
    BOOL bOwnImage;
    CRect rcWorldMt;
    BOOL bDirty;

    inline TImg() 
    { 
      pImage = 0;
      bOwnImage = false; 
      rcWorldMt.SetRect(0,0,0,0);
      bDirty = false;
    }

    inline void Destroy()
    {
      if (pImage && bOwnImage)
        delete pImage;
      pImage = 0;
    }

    inline BOOL IsValid()
    {
      return !!pImage;
    }
  };
  typedef std::map<DWORD, TImg> TImgagesMap;

  struct TLayer {
    BYTE btAlpha;
    TImgagesMap mImages;
    CBitmap *pbmpDraw;
    int nBmpWidth;
    int nBmpHeight;
    int nDirty;
		int histogram[256];
		int histogramSum;
		int autoLevelMin;
		int autoLevelMax;

    inline TLayer() 
    { 
      btAlpha = 255; 
      nBmpWidth = nBmpHeight = 0;
      pbmpDraw = new CBitmap();
      nDirty = 0;
			for (int i = 0; i < 256; ++i)
				histogram[i] = 0;
			histogramSum = 0;
			autoLevelMin = 0;
			autoLevelMax = 255;
    }

    inline void Destroy() 
    {
      for (TImgagesMap::iterator it = mImages.begin(); it != mImages.end(); ++it) 
      {
        if (it->second.bOwnImage)
          it->second.Destroy();
      }

      pbmpDraw->DeleteObject();
    }

		void AddImageToHistogram (CImage* pImage)
		{
			if (pImage == 0)
				return;

			int size = pImage->GetWidth() * pImage->GetHeight();
			uint32* pData = pImage->GetData();
			for (int i = 0; i < size; ++i)
			{
				uint32 pixel = pData[i];
				int r = ( pixel        & 0xff);
				int g = ((pixel >>  8) & 0xff);
				int b = ((pixel >> 16) & 0xff);
				++histogram[(r + g + b) / 3];
			}
			histogramSum += size;
		}

		void RemoveImageFromHistogram (CImage* pImage)
		{
			if (pImage == 0)
				return;

			int size = pImage->GetWidth() * pImage->GetHeight();
			uint32* pData = pImage->GetData();
			for (int i = 0; i < size; ++i)
			{
				uint32 pixel = pData[i];
				int r = ( pixel        & 0xff);
				int g = ((pixel >>  8) & 0xff);
				int b = ((pixel >> 16) & 0xff);
				--histogram[(r + g + b) / 3];
			}
			histogramSum -= size;
		}

		void GetHistogramMinMax (int* pMin, int* pMax)
		{
			const float cutFraction = 0.01f;
			int maxSum = (int)(cutFraction * (float)histogramSum + 0.5f);

			int iMin = 0;
			int iMax = 255;
			int sum = 0;
			while (iMin < 255 && sum < maxSum)
				sum += histogram[iMin++];
			if (iMin > 0) --iMin;
			sum = 0;
			while (iMax >= 0 && sum < maxSum)
				sum += histogram[iMax--];
			if (iMax < 255) ++iMax;

			if (iMin >= iMax)
			{
				iMin = 0;
				iMax = 255;
			}

			*pMin = iMin;
			*pMax = iMax;
		}
  };

  typedef std::map<DWORD, TLayer> TLayersMap;
  TLayersMap m_Layers;

#define ZOOM_LEVELS 6
  INT m_arrMTperPIX[ZOOM_LEVELS];
  INT m_nZoomLevel;

  typedef std::map<int, DWORD> TSegmentMap;

  INT m_nEditSize;
  INT m_arrEditSize[EDIT_SIZES];
  int m_iEditSegX, m_iEditSegY, m_iEditSegW, m_iEditSegH;

  TSegmentMap m_SegmentsStatus;
	std::map<int,TSegmentState> m_mapSegmentStates;
	bool m_bSegInfoUpdateSuspended;

  INT m_nCheckSize;
  TSegmentMap m_SegmentsCheck;

  Vec2i m_nMergeSize;
  EGridEditMode m_editMode;
  EGridEditMode m_lastEditMode;

  //
public:
  CMapWnd();
  virtual ~CMapWnd();

  void Init();

  void UpdateWorld(CRect *rcWorldMt = 0);

  void SetLayerImage(DWORD dwLayer, DWORD dwID, CImage *pImage, CRect *prcWorldMt = 0, bool bOwnImage = false); 
  void SetLayerImage(DWORD dwLayer, DWORD dwID, const char *pcFileName, CRect *prcWorldMt = 0); 
  TImg *GetLayerImage(DWORD dwLayer, DWORD dwID);
  void SetLayerAlpha(DWORD dwLayer, BYTE btAlpha);
  TLayer *GetLayer(DWORD dwLayer);
  void DelLayer(DWORD dwLayer);

  void UpdateLayerBitmap(DWORD dwLayer = (DWORD)-1, DWORD dwBitmap = (DWORD)-1);
  void RedrawLayerBitmaps();

  void SetPos(Vec2 ptPosMt, CPoint *ptScreenPos = 0);  // ptPos - in meters, ptScreenPos - screen pix

  Vec2 GetPos(CPoint *ptScreenPos = 0);  // return map pos in meters of the ptScreenPos(screen pix) default is center of the screen
  CRect GetRectMt();

  void SetZoomLevel(int nLevel);

  int GetSelectionSize();
  void SetSelecionSize(int nSelSize);
  void SetSelecionPos(Vec2 ptPos);  // in meters
  void UpdateSelection(CPoint ptMouse);

  bool SetSegmentState(TSegmentMap &Segments, CPoint ptPos, DWORD dwState);
  DWORD GetSegmentState(TSegmentMap const&Segments, CPoint ptPos) const;
  void PrepareSegmentsForDelete(std::vector<CPoint> &pts);
  void PrepareSegmentsForMerge(CRect &rect);

  void SetEditSeg(int iEditSegX, int iEditSegY, int iEditSegW, int iEditSegH);
  Vec4 GetEditRect() { return Vec4(m_iEditSegX, m_iEditSegY, m_iEditSegX + m_iEditSegW, m_iEditSegY + m_iEditSegH); }

  void SetEditMode(EGridEditMode mode) { m_lastEditMode = m_editMode; m_editMode = mode; }
  EGridEditMode GetEditMode() { return m_editMode; }
  bool IsEditMode(EGridEditMode mode) const { return m_editMode == mode; }
  void RestoreEditMode() { m_editMode = m_lastEditMode; }

  void UpdateWorldBound(CRect &newWorldBound, const char *levelpath);

protected:
  void GetSegmentsInScreenRect(CRect rcMt, std::list<CPoint> &segments);
	HWND CreateTrackingToolTip(int toolID, HWND hDlg, LPSTR pText);
	void DrawLines(CMemDC *pdc, int nWidth, int nHeight);

protected:
  afx_msg void OnPaint();
  afx_msg UINT OnGetDlgCode();
  afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
  afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
  afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
  afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
  afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
  afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
  afx_msg BOOL OnMouseWheel(UINT fFlags, short zDelta, CPoint point);

  afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
  afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
  DECLARE_MESSAGE_MAP()
};

template <class T> inline T MAP(T n, T srcmin, T srcmax, T dstmin, T dstmax)
{
	return (n - srcmin) * (dstmax - dstmin) / (srcmax - srcmin) + dstmin;
}

static Vec2 MT2SCR(Vec2 ptMt, Vec2 szScr, Vec4 rcMt)
{
	float x = MAP<float>(ptMt.x, rcMt.x, rcMt.z, 0, szScr.x);
	float y = MAP<float>(ptMt.y, rcMt.y, rcMt.w, 0, szScr.y);
	return Vec2(x, y);
}

static Vec4 MT2SCR(Vec4 rcSrcMt, Vec2 szScr, Vec4 rcMt)
{
	Vec2 tl = MT2SCR(Vec2(rcSrcMt.x, rcSrcMt.y), szScr, rcMt);
	Vec2 br = MT2SCR(Vec2(rcSrcMt.z, rcSrcMt.w), szScr, rcMt);
	return Vec4(tl.x, tl.y, br.x, br.y);
}

static Vec4 MT2PX(Vec4 rcMt, Vec4 rcImgMt, Vec2 szImgPx)
{
	Vec2 tl;
	tl.x = MAP<float>(rcMt.x, rcImgMt.x, rcImgMt.z, 0, szImgPx.x);
	tl.y = MAP<float>(rcMt.y, rcImgMt.y, rcImgMt.w, 0, szImgPx.y);
	Vec2 br;
	br.x = MAP<float>(rcMt.z, rcImgMt.x, rcImgMt.z, 0, szImgPx.x);
	br.y = MAP<float>(rcMt.w, rcImgMt.y, rcImgMt.w, 0, szImgPx.y);
	return Vec4(tl.x, tl.y, br.x, br.y);
}

inline static void SegIDToWorldCoordPoint(POINT &pt, int nSegID)
{
	pt.y = (int)((short)((nSegID >> 16)&0xFFFF));
	pt.x = (int)((short)(nSegID&0xFFFF));
}

inline static void WorldCoordPointToSegID(int &nSegID, POINT pt)
{
	nSegID = ((pt.y << 16) | (pt.x & 0xFFFF));
}

inline static Vec4 RectToVec4(CRect &rect)
{
	return Vec4(rect.left, rect.top, rect.right, rect.bottom);
}

inline static CRect Vec4ToRect(Vec4 &rect)
{
	return CRect(rect.x, rect.y, rect.z, rect.w);
}

#endif // __Grid_Map_Window_h__