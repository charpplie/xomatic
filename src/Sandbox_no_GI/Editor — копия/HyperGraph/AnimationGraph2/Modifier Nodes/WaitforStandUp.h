/************************************************************************/
/*                   Wait for StandUp Modifier Node                     */
/************************************************************************/
/* This modifier node will keep the graph from leaving the linked state */
/* until the low level animation code has finished standing up this     */
/* this character after a RagDoll/fallAndplay triggered in code.        */
/* It is not needed when the FallAndPlay Modifier is already linked,    */
/* because that one includes this one.                                  */
/************************************************************************/


#ifndef __ANIMATIONGRAPH2_WAITFORSTANDUP_H__
#define __ANIMATIONGRAPH2_WAITFORSTANDUP_H__

#pragma once

#include "../AnimationGraph2_Modifier.h"

class CAG2Modifier_WaitForStandUp : public CAG2ModifierBase
{
public:
	CAG2Modifier_WaitForStandUp();
	virtual ~CAG2Modifier_WaitForStandUp();


	// CAG2ModifierBase functions (see base class for documentation)
	//////////////////////////////////////////////////////////////////////////

	virtual const CString GetHumanReadableName() { return "Wait for StandUp"; };
	virtual const CString GetClassName() { return "WaitForStandUp"; };
	virtual const bool IsSingleton() const { return true; }
	
	virtual const void Save(XmlNodeRef modifierNode) const;
	virtual const void Load(XmlNodeRef modifierNode);
	virtual const void Export( XmlNodeRef node ) const;  

	virtual CAG2ModifierBase* Duplicate() const;

	// Old Graph Version Conversion Stuff
	virtual bool CanConvertFromOldGraphVersion( const XmlNodeRef node ) const;

};


#endif // __ANIMATIONGRAPH2_WAITFORSTANDUP_H__

