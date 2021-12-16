#pragma once

#include "Serialization/StringList.h"
using Serialization::StringList;

#include "PropertyRow.h"

class QPropertyTree;
class PropertyRowPointer;
struct CreatePointerMenuHandler : PropertyRowMenuHandler
{
	Q_OBJECT
public:
	QPropertyTree* tree;
	PropertyRowPointer* row;
	int index;
	bool useDefaultValue;
public slots:
	void onMenuCreateByIndex();
};

class QMenu;
struct ClassMenuItemAdder
{
	virtual void addAction(QMenu& menu, const char* text, int index);
	virtual QMenu* addMenu(QMenu& menu, const char* text);
	void generateMenu(QMenu& createItem, const StringList& comboStrings);
};

class PropertyRowPointer : public PropertyRow
{
public:
	PropertyRowPointer();

	bool assignTo(Serialization::IPointer &ptr);
	void setValueAndContext(const Serialization::IPointer& ptr, Serialization::IArchive& ar);
	using PropertyRow::assignTo;

	Serialization::TypeID baseType() const{ return baseType_; }
	void setBaseType(const Serialization::TypeID& baseType) { baseType_ = baseType; }
	const char* derivedTypeName() const{ return derivedTypeName_.c_str(); }
	Serialization::TypeID getDerivedType(Serialization::IClassFactory* factory) const;
	void setDerivedType(const Serialization::TypeID& typeID, Serialization::IClassFactory* factory);
	void setFactory(Serialization::IClassFactory* factory) { factory_ = factory; }
	Serialization::IClassFactory* factory() const{ return factory_; }
	bool onActivate( QPropertyTree* tree, bool force);
	bool onMouseDown(QPropertyTree* tree, QPoint point, bool& changed);
	bool onContextMenu(QMenu &root, QPropertyTree* tree);
	bool isStatic() const{ return false; }
	bool isPointer() const{ return true; }
	int widgetSizeMin(const QPropertyTree* tree) const override;
	wstring generateLabel() const;
	string valueAsString() const;
	const char* typeNameForFilter(QPropertyTree* tree) const override { return baseType_.name(); }
	void redraw(const PropertyDrawContext& context);
	WidgetPlacement widgetPlacement() const{ return WIDGET_VALUE; }
	void serializeValue(Serialization::IArchive& ar);
	const void* searchHandle() const override { return searchHandle_; }
	Serialization::TypeID typeId() const override{ return pointerType_; }
protected:

	Serialization::TypeID baseType_;
	string derivedTypeName_;
	string derivedLabel_;

	// this member is available for instances deserialized from clipboard:
	Serialization::IClassFactory* factory_;
	const void* searchHandle_;
	Serialization::TypeID  pointerType_;
};

