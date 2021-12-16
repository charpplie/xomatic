#pragma once
////////////////////////////////////////////////////////////////////////////
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2013
////////////////////////////////////////////////////////////////////////////
#include "Asset Browser/AssetBrowserCommon.h"
#include "AssetBrowserPreviewCharacterDlg.h"
#include "AssetBrowserPreviewCharacterDlgFooter.h"
#include <I3DEngine.h>
#include <IRenderAuxGeom.h>
#include <IShader.h>
#include "Util/GdiUtil.h"

namespace AssetBrowser
{
const float kDefaultCharacterRotationAngleX = 10.0f;
const float kDefaultCharacterRotationAngleY = 70.0f;
const float kCharacterAssetAmbienceMultiplier = 7.0f;
};

class CPreviewModelCtrl;

class CAssetCharacterItem : public CAssetItem
{
public:

	friend class CAssetBrowserPreviewCharacterDlg;
	friend class CAssetBrowserPreviewCharacterDlgFooter;

	static const int	kAssetDisplay_MaxThumbImageBufferSize = 2048 * 2048;

	enum EAssetModelItemDragCreationMode
	{
		eAssetCharacterItemDragCreationMode_AsBrush,
		eAssetCharacterItemDragCreationMode_AsGeomEntity
	};

	CAssetCharacterItem();
	~CAssetCharacterItem();

	bool GetAssetFieldValue(const char* pFieldName, void* pDest);
	bool Cache();
	void CacheCurrentThumbAngle();
	void GatherDependenciesInfo();
	void PrepareThumbTextInfo();
	void OnBeginPreview(const HWND hQuickPreviewWnd, const HDC hMemDC);
	void OnEndPreview();
	CDialog* GetCustomPreviewPanelHeader(CWnd* pParentWnd);
	CDialog* GetCustomPreviewPanelFooter(CWnd* pParentWnd);
	void PreviewRender(const HWND hRenderWindow, const CRect& rstViewport, int aMouseX, int aMouseY, int aMouseDeltaX, int aMouseDeltaY, int aMouseWheelDelta, UINT aKeyFlags);
	void OnPreviewRenderKeyEvent(bool bKeyDown, UINT aChar, UINT aKeyFlags);
	bool Render(const HWND hRenderWindow, const CRect& rstViewport, bool bCacheThumbnail = false);
	void* CreateInstanceInViewport(float aX, float aY, float aZ);
	bool MoveInstanceInViewport(const void* pDraggedObject, float aX, float aY, float aZ);
	void AbortCreateInstanceInViewport(const void* pDraggedObject);
	bool SaveReportImage(const char* filePath) const;
	bool SaveReportText(const char* filePath) const;
	void CacheFieldsInfoForLoadedStatObj(IStatObj* pStatObj);
	void CheckIfItsLod();
	void ToXML(XmlNodeRef& node) const;
	void FromXML(const XmlNodeRef& node);
	void ResetView();
	static void SetAssetCharacterPreviewCtrl(CPreviewModelCtrl* pView);
	static CPreviewModelCtrl* GetAssetCharacterPreviewCtrl();
	bool LoadModel();

	// from IUnknown - Inherited through IClassDesc.
	HRESULT STDMETHODCALLTYPE QueryInterface(const IID& riid, void** ppvObj);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

protected:
	void SetCamera(CCamera& cam, const CRect& rcViewportRect);
	void CalculateCameraPosition();
	void DrawTextOnReportImage(CAlphaBitmap& abm) const;
	void MakeLODsTrisString(CString& rOutValue) const;
	void MakeLODsVertsString(CString& rOutValue) const;
	void MakeLODsSubMeshesString(CString& rOutValue) const;
	void MakeLODsMeshSizeString(CString& rOutValue) const;
	void CacheThumbnail(const HWND hRenderWindow, const CRect& rc);
	void CreateViewport(const HWND hRenderWindow, const CRect& rc, bool bThumbnail = false);

	Vec3 m_camTarget;
	float m_camRadius, m_camZoom;
	CCamera m_camera;
	AABB m_aabb;
	Quat m_crtRotation;
	UINT m_lodCount, m_triangleCount, m_vertexCount, m_submeshCount, m_mtlCount;
	UINT m_textureSize, m_nRef, m_physicsTriCount, m_physicsSize;
	UINT m_triCountLOD[MAX_STATOBJ_LODS_NUM];
	UINT m_vertCountLOD[MAX_STATOBJ_LODS_NUM];
	UINT m_subMeshCountLOD[MAX_STATOBJ_LODS_NUM];
	UINT m_meshSizeLOD[MAX_STATOBJ_LODS_NUM];
	IStatObj* m_pObject;
	IRenderer* m_pRenderer;
	ColorF m_clearColor;
	bool m_bRotate;
	bool m_bIsLod;
	bool m_bGrid;
	bool m_bAxis;
	bool m_bShowObject;
	bool m_bSplitLODs;
	bool m_bCameraUpdate;
	bool m_bCacheCurrentThumbAngle;
	float m_rotateAngle;
	float m_fov, m_rotationX, m_rotationY, m_translateX, m_translateY;
	EAssetModelItemDragCreationMode m_dragCreationMode;

	static CAlphaBitmap s_uncachedModelThumbBmp;
	static bool s_bWireframe;
	static bool s_bPhysics;
	static bool s_bNormals;
	static float s_fAmbience;
	static UINT s_frameBufferScreenshot[kAssetDisplay_MaxThumbImageBufferSize];
	static CAssetBrowserPreviewCharacterDlg s_modelPreviewDlgHeader;
	static CAssetBrowserPreviewCharacterDlgFooter s_modelPreviewDlgFooter;
	static CPreviewModelCtrl* s_pPreviewCtrl;
};