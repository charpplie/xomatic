// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.

#pragma once

#include "StdAfx.h"
#include "AudioControlFilters.h"
#include <QTreeWidget>
#include "AudioControlMimeData.h"
#include "ATLControlsModel.h"
#include "AudioControl.h"

SNameFilter::SNameFilter(const QString& filter)
	: m_filter(filter)
{
}

bool SNameFilter::IsItemValid(QTreeWidgetItem* pItem)
{
	if (pItem)
	{
		QVariant variant = pItem->data(0, Qt::UserRole);
		SAudioSystemControlMimeData data = variant.value<SAudioSystemControlMimeData>();
		if (data.m_type != EItemType::eIT_FOLDER)
		{
			return IsNameValid(pItem->text(0));
		}
	}
	return false;
}

void SNameFilter::SetFilter(QString filter)
{
	m_filter = filter;
}

bool SNameFilter::IsNameValid(const QString& name)
{
	if ((m_filter.isEmpty()) || name.contains(m_filter, Qt::CaseInsensitive))
	{
		return true;
	}
	return false;
}

bool SImplNameFilter::IsItemValid(QTreeWidgetItem* pItem)
{
	if (pItem)
	{
		QVariant variant = pItem->data(0, Qt::UserRole);
		SAudioImplControlMimeData data = variant.value<SAudioImplControlMimeData>();
		if (data.m_type != EItemType::eIT_FOLDER)
		{
			return IsNameValid(pItem->text(0));
		}
	}
	return false;
}


// -----------------------------------------------

STypeFilter::STypeFilter()
{
	memset(m_hiddenTypes, false, sizeof(m_hiddenTypes));
}

void STypeFilter::SetControlTypeHidden(AudioControls::EACBControlType type, bool bHidden)
{
	m_hiddenTypes[type] = bHidden;
}

bool STypeFilter::IsControlTypeHidden(AudioControls::EACBControlType type)
{
	return m_hiddenTypes[type];
}

bool STypeFilter::IsItemValid(QTreeWidgetItem* pItem)
{
	if (pItem)
	{
		QVariant variant = pItem->data(0, Qt::UserRole);
		SAudioSystemControlMimeData data = variant.value<SAudioSystemControlMimeData>();
		if (data.m_type == EItemType::eIT_SWITCH)
		{
			return !m_hiddenTypes[AudioControls::EACBControlType::eACBT_SWITCH];
		}
		else if (data.m_type == EItemType::eIT_ITEM)
		{
			return !m_hiddenTypes[data.m_controlType];
		}
		else
		{
			for (int i = 0; i < AudioControls::EACBControlType::eACBT_NUM_TYPES; ++i)
			{
				if (m_hiddenTypes[(AudioControls::EACBControlType)i])
				{
					return false;
				}
			}
			return true;
		}
	}
	return false;
}

bool SImplTypeFilter::IsItemValid(QTreeWidgetItem* pItem)
{
	if (pItem)
	{
		QVariant variant = pItem->data(0, Qt::UserRole);
		SAudioImplControlMimeData data = variant.value<SAudioImplControlMimeData>();
		if (data.m_type == EItemType::eIT_SWITCH)
		{
			return false;
		}
		return data.m_controlType & m_nAllowedControlsMask;
	}
	return false;
}

// -----------------------------------------------


SHideConnectedFilter::SHideConnectedFilter()
	: m_bHideConnected(false)
{
}

void SHideConnectedFilter::SetHideConnected(bool bHideConnected)
{
	m_bHideConnected = bHideConnected;
}

bool SHideConnectedFilter::IsItemValid(QTreeWidgetItem* pItem)
{
	if (pItem)
	{
		QVariant variant = pItem->data(0, Qt::UserRole);
		SAudioSystemControlMimeData data = variant.value<SAudioSystemControlMimeData>();
		return !m_bHideConnected || !data.m_connected;
	}
	return false;
}

bool SScopeFilter::IsItemValid(QTreeWidgetItem* pItem)
{
	if (pItem && m_pModel)
	{
		QVariant variant = pItem->data(0, Qt::UserRole);
		SAudioSystemControlMimeData data = variant.value<SAudioSystemControlMimeData>();
		if (data.m_id != AudioControls::ACB_INVALID_ID)
		{
			AudioControls::CATLControl* pControl = m_pModel->GetControlByID(data.m_id);
			if(pControl)
			{
				string sScope = pControl->GetScope();
				return (sScope.compareNoCase(m_sScope) == 0) || (sScope == "");
			}
		}
	}
	return false;
}
