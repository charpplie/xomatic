////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UIEventSystemTools.h
//  Version:     v1.00
//  Created:     13/10/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////
#ifndef __UIEventSystemTools_H__
#define __UIEventSystemTools_H__


#include "UISimpleTreeScanner.h"
#include "UITools.h"
#include <IFlashUI.h>

//////////////////////////////////////////////////////////////////////////
class CUIEventSystemTool : public CUIBaseTool
{
public:
	CUIEventSystemTool(IUIEventSystem::EEventSystemType type);
	~CUIEventSystemTool();

	virtual void OpenTool();
	virtual void CloseTool();
	virtual void Update();

protected:
	virtual CUIRollupControl* GetRollupControl();

private:
	void OnEventSystemSelected( SSimpleTreeBrowserItem* pItem );
	void OnEventDblClick( SSimpleTreeBrowserItem* pItem );

private:
	CSimpleTreeBrowser* m_pEventBrowser;
	bool m_bCanInvokeEvents;

	SUIEventSystemScanner m_eventSystemScanner;
	SUIEventSystemEventsScanner m_eventsScanner;
};

#endif // __UIEventSystemTools_H__
