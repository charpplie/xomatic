#include "StdAfx.h"
#include "WaitforStandUp.h"


CAG2Modifier_WaitForStandUp::CAG2Modifier_WaitForStandUp() : CAG2ModifierBase()
{

}

CAG2Modifier_WaitForStandUp::~CAG2Modifier_WaitForStandUp()
{

}

const void CAG2Modifier_WaitForStandUp::Save( XmlNodeRef modifierNode ) const
{
	// This node actually has no custom data
}

const void CAG2Modifier_WaitForStandUp::Load( XmlNodeRef modifierNode )
{
	// This node actually has no custom data
}

CAG2ModifierBase* CAG2Modifier_WaitForStandUp::Duplicate() const
{
	return new CAG2Modifier_WaitForStandUp();
}

const void CAG2Modifier_WaitForStandUp::Export( XmlNodeRef node ) const
{
	// <FallAndPlay/>

	XmlNodeRef fallnplayNode = node->createNode("FallAndPlay");
	node->addChild(fallnplayNode);
}

bool CAG2Modifier_WaitForStandUp::CanConvertFromOldGraphVersion( const XmlNodeRef node ) const
{
	if (stricmp(node->getTag(), "FallAndPlay") == 0)
		return true;

	return false;
}
