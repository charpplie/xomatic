// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.

#include "StdAfx.h"
#include "AudioSystemPanel.h"
#include "AudioControl.h"
#include "ATLControlsModel.h"
#include "QAudioControlBrowserIcons.h"
#include "AudioLibrary.h"
#include <IEditor.h>
#include <CryFile.h>
#include <CryPath.h>

#include <QWidgetAction>
#include <QPushButton>
#include <QPaintEvent>
#include <QPainter>
#include <QMessageBox>
#include <QMimeData>

namespace AudioControls
{
	CAudioSystemPanel::CAudioSystemPanel(IAudioSystemEditor* pAudioSystemImpl)
		: m_pAudioSystemImpl(pAudioSystemImpl)
	{
		setupUi(this);

		m_filter.SetTree(m_pExternalList);
		m_filter.AddFilter(&m_nameFilter);
		m_filter.AddFilter(&m_typeFilter);
		m_filter.AddFilter(&m_hideConnectedFilter);

		connect(m_pExternalListFilter, SIGNAL(textChanged(QString)), this, SLOT(SetNameFilter(QString)));
		connect(m_pHideAssignedCheckbox, SIGNAL(clicked(bool)), this, SLOT(SetHideConnected(bool)));

		m_pExternalList->setContextMenuPolicy(Qt::CustomContextMenu);
		connect(m_pExternalList, SIGNAL(customContextMenuRequested(const QPoint&)), SLOT(ShowExternalControlsContextMenu(const QPoint&)));

		m_pExternalList->SetModel(m_pAudioSystemImpl);
	}

	void CAudioSystemPanel::SetNameFilter(QString filter)
	{
		m_nameFilter.SetFilter(filter);
		m_filter.ApplyFilter();
	}

	void CAudioSystemPanel::SetHideConnected(bool bHide)
	{
		m_hideConnectedFilter.SetHideConnected(bHide);
		m_filter.ApplyFilter();
	}

	void CAudioSystemPanel::ShowExternalControlsContextMenu(const QPoint& pos)
	{
		QMenu contextMenu(tr("Context menu"), this);
		//contextMenu.addAction(tr("Connect"), this, SLOT(ConnectSelectedControls()));
		contextMenu.exec(m_pExternalList->mapToGlobal(pos));
	}

	void CAudioSystemPanel::Reload()
	{
		m_pExternalList->Refresh();
	}

	void CAudioSystemPanel::SetAllowedControls(EACBControlType type, bool bAllowed)
	{
		m_allowedATLTypes[type] = bAllowed;
		uint nMask = 0;
		for (int i = 0; i < AudioControls::EACBControlType::eACBT_NUM_TYPES; ++i)
		{
			if (m_allowedATLTypes[i])
			{
				nMask |= m_pAudioSystemImpl->GetCompatibleTypes((EACBControlType)i);
			}
		}
		m_typeFilter.SetAllowedControlsMask(nMask);
		m_filter.ApplyFilter();
	}

}

#include <moc_AudioSystemPanel.cpp>