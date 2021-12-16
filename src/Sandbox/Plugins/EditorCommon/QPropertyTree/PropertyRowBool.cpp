#include "PropertyRowBool.h"
#include "QPropertyTree.h"
#include "PropertyTreeModel.h"
#include "PropertyDrawContext.h"
#include "Serialization/ClassFactory.h"
#include "Serialization.h"
#include <QKeyEvent>

SERIALIZATION_CLASS(PropertyRow, PropertyRowBool, "bool");

PropertyRowBool::PropertyRowBool()
: value_(false)
{
}

bool PropertyRowBool::assignToPrimitive(void* object, size_t size) const
{
	YASLI_ASSERT(size == sizeof(bool));
	*reinterpret_cast<bool*>(object) = value_;
	return true;
}

bool PropertyRowBool::assignToByPointer(void* instance, const Serialization::TypeID& type) const
{
	return assignToPrimitive(instance, type.sizeOf()); 
}

void PropertyRowBool::redraw(const PropertyDrawContext& context)
{
	context.drawCheck(widgetRect(context.tree), userReadOnly(), multiValue() ? CHECK_IN_BETWEEN : (value_ ? CHECK_SET : CHECK_NOT_SET));
}

bool PropertyRowBool::onKeyDown(QPropertyTree* tree, const QKeyEvent* ev)
{
	if (QKeySequence(ev->key()) == QKeySequence(Qt::Key_Space))
	{
		PropertyActivationEvent e;
		e.tree = tree;
		e.reason = e.REASON_KEYBOARD;
		onActivate(e);
		return true;
	}

	return PropertyRow::onKeyDown(tree, ev);
}

bool PropertyRowBool::onActivate(const PropertyActivationEvent& e)
{
	if (e.reason != e.REASON_RELEASE)
	{
		if (!this->userReadOnly()) {
			e.tree->model()->rowAboutToBeChanged(this);
			value_ = !value_;
			e.tree->model()->rowChanged(this);
			return true;
		}
	}
	return false;
}

DragCheckBegin PropertyRowBool::onMouseDragCheckBegin() 
{
	if (userReadOnly())
		return DRAG_CHECK_IGNORE;
	return value_ ? DRAG_CHECK_UNSET : DRAG_CHECK_SET;
}

bool PropertyRowBool::onMouseDragCheck(QPropertyTree* tree, bool value)
{
	if (value_ != value) {
		tree->model()->rowAboutToBeChanged(this);
		value_ = value;
		tree->model()->rowChanged(this);
		return true;
	}
	return false;
}

void PropertyRowBool::serializeValue(Serialization::IArchive& ar)
{
    ar(value_, "value", "Value");
}

int PropertyRowBool::widgetSizeMin(const QPropertyTree* tree) const
{
	return tree->_defaultRowHeight();
}
