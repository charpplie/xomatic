#include "stdafx.h"

#include "plugin_stub.h"
#include <commdlg.h>

#include "Properties.h"
#include "Crosswalk/helper.h"


void LogRCError(CString in_Filename, CString in_MessageType)
{
	Application app;

	HANDLE l_File;
	l_File = CreateFile(in_Filename.GetWideString(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	DWORD l_nFileSize;
	l_nFileSize = GetFileSize(l_File, NULL);
	char *l_Buffer = new char[l_nFileSize+1];
	memset(l_Buffer, 0, l_nFileSize+1);
	DWORD l_nBytesRead;
	ReadFile(l_File, l_Buffer, l_nFileSize, &l_nBytesRead, NULL);

	CString l_Error;
	l_Error.PutAsciiString(l_Buffer);
	app.LogMessage(l_Error);

	if (in_MessageType.Length())
	{
		Application app;

		CustomProperty l_CustomProperty;
		l_CustomProperty = app.GetActiveSceneRoot().AddProperty(L"CustomProperty", false, in_MessageType);
		Parameter l_Parameter;
		l_CustomProperty.AddParameter(in_MessageType,CValue::siString, siPersistable, L"", L"", L"", l_Parameter ) ;
		PPGLayout l_Layout = l_CustomProperty.GetPPGLayout();
		PPGItem l_Item = l_Layout.AddString( in_MessageType, L"", true, 600 );
		l_CustomProperty.PutParameterValue( in_MessageType, l_Error );

		CValueArray args(5);
		args[0] = l_CustomProperty;
		args[2] = in_MessageType;
		args[3] = (LONG)siModal;
		args[4] = false;
		CValue retval;
		app.ExecuteCommand( L"InspectObj", args, retval );

		XSI::COMMANDS::DeleteObj(l_CustomProperty.GetFullName());
	}

	CloseHandle(l_File);
	delete [] l_Buffer;
}

bool IsRCError(CString in_Filename)
{
	Application app;
	app.LogMessage(in_Filename);

	HANDLE l_File;
	l_File = CreateFile(in_Filename.GetWideString(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	DWORD l_nFileSize;
	l_nFileSize = GetFileSize(l_File, NULL);
	CloseHandle(l_File);

	return l_nFileSize ? true : false;
}

CRefArray l_CryExportNodesToExport;

void RunResourceCompiler(CString in_Filename, bool in_SkipMaterialLibraryCreation)
{
	Application app;

	CValue l_Value;
	app.GetPreferences().GetPreferenceValue(L"XSICryExport.Bin32Path", l_Value);
	CString l_Bin32Path = l_Value.GetAsText();
	CString l_Process = l_Bin32Path + L"\\rc\\rc.exe";
	CString l_ExportPath = GetExportPath();

	STARTUPINFO l_SI;
	PROCESS_INFORMATION l_PI;

	ZeroMemory(&l_SI, sizeof(l_SI));
	l_SI.cb = sizeof(l_SI);
	ZeroMemory(&l_PI, sizeof(l_PI));

	CString l_Filename = L" \"" + in_Filename + L"\"";
	if (!in_SkipMaterialLibraryCreation)
	{
		l_Filename += L" /createmtl=1";
	}

	BOOL result;
	result = CreateProcess((LPWSTR)l_Process.GetWideString(), (LPWSTR)l_Filename.GetWideString(), NULL, NULL, FALSE, 0, NULL, NULL, &l_SI, &l_PI);

	if( !result )
	{
		UIToolkit l_UIToolkit = app.GetUIToolkit();
		LONG l_Result;
		l_UIToolkit.MsgBox(L"Resource Compiler (RC) not found!", siMsgOkOnly, L"CryExport", l_Result);
	}

	// Wait until child process exits.
	WaitForSingleObject(l_PI.hProcess, INFINITE);

	// Close process and thread handles. 
	CloseHandle(l_PI.hProcess);
	CloseHandle(l_PI.hThread);

	CString l_LogSrc = l_Bin32Path + L"\\rc";
	CString l_LogDst = l_ExportPath;
	CString l_SrcRCLog = l_LogSrc + L"\\rc_log.log";
	CString l_SrcRCErrorsLog = l_LogSrc + L"\\rc_log_errors.log";
	CString l_SrcRCWarningsLog = l_LogSrc + L"\\rc_log_warnings.log";
	CString l_DstRCLog = l_LogDst + L"\\rc_log.log";
	CString l_DstRCErrorsLog = l_LogDst + L"\\rc_log_errors.log";
	CString l_DstRCWarningsLog = l_LogDst + L"\\rc_log_warnings.log";
	CopyFile( l_SrcRCLog.GetWideString(), l_DstRCLog.GetWideString(), false );
	CopyFile( l_SrcRCErrorsLog.GetWideString(), l_DstRCErrorsLog.GetWideString(), false );
	CopyFile( l_SrcRCWarningsLog.GetWideString(), l_DstRCWarningsLog.GetWideString(), false );

	if (IsRCError(l_DstRCErrorsLog))
	{
		app.LogMessage(l_DstRCErrorsLog, siErrorMsg);
		LogRCError(l_DstRCErrorsLog, L"Error");
	}
	else if (IsRCError(l_DstRCWarningsLog))
	{
		app.LogMessage(l_DstRCWarningsLog, siWarningMsg);
		LogRCError(l_DstRCWarningsLog, L"Warning");
	}

	CString l_FolderPath = GetExportPath();
	l_FolderPath += L"\\";

	for (LONG i = 0; i < l_CryExportNodesToExport.GetCount(); i++)
	{
		X3DObject l_Object(l_CryExportNodesToExport.GetItem(i));
		if (GetCryExportNodeFiletype(l_Object) & CRY_FILE_TYPE_CHR)
		{
			CustomProperty l_Property = l_Object.GetProperties().GetItem(L"ExportProperties");
			bool l_bExportable = l_Property.GetParameterValue(L"Exportable");
			if (!l_bExportable)
				continue;

			CString l_ShortFilename = l_Property.GetParameterValue(L"Filename");
			l_Filename = l_FolderPath + l_ShortFilename + L".chr";

			WIN32_FIND_DATA l_FD;
			HANDLE hFind;
			hFind = FindFirstFile(l_Filename.GetWideString(), &l_FD);
			if (hFind == INVALID_HANDLE_VALUE)
				continue;
			FindClose(hFind);

			ZeroMemory(&l_SI, sizeof(l_SI));
			l_SI.cb = sizeof(l_SI);
			ZeroMemory(&l_PI, sizeof(l_PI));

			l_Filename = L" \"" + l_Filename + L"\"";

			BOOL result;
			result = CreateProcess((LPWSTR)l_Process.GetWideString(), (LPWSTR)l_Filename.GetWideString(), NULL, NULL, FALSE, 0, NULL, NULL, &l_SI, &l_PI);

			// Wait until child process exits.
			WaitForSingleObject(l_PI.hProcess, INFINITE);

			// Close process and thread handles. 
			CloseHandle(l_PI.hProcess);
			CloseHandle(l_PI.hThread);

			l_DstRCErrorsLog = l_LogDst + L"\\" + l_ShortFilename + L"_rc_log_errors.log";
			l_DstRCWarningsLog = l_LogDst + L"\\" + l_ShortFilename + L"_rc_log_warnings.log";
			l_DstRCLog = l_LogDst + L"\\" + l_ShortFilename + L"_rc_log.log";

			CopyFile( l_SrcRCLog.GetWideString(), l_DstRCLog.GetWideString(), false );
			CopyFile( l_SrcRCErrorsLog.GetWideString(), l_DstRCErrorsLog.GetWideString(), false );
			CopyFile( l_SrcRCWarningsLog.GetWideString(), l_DstRCWarningsLog.GetWideString(), false );

			if (IsRCError(l_DstRCErrorsLog))
			{
				app.LogMessage(l_DstRCErrorsLog, siErrorMsg);
				LogRCError(l_DstRCErrorsLog, L"Error");
			}
			else if (IsRCError(l_DstRCWarningsLog))
			{
				app.LogMessage(l_DstRCWarningsLog, siWarningMsg);
				LogRCError(l_DstRCWarningsLog, L"Warning");
			}
		}
	}
}
