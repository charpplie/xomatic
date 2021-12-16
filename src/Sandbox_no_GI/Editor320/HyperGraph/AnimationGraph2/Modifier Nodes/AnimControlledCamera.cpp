#include "StdAfx.h"
#include "AnimControlledCamera.h"

CAG2Modifier_AnimControlledCamera::CAG2Modifier_AnimControlledCamera() : CAG2ModifierBase()
{
}

CAG2Modifier_AnimControlledCamera::~CAG2Modifier_AnimControlledCamera()
{
}

void CAG2Modifier_AnimControlledCamera::Init()
{
}

CAG2ModifierBase* CAG2Modifier_AnimControlledCamera::Duplicate() const
{
	return new CAG2Modifier_AnimControlledCamera();
}


const void CAG2Modifier_AnimControlledCamera::Save( XmlNodeRef modifierNode ) const
{
}

const void CAG2Modifier_AnimControlledCamera::Load( XmlNodeRef modifierNode )
{
}

