#include "stdafx.h"
#include "LiveCreate.h"

/*

  CLiveCreate

*/

CLiveCreate::CLiveCreate()
{
	::GetIEditor()->RegisterNotifyListener(this);
}

CLiveCreate::~CLiveCreate()
{
	while (!m_connections.empty())
	{
		m_connections.back()->Release();
		m_connections.pop_back();
	}
}

//

bool CLiveCreate::Initialize()
{
	RefreshConnections();

	return true;
}

void CLiveCreate::Update()
{
	size_t connectionCount = m_connections.size();
	for (size_t i=0; i<connectionCount; ++i)
	{
		if (!m_connections[i]->IsConnected())
			continue;

		m_connections[i]->Update();
	}
}

void CLiveCreate::CreateConnection(const char* name, const char* path)
{
	CLiveCreateConnection* pConnection =
		CLiveCreateConnection::Create(name, path);
	if (!pConnection)
		return;

	m_connections.push_back(pConnection);
}

void CLiveCreate::RefreshConnections()
{
	// TEMP
	if (!m_connections.empty())
		return;

#ifdef WIN64
	CreateConnection("Vicon_1", "../Code/Tools/LiveCreate/Release/Vicon64.dll");
	CreateConnection("Vicon_2", "../Code/Tools/LiveCreate/Release/Vicon64.dll");
#else
	CreateConnection("Vicon_1", "../Code/Tools/LiveCreate/Release/Vicon.dll");
	CreateConnection("Vicon_2", "../Code/Tools/LiveCreate/Release/Vicon.dll");
#endif
}

// IEditorNotifyListener

void CLiveCreate::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
	if (event == eNotify_OnIdleUpdate)
	{
		Update();
		return;
	}

	if (event == eNotify_OnCloseScene)
	{
		size_t connectionCount = m_connections.size();
		for (size_t i=0; i<connectionCount; ++i)
			m_connections[i]->Reset();
	}

}
