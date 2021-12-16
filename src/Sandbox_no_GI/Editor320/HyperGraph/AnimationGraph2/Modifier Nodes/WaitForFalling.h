/************************************************************************/
/*              Wait for Falling Finished Modifier Node                 */
/************************************************************************/
/* This modifier node will keep the graph from leaving the linked state */
/* until the character has made contact with the terrain again.         */
/************************************************************************/


#ifndef __ANIMATIONGRAPH2_WAITFORFALLING_H__
#define __ANIMATIONGRAPH2_WAITFORFALLING_H__

#pragma once

#include "../AnimationGraph2_Modifier.h"

class CAG2Modifier_WaitForFalling : public CAG2ModifierBase
{
public:
	CAG2Modifier_WaitForFalling();
	virtual ~CAG2Modifier_WaitForFalling();


	// CAG2ModifierBase functions (see base class for documentation)
	//////////////////////////////////////////////////////////////////////////

	virtual const CString GetHumanReadableName() { return "Wait for Falling"; };
	virtual const CString GetClassName() { return "WaitForFalling"; };
	virtual const bool IsSingleton() const { return true; }
	
	virtual const void Save(XmlNodeRef modifierNode) const;
	virtual const void Load(XmlNodeRef modifierNode);
	virtual const void Export( XmlNodeRef node ) const;  

	virtual CAG2ModifierBase* Duplicate() const;

	// Old Graph Version Conversion Stuff
	virtual bool CanConvertFromOldGraphVersion( const XmlNodeRef node ) const;

};


#endif // __ANIMATIONGRAPH2_WAITFORFALLING_H__

