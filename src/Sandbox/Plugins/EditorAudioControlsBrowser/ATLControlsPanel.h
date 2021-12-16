// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#pragma once

#include <QWidget>
#include <QMenu>

#include "ui_ATLControlsPanel.h"
#include "AudioControl.h"
#include "QTreeWidgetFilter.h"
#include "AudioControlFilters.h"
#include <IAudioInterfacesCommonData.h>

// Forward declarations
struct IAudioProxy;

namespace AudioControls
{
	class CATLControlsModel;
	class QFilterButton;
	class IAudioSystemEditor;

	class CATLControlsPanel : public QWidget, public Ui::ATLControlsPanel, public IATLControlModelListener
	{
		Q_OBJECT
	public:
		CATLControlsPanel(CATLControlsModel* pATLModel, IAudioSystemEditor* pAudioSystemEditor);
		~CATLControlsPanel();
		bool eventFilter(QObject* pObject, QEvent* pEvent);
		std::vector<AudioControls::CID> GetSelectedIds();
		void Reload();

	protected:
		virtual void OnControlAdded(CATLControl* pControl);
		virtual void OnControlModified(CATLControl* pControl);
		virtual void OnControlRemoved(CATLControl* pControl) {}

	signals:
		void SelectedControlChanged();
		void ControlTypeFiltered(EACBControlType type, bool bShow);

	private:
		void ShowPreloadColumns(bool bShow);
		void ShowControlType(EACBControlType type, bool bShow, bool bExclusive);
		void UpdateChildrenPaths(const string& path, const string& filename, QTreeWidgetItem* pRoot);

	private slots:
		void UpdatePreloadColumns();

		// Filtering
		void SetATLNameFilter(QString filter);
		void ShowTriggers(bool bShow);
		void ShowRTPCs(bool bShow);
		void ShowEnvironments(bool bShow);
		void ShowSwitches(bool bShow);
		void ShowPreloads(bool bShow);

		// Create controls / folders
		void CreateControl(EACBControlType type, const string& name, const string& virtualpath, const string& filename);
		void CreateControlNextToSelected(EACBControlType type, const string& name, const string& folder);
		void CreateRTPCControl();
		void CreateSwitchControl();
		void CreateStateControl();
		void CreateTriggerControl();
		void CreateEnvironmentsControl();
		void CreatePreloadControl();
		void CreateFolder(bool bAddAsChildOfSelected = false);
		void CreateFolderAsChild();

		// Create controls / folders
		void DeleteSelectedControl();
		void DeleteControlTreeItem(QAudioControlTreeWidget* pTree);

		void GetPathAndFilenameFromItem(QAudioControlTreeWidget* pTree, QTreeWidgetItem* pItem, string& path, string& filename);
		void ShowControlsContextMenu(const QPoint& pos);
		void UpdateChildrenPaths(QTreeWidgetItem* pRoot);
		void ItemChanged(QTreeWidgetItem* pItem, int column);
		void ExecuteControl();
		void StopControlExecution();

	private:
		CATLControlsModel* const m_pATLModel;
		IAudioSystemEditor* const m_pAudioSystemImpl;

		// Context Menu
		QMenu m_addItemMenu;

		// Filtering
		QTreeWidgetFilter m_filter;
		SNameFilter m_nameFilter;
		STypeFilter m_typeFilter;
		QMenu m_filterMenu;
		QFilterButton* m_pControlTypeFilterButtons[eACBT_NUM_TYPES];

		// Preview specific AudioSystem data
		IAudioProxy* const m_pIAudioProxy;
		TAudioControlID m_nAudioTriggerID;
	};
}
