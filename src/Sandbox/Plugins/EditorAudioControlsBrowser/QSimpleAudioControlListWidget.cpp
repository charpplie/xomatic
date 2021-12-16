// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.

#pragma once
#include "StdAfx.h"
#include "QSimpleAudioControlListWidget.h"
#include "QtUtil.h"
#include "QAudioControlBrowserIcons.h"
#include "common/IAudioSystemControl.h"

#include <QApplication>
#include <QMimeData>

using namespace AudioControls;

//-------------------------------------
QSimpleAudioControlListWidget::QSimpleAudioControlListWidget(QWidget* parent)
	: QTreeWidget(parent)
	, m_connectedColor(QApplication::palette().text().color())
	, m_disconnectedColor(QColor(255, 143, 0))
	, m_localisedColor(QColor(36, 180, 245))
	, m_pAudioSystemImpl(nullptr)
{
	qRegisterMetaTypeStreamOperators<SAudioImplControlMimeData>("SAudioImplControlMimeData");
}

void QSimpleAudioControlListWidget::SetModel(AudioControls::IAudioSystemEditor* pAudioSystemImpl)
{
	m_pAudioSystemImpl = pAudioSystemImpl;
	Refresh(true);
}

//-------------------------------------
void QSimpleAudioControlListWidget::LoadControls()
{
	if (m_pAudioSystemImpl)
	{
		ClearControls();
		int size = m_pAudioSystemImpl->ControlCount();
		for (int i = 0; i < size; ++i)
		{
			IAudioSystemControl* pControl = m_pAudioSystemImpl->GetControlByIndex(i);
			if (pControl && !pControl->IsPlaceholder())
			{
				QTreeWidgetItem* item = InsertControl(pControl, invisibleRootItem());
			}
		}
	}
}

//-------------------------------------
void QSimpleAudioControlListWidget::UpdateControl(const AudioControls::IAudioSystemControl& control)
{
	QTreeWidgetItem* pItem = GetItem(control.GetId());
	if (pItem)
	{
		InitItemData(pItem, EItemType::eIT_ITEM, control.GetId());
	}
}

//-------------------------------------
QTreeWidgetItem* QSimpleAudioControlListWidget::InsertControl(AudioControls::IAudioSystemControl* pControl, QTreeWidgetItem* pRoot)
{
	if (pRoot && pControl)
	{
		QTreeWidgetItem* pParent = pRoot;

		QString sParent = QString(pControl->GetVirtualPath());
		if (sParent != "")
		{
			pParent = nullptr;
			int childCount = pRoot->childCount();
			for (int j = 0; j < childCount; ++j)
			{
				QTreeWidgetItem* pItem = pRoot->child(j);
				if (pItem && GetItemType(pItem) != EItemType::eIT_ITEM)
				{
					if (sParent == pItem->text(0))
					{
						pParent = pItem;
						break;
					}
				}
			}
			if (pParent == nullptr)
			{
				pParent = new QTreeWidgetItem();
				pParent->setText(0, sParent);
				InitItemData(pParent, EItemType::eIT_SWITCH);
				pRoot->addChild(pParent);
			}
		}

		QTreeWidgetItem* pItem = new QTreeWidgetItem();
		pItem->setText(0, QString(pControl->GetName()));
		InitItemData(pItem, EItemType::eIT_ITEM, pControl->GetId());
		pParent->addChild(pItem);
		return pItem;
	}
	return NULL;
}

//-------------------------------------
QTreeWidgetItem* QSimpleAudioControlListWidget::GetItem(AudioControls::CID id)
{
	QTreeWidgetItemIterator it(this);
	while (*it)
	{
		QTreeWidgetItem* item = *it;
		if (GetItemId(item) == id)
		{
			return item;
		}
		++it;
	}
	return NULL;
}

//-------------------------------------
EItemType QSimpleAudioControlListWidget::GetItemType(QTreeWidgetItem* item)
{
	if (item)
	{
		QVariant variant = item->data(0, Qt::UserRole);
		SAudioImplControlMimeData data = variant.value<SAudioImplControlMimeData>();
		return data.m_type;
	}
	return EItemType::eIT_INVALID;
}

//-------------------------------------
TImplControlType QSimpleAudioControlListWidget::GetControlType(QTreeWidgetItem* item)
{
	if (item)
	{
		QVariant variant = item->data(0, Qt::UserRole);
		SAudioImplControlMimeData data = variant.value<SAudioImplControlMimeData>();
		return data.m_controlType;
	}
	return AUDIO_IMPL_INVALID_TYPE;
}

//-------------------------------------
AudioControls::CID QSimpleAudioControlListWidget::GetItemId(QTreeWidgetItem* item)
{
	if (item)
	{
		QVariant variant = item->data(0, Qt::UserRole);
		SAudioImplControlMimeData data = variant.value<SAudioImplControlMimeData>();
		return data.m_id;
	}
	return ACB_INVALID_ID;
}

//-------------------------------------
bool QSimpleAudioControlListWidget::IsConnected(QTreeWidgetItem* item)
{
	if (item)
	{
		QVariant variant = item->data(0, Qt::UserRole);
		SAudioImplControlMimeData data = variant.value<SAudioImplControlMimeData>();
		return data.m_connected;
	}
	return false;
}

//-------------------------------------
void QSimpleAudioControlListWidget::InitItem(QTreeWidgetItem* pItem)
{
	if (pItem)
	{
		pItem->setIcon(0, m_pAudioSystemImpl->GetTypeIcon(GetControlType(pItem)));
		switch (GetItemType(pItem))
		{
		case EItemType::eIT_SWITCH:
			pItem->setFlags(pItem->flags() & ~Qt::ItemIsDragEnabled);
			break;
		case EItemType::eIT_ITEM:
			pItem->setFlags(pItem->flags() | Qt::ItemIsDragEnabled);

			IAudioSystemControl* pControl = GetControlFromId(GetItemId(pItem));
			if (pControl && pControl->IsLocalised())
			{
				pItem->setToolTip(0, tr("Localized control"));
				pItem->setForeground(0, m_localisedColor);
			}
			else
			{
				if (IsConnected(pItem))
				{
					pItem->setForeground(0, m_connectedColor);
				}
				else
				{
					pItem->setToolTip(0, tr("Unassigned control"));
					pItem->setForeground(0, m_disconnectedColor);
				}
			}
			break;
		}
		pItem->setFlags(pItem->flags() & ~Qt::ItemIsDropEnabled);
	}
}

//-------------------------------------
void QSimpleAudioControlListWidget::InitItemData(QTreeWidgetItem* pItem, EItemType type, AudioControls::CID id)
{
	SAudioImplControlMimeData data;
	data.m_type = type;
	data.m_id = id;
	data.m_controlType = AUDIO_IMPL_INVALID_TYPE;
	IAudioSystemControl* pControl = NULL;
	if (id > ACB_INVALID_ID)
	{
		pControl = GetControlFromId(id);
		if (pControl)
		{
			data.m_controlType = pControl->GetType();
			data.m_connected = true;	// TODO: Check if control is connected
		}
	}

	QVariant variantData;
	variantData.setValue(data);
	pItem->setData(0, Qt::UserRole, variantData);

	InitItem(pItem);
}

//-------------------------------------
void QSimpleAudioControlListWidget::ClearControls()
{
	std::vector<QTreeWidgetItem*> itemsToDelete;
	QTreeWidgetItemIterator it(this);
	while (*it)
	{
		QTreeWidgetItem* item = *it;
		if (item->childCount() == 0)
		{
			if (GetItemType(item) == EItemType::eIT_ITEM)
			{
				itemsToDelete.push_back(item);
			}
		}
		++it;
	}

	size_t size = itemsToDelete.size();
	for (int i = 0; i < size; ++i)
	{
		delete itemsToDelete[i];
	}
}

//-------------------------------------
std::vector<AudioControls::CID> QSimpleAudioControlListWidget::GetSelectedIds()
{
	std::vector<AudioControls::CID> ids;
	QList<QTreeWidgetItem*> selected = selectedItems();
	int size = selected.length();
	for (int i = 0; i < size; ++i)
	{
		ids.push_back(GetItemId(selected[i]));
	}
	return ids;
}

//-------------------------------------
void QSimpleAudioControlListWidget::Refresh(bool reload)
{
	if (m_pAudioSystemImpl)
	{
		// store the currently selected control to select it again
		std::vector<AudioControls::CID> ids = GetSelectedIds();

		if (reload)
		{
			LoadControls();
		}

		QTreeWidgetItemIterator it(this);
		while (*it)
		{
			InitItem(*it);
			++it;
		}

		// select the control that was previously selected
		size_t size = ids.size();
		for (size_t i = 0; i < size; ++i)
		{
			QTreeWidgetItem* pItem = GetItem(ids[i]);
			if (pItem)
			{
				pItem->setSelected(true);
				setCurrentItem(pItem, 0);
				scrollToItem(pItem);
			}
		}
	}
}

//--------------------------------------------------------------
IAudioSystemControl* QSimpleAudioControlListWidget::GetControlFromId(AudioControls::CID id)
{
	return m_pAudioSystemImpl->GetControlByID(id);
}

#include <moc_QSimpleAudioControlListWidget.cpp>