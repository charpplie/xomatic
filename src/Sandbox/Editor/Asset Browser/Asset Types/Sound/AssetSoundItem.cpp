////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:	AssetSoundItem.cpp
//  Version:	v1.00
//  Created:	12/07/2010 by Nicusor Nedelcu
//  Description: Implementation of AssetSoundItem.h
// -------------------------------------------------------------------------
//  History:
//		05/05/2010	20:12 : Nicusor Nedelcu - created
//		10/05/2010	13:58 : Nicusor Nedelcu - moved common code out of the class to AssetBrowserCommon.h/cpp
//
////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "AssetSoundItem.h"
#include "Util/MemoryBlock.h"
#include "Util/Image.h"
#include "Util/ImageUtil.h"
#include "Util/PathUtil.h"
#include "Include/IAssetItemDatabase.h"
#include "IMusicSystem.h"
#include "IRenderer.h"
#include "Include/IAssetViewer.h"
#include "ImageExtensionHelper.h"
#include "Objects/EntityObject.h"

namespace AssetBrowser
{
const char* kSoundImageFilename = "Editor/UI/Icons/asset_sound.png";
};

CAlphaBitmap CAssetSoundItem::s_uncachedSoundThumbBmp;

CAssetSoundItem::CAssetSoundItem()
	: CAssetItem()
{
	REINST("probably very obsolete!");
	//m_soundId = 0;
	m_nSoundLengthMsec = 0;
	m_bLoopingSound = false;
	m_flags = eFlag_UseGdiRendering;
	m_flags |= eFlag_CanBeDraggedInViewports | eFlag_CanBeMovedAfterDroppedIntoViewport;

	if (!s_uncachedSoundThumbBmp.GetBitmap().GetSafeHandle())
	{
		s_uncachedSoundThumbBmp.Load(AssetBrowser::kSoundImageFilename, true);
	}

	m_pUncachedThumbBmp = &s_uncachedSoundThumbBmp;
}

CAssetSoundItem::~CAssetSoundItem()
{
	// empty, call FreeData first
}

HRESULT STDMETHODCALLTYPE CAssetSoundItem::QueryInterface(const IID& riid, void** ppvObj)
{
	if (riid == __uuidof(IAssetItem))
	{
		*ppvObj = this;
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CAssetSoundItem::AddRef()
{
	return ++m_ref;
};

ULONG STDMETHODCALLTYPE CAssetSoundItem::Release()
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

bool CAssetSoundItem::GetAssetFieldValue(const char* pFieldName, void* pDest)
{
	if (AssetViewer::IsFieldName(pFieldName, "thumbToolTipText"))
	{
		stack_string str;

		CString tagsText;
		CAssetItem::GetAssetFieldValue("tags", &tagsText);

		str.Format("Path: %s\nLength(ms): %d\nLooping: %s\nTags: %s\n",
							m_strRelativePath.GetBuffer(),
							m_nSoundLengthMsec,
							(m_bLoopingSound ? "Yes" : "No"),
							tagsText.GetBuffer());
		*(CString*)pDest = str.c_str();
		return true;
	}
	else if (AssetViewer::IsFieldName(pFieldName, "thumbOneLineText") && m_nSoundLengthMsec)
	{
		stack_string str;

		//TODO: pre-bake in the caching code, as a member
		str.Format("[ Length: %d ms ]", m_nSoundLengthMsec);
		*(CString*)pDest = str.c_str();
		return true;
	}
	else if (AssetViewer::IsFieldName(pFieldName, "errors"))
	{
		CString str;

		//TODO: pre-bake in the caching code, as a member
		if (m_nSoundLengthMsec <= 1)
		{
			str += "WARNING: Sound length (msec) is zero\n";
		}

		*(CString*)pDest = str.GetBuffer();
		return true;
	}
	else if (AssetViewer::IsFieldName(pFieldName, "length"))
	{
		*(int*)pDest = m_nSoundLengthMsec;
		return true;
	}
	else if (AssetViewer::IsFieldName(pFieldName, "loopsound"))
	{
		*(bool*)pDest = m_bLoopingSound;
		return true;
	}

	// else, check if the common fields are requested
	return CAssetItem::GetAssetFieldValue(pFieldName, pDest);
}

void CAssetSoundItem::LoadSound()
{
	/*ISystem* pSystem = GetISystem();
	ISoundSystem* pSoundSystem = pSystem->GetISoundSystem();
	CString soundname = m_strRelativePath + m_strFilename;

	{
		ISound* pSound = pSoundSystem->CreateSound(soundname.GetBuffer(), FLAG_SOUND_LOAD_SYNCHRONOUSLY|FLAG_SOUND_2D);
		m_soundId = 0;

		if (pSound)
		{
			m_nSoundLengthMsec = pSound->GetLengthMs();
			m_bLoopingSound = pSound->GetFlags() & FLAG_SOUND_LOOP;
			m_soundId = pSound->GetId();
		}
	}*/
}

bool CAssetSoundItem::Cache()
{
	if (m_flags & eFlag_Cached)
	{
		return true;
	}

	LoadSound();
	ISystem* pSystem = GetISystem();
	/*ISoundSystem* pSoundSystem = pSystem->GetIAudioSystem();
	_smart_ptr<ISound> pSnd = pSoundSystem->GetSound(m_soundId);

	if (pSnd)
	{
		pSnd->Stop(ESoundStopMode_AtOnce);
	}

	pSoundSystem->Update(eSoundUpdateMode_All);*/
	SetFlag(eFlag_Cached, true);
	GetOwnerDatabase()->OnMetaDataChange(this);

	if (0 == m_nSoundLengthMsec)
	{
		SetFlag(eFlag_Invalid, true);
	}

	return true;
}

void CAssetSoundItem::OnBeginPreview(const HWND hQuickPreviewWnd, const HDC hMemDC)
{
	LoadSound();
	/*ISound* pSound = GetISystem()->GetIAudioSystem()->GetSound(m_soundId);

	if (pSound)
	{
		pSound->GetInterfaceExtended()->SetFlags(pSound->GetFlags() & ~FLAG_SOUND_RADIUS);
		pSound->SetSemantic(eSoundSemantic_Sandbox_Browser);
		pSound->Play();
	}*/
}

void CAssetSoundItem::OnEndPreview()
{
	/*{
		_smart_ptr<ISound> pSound = GetISystem()->GetISoundSystem()->GetSound(m_soundId);

		if (pSound)
		{
			pSound->Stop(ESoundStopMode_AtOnce);
		}
	}

	GetISystem()->GetISoundSystem()->Update(eSoundUpdateMode_All);
	m_soundId = 0;
	m_hPreviewDC = 0;*/
}

void CAssetSoundItem::PreviewRender(
	HWND hRenderWindow,
	const CRect& rstViewport,
	int aMouseX, int aMouseY,
	int aMouseDeltaX, int aMouseDeltaY,
	UINT aKeyFlags)
{
}

void* CAssetSoundItem::CreateInstanceInViewport(float aX, float aY, float aZ)
{
	CBaseObject* pObj = GetIEditor()->NewObject("Entity", "SoundEventSpot");
	
	if (!pObj)
	{
		return NULL;
	}

	CString fullFilename = CString(GetRelativePath()) + GetFilename();
	CEntityObject* pObjEnt = (CEntityObject*)pObj;

	pObjEnt->SetEntityPropertyString("soundName", fullFilename);

	GetIEditor()->GetObjectManager()->BeginEditParams(pObj, OBJECT_CREATE);
	GetIEditor()->SetModifiedFlag();
	GetIEditor()->SetModifiedModule(eModifiedEntities);
	pObj->SetPos(Vec3(aX, aY, aZ));
	GetIEditor()->GetObjectManager()->EndEditParams();

	return pObj;
}

bool CAssetSoundItem::MoveInstanceInViewport(const void* pDraggedObject, float aX, float aY, float aZ)
{
	CBaseObject* pObj = (CBaseObject*)pDraggedObject;

	if (pObj)
	{
		pObj->SetPos(Vec3(aX, aY, aZ));
		return true;
	}

	return false;
}

void CAssetSoundItem::AbortCreateInstanceInViewport(const void* pDraggedObject)
{
	CBaseObject* pObj = (CBaseObject*)pDraggedObject;

	if (pObj)
	{
		GetIEditor()->GetObjectManager()->DeleteObject(pObj);
	}
}

void CAssetSoundItem::OnPreviewRenderKeyEvent(bool bKeyDown, UINT aChar, UINT aKeyFlags)
{
}

void CAssetSoundItem::DrawTextOnReportImage(CAlphaBitmap& rDestDmp) const
{
}

bool CAssetSoundItem::SaveReportImage(const char* pFilePath) const
{
	return false;
}

bool CAssetSoundItem::SaveReportText(const char* pFilePath) const
{
	return false;
}

void CAssetSoundItem::ToXML(XmlNodeRef& node) const
{
	node->setTag("Sound");
	CString fileName = m_strRelativePath + m_strFilename;
	node->setAttr("fileName", fileName.GetBuffer());
	node->setAttr("filesize", m_nFileSize);
	node->setAttr("dccFilename", m_strDccFilename);
	node->setAttr("length", m_nSoundLengthMsec);
	node->setAttr("loopsound", m_bLoopingSound);
	// we give the timestamp, since the sound is inside fmod projects, not a real file
	node->setAttr("timeStamp", "1");
}

void CAssetSoundItem::FromXML(const XmlNodeRef& node)
{
	assert(node->isTag("Sound"));

	if (node->isTag("Sound") == false)
	{
		return;
	}

	node->getAttr("filesize", m_nFileSize);
	node->getAttr("length", m_nSoundLengthMsec);
	node->getAttr("loopsound", m_bLoopingSound);
	const char* dccstr = NULL;
	node->getAttr("dccFilename", &dccstr);

	if (dccstr)
	{
		m_strDccFilename = dccstr;
	}

	SetFlag(eFlag_Cached);
}