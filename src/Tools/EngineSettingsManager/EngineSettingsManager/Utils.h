#pragma once


class CUtils
{
public:
	static BOOL BrowseForFolder(HWND hWnd, LPCTSTR szInitialPath, LPTSTR szPath, LPCTSTR szTitle);
	static string Trim(string& str);
};
