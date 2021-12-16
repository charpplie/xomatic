/************************************************************************/
/*                  Attachment Effect Modifier Node                     */
/************************************************************************/
/* This modifier node plays a particle effect on an attachment.         */
/************************************************************************/


#ifndef __ANIMATIONGRAPH2_ATTACHMENTEFFECT_H__
#define __ANIMATIONGRAPH2_ATTACHMENTEFFECT_H__

#pragma once

#include "../AnimationGraph2_Modifier.h"

class CAG2Modifier_AttachmentEffect : public CAG2ModifierBase
{
public:
	CAG2Modifier_AttachmentEffect();
	virtual ~CAG2Modifier_AttachmentEffect();


	// CAG2ModifierBase functions (see base class for documentation)
	//////////////////////////////////////////////////////////////////////////

	virtual CAG2ModifierBase* Duplicate() const;
	virtual const CString GetHumanReadableName() { return "Attachment Effect"; };
	virtual const CString GetClassName() { return "AttachmentEffect"; };
	virtual const CString GetCustomText() const;

	virtual const void Save(XmlNodeRef modifierNode) const;
	virtual const void Load(XmlNodeRef modifierNode);
	virtual const void Export( XmlNodeRef node ) const;  

	virtual int GetDialogIDD() { return IDD_AG2_MODIFIER_ATTACHMENTEFFECT; }
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

	CEdit		m_countOffset_Ctrl;
	CEdit		m_countMult_Ctrl;
	CEdit		m_countInput_Ctrl;
	CEdit		m_sizeOffset_Ctrl;
	CEdit		m_sizeMult_Ctrl;
	CEdit		m_sizeInput_Ctrl;
	CEdit		m_attachmentName_Ctrl;
	CEdit		m_particleName_Ctrl;
	CEdit		m_posX_Ctrl;
	CEdit		m_posY_Ctrl;
	CEdit		m_posZ_Ctrl;

	float		m_countOffset;
	float		m_countMult;
	float		m_sizeOffset;
	float		m_sizeMult;
	CString	m_attachmentName;
	CString	m_particleName;
	CString	m_countInput;
	CString	m_sizeInput;
	Vec3		m_pos;
};


#endif // __ANIMATIONGRAPH2_ATTACHMENTEFFECT_H__

