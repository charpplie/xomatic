#ifndef LiveMocapConnection_h
#define LiveMocapConnection_h

#include "../../SDKs/LiveMocap/LiveMocap.h"
#include "LiveMocapScene.h"

class CLiveMocapConnection
{
public:
	static CLiveMocapConnection* Create(const char* name, const char* path);

private:
	CLiveMocapConnection();
	~CLiveMocapConnection();

public:
	void Release() { delete this; }

	const char* GetName() { return m_name; }

	CLiveMocapScene& GetScene() { return m_scene; }

	bool Connect(const char* address);
	void Disconnect();

	bool IsConnected();

	void Reset();

	void Update();

private:
	string m_name;
	HMODULE m_hModule;
	LMLiveMocap* m_pLiveMocap;

	CLiveMocapScene m_scene;

	bool m_bConnected; // TEMP

public:
	string m_lastConnection;
};

#endif // LiveMocapConnection_h
