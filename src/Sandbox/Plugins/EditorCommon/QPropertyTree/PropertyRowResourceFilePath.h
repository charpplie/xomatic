#pragma once

#include "PropertyDrawContext.h"
#include "PropertyRowField.h"
#include "QPropertyTree.h"
#include "PropertyTreeModel.h"
#include "Serialization.h"
#include <Serialization/Decorators/ResourceFilePath.h>
#include <Serialization/Decorators/ResourceFilePathImpl.h>
#include <Serialization/Decorators/IconXPM.h>

using Serialization::ResourceFilePath;

class PropertyRowResourceFilePath : public PropertyRowField
{
public:
	void clear();

	bool isLeaf() const override{ return true; }
	bool isStatic() const override{ return false; }

	void setValueAndContext(const Serialization::SStruct& ser, Serialization::IArchive& ar) override;
	bool assignTo(const Serialization::SStruct& ser) const override;
	bool onActivate(const PropertyActivationEvent& e) override;

	int buttonCount() const override { return 1; }
	virtual const QIcon& buttonIcon(const QPropertyTree* tree, int index) const override;
	virtual bool usePathEllipsis() const override { return true; }
	string valueAsString() const;
	void serializeValue(Serialization::IArchive& ar);
	const void* searchHandle() const override { return handle_; }
	Serialization::TypeID typeId() const override { return Serialization::TypeID::get<string>(); }

	bool onContextMenu(QMenu &menu, QPropertyTree* tree);
private:
	string filter_;
	string path_;
	string startFolder_;
	int flags_;
	const void* handle_;
};

struct ResourceFilePathMenuHandler : PropertyRowMenuHandler
{
	Q_OBJECT
public:
	QPropertyTree* tree;
	PropertyRowResourceFilePath* self;

	ResourceFilePathMenuHandler(QPropertyTree* tree, PropertyRowResourceFilePath* container);
public slots:
	void onMenuClear();
};
