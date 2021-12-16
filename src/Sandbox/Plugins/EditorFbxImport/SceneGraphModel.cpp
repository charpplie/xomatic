// Copyright (c) 1999-2014 Crytek.

#include "StdAfx.h"
#include "SceneGraphModel.h"
#include "FbxImportPlugin.h"
#include <QApplication>
#include <QPalette>

#define DEFAULT_SELECT_NODES 0

static QString GetNodeClassName(ENodeClass type)
{
	switch(type)
	{
	case eNC_Animation:
		return QObject::tr("Animation");
	case eNC_Geometry:
		return QObject::tr("Geometry");
	case eNC_Material:
		return QObject::tr("Material");
	case eNC_Skeleton:
		return QObject::tr("Skeleton");
	default:
		assert(false);
		return QString();
	}
}

CSceneGraphModel::CSceneGraphModel(QObject *pParent /*= nullptr*/) 
		: QAbstractItemModel(pParent), m_scene(), m_bSuppressingLayout(false)
		, m_bShouldLayout(false), m_nesting(0)
{ 
	// Set up default options
	m_options.iconColumn = 0;
	m_options.checkboxColumn = 1;
	m_options.bHideNonRootBones = true;
	for (int i = 0; i < eNC_COUNT; ++i)
	{
		m_options.bCheckableType[i] = false;
		m_options.bHideType[i] = i == eNC_Unknown;
	}

	// Update the view
	ComputeViewValues(true);
}

QPixmap CSceneGraphModel::GetIconForType(ENodeClass type)
{
	// Get the shared icons from the plugin instance
	CFbxImportPlugin *pPlugin = CFbxImportPlugin::GetInstance();
	return pPlugin->GetIcon(type);
}

QString CSceneGraphModel::NodePropertyToString(ENodeProperty property)
{
	switch (property)
	{
	case eNP_Name:
		return tr("Name");
	case eNP_Type:
		return tr("Import ?");
	case eNP_Description:
		return tr("Description");
	default:
		assert(false);
		return QStringLiteral("Unknown");
	}
}

QVariant CSceneGraphModel::GetNodeProperty(const SSceneNode *pNode, ENodeProperty property) const
{
	switch (property)
	{
	case eNP_Name:
		return pNode->model.name;
	case eNP_Type:
		if (HasCheckbox(pNode))
		{	
			return pNode->model.type != eNC_None ? GetNodeClassName(pNode->model.type) : tr("Group");
		}
		return QString();
	case eNP_Description:
		return pNode->model.description;
	default:
		assert(false);
		break;
	}
	return QVariant();
}

bool CSceneGraphModel::HasCheckbox(const SSceneNode *pNode) const
{
	if (m_options.bCheckableType[pNode->model.type])
	{
		if (pNode->model.type == eNC_None)
		{
			// This is a dummy node, it only has a checkbox if it has at least one non-dummy child that is checkable
			bool bAnyChild = false;
			pNode->DepthFirstSearch([&bAnyChild, this](const SSceneNode *pChild)
			{
				if (m_options.bCheckableType[pChild->model.type] && pChild->model.type != eNC_None)
				{
					bAnyChild = true;
				}
			});
			return bAnyChild;
		}
		else
		{
			// This node is not a dummy and checkable
			return true;
		}
	}
	return false;
}

bool CSceneGraphModel::HasCheckedParent(const SSceneNode *pNode)
{
	bool bResult = false;
	pNode->RootToNodeSearch([&bResult, pNode](const SSceneNode *pParent)
	{
		bResult |= (pParent != pNode) && pParent->view.bChecked;
	});
	return bResult;
}

QModelIndex CSceneGraphModel::index(int row, int column, const QModelIndex &parent) const
{
	const SSceneNode *pParent = GetNode(parent);
	if (!pParent) pParent = m_scene.GetRootNode();
	if (!pParent || row >= pParent->view.visibleChildren.size())
	{
		// Invalid index request
		return QModelIndex();
	}
	const SSceneNode *pNode = pParent->view.visibleChildren[row];
	return createIndex(row, column, (void *)pNode);
}

QModelIndex CSceneGraphModel::parent(const QModelIndex &index) const
{
	const SSceneNode *pNode = GetNode(index);
	if (pNode && pNode->model.pParent && pNode->model.pParent != m_scene.GetRootNode())
	{
		return createIndex(pNode->model.pParent->view.index, 0, (void *)pNode->model.pParent);
	}
	return QModelIndex();
}

int CSceneGraphModel::rowCount(const QModelIndex &index) const
{
	const SSceneNode *pNode = GetNode(index);
	if (!pNode)
	{
		pNode = m_scene.GetRootNode();
	}
	return pNode ? (int)pNode->view.visibleChildren.size() : 0;
}

Qt::ItemFlags CSceneGraphModel::flags(const QModelIndex &index) const
{
	const SSceneNode *pNode = GetNode(index);
	if (!pNode) return 0;
	Qt::ItemFlags result = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	if (HasCheckbox(pNode) && index.column() == m_options.checkboxColumn)
	{
		result |= Qt::ItemIsUserCheckable;
	}
	return result;
}

void CSceneGraphModel::ComputeViewValues(bool bInit)
{
	const SSceneNode *pRoot = m_scene.GetRootNode();
	if (!pRoot) return;

	pRoot->DepthFirstSearch([bInit, this](const SSceneNode *pNode)
	{
		// Initialize computed constant properties and UI defaults
		ENodeClass nodeClass = pNode->model.type;
		if (bInit)
		{
			if (nodeClass == eNC_Skeleton)
			{
				// For skeleton, default the checkbox to the RC support, but only for the root node of each skeleton
				ENodeClass parentClass = pNode->model.pParent->model.type;
				pNode->view.bChecked = (DEFAULT_SELECT_NODES != 0) && IsImportSupported(pNode->model.type) && parentClass != eNC_Skeleton;
			}
			else
			{
				// Don't pick anything by default
				// Could also use IsImportSupported(pNode->model.type), but this seems a bad idea for larger scenes
				pNode->view.bChecked = (DEFAULT_SELECT_NODES != 0); 
			}
		}

		// Check for visibility
		bool bHidden = m_options.bHideType[pNode->model.type];
		if (nodeClass == eNC_Skeleton && m_options.bHideNonRootBones)
		{
			ENodeClass parentClass = pNode->model.pParent->model.type;
			if (parentClass == eNC_Skeleton) bHidden = true;
		}

		// Set up this node so the parent can determine if it's visible
		pNode->view.index = bHidden ? -1 : 0;
		pNode->view.visibleChildren.clear();

		// Since our search is depth-first, we already know the visibility properties of our children
		// Thus, set up the visibleChildren and indices for model use
		int index = 0;
		for(QList<SSceneNode *>::const_iterator i = pNode->model.children.begin(); i != pNode->model.children.end(); ++i)
		{
			const SSceneNode *pChild = *i;
			bool bChildVisible = pChild->view.index != -1;
			if (bChildVisible)
			{
				pNode->view.visibleChildren.push_back(pChild);
				pChild->view.index = index++;
			}
		}
		if (index)
		{
			// If any child is visible, then so should this node
			pNode->view.index = 0;
		}
	});
}

QVariant CSceneGraphModel::data(const QModelIndex &index, int role) const
{
	const SSceneNode *pNode = GetNode(index);
	if (pNode && role == Qt::DisplayRole)
	{
		// Node property as string
		return GetNodeProperty(pNode, (ENodeProperty)index.column());
	}
	if (pNode && role == Qt::CheckStateRole && index.column() == m_options.checkboxColumn)
	{
		// If any parent of the same type is selected, show this node as half-selected
		if (HasCheckbox(pNode))
		{
			// If any parent is checked, then we are always partially checked
			if (HasCheckedParent(pNode)) return int(Qt::PartiallyChecked);

			// Return current checked state
			return int(pNode->view.bChecked ? Qt::Checked : Qt::Unchecked);
		}	
	}
	if (pNode && role == Qt::DecorationRole && index.column() == m_options.iconColumn)
	{
		// Node icon
		QPixmap icon = GetIconForType(pNode->model.type);
		if (!icon.isNull()) return icon;			
	}
	
	// Just return base-class information, which should be a reasonable default
	return QVariant();
}

bool CSceneGraphModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	const SSceneNode *pNode = GetNode(index);
	if (pNode && role == Qt::CheckStateRole)
	{
		bool bOk;
		int iVal = value.toInt(&bOk);
		if (bOk)
		{
			// Check for partially checked nodes, and don't allow the state to change
			// However, if we are in a nested call the state is changed from code, so we need to accept it always
			if (m_nesting == 0)
			{
				int iPrevVal = data(index, Qt::CheckStateRole).toInt();
				if (iPrevVal == Qt::PartiallyChecked) return false;
			}
			pNode->view.bChecked = iVal == Qt::Checked;

			// Trigger data changed events for children
			// Uncheck all child nodes, since RC will include those anyway
			// This will ensure the children's checkboxes take the appropriate state
			m_nesting++;
			dataChanged(index, index, QVector<int>(1, Qt::CheckStateRole));
			for (QList<const SSceneNode *>::const_iterator it = pNode->view.visibleChildren.begin(); it != pNode->view.visibleChildren.end(); ++it)
			{
				setData(this->index((*it)->view.index, index.column(), index), (int)Qt::Unchecked, Qt::CheckStateRole);
			}
			m_nesting--;

			// Accept the change
			return true;
		}
	}
	return false;
}

QVariant CSceneGraphModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
	{
		return NodePropertyToString((ENodeProperty)section);
	}
	return QVariant();
}

void CSceneGraphModel::HideBones(bool bHide)
{
	if (m_options.bHideNonRootBones == bHide) return;
	m_options.bHideNonRootBones = bHide;
	RequireLayout();
}

void CSceneGraphModel::SetTypeOptions(ENodeClass type, bool bVisible, bool bCheckable)
{
	assert(type < eNC_COUNT);
	bool bLayout = false;
	bool bHiding = m_options.bHideType[type];
	if (bHiding == bVisible)
	{
		m_options.bHideType[type] = !bVisible;
		bLayout = true;
	}
	bool bChecking = m_options.bCheckableType[type];
	if (bChecking != bCheckable)
	{
		m_options.bCheckableType[type] = bCheckable;
		bLayout = true;
	}
	if (bLayout)
	{
		RequireLayout();
	}
}

void CSceneGraphModel::BeginChangeOptions()
{
	assert(!m_bSuppressingLayout);
	m_bSuppressingLayout = true;
	m_bShouldLayout = false;
}

void CSceneGraphModel::EndChangeOptions()
{
	assert(m_bSuppressingLayout);
	m_bSuppressingLayout = false;
	if (m_bShouldLayout) RequireLayout();
}

void CSceneGraphModel::RequireLayout()
{
	if (m_bSuppressingLayout) m_bShouldLayout = true;
	else
	{
		ComputeViewValues(false);
		layoutChanged();
	}
}

void CSceneGraphModel::SetIconColumnIndex(int index)
{
	if (m_options.iconColumn != index)
	{
		m_options.iconColumn = index;
		RequireLayout();
	}
}

void CSceneGraphModel::SetCheckboxColumnIndex(int index)
{
	if (m_options.checkboxColumn != index)
	{
		m_options.checkboxColumn = index;
		RequireLayout();
	}
}

void CSceneGraphModel::GetSelectedItems(QList<const SSceneNode *> &list) const
{
	const SSceneNode *root = m_scene.GetRootNode();
	root->DepthFirstSearch([&list, this](const SSceneNode *pNode)
	{
		if (m_options.bCheckableType[pNode->model.type])
		{
			if (pNode->view.bChecked)
			{
				list.push_back(pNode);
			}
		}
	});
}
