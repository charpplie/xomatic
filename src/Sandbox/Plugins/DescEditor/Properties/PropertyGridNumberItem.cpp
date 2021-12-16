#include "pch.h"
#include "PropertyGridNumberItem.h"
#include "ClassProfile.h"

using namespace CryGame;


IMPLEMENT_SERIAL(CPropertyGridFloatItem, CPropertyGridNumberItem, 0)
REGISTER_PROPERTY_EDITOR(CPropertyGridFloatItem, Default, eVType_Float, "")

IMPLEMENT_SERIAL(CPropertyGridInt32Item, CPropertyGridNumberItem, 0)
REGISTER_PROPERTY_EDITOR(CPropertyGridInt32Item, Default, eVType_Int, "")

IMPLEMENT_SERIAL(CPropertyGridUInt32Item, CPropertyGridNumberItem, 0)
REGISTER_PROPERTY_EDITOR(CPropertyGridUInt32Item, Default, eVType_UInt, "")