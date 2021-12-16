#pragma once

namespace ContextTreeFunctor
{
	template <typename A, typename B> struct AndFilter;
	template <typename A, typename B> struct OrFilter;
	template <typename A> struct NotFilter;

	template <typename T>
	struct FilterBase : public IContextTreeFilter
	{
		template <typename O>
		friend AndFilter<T, O> operator && (const T& a, const O& b)
		{
			return AndFilter<T, O>(a, b);
		}
		
		template <typename O>
		friend OrFilter<T, O> operator || (const T& a, const O& b)
		{
			return OrFilter<T, O>(a, b);
		}

		friend NotFilter<T> operator ! (const T& a)
		{
			return NotFilter<T>(a);
		}
	};

	template <typename A, typename B>
	struct OrFilter : FilterBase<OrFilter<A, B> >
	{
		OrFilter(const A& a, const B& b)
			: a(a), b(b) {}
		bool operator () (const ContextTreeNode& source) const
		{
			return a(source) || b(source);
		}

		const A& a;
		const B& b;
	};

	template <typename A, typename B>
	OrFilter<A, B> MakeOrFilter(const A& a, const B& b)
	{
		return OrFilter<A, B>(a, b);
	}

	template <typename A, typename B>
	struct AndFilter : FilterBase<AndFilter<A, B> >
	{
		AndFilter(const A& a, const B& b)
			: a(a), b(b) {}
		bool operator () (const ContextTreeNode& source) const
		{
			return a(source) && b(source);
		}

		const A& a;
		const B& b;
	};

	template <typename A, typename B>
	AndFilter<A, B> MakeAndFilter(const A& a, const B& b)
	{
		return AndFilter<A, B>(a, b);
	}

	template <typename A>
	struct NotFilter : FilterBase<NotFilter<A> >
	{
		NotFilter(const A& a)
			: a(a) {}

		bool operator () (const ContextTreeNode& source) const
		{
			return !a(source);
		}

		A a;
	};

	template <typename A>
	NotFilter<A> MakeNotFilter(const A& a)
	{
		return NotFilter<A>(a);
	}

	struct TypeFilter : FilterBase<TypeFilter>
	{
		TypeFilter(MemStatContextTypes::Type type)
			: m_type(type) {}

		bool operator () (const ContextTreeNode& source) const
		{
			return source.GetType() == m_type;
		}

		MemStatContextTypes::Type m_type;
	};

	struct NameFilter : FilterBase<NameFilter>
	{
		NameFilter(const char* name)
			: name(name)
		{}

		bool operator () (const ContextTreeNode& source) const
		{
			return strcmp(source.GetName(), name) == 0;
		}

		const char* name;
	};

	struct PhysicsLeafFilter : FilterBase<PhysicsLeafFilter>
	{
		bool operator () (const ContextTreeNode& node) const
		{
			return strcmp(node.GetName(), "Physics") == 0;
		}
	};

	struct EmptyLeafFilter : FilterBase<EmptyLeafFilter>
	{
		bool operator () (const ContextTreeNode& node) const
		{
			return node.GetSize().GetTotal().consumed > 0;
		}
	};

	struct CGFFilter : FilterBase<CGFFilter>
	{
		bool operator () (const ContextTreeNode& source) const
		{
			const char* name = source.GetName();

			size_t nameLen = strlen(name);
			if (nameLen >= 4)
			{
				size_t offset = nameLen - 4;
				if ((strcmp(name + offset, ".cgf") == 0) ||
					(strcmp(name + offset, ".cga") == 0) ||
					(strcmp(name + offset, ".cdf") == 0) ||
					(strcmp(name + offset, ".max") == 0))
				{
					return true;
				}
			}

			return false;
		}
	};

	struct PhysicsLeafFolder
	{
		bool operator () (const ContextTreeNode& parent) const
		{
			return parent.GetChildren() && (strcmp(parent.GetChildren()->GetName(), "Physics") == 0);
		}

		void operator () (ContextTreeNode& target, const ContextTreeNode& input) const
		{
			if (input.GetChildren() == NULL)
				target.GetSize() += input.GetSize(); // only include leaf nodes in the size, to avoid cached totals

			std::vector<ContextStreamOffsetSpan>& allocStreamOffsets = target.GetAllocStreamOffsets();
			allocStreamOffsets.insert(allocStreamOffsets.end(), input.GetAllocStreamOffsets().begin(), input.GetAllocStreamOffsets().end());
		}
	};

	struct MiscLeafFolder
	{
		bool operator () (const ContextTreeNode& parent) const
		{
			return parent.GetChildren() && parent.GetChildren()->GetNextSibling() == NULL && strcmp(parent.GetChildren()->GetName(), "Misc") == 0;
		}

		void operator () (ContextTreeNode& target, const ContextTreeNode& input) const
		{
			if (input.GetChildren() == NULL)
				target.GetSize() += input.GetSize(); // only include leaf nodes in the size, to avoid cached totals

			std::vector<ContextStreamOffsetSpan>& allocStreamOffsets = target.GetAllocStreamOffsets();
			allocStreamOffsets.insert(allocStreamOffsets.end(), input.GetAllocStreamOffsets().begin(), input.GetAllocStreamOffsets().end());
		}
	};

	struct NotEntityLeafFolder
	{
		bool operator () (const ContextTreeNode& parent) const
		{
			// If all children are not entity related, fold it.
			for (const ContextTreeNode* child = parent.GetChildren(); child; child = child->GetNextSibling())
			{
				if (child->GetType() == MemStatContextTypes::MSC_Entity)
					return false;
			}

			return true;
		}

		void operator () (ContextTreeNode& target, const ContextTreeNode& input) const
		{
			if (input.GetChildren() == NULL)
				target.GetSize() += input.GetSize(); // only include leaf nodes in the size, to avoid cached totals

			std::vector<ContextStreamOffsetSpan>& allocStreamOffsets = target.GetAllocStreamOffsets();
			allocStreamOffsets.insert(allocStreamOffsets.end(), input.GetAllocStreamOffsets().begin(), input.GetAllocStreamOffsets().end());
		}
	};

	struct NameGrouper
	{
		typedef const char* KeyType; // ContextTreeNode names are interned, so pointers can be compared directly for equality

		KeyType operator () (const ContextTreeNode& node) const
		{
			return node.GetName();
		}
	};
}
