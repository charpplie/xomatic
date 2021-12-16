#pragma once

#include "PropertyRowField.h"
#include "QPropertyTree.h"
#include "PropertyTreeModel.h"
#include "Unicode.h"

#include <QLineEdit>

class PropertyRowString : public PropertyRowField
{
public:
	bool isLeaf() const override{ return true; }
	bool isStatic() const override{ return false; }
	bool assignTo(string& str) const;
	bool assignTo(wstring& str) const;
	void setValue(const char* str, const void* handle, const Serialization::TypeID& typeId);
	void setValue(const wchar_t* str, const void* handle, const Serialization::TypeID& typeId);
	PropertyRowWidget* createWidget(QPropertyTree* tree);
	string valueAsString() const;
	wstring valueAsWString() const { return value_; }
	WidgetPlacement widgetPlacement() const override{ return WIDGET_VALUE; }
	void serializeValue(Serialization::IArchive& ar) override;
	const wstring& value() const{ return value_; }
	bool assignToByPointer(void* instance, const Serialization::TypeID& type) const;
protected:
	wstring value_;
};

class PropertyRowWidgetString : public PropertyRowWidget
{
	Q_OBJECT
public:
  PropertyRowWidgetString(PropertyRowString* row, QPropertyTree* tree)
	: PropertyRowWidget(row, tree)
	, entry_(new QLineEdit())
	, tree_(tree)
	{
		initialValue_ = QString(fromWideChar(row->value().c_str()).c_str());
		entry_->setText(initialValue_);
		entry_->selectAll();
		connect(entry_.data(), SIGNAL(editingFinished()), this, SLOT(onEditingFinished()));
	}
	~PropertyRowWidgetString()
	{
		entry_->hide();
		entry_->setParent(0);
		entry_.take()->deleteLater();
	}

	void commit(){
		onEditingFinished();
	}
	QWidget* actualWidget() { return entry_.data(); }

	public slots:
	void onEditingFinished(){
		PropertyRowString* row = static_cast<PropertyRowString*>(this->row());
		if(initialValue_ != entry_->text() || row_->multiValue()){
			model()->rowAboutToBeChanged(row);
			vector<wchar_t> str;
			QString text = entry_->text();
			str.resize(text.size() + 1, L'\0');
			if (!text.isEmpty())
				text.toWCharArray(&str[0]);
			row->setValue(&str[0], row->searchHandle(), row->typeId());
			model()->rowChanged(row);
		}
		else
			tree_->_cancelWidget();
	}
protected:
    QPropertyTree* tree_;
	QScopedPointer<QLineEdit> entry_;
    QString initialValue_;
};
