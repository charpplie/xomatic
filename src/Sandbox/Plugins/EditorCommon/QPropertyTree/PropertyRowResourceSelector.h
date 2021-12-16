#pragma once

#include "PropertyDrawContext.h"
#include "QPropertyTree.h"
#include "PropertyTreeModel.h"
#include "PropertyRowField.h"
#include <Serialization.h>
#include "Serialization/Decorators/Resources.h"
#include "IResourceSelectorHost.h"
#include <QIcon>

using Serialization::IResourceSelector;
namespace Serialization { struct INavigationProvider; }

class PropertyRowResourceSelector : public PropertyRowField
{
public:
	PropertyRowResourceSelector() : provider_(0), id_(0), searchHandle_(0) {}
	void clear();

	bool isLeaf() const override{ return true; }
	bool isStatic() const override{ return false; }

	void jumpTo(QPropertyTree* tree);
	bool createFile(QPropertyTree* tree);
	void setValueAndContext(const Serialization::SStruct& ser, Serialization::IArchive& ar) override;
	bool assignTo(const Serialization::SStruct& ser) const override;
	bool onActivate(const PropertyActivationEvent& ev) override;
	bool onActivateButton(int button, const PropertyActivationEvent& e) override;
	bool getHoverInfo(PropertyHoverInfo* hover, const QPoint& cursorPos, const QPropertyTree* tree) const override;
	const void* searchHandle() const override { return searchHandle_; }
	Serialization::TypeID typeId() const override{ return wrappedType_; }

	int buttonCount() const override;
	virtual const QIcon& buttonIcon(const QPropertyTree* tree, int index) const override;
	virtual bool usePathEllipsis() const override { return true; }
	string valueAsString() const;
	void serializeValue(Serialization::IArchive& ar);
	void redraw(const PropertyDrawContext& context);

	bool onContextMenu(QMenu &menu, QPropertyTree* tree);
	bool pickResource(QPropertyTree* tree);
	const char* typeNameForFilter(QPropertyTree* tree) const override { return !type_.empty() ? type_.c_str() : "ResourceSelector"; }
private:
	SResourceSelectorContext context_;
	Serialization::INavigationProvider* provider_;
	const void* searchHandle_;
	Serialization::TypeID wrappedType_;
	QIcon icon_;

	string type_;
	string value_;
	string defaultPath_;
	int id_;
};

struct ResourceSelectorMenuHandler : PropertyRowMenuHandler
{
	Q_OBJECT
public:
	QPropertyTree* tree;
	PropertyRowResourceSelector* self;

	ResourceSelectorMenuHandler(QPropertyTree* tree, PropertyRowResourceSelector* container);
public slots:
	void onMenuCreateFile();
	void onMenuJumpTo();
	void onMenuClear();
	void onMenuPickResource();
};
