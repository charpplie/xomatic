////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UIActionTools.h
//  Version:     v1.00
//  Created:     13/10/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////
#ifndef __UIActionTools_H__
#define __UIActionTools_H__

#include "UITools.h"
#include "UISimpleTreeScanner.h"
#include "UIActionPanels.h"

//////////////////////////////////////////////////////////////////////////
class CUIActionTool : public CUIBaseTool
{
public:
	CUIActionTool();
	~CUIActionTool();

	virtual void OpenTool();
	virtual void CloseTool();
	virtual void Update();

protected:
	virtual CUIRollupControl* GetRollupControl();

private:
	void OnActionSelected( SSimpleTreeBrowserItem* pItem );
	void OnActionDblClick( SSimpleTreeBrowserItem* pItem );

private:
	SUIActionScanner m_ActionScanner;
	CUIActionPropertiesPanel* m_pPropertiesPanel;
};

#endif // __UIActionTools_H__
