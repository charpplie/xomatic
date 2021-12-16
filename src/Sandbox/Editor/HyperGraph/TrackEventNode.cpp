////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   TrackEventNode.cpp
//  Version:     v1.00
//  Created:     4/4/2008 by Kevin.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TrackEventNode.h"
#include "FlowGraph.h"
#include "FlowGraphVariables.h"
#include "HyperGraph.h"

#include "..\Objects\Entity.h"

////////////////////////////////////////////////////////////////////////////
CTrackEventNode::CTrackEventNode()
{
	SetClass(GetClassType());
	m_pSequence = NULL;
}

////////////////////////////////////////////////////////////////////////////
CTrackEventNode::~CTrackEventNode()
{
	if (NULL != m_pSequence)
	{
		m_pSequence->RemoveTrackEventListener(this);
		m_pSequence->Release();
		m_pSequence = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////
void CTrackEventNode::Init()
{
	CFlowNode::Init();
	SetName("");
	m_pSequence = NULL;
}

////////////////////////////////////////////////////////////////////////////
void CTrackEventNode::Done()
{
	if (NULL != m_pSequence)
	{
		m_pSequence->RemoveTrackEventListener(this);
		m_pSequence->Release();
		m_pSequence = NULL;
	}

	CFlowNode::Done();
}

////////////////////////////////////////////////////////////////////////////
CHyperNode* CTrackEventNode::Clone()
{
	CTrackEventNode *pNode = new CTrackEventNode;
	pNode->CopyFrom(*this);

	pNode->m_entityGuid = m_entityGuid;
	pNode->m_pEntity = m_pEntity;
	pNode->m_flowSystemNodeFlags = m_flowSystemNodeFlags;
	pNode->m_szDescription = m_szDescription;
	pNode->m_szUIClassName = m_szUIClassName;

	Ports::iterator iter = pNode->m_inputs.begin();
	while (iter != pNode->m_inputs.end())
	{
		IVariable* pVar = (*iter).pVar;
		CFlowNodeGetCustomItemsBase* pGetCustomItems = static_cast<CFlowNodeGetCustomItemsBase*> (pVar->GetUserData());
		if (pGetCustomItems != 0)
		{
			pGetCustomItems->SetFlowNode(pNode);
		}
		++iter;
	}

	return pNode;
}

////////////////////////////////////////////////////////////////////////////
void CTrackEventNode::Serialize( XmlNodeRef &node,bool bLoading, CObjectArchive* ar)
{
	CFlowNode::Serialize(node, bLoading, ar);

	if (bLoading)
	{
		m_sequenceName = "";
		PopulateOutput(true);

		// Load output ports from file
		XmlNodeRef outputs = node->findChild( "Outputs" );
		if (outputs)
		{
			string eventName;
			for (int i = 0; i < outputs->getChildCount(); i++)
			{
				XmlNodeRef xn = outputs->getChild(i);
				eventName = xn->getAttr( "Name" );
				AddOutputEvent(eventName);
			}
		}
	}
	else
	{
		// Save output ports
		if (NULL != m_pSequence)
		{
			XmlNodeRef outputs = node->newChild( "Outputs" );
			const int iCount = m_pSequence->GetTrackEventsCount();
			for (int i = 0; i < iCount; ++i)
			{
				XmlNodeRef xn = outputs->newChild( "Output" );
				xn->setAttr( "Name", m_pSequence->GetTrackEvent(i) );
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////
CString CTrackEventNode::GetTitle() const
{
	CString title = TRACKEVENTNODE_TITLE;
	if (!m_inputs.empty())
	{
		CString value;
		m_inputs[0].pVar->Get(value);
		if (!value.IsEmpty() && value != "")
		{
			title += ":";
			title += value;
		}
	}
	return title;
}

////////////////////////////////////////////////////////////////////////////
const char* CTrackEventNode::GetClassName() const
{
	return TRACKEVENTNODE_CLASS;
}

////////////////////////////////////////////////////////////////////////////
const char* CTrackEventNode::GetDescription()
{
	return TRACKEVENTNODE_DESC;
}

////////////////////////////////////////////////////////////////////////////
void CTrackEventNode::OnInputsChanged()
{
	CFlowNode::OnInputsChanged();
	PopulateOutput(false);
}

////////////////////////////////////////////////////////////////////////////
void CTrackEventNode::PopulateOutput(bool bLoading)
{
	if (m_inputs.empty()) return;

	// Remove listener to current sequence
	if (NULL != m_pSequence)
	{
		m_pSequence->RemoveTrackEventListener(this);
		m_pSequence->Release();
		m_pSequence = NULL;
	}

	CString sequence;
	m_inputs[0].pVar->Get(sequence);

	// Check if sequence exists
	m_pSequence = GetIEditor()->GetMovieSystem()->FindSequence(sequence);
	if (NULL == m_pSequence)
	{
		m_inputs[0].pVar->Set("");
		sequence.Empty();
	}
	else
	{
		// Add listener
		m_pSequence->AddRef();
		m_pSequence->AddTrackEventListener(this);
	}

	// Check if sequence changed (or loading, don't bother with outputs on load)
	if (false == bLoading && m_sequenceName != sequence)
	{
		m_sequenceName = sequence;

		// Clear all outputs
		while (false == m_outputs.empty())
			RemoveOutputEvent(m_outputs.begin()->GetName());

		// Add sequence event outputs
		if (!m_sequenceName.IsEmpty() && m_sequenceName != "")
		{
			// Create output port for each track event in selected sequence
			if (IAnimSequence *pSequence = GetIEditor()->GetMovieSystem()->FindSequence(m_sequenceName))
			{
				const int iCount = m_pSequence->GetTrackEventsCount();
				for (int i = 0; i < iCount; ++i)
				{
					AddOutputEvent(m_pSequence->GetTrackEvent(i));
				}
			}
		}
	}
	else if (true == bLoading)
	{
		m_sequenceName = sequence;
	}

	// Invalidate and reset properties
	Invalidate(true);
}

////////////////////////////////////////////////////////////////////////////
void CTrackEventNode::AddOutputEvent(const char* event)
{
	CString desc;
	desc.Format("Called when %s is triggered from sequence", event);
	IVariablePtr pVar = new CVariableFlowNode<CString>;
	pVar->SetName(event);
	pVar->SetHumanName(event);
	pVar->SetDescription(desc);
	CHyperNodePort port(pVar, false);
	AddPort(port);
	Invalidate(true);
}

////////////////////////////////////////////////////////////////////////////
void CTrackEventNode::RemoveOutputEvent(const char* event)
{
	// Find it
	Ports::iterator i = m_outputs.begin();
	bool bFound = false;
	while (i != m_outputs.end())
	{
		if (strcmp(i->GetName(),event)==0)
		{
			// Remove the connected edges
			if (CHyperGraph *pGraph = (CHyperGraph*)GetGraph())
			{
				std::vector<CHyperEdge*> edges;
				std::vector<CHyperEdge*>::iterator it;
				if (pGraph->FindEdges(this, edges))
				{
					for (it = edges.begin(); it != edges.end(); ++it)
					{
						if (i->nPortIndex == (*it)->nPortOut)
							pGraph->RemoveEdgeSilent(*it);
					}
				}
			}

			// Erase from map
			i = m_outputs.erase(i);
			bFound = true;
		}
		else
		{
			if (true == bFound)
			{
				// Make a note of any edges connected to it
				if (CHyperGraph *pGraph = (CHyperGraph*)GetGraph())
				{
					std::vector<CHyperEdge*> edges;
					std::vector<CHyperEdge*>::iterator it;
					if (pGraph->FindEdges(this, edges))
					{
						for (it = edges.begin(); it != edges.end(); ++it)
						{
							if (i->nPortIndex == (*it)->nPortOut)
								(*it)->nPortOut--;
						}
					}
				}

				// Decrease its port count so it stays linear
				i->nPortIndex--;
			}
			++i;
		}
	}
	Invalidate(true);
}

////////////////////////////////////////////////////////////////////////////
void CTrackEventNode::OnTrackEvent(IAnimSequence *pSequence, int reason, const char* event, void* pUserData)
{
	switch (reason)
	{
		case ITrackEventListener::TER_ADDED:
			{
				// Check if not already added
				for (Ports::iterator i = m_outputs.begin(); i != m_outputs.end(); ++i)
				{
					if (strcmp(i->GetName(),event) == 0)
						return; // Already got one for it
				}

				// Make a new one
				AddOutputEvent(event);
			}
			break;

		case ITrackEventListener::TER_REMOVED:
			{
				// Remove it
				RemoveOutputEvent(event);
			}
			break;

		case ITrackEventListener::TER_TRIGGERED:
			{
				// Get param
				const char* param = (const char*)pUserData;

				// Find output port and call it
				for (Ports::iterator i = m_outputs.begin(); i != m_outputs.end(); ++i)
				{
					if (strcmp(i->GetName(), event) == 0)
					{
						// Get its graph
						if (CFlowGraph* pGraph = (CFlowGraph*)GetGraph())
						{
							// Construct address
							SFlowAddress address;
							address.node = GetFlowNodeId();
							address.port = i->nPortIndex;
							address.isOutput = true;

							// Call it using param
							TFlowInputData value;
							value.Set(string(param));
							pGraph->GetIFlowGraph()->ActivatePortAny(address, value);
						}

						break;
					}
				}
			}
			break;
	}
}
