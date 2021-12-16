/************************************************************************/
/*                        Ragdoll Modifier Node                         */
/************************************************************************/
/* This modifier will cause the character to ragdoll. The game object   */
/* has to terminate this condition.																			*/
/************************************************************************/


#ifndef __ANIMATIONGRAPH2_RAGDOLL_H__
#define __ANIMATIONGRAPH2_RAGDOLL_H__

#pragma once

#include "../AnimationGraph2_Modifier.h"

class CAG2Modifier_Ragdoll : public CAG2ModifierBase
{
public:
	CAG2Modifier_Ragdoll();
	virtual ~CAG2Modifier_Ragdoll();


	// CAG2ModifierBase functions (see base class for documentation)
	//////////////////////////////////////////////////////////////////////////

	virtual const CString GetHumanReadableName() { return "Ragdoll"; };
	virtual const CString GetClassName() { return "Ragdoll"; };
	virtual const bool IsSingleton() const { return true; }

	virtual const void Save(XmlNodeRef modifierNode) const;
	virtual const void Load(XmlNodeRef modifierNode);
	virtual const void Export( XmlNodeRef node ) const;

	virtual CAG2ModifierBase* Duplicate() const;

	// Old Graph Version Conversion Stuff
	virtual bool CanConvertFromOldGraphVersion( const XmlNodeRef node ) const;
};


#endif // __ANIMATIONGRAPH2_RAGDOLL_H__

