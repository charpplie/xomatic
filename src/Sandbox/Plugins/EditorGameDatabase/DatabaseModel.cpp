//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.

#include "StdAfx.h"
#include "DatabaseModel.h"

#include <QSize>
#include <QFont>
#include <QIcon>
#include <QColor>

#include "Database.h"


DatabaseModel::DatabaseModel( Database& database, QObject* pParent )
	: QAbstractItemModel(pParent)
	, m_database(database)
{
	connect(&m_database, SIGNAL(SignalBeginAddEntryNode(DatabaseEntryNode*)), this, SLOT(OnSignalBeginAddEntryNode(DatabaseEntryNode*)));
	connect(&m_database, SIGNAL(SignalEndAddEntryNode(DatabaseEntryNode*)), this, SLOT(OnSignalEndAddEntryNode(DatabaseEntryNode*)));
	connect(&m_database, SIGNAL(SignalBeginDeleteEntryNode(DatabaseEntryNode*)), this, SLOT(OnSignalBeginDeleteEntryNode(DatabaseEntryNode*)));
	connect(&m_database, SIGNAL(SignalEndDeleteEntryNode()), this, SLOT(OnSignalEndDeleteEntryNode()));
	connect(&m_database, SIGNAL(SignalEntryNodeDataChanged(DatabaseEntryNode*)), this, SLOT(OnSignalEntryNodeDataChanged(DatabaseEntryNode*)));
	connect(&m_database, SIGNAL(SignalEntryNodeReverted(DatabaseEntryNode*)), this, SLOT(OnSignalEntryNodeDataChanged(DatabaseEntryNode*)));
}

QModelIndex DatabaseModel::index( int row, int column, const QModelIndex& parent ) const 
{
	if ((row < 0) || (column < 0))
		return QModelIndex();

	DatabaseEntryNode* pEntryNode = GetEntryNode(parent);
	if (pEntryNode == NULL)
	{
		pEntryNode = m_database.GetRoot();
	}
	CRY_ASSERT(pEntryNode != NULL);

	if (size_t(row) < pEntryNode->children.size())
	{
		return createIndex(row, column, pEntryNode->children[row]);
	}

	return QModelIndex();
}

Qt::ItemFlags DatabaseModel::flags(const QModelIndex& index ) const
{
	if (index.isValid() && (index.column() == eColumn_Name))
	{
		DatabaseEntryNode* pEntryNode = GetEntryNode(index);
		if ((pEntryNode != NULL) && 
			(pEntryNode->type == DatabaseEntryNode::Type::Leaf) && 
			pEntryNode->flags.AreAllFlagsActive(DatabaseEntryNode::FlagInFolder))
		{
			return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
		}
	}

	return QAbstractItemModel::flags(index);
}

int DatabaseModel::rowCount( const QModelIndex& parent ) const 
{
	DatabaseEntryNode* pEntryNode = GetEntryNode(parent);
	if (pEntryNode == NULL)
	{
		pEntryNode = m_database.GetRoot();
	}
	CRY_ASSERT(pEntryNode != NULL);

	return static_cast<int>(pEntryNode->children.size());
}

int DatabaseModel::columnCount( const QModelIndex& parent ) const 
{
	return eColumn_Count;
}

QVariant DatabaseModel::headerData( int section, Qt::Orientation orientation, int role ) const 
{
	if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
	{
		return QString(GetHeaderSectionName(section));
	}
	else if (role == Qt::SizeHintRole && orientation == Qt::Horizontal)
	{
		if (section == eColumn_Name)
		{
			return QSize(200, 20);
		}
		else  if (section == eColumn_PakStatus)
		{
			return QSize(20, 20);
		}
	}
	
	return QVariant();
}

bool DatabaseModel::hasChildren( const QModelIndex &parent ) const 
{
	DatabaseEntryNode* pEntryNode = GetEntryNode(parent);
	if (pEntryNode == NULL)
	{
		pEntryNode = m_database.GetRoot();
	}
	CRY_ASSERT(pEntryNode != NULL);

	return !pEntryNode->children.empty();
}

QVariant DatabaseModel::data( const QModelIndex& index, int role ) const 
{
	if (!index.isValid())
	{
		return QVariant();
	}

	DatabaseEntryNode* pEntryNode = GetEntryNode(index);
	if(pEntryNode == NULL)
	{
		return QVariant();
	}

	switch (role)
	{
	case Qt::DisplayRole:
		{
			if (index.column() == eColumn_Name)
			{
				if ((pEntryNode->type == DatabaseEntryNode::Leaf) && pEntryNode->flags.AreAnyFlagsActive(DatabaseEntryNode::FlagModified))
					return QString(QString(pEntryNode->name.c_str()) + QString(" *"));

				return QString(pEntryNode->name.c_str());
			}
		}
		break;
	case Qt::EditRole:
		{
			if (index.column() == eColumn_Name)
			{
				return QString(pEntryNode->name.c_str());
			}
		}
		break;
	case Qt::FontRole:
		{
			if ((index.column() == eColumn_Name) && (pEntryNode->type == DatabaseEntryNode::Group))
			{
				QFont font;
				font.setBold(true);
				return font;
			}
		}
		break;
	case Qt::TextColorRole:
		{
			if (index.column() == eColumn_Name)
			{
				if (pEntryNode->type == DatabaseEntryNode::Type::Leaf)
				{
					if (pEntryNode->flags.AreAllFlagsActive(DatabaseEntryNode::FlagModified))
						return QColor(Qt::red);
					if (!pEntryNode->flags.AreAllFlagsActive(DatabaseEntryNode::FlagInFolder))
						return QColor(Qt::lightGray);
				}

				return QColor(Qt::black);
			}
		}
		break;
	case Qt::SizeHintRole:
		{
			if (index.column() == eColumn_Name)
			{
				return QSize(200, 20);
			}
			else if (index.column() == eColumn_PakStatus)
			{
				return QSize(20, 20);
			}
		}
		break;
	case Qt::DecorationRole:
		{
			if (index.column() == eColumn_Name)
			{
				if (pEntryNode->type == DatabaseEntryNode::Group)
					return QIcon(":/icons/group.png");
				else if (pEntryNode->type == DatabaseEntryNode::Leaf)
					return QIcon(":/icons/record.png");
			}
			else if (index.column() == eColumn_PakStatus)
			{
				if (pEntryNode->flags.AreAllFlagsActive(DatabaseEntryNode::FlagInPak|DatabaseEntryNode::FlagInFolder))
					return QIcon(":/icons/in_folder_and_pak.png");
				if (pEntryNode->flags.AreAnyFlagsActive(DatabaseEntryNode::FlagInPak))
					return QIcon(":/icons/in_pak.png");
				if (pEntryNode->flags.AreAnyFlagsActive(DatabaseEntryNode::FlagInFolder))
					return QIcon(":/icons/in_folder.png");
			}
		}
		break;
	case Qt::ToolTipRole:
		{
			if (index.column() == eColumn_Name)
			{
				if (pEntryNode->type == DatabaseEntryNode::Leaf)
					return QString( QString(pEntryNode->descriptorTypeName.c_str()) + QString(" : ") + QString(pEntryNode->name.c_str()) );
			}
			else if (index.column() == eColumn_PakStatus)
			{
				if (pEntryNode->flags.AreAllFlagsActive(DatabaseEntryNode::FlagInPak|DatabaseEntryNode::FlagInFolder))
					return QString("In pak and folder");
				if (pEntryNode->flags.AreAnyFlagsActive(DatabaseEntryNode::FlagInPak))
					return QString("In pak");
				if (pEntryNode->flags.AreAnyFlagsActive(DatabaseEntryNode::FlagInFolder))
					return QString("In folder");
			}
		}
		break;
	}

	return QVariant();
}

bool DatabaseModel::setData(const QModelIndex &index, const QVariant &value, int role )
{
	if (!index.isValid() || (index.column() != eColumn_Name))
		return false;

	if (role != Qt::EditRole)
		return false;

	DatabaseEntryNodePtr pEntryNode = GetEntryNode(index);
	if ((pEntryNode == NULL) || (pEntryNode->type != DatabaseEntryNode::Type::Leaf))
		return false;

	if (value.toString().isEmpty())
		return false;

	return m_database.RenameRecord( pEntryNode, value.toString().toStdString().c_str() );
}

QModelIndex DatabaseModel::parent(const QModelIndex& index) const
{
	if (!index.isValid())
		return QModelIndex();

	DatabaseEntryNode* pEntryNode = GetEntryNode(index);
	if (pEntryNode == NULL)
		return QModelIndex();
	if (pEntryNode == m_database.GetRoot())
		return QModelIndex();

	DatabaseEntryNode* pParentEntryNode = pEntryNode->pParent;
	if (pParentEntryNode == NULL)
		return QModelIndex();

	if (pParentEntryNode == m_database.GetRoot())
		return QModelIndex();

	return ModelIndexFromNode(pParentEntryNode);
}

QModelIndex DatabaseModel::ModelIndexFromNode( DatabaseEntryNode* pEntryNode ) const
{
	CRY_ASSERT (pEntryNode != NULL);
	CRY_ASSERT (pEntryNode->pParent != NULL);

	for (size_t i = 0, childCount = pEntryNode->pParent->children.size(); i < childCount; ++i)
		if (pEntryNode->pParent->children[i] == pEntryNode)
			return createIndex(i, 0, pEntryNode);

	CRY_ASSERT(0);
	return createIndex(0, 0, pEntryNode);
}

const char* DatabaseModel::GetHeaderSectionName( int section ) const
{
	if (section == eColumn_Name)
	{
		return "Name";
	}
	else if (section == eColumn_PakStatus)
	{
		return "Pak";
	}

	return "";
}

/* static */ DatabaseEntryNode* DatabaseModel::GetEntryNode( const QModelIndex& index )
{
	return static_cast<DatabaseEntryNode*>(index.internalPointer());
}

void DatabaseModel::OnSignalBeginAddEntryNode(DatabaseEntryNode* pEntryNode)
{
	CRY_ASSERT(pEntryNode->pParent != NULL);

	const QModelIndex parentIndex = (pEntryNode->pParent == m_database.GetRoot()) ? QModelIndex() : ModelIndexFromNode(pEntryNode->pParent);
	const int newRowIndex = pEntryNode->pParent->children.size();
	
	beginInsertRows(parentIndex, newRowIndex, newRowIndex);
}

void DatabaseModel::OnSignalEndAddEntryNode(DatabaseEntryNode* pEntryNode)
{
	endInsertRows();
}

void DatabaseModel::OnSignalBeginDeleteEntryNode(DatabaseEntryNode* pEntryNode)
{
	CRY_ASSERT(pEntryNode != NULL);
	CRY_ASSERT(pEntryNode->pParent != NULL);

	QModelIndex parentIndex = (pEntryNode->pParent == m_database.GetRoot()) ? QModelIndex() :ModelIndexFromNode(pEntryNode->pParent);
	QModelIndex index = ModelIndexFromNode(pEntryNode);

	CRY_ASSERT(pEntryNode == GetEntryNode(index));

	beginRemoveRows(parentIndex, index.row(), index.row());
}

void DatabaseModel::OnSignalEndDeleteEntryNode()
{
	endRemoveRows();
}

void DatabaseModel::OnSignalEntryNodeDataChanged(DatabaseEntryNode* pEntryNode)
{
	CRY_ASSERT(pEntryNode != NULL);

	QModelIndex index = ModelIndexFromNode(pEntryNode);

	dataChanged(index, index);
}

#include <moc_DatabaseModel.cpp>