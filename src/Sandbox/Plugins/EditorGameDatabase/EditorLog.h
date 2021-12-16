////////////////////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.
// ----------------------------------------------------------------------------------------
//  File name:   EditorLog
//  Description: Helper class to log info and redirect it to a listener
//
//////////////////////////////////////////////////////////////////////////////////////////// 

#ifndef _EDITOR_LOG_H_
#define _EDITOR_LOG_H_

#pragma once

#include <functor.h>

#define EDITOR_LOG_DATABASE(...) EditorLog::Log("[Database] " __VA_ARGS__)
#define EDITOR_LOG_SOURCE_CONTROL(...) EditorLog::Log("[Source Control] " __VA_ARGS__)

class EditorLog
{
public:
	typedef Functor1<const char*> TDelegate;

	EditorLog(const TDelegate& _delegate);
	~EditorLog();

	static void Log(const char* format, ...);

private:
	static TDelegate s_delegate;
};

#endif // _EDITOR_LOG_H_

