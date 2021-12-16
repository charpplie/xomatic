/************************************************************************/
/*                       SetInput Modifier Node                         */
/************************************************************************/
/* This modifier can change an input to the animation graph and set it  */
/* to a new value - and optionally restore the original value after the */
/* state or the node is left                                            */
/************************************************************************/


#ifndef __ANIMATIONGRAPH2_SETINPUT_H__
#define __ANIMATIONGRAPH2_SETINPUT_H__

#pragma once

#include "../AnimationGraph2_Modifier.h"

class CAG2Modifier_SetInput : public CAG2ModifierBase
{
public:
	CAG2Modifier_SetInput();
	virtual ~CAG2Modifier_SetInput();


	// CAG2ModifierBase functions (see base class for documentation)
	//////////////////////////////////////////////////////////////////////////

	virtual CAG2ModifierBase* Duplicate() const;
	virtual const CString GetHumanReadableName() { return "SetInput"; };
	virtual const CString GetClassName() { return "SetInput"; };
	virtual const CString GetCustomText() const;
	
	virtual const void Save(XmlNodeRef modifierNode) const;
	virtual const void Load(XmlNodeRef modifierNode);
	virtual const void Export( XmlNodeRef node ) const;  

	virtual int GetDialogIDD() { return IDD_AG2_MODIFIER_SETINPUT; }
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

	void Reset();


	// This can access inputs defined in other graph layers as well, 
	// so do NOT optimize this to be a dropdown list with all the inputs
	// in this graph only 
	// (Example: UpperBody Graph setting inputs that only exist in FullBody)
	CEdit		m_inputName_Ctrl; 
	CEdit		m_timer_Ctrl;
	CEdit		m_newValue_Ctrl;
	CButton	m_restoreOnLeave_Ctrl;

	bool		m_restoreOnLeave;
	float		m_timer;
	CString	m_newValue;
	CString	m_inputName;
};


#endif // __ANIMATIONGRAPH2_SETINPUT_H__

