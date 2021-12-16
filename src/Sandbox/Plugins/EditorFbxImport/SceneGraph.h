// Copyright (c) 1999-2014 Crytek.

#pragma once
#include "RCCapabilities.h"
#include <QString>
#include <QList>

// A node in the scene graph
struct SSceneNode
{
	// The model is populated from the FBX data provided
	struct TModel
	{
		// Child nodes
		QList<SSceneNode *> children;
	
		// Parent node
		SSceneNode *pParent;

		// Type of node
		ENodeClass type;

		// Name of this node
		// This is the same name as the FBX node name
		// Note: Can contain unicode sequences
		QString name;

		// Description of this node
		// This information is generated from the FBX data by this plugin
		QString description;
	} model;

	// The view data is used and manipulated by the UI
	// Since we only have const SSceneNode instances, this struct is mutable
	mutable struct TView
	{
		// The index in the parent list of items
		// Qt requests this frequently, so we cache it
		// This is always equal to parent->visibleChildren.findIndex(this), or -1 if this node is not visible currently
		int index;

		// The visible children, as computed by the CSceneGraphModel
		// This is a subset of the model.children collection
		QList<const SSceneNode *> visibleChildren;

		// If set, this node is currently checked in the UI
		// Do not set this variable from code, it's always overwritten when the UI state changes
		bool bChecked;
	} view;

	// Visit child nodes in depth-first order
	template<typename Visitor>
	void DepthFirstSearch(const Visitor &visitor) const
	{
		for (QList<SSceneNode *>::const_iterator i = model.children.begin(); i != model.children.end(); ++i)
		{
			(*i)->DepthFirstSearch(visitor);
		}
		visitor(this);
	}

	// Visit parent nodes from root to this one
	template<typename Visitor>
	void RootToNodeSearch(const Visitor &visitor) const
	{
		if (model.pParent) model.pParent->RootToNodeSearch(visitor);
		visitor(this);
	}

	// Check if this node only has children of the specified type
	bool HasOnlyChildrenOfType(ENodeClass type) const
	{
		bool bResult = true;
		DepthFirstSearch([&bResult, type, this](const SSceneNode *pNode)
		{ 
			if (pNode != this)
			{
				bResult &= pNode->model.type == type;
			}
		});
		return bResult;
	}

	// The number of children that have the specified type
	// Includes this node, if it has the specified type
	int CountChildrenOfType(ENodeClass type) const
	{
		int result = 0;
		DepthFirstSearch([&result, type](const SSceneNode *pNode)
		{ 
			if (pNode->model.type == type) result++;
		});
		return result;
	}

	// Check if this is the FBX root node
	bool IsRootNode() const
	{
		// In this tree model, a root node is one that is on level 0 or level 1
		// The reason is that we have several root "categories" that are shown, and they are implemented as level 1 nodes
		return model.pParent == nullptr || model.pParent->model.pParent == nullptr;
	}

	// Path of this node, from (but not including) the FBX scene root
	// The returned string can be escaped for consumption by RC
	QString GetFullPath(bool bEscape) const
	{
		QString result;
		RootToNodeSearch([&result, bEscape, this](const SSceneNode *pNode)
		{ 
			if (!pNode->IsRootNode())
			{
				if (!result.isEmpty())
				{
					result += GetNodeNameSeparator();
				}
				QString nodeName = pNode->model.name;
				if (bEscape)
				{
					EscapeNodeName(nodeName);
				}
				result += nodeName;
			}
		});
		return result;
	}

	// Test if this node can be used as an origin node
	bool IsValidOriginNode() const
	{
		switch (model.type)
		{
		case eNC_Animation:
		case eNC_Material:
			return false;
		default:
			if (IsRootNode()) return false;
			return true;
		}
	}
};

// Utility for retrieving the scene-graph information from an FBX file
class CSceneGraph
{
public:
	// Construct empty scene graph
	CSceneGraph() : m_pRoot(nullptr) {}

	// Construct the scene graph from an FBX file
	CSceneGraph(const char *pFbxPath);

	// Clean up all nodes in the graph
	~CSceneGraph();

	// Get the root node of the scene graph
	// returns NULL if the load failed
	const SSceneNode *GetRootNode() const
	{ 
		return m_pRoot;
	}

	// Get the size of a logical unit in cm
	// returns 0.0 if the load failed
	double GetUnitInCm() const
	{
		return m_dUnitInCm;
	}

	// Swap implementation of scene graph
	static void Swap(CSceneGraph &left, CSceneGraph &right)
	{
		std::swap(left.m_pRoot, right.m_pRoot);
		std::swap(left.m_dUnitInCm, right.m_dUnitInCm);
	}

	// An origin node
	struct SOriginNode
	{
		// The path to display in the UI
		QString displayPath;

		// The escaped path to the node, or empty string if this is the "world" node
		QString escapedPath;

		// Test if this is the world node
		bool IsWorldNode() const
		{
			return escapedPath.isEmpty();
		}
	};

	// Retrieve all valid origin nodes in this scene
	void GetValidOriginNodes(QList<SOriginNode> &target) const
	{
		{
			// Origin is selected item (for each item separately)
			SOriginNode item;
			item.displayPath = QStringLiteral("<local>");
			item.escapedPath = QStringLiteral("/local");
			target.push_back(item);
		}
		{
			// Origin is scene root
			SOriginNode item;
			item.displayPath = QStringLiteral("<world>");
			item.escapedPath = QStringLiteral("/world");
			target.push_back(item);
		}

		// Origin is specific node
		GetRootNode()->DepthFirstSearch([&target](const SSceneNode *pNode)
		{
			if (pNode->IsValidOriginNode())
			{
				SOriginNode item;
				item.displayPath = pNode->GetFullPath(false);
				if (!item.displayPath.isEmpty())
				{
					item.escapedPath = pNode->GetFullPath(true);
					target.push_back(item);
				}
			}
		});
	}

private:
	// The root node of the graph
	SSceneNode *m_pRoot;

	// The number of cm in a logical unit
	double m_dUnitInCm;
};