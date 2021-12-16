#include "stdafx.h"

#include "IInterfaceHooks.h"

IInterfaceHooks* IInterfaceHooks::ms_hooks;

void IInterfaceHooks::SetHooks(IInterfaceHooks* hooks)
{
	ms_hooks = hooks;
}

void IInterfaceHooks::InvokeOnMain(InvokeHandler handler, void* user)
{
	if (ms_hooks)
		ms_hooks->InvokeOnMainImpl(handler, user);
}

void IInterfaceHooks::ShowMessage(const TCHAR *msg, const TCHAR *title)
{
	if (ms_hooks)
		ms_hooks->ShowMessageImpl(msg, title);
}

std::string IInterfaceHooks::OpenFile(const char* filter, const TCHAR* title)
{
	if (ms_hooks)
		return ms_hooks->OpenFileImpl(filter, title);
	return std::string();
}

std::string IInterfaceHooks::OpenFolder(const TCHAR* title)
{
	if (ms_hooks)
		return ms_hooks->OpenFolderImpl(title);
	return std::string();
}

void IInterfaceHooks::RunProgressTask(IProgressTask &task)
{
	if (ms_hooks)
		ms_hooks->RunProgressTaskImpl(task);
}
