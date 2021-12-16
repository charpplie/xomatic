// (c) 2001-2012 Crytek GmbH
#pragma once
#include "Include/IEditorFileMonitor.h"
#include "IFileChangeMonitor.h"
#include "Util/FileChangeMonitor.h"

class CEditorFileMonitor
: public IEditorFileMonitor
, public CFileChangeMonitorListener
, public IEditorNotifyListener
{
public:
	CEditorFileMonitor();
	~CEditorFileMonitor();

	bool RegisterListener(IFileChangeListener *pListener, const char* filename) override;
	bool RegisterListener(IFileChangeListener *pListener, const char* folderRelativeToGame, const char* ext) override;
	bool UnregisterListener(IFileChangeListener *pListener) override;

	// from CFileChangeMonitorListener
	void OnFileMonitorChange(const SFileChangeInfo& rChange) override;

	// from IEditorNotifyListener
	void OnEditorNotifyEvent(EEditorNotifyEvent ev) override;
private:

	void MonitorDirectories();
	// File Change Monitor stuff
	struct SFileChangeCallback
	{
		IFileChangeListener *pListener;
		CString	item;
		CString	extension;

		SFileChangeCallback()
			: pListener( NULL )
		{}
		
		SFileChangeCallback(IFileChangeListener* pListener, const char* item, const char* extension)
			: pListener(pListener)
			, item( item )
			, extension( extension )
		{}
	};

	std::vector<SFileChangeCallback> m_vecFileChangeCallbacks;
};
