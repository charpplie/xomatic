#pragma once
#include <QtCore/QAbstractItemModel>

namespace CharacterTool
{

struct ExplorerEntry;
struct ExplorerEntryModifyEvent;
class Explorer;
class ExplorerModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	ExplorerModel(Explorer* list, QObject* parent);

	void SetRootByIndex(int index);
	int GetRootIndex() const;
	ExplorerEntry* GetActiveRoot() const;

	static ExplorerEntry* GetEntry(const QModelIndex& index);
	QModelIndex index(int row, int column, const QModelIndex& parent) const override;

	int rowCount(const QModelIndex& parent) const override;
	int columnCount(const QModelIndex& parent) const override;

	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	bool hasChildren(const QModelIndex &parent) const override;
	QVariant data(const QModelIndex& index, int role) const override;
	QModelIndex ModelIndexFromEntry(ExplorerEntry* entry, int column) const;
	QModelIndex NewModelIndexFromEntry(ExplorerEntry* entry, int column) const;
	QModelIndex parent(const QModelIndex& index) const override;

public slots:
	void OnEntryModified(ExplorerEntryModifyEvent& ev);
	void OnBeginAddEntry(ExplorerEntry* entry);
	void OnEndAddEntry();
	void OnBeginRemoveEntry(ExplorerEntry* entry);
	void OnEndRemoveEntry();
protected:

	bool m_addWithinActiveRoot;
	bool m_removeWithinActiveRoot;
	int m_rootIndex;
	int m_rootSubtree;
	Explorer* m_explorer;
};

}
