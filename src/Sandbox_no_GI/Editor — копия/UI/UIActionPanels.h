////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UIActionPanels.h
//  Version:     v1.00
//  Created:     11/10/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////
#ifndef __UIActionPanels__H_
#define __UIActionPanels__H_

#include "PropertiesPanel.h"
#include <IFlashUI.h>

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
class CUIActionPropertiesPanel : public CPropertiesPanel
{
public:
	CUIActionPropertiesPanel();

	void SetAction( IUIAction *pAction );

protected:
	void UpdateVars();
	void OnVarChange( IVariable *pVar );

	void AddVariables()	{	SetVarBlock( m_pVarBlock, functor(*this,&CUIActionPropertiesPanel::OnVarChangeInt) ); }

private:
	void OnVarChangeInt( IVariable *pVar ) { OnVarChange( pVar ); }

private:
	_smart_ptr<CVarBlock> m_pVarBlock;
	IUIAction* m_pAction;

	CSmartVariable<CString> mv_Name;
	CSmartVariable<CString> mv_File;
	CSmartVariable<bool> mv_enabled;
};

#endif //__UIActionPanels__H_