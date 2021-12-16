////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001.
// -------------------------------------------------------------------------
//  File name:   TerrainMiniMapTool.h
//  Version:     v1.00
//  Created:     11/1/2002 by Timur.
//  Compilers:   Visual C++ 6.0
//  Description: Terrain MiniMap tool.
// -------------------------------------------------------------------------
//  History:     3/3/2010 Refactoring by Sergiy Shaykin
//
////////////////////////////////////////////////////////////////////////////

#ifndef __TerrainMiniMapTool_h__
#define __TerrainMiniMapTool_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include "EditTool.h"
#include "Mission.h"

//////////////////////////////////////////////////////////////////////////
class CTerrainMiniMapTool : public CEditTool, public IEditorNotifyListener, public IScreenshotCallback
{
	DECLARE_DYNCREATE(CTerrainMiniMapTool)
public:
	CTerrainMiniMapTool();
	virtual ~CTerrainMiniMapTool();

	//////////////////////////////////////////////////////////////////////////
	// CEditTool
	//////////////////////////////////////////////////////////////////////////
	virtual void BeginEditParams( IEditor *ie,int flags );
	virtual void EndEditParams();
	virtual void Display( DisplayContext &dc );
	virtual bool MouseCallback( CViewport *view,EMouseEvent event,CPoint &point,int flags );
	virtual void DeleteThis() { delete this; };
	//////////////////////////////////////////////////////////////////////////

	// IEditorNotifyListener
	virtual void OnEditorNotifyEvent( EEditorNotifyEvent event );

	SMinimapInfo GetMinimap(){ return m_minimap; }
	CString GetPath(){ return m_path; }
	void SetPath(const CString& path ){ m_path = path; }
	CString GetFilename(){ return m_filename; }
	void SetFilename(const CString& filename ){ m_filename = filename; }

	void SetResolution(int nResolution);
	void SetCameraHeight(float fHeight);
	void AddToLevelFolder();
	void Generate();

	// IScreenshotCallback
	void SendParameters(void* data, uint32 width, uint32 height, f32 minx, f32 miny, f32 maxx, f32 maxy);

private:

	bool m_bDragging;
	SMinimapInfo m_minimap;

	CString m_path;
	CString m_filename; // without path and extension 

	std::map<string,float> m_ConstClearList;
	bool b_stateScreenShot;
};


#endif // __TerrainMiniMapTool_h__