#pragma once

#include "EditorCommonAPI.h"
#include <QDialog>

class DeepFilterProxyModel;
class QLineEdit;
class QModelIndex;
class QStandardItemModel;
class QStandardItem;
class QString;
class QTreeView;
class QWidget;

class EDITOR_COMMON_API ListSelectionDialog : public QDialog
{
	Q_OBJECT
public:
	ListSelectionDialog(QWidget* parent);
	void SetColumnText(int column, const char* text);
	void SetColumnWidth(int column, int width);
	
	void AddRow(const char* firstColumnValue);
	void AddRow(const char* firstColumnValue, QIcon& icon);
	void AddRowColumn(const char* value);

	const char* ChooseItem(const char* currentValue);

	QSize sizeHint() const override;
protected slots:
	void onActivated(const QModelIndex& index);
	void onFilterChanged(const QString&);
protected:
	bool eventFilter(QObject* obj, QEvent *event);
private:
	QTreeView* m_tree;
	QStandardItemModel* m_model;
	DeepFilterProxyModel* m_filterModel;
	typedef std::map<string, QStandardItem*, stl::less_stricmp<string> > StringToItem;
	StringToItem m_firstColumnToItem;
	QLineEdit* m_filterEdit;
	string m_chosenItem;
	int m_currentColumn;
};

