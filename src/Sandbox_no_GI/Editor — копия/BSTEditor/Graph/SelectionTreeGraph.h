////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:   SelectionTreeGraph.h
//  Version:     v1.00
//  Created:     17/12/2010 by Paul Reindell
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SELECTION_TREE_GRAPH__H__
#define __SELECTION_TREE_GRAPH__H__

#include "HyperGraph/HyperGraph.h"
#include "Nodes/SelectionTree_BaseNode.h"
#include "Util/IXmlHistoryManager.h"
#include <ISelectionTreeManager.h>

class CSelectionTreeGraphView;
class CSelectionTreeGraphManager;
class CSelectionTreeHyperGraphView;
class CLeafMappingManager;
class CSelectionTree_TreeNode;

class CSelectionTreeGraph
	: public CHyperGraph
	, public IXmlUndoEventHandler
{
public:
	CSelectionTreeGraph( CSelectionTreeGraphManager* pSelectionTreeGraphManager );
	virtual ~CSelectionTreeGraph();

	// CHyperGraph
	virtual CHyperEdge* CreateEdge();
	virtual void RegisterEdge( CHyperEdge* pEdge, bool fromSpecialDrag );

	virtual bool CanConnectPorts( CHyperNode* pSrcNode, CHyperNodePort* pSrcPort, CHyperNode* pTrgNode, CHyperNodePort* pTrgPort );

	virtual void RemoveEdge( CHyperEdge* pEdge );
	virtual void RemoveEdgeSilent( CHyperEdge* pEdge );
	virtual void RecordUndo() {};

	// IXmlUndoEventHandler
	virtual bool SaveToXml( XmlNodeRef& xmlNode );
	virtual bool LoadFromXml( const XmlNodeRef& xmlNode );
	virtual bool ReloadFromXml( const XmlNodeRef& xmlNode );

	// CSelectionTreeGraph
	void ClearGraph();

	bool IsCreatingGraph() const { return m_iCreateGraphCount > 0; }
	bool IsInHierarchy( const CHyperNode* pNodeToSearchFor, CHyperNode* pNode, TBSTNodeList* pNodesInbetween = NULL ) const;

	CSelectionTree_BaseNode* CreateNewNode( const XmlNodeRef& xmlNode, bool bAllowCondition = true );
	CSelectionTree_BaseNode* CreateNewNode( CSelectionTree_BaseNode::NodeType );
	void ConnectNodes( CSelectionTree_BaseNode* pNodeFrom, CSelectionTree_BaseNode* pNodeTo );
	void RemoveNode( IHyperNode *pNode );

	void DisconnectNodes( CSelectionTree_BaseNode* pNodeFrom, CSelectionTree_BaseNode* pNodeTo, bool bForce = false );
	void DisconnectAllChilds( CSelectionTree_BaseNode* pNodeFrom );

	void InvalidateNodes();
	void OrderRootNodes();

	bool HideRefSubTree( CSelectionTree_BaseNode* pRefNode );
	bool ShowRefSubTree( CSelectionTree_BaseNode* pRefNode );

	void OnUndoEvent( const char* desc, bool bRecordOnDrop = false );
	void OnMouseUp();
	void OnMouseDown();

	void SetGraphView( CSelectionTreeHyperGraphView* pGraphView ) { m_pGraphView = pGraphView; }
	CSelectionTreeHyperGraphView* GetGraphView() const { return m_pGraphView; }
	void SetCreateGraph( bool bCreate );

	CLeafMappingManager* GetLeafTranslationMan() const { return m_pLeafTranslationMan; }
	bool CheckNodeMapping(CSelectionTree_TreeNode* pNode, string& translatedBehavior, int& translationId ) const;
	void UpdateTranslations();

	bool NodeClicked(CSelectionTree_BaseNode* pNode);
	void StartMappingEditMode(CSelectionTree_TreeNode* pNode);
	void StopMappingEditMode(bool abord = false);
	bool IsInTranslationEditMode() const;

	void GetRootNodes( TBSTNodeList& rootNodeList );

	bool IsClearGraph() const { return m_bClearGraph; }

	bool IsReorderEnabled() const;
	bool IsReorderSmooth() const;
	bool IsSelectionLocked() const { return m_bLockSelected; }

	// debugging
	bool InitDebugTree(const ISelectionTreeObserver::STreeNodeInfo& nodeInfo);
	void StartEval();
	void EvalNode(uint16 nodeId);
	void EvalNodeCondition(uint16 nodeId, bool condition);
	void EvalStateCondition(uint16 nodeId, bool condition);
	void StopEval(uint16 nodeId);
 	void UnmarkNodes();

protected:
	// CHyperGraph
	virtual CHyperEdge* MakeEdge( CHyperNode* pNodeOut, CHyperNodePort* pPortOut, CHyperNode* pNodeIn, CHyperNodePort* pPortIn, bool bEnabled, bool fromSpecialDrag );
	virtual void ClearAll();

private:
	CSelectionTree_BaseNode* CreateRootNode( const XmlNodeRef& xmlNode );
	void CreateHelperNodes();
	void DeleteHelperNodes();
	bool RemoveHelperNodesFromRef(CSelectionTree_BaseNode* pNode);

	CSelectionTree_BaseNode* GetRootNode();

	void RemoveEdgeInt( CHyperEdge* pEdge, bool bForce = false );

	bool ReadNodeIds(CSelectionTree_BaseNode* pNode, const ISelectionTreeObserver::STreeNodeInfo& info);

private:
	CSelectionTreeHyperGraphView* m_pGraphView;
	CLeafMappingManager* m_pLeafTranslationMan;
	int m_iCreateGraphCount;
	bool m_bRecordUndoOnDrop;
	string m_sUndoOnDropDesc;
	bool m_bClearGraph;
	bool m_bForceOrder;
	bool m_bLockSelected;

	typedef std::map<uint16, CSelectionTree_BaseNode*> TNodeIds;
	TNodeIds m_NodeIds;
};

#endif