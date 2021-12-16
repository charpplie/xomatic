////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 1999-2014.
// -------------------------------------------------------------------------
//  File name:   QAudioControlTreeWidget.cpp
//  Version:     v1.00
//  Created:     04/06/2014 by Gabriel Rodriguez Hernandez.
//
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "StdAfx.h"
#include "QAudioControlTreeWidget.h"
#include "QtUtil.h"
#include "QAudioControlTreeWidgetDelegate.h"
#include "QAudioControlBrowserIcons.h"
#include "AudioLibrary.h"
#include "ATLControlsModel.h"
#include <IEditor.h>

#include <QApplication>
#include <QKeyEvent>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QDropEvent>
#include <QItemSelectionModel>

using namespace AudioControls;

class QSortedTreeWidgetItem : public QTreeWidgetItem
{
public:
	QSortedTreeWidgetItem() : QTreeWidgetItem() {}
	QSortedTreeWidgetItem(const QTreeWidgetItem& other) : QTreeWidgetItem(other) {}
	QSortedTreeWidgetItem(QTreeWidgetItem* pParent) : QTreeWidgetItem(pParent) {}
	bool operator< (const QTreeWidgetItem& other) const
	{
		QVariant variant = data(0, Qt::UserRole);
		SAudioSystemControlMimeData data = variant.value<SAudioSystemControlMimeData>();
		EItemType type = (EItemType)data.m_type;

		variant = other.data(0, Qt::UserRole);
		data = variant.value<SAudioSystemControlMimeData>();
		EItemType otherType = (EItemType)data.m_type;

		if (type == otherType)
		{
			return text(0).compare(other.text(0), Qt::CaseInsensitive) > 0;
		}

		return type > otherType;
	}
};

//-------------------------------------
QAudioControlTreeWidget::QAudioControlTreeWidget(QWidget* parent)
	: QTreeWidget(parent)
	, m_connectedColor(QApplication::palette().text().color())
	, m_disconnectedColor(QColor(255, 143, 0))
	, m_pModel(NULL)
{
	qRegisterMetaTypeStreamOperators<SAudioSystemControlMimeData>("SAudioSystemControlMimeData");
	qRegisterMetaTypeStreamOperators<SAudioImplControlMimeData>("SAudioImplControlMimeData");

	setItemDelegateForColumn(0, new QAudioControlTreeWidgetDelegate(this));
	setItemDelegateForColumn(1, new QPreloadRequestTreeWidgetDelegate(this));
	setItemDelegateForColumn(2, new QPreloadRequestTreeWidgetDelegate(this));
	setItemDelegateForColumn(3, new QPreloadRequestTreeWidgetDelegate(this));
	setItemDelegateForColumn(4, new QPreloadRequestTreeWidgetDelegate(this));
	setItemDelegateForColumn(5, new QPreloadRequestTreeWidgetDelegate(this));
}

QAudioControlTreeWidget::~QAudioControlTreeWidget()
{
	if (m_pModel)
	{
		m_pModel->RemoveListener(this);
	}
}

void QAudioControlTreeWidget::SetModel(AudioControls::CATLControlsModel* pModel)
{
	if (m_pModel)
	{
		m_pModel->RemoveListener(this);
	}
	m_pModel = pModel;

	if (m_pModel)
	{
		m_pModel->AddListener(this);
	}

	Refresh(true);
}

//-------------------------------------
void QAudioControlTreeWidget::LoadControls()
{
	if (m_pModel)
	{
		clear();
		int size = m_pModel->ControlCount();
		for (int i = 0; i < size; ++i)
		{
			CATLControl* pControl = m_pModel->GetControlByIndex(i);
			if (pControl)
			{
				QTreeWidgetItem* item = InsertControl(pControl);
			}
		}
	}
}

//-------------------------------------
QTreeWidgetItem* QAudioControlTreeWidget::InsertControl(AudioControls::CATLControl* pControl)
{
	if (pControl)
	{
		QTreeWidgetItem* pItem = new QSortedTreeWidgetItem();
		InitItemFromControl(pItem, pControl);

		QTreeWidgetItem* pParent = CreateFolderPath(pControl);
		pParent->addChild(pItem);
		return pItem;
	}
	return nullptr;
}

//-------------------------------------
void QAudioControlTreeWidget::RemoveControl(AudioControls::CATLControl* pControl)
{
	if (pControl)
	{
		QTreeWidgetItem* pItem = GetItem(pControl->GetId());
		if (pItem)
		{
			delete pItem;
		}
	}
}

//-------------------------------------
QTreeWidgetItem* QAudioControlTreeWidget::UpdateControl(AudioControls::CATLControl* pControl)
{
	if (pControl)
	{
		QTreeWidgetItem* pItem = GetItem(pControl->GetId());
		if (pItem)
		{
			QTreeWidgetItem* pPrevParent = pItem->parent();
			if (!pPrevParent)
			{
				pPrevParent = invisibleRootItem();
			}

			QTreeWidgetItem* pNewParent = CreateFolderPath(pControl);
			if (pNewParent != pPrevParent)
			{
				int index = pPrevParent->indexOfChild(pItem);
				pItem = pPrevParent->takeChild(index);
				pNewParent->addChild(pItem);
			}

			InitItemFromControl(pItem, pControl);
			return pItem;
		}
	}
	return nullptr;
}

//-------------------------------------
QTreeWidgetItem* QAudioControlTreeWidget::CreateFolderPath(AudioControls::CATLControl* pControl)
{
	string path = pControl->GetVirtualPath();
	CAudioLibrary* pLibrary = pControl->GetLibrary();
	if (pLibrary)
	{
		path = pLibrary->GetName() + "/" + path;
	}
	EItemType leaf = pControl->GetType() ==  EACBControlType::eACBT_SWITCH ? EItemType::eIT_SWITCH : EItemType::eIT_FOLDER;
	return CreateFolderPath(path, leaf, invisibleRootItem());
}

//-------------------------------------
QTreeWidgetItem* QAudioControlTreeWidget::CreateFolderPath(const string& path, EItemType leafType, QTreeWidgetItem* pRoot)
{
	QTreeWidgetItem* pParent = pRoot;
	int pos = 0;
	std::vector<string> folders;
	string folder = path.Tokenize("/\\", pos);
	while (!folder.empty())
	{
		folders.push_back(folder);
		folder = path.Tokenize("/\\", pos);
	}

	size_t size = folders.size();
	for (int i = 0; i < size; ++i)
	{
		QTreeWidgetItem* pChild = NULL;
		int childCount = pParent->childCount();
		for (int j = 0; j < childCount; ++j)
		{
			QTreeWidgetItem* pItem = pParent->child(j);
			if (pItem && GetItemType(pItem) != EItemType::eIT_ITEM)
			{
				string name = QtUtil::ToString(pItem->text(0));
				if (name.compare(folders[i]) == 0)
				{
					pChild = pItem;
					break;
				}
			}
		}
		if (pChild == NULL)
		{
			QSortedTreeWidgetItem* pItem = new QSortedTreeWidgetItem();
			if (i < (size - 1))
			{
				InitItemFromType(pItem, folders[i], EItemType::eIT_FOLDER);
			}
			else
			{
				InitItemFromType(pItem, folders[i], leafType);
			}
			pParent->addChild(pItem);
			pChild = pItem;
		}
		pParent = pChild;
	}
	return pParent;
}

//-------------------------------------
void QAudioControlTreeWidget::keyPressEvent(QKeyEvent* pEvent)
{
	if (pEvent->key() == Qt::Key_Delete)
	{
		DeleteItem(this);
		pEvent->accept();
	}
	else
	{
		QTreeWidget::keyPressEvent(pEvent);
	}
}

//-------------------------------------
QTreeWidgetItem* QAudioControlTreeWidget::GetItem(AudioControls::CID id)
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
void QAudioControlTreeWidget::dragEnterEvent(QDragEnterEvent* pEvent)
{
	if (pEvent->dropAction() == Qt::MoveAction)
	{
		const QMimeData* pData = pEvent->mimeData();
		QString format = "application/x-qabstractitemmodeldatalist";
		if (pData->hasFormat(format))
		{
			QByteArray encoded = pData->data(format);
			QDataStream stream(&encoded, QIODevice::ReadOnly);
			while (!stream.atEnd())
			{
				int row, col;
				QMap<int,  QVariant> roleDataMap;
				stream >> row >> col >> roleDataMap;
				if (!roleDataMap.isEmpty())
				{
					QVariant userRoleData = roleDataMap[Qt::UserRole];
					SAudioSystemControlMimeData customData = userRoleData.value<SAudioSystemControlMimeData>();
					if (customData.m_type == EItemType::eIT_FOLDER)
					{
						invisibleRootItem()->setFlags(invisibleRootItem()->flags() | Qt::ItemIsDropEnabled);
					}
					else
					{
						invisibleRootItem()->setFlags(invisibleRootItem()->flags() & ~Qt::ItemIsDropEnabled);
					}
				}
			}
		}
	}
	QTreeWidget::dragEnterEvent(pEvent);
}

//-------------------------------------
void QAudioControlTreeWidget::dropEvent(QDropEvent* pEvent)
{
	QTreeWidgetItem* pDropTarget = this->itemAt(pEvent->pos());
	QTreeWidget::dropEvent(pEvent);
	if (pDropTarget)
	{
		QTreeWidgetItem* pParent = pDropTarget->parent();
		if (pParent)
		{
			ParentChanged(pParent);
		}
		else
		{
			// update all the children's path
			QTreeWidgetItem* pRoot = invisibleRootItem();
			int size = pRoot->childCount();
			for (int i = 0; i < size; ++i)
			{
				ParentChanged(pRoot->child(i));
			}
		}

		const QMimeData* pData = pEvent->mimeData();
		QString format = "application/x-qabstractitemmodeldatalist";
		if (pData->hasFormat(format))
		{
			QByteArray encoded = pData->data(format);
			QDataStream stream(&encoded, QIODevice::ReadOnly);
			while (!stream.atEnd())
			{
				int row, col;
				QMap<int,  QVariant> roleDataMap;
				stream >> row >> col >> roleDataMap;
				if (!roleDataMap.isEmpty())
				{
					QVariant userRoleData = roleDataMap[Qt::UserRole];
					SAudioSystemControlMimeData itemData = userRoleData.value<SAudioSystemControlMimeData>();
					CATLControl* pControl = m_pModel->GetControlByID(itemData.m_id);
					if (pControl)
					{
						m_pModel->OnControlModified(pControl);
					}
				}
			}
		}
	}
}

//-------------------------------------
EItemType QAudioControlTreeWidget::GetItemType(QTreeWidgetItem* item)
{
	if (item)
	{
		QVariant variant = item->data(0, Qt::UserRole);
		SAudioSystemControlMimeData data = variant.value<SAudioSystemControlMimeData>();
		return data.m_type;
	}
	return EItemType::eIT_INVALID;
}

//-------------------------------------
AudioControls::EACBControlType QAudioControlTreeWidget::GetControlType(QTreeWidgetItem* item)
{
	if (item)
	{
		QVariant variant = item->data(0, Qt::UserRole);
		SAudioSystemControlMimeData data = variant.value<SAudioSystemControlMimeData>();
		return data.m_controlType;
	}
	return AudioControls::EACBControlType::eACBT_NUM_TYPES;
}

//-------------------------------------
AudioControls::CID QAudioControlTreeWidget::GetItemId(QTreeWidgetItem* item)
{
	if (item)
	{
		QVariant variant = item->data(0, Qt::UserRole);
		SAudioSystemControlMimeData data = variant.value<SAudioSystemControlMimeData>();
		return data.m_id;
	}
	return ACB_INVALID_ID;
}

//-------------------------------------
bool QAudioControlTreeWidget::IsConnected(QTreeWidgetItem* item)
{
	if (item)
	{
		QVariant variant = item->data(0, Qt::UserRole);
		SAudioSystemControlMimeData data = variant.value<SAudioSystemControlMimeData>();
		return data.m_connected;
	}
	return false;
}

//-------------------------------------
void QAudioControlTreeWidget::InitItem(QTreeWidgetItem* pItem)
{
	if (pItem)
	{
		blockSignals(true);
		switch (GetItemType(pItem))
		{
		case EItemType::eIT_FOLDER:
			pItem->setIcon(0, GetFolderIcon());
			pItem->setFlags(pItem->flags() | Qt::ItemIsDropEnabled);
			pItem->setFlags(pItem->flags() | Qt::ItemIsDragEnabled);
			break;
		case EItemType::eIT_SWITCH:
			pItem->setIcon(0, GetControlTypeIcon(EACBControlType::eACBT_SWITCH));
			pItem->setFlags(pItem->flags() & ~Qt::ItemIsDropEnabled);
			pItem->setFlags(pItem->flags() | Qt::ItemIsDragEnabled);
			break;
		case EItemType::eIT_ITEM:
			AudioControls::EACBControlType controlType = GetControlType(pItem);
			if (controlType == eACBT_SWITCH)
			{
				pItem->setIcon(0, GetPropertyIcon());
				pItem->setFlags(pItem->flags() & ~Qt::ItemIsDragEnabled);
			}
			else
			{
				pItem->setIcon(0, GetControlTypeIcon(controlType));
				pItem->setFlags(pItem->flags() | Qt::ItemIsDragEnabled);
			}
			pItem->setFlags(pItem->flags() & ~Qt::ItemIsDropEnabled);

			if (IsConnected(pItem))
			{
				pItem->setForeground(0, m_connectedColor);
			}
			else
			{
				pItem->setForeground(0, m_disconnectedColor);
			}
			break;
		}
		pItem->setFlags(pItem->flags() | Qt::ItemIsEditable);
		blockSignals(false);
	}
}

//-------------------------------------
void QAudioControlTreeWidget::InitItemFromControl(QTreeWidgetItem* pItem, AudioControls::CATLControl* pControl)
{
	if (pControl)
	{
		blockSignals(true);
		pItem->setText(0, QString(pControl->GetName()));
		blockSignals(false);

		SAudioSystemControlMimeData data;
		data.m_type = EItemType::eIT_ITEM;
		data.m_id = pControl->GetId();
		data.m_controlType = pControl->GetType();
		data.m_connected = pControl->ConnectionCount() > 0;

		if (pControl->GetType() == eACBT_PRELOADS)
		{
			int numPlatforms = m_pModel->GetPlatformCount();
			for (int i = 0; i < numPlatforms; ++i)
			{
				pItem->setData(i + 1, Qt::UserRole, pControl->GetGroupForPlatform(m_pModel->GetPlatformAt(i)));
			}
		}

		QVariant variantData;
		variantData.setValue(data);
		blockSignals(true);
		pItem->setData(0, Qt::UserRole, variantData);
		blockSignals(false);

		InitItem(pItem);
	}
}


void QAudioControlTreeWidget::InitItemFromType(QTreeWidgetItem* pItem, const string& name, EItemType type)
{
	pItem->setText(0, QString(name));

	SAudioSystemControlMimeData data;
	data.m_type = type;
	data.m_id = ACB_INVALID_ID;

	QVariant variantData;
	variantData.setValue(data);
	pItem->setData(0, Qt::UserRole, variantData);

	InitItem(pItem);
}


//-------------------------------------
QTreeWidgetItem* QAudioControlTreeWidget::AddFolder(string name, QTreeWidgetItem* pParent)
{
	while (pParent && GetItemType(pParent) != EItemType::eIT_FOLDER)
	{
		pParent = pParent->parent();
	}

	if (!pParent)
	{
		pParent = invisibleRootItem();
	}

	// check name is not taken, if it is generate a unique one
	string itemName = GenerateUniqueName(name, pParent);

	QSortedTreeWidgetItem* pFolder = new QSortedTreeWidgetItem();
	InitItemFromType(pFolder, itemName, EItemType::eIT_FOLDER);
	pParent->addChild(pFolder);
	return pFolder;
}

//-------------------------------------
std::vector<AudioControls::CID> QAudioControlTreeWidget::GetSelectedIds()
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
void QAudioControlTreeWidget::Refresh(bool reload)
{
	if (m_pModel)
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
			if (ids[i] != ACB_INVALID_ID)
			{
				QTreeWidgetItem* pItem = GetItem(ids[i]);
				if (pItem)
				{
					setCurrentItem(pItem, 0, QItemSelectionModel::ClearAndSelect);
				}
			}
		}
	}
}

void QAudioControlTreeWidget::OnControlAdded(CATLControl* pControl)
{
	if (pControl)
	{
		QTreeWidgetItem* pItem = InsertControl(pControl);
		if (pItem && !pItem->isSelected())
		{
			QTreeWidgetItem* pParent = pItem->parent();
			while (pParent)
			{
				pParent->setExpanded(true);
				pParent = pParent->parent();
			}
			setCurrentItem(pItem, 0, QItemSelectionModel::ClearAndSelect);
		}
	}
}

void QAudioControlTreeWidget::OnControlRemoved(CATLControl* pControl)
{
	RemoveControl(pControl);
}

void QAudioControlTreeWidget::OnControlModified(CATLControl* pControl)
{
	if (pControl)
	{
		QTreeWidgetItem* pItem = UpdateControl(pControl);
		if (pItem && !pItem->isSelected())
		{
			QTreeWidgetItem* pParent = pItem->parent();
			while (pParent)
			{
				pParent->setExpanded(true);
				pParent = pParent->parent();
			}
			setCurrentItem(pItem, 0, QItemSelectionModel::ClearAndSelect);
		}
	}
}

string QAudioControlTreeWidget::GenerateUniqueName(const string& nameRoot, QTreeWidgetItem* pParent)
{
	string itemName = nameRoot;
	bool found = false;
	int size = pParent->childCount();
	int count = 1;
	while (!found)
	{
		found = true;
		for (int i = 0; i < size; ++i)
		{
			QTreeWidgetItem* item = pParent->child(i);
			string sibilingName = QtUtil::ToString(item->text(0));
			if (sibilingName == itemName)
			{
				found = false;
				itemName = QtUtil::ToString(nameRoot + " " + QString::number(count));
				++count;
				break;
			}
		}
	}
	return itemName;
}

void QAudioControlTreeWidget::EnableEditing(bool bEnable)
{
	blockSignals(true);
	QTreeWidgetItemIterator it(this);
	while (*it)
	{
		QTreeWidgetItem* pItem = *it;
		if(pItem)
		{
			if(bEnable)
			{
				pItem->setFlags(pItem->flags() | Qt::ItemIsEditable);
			}
			else
			{
				pItem->setFlags(pItem->flags() & ~Qt::ItemIsEditable);
			}
		}
		++it;
	}
	blockSignals(false);
}

#include <moc_qaudiocontroltreewidget.cpp>