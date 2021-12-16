////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 1999-2014.
// -------------------------------------------------------------------------
//  File name:   QAudioControlTreeWidgetDelegate.h
//  Version:     v1.00
//  Created:     14/07/2014 by Gabriel Rodriguez Hernandez.
//
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "qstyleditemdelegate.h"

class QAudioControlTreeWidgetDelegate : public QStyledItemDelegate
{
	Q_OBJECT

public:
	QAudioControlTreeWidgetDelegate(QObject* pParent = 0);
	virtual void setModelData(QWidget* pEditor, QAbstractItemModel* pModel, const QModelIndex& index) const;
};

class QPreloadRequestTreeWidgetDelegate : public QStyledItemDelegate
{
	Q_OBJECT

public:
	QPreloadRequestTreeWidgetDelegate(QObject* parent = 0);

	virtual QWidget* createEditor(QWidget* pParent, const QStyleOptionViewItem& option, const QModelIndex& index) const;
	virtual void setEditorData(QWidget* pEditor, const QModelIndex& index) const;
	virtual void setModelData(QWidget* pEditor, QAbstractItemModel* pModel, const QModelIndex& index) const;
	void paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

	static void ClearGroups();
	static void AddGroupName(const string& groupName, const QIcon& icon);

	static std::vector<std::pair<QIcon, string> > m_groups;
};