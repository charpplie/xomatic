#include "PropertyRowPointer.h"
#include "QPropertyTree.h"
#include "PropertyTreeModel.h"
#include "PropertyDrawContext.h"
#include "Serialization.h"
#include "Unicode.h"
#include <QMenu>


// ---------------------------------------------------------------------------

static QAction* findAction(const QList<QAction*>& actions, const char* withText)
{
	for (size_t i = 0; i < size_t(actions.size()); ++i)
	{
		if (actions[i]->text() == withText)
			return actions[i];
	}
	return 0;
}

void ClassMenuItemAdder::generateMenu(QMenu& createItem, const StringList& comboStrings)
{
	StringList::const_iterator it;
	int index = 0;
	for (it = comboStrings.begin(); it != comboStrings.end(); ++it) {
		StringList path;
		splitStringList(&path, it->c_str(), '\\');
		int level = 0;
		QMenu* item = &createItem;
		for(int level = 0; level < int(path.size()); ++level){
			const char* leaf = path[level].c_str();
			if(level == path.size() - 1){
				addAction(*item, leaf, index++);
			}
			else{
				if (QMenu* menu = item->findChild<QMenu*>(leaf))
					item = menu;
				else
					item = addMenu(*item, leaf);
			}
		}
	}
}

void ClassMenuItemAdder::addAction(QMenu& menu, const char* text, int index)
{
	menu.addAction(text)->setEnabled(false);
}

QMenu* ClassMenuItemAdder::addMenu(QMenu& menu, const char* text)
{
	QMenu* result = menu.addMenu(text);
	result->setObjectName(text);
	return result;
}


// ---------------------------------------------------------------------------

SERIALIZATION_CLASS(PropertyRow, PropertyRowPointer, "SharedPtr");

PropertyRowPointer::PropertyRowPointer()
: factory_(0)
, searchHandle_(0)
{
}

void PropertyRowPointer::setDerivedType(const TypeID& typeID, Serialization::IClassFactory* factory)
{
	if (!factory) {
		derivedTypeName_.clear();
		return;
	}
	const Serialization::TypeDescription* desc = factory->descriptionByType(typeID);
	if (!desc) {
		derivedTypeName_.clear();
		return;
	}
	derivedTypeName_ = desc->name();
}

TypeID PropertyRowPointer::getDerivedType(Serialization::IClassFactory* factory) const
{
	if (!factory)
		return TypeID();
	return factory->findTypeByName(derivedTypeName_.c_str());
}

bool PropertyRowPointer::assignTo(Serialization::IPointer &ptr)
{
	Serialization::TypeID derivedType = getDerivedType(ptr.factory());
	if ( ptr.type() != derivedType ) {
		ptr.create(derivedType);
	}

	return true;
}


void CreatePointerMenuHandler::onMenuCreateByIndex()
{
	tree->model()->rowAboutToBeChanged(row);
	if(index < 0){ // NULL value
		row->setDerivedType(TypeID(), 0);
		row->clear();
	}
	else{
		const PropertyDefaultTypeValue* defaultValue = tree->model()->defaultType(row->baseType(), index);
		SharedPtr<PropertyRow> clonedDefault = defaultValue->root->clone(tree->model()->constStrings());
		if (defaultValue && defaultValue->root) {
			YASLI_ASSERT(defaultValue->root->refCount() == 1);
			if(useDefaultValue){
				row->clear();
				row->swapChildren(clonedDefault, 0);
			}
			row->setDerivedType(defaultValue->type, row->factory());
			row->setLabelChanged();
			row->setLabelChangedToChildren();
			tree->expandRow(row);
		}
		else{
			row->setDerivedType(TypeID(), 0);
			row->clear();
		}
	}
	tree->model()->rowChanged(row);
}


string PropertyRowPointer::valueAsString() const
{
	string result;
	const Serialization::TypeDescription* desc = 0;
	if (factory_)
		desc = factory_->descriptionByType(getDerivedType(factory_));
	if (desc)
		result = desc->label();
	else
		result = derivedTypeName_;

	return result;
}

wstring PropertyRowPointer::generateLabel() const
{
	if(multiValue())
		return L"...";

	wstring str;
	if(!derivedTypeName_.empty()){
		const char* textStart = derivedTypeName_.c_str();
		if (factory_) {
			const Serialization::TypeDescription* desc = factory_->descriptionByType(getDerivedType(factory_));

			if (desc)
				textStart = desc->label();
		}
		const char* p = textStart + strlen(textStart);
		while(p > textStart){
			if(*(p - 1) == '\\')
				break;
			--p;
		}
		str = toWideChar(p);
		if(p != textStart){
			str += L" (";
			str += toWideChar(string(textStart, p - 1).c_str());
			str += L")";
		}
	}
	else
	{
		if (factory_)
			str = toWideChar(factory_->nullLabel() ? factory_->nullLabel() : "[ null ]");
		else
			str = L"[ null ]";

	}
	return str;
}

void PropertyRowPointer::redraw(const PropertyDrawContext& context)
{
	QRect widgetRect = context.widgetRect;
	QRect rt = widgetRect;
	rt.adjust(-1, 0, 0, 1);
	wstring str = generateLabel();
	const QFont* font = derivedTypeName_.empty() ? &context.tree->font() : &context.tree->_boldFont();
	int buttonFlags = BUTTON_POPUP_ARROW;
	if (userReadOnly())
		buttonFlags |= BUTTON_DISABLED;
	if (context.pressed)
		buttonFlags |= BUTTON_PRESSED;
	context.drawButton(rt, str.c_str(), buttonFlags, font);
}

struct ClassMenuItemAdderRowPointer : ClassMenuItemAdder{
	ClassMenuItemAdderRowPointer(PropertyRowPointer* row, QPropertyTree* tree) : row_(row), tree_(tree) {}    
	void addAction(QMenu& menu, const char* text, int index)
	{
		CreatePointerMenuHandler* handler = new CreatePointerMenuHandler;
		tree_->addMenuHandler(handler);
		handler->row = row_;
		handler->tree = tree_;
		handler->index = index;
		handler->useDefaultValue = !tree_->immediateUpdate();

		QAction* action = menu.addAction(text);

		QObject::connect(action, SIGNAL(triggered()), handler, SLOT(onMenuCreateByIndex()));
	}
protected:
	PropertyRowPointer* row_;
	QPropertyTree* tree_;
};


bool PropertyRowPointer::onActivate( QPropertyTree* tree, bool force)
{
		if(userReadOnly())
				return false;
		QMenu menu;
		ClassMenuItemAdderRowPointer(this, tree).generateMenu(menu, tree->model()->typeStringList(baseType()));
	tree->_setPressedRow(this);
		menu.exec(tree->_toScreen(QPoint(widgetPos_, pos_.y() + tree->_defaultRowHeight())));
	tree->_setPressedRow(0);
		return true;
}

bool PropertyRowPointer::onMouseDown(QPropertyTree* tree, QPoint point, bool& changed) 
{
		if(widgetRect(tree).contains(point)){
				if(onActivate(tree, false))
						changed = true;
		}
		return false; 
}

bool PropertyRowPointer::onContextMenu(QMenu &menu, QPropertyTree* tree)
{
	if(!menu.isEmpty())
		menu.addSeparator();
		if(!userReadOnly()){
			QMenu* createItem = menu.addMenu("Set");
			ClassMenuItemAdderRowPointer(this, tree).generateMenu(*createItem, tree->model()->typeStringList(baseType()));
		}
	return PropertyRow::onContextMenu(menu, tree);
}

void PropertyRowPointer::serializeValue(IArchive& ar)
{
	ar(derivedTypeName_, "derivedTypeName", "Derived Type Name");
}

int PropertyRowPointer::widgetSizeMin(const QPropertyTree* tree) const
{
	QFontMetrics fm(tree->_boldFont());
	QString str(fromWideChar(generateLabel().c_str()).c_str());
  return fm.width(str) + 24;
}

void PropertyRowPointer::setValueAndContext(const Serialization::IPointer& ptr, Serialization::IArchive& ar)
{
	baseType_ = ptr.baseType();
	factory_ = ptr.factory();
	serializer_ = ptr.serializer();
	pointerType_ = ptr.pointerType();
	searchHandle_ = ptr.handle();
	const Serialization::TypeDescription* desc = factory_->descriptionByType(ptr.type());
	if (desc)
		derivedTypeName_ = desc->name();
	else
		derivedTypeName_.clear();
}

#include <QPropertyTree/moc_PropertyRowPointer.cpp>
// vim:ts=4 sw=4:
