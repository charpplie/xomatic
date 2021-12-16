#include "StdAfx.h"
#include "tools.h"
#include "IConfig.h"									// IConfig
#include "ICfgFile.h"									// ICfgFile
#include <assert.h>										// assert()

void CenterWindow( HWND hwndChild, HWND hwndParent ) 
{
	RECT    rChild, rParent;     
	int     wChild, hChild, wParent, hParent;     
	int     wScreen, hScreen, xNew, yNew;     
	HDC     hdc;      

	// Get the Height and Width of the child window     
	GetWindowRect (hwndChild, &rChild);     
	wChild = rChild.right - rChild.left;     
	hChild = rChild.bottom - rChild.top;      

	// Get the display limits     
	hdc = GetDC (hwndChild);     
	wScreen = GetDeviceCaps (hdc, HORZRES);     
	hScreen = GetDeviceCaps (hdc, VERTRES);     
	ReleaseDC (hwndChild, hdc);      

	// Get the Height and Width of the parent window     
	if(hwndParent)			
	{
		GetWindowRect (hwndParent, &rParent);     
		wParent = rParent.right - rParent.left;     
		hParent = rParent.bottom - rParent.top;      
	}
	else // 0 means it's the screen
	{
		wParent = wScreen;     
		hParent = hScreen;
		rParent.left=0;
		rParent.top=0;
	}

	// Calculate new X position, then adjust for screen     
	xNew = rParent.left + ((wParent - wChild) /2);     

	if(xNew<0) xNew = 0;    
	else if((xNew+wChild)>wScreen)         
		xNew = wScreen - wChild;      

	// Calculate new Y position, then adjust for screen     
	yNew = rParent.top  + ((hParent - hChild) /2);    
	if(yNew<0) yNew = 0;     
	else if((yNew+hChild)>hScreen)	yNew = hScreen - hChild;      

	// Set it, and return     
	//	SetWindowPos(hwndChild, NULL, xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER); 
	MoveWindow(hwndChild, xNew, yNew, wChild, hChild, TRUE); 
} 



BOOL CALLBACK WndProcRedirect( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	HWND hParent=GetParent(hWnd);

	if(hParent)
		return SendMessage(hParent,uMsg,wParam,lParam)!=0?TRUE:FALSE;

	//	return DefWindowProc( hWnd, uMsg, wParam, lParam );
	return FALSE;
}




static void JumpOverWhitespace( const char * &p )
{
	while(*p!=0 && *p<=' ')
		p++;
}

bool IsValidNameChar( const unsigned char c )
{
	return c>' ' && c!='=' && c!=';' && c!=':' && c!='/';
}



void UpdateOrCreateEntryFromString( IConfig *pOutCfg, const string sInput )
{
	const char *p = sInput.c_str();

	while(*p)
	{
		string sKey,sValue;

		JumpOverWhitespace(p);

		if(*p != '/')									// /
		{
//			m_pCC->pLog->LogError("Special instructions in TIFF '%s' are broken (/ expected)",lpszPathName);
			assert(0);
			break;
		}
		p++;													// jump over /

		while(IsValidNameChar(*p))		// key
			sKey+=*p++;

		JumpOverWhitespace(p);

		if(*p != '=')									// =
		{
//			m_pCC->pLog->LogError("Special instructions in TIFF '%s' are broken (= expected)",lpszPathName);
			assert(0);
			break;
		}
		p++;													// jump over =

		JumpOverWhitespace(p);

		if(*p=='\"')									// " in quotes
		{
			++p;
			while(*p!=0 && *p!='\"')		// value
				sValue+=*p++;

			if(*p=='\"')								// "
				++p;
		}
		else													// without quotes
		{
			while(IsValidNameChar(*p))	// value
				sValue+=*p++;
		}

		JumpOverWhitespace(p);

		sKey.Trim();sValue.Trim();

//		OutputDebugString(sKey);OutputDebugString(" - ");
//		OutputDebugString(sValue);OutputDebugString("\n");

//		pOutCfg->CheckForUnknownKeys();

		pOutCfg->Set(eCP_PriorityFile,sKey,sValue);
	}
}