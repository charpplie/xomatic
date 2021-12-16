// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#pragma once

#include <QWidget>
#include "IAudioConnectionInspectorPanel.h"
#include "AudioSystemEditor_sdlmixer.h"
#include "ui_ConnectionInspector.h"

namespace AudioControls
{
	class CSDLMixerConnection;

	class CAudioConnectionInspectorPanel_sdlmixer : public IAudioConnectionInspectorPanel, public Ui::CAudioConnectionInspectorPanel_sdlmixer
	{
		Q_OBJECT
	public:
		CAudioConnectionInspectorPanel_sdlmixer::CAudioConnectionInspectorPanel_sdlmixer();
		virtual ~CAudioConnectionInspectorPanel_sdlmixer() {}
		virtual void UpdateControl(IAudioConnection* pConnection, EACBControlType eATLControlType = eACBT_NUM_TYPES);

	private slots:
		void UpdateConnection();

	signals:
		void ConnectionChanged();

	private:
		CSDLMixerConnection* m_pConnection;

	};
}