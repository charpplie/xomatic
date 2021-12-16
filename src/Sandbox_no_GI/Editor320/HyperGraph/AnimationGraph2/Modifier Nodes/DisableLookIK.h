/************************************************************************/
/*                       Disable LookIK Modifier Node                   */
/************************************************************************/


#ifndef __ANIMATIONGRAPH2_DISABLELOOKIK_H__
#define __ANIMATIONGRAPH2_DISABLELOOKIK_H__

#pragma once

#include "../AnimationGraph2_Modifier.h"

class CAG2Modifier_DisableLookIK : public CAG2ModifierBase
{
public:
	CAG2Modifier_DisableLookIK();
	virtual ~CAG2Modifier_DisableLookIK();


	// CAG2ModifierBase functions (see base class for documentation)
	//////////////////////////////////////////////////////////////////////////

	virtual const CString GetHumanReadableName() { return "Disable LookIK"; };
	virtual const CString GetClassName() { return "DisableLookIK"; };
	virtual const bool IsSingleton() const { return true; }

	virtual const void Save(XmlNodeRef modifierNode) const;
	virtual const void Load(XmlNodeRef modifierNode);
	virtual const void Export( XmlNodeRef node ) const;

	virtual CAG2ModifierBase* Duplicate() const;

	// Old Graph Version Conversion Stuff
	virtual bool CanConvertFromOldGraphVersion( const XmlNodeRef node ) const;
};


#endif // __ANIMATIONGRAPH2_DISABLELOOKIK_H__

