/************************************************************************/
/*                   Facial Sequence Modifier Node                      */
/************************************************************************/
/* This modifier plays a facial sequence depending on the alertness     */
/* level of the character. It can randomly pick from a list, if more    */
/* than one sequence is specified for a given alterness.                */
/* Upon leaving the linked state, the facial sequence is stopped.       */
/************************************************************************/


#ifndef __ANIMATIONGRAPH2_FACIALSEQUENCE_H__
#define __ANIMATIONGRAPH2_FACIALSEQUENCE_H__

#pragma once

#include "../AnimationGraph2_Modifier.h"

class CAG2Modifier_FacialSequence : public CAG2ModifierBase
{
public:
	CAG2Modifier_FacialSequence();
	virtual ~CAG2Modifier_FacialSequence();


	// CAG2ModifierBase functions (see base class for documentation)
	//////////////////////////////////////////////////////////////////////////

	virtual CAG2ModifierBase* Duplicate() const;
	virtual const CString GetHumanReadableName() { return "Facial Sequence"; };
	virtual const CString GetClassName() { return "FacialSequence"; };
	virtual const CString GetCustomText() const;
	
	virtual const void Save(XmlNodeRef modifierNode) const;
	virtual const void Load(XmlNodeRef modifierNode);
	virtual const void Export( XmlNodeRef node ) const;  

	virtual int GetDialogIDD() { return IDD_AG2_MODIFIER_FACIAL_SEQUENCE; }
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

	//! Grays out some of the edit boxes when the corresponding checkBox is on
	void GrayOutInactiveElements();

	void Reset();

	CButton	m_useOneForAll_Ctrl;
	CEdit		m_oneForAllSequence_Ctrl;
	CEdit		m_idleSequence_Ctrl;
	CEdit		m_alertedSequence_Ctrl;
	CEdit		m_combatSequence_Ctrl;

	bool		m_useOneForAll;
	CString	m_oneForAllSequence;
	CString	m_idleSequence;
	CString	m_alertedSequence;
	CString	m_combatSequence;
};


#endif // __ANIMATIONGRAPH2_FACIALSEQUENCE_H__

