#pragma once

#include "PropertyDrawContext.h"
#include "PropertyRowField.h"
#include "QPropertyTree.h"
#include "PropertyTreeModel.h"
#include "Serialization.h"
#include <Serialization/Decorators/OutputFilePath.h>
#include <Serialization/Decorators/OutputFilePathImpl.h>

using Serialization::OutputFilePath;

class PropertyRowOutputFilePath : public PropertyRowField
{
public:
	void clear();

	bool isLeaf() const override{ return true; }
	bool isStatic() const override{ return false; }

	bool onActivate(const PropertyActivationEvent& e) override;
	void setValueAndContext(const Serialization::SStruct& ser, Serialization::IArchive& ar) override;
	bool assignTo(const Serialization::SStruct& ser) const override;
	string valueAsString() const;
	void serializeValue(Serialization::IArchive& ar);
	bool onContextMenu(QMenu &menu, QPropertyTree* tree);
	const void* searchHandle() const { return handle_; }

	int buttonCount() const override { return 1; }
	virtual const QIcon& buttonIcon(const QPropertyTree* tree, int index) const override;
	virtual bool usePathEllipsis() const override { return true; }

private:
	string path_;
	string filter_;
	string startFolder_;
	const void* handle_;
};

struct OutputFilePathMenuHandler : PropertyRowMenuHandler
{
	Q_OBJECT
public:
	QPropertyTree* tree;
	PropertyRowOutputFilePath* self;

	OutputFilePathMenuHandler(QPropertyTree* tree, PropertyRowOutputFilePath* container);
public slots:
	void onMenuClear();
};
