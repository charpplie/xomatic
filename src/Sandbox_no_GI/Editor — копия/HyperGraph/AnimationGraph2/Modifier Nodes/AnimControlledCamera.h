/************************************************************************/
/*              Animation Controlled Camera Modifier Node               */
/************************************************************************/
/* This modifier will cause the FirstPerson Camera to be controlled     */
/* by the animation. The Camera position and orientation will be placed */
/* between the two eye bones. See CAnimatedCharacter::FilterView() for  */
/* the actual implementation. This works in collaboration with the game */
/* code (camera implementation).                                        */
/* This will work only with Animation Controlled Movement.              */
/*                                                                      */
/* DO NOT USE THIS MODIFIER AS AN EXAMPLE; IT IS A SPECIAL CASE         */
/************************************************************************/


#ifndef __ANIMATIONGRAPH2_FPCAMERACONTROL_H__
#define __ANIMATIONGRAPH2_FPCAMERACONTROL_H__

#pragma once

#include "../AnimationGraph2_Modifier.h"

class CAG2Modifier_AnimControlledCamera : public CAG2ModifierBase
{
public:
	CAG2Modifier_AnimControlledCamera();
	virtual ~CAG2Modifier_AnimControlledCamera();

	// CAG2ModifierBase functions (see base class for documentation)
	//////////////////////////////////////////////////////////////////////////

	virtual void Init();

	virtual CAG2ModifierBase* Duplicate() const;
	virtual const CString GetHumanReadableName() { return "Anim Controlled Camera"; };
	virtual const CString GetClassName() { return "AnimControlledCamera"; };
	virtual const bool IsSingleton() const { return true; }

	virtual const void Save(XmlNodeRef modifierNode) const;
	virtual const void Load(XmlNodeRef modifierNode);
	virtual const void Export( XmlNodeRef node ) const {};  

};


#endif // __ANIMATIONGRAPH2_FPCAMERACONTROL_H__

