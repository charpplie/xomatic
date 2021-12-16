// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.

#include "StdAfx.h"
#include "AudioConnectionInspectorPanel_wwise.h"
#include "IAudioSystemControl.h"
#include "AudioSystemControl_wwise.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QApplication>
#include <QValidator>

namespace AudioControls
{
	CAudioConnectionInspectorPanel_wwise::CAudioConnectionInspectorPanel_wwise() : m_pConnection(nullptr)
	{
		resize(299, 75);

		QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
		sizePolicy.setHorizontalStretch(0);
		sizePolicy.setVerticalStretch(0);

		m_pLayout = new QVBoxLayout(this);
		m_pLayout->setSpacing(6);
		m_pLayout->setContentsMargins(0, 0, 0, 0);

		// Shift
		m_pShiftLayout = new QHBoxLayout();
		m_pShiftLayout->setContentsMargins(0, -1, 0, 0);
		m_pShiftLabel = new QLabel(this);
		m_pShiftLabel->setAlignment(Qt::AlignCenter);
		m_pShiftLabel->setText(QApplication::translate("WwiseConnectionInspector", "Shift", 0));
		m_pShiftLayout->addWidget(m_pShiftLabel);
		m_pShiftLineEdit = new QLineEdit(this);
		sizePolicy.setHeightForWidth(m_pShiftLineEdit->sizePolicy().hasHeightForWidth());
		m_pShiftLineEdit->setSizePolicy(sizePolicy);
		m_pShiftLineEdit->setAlignment(Qt::AlignCenter);
		m_pShiftLayout->addWidget(m_pShiftLineEdit);
		m_pLayout->addLayout(m_pShiftLayout);

		// Mult
		m_pMultLayout = new QHBoxLayout();
		m_pMultLayout->setContentsMargins(0, -1, 0, 0);
		m_pMultiplicationLabel = new QLabel(this);
		m_pMultiplicationLabel->setAlignment(Qt::AlignCenter);
		m_pMultiplicationLabel->setText(QApplication::translate("WwiseConnectionInspector", "Mult", 0));
		m_pMultLayout->addWidget(m_pMultiplicationLabel);
		m_pMultiplicationLineEdit = new QLineEdit(this);
		sizePolicy.setHeightForWidth(m_pMultiplicationLineEdit->sizePolicy().hasHeightForWidth());
		m_pMultiplicationLineEdit->setSizePolicy(sizePolicy);
		m_pMultiplicationLineEdit->setAlignment(Qt::AlignCenter);
		m_pMultLayout->addWidget(m_pMultiplicationLineEdit);
		m_pLayout->addLayout(m_pMultLayout);

		// Value
		m_pValueLayout = new QHBoxLayout();
		m_pValueLayout->setContentsMargins(-1, -1, -1, 0);
		m_pValueLabel = new QLabel(this);
		m_pValueLabel->setAlignment(Qt::AlignCenter);
		m_pValueLabel->setText(QApplication::translate("WwiseConnectionInspector", "Value", 0));
		m_pValueLayout->addWidget(m_pValueLabel);
		m_pValueLineEdit = new QLineEdit(this);
		sizePolicy.setHeightForWidth(m_pValueLineEdit->sizePolicy().hasHeightForWidth());
		m_pValueLineEdit->setSizePolicy(sizePolicy);
		m_pValueLineEdit->setAlignment(Qt::AlignCenter);
		m_pValueLayout->addWidget(m_pValueLineEdit);
		m_pLayout->addLayout(m_pValueLayout);

		// data validators
		m_pShiftLineEdit->setValidator(new QDoubleValidator(m_pShiftLineEdit));
		m_pMultiplicationLineEdit->setValidator(new QDoubleValidator(m_pMultiplicationLineEdit));
		m_pValueLineEdit->setValidator(new QDoubleValidator(m_pValueLineEdit));

		m_pShiftLineEdit->setHidden(true);
		m_pShiftLabel->setHidden(true);
		m_pMultiplicationLineEdit->setHidden(true);
		m_pMultiplicationLabel->setHidden(true);
		m_pValueLineEdit->setHidden(true);
		m_pValueLabel->setHidden(true);

		connect(m_pValueLineEdit, SIGNAL(editingFinished()), this, SLOT(UpdateConnection()));
		connect(m_pShiftLineEdit, SIGNAL(editingFinished()), this, SLOT(UpdateConnection()));
		connect(m_pMultiplicationLineEdit, SIGNAL(editingFinished()), this, SLOT(UpdateConnection()));
	}

	void CAudioConnectionInspectorPanel_wwise::UpdateControl(IAudioConnection* pConnection, EACBControlType eATLControlType)
	{
		m_pShiftLineEdit->setHidden(true);
		m_pShiftLabel->setHidden(true);
		m_pMultiplicationLineEdit->setHidden(true);
		m_pMultiplicationLabel->setHidden(true);
		m_pValueLineEdit->setHidden(true);
		m_pValueLabel->setHidden(true);

		if (pConnection)
		{
			m_pConnection = static_cast<CWwiseConnection*>(pConnection);
			if (m_pConnection)
			{
				IAudioSystemControl* pControl = pConnection->GetControl();
				if (pControl && pControl->GetType() == eWCT_WWISE_RTPC)
				{
					switch (eATLControlType)
					{
					case EACBControlType::eACBT_RTPC:
						m_pShiftLineEdit->setHidden(false);
						m_pShiftLabel->setHidden(false);
						m_pMultiplicationLineEdit->setHidden(false);
						m_pMultiplicationLabel->setHidden(false);
						m_pShiftLineEdit->setText(QString::number(m_pConnection->fShift));
						m_pMultiplicationLineEdit->setText(QString::number(m_pConnection->fMult));
						break;
					case EACBControlType::eACBT_SWITCH:
						m_pValueLineEdit->setHidden(false);
						m_pValueLabel->setHidden(false);
						m_pValueLineEdit->setText(QString::number(m_pConnection->fValue));
						break;
					}
				}
			}
		}
	}

	void CAudioConnectionInspectorPanel_wwise::UpdateConnection()
	{
		if (m_pConnection)
		{
			m_pConnection->fShift = m_pShiftLineEdit->text().toFloat();
			m_pConnection->fMult = m_pMultiplicationLineEdit->text().toFloat();
			m_pConnection->fValue = m_pValueLineEdit->text().toFloat();
			ConnectionChanged();
		}
	}
}

#include <moc_AudioConnectionInspectorPanel_wwise.cpp>