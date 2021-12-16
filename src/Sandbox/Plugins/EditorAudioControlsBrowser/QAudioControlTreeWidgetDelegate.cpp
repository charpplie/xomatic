////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 1999-2014.
// -------------------------------------------------------------------------
//  File name:   QAudioControlTreeWidgetDelegate.cpp
//  Version:     v1.00
//  Created:     14/07/2014 by Gabriel Rodriguez Hernandez.
//
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "StdAfx.h"
#include "QAudioControlTreeWidgetDelegate.h"
#include "QtUtil.h"
#include <QComboBox>
#include <QStylePainter>
#include <QLineEdit>

void QAudioControlTreeWidgetDelegate::setModelData(QWidget* pEditor, QAbstractItemModel* pModel, const QModelIndex& index) const
{
	QLineEdit* lineEdit = qobject_cast<QLineEdit*>(pEditor);
	if (lineEdit->isModified())
	{
		QRegExp regExpression("^[a-zA-Z0-9_]*$"); // letters + numbers + underscore ONLY
		QString text = lineEdit->text();

		if(regExpression.exactMatch(text))
		{
			QStyledItemDelegate::setModelData(pEditor, pModel, index);
		}
	}
}

QAudioControlTreeWidgetDelegate::QAudioControlTreeWidgetDelegate( QObject* pParent /*= 0*/ )
	: QStyledItemDelegate(pParent)
{
}

class QImageComboBox : public QComboBox
{
public:
	QImageComboBox(QWidget* widget) : QComboBox(widget) {}

protected:
	void paintEvent(QPaintEvent* e)
	{
		QStylePainter painter(this);
		painter.setPen(palette().color(QPalette::Text));

		// draw the combobox frame
		QStyleOptionComboBox opt;
		initStyleOption(&opt);
		opt.frame = false;
		painter.drawComplexControl(QStyle::CC_ComboBox, opt);

		// draw the icon and text
		opt.currentText = "";
		opt.rect.adjust(2, 0, 8, 0);
		painter.drawControl(QStyle::CE_ComboBoxLabel, opt);
	}
};

std::vector<std::pair<QIcon, string> > QPreloadRequestTreeWidgetDelegate::m_groups;

QPreloadRequestTreeWidgetDelegate::QPreloadRequestTreeWidgetDelegate(QObject* parent)
	: QStyledItemDelegate(parent)
{
}

QWidget* QPreloadRequestTreeWidgetDelegate::createEditor(QWidget* pParent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{

	if (index.column() > 0 && index.data(Qt::UserRole).isValid())
	{
		QImageComboBox* pComboBox = new QImageComboBox(pParent);

		size_t size = m_groups.size();
		for (size_t i = 0; i < size; ++i)
		{
			pComboBox->addItem(m_groups[i].first, QtUtil::ToQString(m_groups[i].second));
		}

		// Need to change the style delegate to
		// let the dropdown popup to be styled
		QStyledItemDelegate* itemDelegate = new QStyledItemDelegate();
		pComboBox->setItemDelegate(itemDelegate);
		pComboBox->setStyleSheet("QComboBox {border-width: 0px; border-style: transparent; } \
										 QComboBox::drop-down { border-left-style: transparent; } \
										 QComboBox::down-arrow { image: url(:/Icons/32_arrowDown_Icon.png); width: 8px}\
										 QComboBox QAbstractItemView::item {margin-top: 0px;}");

		return pComboBox;
	}
	return NULL;
}

void QPreloadRequestTreeWidgetDelegate::setEditorData(QWidget* pEditor, const QModelIndex& index) const
{
	if (QComboBox* pComboBox = qobject_cast<QComboBox*>(pEditor))
	{

		int group = index.data(Qt::UserRole).toInt();
		if (group >= 0)
		{
			pComboBox->setCurrentIndex(group);
		}
		pComboBox->showPopup();
	}
	else
	{
		QStyledItemDelegate::setEditorData(pEditor, index);
	}
}

void QPreloadRequestTreeWidgetDelegate::setModelData(QWidget* pEditor, QAbstractItemModel* pModel, const QModelIndex& index) const
{
	if (QComboBox* pComboBox = qobject_cast<QComboBox*>(pEditor))
	{
		pModel->setData(index, pComboBox->currentIndex(), Qt::UserRole);
	}
	else
	{
		QStyledItemDelegate::setModelData(pEditor, pModel, index);
	}
}

void QPreloadRequestTreeWidgetDelegate::paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	if (index.column() > 0 && index.data(Qt::UserRole).isValid())
	{
		int group = index.data(Qt::UserRole).toInt();
		if (group >= 0 && group < m_groups.size())
		{
			m_groups[group].first.paint(pPainter, option.rect);
			return;
		}
	}

	QStyledItemDelegate::paint(pPainter, option, index);
}

void QPreloadRequestTreeWidgetDelegate::AddGroupName(const string& groupName, const QIcon& icon)
{
	m_groups.push_back(std::make_pair(icon, groupName));
}

void QPreloadRequestTreeWidgetDelegate::ClearGroups()
{
	m_groups.clear();
}

#include <moc_qaudiocontroltreewidgetdelegate.cpp>