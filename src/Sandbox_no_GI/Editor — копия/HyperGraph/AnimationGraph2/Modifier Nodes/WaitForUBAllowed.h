/************************************************************************/
/*             Wait for UpperBody Allowed Modifier Node                 */
/************************************************************************/
/* This modifier node will keep the graph from leaving the linked state */
/* until the fullbody animation graph is in a state that allows         */
/* upper body animation.                                                */
/************************************************************************/


#ifndef __ANIMATIONGRAPH2_WAITFORUBALLOWED_H__
#define __ANIMATIONGRAPH2_WAITFORUBALLOWED_H__

#pragma once

#include "../AnimationGraph2_Modifier.h"

class CAG2Modifier_WaitForUBAllowed : public CAG2ModifierBase
{
public:
	CAG2Modifier_WaitForUBAllowed();
	virtual ~CAG2Modifier_WaitForUBAllowed();


	// CAG2ModifierBase functions (see base class for documentation)
	//////////////////////////////////////////////////////////////////////////

	virtual const CString GetHumanReadableName() { return "Wait for UpperBody Allowed"; };
	virtual const CString GetClassName() { return "WaitForUBAllowed"; };
	virtual const bool IsSingleton() const { return true; }
	
	virtual const void Save(XmlNodeRef modifierNode) const;
	virtual const void Load(XmlNodeRef modifierNode);
	virtual const void Export( XmlNodeRef node ) const;  

	virtual CAG2ModifierBase* Duplicate() const;

	// Old Graph Version Conversion Stuff
	virtual bool CanConvertFromOldGraphVersion( const XmlNodeRef node ) const;

};


#endif // __ANIMATIONGRAPH2_WAITFORUBALLOWED_H__

