#ifndef SIZETREE_H
#define SIZETREE_H

template <typename T>
class SizeTreeNode
{
	struct SizeTreeNodeNext
	{
		SizeTreeNode<T>*& operator () (SizeTreeNode<T>& a) const { return a.nextSibling; }
	};

public:
	/*
	void* operator new (size_t sz);
	void operator delete (void* p, size_t sz);
	*/

	SizeTreeNode()
		: parent(NULL)
		, children(NULL)
		, nextSibling(NULL)
	{
	}

	explicit SizeTreeNode(const T& value)
		: parent(NULL)
		, children(NULL)
		, nextSibling(NULL)
		, value(value)
	{
	}
	
	void AddChild(SizeTreeNode* child)
	{
		assert (child != NULL);
		assert (child->nextSibling == NULL);
		child->nextSibling = children;
		children = child;
	}

	template <typename LessFunc>
	void SortChildren()
	{
		SortLL<SizeTreeNode<T>, LessFunc, SizeTreeNodeNext>(children);
	}

	size_t CountChildren() const
	{
		size_t count = 0;
		for (const SizeTreeNode* c = children; c; c = c->nextSibling)
			++ count;
		return count;
	}

public:
	SizeTreeNode<T>* parent;
	SizeTreeNode<T>* children;
	SizeTreeNode<T>* nextSibling;
	T value;

private:
	SizeTreeNode(const SizeTreeNode&);
	SizeTreeNode& operator = (const SizeTreeNode&);
};

template <typename T>
class SizeTree
{
public:
	explicit SizeTree(const T& rootValue = T())
		: m_tree(new SizeTreeNode<T>(rootValue))
	{
	}
	~SizeTree()
	{
		if (m_tree)
			DeleteTree(m_tree);
	}

	SizeTreeNode<T>* GetRoot() 																{ return m_tree; }
	const SizeTreeNode<T>* GetRoot() const											{ return m_tree; }

private:
	SizeTree(const SizeTree<T>&);
	SizeTree<T>& operator = (const SizeTree<T>&);

	void DeleteTree(const SizeTreeNode<T>* node)
	{
		for (const SizeTreeNode<T>* next; node; node = next)
		{
			next = node->nextSibling;

			DeleteTree(node->children);
			delete node;
		}
	}

private:
	SizeTreeNode<T>* m_tree;
};

#endif