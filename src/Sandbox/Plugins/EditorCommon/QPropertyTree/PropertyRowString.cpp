#include <math.h>

#include "PropertyRowString.h"
#include "PropertyTreeModel.h"
#include "PropertyDrawContext.h"
#include "QPropertyTree.h"

#include "Serialization/IArchive.h"
#include "Serialization/ClassFactory.h"
#include <QMenu>
#include "Unicode.h"

// ---------------------------------------------------------------------------
SERIALIZATION_CLASS(PropertyRow, PropertyRowString, "string");

bool PropertyRowString::assignTo(string& str) const
{
    str = fromWideChar(value_.c_str());
    return true;
}

bool PropertyRowString::assignTo(wstring& str) const
{
    str = value_;
    return true;
}

PropertyRowWidget* PropertyRowString::createWidget(QPropertyTree* tree)
{
	return new PropertyRowWidgetString(this, tree);
}

bool PropertyRowString::assignToByPointer(void* instance, const Serialization::TypeID& type) const
{
	if (type == Serialization::TypeID::get<string>()) {
		assignTo(*(string*)instance);
		return true;
	}
	else if (type == Serialization::TypeID::get<wstring>()) {
		assignTo(*(wstring*)instance);
		return true;
	}
	return false;
}

string PropertyRowString::valueAsString() const
{
	return fromWideChar(value_.c_str());
}

void PropertyRowString::setValue(const wchar_t* str, const void* handle, const Serialization::TypeID& type)
{
	value_ = str;
	serializer_.setPointer((void*)handle);
	serializer_.setType(type);
}

void PropertyRowString::setValue(const char* str, const void* handle, const Serialization::TypeID& type)
{
	value_ = toWideChar(str);
	serializer_.setPointer((void*)handle);
	serializer_.setType(type);
}

void PropertyRowString::serializeValue(Serialization::IArchive& ar)
{
	ar(value_, "value", "Value");
}

#include <QPropertyTree/moc_PropertyRowString.cpp>
// vim:ts=4 sw=4:
