#include "StdAfx.h"
#include "DisableAimIK.h"


CAG2Modifier_DisableAimIK::CAG2Modifier_DisableAimIK() : CAG2ModifierBase()
{

}

CAG2Modifier_DisableAimIK::~CAG2Modifier_DisableAimIK()
{

}

const void CAG2Modifier_DisableAimIK::Save( XmlNodeRef modifierNode ) const
{
	// This node actually has no custom data
}

const void CAG2Modifier_DisableAimIK::Load( XmlNodeRef modifierNode )
{
	// This node actually has no custom data
}

CAG2ModifierBase* CAG2Modifier_DisableAimIK::Duplicate() const
{
	return new CAG2Modifier_DisableAimIK();
}

const void CAG2Modifier_DisableAimIK::Export( XmlNodeRef node ) const
{
	XmlNodeRef aimIKNode = node->createNode("AimIk");
	//aimIKNode->setAttr("override", "1");
	node->addChild(aimIKNode);
}