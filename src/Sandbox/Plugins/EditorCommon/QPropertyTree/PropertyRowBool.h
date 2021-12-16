#pragma once

#include "PropertyRow.h"
#include "Unicode.h"

class PropertyRowBool : public PropertyRow
{
public:
	PropertyRowBool();
	bool assignToPrimitive(void* val, size_t size) const override;
	bool assignToByPointer(void* instance, const Serialization::TypeID& type) const;
	void setValue(bool value, const void* handle, const Serialization::TypeID& typeId) { value_ = value; serializer_.setPointer((void*)handle); serializer_.setType(Serialization::TypeID::get<bool>()); }

	void redraw(const PropertyDrawContext& context);
	bool isLeaf() const{ return true; }
	bool isStatic() const{ return false; }

	bool onActivate(const PropertyActivationEvent& e);
	DragCheckBegin onMouseDragCheckBegin() override;
	bool onMouseDragCheck(QPropertyTree* tree, bool value) override;
	wstring valueAsWString() const{ return value_ ? L"true" : L"false"; }
	string valueAsString() const{ return value_ ? "true" : "false"; }
	WidgetPlacement widgetPlacement() const{ return WIDGET_ICON; }
	void serializeValue(Serialization::IArchive& ar);
	int widgetSizeMin(const QPropertyTree* tree) const override;
	bool onKeyDown(QPropertyTree* tree, const QKeyEvent* ev) override;
protected:
	bool value_;
};

