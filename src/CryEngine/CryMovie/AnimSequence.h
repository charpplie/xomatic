////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   animsequence.h
//  Version:     v1.00
//  Created:     26/4/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: Implementation of IAnimSequence interface.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __animsequence_h__
#define __animsequence_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include "IMovieSystem.h"

class CAnimSequence : public IAnimSequence
{
public:
	CAnimSequence( IMovieSystem *pMovieSystem );
	~CAnimSequence();

	//////////////////////////////////////////////////////////////////////////
	virtual void Release() { if (--m_nRefCounter <= 0) delete this; }
	//////////////////////////////////////////////////////////////////////////

	// Movie system.
	IMovieSystem* GetMovieSystem() const { return m_pMovieSystem; };

	void SetName( const char *name );
	const char* GetName();

	VIRTUAL void SetFlags( int flags );
	VIRTUAL int GetFlags() const;
	VIRTUAL int GetCutSceneFlags(const bool localFlags = false) const;

	VIRTUAL void SetParentSequence(IAnimSequence *pParentSequence);
	VIRTUAL const IAnimSequence* GetParentSequence() const;

	void SetTimeRange( Range timeRange );
	Range GetTimeRange() { return m_timeRange; };

	void ScaleTimeRange( const Range &timeRange );

	//! Return number of animation nodes in sequence.
	int GetNodeCount() const;
	//! Get specified animation node.
	IAnimNode* GetNode( int index ) const;
	
	IAnimNode* FindNodeByName( const char *sNodeName );
	IAnimNode* FindNodeById( int nNodeId );
	VIRTUAL void ReorderNode( IAnimNode *node,IAnimNode *pAfterNode );

	void Reset( bool bSeekToStart );
	void Pause();
	void Resume();

	//! Add animation node to sequence.
	bool AddNode( IAnimNode *node );
	IAnimNode* CreateNode( EAnimNodeType nodeType );
	void RemoveNode( IAnimNode *node );
	//! Add scene node to sequence.
	void RemoveAll();

	void Activate();
	void Deactivate();
	void StillUpdate();
	void Animate( SAnimContext &ec );

	void Serialize( XmlNodeRef &xmlNode,bool bLoading, bool bLoadEmptyTracks=true );
	void CopyNodes(XmlNodeRef& xmlNode, IAnimNode** pSelectedNodes, uint32 count);
	void PasteNodes(const XmlNodeRef& xmlNode);

	//! Add/remove track events in sequence
	VIRTUAL bool AddTrackEvent(const char* szEvent);
	VIRTUAL bool RemoveTrackEvent(const char* szEvent);
	VIRTUAL void ClearTrackEvents();

	//! Get the track events in the sequence
	VIRTUAL int GetTrackEventsCount() const;
	VIRTUAL char const* GetTrackEvent(int iIndex) const;

	//! Call to trigger a track event
	VIRTUAL void TriggerTrackEvent(const char* event, const char* param = NULL);

	//! Track event listener
	VIRTUAL void AddTrackEventListener(ITrackEventListener *pListener);
	VIRTUAL void RemoveTrackEventListener(ITrackEventListener *pListener);

	VIRTUAL void SetGroupName( const char *sGroupName ) { m_groupName = sGroupName; };
	VIRTUAL const char* GetGroupName() const { return m_groupName; };

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_name);
		pSizer->AddObject(m_groupName);
		pSizer->AddObject(m_nodes);
		pSizer->AddObject(m_listeners);
	}
private:
	void ComputeTimeRange();
	
	// Create a new animation node.
	IAnimNode* CreateNodeInternal( EAnimNodeType nodeType,uint32 nNodeId=-1 );

	typedef std::vector< _smart_ptr<IAnimNode> > AnimNodes;
	AnimNodes m_nodes;

	string m_name;
	string m_groupName;
	Range m_timeRange;
	TrackEvents m_events;

	// Listeners
	typedef std::list<ITrackEventListener*> TTrackEventListeners;
	TTrackEventListeners m_listeners;

	int m_flags;

	IAnimSequence *m_pParentSequence;

	//
	IMovieSystem *m_pMovieSystem;
	bool m_bPaused;
	bool m_bActive;

	uint32 m_lastGenId;
};

#endif // __animsequence_h__
