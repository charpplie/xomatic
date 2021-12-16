#pragma once

class IProgressTask
{
public:
	virtual ~IProgressTask() {}

	virtual void Start() = 0;

	virtual int GetProgress() const = 0;
	virtual int GetMaximumProgress() const = 0;
};

class IProgressReporter
{
public:
	virtual ~IProgressReporter() {}

	virtual int GetProgress() const = 0;
	virtual int GetMaximumProgress() const = 0;
};

class IInterfaceHooks
{
public:
	typedef void (*InvokeHandler)(void*);

public:
	virtual ~IInterfaceHooks() {}

	static void SetHooks(IInterfaceHooks* hooks);

	static void InvokeOnMain(InvokeHandler handler, void* user);
	static void ShowMessage(const TCHAR* msg, const TCHAR* title);
	static std::string OpenFile(const char* filter, const TCHAR* title);
	static std::string OpenFolder(const TCHAR* title);

	static void RunProgressTask(IProgressTask& task);

private:
	virtual void InvokeOnMainImpl(InvokeHandler handler, void* user) = 0;
	virtual void ShowMessageImpl(const TCHAR* msg, const TCHAR* title) = 0;
	virtual std::string OpenFileImpl(const char* filter, const TCHAR* title) = 0;
	virtual std::string OpenFolderImpl(const TCHAR* title) = 0;
	virtual void RunProgressTaskImpl(IProgressTask& task) = 0;

private:
	static IInterfaceHooks* ms_hooks;
};
