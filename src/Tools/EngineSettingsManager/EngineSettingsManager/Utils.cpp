#include "StdAfx.h"
#include "Utils.h"


//////////////////////////////////////////////////////////////////////////
BOOL CUtils::BrowseForFolder(HWND hWnd, LPCTSTR szInitialPath, LPTSTR szPath, LPCTSTR szTitle)
{
	TCHAR szDisplay[MAX_PATH];

	CoInitialize(NULL);

	BROWSEINFO bi = { 0 };
	bi.hwndOwner = hWnd;
	bi.pszDisplayName = szDisplay;
	bi.lpszTitle = szTitle;
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	bi.lpfn = NULL;
	bi.lParam = (LPARAM)szInitialPath;

	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

	if (pidl != NULL)
	{
		BOOL retval = SHGetPathFromIDList(pidl, szPath);
		CoTaskMemFree(pidl);
		CoUninitialize();
		return TRUE;
	}

	szPath[0] = 0;
	CoUninitialize();
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
tstring CUtils::Trim(string& str)
{
	int begin = 0;
	while (begin<(int)str.length() && (str[begin]==' ' || str[begin]=='\r' || str[begin]=='\t' || str[begin]=='\n'))
		begin++;
	int end = int(str.length()-1);
	while (end>begin && (str[end]==' ' || str[end]=='\r' || str[end]=='\t' || str[end]=='\n'))
		end--;

	return str.substr(begin,end-begin+1);
}