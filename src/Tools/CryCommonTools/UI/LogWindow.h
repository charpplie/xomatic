#ifndef __LOGWINDOW_H__
#define __LOGWINDOW_H__

#include "IUIComponent.h"
#include "Layout.h"
#include "ListView.h"
#include "ToggleButton.h"
#include <list>

class LogWindow : public IUIComponent
{
public:
	enum MessageSeverity
	{
		MessageSeverity_Debug,
		MessageSeverity_Info,
		MessageSeverity_Warning,
		MessageSeverity_Error
	};

	LogWindow();
	~LogWindow();

	void Log(MessageSeverity severity, const TCHAR* message);
	void SetFilter(MessageSeverity, bool visible);

	// IUIComponent
	virtual void CreateUI(void* window, int left, int top, int width, int height);
	virtual void Resize(void* window, int left, int top, int width, int height);
	virtual void DestroyUI(void* window);
	virtual void GetExtremeDimensions(void* window, int& minWidth, int& maxWidth, int& minHeight, int& maxHeight);

private:
	struct LogMessage
	{
		LogMessage(MessageSeverity severity, tstring message): severity(severity), message(message) {}
		MessageSeverity severity;
		tstring message;
	};

	void ErrorsToggled(bool value);
	void WarningsToggled(bool value);
	void InfoToggled(bool value);
	void DebugToggled(bool value);
	void RefillList();
	int GetImageIndex(MessageSeverity severity);

	Layout m_mainLayout;
	Layout m_toolbarLayout;
	ListView m_list;
	std::vector<ToggleButton*> m_buttons;
	std::vector<LogMessage> m_messages;

	unsigned m_filterFlags;
};

#endif //__LOGWINDOW_H__
