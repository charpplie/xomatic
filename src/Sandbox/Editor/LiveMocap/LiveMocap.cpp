#include "stdafx.h"
#include "LiveMocap.h"

/*

  CLiveMocap

*/

CLiveMocap::CLiveMocap()
{
	::GetIEditor()->RegisterNotifyListener(this);
}

CLiveMocap::~CLiveMocap()
{
	::GetIEditor()->UnregisterNotifyListener(this);
	while (!m_connections.empty())
	{
		m_connections.back()->Release();
		m_connections.pop_back();
	}
}

//

bool CLiveMocap::Initialize()
{
	return true;
}

void CLiveMocap::Update()
{
	size_t connectionCount = m_connections.size();
	for (size_t i=0; i<connectionCount; ++i)
	{
		if (!m_connections[i]->IsConnected())
			continue;

		m_connections[i]->Update();
	}
}

void CLiveMocap::CreateConnection(const char* name, const char* path)
{
	CLiveMocapConnection* pConnection =
		CLiveMocapConnection::Create(name, path);
	if (!pConnection)
		return;

	m_connections.push_back(pConnection);
}

void CLiveMocap::AddConnection(const char* name)
{
	string connectionName;
	connectionName.Format("%s_%i", name, GetConnectionCount());
	string pathToDll;

#ifdef WIN64
	pathToDll.Format("Bin64/LiveMocap/%s.dll", name);
#else
	pathToDll.Format("Bin32/LiveMocap/%s.dll", name);
#endif

	CreateConnection(connectionName.c_str(), pathToDll.c_str());
}

// IEditorNotifyListener

void CLiveMocap::OnEditorNotifyEvent(EEditorNotifyEvent event)
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

