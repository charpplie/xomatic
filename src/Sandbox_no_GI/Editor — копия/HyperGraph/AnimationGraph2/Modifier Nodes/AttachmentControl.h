/************************************************************************/
/*                  Attachment Effect Modifier Node                     */
/************************************************************************/
/* This modifier node plays a particle effect on an attachment.         */
/************************************************************************/


#ifndef __ANIMATIONGRAPH2_ATTACHMENTCONTROL_H__
#define __ANIMATIONGRAPH2_ATTACHMENTCONTROL_H__

#pragma once

#include "../AnimationGraph2_Modifier.h"

class CAG2Modifier_AttachmentControl : public CAG2ModifierBase
{
public:
	CAG2Modifier_AttachmentControl();
	virtual ~CAG2Modifier_AttachmentControl();


	// CAG2ModifierBase functions (see base class for documentation)
	//////////////////////////////////////////////////////////////////////////

	virtual CAG2ModifierBase* Duplicate() const;
	virtual const CString GetHumanReadableName() { return "Attachment Control"; };
	virtual const CString GetClassName() { return "AttachmentControl"; };
	virtual const CString GetCustomText() const;

	virtual const void Save(XmlNodeRef modifierNode) const;
	virtual const void Load(XmlNodeRef modifierNode);
	virtual const void Export( XmlNodeRef node ) const;  

	virtual int GetDialogIDD() { return IDD_AG2_MODIFIER_ATTACHMENTCONTROL; }
	virtual void InitParameterPanel();

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnUserInput();
	afx_msg void GrayOutInactiveItems();

	//! This sets up the correspondences between local variables 
	//! and the interface items in the panel, so that the data
	//! that is written in the variables will be displayed.	
	virtual void DoDataExchange( CDataExchange* pDX );

private:

	void Reset();

private:

	enum EAttachmentControl_Type 
	{
		ePlayAnimation = 0,
		eStopAnimation,
		eShowAttachment,
		eHideAttachment,
	};

	//////////////////////////////////////////////////////////////////////////
	// Windows Controls

	CEdit		m_attachmentName_Ctrl;

	CButton	m_animation_Ctrl;
	CButton m_stopAnimation_Ctrl;
	CEdit		m_animationName_Ctrl;
	CButton	m_animationLoop_Ctrl;
	CEdit		m_animationLayer_Ctrl;
	CEdit		m_animationStopLayer_Ctrl;
	CEdit		m_animationStopTime_Ctrl;


	CButton	m_show_Ctrl;
	CButton	m_hide_Ctrl;

	//////////////////////////////////////////////////////////////////////////
	// Internal Variables

	CString	m_attachmentName;
	int			m_controlType; // EAttachmentControl_Type

	CString	m_animationName;
	bool		m_animationLoop;
	int			m_animationLayer;

	int			m_animStopLayer;
	float		m_animStopTime;


};


#endif // __ANIMATIONGRAPH2_ATTACHMENTCONTROL_H__

