#ifndef __ANIMATIONIMAGEGENERATOR2_H__
#define __ANIMATIONIMAGEGENERATOR2_H__

#include "..\Controls\PreviewModelCtrl.h"

class CAnimationImageGenerator2
{
public:
	CAnimationImageGenerator2(CWnd* pParentWindow, const string& modelAssetName);
	void GetAnimationImage(Gdiplus::Bitmap* pBitmap, const string& animationName, float snapshotTime);
	bool IsAnimationValid(const string& animationName);

	void ChangeModelAssetName(const char* modelAssetName);

private:
	void PrepareCharacterForGrab(const string& animationName, float snapshotTime);

	string m_modelAssetName;
	CPreviewModelCtrl m_renderCtrl;
};

#endif //__ANIMATIONIMAGEGENERATOR_H__
