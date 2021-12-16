/************************************************************************/
/*                Time Alignment Group Modifier Node                    */
/************************************************************************/
/* All Animations inside States linked to this state will be using the  */
/* Time Align Flag when transitioning in between. When transitioning to */
/* an Animation not inside this group, regular linear blending will be  */
/* used instead.                                                        */
/*                                                                      */
/* DO NOT USE THIS MODIFIER AS AN EXAMPLE; IT IS A SPECIAL CASE         */
/************************************************************************/


#ifndef __ANIMATIONGRAPH2_TIMEALGINGROUP_H__
#define __ANIMATIONGRAPH2_TIMEALGINGROUP_H__

#pragma once

#include "../AnimationGraph2_Modifier.h"

class CAG2Modifier_TimeAlignGroup : public CAG2ModifierBase
{
public:
	CAG2Modifier_TimeAlignGroup();
	virtual ~CAG2Modifier_TimeAlignGroup();

	// CAG2ModifierBase functions (see base class for documentation)
	//////////////////////////////////////////////////////////////////////////

	virtual void Init();

	virtual CAG2ModifierBase* Duplicate() const;
	virtual const CString GetHumanReadableName() { return "Time Alignment Group"; };
	virtual const CString GetClassName() { return "TimeAlignGroup"; };
	virtual const CString GetCustomText() const;

	virtual const void Save(XmlNodeRef modifierNode) const;
	virtual const void Load(XmlNodeRef modifierNode);
	virtual const void Export( XmlNodeRef node ) const {};  

	virtual int GetDialogIDD() { return IDD_AG2_MODIFIER_TIMEALIGNGROUP; }
	virtual void InitParameterPanel();

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnUserInput();

	virtual void DoDataExchange( CDataExchange* pDX );

private:

	void EnsureValidGroupName();

	CEdit	m_groupName_Ctrl;
	CString	m_groupName;
};


#endif // __ANIMATIONGRAPH2_TIMEALGINGROUP_H__

