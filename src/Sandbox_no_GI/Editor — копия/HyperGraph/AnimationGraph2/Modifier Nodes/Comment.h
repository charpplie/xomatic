/************************************************************************/
/*                       Comment Modifier Node                          */
/************************************************************************/
/* This modifier represents a comment inside the graph, it has no       */
/* actual functionality besides giving information.                     */
/************************************************************************/


#ifndef __ANIMATIONGRAPH2_COMMENT_H__
#define __ANIMATIONGRAPH2_COMMENT_H__

#pragma once

#include "../AnimationGraph2_Modifier.h"

class CAG2Modifier_Comment : public CAG2ModifierBase
{
public:
	CAG2Modifier_Comment();
	virtual ~CAG2Modifier_Comment();


	// CAG2ModifierBase functions (see base class for documentation)
	//////////////////////////////////////////////////////////////////////////

	virtual CAG2ModifierBase* Duplicate() const;
	virtual const CString GetHumanReadableName() { return "Comment Box"; };
	virtual const CString GetClassName() { return "Comment"; };
	const CString CAG2Modifier_Comment::GetCustomText() const;

	virtual const void Save(XmlNodeRef modifierNode) const;
	virtual const void Load(XmlNodeRef modifierNode);
	virtual const void Export( XmlNodeRef node ) const {};  

	virtual int GetDialogIDD() { return IDD_AG2_MODIFIER_COMMENT; }
	virtual void InitParameterPanel();

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnUserInput();

	virtual void DoDataExchange( CDataExchange* pDX );

private:

	CEdit	m_noteText_Ctrl;
	CString	m_noteText;
};


#endif // __ANIMATIONGRAPH2_COMMENT_H__

