////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UIElementTools.h
//  Version:     v1.00
//  Created:     13/10/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////

#ifndef __UIElementTools_H__
#define __UIElementTools_H__

#include "UITools.h"
#include "UISimpleTreeScanner.h"
#include "UIElementPanels.h"

//////////////////////////////////////////////////////////////////////////
class CUIElementTool : public CUIBaseTool
{
public:
	CUIElementTool();
	~CUIElementTool();

	virtual void OpenTool();
	virtual void CloseTool();
	virtual void Update();

protected:
	virtual CUIRollupControl* GetRollupControl();

private:
	void OnElementSelected( SSimpleTreeBrowserItem* pItem );
	void OnElementDblClicked( SSimpleTreeBrowserItem* pItem );
	void OnElementFctCallback( SSimpleTreeBrowserItem* pItem );
	void OnElementVarCallback( SSimpleTreeBrowserItem* pItem );
	void OnElementArrayCallback( SSimpleTreeBrowserItem* pItem );
	void OnElementMCCallback( SSimpleTreeBrowserItem* pItem );

private:
	typedef std::map<SUIElementBaseScanner*, CSimpleTreeBrowser*> TRefreshMap;
	TRefreshMap m_browser;

	CUIElementPropertiesPanelProperties* m_pPanelProperies;
	CUIElementPropertiesPanelConstraints* m_pPanelConstraints;
	CUIElementPropertiesPanelFlags* m_pPanelFlags;

	SUIElementScanner m_elementsScanner;
	SUIElementEventScanner<SUIElementScannerHelperFunctions> m_fctScanner;
	SUIElementEventScanner<SUIElementScannerHelperEvents> m_eventScanner;
	SUIElementItemScanner<SUIElementScannerHelperVariables> m_varScanner;
	SUIElementItemScanner<SUIElementScannerHelperArrays> m_arrayScanner;
	SUIElementItemScanner<SUIElementScannerHelperMovieClips> m_mcScanner;
};

#endif // __UIElementTools_H__
