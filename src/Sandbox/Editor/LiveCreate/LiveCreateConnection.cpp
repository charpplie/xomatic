#include "stdafx.h"
#include "LiveCreateConnection.h"

class CLiveCreateSystem :
	public ILCSystem
{
	// ILCSystem
public:
	void Log(LCString message, ...)
	{
		char buffer[2048];

		va_list arguments;
		va_start(arguments, message);
		::vsnprintf(buffer, sizeof(buffer), message, arguments);
		va_end(arguments);

		::gEnv->pLog->Log(buffer);
	}
} liveCreateSystem;

/*

  CLiveCreateConnection

*/

CLiveCreateConnection *CLiveCreateConnection::Create(const char* name, const char* path)
{
	HMODULE hModule = ::LoadLibrary(path);
	if (!hModule)
		return NULL;

	FLiveCreate LiveCreate = (FLiveCreate)
		::GetProcAddress(hModule, "LiveCreate");
	if (!LiveCreate)
	{
		::FreeLibrary(hModule);
		return NULL;
	}

	LCLiveCreate* pLiveCreate = LiveCreate(&::liveCreateSystem);
	if (!pLiveCreate)
	{
		::FreeLibrary(hModule);
		return NULL;
	}

	CLiveCreateConnection* pConnection = new CLiveCreateConnection();
	pConnection->m_name = name;
	pConnection->m_hModule = hModule;
	pConnection->m_pLiveCreate = pLiveCreate;
	return pConnection;
}

//

CLiveCreateConnection::CLiveCreateConnection()
{
	m_hModule = NULL;
	m_pLiveCreate = NULL;

	m_bConnected = false;
}

CLiveCreateConnection::~CLiveCreateConnection()
{
	if (m_pLiveCreate)
		m_pLiveCreate->Release();

	if (m_hModule)
		::FreeLibrary(m_hModule);
}

//

bool CLiveCreateConnection::Connect(const char* address)
{
	if (IsConnected())
		return true;

	return m_bConnected = m_pLiveCreate->Connect(address, m_scene);
}

void CLiveCreateConnection::Disconnect()
{
	if (!IsConnected())
		return;

	m_pLiveCreate->Disconnect();
	IEntity* pEntity = m_scene.GetEntity();
	m_scene.Reset();
	m_scene.SetEntity(pEntity);
	m_bConnected = false;
}

bool CLiveCreateConnection::IsConnected()
{
	return m_bConnected;
}

void CLiveCreateConnection::Reset()
{
	m_scene.Reset();
}

void CLiveCreateConnection::Update()
{
	m_pLiveCreate->Update();
	m_scene.Update();
}
