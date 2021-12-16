#if !defined(__TREE_H__)
#define __TREE_H__


#include <map>

namespace NewBehaviourTree
{
	struct	Node;
	class		BehaviourTree;

	struct Condition
	{
		uint32	NamehHash;
		bool		value;

		bool		defaultValue;

		Condition() : defaultValue(false)
		{}

		operator bool() const
		{
			return value;
		}

		void	Reset()	{ value = defaultValue; }
		void	Parse(XmlNodeRef &xmlNode);
	};

	typedef stl::hash_map<uint32, Condition> ConditionContainer;


	struct Command
	{
		enum CommandState
		{
			CS_INVALID,
			CS_STOPPED,
			CS_RUNNING,
			CS_COMMANDSRESET,
			CS_DONTEXECUTE,
		};

		enum CommandType
		{
			CT_INVALID,
			CT_SETBEHAVIOUR,
			CT_SETGOALPIPE,
			CT_TRANSITION,
			CT_SETVALUE,
			CT_UNSETVALUE
		};

		CommandState	m_State;
		CommandType		m_CommandType;
		float					m_fTimeout;
		string				m_Command;
		bool					m_NonBlocking;

		std::vector<uint32> Conditions;

		Command() : m_State(CS_STOPPED), m_CommandType(CT_INVALID), m_fTimeout(-1.0f), m_NonBlocking(false)
		{}

		void					Parse(XmlNodeRef &xmlNode, Node &rootNode);
		CommandState	Execute(BehaviourTree &tree, IAIObject &object, float fTimeDelta);

		bool					IsNonBlocking() const { return m_NonBlocking;}
	};

	typedef std::vector<Command>	CommandQueue;

	struct Node
	{
		uint32								m_NameHash;
		std::vector<Command>	m_Commands;
		std::vector<uint32>		m_Conditions;

#if defined(_DEBUG)
		string								m_NameReadable;
		string								m_Comment; 
#endif

		bool									m_EnterOnce;
		std::vector<Node>			Childs;

		Node() : m_EnterOnce(false)
		{}

		void Parse			(XmlNodeRef &xmlNode, Node &rootNode);
		bool Evaluate		(BehaviourTree &tree, uint32 &newBehaviour, Node* &node);

		const Node*			GetSubtree(const string &name) const;
		const Node*			GetSubtree(uint32 nameHash) const;

		const std::vector<Command>& GetCommands() const { return m_Commands; }
	};


	class TreeProfile
	{
		ConditionContainer			m_Conditions;
		string									m_DefaultState;
		string									m_ProfileName;

		Node										m_RootNode;
		Node										m_RootBlocks;

	public:

		TreeProfile();
		TreeProfile(XmlNodeRef &xmlNode);

		const ConditionContainer& GetConditions() const		{ return m_Conditions; }
		const Node&								GetRootNode() const			{ return m_RootNode;}
		const string&							GetDefaultState() const	{ return m_DefaultState;}
		const string&							GetProfileName() const	{ return m_ProfileName;}
	};
	

	class BehaviourTree
	{
		
		stl::hash_map<uint32, Condition>		m_Conditions;
		Node																m_RootNode;

		string															m_DefaultState;
		uint32															m_CurrentNode;

		CommandQueue												m_CommandQueue;

		

	public:

		BehaviourTree(const TreeProfile& profile)
		{
			m_Conditions = profile.GetConditions();
			m_RootNode		 = profile.GetRootNode();

			m_DefaultState = profile.GetDefaultState();
		}

		uint32			GetCurrentHash			() const { return m_CurrentNode; }

		bool				CheckCondition			(uint32 nameHash) const;
		void				SetCondition				(uint32 nameHash, bool value);
		void				SetState						(IAIObject &object, const string & state);
		void				SetCommands					(uint32 nameHash, const Node *node);
		void				Init								(IAIObject &object);

		const Node* GetSubtree					(const string &name) const { return m_RootNode.GetSubtree(name); }
		void				executeCommands			(IAIObject &object, float fTimeDelta);
		void				Update							(IAIObject &object, float fTimeDelta);
	};
};


#endif