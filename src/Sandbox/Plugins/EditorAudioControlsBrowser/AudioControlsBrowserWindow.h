////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 1999-2014.
// -------------------------------------------------------------------------
//  File name:   AudioControlsBrowserWindow.h
//  Version:     v1.00
//  Created:     07/04/2014 by Gabriel Rodriguez Hernandez.
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#include <QMainWindow>
#include <IEditor.h>
#include "ui_AudioControlsBrowserMainWindow.h"
#include "ATLControlsModel.h"

namespace AudioControls
{
	class CATLControlsModel;
	class IAudioSystemEditor;
	class CATLControlsPanel;
	class CInspectorPanel;
	class CAudioSystemPanel;
	class CATLControl;

	class CAudioControlsBrowserWindow : public QMainWindow, public Ui::MainWindow, public IATLControlModelListener, public IEditorNotifyListener
	{
		Q_OBJECT
	public:
		CAudioControlsBrowserWindow();
		~CAudioControlsBrowserWindow();
		virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);

	private slots:
		void Reload();
		void Save();
		void UpdateFilterFromSelection();
		void UpdateInspector();
		void FilterControlType(EACBControlType type, bool bShow);

	protected:
		void keyPressEvent(QKeyEvent* pEvent);
		void closeEvent(QCloseEvent* pEvent);

		// IATLControlModelListener
		virtual void OnControlAdded(CATLControl* pControl);
		virtual void OnControlModified(CATLControl* pControl);
		virtual void OnControlRemoved(CATLControl* pControl);

	private:
		void Update();
		void SetModelModified(bool modified);
		void UpdateAudioSystemData();

		CATLControlsModel* m_pATLModel;
		IAudioSystemEditor* m_pAudioSystemImpl;

		bool m_bModelModified;

		CATLControlsPanel* m_pATLControlsPanel;
		CInspectorPanel* m_pInspectorPanel;
		CAudioSystemPanel* m_pAudioSystemPanel;
	};
}