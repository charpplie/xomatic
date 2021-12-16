#include "StdAfx.h"
#include "WaitForFalling.h"


CAG2Modifier_WaitForFalling::CAG2Modifier_WaitForFalling() : CAG2ModifierBase()
{

}

CAG2Modifier_WaitForFalling::~CAG2Modifier_WaitForFalling()
{

}

const void CAG2Modifier_WaitForFalling::Save( XmlNodeRef modifierNode ) const
{
	// This node actually has no custom data
}

const void CAG2Modifier_WaitForFalling::Load( XmlNodeRef modifierNode )
{
	// This node actually has no custom data
}

CAG2ModifierBase* CAG2Modifier_WaitForFalling::Duplicate() const
{
	return new CAG2Modifier_WaitForFalling();
}

const void CAG2Modifier_WaitForFalling::Export( XmlNodeRef node ) const
{
	// <FreeFall/>

	XmlNodeRef fallnplayNode = node->createNode("FreeFall");
	node->addChild(fallnplayNode);
}

bool CAG2Modifier_WaitForFalling::CanConvertFromOldGraphVersion( const XmlNodeRef node ) const
{
	if (stricmp(node->getTag(), "FreeFall") == 0)
		return true;

	return false;
}
