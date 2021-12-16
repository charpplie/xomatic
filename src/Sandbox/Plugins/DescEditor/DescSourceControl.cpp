////////////////////////////////////////////////////////////////////////////
//
//  Crytek Source File.
//  Copyright (C), Crytek Studios, 2013-3013
// -------------------------------------------------------------------------
//  File Name        : DescEditor.cpp
//  Version          : v1.00
//  Created          : 3/21/2013 by Jack Harmon
//  Description      : Custom editor dialog for ObjectDescFactory objects
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "DescSourceControl.h"

using namespace CryGame;

bool CDescSourceControl::AddFile(const char* pPath)
{
	string response;
	bool result;

	result = P4Command("add", pPath, response);
	if (!result || response.find(string(" opened for add")) == -1)
	{
		// Todo: Log the response string to error log
		return false;
	}

	return true;
}

bool CDescSourceControl::CheckoutFile(const char* pPath)
{
	string response;
	bool result;

	result = P4Command("edit", pPath, response);
	if (!result || response.find(string(" opened")) == -1)
	{
		// Todo: Log the response string to error log
		return false;
	}

	return true;
}

bool CDescSourceControl::DeleteFile(const char* pPath)
{
	string response;
	bool result;

	result = P4Command("revert", pPath, response);
	if (!result || response.find(string(" opened")) == -1)
	{
		if (response.find(string(" was add")) != -1)
		{
			// Was added but not submitted. Verify they want to delete the local file.
			if (MessageBox(NULL, "File had not been submitted to source control and was reverted.\nDelete local file?", "Notice", MB_YESNO | MB_ICONSTOP) == IDYES)
			{
				// Delete the local file
				if (!::DeleteFileA(pPath))
				{
					return false;
				}
			}

			return true;
		}

		if (response.find(string(" was edit")) == -1)
		{
			// Todo: Log the response string to error log
			return false;
		}
	}

	// Verify it exists.  
	if (!CFileUtil::FileExists(pPath))
	{
		MessageBox(NULL, "File doesn't exist. (Is mod pack loaded?)", "Notice", MB_OK); 
		return false;
	}

	result = P4Command("delete", pPath, response);
	if (!result || response.find(string("files(s) not on client.")) != -1)
	{
		// Todo: Log the response string to error log
		return false;
	}

	// Success
	return true;
}

bool CDescSourceControl::RenameFile(const char* pOldPath, const char* pNewPath)
{
	string response;
	bool result;
	string argument = string(pOldPath) + " " + string(pNewPath);

	if (!CheckoutFile(pOldPath))
	{
		return false;
	}

	result = P4Command("move", argument, response);
	if (!result || response.find(string(" moved from")) == -1)
	{
		// Todo: Log the response string to error log
		return false;
	}

	return true;
}

bool CDescSourceControl::P4Command(const char* pCommand, const char* pPath, string& response)
{
	SECURITY_ATTRIBUTES securityAttributes;
	STARTUPINFO startupInfo;
	PROCESS_INFORMATION processInfo;
	HANDLE readPipe = 0;
	HANDLE writePipe = 0;
	string commandline;

	ZeroMemory(&securityAttributes, sizeof(securityAttributes));
	ZeroMemory(&startupInfo, sizeof(startupInfo));
	ZeroMemory(&processInfo, sizeof(processInfo));

	response.clear();

	securityAttributes.nLength = sizeof(securityAttributes);
	securityAttributes.bInheritHandle = TRUE;

	// Create pipes to write and read data
	if (!CreatePipe(&readPipe, &writePipe, &securityAttributes, 0))
	{
		return false;
	}

	startupInfo.cb = sizeof(startupInfo);
	startupInfo.dwFlags = STARTF_USESTDHANDLES;
	startupInfo.hStdInput = NULL;
	startupInfo.hStdOutput = writePipe;
	startupInfo.hStdError = writePipe;

	commandline.Format("p4.exe %s %s", pCommand, pPath);

	if (CreateProcess(NULL, (char*)commandline.c_str(), NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, NULL, NULL, &startupInfo, &processInfo) == 0)
	{
		return false;
	}

	CloseHandle(writePipe);

	char buffer[100];
	DWORD bytesRead;
	BOOL result;

	do
	{
		result = ReadFile(readPipe, buffer, 100, &bytesRead, NULL);
		response.append(buffer, bytesRead);
	}
	while (result);

	CloseHandle(readPipe);
	CloseHandle(processInfo.hProcess);
	CloseHandle(processInfo.hThread);

	if (response.find("connect to server failed") != -1)
	{
		// Todo: Log the response string to error log
		return false;
	}

	return true;
}
