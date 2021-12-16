#include "stdafx.h"
#include "GenericTree.h"

static PoolAllocator<GenericTreeNode, 4096> s_nodePool;

void* GenericTreeNode::operator new (size_t sz)
{
	return s_nodePool.Allocate();
}

void GenericTreeNode::operator delete (void* p, size_t sz)
{
	s_nodePool.Free(p);
}

GenericTree::GenericTree()
	: m_root(NULL)
	, m_count(1)
{
	m_root = new GenericTreeNode();
}

GenericTree::~GenericTree()
{
	DeleteTree(m_root);
}

void GenericTree::AddChild(GenericTreeNode* parent, GenericTreeNode* node)
{
	assert (!node->parent);

	node->parent = parent;
	node->sibling = parent->children;
	parent->children = node;

	node->id = m_count ++;

	for (std::map<std::string, SharedPtr<IGenericTreeStream> >::iterator it = m_streams.begin(), itEnd = m_streams.end();
		it != itEnd;
		++ it)
	{
		it->second->Expand();
	}
}

bool GenericTree::HasStream(const std::string& name) const
{
	return m_streams.find(name) != m_streams.end();
}

IGenericTreeStream* GenericTree::GetStream(const std::string& name)
{
	std::map<std::string, SharedPtr<IGenericTreeStream> >::iterator it = m_streams.find(name);
	if (it != m_streams.end())
	{
		return &*it->second;
	}

	return NULL;
}

const IGenericTreeStream* GenericTree::GetStream(const std::string& name) const
{
	std::map<std::string, SharedPtr<IGenericTreeStream> >::const_iterator it = m_streams.find(name);
	if (it != m_streams.end())
	{
		return &*it->second;
	}

	return NULL;
}

void GenericTree::AddStream(const std::string& name, const SharedPtr<IGenericTreeStream>& stream)
{
	if (!stream.IsValid())
		return;

	stream->Resize(m_count);
	m_streams.insert(std::make_pair(name, stream));
}

void GenericTree::DeleteTree(GenericTreeNode* n)
{
	for (GenericTreeNode* c = n->children, *nxt; c; c = nxt)
	{
		nxt = c->sibling;
		DeleteTree(c);
	}
	delete n;
}

void SumSizes(GenericTreeNode* node, GenericTreeStream<SizeInfoGroups>& exclusive, GenericTreeStream<SizeInfoGroups>& inclusive)
{
	SizeInfoGroups& incl = (*node)[inclusive];
	incl = (*node)[exclusive];

	for (GenericTreeNode* child = node->children; child; child = child->sibling)
	{
		SumSizes(child, exclusive, inclusive);

		incl += (*child)[inclusive];
	}

	SortLL<GenericTreeNode, GenericTreeNode_Less, GenericTreeNode_Next>(node->children);
}
