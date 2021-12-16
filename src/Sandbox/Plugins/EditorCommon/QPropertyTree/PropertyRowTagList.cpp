#include "PropertyRowTagList.h"
#include "PropertyRowString.h"
#include "QPropertyTree.h"
#include "Serialization/Decorators/TagList.h"
#include "Serialization/ClassFactory.h"
#include "Serialization/IArchive.h"
#include "Serialization/Decorators/TagListImpl.h"
#include <QMenu>

PropertyRowTagList::PropertyRowTagList()
: source_(0)
{
}

PropertyRowTagList::~PropertyRowTagList()
{
	if (source_)
		source_->Release();
}

void PropertyRowTagList::generateMenu(QMenu& item, QPropertyTree* tree, bool addActions)
{
	if (userReadOnly() || isFixedSize())
		return;

	if (!source_)
		return;

	TagListMenuHandler* handler = new TagListMenuHandler();
	handler->tree = tree;
	handler->row = this;
	tree->addMenuHandler(handler);

	unsigned int numGroups = source_->GroupCount();
	for (unsigned int group = 0; group < numGroups; ++group) {
		unsigned int tagCount = source_->TagCount(group);
		if (tagCount == 0)
			continue;
		const char* groupName = source_->GroupName(group);
		QString title = QString("From ") + groupName;
		QMenu* menu = item.addMenu(title);
		for (unsigned int tagIndex = 0; tagIndex < tagCount; ++tagIndex) {
			QString str;
			str = source_->TagValue(group, tagIndex);
			const char* desc = source_->TagDescription(group, tagIndex);
			if (desc && desc[0] != '\0') {
				str += "\t";
				str += desc;
			}
			QAction* action = menu->addAction(str);
			QString tag = QString::fromLocal8Bit(source_->TagValue(group, tagIndex));
			action->setData(QVariant(tag));
			QObject::connect(action, SIGNAL(triggered()), handler, SLOT(onMenuAddTag()));
		}
	}


	QAction* action = item.addAction("Add");
	action->setData(QVariant(QString()));
	QObject::connect(action, SIGNAL(triggered()), handler, SLOT(onMenuAddTag()));

	PropertyRowContainer::generateMenu(item, tree, false);
}

void PropertyRowTagList::addTag(const char* tag, QPropertyTree* tree)
{
	Serialization::SharedPtr<PropertyRowTagList> ref(this);

	PropertyRow* child = addElement(tree, false);
	if (child && strcmp(child->typeName(), "string") == 0)
	{
		PropertyRowString* stringRow = static_cast<PropertyRowString*>(child);
		tree->model()->rowAboutToBeChanged(stringRow);
		stringRow->setValue(tag, stringRow->searchHandle(), stringRow->typeId());
		tree->model()->rowChanged(stringRow);
	}
}

void TagListMenuHandler::onMenuAddTag()
{
	if (QAction* action = qobject_cast<QAction*>(sender())) {
		QString str = action->data().toString();
		row->addTag(str.toLocal8Bit().data(), tree);
	}
}

void PropertyRowTagList::setValueAndContext(const Serialization::IContainer& value, Serialization::IArchive& ar)
{
		if (source_)
			source_->Release();
		source_ = ar.FindContext<ITagSource>();
		if (source_)
			source_->AddRef();

		PropertyRowContainer::setValueAndContext(value, ar);
}


REGISTER_PROPERTY_ROW(TagList, PropertyRowTagList)
DECLARE_SEGMENT(PropertyRowTagList)

#include <QPropertyTree/moc_PropertyRowTagList.cpp>
