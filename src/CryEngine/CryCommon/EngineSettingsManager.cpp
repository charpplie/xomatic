#include "StdAfx.h"


// #include <CryModuleDefs.h>
#include "EngineSettingsManager.h"

#pragma warning (disable:4312)
#pragma warning(disable: 4244)

#if defined(WIN32) || defined(WIN64)

#include <assert.h>										// assert()
#include <windows.h>

#define IEntity IEntity_AfxOleOverride
#include <shlobj.h>
#include <shellapi.h>
#undef IEntity


// pseudo-variable that represents the DOS header of the module
EXTERN_C IMAGE_DOS_HEADER __ImageBase;


#define INFOTEXT		 _T("Please specify the directory of your CryENGINE installation (RootPath):")


static bool g_bWindowQuit;
static CEngineSettingsManager *g_pThis	= 0;
static const unsigned int IDC_hEditRootPath	= 100;
static const unsigned int IDC_hBtnBrowse		= 101;


BOOL BrowseForFolder(HWND hWnd, LPCTSTR szInitialPath, LPTSTR szPath, LPCTSTR szTitle);

// Desc: Static msg handler which passes messages to the application class.
LRESULT static CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{ 
	assert(g_pThis);

	if (uMsg == WM_INITDIALOG) 
	{
		int f = 0;
	}

	return g_pThis->HandleProc(hWnd, uMsg, wParam, lParam);
}



//////////////////////////////////////////////////////////////////////////
CEngineSettingsManager::CEngineSettingsManager(const TCHAR* moduleName, const TCHAR* iniFileName)
	: m_hWndParent(0)
{
	m_sModuleName = _T("");

	// std initialization
	RestoreDefaults();

	// try to load content from INI file
	if (moduleName != NULL)
	{
		m_sModuleName = tstring(moduleName);

		if (iniFileName==NULL)
		{
			// find INI filename located in module path
			HMODULE hInstance = GetModuleHandle(moduleName);
			TCHAR szFilename[_MAX_PATH];
			GetModuleFileName((HINSTANCE)&__ImageBase, szFilename, _MAX_PATH);
			TCHAR drive[_MAX_DRIVE];
			TCHAR dir[_MAX_DIR];
			TCHAR fname[_MAX_FNAME];
			
			_tsplitpath( szFilename,drive,dir,fname, NULL );
			_tmakepath( szFilename,drive,dir,fname, _T("INI") );
			m_sModuleFileName = tstring(szFilename);
		}
		else
			m_sModuleFileName = iniFileName;

		if (LoadValuesFromConfigFile(m_sModuleFileName.c_str()))
		{
			m_bGetDataFromRegistry = false;
			return;
		}
	}

	m_bGetDataFromRegistry = true;

	// load basic content from registry
	LoadEngineSettingsFromRegistry();
}


//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManager::RestoreDefaults()
{
	// Engine
	SetKey(_T("ENG_RootPath"), _T(""));

	// RC
	SetKey(_T("RC_ShowWindow"), false);
	SetKey(_T("RC_HideCustom"), false);
	SetKey(_T("RC_Parameters"), "");

	// Editor
	SetKey(_T("EDT_Prefer32Bit"), false);
	SetKey(_T("EDT_LicenseManualConfig"), false);

}

//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManager::SetRootPath( const tstring& szRootPath )
{ 
	tstring path = szRootPath;
	size_t const len = path.length();

	if ((len > 0) && ((path[len - 1] == '\\') || (path[len - 1] == '/')))
	{
		path = path.substr(0, len-1);
	}

	SetKey(_T("ENG_RootPath"), path);
}


//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::HasKey(const tstring& key)
{
	return m_keyToValueMap.find(key)!=m_keyToValueMap.end();
}

//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManager::SetKey(const tstring& key, const TCHAR* value)
{
	m_keyToValueMap[key] = value;
}

//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManager::SetKey(const tstring& key, const tstring& value)
{
	m_keyToValueMap[key] = value;
}


//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManager::SetKey(const tstring& key, bool value)
{
	m_keyToValueMap[key] = value ? _T("true") : _T("false");
}

//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManager::SetKey(const tstring& key, int value)
{
	TCHAR buf[1024];
	_stprintf(buf, _T("%d"), value);
	m_keyToValueMap[key] = tstring(buf);
}

//////////////////////////////////////////////////////////////////////////
tstring CEngineSettingsManager::GetRootPath() 
{ 
	LoadEngineSettingsFromRegistry();
	return GetValue<tstring>(_T("ENG_RootPath")); 
}


//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManager::CallSettingsDialog(void* pParent)
{
	HWND hParent = (HWND)pParent;

	tstring rootPath = GetRootPath();

	if (rootPath.empty())
	{
		CallRootPathDialog(hParent);
		return;
	}

	if (FindWindow(NULL,_T("CryENGINE Settings")))
		return;

	tstring params = tstring(m_sModuleName+_T(" \"")+m_sModuleFileName+_T("\""));
	HINSTANCE res = ::ShellExecute(hParent, _T("open"), _T("Tools\\SettingsMgr.exe"), params.c_str(), rootPath.c_str(), SW_SHOWNORMAL);

	if (res<(HINSTANCE)33)
	{
		MessageBox(hParent,_T("Could not execute CryENGINE settings dialog.\nPlease verify RootPath."),_T("Error"),MB_OK|MB_ICONERROR);
		CallRootPathDialog(hParent);
		return;
	}

	Sleep(1000);
}


//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManager::CallRootPathDialog(void* pParent)
{
	HWND hParent = (HWND)pParent;

	g_bWindowQuit = false;
	g_pThis = this;

	const TCHAR *szWindowClass = _T("CRYENGINEROOTPATHUI");

	// Register the window class
	WNDCLASS wndClass = { 0, WndProc, 0, DLGWINDOWEXTRA, GetModuleHandle(0), 
		NULL, LoadCursor( NULL, IDC_ARROW ), (HBRUSH)COLOR_BTNSHADOW, NULL, szWindowClass };

	RegisterClass(&wndClass);

	bool bReEnableParent=false;

	if(IsWindowEnabled(hParent))
	{
		bReEnableParent=true;
		EnableWindow(hParent,false);
	}

	int cwX = CW_USEDEFAULT;
	int cwY = CW_USEDEFAULT;
	int cwW = 400+2*GetSystemMetrics(SM_CYFIXEDFRAME);
	int cwH = 92+2*GetSystemMetrics(SM_CYFIXEDFRAME)+GetSystemMetrics(SM_CYSMCAPTION);

	if (hParent!=NULL)
	{
		// center window over parent
		RECT rParentRect;
		GetWindowRect(hParent, &rParentRect);
		cwX = rParentRect.left + (rParentRect.right-rParentRect.left)/2 - cwW/2;
		cwY = rParentRect.top + (rParentRect.bottom-rParentRect.top)/2 - cwH/2;
	}

	// Create the window
	HWND hDialogWnd = CreateWindowEx( WS_EX_TOOLWINDOW|WS_EX_CONTROLPARENT,szWindowClass,_T("CryENGINE RootPath"),WS_BORDER|WS_CAPTION|WS_SYSMENU|WS_VISIBLE, 
		cwX,cwY,cwW,cwH,hParent,NULL,GetModuleHandle(0),NULL);

	// ------------------------------------------

	LoadEngineSettingsFromRegistry();

	HINSTANCE hInst = GetModuleHandle(0);
	HGDIOBJ hDlgFont = GetStockObject (DEFAULT_GUI_FONT);

	// Engine Root Path

	HWND hStat0 = CreateWindow(_T("STATIC"),INFOTEXT, WS_CHILD | WS_VISIBLE,
		8,8,380,16, hDialogWnd,NULL, hInst, NULL);
	SendMessage(hStat0,WM_SETFONT,(WPARAM)hDlgFont,FALSE);

	HWND hWndRCPath = CreateWindowEx(WS_EX_CLIENTEDGE,_T("EDIT"),GetValue<tstring>(_T("ENG_RootPath")).c_str(), WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT | WS_TABSTOP| ES_READONLY,
		8,32,344,22, hDialogWnd,(HMENU)IDC_hEditRootPath, hInst, NULL);
	SendMessage(hWndRCPath,WM_SETFONT,(WPARAM)hDlgFont,FALSE);

	m_hBtnBrowse = CreateWindow(_T("BUTTON"),_T("..."), WS_CHILD | WS_VISIBLE,
		360,32,32,22, hDialogWnd,(HMENU)IDC_hBtnBrowse, hInst, NULL);
	SendMessage((HWND)m_hBtnBrowse,WM_SETFONT,(WPARAM)hDlgFont,FALSE);

	// std buttons

	HWND hWndOK = CreateWindow(_T("BUTTON"),_T("OK"), WS_CHILD | BS_DEFPUSHBUTTON | WS_CHILD | WS_VISIBLE | ES_LEFT | WS_TABSTOP,
		130,62,70,22, hDialogWnd,(HMENU)IDOK, hInst, NULL);
	SendMessage(hWndOK,WM_SETFONT,(WPARAM)hDlgFont,FALSE);

	HWND hWndCancel = CreateWindow(_T("BUTTON"),_T("Cancel"), WS_CHILD | WS_CHILD | WS_VISIBLE | ES_LEFT | WS_TABSTOP,
		210,62,70,22, hDialogWnd,(HMENU)IDCANCEL, hInst, NULL);
	SendMessage(hWndCancel,WM_SETFONT,(WPARAM)hDlgFont,FALSE);


	SetFocus(hWndRCPath);

	// ------------------------------------------

	{
		MSG msg;

		while(!g_bWindowQuit) 
		{
			GetMessage(&msg, NULL, 0, 0);

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	// ------------------------------------------

	DestroyWindow(hDialogWnd);
	UnregisterClass(szWindowClass,GetModuleHandle(0));

	if(bReEnableParent)
		EnableWindow(hParent,true);

	BringWindowToTop(hParent);

	g_pThis = NULL;
}


//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManager::SetParentDialog(unsigned long window)
{
	m_hWndParent = window;
}


//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::StoreData()
{
	if (m_bGetDataFromRegistry)
	{
		bool res = StoreEngineSettingsToRegistry();
		
		if (!res)
			MessageBox((HWND)m_hWndParent,_T("Could not store data to registry."),_T("Error"),MB_OK | MB_ICONERROR);
		return res;
	}

	// store data to INI file

	FILE* file;	
	_tfopen_s(&file, m_sModuleFileName.c_str(), _T("wb"));
	if (file==NULL)
		return false;

	for(TKeyValueMap::iterator it=m_keyToValueMap.begin();it!=m_keyToValueMap.end();it++)
		_ftprintf_s(file, (it->first+_T(" = ")+it->second+_T("\r\n")).c_str());

	fclose(file);

	return true;
}


//////////////////////////////////////////////////////////////////////////
tstring CEngineSettingsManager::Trim(tstring& str)
{
	int begin = 0;
	while (begin<(int)str.length() && (str[begin]==' ' || str[begin]=='\r' || str[begin]=='\t' || str[begin]=='\n'))
		begin++;
	int end = int(str.length()-1);
	while (end>begin && (str[end]==' ' || str[end]=='\r' || str[end]=='\t' || str[end]=='\n'))
		end--;

	return str.substr(begin,end-begin+1);
}


//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::LoadValuesFromConfigFile(const TCHAR* szFileName)
{
	m_keyToValueMap.clear();

	// read file to memory

	FILE* file;	
	_tfopen_s(&file, szFileName, _T("rb"));
	if (file==NULL)
		return false;

	fseek(file,0,SEEK_END);
	long size = ftell(file);
	fseek(file,0,SEEK_SET);
	TCHAR* data = new TCHAR[size+1];
	fread_s(data,size,1,size,file);
	fclose(file);

	// parse file for root path

	int start = 0, end = 0;
	while (end<size)
	{
		while (data[end]!='\n' && end<size)
			end++;

		memcpy(data,&data[start],end-start);
		data[end-start] = 0;
		start = end = end+1;

		tstring line(data);		
		int equalsOfs = int(line.find('='));
		if (equalsOfs>=0)
		{
			tstring key(Trim(line.substr(0, equalsOfs)));
			tstring value(Trim(line.substr(equalsOfs+1)));

			// Stay compatible to deprecated rootpath key
			if (key==_T("RootPath"))
			{
				key = _T("ENG_RootPath");
				if(value[value.length()-1]=='\\' || value[value.length()-1]=='/')
					value = value.substr(0, value.length()-1);
			}

			m_keyToValueMap[key] = value;
		}
	}
	delete [] data;

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::SetRegValue(void* key, const tstring& valueName, const tstring& value)
{
	size_t const sizeInBytes = (value.length() + 1) * sizeof(value[0]);
	return (ERROR_SUCCESS == RegSetValueEx((HKEY)key,(LPCTSTR)valueName.c_str(), 0, REG_SZ, (BYTE*)value.c_str(), DWORD(sizeInBytes)));
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::SetRegValue(void* key, const tstring& valueName, const TCHAR* value)
{
	size_t const sizeInBytes = (_tcslen(value) + 1) * sizeof(value[0]);
	return (ERROR_SUCCESS == RegSetValueEx((HKEY)key,(LPCTSTR)valueName.c_str(), 0, REG_SZ, (BYTE*)value, DWORD(sizeInBytes)));
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::SetRegValue(void* key, const tstring& valueName, bool value)
{
	DWORD dwVal = value;
	return (ERROR_SUCCESS == RegSetValueEx((HKEY)key,(LPCTSTR)valueName.c_str(), 0, REG_DWORD, (BYTE *)&dwVal, sizeof(dwVal)));
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::SetRegValue(void* key, const tstring& valueName, int value)
{
	DWORD dwVal = value;
	return (ERROR_SUCCESS == RegSetValueEx((HKEY)key,(LPCTSTR)valueName.c_str(), 0, REG_DWORD, (BYTE *)&dwVal, sizeof(dwVal)));
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::GetRegValue(void* key, const tstring& valueName, tstring& value)
{
	// Open the appropriate registry key
	enum {MAX_STRING_LENGTH = 2000};
	TCHAR str[MAX_STRING_LENGTH + 1];
	DWORD type;
	DWORD size = MAX_STRING_LENGTH;

	if (ERROR_SUCCESS == RegQueryValueEx( (HKEY)key, (LPCTSTR)valueName.c_str(), NULL, &type, (BYTE*)str, &size))
	{
		value = tstring(str);
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::GetRegValue(void* key, const tstring& valueName, bool& value)
{
	// Open the appropriate registry key
	DWORD type, dwVal=0, size = sizeof(dwVal);
	bool res = (ERROR_SUCCESS == RegQueryValueEx( (HKEY)key, (LPCTSTR)valueName.c_str(), NULL, &type, (BYTE*)&dwVal, &size));
	if( res )
	{
		value = dwVal!=0;
	}
	else
	{
		tstring valueStr;
		res = GetRegValue( (HKEY)key, (LPCTSTR)valueName.c_str(), valueStr );
		if ( res )
		{
			value = ( valueStr == _T( "true" ) );
		}
	}
	return res;
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::GetRegValue(void* key, const tstring& valueName, int& value)
{
	// Open the appropriate registry key
	DWORD type, dwVal=0, size = sizeof(dwVal);

	bool res = (ERROR_SUCCESS == RegQueryValueEx( (HKEY)key, (LPCTSTR)valueName.c_str(), NULL, &type, (BYTE*)&dwVal, &size));
	if( res )
		value = dwVal;

	return res;
}

//////////////////////////////////////////////////////////////////////////
bool CEngineSettingsManager::StoreEngineSettingsToRegistry()
{
	if (!m_bGetDataFromRegistry)
		return true;

	bool bRet=true;

	// make sure the path in registry exists

	RegKey key1(_T("Software\\Crytek"), true);
	if (!key1.pKey)
	{
		RegKey key0(_T("Software"), true);
		HKEY hKey1;
		RegCreateKey((HKEY)key0.pKey, _T("Crytek"), &hKey1);
		if(!hKey1)
			return false;
	}

	RegKey key2(_T("Software\\Crytek\\Settings"), true);
	if (!key2.pKey)
	{
		RegKey key1(_T("Software\\Crytek"), true);
		HKEY hKey2;
		RegCreateKey((HKEY)key1.pKey, _T("Settings"), &hKey2);
		if(!hKey2)
			return false;
	}


	RegKey key(_T("Software\\Crytek\\Settings"), true);
	if (!key.pKey)
	{
		bRet = false;
	}
	else
	{
		// Engine Specific
		bRet &= SetRegValue(key.pKey,_T("ENG_RootPath"), GetValue<tstring>(_T("ENG_RootPath")));

		// ResourceCompiler Specific
		bRet &= SetRegValue(key.pKey,_T("RC_ShowWindow"), GetValue<tstring>(_T("RC_ShowWindow"))==_T("true"));
		bRet &= SetRegValue(key.pKey,_T("RC_HideCustom"), GetValue<tstring>(_T("RC_HideCustom"))==_T("true"));
		bRet &= SetRegValue(key.pKey,_T("RC_Parameters"), GetValue<tstring>(_T("RC_Parameters")));

		// Editor Specific
		bRet &= SetRegValue(key.pKey,_T("EDT_Prefer32Bit"), GetValue<tstring>(_T("EDT_Prefer32Bit"))==_T("true"));
		bRet &= SetRegValue(key.pKey,_T("EDT_InstanceKey"), GetValue<tstring>(_T("EDT_InstanceKey")));
		bRet &= SetRegValue(key.pKey,_T("EDT_LicenseIp"), GetValue<tstring>(_T("EDT_LicenseIp")));
		bRet &= SetRegValue(key.pKey,_T("EDT_LicensePort"), GetValue<tstring>(_T("EDT_LicensePort")));
		bRet &= SetRegValue(key.pKey,_T("EDT_LicenseManualConfig"), GetValue<tstring>(_T("EDT_LicenseManualConfig")));
		bRet &= SetRegValue(key.pKey,_T("EDT_LevelDecryptionKey"), GetValue<tstring>(_T("EDT_LevelDecryptionKey")));
	}

	return bRet;
}


//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManager::LoadEngineSettingsFromRegistry()
{
	if (!m_bGetDataFromRegistry)
		return;

	TCHAR szKey[512] = {_T("Software\\Crytek\\Settings")};

	tstring sResult;
	bool bResult;

  // Engine Specific (Deprecated value)
	RegKey key(szKey, false);
	if (key.pKey)
	{
		if (GetRegValue(key.pKey, _T("RootPath"), sResult))
			SetKey(_T("ENG_RootPath"),sResult);

		// Engine Specific 
		if (GetRegValue(key.pKey, _T("ENG_RootPath"), sResult))
			SetKey(_T("ENG_RootPath"),sResult);

		// ResourceCompiler Specific
		if (GetRegValue(key.pKey, _T("RC_ShowWindow"), bResult))
			SetKey(_T("RC_ShowWindow"),bResult);
		if (GetRegValue(key.pKey, _T("RC_HideCustom"), bResult))
			SetKey(_T("RC_HideCustom"),bResult);
		if (GetRegValue(key.pKey, _T("RC_Parameters"), sResult))
			SetKey(_T("RC_Parameters"),sResult);

		// Editor Specific
		if (GetRegValue(key.pKey, _T("EDT_Prefer32Bit"), bResult))
			SetKey(_T("EDT_Prefer32Bit"),bResult);
		if (GetRegValue(key.pKey, _T("EDT_InstanceKey"), sResult))
			SetKey(_T("EDT_InstanceKey"),sResult);
		if (GetRegValue(key.pKey, _T("EDT_LicenseIp"), sResult))
			SetKey(_T("EDT_LicenseIp"),sResult);
		if (GetRegValue(key.pKey, _T("EDT_LicensePort"), sResult))
			SetKey(_T("EDT_LicensePort"),sResult);
		if (GetRegValue(key.pKey, _T("EDT_LicenseManualConfig"), sResult))
			SetKey(_T("EDT_LicenseManualConfig"),sResult);
		if (GetRegValue(key.pKey, _T("EDT_LevelDecryptionKey"), sResult))
			SetKey(_T("EDT_LevelDecryptionKey"),sResult);
	}
}


//////////////////////////////////////////////////////////////////////////
long CEngineSettingsManager::HandleProc(void* pWnd, long uMsg, long wParam, long lParam)
{
	HWND hWnd = (HWND)pWnd;

	switch(uMsg)
	{
	case WM_CREATE:
		break;

	case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
			case IDC_hBtnBrowse:
				{
					HWND hWndEdit = GetDlgItem(hWnd, IDC_hEditRootPath);
					LPSTR target[MAX_PATH];
					BrowseForFolder(hWnd,NULL, (LPTSTR)&target, INFOTEXT);
					SetWindowText(hWndEdit, (LPTSTR)&target);
				}
				break;

			case IDOK:
				{
					HWND hItemWnd = GetDlgItem(hWnd, IDC_hEditRootPath);
					TCHAR szPath[MAX_PATH];
					GetWindowText(hItemWnd, szPath, MAX_PATH);
					SetRootPath(szPath);
					StoreData();
				}

			case IDCANCEL:
				{
					g_bWindowQuit=true;
					break;
				}
			}
			break;
		}

	case WM_CLOSE:
		{
			g_bWindowQuit = true;
			break;
		}
	}

	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}


//////////////////////////////////////////////////////////////////////////
BOOL BrowseForFolder(HWND hWnd, LPCTSTR szInitialPath, LPTSTR szPath, LPCTSTR szTitle)
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
void CEngineSettingsManager::GetValueByRef(const tstring& key, tstring& value)
{
	value = m_keyToValueMap[key];
}

//////////////////////////////////////////////////////////////////////////
void CEngineSettingsManager::GetValueByRef(const tstring& key, int& value)
{
	value = _tcstol(m_keyToValueMap[key].c_str(), 0, 10);
}

//////////////////////////////////////////////////////////////////////////
CEngineSettingsManager::RegKey::RegKey(const tstring& key, bool writeable)
{
	HKEY  hKey;
	LONG result;
	if (writeable)
		result = RegCreateKeyEx(HKEY_CURRENT_USER, key.c_str(), 0, 0, 0, KEY_WRITE, 0, &hKey, 0);
	else
		result = RegOpenKeyEx( HKEY_CURRENT_USER,key.c_str(),0, KEY_READ, &hKey );
	pKey = hKey;
}

//////////////////////////////////////////////////////////////////////////
CEngineSettingsManager::RegKey::~RegKey()
{
	RegCloseKey((HKEY)pKey);
}

#endif //(WIN32) || (WIN64)
