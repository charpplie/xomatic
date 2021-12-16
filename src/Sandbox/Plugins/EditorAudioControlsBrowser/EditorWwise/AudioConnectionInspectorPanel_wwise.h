// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#pragma once

#include <QWidget>
#include "IAudioConnectionInspectorPanel.h"
#include "AudioSystemEditor_wwise.h"

class QVBoxLayout;
class QHBoxLayout;
class QLabel;
class QLineEdit;

namespace AudioControls
{
	class CWwiseConnection;

	class CAudioConnectionInspectorPanel_wwise : public IAudioConnectionInspectorPanel
	{
		Q_OBJECT
	public:
		CAudioConnectionInspectorPanel_wwise::CAudioConnectionInspectorPanel_wwise();
		virtual ~CAudioConnectionInspectorPanel_wwise() {}
		virtual void UpdateControl(IAudioConnection* pConnection, EACBControlType eATLControlType = eACBT_NUM_TYPES);

	private slots:
		void UpdateConnection();

	signals:
		void ConnectionChanged();

	private:
		CWwiseConnection* m_pConnection;

		QVBoxLayout* m_pLayout;

		// Shift
		QHBoxLayout* m_pShiftLayout;
		QLabel* m_pShiftLabel;
		QLineEdit* m_pShiftLineEdit;

		// Mult
		QHBoxLayout* m_pMultLayout;
		QLabel* m_pMultiplicationLabel;
		QLineEdit* m_pMultiplicationLineEdit;

		// Layout
		QHBoxLayout* m_pValueLayout;
		QLabel* m_pValueLabel;
		QLineEdit* m_pValueLineEdit;
	};
}