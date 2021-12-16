#include "StdAfx.h"
#include "MannequinChangeMonitor.h"
#include "helper/MannequinFileChangeWriter.h"

CMannequinChangeMonitor::CMannequinChangeMonitor()
	:
	m_pFileChangeWriter(new CMannequinFileChangeWriter(true))
{
	GetIEditor()->GetFileMonitor()->RegisterListener(this, "animations/mannequin","adb");
}

CMannequinChangeMonitor::~CMannequinChangeMonitor()
{
	SAFE_DELETE(m_pFileChangeWriter);
	GetIEditor()->GetFileMonitor()->UnregisterListener(this);
}

void CMannequinChangeMonitor::OnFileChange(const char* sFilename, EChangeType eType)
{
	CryLog("CMannequinChangeMonitor - %s has changed", sFilename);

	if (CMannequinFileChangeWriter::UpdateActiveWriter() == false)
	{
		m_pFileChangeWriter->ShowFileManager();
	}
}
