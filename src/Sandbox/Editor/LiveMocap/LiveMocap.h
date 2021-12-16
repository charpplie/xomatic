#ifndef LiveMocap_h
#define LiveMocap_h

#include "../../SDKs/LiveMocap/LiveMocap.h"
#include "../LiveMocap/LiveMocapConnection.h"

class CLiveMocap :
	public IEditorNotifyListener
{
public:
	static CLiveMocap& Instance()
	{
		static CLiveMocap instance;

		// TEMP
		static bool bOnce = true;
		if (bOnce) bOnce = !instance.Initialize();

		return instance;
	}

private:
	CLiveMocap();
	~CLiveMocap();

public:
	bool Initialize();
	void Update();

	uint32 GetConnectionCount() { return (uint32)m_connections.size(); }
	CLiveMocapConnection* GetConnection(uint32 index) { return m_connections[index]; }
	void AddConnection(const char* name);

private:
	void CreateConnection(const char* name, const char* path);

	// IEditorNotifyListener
public:
	void OnEditorNotifyEvent(EEditorNotifyEvent event);

private:
	std::vector<CLiveMocapConnection*> m_connections;
};

#endif // LiveMocap_h
