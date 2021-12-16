////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2005.
// -------------------------------------------------------------------------
//  File name:   PhonemeAnalyzer.h
//  Version:     v1.00
//  Created:     11/1/2006 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "PhonemeAnalyzer.h"

typedef ILipSyncPhonemeRecognizer* (*CreatePhonemeParserFunc)();

//////////////////////////////////////////////////////////////////////////
CPhonemesAnalyzer::CPhonemesAnalyzer()
{
	m_hDLL = 0;
	m_pPhonemeParser = 0;
}

//////////////////////////////////////////////////////////////////////////
CPhonemesAnalyzer::~CPhonemesAnalyzer()
{
	if (m_pPhonemeParser)
		m_pPhonemeParser->Release();
	if (m_hDLL)
		FreeLibrary(m_hDLL);
}

//////////////////////////////////////////////////////////////////////////
bool CPhonemesAnalyzer::Analyze(  const char *wavfile,const char *text,ILipSyncPhonemeRecognizer::SSentance** pOutSetence )
{
	if (m_pPhonemeParser)
		m_pPhonemeParser->Release();

	CString strText = text;
	strText.Replace( '\r',' ' );
	strText.Replace( '\n',' ' );
	strText.Replace( '\t',' ' );

	bool bRes = false;
	WCHAR szCurDir[1024];
	GetCurrentDirectoryW(sizeof(szCurDir),szCurDir);
	SetCurrentDirectoryW( L"Editor\\Plugins\\LipSync\\Annosoft" );
	m_hDLL = LoadLibrary( "LipSync_Annosoft.dll" );
	if (m_hDLL)
	{
		CreatePhonemeParserFunc func = (CreatePhonemeParserFunc)::GetProcAddress(m_hDLL,"CreatePhonemeParser");
		if (func)
		{
			m_pPhonemeParser = func();

			// Find the full path.
			CStringW currentDirectory = szCurDir;
			if (!(currentDirectory.GetLength() > 0 && (currentDirectory[currentDirectory.GetLength() - 1] == '\\' || currentDirectory[currentDirectory.GetLength() - 1] == '/')))
				currentDirectory.Append(L"\\");
			CStringW fullPath = currentDirectory + wavfile;
			char fullPathASCIIBuffer[2048] = "";
			WideCharToMultiByte(CP_ACP, 0, fullPath.GetString(), fullPath.GetLength(), fullPathASCIIBuffer, sizeof(fullPathASCIIBuffer) / sizeof(fullPathASCIIBuffer[0]), 0, 0);

			bRes = m_pPhonemeParser->RecognizePhonemes( fullPathASCIIBuffer,strText,pOutSetence );
			m_LastError = m_pPhonemeParser->GetLastError();
		}
	}
	// Restore current directory.
	SetCurrentDirectoryW( szCurDir );
	return bRes;
}

//////////////////////////////////////////////////////////////////////////
const char* CPhonemesAnalyzer::GetLastError()
{
	return m_LastError;
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
/*
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

*/
