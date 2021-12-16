////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2011.
// -------------------------------------------------------------------------
//  File name:   UIEmulatePanel.h
//  Version:     v1.00
//  Created:     11/10/2011 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//  History:
////////////////////////////////////////////////////////////////////////////
#ifndef __UIEmulatePanel__H_
#define __UIEmulatePanel__H_

#include "PropertiesPanel.h"
#include <IFlashUI.h>

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
class CUIEmulatePanel 
	: public CPropertiesPanel
	, public IEditorNotifyListener
{
public:
	CUIEmulatePanel(const char* invokeBtnCaption, bool bRemoveable = true);
	virtual ~CUIEmulatePanel();

	virtual void* GetData() const = 0;
	virtual void Refresh() = 0;
	bool IsRemoveAble() const { return m_bRemoveable; }

	// CPropertiesPanel
	virtual void ResizeToFitProperties();

	// IEditorNotifyListener
	virtual void OnEditorNotifyEvent( EEditorNotifyEvent event );

protected:
	virtual void CallEvent( const SUIArguments& args ) = 0;
	virtual void InitVars() = 0;
	virtual void OnVarChange( IVariable *pVar ) = 0;

	inline CVarBlock* GetVarBlock() { return m_pVarBlock; }

	inline void ClearArgVars() { m_ArgVars.clear(); }
	void SetArgVarsCount( int count );
	inline int GetArgVarsCount() const { return m_ArgVars.size(); }
	inline CSmartVariable<CString>& GetArgVar(int index) { return m_ArgVars[index].first; }
	inline void SetArgVarDyn(int index) { m_ArgVars[index].second = true; }

	inline void Invalidate() { m_bInvalidated = true; }
	void RemoveThis();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnBtnInvoke();
	afx_msg void OnBtnRemove();

private:
	inline void AddVariables();
	virtual void OnVarChangeInt( IVariable *pVar );

private:
	CXTPButton* m_pInvokeButton;
	CXTPButton* m_pRemoveButton;
	bool m_bRemoveable;
	bool m_bInvalidated;
	const char* m_invokeBtnCaption;

	_smart_ptr<CVarBlock> m_pVarBlock;
	typedef std::vector< std::pair<CSmartVariable<CString>, bool> > TArgVars;
	TArgVars m_ArgVars;
};

#endif //__UIEmulatePanel__H_