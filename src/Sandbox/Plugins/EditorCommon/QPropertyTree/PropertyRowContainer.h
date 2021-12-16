#pragma once
#include "PropertyRow.h"


class PropertyRowContainer;
struct ContainerMenuHandler : PropertyRowMenuHandler
{
	Q_OBJECT
public:

	QPropertyTree* tree;
	PropertyRowContainer* container;
	PropertyRow* element;
	int pointerIndex;

	ContainerMenuHandler(QPropertyTree* tree, PropertyRowContainer* container);

public slots:
	void onMenuAddElement();
	void onMenuAppendElement();
	void onMenuAppendPointerByIndex();
	void onMenuRemoveAll();
	void onMenuChildInsertBefore();
	void onMenuChildRemove();
};

class PropertyRowContainer : public PropertyRow
{
public:
	PropertyRowContainer();
	bool isContainer() const{ return true; }
	bool onActivate(const PropertyActivationEvent& e);
	bool onContextMenu(QMenu& item, QPropertyTree* tree);
	void redraw(const PropertyDrawContext& context);
	bool onKeyDownContainer(QPropertyTree* tree, const QKeyEvent* key);
	bool onKeyDown(QPropertyTree* tree, const QKeyEvent* key) override;

	void labelChanged() override;
	bool isStatic() const{ return false; }
	bool isSelectable() const{ return userWidgetSize() == 0 ? false : true; }
	PropertyRow* addElement(QPropertyTree* tree, bool append);
	void setInlined(bool inlined) { inlined_ = inlined; }
	bool isInlined() const{ return inlined_; }

	PropertyRow* defaultRow(PropertyTreeModel* model);
	const PropertyRow* defaultRow(const PropertyTreeModel* model) const;
	void serializeValue(Serialization::IArchive& ar);

	const char* elementTypeName() const{ return elementTypeName_; }
	virtual void setValueAndContext(const Serialization::IContainer& value, Serialization::IArchive& ar) {
		fixedSize_ = value.isFixedSize();
		elementTypeName_ = value.elementType().name();
		serializer_.setPointer(value.pointer());
		serializer_.setType(value.containerType());
	}
	const char* typeNameForFilter(QPropertyTree* tree) const override;
	string valueAsString() const;
	// C-array is an example of fixed size container
	bool isFixedSize() const{ return fixedSize_; }
	WidgetPlacement widgetPlacement() const override{ return inlined_ ? WIDGET_NONE : WIDGET_AFTER_NAME; }
	int widgetSizeMin(const QPropertyTree* tree) const override;

protected:
	virtual void generateMenu(QMenu& menu, QPropertyTree* tree, bool addActions);

	const char* elementTypeName_;
	wchar_t buttonLabel_[8];
	bool fixedSize_;
	bool inlined_;
};
