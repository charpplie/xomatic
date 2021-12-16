#include "StdAfx.h"
#include "FolderBrowser.h"
#include <shlobj.h>

std::string BrowseForFolder(HWND owner, const char* title, const char* defaultPath)
{
	wchar_t unicodePath[MAX_PATH + 1];
	int length = strlen(defaultPath);
	if (length == mbstowcs(unicodePath, defaultPath, length))
		unicodePath[length] = 0;

	class DialogProc
	{
	public:
		DialogProc(const char* defaultPath): defaultPath(defaultPath) {}

		static int __stdcall BrowseCtrlCallbackStatic(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
		{
			return ((DialogProc*)lpData)->BrowseCtrlCallback(hwnd, uMsg, lParam);
		}

		int BrowseCtrlCallback(HWND hwnd, UINT uMsg, LPARAM lParam)
		{
			if (uMsg == BFFM_INITIALIZED && this->defaultPath && this->defaultPath[0])
			{
				::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)this->defaultPath);
			}
			else // uMsg == BFFM_SELCHANGED
			{
			}

			return 0;
		}

	private:
		const char* defaultPath;
	};

	DialogProc dlgProc(defaultPath);

	TCHAR dname[MAX_PATH];
	IMalloc *imalloc; SHGetMalloc(&imalloc);
	BROWSEINFO bi; ZeroMemory(&bi,sizeof(bi));
	bi.hwndOwner = owner;
	bi.pszDisplayName=dname;
	bi.lpszTitle = title;
	bi.lpfn = &DialogProc::BrowseCtrlCallbackStatic;
  bi.lParam = (LPARAM)&defaultPath;

	bi.ulFlags = BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
	ITEMIDLIST *pidl = SHBrowseForFolder(&bi);

	std::string directory;
	if (pidl)
	{
		char buffer[MAX_PATH + 1];
		if (SHGetPathFromIDList(pidl,buffer))
			directory = buffer;
	}

	if (pidl)
		imalloc->Free(pidl);
	imalloc->Release();

	return directory;
}
