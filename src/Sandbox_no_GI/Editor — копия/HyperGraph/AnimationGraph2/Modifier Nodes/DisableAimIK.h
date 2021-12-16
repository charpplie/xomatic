/************************************************************************/
/*                       Disable AimIK Modifier Node                   */
/************************************************************************/


#ifndef __ANIMATIONGRAPH2_DISABLEAIMIK_H__
#define __ANIMATIONGRAPH2_DISABLEAIMIK_H__

#pragma once

#include "../AnimationGraph2_Modifier.h"

class CAG2Modifier_DisableAimIK : public CAG2ModifierBase
{
public:
	CAG2Modifier_DisableAimIK();
	virtual ~CAG2Modifier_DisableAimIK();


	// CAG2ModifierBase functions (see base class for documentation)
	//////////////////////////////////////////////////////////////////////////

	virtual const CString GetHumanReadableName() { return "Disable AimIK"; };
	virtual const CString GetClassName() { return "DisableAimIK"; };
	virtual const bool IsSingleton() const { return true; }

	virtual const void Save(XmlNodeRef modifierNode) const;
	virtual const void Load(XmlNodeRef modifierNode);
	virtual const void Export( XmlNodeRef node ) const;

	virtual CAG2ModifierBase* Duplicate() const;
};


#endif // __ANIMATIONGRAPH2_DISABLEAIMIK_H__

