////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
//
////////////////////////////////////////////////////////////////////////////

#ifndef __MannequinChangeMonitor_h__
#define __MannequinChangeMonitor_h__

//#include "Util/FileChangeMonitor.h"
#include "Include/IEditorFileMonitor.h"

class CMannequinChangeMonitor : public IFileChangeListener
{
public:
	CMannequinChangeMonitor();
	~CMannequinChangeMonitor();

	virtual void OnFileChange(const char* sFilename, EChangeType eType) override;	

	class CMannequinFileChangeWriter *m_pFileChangeWriter;

};

#endif //__MannequinChangeMonitor_h__
