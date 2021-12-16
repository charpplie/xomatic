#pragma once

#include "PropertyDrawContext.h"
#include "PropertyRowImpl.h"
#include "QPropertyTree.h"
#include "PropertyTreeModel.h"
#include "Serialization.h"
#include <Serialization/Decorators/ResourceFolderPath.h>
#include <Serialization/Decorators/ResourceFolderPathImpl.h>
#include <QFileDialog>
#include <QtGui/QIcon>

using Serialization::ResourceFolderPath;

class PropertyRowResourceFolderPath : public PropertyRowField
{
public:
	PropertyRowResourceFolderPath() : handle_() {}
	bool isLeaf() const override{ return true; }
	bool isStatic() const override{ return false; }
	bool onActivate(const PropertyActivationEvent& e) override;
	void setValueAndContext(const Serialization::SStruct& ser, Serialization::IArchive& ar) override;
	bool assignTo(const Serialization::SStruct& ser) const override;
	string valueAsString() const;
	bool onContextMenu(QMenu &menu, QPropertyTree* tree);

	int buttonCount() const override { return 1; }
	virtual const QIcon& buttonIcon(const QPropertyTree* tree, int index) const override;
	virtual bool usePathEllipsis() const override { return true; }
	void serializeValue(Serialization::IArchive& ar) override;
	const void* searchHandle() const override { return handle_; }
	Serialization::TypeID typeId() const override { return Serialization::TypeID::get<string>(); }
	void clear();
private:
	string path_;
	string startFolder_;
	const void* handle_;
};

struct ResourceFolderPathMenuHandler : PropertyRowMenuHandler
{
	Q_OBJECT
public:
	QPropertyTree* tree;
	PropertyRowResourceFolderPath* self;

	ResourceFolderPathMenuHandler(QPropertyTree* tree, PropertyRowResourceFolderPath* container);
public slots:
	void onMenuClear();
};
