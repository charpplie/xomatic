#include "StdAfx.h"
#include "LogWindow.h"

LogWindow::LogWindow()
: m_mainLayout(Layout::DirectionVertical),
	m_toolbarLayout(Layout::DirectionHorizontal),
	m_filterFlags(0)
{
	m_buttons.push_back(new ToggleButton(_T("Debug"), this, &LogWindow::DebugToggled));
	m_buttons.push_back(new ToggleButton(_T("Info"), this, &LogWindow::InfoToggled));
	m_buttons.push_back(new ToggleButton(_T("Warnings"), this, &LogWindow::WarningsToggled));
	m_buttons.push_back(new ToggleButton(_T("Errors"), this, &LogWindow::ErrorsToggled));

	SetFilter(MessageSeverity_Error, true);
	SetFilter(MessageSeverity_Warning, true);
	SetFilter(MessageSeverity_Info, true);

	m_toolbarLayout.AddComponent(m_buttons[3]);
	m_toolbarLayout.AddComponent(m_buttons[2]);
	m_toolbarLayout.AddComponent(m_buttons[1]);
	m_toolbarLayout.AddComponent(m_buttons[0]);

	m_mainLayout.AddComponent(&m_toolbarLayout);
	m_mainLayout.AddComponent(&m_list);
}

LogWindow::~LogWindow()
{
	for (std::vector<ToggleButton*>::iterator button = m_buttons.begin(), end = m_buttons.end(); button != end; ++button)
		delete *button;
}

void LogWindow::Log(MessageSeverity severity, const TCHAR* message)
{
	m_messages.push_back(LogMessage(severity, message));

	if (m_filterFlags & (1 << severity))
	{
		m_list.Add(GetImageIndex(severity), message);
	}
}

void LogWindow::SetFilter(MessageSeverity severity, bool visible)
{
	m_filterFlags &= ~(1 << severity);
	m_filterFlags |= ((visible ? 1 : 0) << severity);
	m_buttons[severity]->SetState(visible);
	RefillList();
}

void LogWindow::CreateUI(void* window, int left, int top, int width, int height)
{
	m_mainLayout.CreateUI(window, left, top, width, height);
}

void LogWindow::Resize(void* window, int left, int top, int width, int height)
{
	m_mainLayout.Resize(window, left, top, width, height);
}

void LogWindow::DestroyUI(void* window)
{
	m_mainLayout.DestroyUI(window);
}

void LogWindow::GetExtremeDimensions(void* window, int& minWidth, int& maxWidth, int& minHeight, int& maxHeight)
{
	m_mainLayout.GetExtremeDimensions(window, minWidth, maxWidth, minHeight, maxHeight);
}

void LogWindow::ErrorsToggled(bool value)
{
	m_filterFlags &= ~(1 << MessageSeverity_Error);
	m_filterFlags |= ((value ? 1 : 0) << MessageSeverity_Error);
	RefillList();
}

void LogWindow::WarningsToggled(bool value)
{
	m_filterFlags &= ~(1 << MessageSeverity_Warning);
	m_filterFlags |= ((value ? 1 : 0) << MessageSeverity_Warning);
	RefillList();
}

void LogWindow::InfoToggled(bool value)
{
	m_filterFlags &= ~(1 << MessageSeverity_Info);
	m_filterFlags |= ((value ? 1 : 0) << MessageSeverity_Info);
	RefillList();
}

void LogWindow::DebugToggled(bool value)
{
	m_filterFlags &= ~(1 << MessageSeverity_Debug);
	m_filterFlags |= ((value ? 1 : 0) << MessageSeverity_Debug);
	RefillList();
}

void LogWindow::RefillList()
{
	m_list.Clear();
	for (int messageIndex = 0, messageCount = int(m_messages.size()); messageIndex < messageCount; ++messageIndex)
	{
		if (m_filterFlags & (1 << m_messages[messageIndex].severity))
			m_list.Add(GetImageIndex(m_messages[messageIndex].severity), m_messages[messageIndex].message.c_str());
	}
}

int LogWindow::GetImageIndex(MessageSeverity severity)
{
	int imageIndex = 0;
	switch (severity)
	{
	case MessageSeverity_Debug: imageIndex = 2; break;
	case MessageSeverity_Info: imageIndex = -1; break;
	case MessageSeverity_Warning: imageIndex = 1; break;
	case MessageSeverity_Error: imageIndex = 0; break;
	}
	return imageIndex;
}
