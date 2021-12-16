#include "PropertyRowObject.h"
#include "PropertyTreeModel.h"

PropertyRowObject::PropertyRowObject()
: model_(0)
{
}

bool PropertyRowObject::assignTo(Serialization::Object* obj)
{
	if (object_.type() == obj->type()) {
		*obj = object_;
		return true;
	}
	return false;
}

PropertyRowObject::~PropertyRowObject()
{
	object_ = Serialization::Object();
}

void PropertyRowObject::Serialize(Serialization::IArchive& ar)
{
    PropertyRow::Serialize(ar);
}

