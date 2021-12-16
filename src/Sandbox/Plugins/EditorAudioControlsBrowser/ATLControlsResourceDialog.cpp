// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.

#include "StdAfx.h"
#include "ATLControlsResourceDialog.h"
#include "AudioControlsBrowserPlugin.h"
#include "ATLControlsModel.h"
#include "QAudioControlTreeWidget.h"

#include <QDialogButtonBox>
#include <QLineEdit>
#include <QBoxLayout>
#include <QApplication>
#include <QHeaderView>

namespace AudioControls
{
	ATLControlsDialog::ATLControlsDialog(QWidget* parent, EACBControlType type) : QDialog(parent)
	{
		setWindowTitle("Choose...");
		setWindowModality(Qt::ApplicationModal);

		QBoxLayout* pLayout = new QBoxLayout(QBoxLayout::TopToBottom);
		setLayout(pLayout);

		QLineEdit* pTextFilterLineEdit = new QLineEdit(this);
		pTextFilterLineEdit->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignVCenter);
		pTextFilterLineEdit->setPlaceholderText(QApplication::translate("ATLControlsPanel", "Search", 0));
		connect(pTextFilterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(SetTextFilter(QString)));
		pLayout->addWidget(pTextFilterLineEdit, 0);

		m_pControlTree = new QAudioControlTreeWidget(this);
		m_pControlTree->header()->hide();
		m_pControlTree->setEnabled(true);
		m_pControlTree->setAutoScroll(true);
		m_pControlTree->setDragEnabled(false);
		m_pControlTree->setDragDropMode(QAbstractItemView::NoDragDrop);
		m_pControlTree->setDefaultDropAction(Qt::IgnoreAction);
		m_pControlTree->setAlternatingRowColors(false);
		m_pControlTree->setSelectionMode(QAbstractItemView::SingleSelection);
		m_pControlTree->setIndentation(15);
		m_pControlTree->setRootIsDecorated(true);
		m_pControlTree->setSortingEnabled(true);
		m_pControlTree->setAnimated(false);
		m_pControlTree->setColumnCount(1);
		m_pControlTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
		m_pControlTree->EnableEditing(false);
		m_pControlTree->SetModel(CAudioControlsBrowserPlugin::GetATLModel());
		connect(m_pControlTree, SIGNAL(itemSelectionChanged()), this, SLOT(ControlChanged()));
		pLayout->addWidget(m_pControlTree, 0);

		QDialogButtonBox* buttons = new QDialogButtonBox(this);
		buttons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
		connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
		connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
		pLayout->addWidget(buttons, 0);

		m_scopeFilter.SetModel(CAudioControlsBrowserPlugin::GetATLModel());
		m_filter.SetTree(m_pControlTree);
		m_filter.AddFilter(&m_nameFilter);
		m_filter.AddFilter(&m_typeFilter);
		m_filter.AddFilter(&m_scopeFilter);

		for (int i = 0; i < eACBT_NUM_TYPES; ++i)
		{
			EACBControlType controlType = (EACBControlType)i;
			m_typeFilter.SetControlTypeHidden(controlType, true);
		}
		m_typeFilter.SetControlTypeHidden(type, false);
		m_filter.ApplyFilter();
	}

	const char* ATLControlsDialog::ChooseItem(const char* currentValue)
	{
		if (exec() == QDialog::Accepted)
		{
			return m_sControlName.c_str();
		}
		return currentValue;
	}

	void ATLControlsDialog::ControlChanged()
	{
		CATLControlsModel* pModel = CAudioControlsBrowserPlugin::GetATLModel();
		if (pModel)
		{
			std::vector<AudioControls::CID> ids = m_pControlTree->GetSelectedIds();
			if (ids.size() > 0)
			{
				CATLControl* pControl = pModel->GetControlByID(ids[0]);
				if (pControl)
				{
					m_sControlName = pControl->GetName();
				}
			}
		}
	}

	void ATLControlsDialog::SetTextFilter(QString filter)
	{
		m_nameFilter.SetFilter(filter);
		m_filter.ApplyFilter();
	}

	QSize ATLControlsDialog::sizeHint() const
	{
		return QSize(400, 900);
	}

	void ATLControlsDialog::SetScope(string sScope)
	{
		m_scopeFilter.SetScope(sScope);
		m_filter.ApplyFilter();
	}

}

#include <moc_ATLControlsResourceDialog.cpp>