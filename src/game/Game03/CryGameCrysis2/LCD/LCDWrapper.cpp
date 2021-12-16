#include "StdAfx.h"
#ifdef USE_G15_LCD
#include "LCDWrapper.h"
#include "EZ_LCD.h"

CLCDWrapper::CLCDWrapper()
: m_pImpl(new CEzLcd())
, m_currentPage(0)
{
	m_pImpl->InitYourself(_T("Crysis"));
	m_pImpl->SetDeviceFamilyToUse(LGLCD_DEVICE_FAMILY_KEYBOARD_G15);
	m_pImpl->AddNewPage();

	m_pImpl->ModifyControlsOnPage(0);
	HANDLE titleScrolling_ = m_pImpl->AddText(LG_SCROLLING_TEXT, LG_BIG, DT_CENTER, 160);
	m_pImpl->SetOrigin(titleScrolling_, 0, 2);
	m_pImpl->SetText(titleScrolling_, _T("Crysis"));

	m_pImpl->ModifyControlsOnPage(1);
	HANDLE bitmapIndex = m_pImpl->AddBitmap();
	HBITMAP bitmap = (HBITMAP)LoadImage(0, "\\Textures\\LCD\\crysis.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	m_pImpl->SetBitmap(bitmapIndex, bitmap);
	m_pImpl->SetOrigin(bitmapIndex, 0,0);
	m_pImpl->SetVisible(bitmapIndex, true);
	m_time = 1.0f;
}

CLCDWrapper::~CLCDWrapper()
{
	SAFE_DELETE(m_pImpl);
}

bool CLCDWrapper::IsConnected()
{
	return m_pImpl->IsConnected()!=FALSE;
}

void CLCDWrapper::Update(float frameTime)
{
	FUNCTION_PROFILER(GetISystem(),PROFILE_GAME);
	m_time -= frameTime;
	if (m_time<0.0f)
	{
		m_time = 1.0f;
		m_currentPage = (m_currentPage+1)%m_pImpl->GetPageCount();
	}
	if (IsConnected())
	{
		m_pImpl->ShowPage(m_currentPage);
	}
	m_pImpl->Update();
	//if (m_pImpl->AnyDeviceOfThisFamilyPresent(LGLCD_DEVICE_FAMILY_KEYBOARD_G15) == TRUE)
	//{
	//	// Found the device. We will now run on this device automatically
	//	m_pImpl->SetDeviceFamilyToUse(LGLCD_DEVICE_FAMILY_KEYBOARD_G15);
	//}
}

void* CLCDWrapper::AddBitmap()
{
	return m_pImpl->AddBitmap();
}

bool CLCDWrapper::SetBitmap(void* handle, const char* name)
{ 
	HANDLE bitmap = LoadImage(0, name, IMAGE_BITMAP, 0, 0, LR_MONOCHROME);
	return SUCCEEDED(m_pImpl->SetBitmap(handle, (HBITMAP)bitmap));
}

#endif //USE_G15_LCD
