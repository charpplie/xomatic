#include "StdAfx.h"
#include "ColourFromXML.h"
#include "ItemParams.h"

CColourFromXml::CColourFromXml()
{
	r = g = b = a = 1.f;
}

void CColourFromXml::Read(const IItemParamsNode * xml)
{
	assert (xml);
	xml->GetAttribute("r", r);
	xml->GetAttribute("g", g);
	xml->GetAttribute("b", b);
	xml->GetAttribute("a", a);
}
