////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   animsequence.cpp
//  Version:     v1.00
//  Created:     29/4/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "AnimSequence.h"

#include "EntityNode.h"
#include "CVarNode.h"
#include "ScriptVarNode.h"
#include "AnimCameraNode.h"
#include "SceneNode.h"
#include "StlUtils.h"
#include "MaterialNode.h"
#include "EventNode.h"
#include "LayerNode.h"


#include "IScriptSystem.h"

//////////////////////////////////////////////////////////////////////////
CAnimSequence::CAnimSequence( IMovieSystem *pMovieSystem )
{
	m_lastGenId = 1;
	m_pMovieSystem = pMovieSystem;
	m_flags = 0;
	m_pParentSequence=NULL;
	m_timeRange.Set( 0,10 );
	m_bPaused = false;
	m_bActive = false;
}

//////////////////////////////////////////////////////////////////////////
CAnimSequence::~CAnimSequence()
{
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::SetName( const char *name )
{
	string name0 = m_name;
	m_name = name;
	if (name0.length() > 0)
		m_pMovieSystem->OnSequenceRenamed(name0.c_str(), m_name.c_str());
}

//////////////////////////////////////////////////////////////////////////
const char* CAnimSequence::GetName()
{
	return m_name.c_str();
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::SetFlags( int flags )
{
	m_flags = flags;
}

//////////////////////////////////////////////////////////////////////////
int CAnimSequence::GetFlags() const
{
	return m_flags;
}

//////////////////////////////////////////////////////////////////////////
int CAnimSequence::GetCutSceneFlags(const bool localFlags) const
{
	int currentFlags = m_flags & (NO_HUD | NO_PLAYER | NO_PHYSICS | NO_AI | IS_16TO9 | NO_GAMESOUNDS | NO_ABORT | NO_TRIGGERS);

	if (m_pParentSequence != NULL)
	{
		if (localFlags == true)
		{
			currentFlags &= ~m_pParentSequence->GetCutSceneFlags();
		}
		else
		{
			currentFlags |= m_pParentSequence->GetCutSceneFlags();
		}
	}

	return currentFlags;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::SetParentSequence(IAnimSequence *pParentSequence)
{
	m_pParentSequence = pParentSequence;
}

//////////////////////////////////////////////////////////////////////////
const IAnimSequence* CAnimSequence::GetParentSequence() const
{
	return m_pParentSequence;
}

//////////////////////////////////////////////////////////////////////////
int CAnimSequence::GetNodeCount() const
{
	return m_nodes.size();
}

//////////////////////////////////////////////////////////////////////////
IAnimNode* CAnimSequence::GetNode( int index ) const
{
	assert( index >= 0 && index < (int)m_nodes.size() );
	return m_nodes[index];
}

//////////////////////////////////////////////////////////////////////////
bool CAnimSequence::AddNode( IAnimNode *node )
{
	assert( node != 0 );

	// Check if this node already in sequence.
	for (int i = 0; i < (int)m_nodes.size(); i++)
	{
		if (node == m_nodes[i])
		{
			// Fail to add node second time.
			return false;
		}
	}
	
	((CAnimNode*)node)->SetSequence(this);
	node->SetTimeRange( m_timeRange );
	m_nodes.push_back( node );

	if (node->GetId() >= m_lastGenId)
		m_lastGenId = node->GetId()+1;

	return true;
}

//////////////////////////////////////////////////////////////////////////
IAnimNode* CAnimSequence::CreateNodeInternal( EAnimNodeType nodeType,uint32 nNodeId )
{
	CAnimNode *node = NULL;

	switch (nodeType)
	{
	case ANODE_ENTITY:
		node = new CAnimEntityNode();
		break;
	case ANODE_CAMERA:
		node = new CAnimCameraNode();
		break;
	case ANODE_CVAR:
		node = new CAnimCVarNode();
		break;
	case ANODE_SCRIPTVAR:
		node = new CAnimScriptVarNode();
		break;
	case ANODE_SCENE:
		node = new CAnimSceneNode();
		break;
	case ANODE_MATERIAL:
		node = new CAnimMaterialNode();
		break;
	case ANODE_EVENT:
		node = new CAnimEventNode();
		break;
	case ANODE_GROUP:
		node = new CAnimNodeGroup();
		break;
	case  ANODE_LAYER:
		node = new CLayerNode();
	}
	if (node)
	{
		node->SetSequence(this);
		node->SetTimeRange( m_timeRange );

		if (nNodeId == -1)
		{
			m_lastGenId++;
			nNodeId = m_lastGenId;
		}
		node->SetId(nNodeId);

		m_nodes.push_back( node );
	}
	return node;
}

//////////////////////////////////////////////////////////////////////////
IAnimNode* CAnimSequence::CreateNode( EAnimNodeType nodeType )
{
	return CreateNodeInternal(nodeType);
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::RemoveNode( IAnimNode *node )
{
	assert( node != 0 );
	for (int i = 0; i < (int)m_nodes.size();)
	{
		if (node == m_nodes[i])
		{
			m_nodes.erase( m_nodes.begin()+i );
			continue;
		}
		if (m_nodes[i]->GetParent() == node)
			m_nodes[i]->SetParent(0);
		i++;
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::RemoveAll()
{
	m_nodes.clear();
	m_events.resize(0);
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::Reset( bool bSeekToStart )
{
	if (!bSeekToStart)
	{
		for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
		{
			(*it)->Reset();
		}
		return;
	}

	bool bWasActive = m_bActive;

	if (!bWasActive)
		Activate();
	
	SAnimContext ec;
	ec.bSingleFrame = true;
	ec.bResetting = true;
	ec.sequence = this;
	ec.time = m_timeRange.start;
	Animate( ec );
	
	if (!bWasActive)
		Deactivate();
	else
	{
		for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
		{
			IAnimNode *anode = *it;
			anode->Reset();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::Pause()
{
	if (m_bPaused)
		return;
	m_bPaused = true;
	// Detach animation block from all nodes in this sequence.
	for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
	{
		IAnimNode *anode = *it;
		anode->Pause();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::Resume()
{
	if (!m_bPaused)
		return;
	m_bPaused = false;
	// Detach animation block from all nodes in this sequence.
	for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
	{
		IAnimNode *anode = *it;
		anode->Resume();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::StillUpdate()
{
	for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
	{
		IAnimNode *anode = *it;
		anode->StillUpdate();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::Animate( SAnimContext &ec )
{
	if(CMovieSystem::m_mov_DebugEvents)
		gEnv->pScriptSystem->SetGlobalValue("CurrentCinematicName",m_name.c_str());
	ec.sequence = this;
	// Evaluate all animation nodes in sequence.
	for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
	{
		// Make sure correct animation block is binded to node.
		IAnimNode *anode = *it;
		// Animate node.
		anode->Animate( ec );
	}
	if(CMovieSystem::m_mov_DebugEvents)
		gEnv->pScriptSystem->SetGlobalToNull("CurrentCinematicName");
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::Activate()
{
	if (m_bActive)
		return;

	m_bActive = true;
	// Assign animation block to all nodes in this sequence.
	for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
	{
		IAnimNode *anode = *it;
		((CAnimNode*)anode)->Activate(true);
	}

	// If this sequence is cut scene disable player.
	if (m_flags & CUT_SCENE)
	{
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::Deactivate()
{
	if (!m_bActive)
		return;

	// Detach animation block from all nodes in this sequence.
	for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
	{
		IAnimNode *anode = *it;
		((CAnimNode*)anode)->Activate(false);
		anode->Reset();
	}
	// If this sequence is cut scene, enable player.
	if (m_flags & CUT_SCENE)
	{
	}
	m_bActive = false;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::Serialize( XmlNodeRef &xmlNode,bool bLoading, bool bLoadEmptyTracks )
{
	if (bLoading)
	{
		// Load.
		RemoveAll();

		Range timeRange;
		const char *name = xmlNode->getAttr( "Name" );
		xmlNode->getAttr( "Flags",m_flags );
		xmlNode->getAttr( "StartTime",timeRange.start );
		xmlNode->getAttr( "EndTime",timeRange.end );
		m_groupName = xmlNode->getAttr( "Group" );

		SetName( name );
		SetTimeRange( timeRange );
		// Loading.
		XmlNodeRef nodes = xmlNode->findChild( "Nodes" );
		if (nodes)
		{
			uint32 id;
			int type;
			for (int i = 0; i < nodes->getChildCount(); i++)
			{
				XmlNodeRef xn = nodes->getChild(i);
				xn->getAttr( "Id",id );
				
				if (!xn->getAttr( "Type",type ))
					continue;

				IAnimNode *node = CreateNodeInternal( (EAnimNodeType)type,id );
				if (!node)
					continue;
				
				node->Serialize( xn,bLoading,bLoadEmptyTracks );
			}

			// When all nodes loaded restore group hierarchy
			for (AnimNodes::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
			{
				IAnimNode *anode = *it;
				((CAnimNode*)anode)->PostLoad();
				
				// And properly adjust the 'm_lastGenId' to prevent the id clash.
				if (anode->GetId() >= m_lastGenId)
					m_lastGenId = anode->GetId()+1;
			}
		}
		XmlNodeRef events = xmlNode->findChild( "Events" );
		if (events)
		{
			string eventName;
			for (int i = 0; i < events->getChildCount(); i++)
			{
				XmlNodeRef xn = events->getChild(i);
				eventName = xn->getAttr( "Name" );
				m_events.push_back(eventName);

				// Added
				for (TTrackEventListeners::iterator j = m_listeners.begin(); j != m_listeners.end(); ++j)
					(*j)->OnTrackEvent(this, ITrackEventListener::TER_ADDED, eventName, NULL);
			}
		}
		Deactivate();
		//ComputeTimeRange();
	}
	else
	{
		xmlNode->setAttr( "Name",m_name.c_str() );
		xmlNode->setAttr( "Group",m_groupName.c_str() );
		xmlNode->setAttr( "Flags",m_flags );
		xmlNode->setAttr( "StartTime",m_timeRange.start );
		xmlNode->setAttr( "EndTime",m_timeRange.end );

		// Save.
		XmlNodeRef nodes = xmlNode->newChild( "Nodes" );
		int num = GetNodeCount();
		for (int i = 0; i < num; i++)
		{
			IAnimNode *node = GetNode(i);
			if (!node)
				continue;
			XmlNodeRef xn = nodes->newChild( "Node" );
			node->Serialize( xn,bLoading,true );
		}

		XmlNodeRef events = xmlNode->newChild( "Events" );
		TrackEvents::iterator event = m_events.begin();
		TrackEvents::iterator eventEnd = m_events.end();
		for (; event != eventEnd; ++event)
		{
			XmlNodeRef xn = events->newChild( "Event" );
			xn->setAttr( "Name", *event );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::SetTimeRange( Range timeRange )
{
	m_timeRange = timeRange;
	// Set this time range for every track in animation.
		// Set time range to be in range of largest animation track.
	for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
	{
		IAnimNode *anode = *it;
		anode->SetTimeRange( timeRange );
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::ScaleTimeRange( const Range &timeRange )
{
	// Calculate scale ratio.
	float scale = timeRange.Length() / m_timeRange.Length();
	m_timeRange = timeRange;

	// Set time range to be in range of largest animation track.
	for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
	{
		IAnimNode *anode = *it;

		int trackCount = anode->GetTrackCount();
		for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
		{
			IAnimTrack *pTrack = anode->GetTrackByIndex(paramIndex);
			int nkey = pTrack->GetNumKeys();
			for (int k = 0; k < nkey; k++)
			{
				float keytime = pTrack->GetKeyTime(k);
				keytime = keytime*scale;
				pTrack->SetKeyTime(k,keytime);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::ComputeTimeRange()
{
	Range timeRange;
	//timeRange.start = FLT_MAX;
	//timeRange.end = FLT_MIN;

	timeRange = m_timeRange;
	
	// Set time range to be in range of largest animation track.
	for (AnimNodes::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
	{
		IAnimNode *anode = *it;
		
		int trackCount = anode->GetTrackCount();
		for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
		{
			IAnimTrack *pTrack = anode->GetTrackByIndex(paramIndex);
			int nkey = pTrack->GetNumKeys();
			if (nkey > 0)
			{
				timeRange.start = std::min( timeRange.start,pTrack->GetKeyTime(0) );
				timeRange.end = std::max( timeRange.end,pTrack->GetKeyTime(nkey-1) );
			}
		}
	}

	if (timeRange.start > 0)
		timeRange.start = 0;

	m_timeRange = timeRange;
}

bool CAnimSequence::AddTrackEvent(const char* szEvent)
{
	CRY_ASSERT(szEvent && szEvent[0]);
	return stl::push_back_unique(m_events, szEvent);
}

bool CAnimSequence::RemoveTrackEvent(const char* szEvent)
{
	CRY_ASSERT(szEvent && szEvent[0]);
	return stl::find_and_erase(m_events, szEvent);
}

void CAnimSequence::ClearTrackEvents()
{
	m_events.clear();
}

int CAnimSequence::GetTrackEventsCount() const
{
	return (int)m_events.size();
}

char const* CAnimSequence::GetTrackEvent(int iIndex) const
{
	char const* szResult = NULL;
	const bool bValid = (iIndex >= 0 && iIndex < GetTrackEventsCount());
	CRY_ASSERT(bValid);

	if (bValid)
		szResult = m_events[iIndex];

	return szResult;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::TriggerTrackEvent(const char* event, const char* param)
{
	// Notify listeners
	for (TTrackEventListeners::iterator j = m_listeners.begin(); j != m_listeners.end(); ++j)
		(*j)->OnTrackEvent(this, ITrackEventListener::TER_TRIGGERED, event, (void*)param);
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::AddTrackEventListener(ITrackEventListener *pListener)
{
	if (std::find(m_listeners.begin(),m_listeners.end(),pListener) == m_listeners.end())
	{
		m_listeners.push_back(pListener);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::RemoveTrackEventListener(ITrackEventListener *pListener)
{
	TTrackEventListeners::iterator it = std::find(m_listeners.begin(),m_listeners.end(),pListener);
	if (it != m_listeners.end())
	{
		m_listeners.erase(it);
	}
}

//////////////////////////////////////////////////////////////////////////
IAnimNode* CAnimSequence::FindNodeById( int nNodeId )
{
	for (AnimNodes::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
	{
		IAnimNode *anode = *it;
		if (((CAnimNode*)anode)->GetIDFast() == nNodeId)
		{
			return anode;
		}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
IAnimNode* CAnimSequence::FindNodeByName( const char *sNodeName )
{
	for (AnimNodes::const_iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
	{
		IAnimNode *anode = *it;
		// Case insesetive name comparasion.
		if (stricmp(((CAnimNode*)anode)->GetNameFast(),sNodeName) == 0)
		{
			return anode;
		}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
void CAnimSequence::ReorderNode( IAnimNode *node,IAnimNode *pAfterNode )
{
	if (node == pAfterNode || !node)
		return;

	_smart_ptr<IAnimNode> pTempHolder = node; // Keep reference to node so it is not deleted by erasing from list.
	stl::find_and_erase( m_nodes,node );

	AnimNodes::iterator it;
	for (it = m_nodes.begin(); it != m_nodes.end(); ++it)
	{
		IAnimNode *anode = *it;
		if (anode == pAfterNode)
		{
			m_nodes.insert( it+1,node );
			break;
		}
	}
	if (it == m_nodes.end())
	{
		m_nodes.insert( m_nodes.begin(),node );
	}
}

void CAnimSequence::CopyNodes(XmlNodeRef& xmlNode, IAnimNode** pSelectedNodes, uint32 count)
{
	for (int i = 0; i < count; ++i)
	{
		IAnimNode *pAnimNode = pSelectedNodes[i];
		if (pAnimNode)
		{
			XmlNodeRef xn = xmlNode->newChild( "Node" );
			pAnimNode->Serialize( xn,false,true );
		}
	}
}

void CAnimSequence::PasteNodes(const XmlNodeRef& xmlNode)
{
	int type;
	for (int i = 0; i < xmlNode->getChildCount(); i++)
	{
		XmlNodeRef xn = xmlNode->getChild(i);

		if (!xn->getAttr( "Type",type ))
			continue;

		IAnimNode *node = CreateNode((EAnimNodeType)type);
		if (!node)
			continue;

		node->Serialize( xn,true,true );
	}
}

#include UNIQUE_VIRTUAL_WRAPPER(IAnimSequence)
