/************************************************************************/
/*                         Event Modifier Node                          */
/************************************************************************/
/* This modifier makes a callback to the game upon entering and leaving */
/* the state and sends a string/event to the game object.               */
/************************************************************************/


#ifndef __ANIMATIONGRAPH2_EVENT_H__
#define __ANIMATIONGRAPH2_EVENT_H__

#pragma once

#include "../AnimationGraph2_Modifier.h"

class CAG2Modifier_Event : public CAG2ModifierBase
{
public:
	CAG2Modifier_Event();
	virtual ~CAG2Modifier_Event();


	// CAG2ModifierBase functions (see base class for documentation)
	//////////////////////////////////////////////////////////////////////////

	virtual CAG2ModifierBase* Duplicate() const;
	virtual const CString GetHumanReadableName() { return "Event"; };
	virtual const CString GetClassName() { return "Event"; };
	virtual const CString GetCustomText() const;
	
	virtual const void Save(XmlNodeRef modifierNode) const;
	virtual const void Load(XmlNodeRef modifierNode);
	virtual const void Export( XmlNodeRef node ) const;  

	virtual int GetDialogIDD() { return IDD_AG2_MODIFIER_EVENT; }
	virtual void InitParameterPanel();

	// Old Graph Version Conversion Stuff
	virtual bool CanConvertFromOldGraphVersion( const XmlNodeRef node ) const;
	virtual void ConvertFromOldStateNode( const XmlNodeRef node, CAGState2Ptr pAnimNode );

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnUserInput();

	//! This sets up the correspondences between local variables 
	//! and the interface items in the panel, so that the data
	//! that is written in the variables will be displayed.	
	virtual void DoDataExchange( CDataExchange* pDX );

private:

	//! Grays out the edit boxes when the corresponding checkbox is off
	void GrayOutInactiveElements();

	void Reset();

	CButton	m_useEventOnEnter_Ctrl;
	CButton	m_useEventOnExit_Ctrl;
	CButton	m_matchAnimation_Ctrl;
	CEdit	m_eventOnEnter_Ctrl;
	CEdit	m_eventOnExit_Ctrl;

	bool	m_useEventOnEnter;
	bool	m_useEventOnExit;
	bool	m_matchAnimation;
	CString	m_eventOnEnter;
	CString	m_eventOnExit;
};


#endif // __ANIMATIONGRAPH2_EVENT_H__

