#include "PropertyRowResourceFilePath.h"
#include "Serialization/ClassFactory.h"
#include <IEditor.h>
#include <QMenu>
#include <QFileDialog>
#include <QIcon>

ResourceFilePathMenuHandler::ResourceFilePathMenuHandler(QPropertyTree* tree, PropertyRowResourceFilePath* self)
: self(self), tree(tree)
{
}


void ResourceFilePathMenuHandler::onMenuClear()
{
	tree->model()->rowAboutToBeChanged(self);
	self->clear();
	tree->model()->rowChanged(self);
}


bool PropertyRowResourceFilePath::onActivate(const PropertyActivationEvent& e)
{
	if (e.reason == e.REASON_RELEASE)
		return false;
	if (!GetIEditor())
		return true;
	string filter = filter_;
	size_t filterLen = filter.size();
	if (filterLen < 2 || filter[filterLen-1] != '|' || filter[filterLen-2] != '|')
		filter += "||";
	string filename = GetIEditor()->SelectFile(filter_.c_str(), startFolder_.c_str(), path_.c_str());
	if (filename.empty())
		return true;
	if (flags_ & ResourceFilePath::STRIP_EXTENSION)
	{
		size_t ext = filename.rfind('.');
		if (ext != filename.npos)
			filename.erase(ext, filename.length() - ext);
	}

	e.tree->model()->rowAboutToBeChanged(this);
	path_ = filename;
	e.tree->model()->rowChanged(this);
	return true;
}
void PropertyRowResourceFilePath::setValueAndContext(const Serialization::SStruct& ser, Serialization::IArchive& ar) 
{
	ResourceFilePath* value = (ResourceFilePath*)ser.pointer();
	filter_ = value->filter.c_str();
	path_ = value->path->c_str();
	flags_ = value->flags;
	handle_ = value->path;
}

bool PropertyRowResourceFilePath::assignTo(const Serialization::SStruct& ser) const 
{
	((ResourceFilePath*)ser.pointer())->SetPath(path_.c_str());
	return true;
}

void PropertyRowResourceFilePath::serializeValue(Serialization::IArchive& ar)
{
	ar(filter_, "filter");
	ar(path_, "path");
	ar(startFolder_, "startFolder");
}

const QIcon& PropertyRowResourceFilePath::buttonIcon(const QPropertyTree* tree, int index) const
{ 
  #include "file_open.xpm"
	static QIcon fileOpenIcon = QIcon(QPixmap::fromImage(*tree->_iconCache()->getImageForIcon(Serialization::IconXPM(file_open_xpm))));
	return fileOpenIcon;
}

string PropertyRowResourceFilePath::valueAsString() const
{
	return path_;
}


void PropertyRowResourceFilePath::clear()
{
	path_.clear();
}

bool PropertyRowResourceFilePath::onContextMenu(QMenu &menu, QPropertyTree* tree)
{
	QAction* action = menu.addAction("Clear");
	Serialization::SharedPtr<PropertyRow> selfPointer(this);

	ResourceFilePathMenuHandler* handler = new ResourceFilePathMenuHandler(tree, this);
	QObject::connect(action, SIGNAL(triggered()), handler, SLOT(onMenuClear()));
	tree->addMenuHandler(handler);	
	return true;
}


REGISTER_PROPERTY_ROW(ResourceFilePath, PropertyRowResourceFilePath); 
DECLARE_SEGMENT(PropertyRowResourceFilePath)

#include <QPropertyTree/moc_PropertyRowResourceFilePath.cpp>