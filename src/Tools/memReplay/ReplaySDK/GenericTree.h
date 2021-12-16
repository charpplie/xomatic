#pragma once

#include "SizeInfo.h"

template <typename T> class GenericTreeStream;

class GenericTreeNode
{
public:
	void* operator new (size_t sz);
	void operator delete (void* p, size_t sz);

public:
	GenericTreeNode()
	{
		memset(this, 0, sizeof(*this));
	}

	template <typename KeyT>
	explicit GenericTreeNode(KeyT key)
	{
		assert(sizeof(KeyT) <= sizeof(m_key));
		assert(IsPod<KeyT>::Result);

		new (m_key) KeyT(key);
		id = 0;
		parent = 0;
		children = 0;
		sibling = 0;
	}
	
	template <typename KeyT>
	KeyT GetKey() const
	{
		assert (sizeof(KeyT) <= sizeof(m_key));
		assert(IsPod<KeyT>::Result);
		return *reinterpret_cast<const KeyT*>(m_key);
	}

	template <typename KeyT>
	void SetKey(KeyT key)
	{
		assert (sizeof(KeyT) <= sizeof(m_key));
		assert(IsPod<KeyT>::Result);
		new (m_key) KeyT (key);
	}

	template <typename T>
	T& operator [] (GenericTreeStream<T>& stream) { return stream[id]; }

	template <typename T>
	const T& operator [] (const GenericTreeStream<T>& stream) const { return stream[id]; }

	size_t CountChildren() const
	{
		size_t count = 0;
		for (GenericTreeNode* c = children; c; c = c->sibling)
			++ count;
		return count;
	}

public:
	size_t id;
	GenericTreeNode* parent;
	GenericTreeNode* children;
	GenericTreeNode* sibling;

private:
	char m_key[sizeof(void*)];
};

struct GenericTreeNode_Less
{
	bool operator () (const GenericTreeNode& a, const GenericTreeNode& b) const { return a.GetKey<size_t>() < b.GetKey<size_t>(); }
};

struct GenericTreeNode_Next
{
	GenericTreeNode*& operator () (GenericTreeNode& a) const { return a.sibling; }
};

class IGenericTreeStream
{
public:
	virtual ~IGenericTreeStream() {}

	virtual IGenericTreeStream* CloneDefinition() const = 0;

	virtual void Resize(size_t count) = 0;
	virtual void Expand() = 0;
};

template <typename T>
class GenericTreeStream : public IGenericTreeStream
{
public:
	T& operator [] (size_t idx) { return m_items[idx]; }
	const T& operator [] (size_t idx) const { return m_items[idx]; }

	IGenericTreeStream* CloneDefinition() const { return new GenericTreeStream<T>(); }

	void Resize(size_t count)
	{
		m_items.resize(count);
	}

	void Expand()
	{
		m_items.push_back(T());
	}

private:
	std::deque<T> m_items;
};

class GenericTree
{
public:
	GenericTree();
	~GenericTree();

	GenericTreeNode* GetRoot() const { return m_root; }

	void AddChild(GenericTreeNode* parent, GenericTreeNode* node);

	bool HasStream(const std::string& name) const;
	IGenericTreeStream* GetStream(const std::string& name);
	const IGenericTreeStream* GetStream(const std::string& name) const;

	template <typename T>
	GenericTreeStream<T>* GetStream(const std::string& name)
	{
		return reinterpret_cast<GenericTreeStream<T>*>(GetStream(name));
	}

	template <typename T>
	const GenericTreeStream<T>* GetStream(const std::string& name) const
	{
		return reinterpret_cast<const GenericTreeStream<T>*>(GetStream(name));
	}

	void AddStream(const std::string& name, const SharedPtr<IGenericTreeStream>& stream);

private:
	static void DeleteTree(GenericTreeNode* n);

private:
	GenericTree(const GenericTree&);
	GenericTree& operator = (const GenericTree&);

private:
	GenericTreeNode* m_root;
	std::map<std::string, SharedPtr<IGenericTreeStream> > m_streams;
	size_t m_count;
};

void SumSizes(GenericTreeNode* node, GenericTreeStream<SizeInfoGroups>& exclusive, GenericTreeStream<SizeInfoGroups>& inclusive);
