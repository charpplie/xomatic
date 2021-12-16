/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2001-2004.
	-------------------------------------------------------------------------
	$Id: AssetModelItem.h,v 1.0 2009/04/15 11:00:00 PauloZaffari Exp wwwrun $
	$DateTime$
	Description: Header file for the class implementing IAssetDisplayItem
	interface. It declares the headers of the actual used 
	functions.
	-------------------------------------------------------------------------
	History:
	- 15/04/2009	11:00 : Created by Paulo Zaffari
	- 15/03/2010	19:51 : Nicusor Nedelcu - refactored
*************************************************************************/
#ifndef __AssetModelItem_H__
#define __AssetModelItem_H__

#pragma once

struct  IStatObj;

#include "Include/IAssetDisplay.h"
#include <I3DEngine.h>
//#include <IEntitySystem.h>
//#include <ICryAnimation.h>
//#include <IEntityRenderState.h>
#include <IRenderAuxGeom.h>
//#include <ParticleParams.h>
#include <IShader.h>
#include "Util/GdiUtil.h"

class CAssetModelItem: public IAssetDisplay
{
	public:

		static const int	kAssetDisplay_MaxThumbImageBufferSize = 2048*2048;

		CAssetModelItem();
		~CAssetModelItem();
		

		IAssetDisplayDatabase*	GetOwnerDisplayDatabase();
		void										SetOwnerDisplayDatabase( IAssetDisplayDatabase* piOwnerDisplayDatabase );
		const std::vector<string>& GetDependencies();
		void										SetFileSize( unsigned __int64 aSize );
		unsigned __int64				GetFileSize();
		void										SetFilename( const char* pName );
		const char*							GetFilename();
		void										SetRelativePath( const char* pPath );
		const char*							GetRelativePath();
		void										SetFileExtension( const char* pExt );
		const char*							GetFileExtension();
		void										SetUsedInLevel( bool bUsedInLevel )
		{ m_bUsedInLevel = bUsedInLevel; }
		bool										IsUsedInLevel() const
		{ return m_bUsedInLevel; }
		UINT										GetFlags() const;
		void										SetFlags( UINT aFlags );
		void										SetFlag( EAssetFlags aFlag, bool bSet = true );
		bool										IsFlagSet( EAssetFlags aFlag );
		void										SetIndex( UINT aIndex );
		UINT										GetIndex();
		bool										GetAssetFieldValue( const char* pFieldName, void* pDest );
		void										GetDrawingRectangle( CRect& rstDrawingRectangle ) const;
		void										SetDrawingRectangle( const CRect& crstDrawingRectangle );
		bool										HitTest( int nX, int nY );
		bool										HitTest( const CRect& roTestRect );
		bool										Cache();
		bool										CacheFieldsInfo();
		bool										UnCache();
		bool										UnCacheThumbnail();
		void										InteractiveRender( HWND hRenderWindow, const CRect& rstViewport, int aMouseX, int aMouseY, int aMouseDeltaX, int aMouseDeltaY, UINT aKeyFlags );
		bool										Render( HWND hRenderWindow, const CRect& rstViewport, bool bCacheThumbnail = false );
		bool										DrawThumbImage( HDC hDC, const CRect& rRect );
		bool										SaveReportImage( const char *filePath ) const;
		bool										SaveReportText( const char *filePath ) const;
		void										FreeData();
		void										CacheFieldsInfoForLoadedStatObj( IStatObj *pStatObj );

		// from IUnknown - Inherited through IClassDesc.
		HRESULT STDMETHODCALLTYPE QueryInterface( const IID &riid, void **ppvObj ); 
		ULONG STDMETHODCALLTYPE AddRef();
		ULONG STDMETHODCALLTYPE Release(); 

protected:
	void							SetCamera( CCamera &cam,const CRect &rcViewportRect );
	void							CalculateCameraPosition();
	void							DrawGrid();
	void							DrawTextOnReportImage( CAlphaBitmap &abm ) const;
	string						MakeLODsTrisString() const;

	ULONG							m_ref;
	string						m_strFilename;
	string						m_strExtension;
	string						m_strRelativePath;
	bool							m_bUsedInLevel;
	std::vector<string> m_dependencies;
	unsigned __int64	m_nFileSize;
	CRect							m_oDrawingRectangle;
	IAssetDisplayDatabase*	m_piOwnerDatabase;
	Vec3							m_camTarget;
	float							m_camRadius, m_camZoom;
	CCamera						m_camera;
	AABB							m_aabb;
	Quat							m_crtRotation;
	UINT							m_lodCount, m_triangleCount, m_vertexCount, m_submeshCount;
	UINT							m_textureSize, m_nRef, m_physicsTriCount, m_physicsSize;
	bool							m_bSplitLODs;
	UINT							m_triCountLOD[MAX_STATOBJ_LODS_NUM];
	IStatObj					*m_pObject;
	IRenderer					*m_pRenderer;
	bool							m_bGrid;
	bool							m_bAxis;
	std::vector<CDLight> m_lights;
	ColorF						m_clearColor;
	bool							m_bRotate;
	float							m_rotateAngle;
	bool							m_bShowObject;
	float							m_fov, m_rotationX, m_rotationY, m_translateX, m_translateY;
	volatile UINT			m_flags;
	CAlphaBitmap			m_oCachedBmp;
	static UINT				m_frameBufferScreenshot[kAssetDisplay_MaxThumbImageBufferSize];
	UINT							m_assetIndex;
	string						m_errorText;
};

#endif // __AssetModelItem_H__
