#include "stdafx.h"
#include "AnimationImageManager_2.h"
#include "smartptr.h"
#include "AnimationGraph_2.h"

CAnimationImage2::CAnimationImage2(CAnimationImageManager2* pManager, const string& animationName, float fSnapshotTime)
:	m_pManager(pManager),
	m_pBitmap(0),
	m_animationName(animationName),
	m_fSnapshotTime(fSnapshotTime),
	m_imageValid(false)
{
}

CAnimationImage2::~CAnimationImage2()
{
	if (m_pBitmap)
		delete m_pBitmap;
}

Gdiplus::Image* CAnimationImage2::GetImage()
{
	if (!m_imageValid)
	{
		m_pBitmap = m_pManager->CreateImage(m_animationName, m_fSnapshotTime);
		m_imageValid = true;
	}

	Gdiplus::Image* pImage = 0;
	if (m_pBitmap)
		pImage = m_pBitmap;
	if (pImage == 0)
		pImage = m_pManager->GetDefaultImage();
	return pImage;
}

void CAnimationImage2::Invalidate()
{
	if (m_pBitmap)
	{
		delete m_pBitmap;
		m_pBitmap = 0;
	}
	m_imageValid = false;
}

CAnimationImageManager2::CAnimationImageManager2(CWnd* pParentWindow, const string& modelAssetName, const ImageSize& imageSize)
:	m_pDefaultImage(0),
	m_imageSize(imageSize),
	m_imageGenerator(pParentWindow, modelAssetName)
{
}

CAnimationImageManager2::~CAnimationImageManager2()
{
	if (m_pDefaultImage)
		delete m_pDefaultImage;
}

CAnimationImage2Ptr CAnimationImageManager2::GetImage(CAnimationGraph2* pGraph, CAGState2* pState)
{
	ImageMap::iterator itImage = m_imageMap.find(pState);
	if (itImage == m_imageMap.end())
	{
		TParameterizationId2 paramId;
		if (pState->IsParameterized())
		{
			const CParamsDeclaration2* pParams = pState->GetParamsDeclaration();
			CParamsDeclaration2::const_iterator itParams, itParamsEnd = pParams->end();
			for ( itParams = pParams->begin(); itParams != itParamsEnd; ++itParams )
				if ( !itParams->second.empty() )
					paramId[ itParams->first ] = *itParams->second.begin();
		}
		string animName = "";
		string name = pState->GetUnparamedPerParamDetails()->GetAnimationName();

		// TODO: Replace the parameters in that name so this works for parameterized states too
		// CAGState should get a function that does this
		//		--> See AnimationGraphPreviewManager SetParameter and ReplaceParameters for an example

		if (m_imageGenerator.IsAnimationValid(name))
		{
			animName = name;
		}

		if (!animName.empty())
		{
			CAnimationImage2* pNewImage = new CAnimationImage2(this, animName, pState->GetIconSnapshotTime());
			itImage = m_imageMap.insert(std::make_pair(pState, pNewImage)).first;
		}
	}

	if (itImage != m_imageMap.end())
		return (*itImage).second;
	else
		return CAnimationImage2Ptr(0);
}

void CAnimationImageManager2::InvalidateImage(CAGState2* pState)
{
	ImageMap::iterator itImage = m_imageMap.find(pState);
	if (itImage != m_imageMap.end())
	{
		CAnimationImage2Ptr& pImage = (*itImage).second;
		pImage->m_fSnapshotTime = pState->GetIconSnapshotTime();
		pImage->Invalidate();
	}
}

Gdiplus::Image* CAnimationImageManager2::GetDefaultImage()
{
	if (!m_pDefaultImage)
		m_pDefaultImage = CreateBitmap();
	return m_pDefaultImage;
}

void CAnimationImageManager2::SetImageSize(const ImageSize& size)
{
	m_imageSize = size;
	InvalidateAllImages();
}

void CAnimationImageManager2::InvalidateAllImages()
{
	// Invalidate every icon.
	for (ImageMap::iterator itImageEntry = m_imageMap.begin(); itImageEntry != m_imageMap.end(); ++itImageEntry)
	{
		CAnimationImage2Ptr pImage = (*itImageEntry).second;
		pImage->Invalidate();
	}
}

Gdiplus::Bitmap* CAnimationImageManager2::CreateBitmap()
{
	Gdiplus::Bitmap* pBitmap = new Gdiplus::Bitmap(this->m_imageSize.first, this->m_imageSize.second, PixelFormat32bppARGB);
	Gdiplus::BitmapData bitmapData;

	pBitmap->LockBits(
		0,
		Gdiplus::ImageLockModeWrite,
		PixelFormat32bppARGB,
		&bitmapData);

	UINT* pixels = (UINT*)bitmapData.Scan0;

	for(UINT row = 0; row < bitmapData.Height; ++row)
	{
		for(UINT col = 0; col < bitmapData.Width; ++col)
			pixels[row * bitmapData.Stride / 4 + col] = 0xFF000000;
	}

	pBitmap->UnlockBits(&bitmapData);

	return pBitmap;
}

Gdiplus::Bitmap* CAnimationImageManager2::CreateImage(const string& animName, float snapshotTime)
{
	Gdiplus::Bitmap* pBitmap = CreateBitmap();
	m_imageGenerator.GetAnimationImage(pBitmap, animName, snapshotTime);
	return pBitmap;
}

void CAnimationImageManager2::ChangeModelAssetName(const char* modelAssetName)
{
	m_imageGenerator.ChangeModelAssetName( modelAssetName );
	InvalidateAllImages();
}
