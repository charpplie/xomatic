#include "stdafx.h"
#include "CodeTreeUtility.h"

#include "ReplaySDK/GenericTree.h"

namespace
{
	SharedPtr<GenericTree> CreateTree()
	{
		SharedPtr<GenericTree> destTree = new GenericTree();
		destTree->AddStream("Inclusive", new GenericTreeStream<SizeInfoGroups>());
		destTree->AddStream("Exclusive", new GenericTreeStream<SizeInfoGroups>());
		return destTree;
	}

	struct GatherCodeBottomUpEnumeratee
	{
		GatherCodeBottomUpEnumeratee(GenericTree* dst, const GenericTree* src, const std::vector<TAddress>& addresses)
			: m_dst(dst)
			, m_dstRoot(dst->GetRoot())
			, m_srcExclusive(src->GetStream<SizeInfoGroups>("Exclusive"))
			, m_dstExclusive(dst->GetStream<SizeInfoGroups>("Exclusive"))
			, m_addresses(&addresses)
		{
			m_stack.reserve(1024);
			m_matchStack.reserve(1024);
		}

		void Enter(TAddress key)
		{
			m_stack.push_back(key);

			if (std::binary_search(m_addresses->begin(), m_addresses->end(), key))
				m_matchStack.push_back(m_stack.size() - 1);
		}

		void Leave(const GenericTreeNode* node)
		{
			if (!m_matchStack.empty())
			{
				// match bottom part of stack
				GenericTreeNode* dst = m_dstRoot;

				for (size_t j = m_matchStack.back(), jc = m_stack.size(); j < jc; ++ j)
				{
					GenericTreeNode* dstMatch = dst->children;
					for (; dstMatch && dstMatch->GetKey<TAddress>() != m_stack[j]; dstMatch = dstMatch->sibling)
						;
					if (!dstMatch)
					{
						dstMatch = new GenericTreeNode(m_stack[j]);
						m_dst->AddChild(dst, dstMatch);
					}

					dst = dstMatch;
				}

				(*dst)[*m_dstExclusive] += (*node)[*m_srcExclusive];

				m_stack.pop_back();
				if (m_matchStack.back() == m_stack.size())
					m_matchStack.pop_back();
			}
			else
			{
				m_stack.pop_back();
			}
		}

		GenericTree* m_dst;
		GenericTreeNode* m_dstRoot;
		const GenericTreeStream<SizeInfoGroups>* m_srcExclusive;
		GenericTreeStream<SizeInfoGroups>* m_dstExclusive;
		const std::vector<TAddress>* m_addresses;

		std::vector<TAddress> m_stack;
		std::vector<size_t> m_matchStack;
	};

	template <typename KeyT, typename EnumerateeT>
	void EnumerateTree(const GenericTreeNode* node, EnumerateeT& enumeratee)
	{
		enumeratee.Enter(node->GetKey<KeyT>());

		for (const GenericTreeNode* child = node->children; child; child = child->sibling)
			EnumerateTree<KeyT>(child, enumeratee);

		enumeratee.Leave(node);
	}
}

SharedPtr<GenericTree> ReverseCode(const GenericTree& other, const std::vector<TAddress>& keySet)
{
	SharedPtr<GenericTree> dstTree = CreateTree();

	GenericTreeStream<SizeInfoGroups>& dstExclusive = *dstTree->GetStream<SizeInfoGroups>("Exclusive");
	const GenericTreeStream<SizeInfoGroups>& srcInclusive = *other.GetStream<SizeInfoGroups>("Inclusive");

	std::vector<const GenericTreeNode*> stck;
	stck.reserve(256);

	stck.push_back(other.GetRoot());

	std::vector<const GenericTreeNode*> roots;
	roots.reserve(256);

	while (stck.empty() == false)
	{
		const GenericTreeNode* top = stck.back();
		stck.pop_back();

		if (std::binary_search(keySet.begin(), keySet.end(), top->GetKey<TAddress>()))
		{
			roots.push_back(top);
		}
		else
		{
			for (const GenericTreeNode* c = top->children; c; c = c->sibling)
				stck.push_back(c);
		}
	}

	for (std::vector<const GenericTreeNode*>::const_iterator it = roots.begin(), itEnd = roots.end(); it != itEnd; ++ it)
	{
		const GenericTreeNode* root = *it;

		GenericTreeNode* dest = dstTree->GetRoot();

		while (root)
		{
			GenericTreeNode* dc = NULL;

			for (dc = dest->children; dc && (dc->GetKey<TAddress>() != root->GetKey<TAddress>()); dc = dc->sibling)
				;

			if (!dc)
			{
				dc = new GenericTreeNode(root->GetKey<TAddress>());
				dstTree->AddChild(dest, dc);
			}

			if (!root->parent)
				(*dc)[dstExclusive] += (**it)[srcInclusive];

			dest = dc;
			root = root->parent;
		}
	}

	SumSizes(dstTree->GetRoot(), dstExclusive, *dstTree->GetStream<SizeInfoGroups>("Inclusive"));

	return dstTree;
}
		
static void GatherCodeTopDownRecurse(
													GenericTree& destTree, GenericTreeNode *dest, const GenericTreeNode *root,
													GenericTreeStream<SizeInfoGroups>& destExclusive,
													const GenericTreeStream<SizeInfoGroups>& srcExclusive)
{
	GenericTreeNode* dc = NULL;
	GenericTreeNode* sc = NULL;

	for (dc = dest->children; dc && (dc->GetKey<TAddress>() != root->GetKey<TAddress>()); dc = dc->sibling)
		;

	if (!dc)
	{
		dc = new GenericTreeNode(root->GetKey<TAddress>());
		destTree.AddChild(dest, dc);
	}

	(*dc)[destExclusive] += (*root)[srcExclusive];

	for (sc = root->children; sc; sc = sc->sibling)
		GatherCodeTopDownRecurse(destTree, dc, sc, destExclusive, srcExclusive);
}

SharedPtr<GenericTree> GatherCodeTopDown(const GenericTree& other, const std::vector<TAddress>& addresses)
{
	SharedPtr<GenericTree> destTree = CreateTree();

	GenericTreeStream<SizeInfoGroups>& destExclusive = *destTree->GetStream<SizeInfoGroups>("Exclusive");
	const GenericTreeStream<SizeInfoGroups>& srcExclusive = *other.GetStream<SizeInfoGroups>("Exclusive");

	std::vector<const GenericTreeNode*> stck, roots;
	stck.reserve(256);
	roots.reserve(256);

	stck.push_back(other.GetRoot());

	while (!stck.empty())
	{
		const GenericTreeNode* top = stck.back();
		stck.pop_back();

		if (std::binary_search(addresses.begin(), addresses.end(), top->GetKey<TAddress>()))
		{
			roots.push_back(top);
		}
		else
		{
			for (const GenericTreeNode* c = top->children; c; c = c->sibling)
				stck.push_back(c);
		}
	}

	for (std::vector<const GenericTreeNode*>::const_iterator it = roots.begin(), itEnd = roots.end(); it != itEnd; ++ it)
	{
		const GenericTreeNode* root = *it;
		GenericTreeNode* dest = destTree->GetRoot();

		GatherCodeTopDownRecurse(*destTree, dest, root, destExclusive, srcExclusive);
	}

	SumSizes(destTree->GetRoot(), destExclusive, *destTree->GetStream<SizeInfoGroups>("Inclusive"));

	return destTree;
}

SharedPtr<GenericTree> GatherCodeBottomUp(const GenericTree& other, const std::vector<TAddress>& addresses)
{
	SharedPtr<GenericTree> destTree = CreateTree();

	GenericTreeStream<SizeInfoGroups>& destExclusive = *destTree->GetStream<SizeInfoGroups>("Exclusive");
	const GenericTreeStream<SizeInfoGroups>& srcExclusive = *other.GetStream<SizeInfoGroups>("Exclusive");

	const GenericTreeNode* srcRoot = other.GetRoot();

	GatherCodeBottomUpEnumeratee f(&*destTree, &other, addresses);
	EnumerateTree<TAddress>(other.GetRoot(), f);

	SumSizes(destTree->GetRoot(), *destTree->GetStream<SizeInfoGroups>("Exclusive"), *destTree->GetStream<SizeInfoGroups>("Inclusive"));

	return destTree;
}

static void ExcludeRecurse(GenericTree& destTree,
													 GenericTreeStream<SizeInfoGroups>& destExclusive,
													 const GenericTreeStream<SizeInfoGroups>& srcExclusive,
													 GenericTreeNode* dest, const GenericTreeNode* src, const std::vector<TAddress>& addresses)
{
	dest->SetKey(src->GetKey<TAddress>());
	(*dest)[destExclusive] = (*src)[srcExclusive];

	for (const GenericTreeNode* sc = src->children; sc; sc = sc->sibling)
	{
		if (std::binary_search(addresses.begin(), addresses.end(), sc->GetKey<TAddress>()))
			continue;

		GenericTreeNode* nc = new GenericTreeNode();
		destTree.AddChild(dest, nc);

		ExcludeRecurse(destTree, destExclusive, srcExclusive, nc, sc, addresses);
	}
}

SharedPtr<GenericTree> ExcludeCode(const GenericTree& other, const std::vector<TAddress>& addresses)
{
	SharedPtr<GenericTree> destTree = CreateTree();

	ExcludeRecurse(*destTree, *destTree->GetStream<SizeInfoGroups>("Exclusive"), *other.GetStream<SizeInfoGroups>("Exclusive"), destTree->GetRoot(), other.GetRoot(), addresses);

	SumSizes(destTree->GetRoot(), *destTree->GetStream<SizeInfoGroups>("Exclusive"), *destTree->GetStream<SizeInfoGroups>("Inclusive"));

	return destTree;
}
