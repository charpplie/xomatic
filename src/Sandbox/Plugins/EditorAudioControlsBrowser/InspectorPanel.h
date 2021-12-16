// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#pragma once

#include <QWidget>
#include <QMenu>

#include "ui_InspectorPanel.h"
#include "ATLControlsModel.h"
#include "AudioControl.h"
#include "common/IAudioSystemEditor.h"

namespace AudioControls
{
	class CATLControl;
	class IAudioConnectionInspectorPanel;

	class CInspectorPanel : public QWidget, public Ui::InspectorPanel, public IATLControlModelListener
	{
		Q_OBJECT
	public:
		CInspectorPanel(CATLControlsModel* pATLModel, IAudioSystemEditor* pAudioSystemImpl);
		~CInspectorPanel();
		bool eventFilter(QObject* object, QEvent* event);
		void Update();

	public slots:
		void SetSelectedControls(std::vector<CID> selectedControls);
		
	private slots:
		void SetControlName(QString name);
		void SetControlScope(QString scope);
		void SetAutoLoadForCurrentControl(bool bAutoLoad);
		void FinishedEditingName();
		void CurrentConnectionGroupChanged(QString group);
		void SelectedConnectionChanged();
		void ShowConnectionsContextMenu(const QPoint& pos);
		void RemoveSelectedConnection();
		void CurrentConnectionModified();

	protected:
		// IATLControlModelListener
		virtual void OnControlAdded(AudioControls::CATLControl* pControl) {};
		virtual void OnControlModified(AudioControls::CATLControl* pControl);
		virtual void OnControlRemoved(AudioControls::CATLControl* pControl) {};

	private:
		void UpdateInspector();
		void UpdateConnectionList();
		void UpdateScopeList();
		void ConnectControls(CID internalId, CID externalId, const string& group);
		void ConnectControls(CATLControl* pATLControl, IAudioSystemControl* pMiddlewareControl, const string& group = "");

		CATLControlsModel* m_pATLModel;
		IAudioSystemEditor* m_pAudioSystemImpl;

		EACBControlType m_selectedType;
		std::vector<CATLControl*> m_selectedControls;
		QColor m_notFoundColor;
		IAudioConnectionInspectorPanel* m_pAudioConnectionInspector;
	};
}