#pragma once

#include "Serialization/Decorators/IGizmoSink.h"
#include "PropertyRowField.h"
#include "QPropertyTree.h"

struct IGizmoSink;

class PropertyRowLocalFrameBase : public PropertyRow
{
public:
	PropertyRowLocalFrameBase();
	~PropertyRowLocalFrameBase();

	bool isLeaf() const override{ return m_reset; }
	bool isStatic() const override{ return false; }

	bool onActivate(const PropertyActivationEvent& e) override;

	WidgetPlacement widgetPlacement() const override { return WIDGET_AFTER_PULLED; }
	int widgetSizeMin(const QPropertyTree* tree) const override{ return tree->_defaultRowHeight(); }

	string valueAsString() const override;
	bool onContextMenu(QMenu &menu, QPropertyTree* tree) override;
	const void* searchHandle() const override { return m_handle; }
	void redraw(const PropertyDrawContext& context) override;

	void reset(QPropertyTree* tree);
protected:
	Serialization::IGizmoSink* m_sink;
	const void* m_handle;
	int m_gizmoIndex;
	mutable Serialization::GizmoFlags m_gizmoFlags;
	bool m_reset;
};

struct LocalFrameMenuHandler : PropertyRowMenuHandler
{
	Q_OBJECT
public:
	QPropertyTree* tree;
	PropertyRowLocalFrameBase* self;

	LocalFrameMenuHandler(QPropertyTree* tree, PropertyRowLocalFrameBase* self) : tree(tree), self(self) {}
public slots:
	void onMenuReset();
};
