#ifndef LiveCreateConnection_h
#define LiveCreateConnection_h

#include "../../SDKs/LiveCreate/LiveCreate.h"
#include "LiveCreateScene.h"

class CLiveCreateConnection
{
public:
	static CLiveCreateConnection* Create(const char* name, const char* path);

private:
	CLiveCreateConnection();
	~CLiveCreateConnection();

public:
	void Release() { delete this; }

	const char* GetName() { return m_name; }

	CLiveCreateScene& GetScene() { return m_scene; }

	bool Connect(const char* address);
	void Disconnect();

	bool IsConnected();

	void Reset();

	void Update();

private:
	string m_name;
	HMODULE m_hModule;
	LCLiveCreate* m_pLiveCreate;

	CLiveCreateScene m_scene;

	bool m_bConnected; // TEMP
};

#endif // LiveCreateConnection_h
