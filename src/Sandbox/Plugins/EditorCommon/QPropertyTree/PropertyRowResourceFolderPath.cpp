#include <Serialization/Decorators/IconXPM.h>
#include "PropertyRowResourceFolderPath.h"
#include "Serialization/ClassFactory.h"
#include <IEditor.h>
#include <ICryPak.h>
#include <QMenu>
#include <shlobj.h>

ResourceFolderPathMenuHandler::ResourceFolderPathMenuHandler(QPropertyTree* tree, PropertyRowResourceFolderPath* self)
: self(self), tree(tree)
{
}

void ResourceFolderPathMenuHandler::onMenuClear()
{
	tree->model()->rowAboutToBeChanged(self);
	self->clear();
	tree->model()->rowChanged(self);
}

bool PropertyRowResourceFolderPath::onActivate(const PropertyActivationEvent& e)
{
	if (e.reason == e.REASON_RELEASE)
		return false;
	if (!GetIEditor())
		return true;

	if (userReadOnly())
		return false;

	QString title;
	if (labelUndecorated() && labelUndecorated()[0] != '\0')
		title = QString("Choose folder for '") + QString::fromLocal8Bit(labelUndecorated()) + "'";
	else
		title = "Choose folder";

	QString gameFolder = QString::fromLocal8Bit(GetIEditor()->GetSystem()->GetIPak()->GetGameFolder());
	QString startFolder = gameFolder + QDir::separator();
	
	if (path_.empty() || !QDir().exists(startFolder))
		startFolder += QString::fromLocal8Bit(startFolder_.c_str());
	else
		startFolder += QString::fromLocal8Bit(path_.c_str());
	
	QString filename = QFileDialog::getExistingDirectory(e.tree, title, startFolder, QFileDialog::ShowDirsOnly);
	if (filename.isEmpty())
		return true;

	e.tree->model()->rowAboutToBeChanged(this);
	QString result = QDir(gameFolder).relativeFilePath(filename);
	path_ = result.toLocal8Bit().data();
	e.tree->model()->rowChanged(this);
	return true;
}
void PropertyRowResourceFolderPath::setValueAndContext(const Serialization::SStruct& ser, Serialization::IArchive& ar) 
{
	ResourceFolderPath* value = (ResourceFolderPath*)(ser.pointer());
	path_ = value->path->c_str();
	startFolder_ = value->startFolder.c_str();
	handle_ = value->path;
}

bool PropertyRowResourceFolderPath::assignTo(const Serialization::SStruct& ser) const 
{
	((ResourceFolderPath*)ser.pointer())->SetPath(path_.c_str());
	return true;
}

const QIcon& PropertyRowResourceFolderPath::buttonIcon(const QPropertyTree* tree, int index) const
{ 
  #include "file_open.xpm"
	static QIcon fileOpenIcon = QIcon(QPixmap::fromImage(*tree->_iconCache()->getImageForIcon(Serialization::IconXPM(file_open_xpm))));
	return fileOpenIcon;
}

string PropertyRowResourceFolderPath::valueAsString() const
{
	return path_;
}

void PropertyRowResourceFolderPath::clear()
{
	path_.clear();
}

void PropertyRowResourceFolderPath::serializeValue(Serialization::IArchive& ar)
{
	ar(path_, "path");
	ar(startFolder_, "startFolder");
}

bool PropertyRowResourceFolderPath::onContextMenu(QMenu &menu, QPropertyTree* tree)
{
	ResourceFolderPathMenuHandler* handler = new ResourceFolderPathMenuHandler(tree, this);
	QAction* action = menu.addAction("Clear", handler, SLOT(onMenuClear()));
	action->setEnabled(!userReadOnly());
	SharedPtr<PropertyRow> selfPointer(this);

	tree->addMenuHandler(handler);	
	return true;
}

REGISTER_PROPERTY_ROW(ResourceFolderPath, PropertyRowResourceFolderPath); 
DECLARE_SEGMENT(PropertyRowResourceFolderPath)

#include <QPropertyTree/moc_PropertyRowResourceFolderPath.cpp>