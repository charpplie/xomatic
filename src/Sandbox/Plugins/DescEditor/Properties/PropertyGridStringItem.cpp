#include "pch.h"
#include "PropertyGridStringItem.h"
#include "ClassProfile.h"

using namespace CryGame;


IMPLEMENT_SERIAL(CPropertyGridStringItem, CPropertyGridItem, 0)
REGISTER_PROPERTY_EDITOR(CPropertyGridStringItem, Default, eVType_String, "")