#ifndef __EXPORTERSTATUSWINDOW_H__
#define __EXPORTERSTATUSWINDOW_H__

#include "UI/FrameWindow.h"
#include "UI/ProgressBar.h"
#include "UI/TaskList.h"
#include "UI/LogWindow.h"
#include "UI/Spacer.h"
#include "UI/Layout.h"
#include "UI/PushButton.h"

class ExportStatusWindow
{
public:
	enum MessageSeverity
	{
		MessageSeverity_Debug,
		MessageSeverity_Info,
		MessageSeverity_Warning,
		MessageSeverity_Error
	};

	ExportStatusWindow(int width, int height, const std::vector<std::pair<std::string, std::string> >& tasks);
	~ExportStatusWindow();

	void AddTask(const std::string& id, const std::string& description);
	void SetCurrentTask(const std::string& id);
	void SetProgress(float progress);
	void Log(MessageSeverity severity, const char* message);

private:
	void Initialize(int width, int height, const std::vector<std::pair<std::string, std::string> >& tasks);
	void Run();
	void OkPressed();

	FrameWindow m_frameWindow;
	TaskList m_taskList;
	ProgressBar m_progressBar;
	Spacer m_okButtonSpacer;
	PushButton m_okButton;
	Layout m_okButtonLayout;
	LogWindow m_logWindow;
	void* m_threadHandle;
	bool m_warningsOrErrorsEncountered;
};

#endif //__EXPORTERSTATUSWINDOW_H__
