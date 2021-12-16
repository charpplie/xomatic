/************************************************************************/
/*                      Play Sound Modifier Node                        */
/************************************************************************/
/* This modifier plays a sound after a delay when entering the          */
/* associated state. If everything is set up correctly, lip sync can    */
/* optionally be switched on.                                           */
/* By default, the sound is started as a 3D Sound, but custom flags     */
/* can optionally be specified (see ISound.h).                          */
/************************************************************************/


#ifndef __ANIMATIONGRAPH2_PLAYSOUND_H__
#define __ANIMATIONGRAPH2_PLAYSOUND_H__

#pragma once

#include "../AnimationGraph2_Modifier.h"

class CAG2Modifier_PlaySound : public CAG2ModifierBase
{
public:
	CAG2Modifier_PlaySound();
	virtual ~CAG2Modifier_PlaySound();


	// CAG2ModifierBase functions (see base class for documentation)
	//////////////////////////////////////////////////////////////////////////

	virtual CAG2ModifierBase* Duplicate() const;
	virtual const CString GetHumanReadableName() { return "Play Sound"; };
	virtual const CString GetClassName() { return "PlaySound"; };
	virtual const CString GetCustomText() const;
	
	virtual const void Save(XmlNodeRef modifierNode) const;
	virtual const void Load(XmlNodeRef modifierNode);
	virtual const void Export( XmlNodeRef node ) const;  

	virtual int GetDialogIDD() { return IDD_AG2_MODIFIER_PLAYSOUND; }
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

	CButton	m_voice_Ctrl;
	CEdit		m_flags_Ctrl;
	CEdit		m_delay_Ctrl;
	CEdit		m_soundName_Ctrl;

	bool		m_voice;
	int			m_flags;
	float		m_delay;
	CString	m_soundName;
};


#endif // __ANIMATIONGRAPH2_PLAYSOUND_H__

