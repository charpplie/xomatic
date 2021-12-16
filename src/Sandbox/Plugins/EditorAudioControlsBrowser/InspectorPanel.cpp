// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.

#include "StdAfx.h"
#include "InspectorPanel.h"
#include "QtUtil.h"
#include "QAudioControlBrowserIcons.h"
#include "AudioControlMimeData.h"
#include <IEditor.h>
#include "AudioControlsBrowserPlugin.h"
#include "common/IAudioSystemControl.h"
#include "common/IAudioConnectionInspectorPanel.h"

#include <QMessageBox>
#include <QMimeData>
#include <QDropEvent>
#include <QKeyEvent>

using namespace QtUtil;

namespace AudioControls
{
	CInspectorPanel::CInspectorPanel(CATLControlsModel* pATLModel, IAudioSystemEditor* pAudioSystemImpl)
		: m_pATLModel(pATLModel)
		, m_pAudioSystemImpl(pAudioSystemImpl)
		, m_selectedType(eACBT_NUM_TYPES)
		, m_notFoundColor(QColor(255, 128, 128))
	{
		assert(m_pATLModel);
		assert(m_pAudioSystemImpl);

		setupUi(this);

		connect(m_pNameLineEditor, SIGNAL(editingFinished()), this, SLOT(FinishedEditingName()));
		connect(m_pLevelDropDown, SIGNAL(activated(QString)), this, SLOT(SetControlScope(QString)));
		connect(m_pConnectionGroupComboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(CurrentConnectionGroupChanged(QString)));
		connect(m_pConnectionList, SIGNAL(itemSelectionChanged()), this, SLOT(SelectedConnectionChanged()));
		connect(m_pAutoLoadCheckBox, SIGNAL(clicked(bool)), this, SLOT(SetAutoLoadForCurrentControl(bool)));

		// context menu
		m_pConnectionList->setContextMenuPolicy(Qt::CustomContextMenu);
		connect(m_pConnectionList, SIGNAL(customContextMenuRequested(const QPoint&)), SLOT(ShowConnectionsContextMenu(const QPoint&)));

		// data validators
		m_pNameLineEditor->setValidator(new QRegExpValidator(QRegExp("^[a-zA-Z0-9_]*$"), m_pNameLineEditor));

		m_pConnectionGroupComboBox->clear();
		int size = m_pATLModel->GetConnectionGroupCount();
		for (int i = 0; i < size; ++i)
		{
			m_pConnectionGroupComboBox->addItem(GetGroupIcon(i), m_pATLModel->GetConnectionGroupAt(i).c_str());
		}

		m_pConnectionList->viewport()->installEventFilter(this);
		m_pConnectionList->installEventFilter(this);

		m_pATLModel->AddListener(this);

		m_pAudioConnectionInspector = pAudioSystemImpl->NewConnectionInspectorPanel();
		if (m_pAudioConnectionInspector)
		{
			connect(m_pAudioConnectionInspector, SIGNAL(ConnectionChanged()), this, SLOT(CurrentConnectionModified()));
			m_pConnectionsWidget->layout()->addWidget(m_pAudioConnectionInspector);
			m_pAudioConnectionInspector->UpdateControl(nullptr);
		}
		m_pConnectionsWidget->setHidden(false);

		Update();
	}

	CInspectorPanel::~CInspectorPanel()
	{
		m_pATLModel->RemoveListener(this);
	}

	void CInspectorPanel::Update()
	{
		UpdateScopeList();
		UpdateInspector();
		UpdateConnectionList();
	}

	void CInspectorPanel::SetSelectedControls(std::vector<CID> selectedControls)
	{
		m_selectedType = eACBT_NUM_TYPES;
		m_selectedControls.clear();
		size_t size = selectedControls.size();
		for (size_t i = 0; i < size; ++i)
		{
			CATLControl* pControl = m_pATLModel->GetControlByID(selectedControls[i]);
			if (pControl)
			{
				m_selectedControls.push_back(pControl);
				m_selectedType = pControl->GetType();
			}
		}

		size = m_selectedControls.size();
		for (size_t i = 0; i < size; ++i)
		{
			if (m_selectedControls[i]->GetType() != m_selectedType)
			{
				m_selectedType = eACBT_NUM_TYPES;
				break;
			}
		}
		UpdateInspector();
		UpdateConnectionList();
	}

	void CInspectorPanel::UpdateInspector()
	{
		size_t size = m_selectedControls.size();
		if (size > 0)
		{
			m_pInspectorControls->setHidden(false);

			if (m_selectedType != eACBT_NUM_TYPES)
			{
				m_pInspectorAudioWidgets->setHidden(false);

				if (size > 1)
				{
					m_pNameLineEditor->setText(" <" +  QString::number(m_selectedControls.size()) + tr(" items selected>"));
					m_pNameLineEditor->setEnabled(false);
					m_pConnectionsWidget->setHidden(true);
					m_pPlatformGroupsWidget->setHidden(true);
					m_pLevelDropDown->setCurrentIndex(-1);

					if (m_selectedType == eACBT_PRELOADS)
					{
						m_pAutoLoadWidget->setHidden(false);
						m_pAutoLoadCheckBox->setChecked(false);
					}
					else
					{
						m_pAutoLoadWidget->setHidden(true);
					}
				}
				else
				{
					CATLControl* pControl = m_selectedControls[0];
					if (pControl)
					{
						m_pNameLineEditor->setText(ToQString(pControl->GetName()));
						m_pNameLineEditor->setEnabled(true);
						m_pConnectionsWidget->setHidden(false);

						// Control scope (global or level specific?)
						QString scope = QtUtil::ToQString(pControl->GetScope());
						if (scope == "")
						{
							m_pLevelDropDown->setCurrentIndex(0);
						}
						else
						{
							int index = m_pLevelDropDown->findText(scope);
							m_pLevelDropDown->setCurrentIndex(index);
						}

						// Preloads connections can be changed depending on platform groups
						if (pControl->GetType() == eACBT_PRELOADS)
						{
							m_pPlatformGroupsWidget->setHidden(false);
							m_pAutoLoadWidget->setHidden(false);
							m_pAutoLoadCheckBox->setChecked(pControl->IsAutoLoad());
						}
						else
						{
							m_pPlatformGroupsWidget->setHidden(true);
							m_pAutoLoadWidget->setHidden(true);
						}
					}
				}
			}
			else
			{
				// items of different types selected
				m_pInspectorControls->setHidden(false);
				m_pInspectorAudioWidgets->setHidden(true);
				m_pNameLineEditor->setText(" <" +  QString::number(m_selectedControls.size()) + tr(" items selected>"));
				m_pNameLineEditor->setEnabled(false);
			}
		}
		else
		{
			m_pInspectorControls->setHidden(true);
		}
	}

	void CInspectorPanel::UpdateConnectionList()
	{
		m_pConnectionList->clear();

		bool bEnableConnectionList = false;

		if (m_selectedControls.size() == 1)
		{
			CATLControl* pControl = m_selectedControls[0];
			if (pControl)
			{
				bEnableConnectionList = true;
				const size_t size = pControl->ConnectionCount();
				for (size_t j = 0; j < size; ++j)
				{
					IAudioConnection* pConnection = pControl->GetConnectionAt(j);
					if (pConnection)
					{
						IAudioSystemControl* pAudioSystemControl = pConnection->GetControl();
						if (pAudioSystemControl)
						{
							const string sGroup = pConnection->GetGroup();
							if (sGroup == "" || sGroup == ToString(m_pConnectionGroupComboBox->currentText()))
							{
								const TImplControlType nType = pAudioSystemControl->GetType();

								QListWidgetItem* pListItem = nullptr;
								pListItem = new QListWidgetItem(m_pAudioSystemImpl->GetTypeIcon(nType), QString(pAudioSystemControl->GetName()));
								pListItem->setData(Qt::UserRole, pAudioSystemControl->GetId());
								if (pAudioSystemControl->IsPlaceholder())
								{
									pListItem->setToolTip(tr("Control not found in currently loaded audio system project"));
									pListItem->setForeground(m_notFoundColor);
								}
								m_pConnectionList->insertItem(0, pListItem);
							}
						}
					}
				}
			}
		}
		m_pConnectionList->setEnabled(bEnableConnectionList);
	}

	void CInspectorPanel::UpdateScopeList()
	{
		m_pLevelDropDown->clear();
		for (int j = 0; j < m_pATLModel->GetScopeCount(); ++j)
		{
			SControlScope scope = m_pATLModel->GetScopeAt(j);
			m_pLevelDropDown->insertItem(0, QString(scope.name));
			if (scope.bOnlyLocal)
			{
				m_pLevelDropDown->setItemData(0, m_notFoundColor, Qt::ForegroundRole);
				m_pLevelDropDown->setItemData(0, "Level not found but it is referenced by some audio controls", Qt::ToolTipRole);
			}
			else
			{
				m_pLevelDropDown->setItemData(0, "", Qt::ToolTipRole);
			}
		}
		m_pLevelDropDown->insertItem(0, tr("Global"));
	}

	void CInspectorPanel::SetControlName(QString name)
	{
		if (m_selectedControls.size() == 1)
		{
			CUndo undo("Audio Control Name Changed");
			string newName = QtUtil::ToString(name);
			CATLControl* pControl = m_selectedControls[0];
			if (pControl && pControl->GetName() != newName)
			{
				if (!m_pATLModel->IsNameValid(newName, pControl->GetType(), pControl->GetScope(), pControl->GetVirtualPath()))
				{
					newName = m_pATLModel->GenerateUniqueName(newName, pControl->GetType(), pControl->GetScope(), pControl->GetVirtualPath());
				}
				pControl->SetName(newName);
				m_pATLModel->OnControlModified(pControl);
			}
		}
	}

	void CInspectorPanel::SetControlScope(QString scope)
	{
		CUndo undo("Audio Control Scope Changed");
		size_t size = m_selectedControls.size();
		for (size_t i = 0; i < size; ++i)
		{
			CATLControl* pControl = m_selectedControls[i];
			if (pControl)
			{
				QString currentScope = QtUtil::ToQString(pControl->GetScope());
				if (currentScope != scope && (scope != tr("Global") || currentScope != ""))
				{
					if (scope == tr("Global"))
					{
						pControl->SetScope("");
					}
					else
					{
						pControl->SetScope(QtUtil::ToString(scope));
					}
					m_pATLModel->OnControlModified(pControl);
				}
			}
		}
	}

	void CInspectorPanel::SetAutoLoadForCurrentControl(bool bAutoLoad)
	{
		CUndo undo("Audio Control Auto-Load Property Changed");
		size_t size = m_selectedControls.size();
		for (size_t i = 0; i < size; ++i)
		{
			CATLControl* pControl = m_selectedControls[i];
			if (pControl)
			{
				pControl->SetAutoLoad(bAutoLoad);
				m_pATLModel->OnControlModified(pControl);
			}
		}
	}

	void CInspectorPanel::FinishedEditingName()
	{
		SetControlName(m_pNameLineEditor->text());
	}

	void CInspectorPanel::CurrentConnectionGroupChanged(QString group)
	{
		UpdateConnectionList();
	}

	void CInspectorPanel::RemoveSelectedConnection()
	{
		CUndo undo("Disconnected Audio Control from Audio System");
		if (m_selectedControls.size() == 1)
		{
			CATLControl* pControl = m_selectedControls[0];
			if (pControl)
			{
				QMessageBox messageBox;
				messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
				messageBox.setDefaultButton(QMessageBox::Yes);
				messageBox.setWindowTitle("Audio Controls Browser");
				QList<QListWidgetItem*> selected = m_pConnectionList->selectedItems();
				int size = selected.length();
				if (size == 1)
				{
					messageBox.setText("Are you sure you want to delete the connection between \"" + QtUtil::ToQString(pControl->GetName()) + "\" and \"" + selected[0]->text() + "\"?");
				}
				else
				{
					messageBox.setText("Are you sure you want to delete the " + QString::number(size) + " selected connections?");
				}
				if (messageBox.exec() == QMessageBox::Yes)
				{
					for (int i = 0; i < size; ++i)
					{
						CID nAudioSystemControlID = selected[i]->data(Qt::UserRole).toInt();
						IAudioSystemControl* pAudioSystemControl = m_pAudioSystemImpl->GetControlByID(nAudioSystemControlID);
						if (pAudioSystemControl)
						{
							pControl->RemoveConnectionTo(pAudioSystemControl);
							delete selected[i];
						}
					}
				}
			}
		}
	}

	void CInspectorPanel::SelectedConnectionChanged()
	{
		QList<QListWidgetItem*> selected = m_pConnectionList->selectedItems();
		if (selected.length() == 1)
		{
			QListWidgetItem* pCurrent = selected[0];
			if (pCurrent)
			{
				if (m_selectedControls.size() == 1)
				{

					CID externalId = pCurrent->data(Qt::UserRole).toInt();
					CATLControl* pControl = m_selectedControls[0];
					if (pControl)
					{
						IAudioConnection* pConnection = pControl->GetConnectionTo(externalId);
						if (pConnection)
						{
							m_pAudioConnectionInspector->UpdateControl(pConnection, pControl->GetType());
							return;
						}
					}
				}
			}
		}
		m_pAudioConnectionInspector->UpdateControl(nullptr);
	}

	void CInspectorPanel::ShowConnectionsContextMenu(const QPoint& pos)
	{
		QMenu contextMenu(tr("Context menu"), this);
		contextMenu.addAction(tr("Remove Connection"), this, SLOT(RemoveSelectedConnection()));
		contextMenu.exec(m_pConnectionList->mapToGlobal(pos));
	}

	void CInspectorPanel::OnControlModified(AudioControls::CATLControl* pControl)
	{
		size_t size = m_selectedControls.size();
		for (size_t i = 0; i < size; ++i)
		{
			if (m_selectedControls[i] == pControl)
			{
				UpdateInspector();
				break;
			}
		}
	}

	bool CInspectorPanel::eventFilter(QObject* pObject, QEvent* pEvent)
	{
		if (pEvent->type() == QEvent::Drop)
		{
			QDropEvent* pDropEvent = static_cast<QDropEvent*>(pEvent);
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
						if (userRoleData.userType() == qMetaTypeId<SAudioImplControlMimeData>())
						{
							SAudioImplControlMimeData customData = userRoleData.value<SAudioImplControlMimeData>();
							IAudioSystemControl* pControl = m_pAudioSystemImpl->GetControlByID(customData.m_id);
							if (pControl)
							{
								const size_t size = m_selectedControls.size();
								for (size_t i = 0; i < size; ++i)
								{
									IAudioSystemControl* pAudioSystemControl = m_pAudioSystemImpl->GetControlByID(customData.m_id);
									if (pAudioSystemControl)
									{
										if (m_selectedControls[i]->GetType() == eACBT_PRELOADS)
										{
											ConnectControls(m_selectedControls[i], pAudioSystemControl, ToString(m_pConnectionGroupComboBox->currentText()));
										}
										else
										{
											ConnectControls(m_selectedControls[i], pAudioSystemControl);
										}
									}
								}
							}
						}
					}
				}
			}
			return true;
		}
		else if (pEvent->type() == QEvent::KeyPress)
		{
			QKeyEvent* pDropEvent = static_cast<QKeyEvent*>(pEvent);
			if (pDropEvent && pDropEvent->key() == Qt::Key_Delete && pObject == m_pConnectionList)
			{
				RemoveSelectedConnection();
				return true;
			}
		}
		return QWidget::eventFilter(pObject, pEvent);
	}

	void CInspectorPanel::ConnectControls(CATLControl* pATLControl, IAudioSystemControl* pAudioSystemControl, const string& group)
	{
		CUndo undo("Connected Audio Control to Audio System");
		if (pATLControl && pAudioSystemControl && m_pAudioSystemImpl)
		{
			IAudioConnection* pConnection = pATLControl->GetConnectionTo(pAudioSystemControl, group);
			if (pConnection == nullptr)
			{

				pConnection = m_pAudioSystemImpl->CreateConnectionToControl(pAudioSystemControl);
				if (pConnection)
				{
					pATLControl->AddConnection(pConnection);
					pConnection->SetGroup(group);
					UpdateConnectionList();
					m_pATLModel->OnControlModified(pATLControl);
				}
			}
			else
			{
				int size = m_pConnectionList->count();
				for (int i = 0; i < size; ++i)
				{
					QListWidgetItem* pItem = m_pConnectionList->item(i);
					CID id = pItem->data(Qt::UserRole).toInt();
					if (id == pAudioSystemControl->GetId())
					{
						m_pConnectionList->clearSelection();
						pItem->setSelected(true);
						m_pConnectionList->setCurrentItem(pItem);
						m_pConnectionList->scrollToItem(pItem);
					}
				}
			}
		}
	}

	void CInspectorPanel::CurrentConnectionModified()
	{
		QListWidgetItem* pConnectionItem = m_pConnectionList->currentItem();
		if (m_selectedControls.size() == 1 && pConnectionItem)
		{
			CID externalId = pConnectionItem->data(Qt::UserRole).toInt();
			CATLControl* pControl = m_selectedControls[0];
			if (pControl)
			{
				pControl->SetModified(true);
			}
		}
	}
}

#include <moc_InspectorPanel.cpp>