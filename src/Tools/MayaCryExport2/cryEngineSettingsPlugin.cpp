
#include "StdAfx.h"
#include "stdio.h"

#include "mayaIncludes.h"
#include "cryEngineSettingsPlugin.h"

#include "ResourceCompilerHelper.h"

// The names of the commands that can be passed in
char *cryEngineSettingsPlugin::buildPathString = "buildPath";

cryEngineSettingsPlugin::cryEngineSettingsPlugin()
{
}

cryEngineSettingsPlugin::~cryEngineSettingsPlugin()
{
}

MStatus cryEngineSettingsPlugin::doIt( const MArgList& args )
{
	CResourceCompilerHelper rHelper;
	TCHAR szRemoteCmdLine[MAX_PATH*3];
	tstring rootPath = rHelper.GetRootPath( true );
	_stprintf(szRemoteCmdLine,"%s\\%s",rootPath.c_str(),"Tools\\SettingsMgr.exe");
	// Create a pipe to read the stdout of the RC.
	SECURITY_ATTRIBUTES saAttr;
	std::memset(&saAttr, 0, sizeof(saAttr));
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = 0;
	HANDLE hChildStdOutRd, hChildStdOutWr;
	CreatePipe(&hChildStdOutRd, &hChildStdOutWr, &saAttr, 0);
	SetHandleInformation(hChildStdOutRd, HANDLE_FLAG_INHERIT, 0); // Need to do this according to MSDN
	HANDLE hChildStdInRd, hChildStdInWr;
	CreatePipe(&hChildStdInRd, &hChildStdInWr, &saAttr, 0);
	SetHandleInformation(hChildStdInWr, HANDLE_FLAG_INHERIT, 0); // Need to do this according to MSDN

	STARTUPINFO si;
	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	si.dwX = 100;
	si.dwY = 100;
	si.hStdError = hChildStdOutWr;
	si.hStdOutput = hChildStdOutWr;
	si.hStdInput = hChildStdInRd;
	si.dwFlags = STARTF_USEPOSITION | STARTF_USESTDHANDLES;

	PROCESS_INFORMATION pi;
	ZeroMemory( &pi, sizeof(pi) );

	if( !CreateProcess( NULL, // No module name (use command line). 
		szRemoteCmdLine,				// Command line. 
		NULL,									  // Process handle not inheritable. 
		NULL,									  // Thread handle not inheritable. 
		TRUE,								  // Set handle inheritance to TRUE. 
		CREATE_NEW_PROCESS_GROUP,
		NULL,									  // Use parent's environment block. 
		rootPath.c_str(),			// Set starting directory. 
		&si,										// Pointer to STARTUPINFO structure.
		&pi ))									  // Pointer to PROCESS_INFORMATION structure.
	{
		TCHAR szMessage[65535]=_T("");
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,GetLastError(),0,szMessage,65354,NULL);
		GetCurrentDirectory(65534,szMessage);
		MessageBox(0,_T("CryEngine SettingsMgr.exe was not found."),_T("Error"),MB_ICONERROR|MB_OK);
		return MStatus::kFailure;
	}

	// Close the pipe that writes to the child process, since we don't actually have any input for it.
	CloseHandle(hChildStdInWr);

	// Read all the output from the child process.
	CloseHandle(hChildStdOutWr);

	// Wait until child process exits.
	WaitForSingleObject( pi.hProcess, INFINITE );
	// Close process and thread handles. 
	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );
	return MS::kSuccess;
}

bool cryEngineSettingsPlugin::isUndoable() const
{
	return false;
}

void* cryEngineSettingsPlugin::creator()
{
	return new cryEngineSettingsPlugin();
}