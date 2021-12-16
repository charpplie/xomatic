#pragma once

#include "PropertyRowContainer.h"

struct ITagSource;

class PropertyRowTagList : public PropertyRowContainer
{
public:
	PropertyRowTagList();
	~PropertyRowTagList();
	void setValueAndContext(const Serialization::IContainer& value, Serialization::IArchive& ar) override;
	void generateMenu(QMenu& item, QPropertyTree* tree, bool addActions) override;
	void addTag(const char* tag, QPropertyTree* tree);

private:
	using PropertyRow::setValueAndContext;
	ITagSource* source_;
};

struct TagListMenuHandler : public PropertyRowMenuHandler
{
	Q_OBJECT
public:

	PropertyRowTagList* row;
	QPropertyTree* tree;
public slots:
	void onMenuAddTag();
};
