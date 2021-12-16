#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   EventPresetButtonSet.h
//  Created:     June/19/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "Controls/ButtonSetCtrl.h"
#include "CryCharAnimationParams.h"

class CModelViewportCE;

class IEventPresetButtonListener
{
public:

	virtual void OnPresetButtonClicked( ButtonItemPtr pButtonItem, const CAnimEventData* eventData ) = 0;
};

class CEventPresetButtonSet : public CButtonSetCtrl
{

public:

	CEventPresetButtonSet(){
		m_pButtonListener = NULL;
	}
		
	afx_msg void OnLButtonUp( UINT nFlags, CPoint point );
	afx_msg void OnRButtonDown( UINT nFlags, CPoint point );
	afx_msg int OnCreate( LPCREATESTRUCT lpCreateStruct );

	afx_msg void OnSize(UINT nType, int cx, int cy);


	void SetModelViewportCE( CModelViewportCE* pModelViewportCE )	{	m_pModelViewportCE = pModelViewportCE;	}

	void OnMouseEventFromButtons( EMouseEvent event, CButton* pButton, UINT nFlags, CPoint point ) override;
	void SetEventPresetButtonListener( IEventPresetButtonListener* pListener )	{		m_pButtonListener = pListener;	}
	void DeleteButton( const ButtonItemPtr& pButtonItem ) override;
	void RemoveAll() override;

	void SaveEventPresetsToRegistry( CXTRegistryManager& regMgr, const CString& strSection );	
	void SaveEventPresetsToFile( const CString& pathName );
	void LoadEventPresetsFromFile( const CString& pathName );

protected:

	void DisplayPopMenus( bool bHitButton );
	bool DoEventPresetModal( SButtonItem* pButtonItem, CAnimEventData* pAnimEventData );
	CAnimEventData* GetAnimEventData( ButtonItemPtr pButtonItem );
	
	void OnEventPresetNew();
	void OnEventPresetEdit();
	void OnEventPresetClone();
	void OnEventPresetDelete();
	void OnEventPresetSort();

	void ShowPresetEditUp( ButtonItemPtr pButtonItem, CAnimEventData* pAnimEventData );

	DECLARE_MESSAGE_MAP()

	void ReadEventPresetsToRegistry();

private:

	std::map<ButtonItemPtr,CAnimEventData> m_AnimEventDataMap;
	CModelViewportCE * m_pModelViewportCE;
	IEventPresetButtonListener* m_pButtonListener;
	CPoint m_PopupMenuPos;

};