////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2008.
// -------------------------------------------------------------------------
//  File name:   BlackBoxNode.h
//  Version:     v1.00
//  Created:     25/2/2008 by Jan M.
//  Compilers:   Visual Studio 2005
//  Description: Black Box composite node
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __BLACKBOXNODE_H__
#define __BLACKBOXNODE_H__

#pragma once

#include "HyperGraphNode.h"
class CFlowNode;

class CBlackBoxNode : public CHyperNode
{
public:
	static const char* GetClassType() {
		return "_blackbox";
	}
	CBlackBoxNode();
	void Init();
	void Done();
	CHyperNode * Clone();
	virtual void Serialize( XmlNodeRef &node,bool bLoading, CObjectArchive* ar);

	virtual void SetPos( Gdiplus::PointF pos );

	virtual int GetObjectAt( Gdiplus::Graphics * pGr, Gdiplus::PointF point );

	void AddNode(CHyperNode *pNode);
	void RemoveNode(CHyperNode *pNode);
	bool IncludesNode(CHyperNode *pNode);
	bool IncludesNode(HyperNodeID nodeId);
	void Clear() {m_nodes.clear();}
	std::vector<CHyperNode*>* GetNodes() { return &m_nodes; }
	std::vector<CHyperNode*>* GetNodesSafe();
	Gdiplus::PointF GetPointForPort( CHyperNodePort *port );
	CHyperNodePort * GetPortAtPoint( Gdiplus::PointF );
	CHyperNode * GetNode( CHyperNodePort *port );
	ILINE void SetPort(CHyperNodePort *port, Gdiplus::PointF pos)
	{
		m_ports[port] = pos;
	}

	void SetNormalBounds( Gdiplus::RectF rect ) { m_normalBounds = rect; }

	void Minimize()
	{
		m_bCollapsed = !m_bCollapsed;
		for(int i = 0; i < m_nodes.size(); ++i)
			m_nodes[i]->Invalidate(true);
		if(m_bCollapsed)
			m_iBrackets = -1;
		Invalidate(true);
	}
	bool IsMinimized() { return m_bCollapsed; }

	bool PortActive( const CHyperNodePort* port );

	virtual CHyperNodePort * GetPortAtPoint( Gdiplus::Graphics * pGr, Gdiplus::PointF point )
	{
		//go through sub-nodes to retrieve port .. ?
		return NULL;
	}
	virtual bool IsEditorSpecialNode() { return true; }
	virtual bool IsFlowNode() { return false; }

private:

	std::vector<CHyperNode*> m_nodes;
	std::vector<HyperNodeID> m_nodesIDs;
	std::map<CHyperNodePort *, Gdiplus::PointF> m_ports;
	bool	m_bCollapsed;
	int		m_iBrackets;
	Gdiplus::RectF m_normalBounds;
};

#endif
