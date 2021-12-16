// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.

#include "StdAfx.h"
#include "AudioConnectionInspectorPanel_sdlmixer.h"
#include "IAudioSystemControl.h"
#include "AudioSystemEditor_sdlmixer.h"

#include <QValidator>

namespace AudioControls
{
	CAudioConnectionInspectorPanel_sdlmixer::CAudioConnectionInspectorPanel_sdlmixer()
	{
		setupUi(this);

		connect(m_pMinDistanceAttenuationText, SIGNAL(editingFinished()), this, SLOT(UpdateConnection()));
		connect(m_pMaxDistanceAttenuationText, SIGNAL(editingFinished()), this, SLOT(UpdateConnection()));
		connect(m_pEnablePanningCheckBox, SIGNAL(clicked()), this, SLOT(UpdateConnection()));
		connect(m_pEnableAttenuationGroup, SIGNAL(clicked()), this, SLOT(UpdateConnection()));
		connect(m_pCountLoopingRadio, SIGNAL(clicked()), this, SLOT(UpdateConnection()));
		connect(m_pInfiniteLoopingRadio, SIGNAL(clicked()), this, SLOT(UpdateConnection()));
		connect(m_pLoopCountSpinBox, SIGNAL(valueChanged(int)), this, SLOT(UpdateConnection()));
		connect(m_pVolumeSlider, SIGNAL(valueChanged(int)), this, SLOT(UpdateConnection()));
		connect(m_pVolumeField, SIGNAL(editingFinished()), this, SLOT(UpdateConnection()));

		// data validators
		m_pMinDistanceAttenuationText->setValidator(new QDoubleValidator(m_pMinDistanceAttenuationText));
		m_pMaxDistanceAttenuationText->setValidator(new QDoubleValidator(m_pMaxDistanceAttenuationText));
		m_pVolumeField->setValidator(new QDoubleValidator(m_pVolumeField));
	}

	void CAudioConnectionInspectorPanel_sdlmixer::UpdateControl(IAudioConnection* pConnection, EACBControlType eATLControlType)
	{
		if (pConnection)
		{
			setHidden(false);

			m_pConnection = static_cast<CSDLMixerConnection*>(pConnection);
			if (m_pConnection)
			{
				m_pEnablePanningCheckBox->setChecked(m_pConnection->bPanningEnabled);
				m_pEnableAttenuationGroup->setChecked(m_pConnection->bAttenuationEnabled);
				m_pMinDistanceAttenuationText->setText(QString::number(m_pConnection->fMinAttenuation));
				m_pMaxDistanceAttenuationText->setText(QString::number(m_pConnection->fMaxAttenuation));

				bool bInfiniteLoop = (m_pConnection->nLoopCount == -1);
				m_pInfiniteLoopingRadio->setChecked(bInfiniteLoop);
				m_pCountLoopingRadio->setChecked(!bInfiniteLoop);
				m_pLoopCountSpinBox->setEnabled(!bInfiniteLoop);
				m_pLoopCountSpinBox->setValue(m_pConnection->nLoopCount);

				m_pVolumeSlider->blockSignals(true);
				m_pVolumeField->blockSignals(true);
				m_pVolumeSlider->setValue(m_pConnection->fVolume);
				m_pVolumeField->setText(QString::number(m_pConnection->fVolume));
				m_pVolumeSlider->blockSignals(false);
				m_pVolumeField->blockSignals(false);

			}
		}
		else
		{
			setHidden(true);
		}
	}

	void CAudioConnectionInspectorPanel_sdlmixer::UpdateConnection()
	{
		if (m_pConnection)
		{
			m_pConnection->bPanningEnabled = m_pEnablePanningCheckBox->isChecked();
			m_pConnection->bAttenuationEnabled = m_pEnableAttenuationGroup->isChecked();

			float fMinAttenuation = m_pMinDistanceAttenuationText->text().toFloat();
			float fMaxAttenuation = m_pMaxDistanceAttenuationText->text().toFloat();


			if (m_pConnection->fMinAttenuation != fMinAttenuation)
			{
				// changing min attenuation distance
				if (fMinAttenuation > fMaxAttenuation)
				{
					fMaxAttenuation = fMinAttenuation;
					m_pMaxDistanceAttenuationText->setText(QString::number(fMaxAttenuation));
				}
			}
			else if (m_pConnection->fMaxAttenuation != fMaxAttenuation)
			{
				// changing max attenuation distance
				if (fMaxAttenuation < fMinAttenuation)
				{
					fMinAttenuation = fMaxAttenuation;
					m_pMinDistanceAttenuationText->setText(QString::number(fMinAttenuation));
				}
			}

			if (m_pInfiniteLoopingRadio->isChecked())
			{
				m_pLoopCountSpinBox->setEnabled(false);
				m_pConnection->nLoopCount = -1;
			}
			else
			{
				m_pLoopCountSpinBox->setEnabled(true);
				m_pConnection->nLoopCount = m_pLoopCountSpinBox->value();
			}

			float fFieldVolume = m_pVolumeField->text().toFloat();
			float fSliderVolume = m_pVolumeSlider->value();

			if (fFieldVolume != m_pConnection->fVolume)
			{
				// Field changed
				m_pConnection->fVolume = fFieldVolume;
				m_pVolumeSlider->setValue(fFieldVolume);
			}
			else if (fSliderVolume != m_pConnection->fVolume)
			{
				//slider changed
				m_pConnection->fVolume = fSliderVolume;
				m_pVolumeField->setText(QString::number(fSliderVolume));
			}

			m_pConnection->fMinAttenuation = fMinAttenuation;
			m_pConnection->fMaxAttenuation = fMaxAttenuation;
			ConnectionChanged();
		}
	}
}

#include <moc_AudioConnectionInspectorPanel_sdlmixer.cpp>
