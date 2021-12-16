#ifndef LiveCreate_h
#define LiveCreate_h

#include "../../SDKs/LiveCreate/LiveCreate.h"
#include "../LiveCreate/LiveCreateConnection.h"

class CLiveCreate :
	public IEditorNotifyListener
{
public:
	static CLiveCreate& Instance()
	{
		static CLiveCreate instance;

		// TEMP
		static bool bOnce = true;
		if (bOnce) bOnce = !instance.Initialize();

		return instance;
	}

private:
	CLiveCreate();
	~CLiveCreate();

public:
	bool Initialize();
	void Update();

	uint32 GetConnectionCount() { return (uint32)m_connections.size(); }
	CLiveCreateConnection* GetConnection(uint32 index) { return m_connections[index]; }

private:
	void CreateConnection(const char* name, const char* path);
	void RefreshConnections();

	// IEditorNotifyListener
public:
	void OnEditorNotifyEvent(EEditorNotifyEvent event);

private:
	std::vector<CLiveCreateConnection*> m_connections;
};

#endif // LiveCreate_h
