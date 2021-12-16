////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   AVI_Writer.cpp
//  Version:     v1.00
//  Created:     13/5/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "AVI_Writer.h"
#include "AVIGenerator.h"

#include "..\Settings.h"

//////////////////////////////////////////////////////////////////////////
CAVI_Writer::CAVI_Writer()
{
	m_pAVI = 0;
}

//////////////////////////////////////////////////////////////////////////
CAVI_Writer::~CAVI_Writer()
{
	CloseFile();
}

//////////////////////////////////////////////////////////////////////////
bool CAVI_Writer::OpenFile( const char *filename,int width,int height )
{
	m_pAVI = new CAVIGenerator;
	m_pAVI->SetFileName( filename );
	m_pAVI->SetRate(20);
	
	BITMAPINFOHEADER bmi;
	ZeroStruct(bmi);
	bmi.biSize = sizeof(bmi);
	bmi.biWidth = width;
	bmi.biHeight = height;
	bmi.biPlanes = 1;
	bmi.biBitCount = 32;
	bmi.biCompression = BI_RGB;
	bmi.biSizeImage = 4*width*height;
	m_pAVI->SetBitmapHeader( &bmi );
	m_pAVI->SetRate( gSettings.aviSettings.nFrameRate );
	if (FAILED(m_pAVI->InitEngine( gSettings.aviSettings.codec )))
	{
		Warning( "AVI Engine Initialization Failed (%s)",filename );
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CAVI_Writer::CloseFile()
{
	if (m_pAVI)
	{
		m_pAVI->ReleaseEngine();
		delete m_pAVI;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CAVI_Writer::AddFrame( CImage &image )
{
	if (m_pAVI)
	{
		if (!FAILED(m_pAVI->AddFrame( (BYTE*)image.GetData() )))
			return true;
	}
	return false;
}



    //BSTR __stdcall ConvertStringToBSTR(const char* pSrc) ;

    // Convert BSTR to char *
    //
    //char* __stdcall ConvertBSTRToString(BSTR pSrc) ;

    //BSTR __stdcall ConvertStringToBSTR(const char* pSrc) ;

    // Convert BSTR to char *
    //
    //char* __stdcall ConvertBSTRToString(BSTR pSrc) ;

// ConvertBSTRToString.cpp
#include <comutil.h>
#include <stdio.h>

#pragma comment(lib, "comsuppw.lib")
#pragma comment(lib, "kernel32.lib")

int main() 
{


  char* lpszText = "Test";
   printf_s("char * text: %s\n", lpszText);

   BSTR bstrText = _com_util::ConvertStringToBSTR(lpszText);
   wprintf_s(L"BSTR text: %s\n", bstrText);

   SysFreeString(bstrText);



   //BSTR bstrText = ::SysAllocString(L"Test");
   wprintf_s(L"BSTR text: %s\n", bstrText);

   char* lpszText2 = _com_util::ConvertBSTRToString(bstrText);
   printf_s("char * text: %s\n", lpszText2);

   SysFreeString(bstrText);
   delete[] lpszText2;
}