/************************************************************************/
/*                     Fall And Play Modifier Node                      */
/************************************************************************/
/* This modifier triggers a ragdoll event in the game that has the      */
/* character fall down and then standup again.                          */
/* The graph will not be able to leave the attached state until the     */
/* character has finished standing up.                                  */
/* Technically this node consists of the Event and a WaitForStandup     */
/************************************************************************/


#ifndef __ANIMATIONGRAPH2_FALLANDPLAY_H__
#define __ANIMATIONGRAPH2_FALLANDPLAY_H__

#pragma once

#include "../AnimationGraph2_Modifier.h"

class CAG2Modifier_FallAndPlay : public CAG2ModifierBase
{
public:
	CAG2Modifier_FallAndPlay();
	virtual ~CAG2Modifier_FallAndPlay();


	// CAG2ModifierBase functions (see base class for documentation)
	//////////////////////////////////////////////////////////////////////////

	virtual const CString GetHumanReadableName() { return "Fall And Play"; };
	virtual const CString GetClassName() { return "FallAndPlay"; };
	virtual const bool IsSingleton() const { return true; }
	
	virtual const void Save(XmlNodeRef modifierNode) const;
	virtual const void Load(XmlNodeRef modifierNode);
	virtual const void Export( XmlNodeRef node ) const;  

	virtual CAG2ModifierBase* Duplicate() const;

	// Old Graph Version Conversion Stuff
	virtual bool CanConvertFromOldGraphVersion( const XmlNodeRef node ) const;

};


#endif // __ANIMATIONGRAPH2_FALLANDPLAY_H__

