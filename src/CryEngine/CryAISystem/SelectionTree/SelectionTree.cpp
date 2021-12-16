#include "StdAfx.h"
#include "SelectionTree.h"
#include "SelectionContext.h"
#include "SelectionTreeManager.h"


SelectionTree::SelectionTree()
: m_currentNodeID(0)
, m_templateID(0)
{
}

SelectionTree::SelectionTree(const SelectionTreeTemplateID& templateID)
: m_currentNodeID(0)
, m_templateID(templateID)
{
}

void SelectionTree::Clear()
{
	m_currentNodeID = 0;
	SelectionTreeNodes().swap(m_nodes);
}

void SelectionTree::Reset()
{
	m_currentNodeID = 0;
}

const SelectionNodeID& SelectionTree::Evaluate(const SelectionVariables& variables)
{
	SelectionContext context(m_nodes, m_currentNodeID, variables);
	if (!m_nodes.front().Evaluate(context, m_currentNodeID))
		m_currentNodeID = 0;

	return m_currentNodeID;
}

SelectionNodeID SelectionTree::AddNode(const SelectionTreeNode& node)
{
	m_nodes.push_back(node);
	m_nodes.back().SetNodeID(m_nodes.size());
	return m_nodes.size();
}

SelectionTreeNode& SelectionTree::GetNode(const NodeID& nodeID)
{
	return m_nodes[nodeID - 1];
}

const SelectionTreeNode& SelectionTree::GetNode(const NodeID& nodeID) const
{
	return m_nodes[nodeID - 1];
}

const SelectionNodeID& SelectionTree::GetCurrentNodeID() const
{
	return m_currentNodeID;
}

const SelectionTreeTemplateID& SelectionTree::GetTemplateID() const
{
	return m_templateID;
}

const SelectionTreeTemplate& SelectionTree::GetTemplate() const
{
	return gAIEnv.pSelectionTreeManager->GetTreeTemplate(m_templateID);
}

uint32 SelectionTree::GetNodeCount() const
{
	return m_nodes.size();
}

const SelectionTreeNode& SelectionTree::GetNodeAt(uint32 index) const
{
	assert(index < m_nodes.size());

	return m_nodes[index];
}

void SelectionTree::ReserveNodes(uint32 nodeCount)
{
	m_nodes.reserve(nodeCount);
}

void SelectionTree::Swap(SelectionTree& other)
{
	std::swap(m_templateID, other.m_templateID);
	std::swap(m_currentNodeID, other.m_currentNodeID);
	m_nodes.swap(other.m_nodes);
}

bool SelectionTree::Empty() const
{
	return m_nodes.empty();
}

void SelectionTree::Validate()
{
	SelectionTreeNodes(m_nodes).swap(m_nodes);
}

void SelectionTree::DebugDraw()
{
}