////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 1999-2014.
// -------------------------------------------------------------------------
//  File name:   AudioControlsBrowserWindow.cpp
//  Version:     v1.00
//  Created:     07/04/2014 by Gabriel Rodriguez Hernandez.
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "AudioControlsBrowserWindow.h"
#include "AudioControlsBrowserPlugin.h"
#include "QAudioControlTreeWidgetDelegate.h"
#include "QAudioControlBrowserIcons.h"
#include "ATLControlsModel.h"
#include <IAudioSystem.h>
#include "AudioControlsBrowserUndo.h"
#include "ATLControlsPanel.h"
#include "InspectorPanel.h"
#include "AudioSystemPanel.h"

#include <QPaintEvent>
#include <QPushButton>
#include <QApplication>
#include <QPainter>
#include <QMessageBox>

namespace AudioControls
{
	//---------------------------------------------
	CAudioControlsBrowserWindow::CAudioControlsBrowserWindow()
		: m_bModelModified(false)
	{
		setupUi(this);

		m_pATLModel = CAudioControlsBrowserPlugin::GetATLModel();
		m_pAudioSystemImpl = CAudioControlsBrowserPlugin::GetAudioSystemEditorImpl();

		m_pATLControlsPanel = new CATLControlsPanel(m_pATLModel, m_pAudioSystemImpl);
		m_pInspectorPanel = new CInspectorPanel(m_pATLModel, m_pAudioSystemImpl);
		m_pAudioSystemPanel = new CAudioSystemPanel(m_pAudioSystemImpl);
		m_pSplitter->insertWidget(0, m_pAudioSystemPanel);
		m_pSplitter->insertWidget(0, m_pInspectorPanel);
		m_pSplitter->insertWidget(0, m_pATLControlsPanel);

		SetModelModified(false);
		Update();

		connect(m_pATLControlsPanel, SIGNAL(SelectedControlChanged()), this, SLOT(UpdateInspector()));
		connect(m_pATLControlsPanel, SIGNAL(SelectedControlChanged()), this, SLOT(UpdateFilterFromSelection()));
		connect(m_pATLControlsPanel, SIGNAL(ControlTypeFiltered(EACBControlType, bool)), this, SLOT(FilterControlType(EACBControlType, bool)));

		m_pATLModel->AddListener(this);
		GetIEditor()->RegisterNotifyListener(this);
	}

	CAudioControlsBrowserWindow::~CAudioControlsBrowserWindow()
	{
		m_pATLModel->RemoveListener(this);
		GetIEditor()->UnregisterNotifyListener(this);
	}

	void CAudioControlsBrowserWindow::keyPressEvent(QKeyEvent* pEvent)
	{

		uint16 mod = pEvent->modifiers();
		if (pEvent->key() == Qt::Key_S && pEvent->modifiers() == Qt::ControlModifier)
		{
			Save();
		}
		else if (pEvent->key() == Qt::Key_Z && (pEvent->modifiers() & Qt::ControlModifier))
		{
			if (pEvent->modifiers() & Qt::ShiftModifier)
			{
				GetIEditor()->Redo();
			}
			else
			{
				GetIEditor()->Undo();
			}
		}
		QMainWindow::keyPressEvent(pEvent);
	}

	void CAudioControlsBrowserWindow::closeEvent(QCloseEvent* pEvent)
	{
		if (m_bModelModified)
		{
			QMessageBox messageBox;
			messageBox.setText(tr("There are unsaved changes."));
			messageBox.setInformativeText(tr("Do you want to save your changes?"));
			messageBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
			messageBox.setDefaultButton(QMessageBox::Save);
			messageBox.setWindowTitle("Audio Controls Browser");
			switch (messageBox.exec())
			{
			case QMessageBox::Save:
				QApplication::setOverrideCursor(Qt::WaitCursor);
				Save();
				QApplication::restoreOverrideCursor();
				pEvent->accept();
				break;
			case QMessageBox::Discard:
				pEvent->accept();
				break;
			default:
				pEvent->ignore();
				break;
			}
		}
		else
		{
			pEvent->accept();
		}
	}

	void CAudioControlsBrowserWindow::Reload()
	{
		bool bReload = true;
		if (m_bModelModified)
		{
			QMessageBox messageBox;
			messageBox.setText(tr("If you reload you will lose all your unsaved changes."));
			messageBox.setInformativeText(tr("Are you sure you want to reload?"));
			messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
			messageBox.setDefaultButton(QMessageBox::No);
			messageBox.setWindowTitle("Audio Controls Browser");
			bReload = (messageBox.exec() == QMessageBox::Yes);
		}

		if (bReload)
		{
			CAudioControlsBrowserPlugin::ReloadModels();
			SetModelModified(false);
			Update();
		}
	}

	void CAudioControlsBrowserWindow::Update()
	{
		m_pATLControlsPanel->Reload();
		m_pAudioSystemPanel->Reload();

		QPreloadRequestTreeWidgetDelegate::ClearGroups();
		int size = m_pATLModel->GetConnectionGroupCount();
		for (int i = 0; i < size; ++i)
		{
			QPreloadRequestTreeWidgetDelegate::AddGroupName(m_pATLModel->GetConnectionGroupAt(i), GetGroupIcon(i));
		}
		SetModelModified(false);
	}

	void CAudioControlsBrowserWindow::Save()
	{
		int numPreloadsModified = 0;
		string preloadName = "";
		int size = m_pATLModel->ControlCount();
		for (int i = 0; i < size; ++i)
		{
			CATLControl* pControl = m_pATLModel->GetControlByIndex(i);
			if (pControl && pControl->IsModified() && pControl->GetType() == eACBT_PRELOADS)
			{
				if (numPreloadsModified == 0)
				{
					preloadName = pControl->GetName();
				}
				++numPreloadsModified;
			}
		}

		CAudioControlsBrowserPlugin::SaveModels();
		UpdateAudioSystemData();
		SetModelModified(false);

		// if preloads have been modified, ask the user if s/he wants to refresh the audio system
		if (numPreloadsModified > 0)
		{
			QMessageBox messageBox;
			if (numPreloadsModified == 1)
			{
				messageBox.setText(tr("The preload request \"" + preloadName + "\" has been modified. \n\nFor the new data to be loaded the audio system needs to be refreshed, this will stop all currently playing audio. Do you want to do this now?. \n\nYou can always refresh manually at a later time through the Audio menu."));
			}
			else
			{
				messageBox.setText(tr("Several preload requests have been modified. \n\nFor the new data to be loaded the audio system needs to be refreshed, this will stop all currently playing audio. Do you want to do this now?. \n\nYou can always refresh manually at a later time through the Audio menu."));
			}
			messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
			messageBox.setDefaultButton(QMessageBox::No);
			messageBox.setWindowTitle("Audio Controls Browser");
			if (messageBox.exec() == QMessageBox::Yes)
			{
				SAudioRequest oAudioRequestData;
				char const* sLevelName = GetIEditor()->GetLevelName();

				if (_stricmp(sLevelName, "Untitled") == 0)
				{
					// Rather pass NULL to indicate that no level is loaded!
					sLevelName= NULL;
				}

				SAudioManagerRequestData<eAMRT_REFRESH_AUDIO_SYSTEM> oAMData(sLevelName);
				oAudioRequestData.nFlags	= eARF_PRIORITY_HIGH | eARF_EXECUTE_BLOCKING;
				oAudioRequestData.pData		= &oAMData;
				gEnv->pAudioSystem->PushRequest(oAudioRequestData);
			}
		}
	}

	void CAudioControlsBrowserWindow::UpdateInspector()
	{
		m_pInspectorPanel->SetSelectedControls(m_pATLControlsPanel->GetSelectedIds());
	}

	void CAudioControlsBrowserWindow::UpdateFilterFromSelection()
	{
		bool bAllSameType = true;
		EACBControlType selectedType = eACBT_NUM_TYPES;
		std::vector<AudioControls::CID> ids = m_pATLControlsPanel->GetSelectedIds();
		size_t size = ids.size();
		for (size_t i = 0; i < size; ++i)
		{
			CATLControl* pControl = m_pATLModel->GetControlByID(ids[i]);
			if (pControl)
			{
				if (selectedType == eACBT_NUM_TYPES)
				{
					selectedType = pControl->GetType();
				}
				else if (selectedType != pControl->GetType())
				{
					bAllSameType = false;
				}
			}
		}


		// If the selected item is a folder show all controls
		// If not, show only controls compatible with the one selected
		if (selectedType == eACBT_NUM_TYPES)
		{
			// Selected a folder
			for (int i = 0; i < eACBT_NUM_TYPES; ++i)
			{
				m_pAudioSystemPanel->SetAllowedControls((EACBControlType)i, true);
			}
		}
		else
		{
			for (int i = 0; i < eACBT_NUM_TYPES; ++i)
			{
				EACBControlType type = (EACBControlType)i;
				if (bAllSameType)
				{
					bool bVisible = true;
					if (selectedType != type)
					{
						if ((selectedType == eACBT_SWITCH))
						{
							// allow switch -> rtpc connections
							if (type != eACBT_RTPC)
							{
								bVisible = false;
							}
						}
						else if (selectedType == eACBT_ENVIRONMENTS)
						{
							// allow environment -> rtpc
							if (type != eACBT_RTPC)
							{
								bVisible = false;
							}
						}
						else
						{
							bVisible = false;
						}
					}
					m_pAudioSystemPanel->SetAllowedControls(type, bVisible);
				}
				else
				{
					m_pAudioSystemPanel->SetAllowedControls(type, false);
				}
			}
		}
	}

	void CAudioControlsBrowserWindow::SetModelModified(bool modified)
	{
		m_bModelModified = modified;
	}

	void CAudioControlsBrowserWindow::UpdateAudioSystemData()
	{
		SAudioRequest oConfigDataRequest;
		oConfigDataRequest.nFlags = eARF_PRIORITY_HIGH;

		//clear the AudioSystem control config data
		SAudioManagerRequestData<eAMRT_CLEAR_CONTROLS_DATA> oClearRequestData(eADS_ALL);
		oConfigDataRequest.pData = &oClearRequestData;
		gEnv->pAudioSystem->PushRequest(oConfigDataRequest);

		//parse the AudioSystem global config data
		string sControlPath(gEnv->pAudioSystem->GetConfigPath());
		SAudioManagerRequestData<eAMRT_PARSE_CONTROLS_DATA> oParseGloabalRequestData(sControlPath.c_str(), eADS_GLOBAL);
		oConfigDataRequest.pData = &oParseGloabalRequestData;
		gEnv->pAudioSystem->PushRequest(oConfigDataRequest);

		//parse the AudioSystem level-specific config data
		string sLevelName = GetIEditor()->GetLevelName();
		sControlPath += "levels/" + sLevelName;
		SAudioManagerRequestData<eAMRT_PARSE_CONTROLS_DATA> oParseLevelRequestData(sControlPath.c_str(), eADS_LEVEL_SPECIFIC);
		oConfigDataRequest.pData = &oParseLevelRequestData;
		gEnv->pAudioSystem->PushRequest(oConfigDataRequest);
	}

	void CAudioControlsBrowserWindow::OnControlModified(CATLControl* pControl)
	{
		SetModelModified(true);
	}

	void CAudioControlsBrowserWindow::OnControlAdded(CATLControl* pControl)
	{
		SetModelModified(true);
	}

	void CAudioControlsBrowserWindow::OnControlRemoved(CATLControl* pControl)
	{
		SetModelModified(true);
	}

	void CAudioControlsBrowserWindow::OnEditorNotifyEvent(EEditorNotifyEvent event)
	{
		if (event ==  eNotify_OnEndSceneSave)
		{
			CAudioControlsBrowserPlugin::ReloadScopes();
			m_pInspectorPanel->Update();
		}
	}

	void CAudioControlsBrowserWindow::FilterControlType(EACBControlType type, bool bShow)
	{
		m_pAudioSystemPanel->SetAllowedControls(type, bShow);
	}
}

#include <moc_AudioControlsBrowserWindow.cpp>