#include "StdAfx.h"
#include "NewBehaviourTree/Tree.h"

#include "IXml.h"
#include "StringUtils.h"

#include "PipeUser.h"


#pragma optimize("", off)
#pragma inline_depth(0)

//#include "../CryAction/AIHandler.h"

namespace NewBehaviourTree
{
	void NewBehaviourTree::Condition::Parse(XmlNodeRef &xmlNode)
	{
		uint32 attribCount	= xmlNode->getNumAttributes();
		const char *key;
		const char *value;

		for (uint32 loop = 0; attribCount > loop; ++loop)
		{
			if (xmlNode->getAttributeByIndex(loop, &key, &value))
			{
				if (!stricmp(key, "default"))
				{
					defaultValue = !stricmp(value, "true");
				}
				else if (!stricmp(key, "name"))
				{
					NamehHash = CryStringUtils::CalculateHashLowerCase(value);
				}
			}
		}
	}

	void Command::Parse(XmlNodeRef &xmlNode, Node &rootNode)
	{
		const char *key;
		const char *value;

		uint32 commandparams	= xmlNode->getNumAttributes();

		for (uint32 loop = 0; loop < commandparams; ++loop)
		{
			if (xmlNode->getAttributeByIndex(loop, &key, &value))
			{
				if (!stricmp(key, "SetBehaviour"))
				{
					m_Command = value;
					m_CommandType = Command::CT_SETBEHAVIOUR;
				}
				else if (!stricmp(key, "SetGoalPipe"))
				{
					m_Command = value;
					m_CommandType = Command::CT_SETGOALPIPE;
				}
				else if (!stricmp(key, "SetTransition"))
				{
					m_Command = value;
					m_CommandType = CT_TRANSITION;
				}
				else if (!stricmp(key, "SetValue"))
				{
					m_Command = value;
					m_CommandType = CT_SETVALUE;
				}
				else if (!stricmp(key, "UnsetValue"))
				{
					m_Command = value;
					m_CommandType = CT_UNSETVALUE;
				}
				else if (!stricmp(key, "Condition"))
				{
					Conditions.push_back(CryStringUtils::CalculateHashLowerCase(value));
				}
				else if (!stricmp(key, "timeout"))
				{
					m_fTimeout = static_cast<float>(atof(value));
				}
				else if (!stricmp(key, "nonBlocking"))
				{
					m_NonBlocking = !stricmp(value, "true");
				}
			}
		}
	}

	Command::CommandState Command::Execute(BehaviourTree &tree, IAIObject &object, float fTimeDelta)
	{
		switch(m_State)
		{
		case CS_STOPPED:
		
			if (!Conditions.empty())
			{
				for (std::vector<uint32>::iterator it = Conditions.begin(); Conditions.end() != it; ++it)
				{
					if (!tree.CheckCondition(*it))
					{
						return m_State;
					}
				}
			}

			switch(m_CommandType)
			{
			case CT_SETBEHAVIOUR:
				tree.SetState(object, m_Command);
				break;
			case CT_SETGOALPIPE:
				if (object.CastToCPipeUser())
				{
					object.CastToCPipeUser()->SelectPipe(0, m_Command.c_str(), GetWeakRefSafe(0));
				}
				break;
			case CT_TRANSITION:
				if (const Node* newnode = tree.GetSubtree(m_Command))
				{
					tree.SetCommands(newnode->m_NameHash, newnode);
					return CS_COMMANDSRESET;
				}
				break;
			case CT_SETVALUE:
				tree.SetCondition(CryStringUtils::CalculateHashLowerCase(m_Command.c_str()), true);
				return CS_STOPPED;
				break;
			case CT_UNSETVALUE:
				tree.SetCondition(CryStringUtils::CalculateHashLowerCase(m_Command.c_str()), false);
				return CS_STOPPED;
				break;
			}


			if (m_fTimeout > 0.0f)
			{
				m_State = CS_RUNNING;
			}
			break;
		case CS_RUNNING:
			m_fTimeout -= fTimeDelta;

			if (m_fTimeout < 0.0f)
			{
				m_State = CS_STOPPED;
			}
			break;
		}

		return m_State;
	}


	void Node::Parse(XmlNodeRef &xmlNode, Node &rootNode)
	{
		uint32 attribCount	= xmlNode->getNumAttributes();
		const char *key;
		const char *value;

		for (uint32 loop = 0; attribCount > loop; ++loop)
		{
			if (xmlNode->getAttributeByIndex(loop, &key, &value))
			{
				if (!stricmp(key, "condition"))
				{
					m_Conditions.push_back(CryStringUtils::CalculateHashLowerCase(value));
				}
				else if (!stricmp(key, "name"))
				{
					m_NameHash = CryStringUtils::CalculateHashLowerCase(value);
#if defined(_DEBUG)
					m_NameReadable = value;
#endif
				}
				else if (!stricmp(key, "onlyIfNotEntered"))
				{
					m_EnterOnce = !stricmp(value, "true");
				}
#if defined(_DEBUG)
				else if (!stricmp(key, "comment"))
				{
					m_Comment = value;
				}
#endif
			}
		}

		for( int iRow = 0; iRow < xmlNode->getChildCount(); ++iRow )
		{
			XmlNodeRef child		= xmlNode->getChild( iRow );
			const char *tag			=	child->getTag();

			if (!stricmp(tag, "Node"))
			{
				Childs.push_back(Node());
				Childs.back().Parse(child, rootNode);
			}
			else if (!stricmp(tag, "Reference"))
			{
				if (child->getAttributeByIndex(0, &key, &value))
				{
					if (const Node* temp = rootNode.GetSubtree(value))
					{
						Childs.push_back(*temp);
					}
				}
			}
			else if (!stricmp(tag, "Command"))
			{
				Command temp;
				temp.Parse(child, rootNode);
				m_Commands.insert(m_Commands.begin(), temp);
			}
		}
	}

	bool	Node::Evaluate(BehaviourTree &tree, uint32 &newBehaviour, Node* &node)
	{
		bool eval = true;

		for (std::vector<uint32>::iterator it = m_Conditions.begin(); m_Conditions.end() != it; ++it)
		{
			eval &= tree.CheckCondition(*it);
		}

		if (eval)
		{
			if (Childs.empty())
			{
				newBehaviour = m_NameHash;
				node = this;
			}
			else
			{
				eval = false;
				for (std::vector<Node>::iterator it = Childs.begin(); !eval && (Childs.end() != it); ++it)
				{
					eval = it->Evaluate(tree, newBehaviour, node);
				}

				if (m_EnterOnce && GetSubtree(tree.GetCurrentHash()))
				{
					node = 0;
					newBehaviour = tree.GetCurrentHash();
				}
			}
		}

		return eval;
	}

	const Node* Node::GetSubtree(const string &name) const
	{
		const Node* ret = 0;

		uint32 hashValue = CryStringUtils::CalculateHashLowerCase(name.c_str());

		if (hashValue == m_NameHash)
		{
			ret = this; 
		}
		else if (!Childs.empty())
		{
			for (std::vector<Node>::const_iterator it = Childs.begin(); !ret && (Childs.end() != it); ++it)
			{
				ret = it->GetSubtree(hashValue);
			}
		}

		return ret;
	}


	const Node* Node::GetSubtree(uint32 nameHash) const
	{
		const Node* ret = 0;

		if (nameHash == m_NameHash)
		{
			ret = this; 
		}
		else if (!Childs.empty())
		{
			for (std::vector<Node>::const_iterator it = Childs.begin(); !ret && (Childs.end() != it); ++it)
			{
				ret = it->GetSubtree(nameHash);
			}
		}

		return ret;
	}

	TreeProfile::TreeProfile()
	{
		m_ProfileName = "InvalidProfile";
	}


	TreeProfile::TreeProfile(XmlNodeRef &xmlNode)
	{
		const char *key;
		const char *value;
		const char *tag			=	xmlNode->getTag();

		if (!stricmp(tag, "TreeProfile"))
		{
			if (xmlNode->getAttributeByIndex(0, &key, &value))
			{
				if (!stricmp("name", key))
				{
					m_ProfileName = value;	
				}
			}

			for (int iProfileSection = 0; iProfileSection < xmlNode->getChildCount(); ++iProfileSection)
			{
				XmlNodeRef child		= xmlNode->getChild( iProfileSection );
				tag									=	child->getTag();

				if (!stricmp(tag, "Tree"))
				{
					//Todo: get name for profile
					m_RootNode.m_NameHash = CryStringUtils::CalculateHashLowerCase("BehaviourTree");

					for( int iRow = 0; iRow < child->getChildCount(); ++iRow )
					{
						XmlNodeRef state		= child->getChild( iRow );
						tag									=	state->getTag();

						if (!stricmp(tag, "Node"))
						{
							m_RootNode.Childs.push_back(Node());
							m_RootNode.Childs.back().Parse(state, m_RootNode);
						}
						if (!stricmp(tag, "Block"))
						{
							m_RootBlocks.Childs.push_back(Node());
							m_RootBlocks.Childs.back().Parse(state, m_RootNode);
						}
						else if (!stricmp(tag, "default"))
						{
							state->getAttributeByIndex(0, &key, &value);
							m_DefaultState = value;
						}
					}
				}
				else if (!stricmp(tag, "Conditions"))
				{
					for( int iRow = 0; iRow < child->getChildCount(); ++iRow )
					{
						XmlNodeRef condition		= child->getChild( iRow );
						tag											=	condition->getTag();

						if (!stricmp(tag, "Condition"))
						{
							Condition temp;
							temp.Parse(condition);

							m_Conditions[temp.NamehHash] = temp;
						}
					}
				}
			}
		}
		else if (!stricmp(tag, "Conditions"))
		{
			for (int iConditionNumber = 0; iConditionNumber < xmlNode->getChildCount(); ++iConditionNumber)
			{
				XmlNodeRef child		= xmlNode->getChild( iConditionNumber );
				tag									=	child->getTag();


				if (!stricmp(tag, "Condition"))
				{
					Condition temp;
					temp.Parse(child);
					m_Conditions[temp.NamehHash] = temp;
				}
			}
		}
	}


	void BehaviourTree::SetCommands(uint32 nameHash, const Node *node)
	{
		if (nameHash != m_CurrentNode)
		{
			if (node)
			{
				m_CommandQueue = node->GetCommands();
				m_CurrentNode = nameHash;
			}
		}
	}


	void BehaviourTree::SetState(IAIObject &object, const string & state)
	{
		//if (state.compare(currentState))
		//{
			if (object.GetProxy())
			{
				object.GetProxy()->SetBehaviour(state.c_str());
				//currentState = state;
			}
		//}
	}

	void BehaviourTree::Init(IAIObject &object)
	{
		for (ConditionContainer::iterator it = m_Conditions.begin(); m_Conditions.end() != it; ++it)
		{
			it->second.Reset();
		}

		SetState(object, m_DefaultState);
	}

	void BehaviourTree::Update(IAIObject &object, float fTimeDelta)
	{
		FRAME_PROFILER("BehaviourTree::Update", GetISystem(), PROFILE_AI)
		uint32 newBehaviour;

		Node *node = 0;
		if (m_RootNode.Evaluate(*this, newBehaviour, node))
		{
			SetCommands(newBehaviour, node);
		}

		executeCommands(object, fTimeDelta);
	}

	void BehaviourTree::executeCommands(IAIObject &object, float fTimeDelta)
	{
		//while 
		if (!m_CommandQueue.empty())
		{
			CommandQueue::reverse_iterator it = m_CommandQueue.rbegin();

			do 
			{
				if (it->m_State != Command::CS_DONTEXECUTE)
				{
					switch (it->Execute(*this, object, fTimeDelta))
					{
					case Command::CS_STOPPED:
						it->m_State = Command::CS_DONTEXECUTE;
						break;
					case Command::CS_RUNNING:
						if (!it->IsNonBlocking())
						{
							return;
						}
						break;
					case Command::CS_COMMANDSRESET:
						return;
						break;
					}
				}

				++it;
			} while (m_CommandQueue.rend() != it);
		}
	}

	bool BehaviourTree::CheckCondition(uint32 nameHash) const
	{
		bool ret = false;

		ConditionContainer::const_iterator it = m_Conditions.find(nameHash);

		if (m_Conditions.end() != it)
		{
			ret = it->second.value;
		}

		return ret;
	}

	void BehaviourTree::SetCondition(uint32 nameHash, bool value)
	{
		ConditionContainer::iterator it = m_Conditions.find(nameHash);

		if (m_Conditions.end() != it)
		{
			it->second.value = value;
		}
	}
};
