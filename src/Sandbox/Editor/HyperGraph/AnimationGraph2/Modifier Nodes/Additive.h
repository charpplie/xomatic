/************************************************************************/
/*                       Additive Modifier Node                         */
/************************************************************************/
/* This modifier allows playing an animation in a higher layer, usually */
/* additives. The asset will be started upon entering a state. It can   */
/* can either play through once or loop. If desired, the animation can  */
/* can be stopped when the state is left, or keep playing.              */
/* This modifier can also be used to stop animation in a certain layer. */
/* The Animation Name may contain Variation Inputs                      */
/************************************************************************/


#ifndef __ANIMATIONGRAPH2_ADDITIVE_H__
#define __ANIMATIONGRAPH2_ADDITIVE_H__

#pragma once

#include "../AnimationGraph2_Modifier.h"

class CAG2Modifier_Additive : public CAG2ModifierBase
{
public:
	CAG2Modifier_Additive();
	virtual ~CAG2Modifier_Additive();


	// CAG2ModifierBase functions (see base class for documentation)
	//////////////////////////////////////////////////////////////////////////

	virtual CAG2ModifierBase* Duplicate() const;
	virtual const CString GetHumanReadableName() { return "Additive"; };
	virtual const CString GetClassName() { return "Additive"; };
	virtual const CString GetCustomText() const;
	
	virtual const void Save(XmlNodeRef modifierNode) const;
	virtual const void Load(XmlNodeRef modifierNode);
	virtual const void Export( XmlNodeRef node ) const;  

	virtual int GetDialogIDD() { return IDD_AG2_MODIFIER_ADDITIVE; }
	virtual void InitParameterPanel();

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnUserInput();

	//! This sets up the correspondences between local variables 
	//! and the interface items in the panel, so that the data
	//! that is written in the variables will be displayed.	
	virtual void DoDataExchange( CDataExchange* pDX );

private:

	enum EPlayModes
	{
		PLAY_ONCE = 0,
		LOOP_ASSET = 1,
		STOP_LAYER = 2,
	};

	//! Grays out the edit boxes when the corresponding checkbox is off
	void GrayOutInactiveElements();

	void Reset();

	CButton	m_playMode_PlayOnce_Ctrl;
	CButton	m_playMode_Loop_Ctrl;
	CButton	m_playMode_StopLayer_Ctrl;
	CButton m_fadeOutOneLeaving_Ctrl;
	CEdit		m_layer_Ctrl;
	CEdit		m_fadeOutTime_Ctrl;
	CEdit		m_blendInTime_Ctrl;
	CEdit		m_assetWeight_Ctrl;
	CEdit		m_assetSpeed_Ctrl;
	CEdit		m_assetName_Ctrl;

	EPlayModes	m_playMode;
	bool				m_fadeOutOneLeaving;
	int					m_layer;
	float				m_fadeOutTime;
	float				m_blendInTime;
	float				m_assetWeight;
	float				m_assetSpeed;
	CString			m_assetName;
};


#endif // __ANIMATIONGRAPH2_ADDITIVE_H__

