#pragma once

////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  Copyright (C), Crytek GmbH, 2011.
////////////////////////////////////////////////////////////////////////////

#ifndef NO_LIVECREATE

namespace LiveCreate
{

class CObjectSync
{
private:
	CEditorManager* m_pManager;

	struct SyncType
	{
		CTimeValue m_lastObjectSyncSentTime;
		bool m_bHasDirtyObjectsArea;
		AABB m_dirtyObjectArea;

		SyncType()
			: m_bHasDirtyObjectsArea(false)
		{
			m_dirtyObjectArea.Reset();
		}
	};

	SyncType m_types[eLiveCreateObjectType_MAX];

public:
	CObjectSync(CEditorManager* pManager);
	~CObjectSync();

	// Flush any pending changes to the clients
	void FlushChanges();

	// Add given area to the dirty region that should be resent to LiveCreate hosts
	void AddDirtyObjectArea(const AABB& area, ELiveCreateObjectType objectType);

};

}

#endif