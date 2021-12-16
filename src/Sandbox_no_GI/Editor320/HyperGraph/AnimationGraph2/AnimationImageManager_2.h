#ifndef __ANIMATIONIMAGEMANAGER2_H__
#define __ANIMATIONIMAGEMANAGER2_H__

#include "AnimationImageGenerator_2.h"

class CAGState2;
class CAnimationGraph2;
class CAnimationImage2 : public _reference_target_t
{
	friend class CAnimationImageManager2;
public:
	~CAnimationImage2();

	Gdiplus::Image* GetImage();

private:
	CAnimationImage2(CAnimationImageManager2* pManager, const string& animationName, float fSnapshotTime);

	void Invalidate();

	CAnimationImageManager2* m_pManager;
	Gdiplus::Bitmap* m_pBitmap;
	string m_animationName;
	float m_fSnapshotTime;
	bool m_imageValid;
};
typedef _smart_ptr<CAnimationImage2> CAnimationImage2Ptr;

class CAnimationImageManager2 : public _reference_target_t
{
	friend class CAnimationImage2;
public:
	typedef std::pair<int, int> ImageSize;

	CAnimationImageManager2(CWnd* pParentWindow, const string& modelAssetName, const ImageSize& imageSize);
	~CAnimationImageManager2();

	CAnimationImage2Ptr GetImage(CAnimationGraph2* pGraph, CAGState2* pState);
	void InvalidateImage(CAGState2* pState);
	void InvalidateAllImages();
	Gdiplus::Image* GetDefaultImage();
	const ImageSize& GetImageSize() {return m_imageSize;}
	void SetImageSize(const ImageSize& size);
	void ChangeModelAssetName(const char* modelAssetName);

private:
	Gdiplus::Bitmap* CreateBitmap();
	Gdiplus::Bitmap* CreateImage(const string& animName, float snapshotTime);

	Gdiplus::Image* m_pDefaultImage;
	typedef std::map<CAGState2*, CAnimationImage2Ptr> ImageMap;
	ImageMap m_imageMap;
	ImageSize m_imageSize;
	CAnimationImageGenerator2 m_imageGenerator;
};

typedef _smart_ptr<CAnimationImageManager2> CAnimationImageManager2Ptr;

#endif //__ANIMATIONIMAGEMANAGER_H__
