#include "Factory.h"
#include "PropertyRowStringListValue.h"

#include "Serialization/IArchive.h"
#include "Serialization/ClassFactory.h"

using Serialization::StringList;
using Serialization::StringListValue;

REGISTER_PROPERTY_ROW(StringListValue, PropertyRowStringListValue)

PropertyRowWidget* PropertyRowStringListValue::createWidget(QPropertyTree* tree)
{
	return new PropertyRowWidgetStringListValue(this, tree);
}

// ---------------------------------------------------------------------------
REGISTER_PROPERTY_ROW(StringListStaticValue, PropertyRowStringListStaticValue)

PropertyRowWidget* PropertyRowStringListStaticValue::createWidget(QPropertyTree* tree)
{
	return new PropertyRowWidgetStringListValue(this, tree);
}

DECLARE_SEGMENT(PropertyRowStringList)

#include <QPropertyTree/moc_PropertyRowStringListValue.cpp>
// vim:ts=4 sw=4:
