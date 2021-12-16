////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File name:   FlowGraphDebugger.h
//  Version:     v1.00
//  Created:     13/09/2011 by Sascha Hoba.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __FLOWGRAPHDEBUGGEREDITOR_H__
#define __FLOWGRAPHDEBUGGEREDITOR_H__
#pragma once

#include "IHyperGraph.h" //IHyperGraphManagerListener
#include <IFlowGraphDebugger.h> //IFlowGraphDebugListener


class CFlowGraphDebuggerEditor : public IFlowGraphDebugListener, IHyperGraphManagerListener, IEditorNotifyListener
{
public:
	CFlowGraphDebuggerEditor();
	virtual ~CFlowGraphDebuggerEditor();

	bool Init();
	bool Shutdown();

	bool AddBreakpoint(CFlowNode* pFlowNode, CHyperNodePort* pHyperNodePort);
	bool RemoveBreakpoint(CFlowNode* pFlowNode, CHyperNodePort* pHyperNodePort);
	bool RemoveAllBreakpointsForNode(CFlowNode* pFlowNode);
	bool RemoveAllBreakpointsForGraph(IFlowGraphPtr pFlowGraph);
	bool HasBreakpoint(CFlowNode* pFlowNode, const CHyperNodePort* pHyperNodePort) const;
	bool HasBreakpoint(IFlowGraphPtr pFlowGraph, TFlowNodeId nodeID) const;
	bool HasBreakpoint( IFlowGraphPtr pFlowGraph ) const;
	bool EnableBreakpoint( CFlowNode* pFlowNode, CHyperNodePort* pHyperNodePort, bool enable );
	bool EnableTracepoint( CFlowNode* pFlowNode, CHyperNodePort* pHyperNodePort, bool enable );
	void PauseGame();
	void ResumeGame();

	// IHyperGraphManagerListener
	virtual void OnHyperGraphManagerEvent( EHyperGraphEvent event, IHyperGraph* pGraph, IHyperNode* pNode );
	// ~IHyperGraphManagerListener

	// IEditorNotifyListener
	virtual void OnEditorNotifyEvent( EEditorNotifyEvent event );
	// ~IEditorNotifyListener

	// IFlowgraphDebugListener
	virtual void OnBreakpointAdded(const SBreakPoint& breakpoint){}
	virtual void OnBreakpointRemoved(const SBreakPoint& breakpoint){}
	virtual void OnAllBreakpointsRemovedForNode(IFlowGraphPtr pFlowGraph, TFlowNodeId nodeID){}
	virtual void OnAllBreakpointsRemovedForGraph(IFlowGraphPtr pFlowGraph){}
	virtual void OnBreakpointHit(const SBreakPoint& breakpoint);
	virtual void OnTracepointHit(const STracePoint& tracepoint);
	virtual void OnBreakpointInvalidated(const SBreakPoint& breakpoint);
	virtual void OnEnableBreakpoint(const SBreakPoint& breakpoint, bool enable){}
	virtual void OnEnableTracepoint(const STracePoint& tracepoint, bool enable){}
	// ~IFlowgraphDebugListener

private:
	void CenterViewAroundNode( CFlowNode* pNode ) const;
	
	bool m_InGame;
	bool m_CursorVisible;
	IFlowGraphDebuggerPtr m_pFlowGraphDebugger;
};

#endif
