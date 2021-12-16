#ifndef __IEXPORTCONTEXT_H__
#define __IEXPORTCONTEXT_H__

#include <cstdarg>
#include "Exceptions.h"

class IPakSystem;
class ISettings;

class IExportContext
{
public:
	enum MessageSeverity
	{
		MessageSeverity_Debug,
		MessageSeverity_Info,
		MessageSeverity_Warning,
		MessageSeverity_Error
	};

	// Declare an exception type to report the case where the scene must be saved before exporting.
	struct NeedSaveErrorTag {};
	typedef Exception<NeedSaveErrorTag> NeedSaveError;
	struct PakSystemErrorTag {};
	typedef Exception<PakSystemErrorTag> PakSystemError;

	virtual void SetProgress(float progress) = 0;
	virtual void SetCurrentTask(const std::string& id) = 0;
	virtual void Log(MessageSeverity severity, const char* message) = 0;
	virtual IPakSystem* GetPakSystem() = 0;
	virtual ISettings* GetSettings() = 0;
	virtual void GetRootPath(char* buffer, int bufferSize) = 0;
};

struct CurrentTaskScope
{
	CurrentTaskScope(IExportContext* context, const std::string& id): context(context) {context->SetCurrentTask(id);}
	~CurrentTaskScope() {context->SetCurrentTask("");}
	IExportContext* context;
};

inline void Log(IExportContext* context, IExportContext::MessageSeverity severity, const char* messageFmt, ...)
{
	char buffer[2048];
	{
		va_list args;
		va_start(args, messageFmt);
		vsprintf(buffer, messageFmt, args);
		va_end(args);
	}

	context->Log(severity, buffer);
}

#endif //__IEXPORTCONTEXT_H__
