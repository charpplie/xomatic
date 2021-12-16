////////////////////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.
// ----------------------------------------------------------------------------------------
//  File name:   DatabaseModel.h
//  Description: Qt model representation of the database
//
//////////////////////////////////////////////////////////////////////////////////////////// 

#ifndef _DATABASE_MODEL_H_
#define _DATABASE_MODEL_H_

#pragma once

#include <QtCore/QAbstractItemModel>

class  Database;
struct DatabaseEntryNode;

class DatabaseModel : public QAbstractItemModel
{
	Q_OBJECT

	enum EColumns
	{
		eColumn_Name = 0,
		eColumn_PakStatus,
		eColumn_Count,
	};

public:
	DatabaseModel(Database& database, QObject* pParent);

	QModelIndex index(int row, int column, const QModelIndex& parent) const OVERRIDE;
	Qt::ItemFlags flags(const QModelIndex& index ) const;

	int rowCount(const QModelIndex& parent) const OVERRIDE;
	int columnCount(const QModelIndex& parent) const OVERRIDE;

	QVariant headerData(int section, Qt::Orientation orientation, int role) const OVERRIDE;
	bool hasChildren(const QModelIndex &parent) const OVERRIDE;
	QVariant data(const QModelIndex& index, int role) const OVERRIDE;
	bool setData(const QModelIndex &index, const QVariant &value, int role /* = Qt::EditRole */) OVERRIDE;
	QModelIndex parent(const QModelIndex& index) const OVERRIDE;

	QModelIndex ModelIndexFromNode(DatabaseEntryNode* pEntryNode) const;

	const char* GetHeaderSectionName( int section ) const;

	static DatabaseEntryNode* GetEntryNode(const QModelIndex& index);

public slots:
	void OnSignalBeginAddEntryNode(DatabaseEntryNode* pEntryNode);
	void OnSignalEndAddEntryNode(DatabaseEntryNode* pEntryNode);
	void OnSignalBeginDeleteEntryNode(DatabaseEntryNode* pEntryNode);
	void OnSignalEndDeleteEntryNode();
	void OnSignalEntryNodeDataChanged(DatabaseEntryNode* pEntryNode);

private:

	Database& m_database;
};

#endif 

