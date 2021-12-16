////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2014.
// -------------------------------------------------------------------------
//  File name:   SegmentStatusPanel.h
//  Version:     v1.00
//  Created:     17/2/2014 by Allen Chen
//  Compilers:   Visual Studio.NET
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __GRID_MAP_RENDER_WND_H__
#define __GRID_MAP_RENDER_WND_H__

#include "2DViewport.h"

struct SSegmentTexture
{
	int textureId;
	Recti bbox;

	SSegmentTexture()
	{
		textureId = 0;
		bbox = Recti(0, 0, 0, 0);
	}
};

typedef std::map<int, SSegmentTexture> SegmentTextureMap;

struct SSegmentTextureLayer
{
	int width;
	int height;

	SegmentTextureMap textures;

	SSegmentTextureLayer()
	{
		width = height = 0;
	}
};

class CGridMapRenderWnd : public C2DViewport
{
	DECLARE_DYNCREATE(CGridMapRenderWnd);
public:
	CGridMapRenderWnd();
	virtual ~CGridMapRenderWnd();

	void DeleteLayer();
	void SetLayerImage(int segmentId, CImageEx *pImage, const Recti &rect);
	void UpdateViewSettings(bool bLoading);
	void UpdateView();

protected:
	void Draw(DisplayContext &dc);
	void UpdateContent(int flags);

	void SetScrollOffset(float x,float y,bool bLimits=true);
	void SetZoom(float fZoomFactor, CPoint center);
	void SetZoomRange(float minZoomFactor, float maxZoomFactor);

	DECLARE_MESSAGE_MAP()

private:
	SSegmentTextureLayer m_layer;
	class CSegmentSelectTool *m_pSegmentTool;

	float m_minZoomFactor;
	float m_maxZoomFactor;

	bool m_bContentsUpdated;
	bool m_bFirstViewportUpdate;
};

#endif // __GRID_MAP_RENDER_WND_H__