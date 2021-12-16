#pragma once

#include <QDialog>

class DeepFilterProxyModel;
class QLabel;
class QLineEdit;
class QModelIndex;
class QStandardItemModel;
class QString;
class QTreeView;
class QWidget;
struct IDefaultSkeleton;

class JointSelectionDialog : public QDialog
{
	Q_OBJECT
public:
	JointSelectionDialog(QWidget* parent);
	QSize sizeHint() const override;
	
	bool chooseJoint(string* name, IDefaultSkeleton* skeleton);
protected slots:
	void onActivated(const QModelIndex& index);
	void onFilterChanged(const QString&);
protected:
	bool eventFilter(QObject* obj, QEvent *event);
private:
	QTreeView* m_tree;
	QStandardItemModel* m_model;
	DeepFilterProxyModel* m_filterModel;
	QLineEdit* m_filterEdit;
	QLabel* m_skeletonLabel;
};

