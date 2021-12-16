/************************************************************************/
/*                        Output Modifier Node                          */
/************************************************************************/
/* This modifier creates a named variable and sets it to a string       */
/* value which can be requested by the game code.                       */
/************************************************************************/


#ifndef __ANIMATIONGRAPH2_OUTPUT_H__
#define __ANIMATIONGRAPH2_OUTPUT_H__

#pragma once

#include "../AnimationGraph2_Modifier.h"

class CAG2Modifier_Output : public CAG2ModifierBase
{
public:
	CAG2Modifier_Output();
	virtual ~CAG2Modifier_Output();


	// CAG2ModifierBase functions (see base class for documentation)
	//////////////////////////////////////////////////////////////////////////

	virtual CAG2ModifierBase* Duplicate() const;
	virtual const CString GetHumanReadableName() { return "Output"; };
	virtual const CString GetClassName() { return "Output"; };
	virtual const CString GetCustomText() const;
	
	virtual const void Save(XmlNodeRef modifierNode) const;
	virtual const void Load(XmlNodeRef modifierNode);
	virtual const void Export( XmlNodeRef node ) const;  

	virtual int GetDialogIDD() { return IDD_AG2_MODIFIER_OUTPUT; }
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

	CEdit	m_outputName_Ctrl;
	CEdit	m_outputValue_Ctrl;

	CString	m_outputName;
	CString	m_outputValue;
};


#endif // __ANIMATIONGRAPH2_OUTPUT_H__

