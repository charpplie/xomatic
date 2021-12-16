////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:	AssetMaterialItem.cpp
//  Version:	v1.00
//  Created:	12/07/2010 by Nicusor Nedelcu
//  Description: Implementation of AssetMaterialItem.h
// -------------------------------------------------------------------------
//  History:
//		05/05/2010	20:12 : Nicusor Nedelcu - created
//		10/05/2010	13:58 : Nicusor Nedelcu - moved common code out of the class to AssetBrowserCommon.h/cpp
//
////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "AssetMaterialItem.h"
#include "Util/MemoryBlock.h"
#include "Util/Image.h"
#include "Util/ImageUtil.h"
#include "Util/PathUtil.h"
#include "Include/IAssetItemDatabase.h"
#include "IMaterial.h"
#include "IMusicSystem.h"
#include "IRenderer.h"
#include "Include/IAssetViewer.h"
#include "ImageExtensionHelper.h"
#include "Controls/PreviewModelCtrl.h"
#include "IEditor.h"
#include "IStreamEngine.h"
#include "Material/MaterialManager.h"
#include "Asset Browser/AssetBrowserDialog.h"

namespace AssetBrowser
{
const char* kMaterialImageFilename = "Editor/UI/Icons/asset_material.png";
const char* kMaterialPreviewBackTexture = "Editor/Materials/Stripes.dds";
const char* kMaterialPreviewModelFile = "Editor/Objects/MtlSphere.cgf";
};

CAlphaBitmap CAssetMaterialItem::s_uncachedMaterialThumbBmp;
CPreviewModelCtrl* CAssetMaterialItem::s_pPreviewCtrl = NULL;

CAssetMaterialItem::CAssetMaterialItem()
	: CAssetItem()
{
	m_flags = eFlag_UseGdiRendering;

	if (!s_uncachedMaterialThumbBmp.GetBitmap().GetSafeHandle())
	{
		s_uncachedMaterialThumbBmp.Load(AssetBrowser::kMaterialImageFilename, true);
	}

	m_pUncachedThumbBmp = &s_uncachedMaterialThumbBmp;
	m_pMaterial = NULL;

	if (!s_pPreviewCtrl)
	{
		s_pPreviewCtrl = new CPreviewModelCtrl();
	}
}

CAssetMaterialItem::~CAssetMaterialItem()
{
	// empty, call FreeData first
}

HRESULT STDMETHODCALLTYPE CAssetMaterialItem::QueryInterface(const IID& riid, void** ppvObj)
{
	if (riid == __uuidof(IAssetItem))
	{
		*ppvObj = this;
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CAssetMaterialItem::AddRef()
{
	return ++m_ref;
};

ULONG STDMETHODCALLTYPE CAssetMaterialItem::Release()
{
	if ((--m_ref) == 0)
	{
		FreeData();
		delete this;
		return 0;
	}
	else
	{
		return m_ref;
	}
}

bool CAssetMaterialItem::GetAssetFieldValue(const char* pFieldName, void* pDest)
{
	if (AssetViewer::IsFieldName(pFieldName, "thumbToolTipText"))
	{
		stack_string str;

		CString tagsText;
		CAssetItem::GetAssetFieldValue("tags", &tagsText);

		str.Format("Path: %s\nTags: %s\n",
							m_strRelativePath.GetBuffer(),
							tagsText.GetBuffer());
		*(CString*)pDest = str.c_str();
		return true;
	}
	else if (AssetViewer::IsFieldName(pFieldName, "thumbOneLineText"))
	{
		stack_string str;

		//TODO: pre-bake in the caching code, as a member
		str.Format("");
		*(CString*)pDest = str.c_str();
		return true;
	}
	else if (AssetViewer::IsFieldName(pFieldName, "errors"))
	{
		CString str;

		*(CString*)pDest = str.GetBuffer();
		return true;
	}

	// else, check if the common fields are requested
	return CAssetItem::GetAssetFieldValue(pFieldName, pDest);
}

void CAssetMaterialItem::LoadMaterial()
{
	if (!m_pMaterial)
	{
		CString fullPath = m_strRelativePath + m_strFilename;
		m_pMaterial = GetIEditor()->GetMaterialManager()->LoadMaterial(fullPath, false);
	}
}

bool CAssetMaterialItem::Cache()
{
	if (IsFlagSet(eFlag_Cached))
	{
		return true;
	}
	
	LoadMaterial();

	Render(
		CAssetBrowserDialog::Instance()->GetAssetViewer().GetRenderWindow(),
		CRect(0, 0, gSettings.sAssetBrowserSettings.nThumbSize, gSettings.sAssetBrowserSettings.nThumbSize),
		true);
	CAssetItem::Cache();
	m_cachedThumbBmp.Free();

	SetFlag(eFlag_Cached, true);
	GetOwnerDatabase()->OnMetaDataChange(this);

	if (m_pMaterial)
	{
		m_pMaterial = 0;
	}

	return true;
}

void CAssetMaterialItem::OnBeginPreview(const HWND hQuickPreviewWnd, const HDC hMemDC)
{
	LoadMaterial();

	if (!s_pPreviewCtrl->GetSafeHwnd())
	{
		RECT rc;
		CWnd* pWnd = CWnd::FromHandle(hQuickPreviewWnd);

		GetClientRect(hQuickPreviewWnd, &rc);
		s_pPreviewCtrl->Create(pWnd, rc, WS_CHILD|WS_VISIBLE);
		s_pPreviewCtrl->SetGrid(false);
		s_pPreviewCtrl->SetAxis(false);
		s_pPreviewCtrl->SetBackgroundTexture(AssetBrowser::kMaterialPreviewBackTexture);
		s_pPreviewCtrl->SetClearColor(ColorF(0,0,0));
		s_pPreviewCtrl->LoadFile(AssetBrowser::kMaterialPreviewModelFile);
	}

	s_pPreviewCtrl->SetMaterial(m_pMaterial);
}

void CAssetMaterialItem::OnEndPreview()
{
	m_hPreviewDC = 0;
	s_pPreviewCtrl->DestroyWindow();
}

void CAssetMaterialItem::PreviewRender(
	HWND hRenderWindow,
	const CRect& rstViewport,
	int aMouseX, int aMouseY,
	int aMouseDeltaX, int aMouseDeltaY,
	UINT aKeyFlags)
{
}

bool CAssetMaterialItem::Render(const HWND hRenderWindow, const CRect& rstViewport, bool bCacheThumbnail)
{
	if (!m_pMaterial)
	{
		return false;
	}

	CWnd* pWnd = CWnd::FromHandle(hRenderWindow);

	if (!s_pPreviewCtrl->GetSafeHwnd())
	{
		s_pPreviewCtrl->Create(pWnd, rstViewport, WS_CHILD|WS_VISIBLE);
		s_pPreviewCtrl->SetGrid(false);
		s_pPreviewCtrl->SetAxis(false);
		s_pPreviewCtrl->SetBackgroundTexture(AssetBrowser::kMaterialPreviewBackTexture);
		s_pPreviewCtrl->SetClearColor(ColorF(0,0,0));
		s_pPreviewCtrl->LoadFile(AssetBrowser::kMaterialPreviewModelFile);
	}
	else
	{
		s_pPreviewCtrl->SetParent(pWnd);
	}
	
	s_pPreviewCtrl->FitToScreen();
	s_pPreviewCtrl->SetMaterial(m_pMaterial);
	
	if (s_pPreviewCtrl)
	{
		s_pPreviewCtrl->MoveWindow(rstViewport);
	}

	if (bCacheThumbnail)
	{
		CacheThumbnail(hRenderWindow, rstViewport);
	}

	return true;
}

void CAssetMaterialItem::CacheThumbnail(const HWND hRenderWindow, const CRect& rc)
{
	if (s_pPreviewCtrl)
	{
		s_pPreviewCtrl->ShowWindow(SW_SHOW);
		s_pPreviewCtrl->Update();
		s_pPreviewCtrl->RedrawWindow();
		s_pPreviewCtrl->ShowWindow(SW_HIDE);
		s_pPreviewCtrl->FitToScreen();
		CImageEx img;
	
		s_pPreviewCtrl->GetImageOffscreen(img, 
			CSize(gSettings.sAssetBrowserSettings.nThumbSize,
			gSettings.sAssetBrowserSettings.nThumbSize));
	
		m_cachedThumbBmp.Create(img.GetData(),
			img.GetWidth(),
			img.GetHeight());

		img.Release();
	
		m_flags |= eFlag_ThumbnailLoaded;
	}
}

void CAssetMaterialItem::OnPreviewRenderKeyEvent(bool bKeyDown, UINT aChar, UINT aKeyFlags)
{
}

void CAssetMaterialItem::DrawTextOnReportImage(CAlphaBitmap& rDestDmp) const
{
}

bool CAssetMaterialItem::SaveReportImage(const char* pFilePath) const
{
	return false;
}

bool CAssetMaterialItem::SaveReportText(const char* pFilePath) const
{
	return false;
}

void CAssetMaterialItem::ToXML(XmlNodeRef& node) const
{
	node->setTag("Material");
	CString fileName = m_strRelativePath + m_strFilename;
	node->setAttr("fileName", fileName.GetBuffer());
	node->setAttr("filesize", m_nFileSize);
	node->setAttr("dccFilename", m_strDccFilename);
}

void CAssetMaterialItem::FromXML(const XmlNodeRef& node)
{
	assert(node->isTag("Material"));

	if (node->isTag("Material") == false)
	{
		return;
	}

	node->getAttr("filesize", m_nFileSize);
	const char* dccstr = NULL;
	node->getAttr("dccFilename", &dccstr);

	if (dccstr)
	{
		m_strDccFilename = dccstr;
	}

	SetFlag(eFlag_Cached);
}