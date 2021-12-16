#include "stdafx.h"
#include <commdlg.h>
#include "plugin_stub.h"

#define XSI_CRYTEK_EXPORT_VERSION L"0.1.0"
#define XSI_CRYTEK_EXPORT_FEATURES \
		L"Exports CGF static geometry\r\n"\
		L"Exports CHR character files\r\n"\
		L"Exports CAF character animations\r\n"

BOOL GetAppVersion( TCHAR *pLibName, WORD *pMajorVersion, WORD *pMinorVersion, WORD *pBuildNumber, WORD *pRevisionNumber )
{
	DWORD				dwHandle, dwLen;
	UINT				BufLen;
	LPTSTR				lpData;
	VS_FIXEDFILEINFO	*pFileInfo;

	dwLen = GetFileVersionInfoSize( pLibName, &dwHandle );

	if( !dwLen ) 
		return FALSE;

	lpData = new TCHAR[dwLen];

	if( !lpData ) 
		return FALSE;

	if( !GetFileVersionInfo( pLibName, dwHandle, dwLen, lpData ) )
	{
		delete [] lpData;

		return FALSE;
	}

	if( VerQueryValue( lpData, L"\\", (LPVOID *) &pFileInfo, (PUINT)&BufLen ) ) 
	{
		*pMajorVersion = HIWORD(pFileInfo->dwFileVersionMS);
		*pMinorVersion = LOWORD(pFileInfo->dwFileVersionMS);
		*pBuildNumber = HIWORD(pFileInfo->dwFileVersionLS);
		*pRevisionNumber = LOWORD(pFileInfo->dwFileVersionLS);
		delete [] lpData;

		return TRUE;
	}

	delete [] lpData;

	return FALSE;
}


_XSI_EXTERN_ CStatus CryExportAbout_Init( CRef& in_ref )
{
	CStatus status = CStatus::OK;

	Context ctxt = in_ref;
	Menu menu = ctxt.GetSource();

	CStatus st;
	MenuItem item;

	// Here we add our menu item in the export menu
	menu.AddCallbackItem(L"CryExportAbout", L"CryExportAbout_Execute", item);

	return status;
}

_XSI_EXTERN_ CStatus CryExportAbout_Execute( CRef& in_ref )
{
	CStatus status = CStatus::OK;
	Application app;

	Property prop = app.GetActiveSceneRoot().GetProperties().GetItem( L"CryExportAbout" ) ;

	if (!prop.IsValid())
	{
		prop = app.GetActiveSceneRoot().AddProperty( L"CryExportAbout" ) ;
	}

	CValue retval = XSI::COMMANDS::InspectObj(L"CryExportAbout",CValue(),L"XSICryExport About",(LONG)siModal,false);

	return status;
}

/****************************************************************************************
XSI plugin custom properties function
****************************************************************************************/
_XSI_EXTERN_ CStatus CryExportAbout_Define( const CRef & in_Ctx )
{
	CStatus status = CStatus::OK;
	Application app ;
	CustomProperty prop = Context(in_Ctx).GetSource() ;
	Parameter param;

	// Default capabilities for most of these parameters
	int caps = siPersistable|siReadOnly;
	prop.AddParameter( L"CryExportAbout_XSIAddonVersion",CValue::siString, caps, L"CryExportAbout_XSIAddonVersion", L"", XSI_CRYTEK_EXPORT_VERSION, param );
	wchar_t* pCrtDate = _T(__DATE__) L" @ " _T(__TIME__) ;
	prop.AddParameter( L"CryExportAbout_XSIAddonBuildDate",CValue::siString, caps, L"CryExportAbout_XSIAddonBuildDate", L"", CValue(pCrtDate), param );
	prop.AddParameter( L"CryExportAbout_RCBuildVersion",CValue::siString, caps, L"CryExportAbout_RCBuildVersion", L"", L"", param );
	
	CValue l_Value;
	app.GetPreferences().GetPreferenceValue(L"XSICryExport.Bin32Path", l_Value);
	CString l_Bin32Path = l_Value.GetAsText();
	CString l_Process = l_Bin32Path + L"\\rc\\rc.exe";
	
	HANDLE hRcExe = CreateFile( l_Process.GetWideString(), GENERIC_READ, FILE_SHARE_READ, NULL,	OPEN_EXISTING, 0, NULL );
	bool bError = false;

	if( INVALID_HANDLE_VALUE != hRcExe )
	{
		FILETIME ftCreation, ftLastAccess, ftLastWrite;

		if( GetFileTime( hRcExe, &ftCreation, &ftLastAccess, &ftLastWrite ) )
		{
			TCHAR strFmt[1000];
			SYSTEMTIME stUTC;
			WORD MajorVersion, MinorVersion, BuildNumber, RevisionNumber;

			FileTimeToSystemTime( &ftCreation, &stUTC );
			GetAppVersion( (TCHAR*)l_Process.GetWideString(), &MajorVersion, &MinorVersion, &BuildNumber, &RevisionNumber );
			swprintf_s( strFmt, L"%d.%d.%d rev#%d, %2d/%2d/%d @ %2d:%2d",
						MajorVersion, MinorVersion, BuildNumber, RevisionNumber,
						stUTC.wMonth, stUTC.wDay, stUTC.wYear, stUTC.wHour, stUTC.wMinute );
			param.PutValue( strFmt );
		}
		else
		{
			bError = true;
		}
	}
	else
		bError = true;

	if( bError )
	{
		app.LogMessage( L"Could not find RC.exe, did you set the path to the engine binaries ?", siErrorMsg );
		param.PutValue( L"*Error: rc.exe not found*" );
	}


	prop.AddParameter( L"CryExportAbout_RCPath", CValue::siString, caps, L"CryExportAbout_RCPath", L"", L"", param );
	param.PutValue( l_Bin32Path );

	prop.AddParameter( L"CryExportAbout_Features", CValue::siString, caps, L"CryExportAbout_Features", L"", XSI_CRYTEK_EXPORT_FEATURES, param );

	return status;
}


_XSI_EXTERN_ CStatus CryExportAbout_RebuildLayout( PPGLayout &in_Layout, CustomProperty &in_CustomProp )
{
	CStatus status = CStatus::OK;
	CString l_Empty;

	// XSI will call this to define the visual appearance of the CustomProperty
	// The layout is shared between all instances of the CustomProperty
	// and is cached.  You can force the code to re-execute by using the 
	// XSIUtils.Refresh feature.

	PPGItem item;

	in_Layout.Clear();
	in_Layout.AddRow();
	in_Layout.AddGroup( L"XSICrytek Plugin Information", true, 0);
	in_Layout.AddString( L"CryExportAbout_XSIAddonVersion", L"XSI Addon Version:" );
	in_Layout.AddString( L"CryExportAbout_XSIAddonBuildDate", L"XSI Addon Build Date:" );
	in_Layout.AddString( L"CryExportAbout_RCBuildVersion", L"RC.exe Build Version:" );
	in_Layout.AddString( L"CryExportAbout_RCPath", L"RC.exe Path:" );
	in_Layout.AddString( L"CryExportAbout_Features", L"Current features:", true );
	in_Layout.EndGroup();
	in_Layout.EndRow();

	return status;
}

_XSI_EXTERN_ CStatus CryExportAbout_DefineLayout( const CRef & in_Ctx )
{
	CStatus status = CStatus::OK;

	// XSI will call this to define the visual appearance of the CustomProperty
	// The layout is shared between all instances of the CustomProperty
	// and is cached.  You can force the code to re-execute by using the 
	// XSIUtils.Refresh feature.

	PPGLayout oLayout = Context( in_Ctx ).GetSource() ;

	CustomProperty cusProp;
	status = CryExportAbout_RebuildLayout(oLayout, cusProp);

	return status;
}

_XSI_EXTERN_ CStatus CryExportAbout_PPGEvent( const CRef& io_Ctx )
{
	CStatus status = CStatus::OK;
	Application app ;

	PPGEventContext ctx( io_Ctx ) ;

	//Redraw the PPG to show the new combo items
	ctx.PutAttribute(L"Refresh",true);

	PPGEventContext::PPGEvent eventID = ctx.GetEventID() ;

	if( eventID == PPGEventContext::siOnInit )
	{
		CustomProperty prop = ctx.GetSource();
		PPGLayout oPPGLayout = prop.GetPPGLayout();		
		CryExportAbout_RebuildLayout(oPPGLayout, prop);

		//Redraw the PPG to show the new combo items
		ctx.PutAttribute(L"Refresh",true);
	}

	return status;
}
