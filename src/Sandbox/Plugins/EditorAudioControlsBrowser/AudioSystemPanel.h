// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#pragma once

#include <QWidget>
#include <QMenu>

#include "ui_AudioSystemPanel.h"
#include "AudioControl.h"
#include "QTreeWidgetFilter.h"
#include "AudioControlFilters.h"

namespace AudioControls
{
	class CATLControlsModel;
	class QFilterButton;

	class CAudioSystemPanel : public QWidget, public Ui::AudioSystemPanel
	{
		Q_OBJECT
	public:
		CAudioSystemPanel(IAudioSystemEditor* pAudioSystemImpl);
		void Reload();
		void SetAllowedControls(EACBControlType type, bool bAllowed);

	private slots:
		void SetNameFilter(QString filter);
		void SetHideConnected(bool bHide);
		void ShowExternalControlsContextMenu(const QPoint& pos);

	private:
		IAudioSystemEditor* m_pAudioSystemImpl;

		// Filtering
		QTreeWidgetFilter m_filter;
		SImplNameFilter m_nameFilter;
		SImplTypeFilter m_typeFilter;
		bool m_allowedATLTypes[AudioControls::EACBControlType::eACBT_NUM_TYPES];
		SHideConnectedFilter m_hideConnectedFilter;
	};
}