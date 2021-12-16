//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.

#include "StdAfx.h"
#include "SourceControl.h"

#include "GameBind.h"
#include "EditorLog.h"
#include "InputDialog.h"

#include <ISourceControl.h>

#define SOURCECONTROL_DEFAULT_CHANGELIST  "Gamedatabase Default CL"

SourceControl::SourceControl()
	: m_changeId("default")
{

}

bool SourceControl::CheckoutFile( const char* filePath )
{
	CreateChangeList();

	bool checkOutDone = false;

	ISourceControl* pSourceControl = GetISourceControl();
	if (pSourceControl)
	{
		const uint32 fileAttributes = pSourceControl->GetFileAttributes(filePath);
		if (fileAttributes & SCC_FILE_ATTRIBUTE_CHECKEDOUT)
		{
			checkOutDone = true;
		}
		else if ((fileAttributes & SCC_FILE_ATTRIBUTE_MANAGED) && !(fileAttributes & SCC_FILE_ATTRIBUTE_BYANOTHER))
		{
			if (pSourceControl->GetLatestVersion(filePath))
			{
				char* changeId = m_changeId.GetBuffer();
				checkOutDone = pSourceControl->CheckOut(filePath, ADD_CHANGELIST, changeId);
			}

			EDITOR_LOG_SOURCE_CONTROL("Checking out file: %s (%s)", filePath, checkOutDone ? "OK" : "Failed");
		}
		else if (fileAttributes & SCC_FILE_ATTRIBUTE_NORMAL)
		{
			char* changeId = m_changeId.GetBuffer();
			checkOutDone = pSourceControl->Add(filePath, NULL, ADD_WITHOUT_SUBMIT | ADD_CHANGELIST, changeId);

			EDITOR_LOG_SOURCE_CONTROL("Adding file: %s (%s)", filePath, checkOutDone ? "OK" : "Failed");
		}
	}
	
	bool attemptOverride = !checkOutDone;
	if(attemptOverride)
	{
		DWORD fileAttributes = GetFileAttributesA(filePath);
		if ((fileAttributes != INVALID_FILE_ATTRIBUTES) && (fileAttributes & FILE_ATTRIBUTE_READONLY))
		{
			// file is read-only AND there's no source control attached
			stack_string message;
			message.Format("File '%s' is read-only and source control was not detected/failed, file will be overriden", filePath);
			ConfirmationDialog confirmOverrideDialog("Overriding file", message.c_str(), ConfirmationDialog::OPTIONS_OK);
			if(confirmOverrideDialog.exec())
			{
				const BOOL flagChanged = SetFileAttributesA(filePath, FILE_ATTRIBUTE_NORMAL);

				EDITOR_LOG_SOURCE_CONTROL("Removing read-only flag from: %s (%s)", filePath, flagChanged ? "OK" : "Failed");
			}
		}
	}

	return checkOutDone;
}

bool SourceControl::RevertFile( const char* filePath )
{
	bool fileReverted = false;
	
	ISourceControl* pSourceControl = GetISourceControl();
	if (pSourceControl)
	{
		fileReverted = pSourceControl->UndoCheckOut(filePath);
	}

	if (fileReverted)
	{
		EDITOR_LOG_SOURCE_CONTROL("File '%s' reverted", filePath);
	}

	return fileReverted;
}

bool SourceControl::DeleteFile( const char* filePath )
{
	CreateChangeList();

	bool fileDeleted = CFileUtil::DeleteFileA(CString(filePath));

	ISourceControl* pSourceControl = GetISourceControl();
	if (pSourceControl)
	{
		char* changeId = m_changeId.GetBuffer();
		pSourceControl->Delete(filePath, NULL, ADD_CHANGELIST | DELETE_WITHOUT_SUBMIT, changeId);
	}
	
	if (fileDeleted)
	{
		EDITOR_LOG_SOURCE_CONTROL("File '%s' deleted", filePath);
	}

	return fileDeleted;
}

bool SourceControl::CreateChangeList()
{
	ISourceControl* pISourceControl = GetISourceControl();
	if (pISourceControl)
	{
		char changeId[16];
		if (pISourceControl->DoesChangeListExist(SOURCECONTROL_DEFAULT_CHANGELIST, changeId, sizeof(changeId)))
		{
			m_changeId = CString(changeId);
			return true;
		}

		if (pISourceControl->CreateChangeList(SOURCECONTROL_DEFAULT_CHANGELIST, changeId, sizeof(changeId)))
		{
			m_changeId = CString(changeId);
			return true;
		}
	}

	return false;
}

ISourceControl* SourceControl::GetISourceControl() const
{
	return GameBind::Get().GetSourceControl();
}
