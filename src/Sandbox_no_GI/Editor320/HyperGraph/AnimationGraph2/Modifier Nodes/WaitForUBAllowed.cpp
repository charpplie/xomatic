#include "StdAfx.h"
#include "WaitForUBAllowed.h"


CAG2Modifier_WaitForUBAllowed::CAG2Modifier_WaitForUBAllowed() : CAG2ModifierBase()
{

}

CAG2Modifier_WaitForUBAllowed::~CAG2Modifier_WaitForUBAllowed()
{

}

const void CAG2Modifier_WaitForUBAllowed::Save( XmlNodeRef modifierNode ) const
{
	// This node actually has no custom data
}

const void CAG2Modifier_WaitForUBAllowed::Load( XmlNodeRef modifierNode )
{
	// This node actually has no custom data
}

CAG2ModifierBase* CAG2Modifier_WaitForUBAllowed::Duplicate() const
{
	return new CAG2Modifier_WaitForUBAllowed();
}

const void CAG2Modifier_WaitForUBAllowed::Export( XmlNodeRef node ) const
{
	// <WaitForUBAllowed/>

	XmlNodeRef fallnplayNode = node->createNode("WaitForUBAllowed");
	node->addChild(fallnplayNode);
}

bool CAG2Modifier_WaitForUBAllowed::CanConvertFromOldGraphVersion( const XmlNodeRef node ) const
{
	if (stricmp(node->getTag(), "WaitForUBAllowed") == 0)
		return true;

	return false;
}
