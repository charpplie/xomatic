#pragma once

#include <memory>
#include <QWidget>
#include <QItemSelection>
#include <Serialization/IArchive.h>
#include "../EditorCommon/QPropertyTree/QPropertyTree.h"

class QToolButton;
class QTreeView;
class QLineEdit;
class QPushButton;
class QString;
class QItemSelection;
class QModelIndex;
class QAbstractItemModel;
class QMenu;
class QDockWidget;

namespace Serialization { class IArchive; }

namespace CharacterTool {

class CharacterToolForm;
class Explorer;
class ExplorerModel;
class ExplorerActionHandler;
struct System;
class ExplorerFilterProxyModel;
struct ExplorerAction;
struct ExplorerEntry;

struct FilterOptions
{
	bool inPak;
	bool onDisk;
	bool onlyNew;
	bool withAudio;
	bool withoutAudio;

	FilterOptions()
	: inPak(true)
	, onDisk(true)
	, onlyNew(false)
	, withAudio(true) 
	, withoutAudio(true) 
	{
	}

	void Serialize(Serialization::IArchive& ar)
	{
		if (ar.OpenBlock("files", "Files:"))
		{
			ar(inPak, "inPak", "^In Pak");
			ar(onDisk, "onDisk", "^On Disk");
			ar(onlyNew, "onlyNew", "^Only New");
			ar.CloseBlock();
		}

		if (ar.OpenBlock("audioEvents", "Audio Events:"))
		{
			ar(withAudio, "withAudio", "^With");
			ar(withoutAudio, "withoutAudio", "^Without");
			ar.CloseBlock();
		}
	}
};

class ExplorerPanel : public QWidget
{
	Q_OBJECT
public:
	ExplorerPanel(QWidget* parent, System* system, QMainWindow* mainWindow);
	~ExplorerPanel();
	void SetDockWidget(QDockWidget* dockWidget);
	void SetRootIndex(int rootIndex);
	int RootIndex() const{ return m_explorerRootIndex; }

	void Serialize(Serialization::IArchive& ar);
	QSize sizeHint() const override { return QSize(240, 400); }
public slots:
	void OnExplorerSelectionChanged();
	void OnFilterTextChanged(const QString& str);
	void OnTreeSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
	void OnHeaderContextMenu(const QPoint& pos);
	void OnHeaderColumnToggle();
	void OnContextMenu(const QPoint& pos);
	void OnMenuCopyName();
	void OnMenuCopyPath();
	void OnMenuPasteSelection();
	void OnActivated(const QModelIndex& index);
	void OnExplorerAction(const ExplorerAction& action);
	void OnEntryImported(ExplorerEntry* entry, ExplorerEntry* oldEntry);	
	void OnEntryLoaded(ExplorerEntry* entry);	
	void OnRootButtonPressed();
	void OnRootSelected(bool);
	void OnExplorerEndReset();
	void OnCharacterLoaded();
	void OnRefreshFilter();
	void OnFilterButtonToggled(bool filterMode);
	void OnFilterOptionsChanged();
protected:
	bool eventFilter(QObject* sender, QEvent* ev) override;
private:
	void ExecuteExplorerAction(const ExplorerAction& action);
	void SetTreeViewModel(QAbstractItemModel* model);
	void FillAnimations();
	void UpdateRootMenu();
	void ExpandTree();

	FilterOptions m_filterOptions;
	QDockWidget* m_dockWidget;
	QTreeView* m_treeView;
	System* m_system;
	ExplorerModel* m_model;
	ExplorerFilterProxyModel* m_filterModel;
	QLineEdit* m_filterEdit;
	QPushButton* m_rootButton;
	QToolButton* m_filterButton;
	QMenu* m_rootMenu;

	std::vector<QAction*> m_rootMenuActions;
	int m_explorerRootIndex;
	bool m_ignoreTreeSelectionChange;
	bool m_filterMode;
	std::vector<std::unique_ptr<ExplorerActionHandler> > m_explorerActionHandlers;
	QPropertyTree* m_filterOptionsTree;
	CharacterToolForm* m_mainWindow;
};

}
