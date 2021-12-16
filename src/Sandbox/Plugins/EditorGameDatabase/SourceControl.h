////////////////////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.
// ----------------------------------------------------------------------------------------
//  File name:   SourceControl
//  Description: Helper class which gives access to editor source control interface
//
//////////////////////////////////////////////////////////////////////////////////////////// 

#ifndef _SOURCE_CONTROL_H_
#define _SOURCE_CONTROL_H_

#pragma once

struct ISourceControl;

class SourceControl
{
public:
	SourceControl();

	bool CheckoutFile( const char* filePath );
	bool RevertFile( const char* filePath );
	bool DeleteFile( const char* filePath );

private:
	bool CreateChangeList();
	
	ISourceControl* GetISourceControl() const;

private:
	CString m_changeId;
};

#endif // _SOURCE_CONTROL_H_

