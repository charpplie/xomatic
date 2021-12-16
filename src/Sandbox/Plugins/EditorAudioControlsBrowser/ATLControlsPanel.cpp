// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.

#include "StdAfx.h"
#include "ATLControlsPanel.h"
#include "AudioControl.h"
#include "ATLControlsModel.h"
#include "QAudioControlBrowserIcons.h"
#include "AudioLibrary.h"
#include <IEditor.h>
#include <CryFile.h>
#include <CryPath.h>
#include <Cry_Camera.h>
#include <IAudioSystem.h>
#include "common/IAudioSystemEditor.h"
#include "common/IAudioSystemControl.h"

#include <QWidgetAction>
#include <QPushButton>
#include <QPaintEvent>
#include <QPainter>
#include <QMessageBox>
#include <QMimeData>

namespace AudioControls
{
	class QFilterButton : public QPushButton
	{
	public:
		QFilterButton(const QIcon& icon, const QString& text, QWidget* parent = 0) : QPushButton(icon, text, parent)
		{
			setStyleSheet("text-align: left;");
			setCheckable(true);
			setFlat(true);
		}
	protected:
		void paintEvent(QPaintEvent* event)
		{
			QPushButton::paintEvent(event);
			if (isChecked())
			{
				QPainter painter(this);
				const int heightPadding = 4;
				const int widthPadding = 3;
				painter.setPen(QPen(QApplication::palette().color(QPalette::Highlight), 2));
				painter.drawLine(QPoint(event->rect().width() - widthPadding, heightPadding),
				                 QPoint(event->rect().width() - widthPadding, event->rect().height() - heightPadding));
			}
		}
	};

	CATLControlsPanel::CATLControlsPanel(CATLControlsModel* pATLModel, IAudioSystemEditor* pAudioSystemEditor)
		:	m_pATLModel(pATLModel)
		,	m_pAudioSystemImpl(pAudioSystemEditor)
		,	m_pIAudioProxy(gEnv->pAudioSystem->GetFreeAudioProxy())
		,	m_nAudioTriggerID(INVALID_AUDIO_CONTROL_ID)
	{
		setupUi(this);

		// ********** Control List Behaviour ***************
		m_pInternalControlTree->viewport()->installEventFilter(this);

		m_pInternalControlTree->header()->setStretchLastSection(false);
		m_pInternalControlTree->header()->setSectionResizeMode(0, QHeaderView::ResizeMode::Stretch);
		m_pInternalControlTree->header()->setSectionResizeMode(1, QHeaderView::ResizeMode::Fixed);
		m_pInternalControlTree->header()->setSectionResizeMode(2, QHeaderView::ResizeMode::Fixed);
		m_pInternalControlTree->header()->setSectionResizeMode(3, QHeaderView::ResizeMode::Fixed);
		m_pInternalControlTree->header()->setSectionResizeMode(4, QHeaderView::ResizeMode::Fixed);
		m_pInternalControlTree->header()->setSectionResizeMode(5, QHeaderView::ResizeMode::Fixed);

		const int platformColumnSize = 38;
		m_pInternalControlTree->header()->resizeSection(1, platformColumnSize);
		m_pInternalControlTree->header()->resizeSection(2, platformColumnSize);
		m_pInternalControlTree->header()->resizeSection(3, platformColumnSize);
		m_pInternalControlTree->header()->resizeSection(4, platformColumnSize);
		m_pInternalControlTree->header()->resizeSection(5, platformColumnSize);
		connect(m_pInternalControlTree, SIGNAL(itemSelectionChanged()), this, SLOT(UpdatePreloadColumns()));
		// ***********************************************

		// ************ Context Menu ************
		m_addItemMenu.addAction(GetControlTypeIcon(eACBT_TRIGGER), tr("Trigger"), this, SLOT(CreateTriggerControl()));
		m_addItemMenu.addAction(GetControlTypeIcon(eACBT_RTPC), tr("RTPC"), this, SLOT(CreateRTPCControl()));
		m_addItemMenu.addAction(GetControlTypeIcon(eACBT_SWITCH), tr("Switch"), this, SLOT(CreateSwitchControl()));
		m_addItemMenu.addAction(GetControlTypeIcon(eACBT_ENVIRONMENTS), tr("Environment"), this, SLOT(CreateEnvironmentsControl()));
		m_addItemMenu.addAction(GetControlTypeIcon(eACBT_PRELOADS), tr("Preload"), this, SLOT(CreatePreloadControl()));
		m_addItemMenu.addSeparator();
		m_addItemMenu.addAction(GetFolderIcon(), tr("Folder"), this, SLOT(CreateFolder()));
		m_pAddButton->setMenu(&m_addItemMenu);
		m_pInternalControlTree->setContextMenuPolicy(Qt::CustomContextMenu);
		connect(m_pInternalControlTree, SIGNAL(customContextMenuRequested(const QPoint&)), SLOT(ShowControlsContextMenu(const QPoint&)));
		// *********************************

		// ************ Filtering ************
		m_filter.SetTree(m_pInternalControlTree);
		m_filter.AddFilter(&m_nameFilter);
		m_filter.AddFilter(&m_typeFilter);
		int margin = 5;
		for (int i = 0; i < eACBT_NUM_TYPES; ++i)
		{
			EACBControlType type = (EACBControlType)i;
			QWidgetAction* pWidgetAction = new QWidgetAction(this);
			QWidget* pWidget = new QWidget(this);
			QHBoxLayout* pLayout = new QHBoxLayout(pWidget);
			pLayout->setSpacing(1);
			m_pControlTypeFilterButtons[type] = new QFilterButton(GetControlTypeIcon(type), "", this);
			m_pControlTypeFilterButtons[type]->setChecked(true);
			pLayout->addWidget(m_pControlTypeFilterButtons[type]);
			pLayout->setContentsMargins(margin, margin, margin, 0);
			pWidget->setLayout(pLayout);
			pWidgetAction->setDefaultWidget(pWidget);
			m_filterMenu.addAction(pWidgetAction);
		}
		m_pFiltersButton->setMenu(&m_filterMenu);
		m_pControlTypeFilterButtons[eACBT_TRIGGER]->setText("Triggers");
		m_pControlTypeFilterButtons[eACBT_RTPC]->setText("RTPCs");
		m_pControlTypeFilterButtons[eACBT_SWITCH]->setText("Switches");
		m_pControlTypeFilterButtons[eACBT_ENVIRONMENTS]->setText("Environments");
		m_pControlTypeFilterButtons[eACBT_PRELOADS]->setText("Preloads");
		connect(m_pInternalListFilter, SIGNAL(textChanged(QString)), this, SLOT(SetATLNameFilter(QString)));
		connect(m_pControlTypeFilterButtons[eACBT_TRIGGER], SIGNAL(clicked(bool)), this, SLOT(ShowTriggers(bool)));
		connect(m_pControlTypeFilterButtons[eACBT_RTPC], SIGNAL(clicked(bool)), this, SLOT(ShowRTPCs(bool)));
		connect(m_pControlTypeFilterButtons[eACBT_SWITCH], SIGNAL(clicked(bool)), this, SLOT(ShowSwitches(bool)));
		connect(m_pControlTypeFilterButtons[eACBT_ENVIRONMENTS], SIGNAL(clicked(bool)), this, SLOT(ShowEnvironments(bool)));
		connect(m_pControlTypeFilterButtons[eACBT_PRELOADS], SIGNAL(clicked(bool)), this, SLOT(ShowPreloads(bool)));
		//	*********************************

		m_pInternalControlTree->SetModel(m_pATLModel);

		connect(m_pInternalControlTree, SIGNAL(DeleteItem(QAudioControlTreeWidget*)), this, SLOT(DeleteControlTreeItem(QAudioControlTreeWidget*)));
		connect(m_pInternalControlTree, SIGNAL(itemSelectionChanged()), this, SIGNAL(SelectedControlChanged()));
		connect(m_pInternalControlTree, SIGNAL(itemSelectionChanged()), this, SLOT(StopControlExecution()));
		connect(m_pInternalControlTree, SIGNAL(ParentChanged(QTreeWidgetItem*)), this, SLOT(UpdateChildrenPaths(QTreeWidgetItem*)));
		connect(m_pInternalControlTree, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(ItemChanged(QTreeWidgetItem*, int)));

		m_pATLModel->AddListener(this);
		UpdatePreloadColumns();

		// Init preview with engine audio system
		if (m_pIAudioProxy != NPTR)
		{
			m_pIAudioProxy->Initialize("ACB trigger preview");
			m_pIAudioProxy->SetObstructionCalcType(eAOOCT_IGNORE);
		}
	}

	CATLControlsPanel::~CATLControlsPanel()
	{
		if (m_pIAudioProxy != NPTR)
		{
			StopControlExecution();
			m_pIAudioProxy->Release();
		}
		m_pATLModel->RemoveListener(this);
	}

	bool CATLControlsPanel::eventFilter(QObject* pObject, QEvent* pEvent)
	{
		if (pEvent->type() == QEvent::Drop)
		{
			// If the drop comes from outside the ATL Control Panel we need to handle it manually
			// If it's an internal drop let the tree handle the internal move
			QDropEvent* pDropEvent = static_cast<QDropEvent*>(pEvent);
			if (pDropEvent)
			{
				if (pDropEvent->source() != m_pInternalControlTree)
				{
					const QMimeData* pData = pDropEvent->mimeData();
					QString format = "application/x-qabstractitemmodeldatalist";
					if (pData->hasFormat(format))
					{
						QByteArray encoded = pData->data(format);
						QDataStream stream(&encoded, QIODevice::ReadOnly);
						while (!stream.atEnd())
						{
							int row, col;
							QMap<int,  QVariant> roleDataMap;
							stream >> row >> col >> roleDataMap;
							if (!roleDataMap.isEmpty())
							{
								QVariant userRoleData = roleDataMap[Qt::UserRole];
								SAudioImplControlMimeData externalData = userRoleData.value<SAudioImplControlMimeData>();
								string filename = "";
								string path = "";
								QModelIndex index = m_pInternalControlTree->indexAt(pDropEvent->pos());
								if (!index.isValid() || !m_pInternalControlTree->visualRect(index).contains(pDropEvent->pos()))
								{
									// dropped at the root, need to create a new folder to hold the control
									QTreeWidgetItem* pFolderItem = m_pInternalControlTree->AddFolder("new_folder", nullptr);
									if (pFolderItem)
									{
										filename = QtUtil::ToString(pFolderItem->text(0));
									}
								}
								else
								{
									QVariant variant = index.data(Qt::UserRole);
									SAudioSystemControlMimeData data = variant.value<SAudioSystemControlMimeData>();
									if (data.m_type == EItemType::eIT_FOLDER || data.m_type == EItemType::eIT_SWITCH)
									{
										GetPathAndFilenameFromItem(m_pInternalControlTree, m_pInternalControlTree->itemAt(pDropEvent->pos()), path, filename);
									}
									else if (data.m_type == EItemType::eIT_ITEM)
									{
										CATLControl* pTargetControl = m_pATLModel->GetControlByID(data.m_id);
										if (pTargetControl)
										{
											CAudioLibrary* pLibrary = pTargetControl->GetLibrary();
											if (pLibrary)
											{
												filename = pLibrary->GetName();
											}
											path = pTargetControl->GetVirtualPath();
										}
									}

									if (externalData.m_controlType == eACBT_SWITCH)
									{
										IAudioSystemControl* pAudioSystemControl = m_pAudioSystemImpl->GetControlByID(externalData.m_id);
										if (pAudioSystemControl)
										{
											string virtualpath = pAudioSystemControl->GetVirtualPath();
											string::size_type pos = virtualpath.find_last_of("/");
											if (pos != string::npos)
											{
												path += "/" + virtualpath.substr(0, pos + 1);
											}
											else
											{
												path += "/" + virtualpath;
											}
										}
									}
								}

								string controlName = QtUtil::ToString(roleDataMap[Qt::DisplayRole].toString());
								if (externalData.m_controlType  == eACBT_PRELOADS)
								{
									PathUtil::RemoveExtension(controlName);
								}

								CreateControl(m_pAudioSystemImpl->ImplTypeToATLType(externalData.m_controlType), controlName, path, filename);

								// Connect controls
								if (m_pAudioSystemImpl)
								{
									IAudioSystemControl* pAudioSystemControl = m_pAudioSystemImpl->GetControlByID(externalData.m_id);
									if (pAudioSystemControl)
									{
										CUndo undo("Connected Audio Control to Audio System");
										std::vector<AudioControls::CID> ids = m_pInternalControlTree->GetSelectedIds();
										const size_t size = ids.size();
										for (size_t i = 0; i < size; ++i)
										{
											CATLControl* pControl = m_pATLModel->GetControlByID(ids[i]);
											if (pControl)
											{
												IAudioConnection* pAudioConnection = m_pAudioSystemImpl->CreateConnectionToControl(pAudioSystemControl);
												if (pAudioConnection)
												{
													pControl->AddConnection(m_pAudioSystemImpl->CreateConnectionToControl(pAudioSystemControl));
												}
												m_pATLModel->OnControlModified(pControl);
											}
										}
									}
								}
							}
						}
					}
					pEvent->accept();
				}
			}
		}
		return QWidget::eventFilter(pObject, pEvent);
	}

	std::vector<AudioControls::CID> CATLControlsPanel::GetSelectedIds()
	{
		return m_pInternalControlTree->GetSelectedIds();
	}

	void CATLControlsPanel::UpdatePreloadColumns()
	{
		std::vector<AudioControls::CID> ids = m_pInternalControlTree->GetSelectedIds();
		size_t size = ids.size();
		if (size > 0)
		{
			bool bShowColumns = true;
			for (size_t i = 0; i < size; ++i)
			{
				CATLControl* pControl = m_pATLModel->GetControlByID(ids[i]);
				if (pControl)
				{
					if (pControl->GetType() != eACBT_PRELOADS)
					{
						bShowColumns = false;
						break;
					}
				}
				else
				{
					bShowColumns = false;
					break;
				}
			}
			ShowPreloadColumns(bShowColumns);
		}
		else
		{
			ShowPreloadColumns(false);
		}
	}

	void CATLControlsPanel::ShowPreloadColumns(bool bShow)
	{
		if (bShow)
		{
			m_pInternalControlTree->header()->showSection(1);
			m_pInternalControlTree->header()->showSection(2);
			m_pInternalControlTree->header()->showSection(3);
			m_pInternalControlTree->header()->showSection(4);
			m_pInternalControlTree->header()->showSection(5);
		}
		else
		{
			m_pInternalControlTree->header()->hideSection(1);
			m_pInternalControlTree->header()->hideSection(2);
			m_pInternalControlTree->header()->hideSection(3);
			m_pInternalControlTree->header()->hideSection(4);
			m_pInternalControlTree->header()->hideSection(5);
		}
	}

	void CATLControlsPanel::Reload()
	{
		m_pInternalControlTree->Refresh();

		// Remove filters if the control added is hidden
		for (int i = 0; i < eACBT_NUM_TYPES; ++i)
		{
			EACBControlType controlType = (EACBControlType)i;
			m_typeFilter.SetControlTypeHidden(controlType, false);
			m_pControlTypeFilterButtons[controlType]->setChecked(true);
		}

		m_nameFilter.SetFilter("");
		m_pInternalListFilter->setText("");

		m_filter.ApplyFilter();
	}

	void CATLControlsPanel::SetATLNameFilter(QString filter)
	{
		m_nameFilter.SetFilter(filter);
		m_filter.ApplyFilter();
	}

	void CATLControlsPanel::ShowControlType(EACBControlType type, bool bShow, bool bExclusive)
	{
		if (bExclusive)
		{
			for (int i = 0; i < eACBT_NUM_TYPES; ++i)
			{
				EACBControlType controlType = (EACBControlType)i;
				m_typeFilter.SetControlTypeHidden(controlType, bShow);
				ControlTypeFiltered(controlType, !bShow);
				m_pControlTypeFilterButtons[controlType]->setChecked(!bShow);
			}
		}
		m_typeFilter.SetControlTypeHidden(type, !bShow);
		ControlTypeFiltered(type, bShow);
		m_pControlTypeFilterButtons[type]->setChecked(bShow);

		m_filter.ApplyFilter();
	}

	void CATLControlsPanel::ShowTriggers(bool bShow)
	{
		ShowControlType(eACBT_TRIGGER, bShow, QGuiApplication::keyboardModifiers() & Qt::ControlModifier);
	}

	void CATLControlsPanel::ShowRTPCs(bool bShow)
	{
		ShowControlType(eACBT_RTPC, bShow, QGuiApplication::keyboardModifiers() & Qt::ControlModifier);
	}

	void CATLControlsPanel::ShowEnvironments(bool bShow)
	{
		ShowControlType(eACBT_ENVIRONMENTS, bShow, QGuiApplication::keyboardModifiers() & Qt::ControlModifier);
	}

	void CATLControlsPanel::ShowSwitches(bool bShow)
	{
		ShowControlType(eACBT_SWITCH, bShow, QGuiApplication::keyboardModifiers() & Qt::ControlModifier);
	}

	void CATLControlsPanel::ShowPreloads(bool bShow)
	{
		ShowControlType(eACBT_PRELOADS, bShow, QGuiApplication::keyboardModifiers() & Qt::ControlModifier);
	}

	void CATLControlsPanel::CreateRTPCControl()
	{
		CreateControlNextToSelected(eACBT_RTPC, "rtpc", "new_folder");
	}

	void CATLControlsPanel::CreateSwitchControl()
	{
		string filename = "";
		string path = "";

		QTreeWidgetItem* pItem = m_pInternalControlTree->currentItem();
		GetPathAndFilenameFromItem(m_pInternalControlTree, pItem, path, filename);
		if (filename == "")
		{
			filename = "new_folder";
		}

		EItemType type = m_pInternalControlTree->GetItemType(pItem);

		if (type == EItemType::eIT_FOLDER)
		{
			path += "/" + m_pInternalControlTree->GenerateUniqueName("switch", pItem);
		}
		else
		{
			path += "/" + m_pInternalControlTree->GenerateUniqueName("switch", pItem->parent());
		}

		CreateControl(eACBT_SWITCH, "state", path, filename);
	}

	void CATLControlsPanel::CreateStateControl()
	{
		string filename = "";
		string path = "";
		QTreeWidgetItem* pItem = m_pInternalControlTree->currentItem();
		if (m_pInternalControlTree->GetItemType(pItem) == EItemType::eIT_ITEM)
		{
			pItem = pItem->parent();
		}

		// concatenate the names of all the ancestors to form the path
		// the last item is the filename (and is not part of the path)
		while (pItem)
		{
			QTreeWidgetItem* pParent = pItem->parent();
			if (pParent)
			{
				if (!path.empty())
				{
					path =  QtUtil::ToString(pItem->text(0)) + "/" + path;
				}
				else
				{
					path =  QtUtil::ToString(pItem->text(0));
				}
			}
			else
			{
				filename = QtUtil::ToString(pItem->text(0));
			}
			pItem = pParent;
		}
		if (filename.compare("") == 0)
		{
			filename = "new_folder";
		}
		CreateControl(eACBT_SWITCH, "state", path, filename);
	}

	void CATLControlsPanel::CreateTriggerControl()
	{
		CreateControlNextToSelected(eACBT_TRIGGER, "trigger", "new_folder");
	}

	void CATLControlsPanel::CreateEnvironmentsControl()
	{
		CreateControlNextToSelected(eACBT_ENVIRONMENTS, "environment", "new_folder");
	}

	void CATLControlsPanel::CreatePreloadControl()
	{
		CreateControlNextToSelected(eACBT_PRELOADS, "preload", "new_folder");
	}

	void CATLControlsPanel::CreateFolderAsChild()
	{
		CreateFolder(true);
	}

	void CATLControlsPanel::CreateFolder(bool bAddAsChildOfSelected /*= false*/)
	{
		QTreeWidgetItem* pFolder = NULL;
		if (bAddAsChildOfSelected)
		{
			pFolder = m_pInternalControlTree->AddFolder("new_folder", m_pInternalControlTree->currentItem());
		}
		else
		{
			QTreeWidgetItem* pParent = m_pInternalControlTree->currentItem();
			if (pParent)
			{
				pParent = pParent->parent();
			}
			pFolder = m_pInternalControlTree->AddFolder("new_folder", pParent);
		}

		if (pFolder)
		{
			m_pInternalControlTree->setCurrentItem(pFolder, 0);
			m_pInternalControlTree->scrollToItem(pFolder);
			m_pInternalControlTree->editItem(pFolder);
		}
	}

	void CATLControlsPanel::CreateControlNextToSelected(EACBControlType type, const string& name, const string& folder)
	{
		string filename = "";
		string path = "";
		GetPathAndFilenameFromItem(m_pInternalControlTree, m_pInternalControlTree->currentItem(), path, filename);
		if (filename.compare("") == 0)
		{
			filename = folder;
		}
		CreateControl(type, name, path, filename);
	}

	void CATLControlsPanel::GetPathAndFilenameFromItem(QAudioControlTreeWidget* pTree, QTreeWidgetItem* pItem, string& path, string& filename)
	{
		filename = "";
		path = "";

		// look for the first folder up the hierarchy
		while (pItem && pTree->GetItemType(pItem) != EItemType::eIT_FOLDER)
		{
			pItem = pItem->parent();
		}

		// concatenate the names of all the ancestors to form the path
		// the last item is the filename (and is not part of the path)
		while (pItem)
		{
			QTreeWidgetItem* pParent = pItem->parent();
			if (pParent)
			{
				if (path.empty())
				{
					path =  QtUtil::ToString(pItem->text(0));

				}
				else
				{
					path =  QtUtil::ToString(pItem->text(0)) + "/" + path;
				}
			}
			else
			{
				filename = QtUtil::ToString(pItem->text(0));
			}
			pItem = pParent;
		}
	}

	void CATLControlsPanel::CreateControl(EACBControlType type, const string& name, const string& virtualpath, const string& filename)
	{
		string finalName = m_pATLModel->GenerateUniqueName(name, type, "", virtualpath);
		m_pATLModel->CreateControlInLibrary(finalName, type, filename, "libs/gameaudio", virtualpath);
	}

	void CATLControlsPanel::ShowControlsContextMenu(const QPoint& pos)
	{
		QTreeWidgetItem* pItem = m_pInternalControlTree->currentItem();
		if (pItem)
		{
			QMenu contextMenu(tr("Context menu"), this);

			if (m_pInternalControlTree->GetControlType(pItem) == eACBT_TRIGGER)
			{
				contextMenu.addAction(tr("Execute Trigger"), this, SLOT(ExecuteControl()));
				contextMenu.addSeparator();
			}

			// Add control menu
			QMenu addMenu(tr("Add"));
			bool bIsState = false;
			EItemType type = m_pInternalControlTree->GetItemType(pItem);
			if (type == EItemType::eIT_SWITCH)
			{
				addMenu.addAction(GetPropertyIcon(), tr("State"), this, SLOT(CreateStateControl()));
			}
			else if (type == EItemType::eIT_ITEM)
			{
				QTreeWidgetItem* pParent = pItem->parent();
				if (pParent && m_pInternalControlTree->GetItemType(pParent) == EItemType::eIT_SWITCH)
				{
					bIsState = true;
				}
			}

			if (bIsState)
			{
				addMenu.addAction(GetPropertyIcon(), tr("State"), this, SLOT(CreateStateControl()));
			}
			else
			{
				addMenu.addSeparator();
				addMenu.addAction(GetControlTypeIcon(eACBT_TRIGGER), tr("Trigger"), this, SLOT(CreateTriggerControl()));
				addMenu.addAction(GetControlTypeIcon(eACBT_RTPC), tr("RTPC"), this, SLOT(CreateRTPCControl()));
				addMenu.addAction(GetControlTypeIcon(eACBT_SWITCH), tr("Switch"), this, SLOT(CreateSwitchControl()));
				addMenu.addAction(GetControlTypeIcon(eACBT_ENVIRONMENTS), tr("Environment"), this, SLOT(CreateEnvironmentsControl()));
				addMenu.addAction(GetControlTypeIcon(eACBT_PRELOADS), tr("Preload"), this, SLOT(CreatePreloadControl()));
				addMenu.addSeparator();
				addMenu.addAction(GetFolderIcon(), tr("Folder"), this, SLOT(CreateFolderAsChild()));
			}
			contextMenu.addMenu(&addMenu);

			QAction* pAction = new QAction(tr("Rename"), this);
			connect(pAction, &QAction::triggered,  [&]()
			{
				QTreeWidgetItem* pItem = m_pInternalControlTree->currentItem();
				m_pInternalControlTree->editItem(pItem);

			});
			contextMenu.addAction(pAction);

			pAction = new QAction(tr("Delete"), this);
			connect(pAction, SIGNAL(triggered()), this, SLOT(DeleteSelectedControl()));
			contextMenu.addAction(pAction);

			contextMenu.addSeparator();
			pAction = new QAction(tr("Expand All"), this);
			connect(pAction, SIGNAL(triggered()), m_pInternalControlTree, SLOT(expandAll()));
			contextMenu.addAction(pAction);
			pAction = new QAction(tr("Collapse All"), this);
			connect(pAction, SIGNAL(triggered()), m_pInternalControlTree, SLOT(collapseAll()));
			contextMenu.addAction(pAction);

			contextMenu.exec(m_pInternalControlTree->mapToGlobal(pos));
		}
	}

	void CATLControlsPanel::DeleteSelectedControl()
	{
		DeleteControlTreeItem(m_pInternalControlTree);
	}

	void CATLControlsPanel::DeleteControlTreeItem(QAudioControlTreeWidget* pTree)
	{
		if (pTree)
		{
			QMessageBox messageBox;

			QList<QTreeWidgetItem*> selected = pTree->selectedItems();
			int size = selected.length();

			if (size > 0)
			{
				if (size == 1)
				{
					messageBox.setText("Are you sure you want to delete \"" + selected[0]->text(0) + "\"?.");
				}
				else
				{
					messageBox.setText("Are you sure you want to delete the " + QString::number(size) + " selected controls?.");
				}
				messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
				messageBox.setDefaultButton(QMessageBox::Yes);
				messageBox.setWindowTitle("Audio Controls Browser");
				if (messageBox.exec() == QMessageBox::Yes)
				{
					std::vector<QTreeWidgetItem*> itemQueue;
					for (int i = 0; i < size; ++i)
					{
						itemQueue.push_back(selected[i]);
					}

					// Get list of all the items to delete (including children of the ones selected)
					std::vector<QTreeWidgetItem*> itemsToDelete;
					while (!itemQueue.empty())
					{
						QTreeWidgetItem* pItemToDelete = itemQueue.back();
						itemQueue.pop_back();

						stl::push_back_unique(itemsToDelete, pItemToDelete);

						const int size = pItemToDelete->childCount();
						for (int i = 0; i < size; ++i)
						{
							stl::push_back_unique(itemQueue, pItemToDelete->child(i));
						}
					}

					// Deleting a QTreeWidgetItem deletes all its children automatically, so we need
					// to sort them from the bottom of the tree upwards so we can then traverse them
					// in that order and delete each one manually (otherwise we need might try to
					// delete items already deleted because we previously deleted their parent)
					std::sort(itemsToDelete.begin(), itemsToDelete.end(), [](const QTreeWidgetItem * left, const QTreeWidgetItem * right) -> bool
					{
						QTreeWidgetItem* pLeftParent = left->parent();
						QTreeWidgetItem* pRightParent = right->parent();

						int leftLevel = 0;
						int rightLevel = 0;
						while (pLeftParent || pRightParent)
						{
							if (pLeftParent)
							{
								++leftLevel;
								pLeftParent = pLeftParent->parent();
							}
							if (pRightParent)
							{
								++rightLevel;
								pRightParent = pRightParent->parent();
							}
						}
						return rightLevel < leftLevel;
					});

					std::vector<QString> names;
					size_t itemCount = itemsToDelete.size();
					for (size_t i = 0; i < itemCount; ++i)
					{
						names.push_back(itemsToDelete[i]->text(0));
					}

					CUndo undo("Audio Control Removed");
					itemCount = itemsToDelete.size();
					for (size_t i = 0; i < itemCount; ++i)
					{
						QTreeWidgetItem* pItemToDelete = itemsToDelete[i];
						if (pTree->GetItemType(pItemToDelete) == EItemType::eIT_ITEM)
						{
							m_pATLModel->RemoveControl(pTree->GetItemId(pItemToDelete));
						}
						else
						{
							delete pItemToDelete;
						}
					}
				}
			}
		}
	}

	void CATLControlsPanel::UpdateChildrenPaths(QTreeWidgetItem* pRoot)
	{
		// calculate the path in the tree
		QTreeWidgetItem* pLastItem = pRoot;
		QTreeWidgetItem* pItem = pRoot->parent();

		string path = "";
		while (pItem)
		{
			if (path.empty())
			{
				path = QtUtil::ToString(pLastItem->text(0));
			}
			else
			{
				path = QtUtil::ToString(pLastItem->text(0)) + "/" + path;

			}
			pLastItem = pItem;
			pItem = pItem->parent();
		}

		// update all the children's path
		int size = pRoot->childCount();
		for (int i = 0; i < size; ++i)
		{
			UpdateChildrenPaths(path, QtUtil::ToString(pLastItem->text(0)), pRoot->child(i));
		}
	}

	void CATLControlsPanel::UpdateChildrenPaths(const string& path, const string& filename, QTreeWidgetItem* pRoot)
	{
		if (m_pInternalControlTree->GetItemType(pRoot) == EItemType::eIT_ITEM)
		{
			CID id = m_pInternalControlTree->GetItemId(pRoot);
			CATLControl* pControl = m_pATLModel->GetControlByID(id);
			if (pControl)
			{
				CAudioLibrary* pLibrary = pControl->GetLibrary();
				if ((pControl->GetVirtualPath() != path) || (pLibrary && (pLibrary->GetName() != filename)))
				{
					pControl->SetVirtualPath(path);
					pLibrary = m_pATLModel->AddLibrary(filename);
					if (pLibrary)
					{
						pControl->SetLibrary(pLibrary);
					}
				}
			}
		}

		string newPath = QtUtil::ToString(pRoot->text(0));
		if (!path.empty())
		{
			newPath = path + "/" + newPath;
		}
		int size = pRoot->childCount();
		for (int i = 0; i < size; ++i)
		{
			UpdateChildrenPaths(newPath, filename, pRoot->child(i));
		}
	}

	void CATLControlsPanel::ItemChanged(QTreeWidgetItem* pItem, int column)
	{
		CID id = m_pInternalControlTree->GetItemId(pItem);
		CATLControl* pControl = m_pATLModel->GetControlByID(id);
		if (pControl)
		{
			if (column == 0)
			{
				CUndo undo("Audio Control Name Changed");
				string newName = QtUtil::ToString(pItem->text(0));
				if (!m_pATLModel->IsNameValid(newName, pControl->GetType(), pControl->GetScope(), pControl->GetVirtualPath()))
				{
					newName = m_pATLModel->GenerateUniqueName(newName, pControl->GetType(), pControl->GetScope(), pControl->GetVirtualPath());
				}
				pControl->SetName(newName);
			}
			else if (m_pInternalControlTree->GetItemType(pItem) == EItemType::eIT_ITEM)
			{
				if (pControl)
				{
					CUndo undo("Audio Control Group Changed for Platform");
					int size = pItem->columnCount();
					for (int i = 1; i < size; ++i)
					{
						int groupId = pItem->data(i, Qt::UserRole).toInt();
						pControl->SetGroupForPlatform(m_pATLModel->GetPlatformAt(i - 1), groupId);
					}
				}
			}
			m_pATLModel->OnControlModified(pControl);
		}
		else
		{
			// changed folder or switch name
			UpdateChildrenPaths(pItem);
		}
	}

	void CATLControlsPanel::OnControlAdded(CATLControl* pControl)
	{
		// Remove filters if the control added is hidden
		EACBControlType controlType = pControl->GetType();
		if (m_typeFilter.IsControlTypeHidden(controlType))
		{
			m_typeFilter.SetControlTypeHidden(controlType, false);
			m_pControlTypeFilterButtons[controlType]->setChecked(true);
		}

		if (!m_nameFilter.IsNameValid(QtUtil::ToQString(pControl->GetName())))
		{
			m_nameFilter.SetFilter("");
			m_pInternalListFilter->setText("");
		}

		m_filter.ApplyFilter();
	}

	void CATLControlsPanel::OnControlModified(CATLControl* pControl)
	{
		m_pInternalControlTree->UpdateControl(pControl);
		m_pInternalControlTree->Refresh(false);
	}

	void CATLControlsPanel::ExecuteControl()
	{
		QTreeWidgetItem* const pItem = m_pInternalControlTree->currentItem();

		if (pItem != NPTR)
		{
			AudioControls::CID const nAudioControlID = m_pInternalControlTree->GetItemId(pItem);
			CATLControl const* const pControl = m_pATLModel->GetControlByID(nAudioControlID);

			if (pControl != NPTR && m_pIAudioProxy != NPTR)
			{
				StopControlExecution();
				gEnv->pAudioSystem->GetAudioTriggerID(pControl->GetName().c_str(), m_nAudioTriggerID);

				if (m_nAudioTriggerID != INVALID_AUDIO_CONTROL_ID)
				{
					CCamera const& camera = GetIEditor()->GetSystem()->GetViewCamera();
					m_pIAudioProxy->SetPosition(SATLWorldPosition(camera.GetMatrix(), ZERO));
					m_pIAudioProxy->ExecuteTrigger(m_nAudioTriggerID, eLSM_None);
				}
			}
		}
	}

	void CATLControlsPanel::StopControlExecution()
	{
		if (m_nAudioTriggerID != INVALID_AUDIO_CONTROL_ID)
		{
			m_pIAudioProxy->StopTrigger(m_nAudioTriggerID);
			m_nAudioTriggerID = INVALID_AUDIO_CONTROL_ID;
		}
	}
}

#include <moc_ATLControlsPanel.cpp>
