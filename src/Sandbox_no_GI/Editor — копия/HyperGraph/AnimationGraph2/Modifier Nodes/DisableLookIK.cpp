#include "StdAfx.h"
#include "DisableLookIK.h"


CAG2Modifier_DisableLookIK::CAG2Modifier_DisableLookIK() : CAG2ModifierBase()
{

}

CAG2Modifier_DisableLookIK::~CAG2Modifier_DisableLookIK()
{

}

const void CAG2Modifier_DisableLookIK::Save( XmlNodeRef modifierNode ) const
{
	// This node actually has no custom data
}

const void CAG2Modifier_DisableLookIK::Load( XmlNodeRef modifierNode )
{
	// This node actually has no custom data
}

CAG2ModifierBase* CAG2Modifier_DisableLookIK::Duplicate() const
{
	return new CAG2Modifier_DisableLookIK();
}

const void CAG2Modifier_DisableLookIK::Export( XmlNodeRef node ) const
{
	// <LookIk override="1" />

	XmlNodeRef lookIKNode = node->createNode("LookIk");
	lookIKNode->setAttr("override", "1");
	node->addChild(lookIKNode);
}

bool CAG2Modifier_DisableLookIK::CanConvertFromOldGraphVersion( const XmlNodeRef node ) const
{
	if (stricmp(node->getTag(), "LookIk") == 0)
	{
		if (!node->haveAttr("override") || atoi(node->getAttr("override")) != 1)
			return false;

		return true;
	}

	return false;
}
