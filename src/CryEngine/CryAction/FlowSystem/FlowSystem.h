#ifndef __FLOWSYSTEM_H__
#define __FLOWSYSTEM_H__

#pragma once

#include "IFlowSystem.h"
#include <queue>
#include "FlowSystemCVars.h"

class CFlowGraphBase;

class CFlowSystem : public IFlowSystem
{
public:
	CFlowSystem();
	~CFlowSystem();
	
	// IFlowSystem
	virtual void Release();
	VIRTUAL void Update();
	VIRTUAL void Reset();
	VIRTUAL IFlowGraphPtr CreateFlowGraph();
	VIRTUAL TFlowNodeTypeId RegisterType( const char *type, IFlowNodeFactoryPtr factory );
	VIRTUAL const char* GetTypeName( TFlowNodeTypeId typeId );
	VIRTUAL TFlowNodeTypeId GetTypeId( const char* typeName );
	VIRTUAL IFlowNodeTypeIteratorPtr CreateNodeTypeIterator();
	VIRTUAL void RegisterInspector(IFlowGraphInspectorPtr pInspector, IFlowGraphPtr pGraph = 0);
	VIRTUAL void UnregisterInspector(IFlowGraphInspectorPtr pInspector, IFlowGraphPtr pGraph = 0);
	VIRTUAL void EnableInspecting(bool bEnable) { m_bInspectingEnabled = bEnable; }
	VIRTUAL bool IsInspectingEnabled() const { return m_bInspectingEnabled; }
	VIRTUAL IFlowGraphInspectorPtr GetDefaultInspector() const { return m_pDefaultInspector; }
	VIRTUAL void GetMemoryStatistics(ICrySizer * s);
	// ~IFlowSystem

	IFlowNodePtr CreateNodeOfType( IFlowNode::SActivationInfo *, TFlowNodeTypeId typeId );

	void RegisterGraph( CFlowGraphBase *pGraph );
	void UnregisterGraph( CFlowGraphBase *pGraph );

	// resembles IFlowGraphInspector currently
	void NotifyFlow(CFlowGraphBase *pGraph, const SFlowAddress from, const SFlowAddress to);
	void NotifyProcessEvent(CFlowGraphBase *pGraph, IFlowNode::EFlowEvent event, IFlowNode::SActivationInfo *pActInfo, IFlowNode *pImpl);

	struct STypeInfo
	{
		STypeInfo(const string& typeName, IFlowNodeFactoryPtr pFactory) : name(typeName), factory(pFactory) {}
		string name;
		IFlowNodeFactoryPtr factory;
	};

	const STypeInfo& GetTypeInfo(TFlowNodeTypeId typeId) const;

private:
	typedef std::queue<XmlNodeRef> TPendingComposites;

	void LoadExtensions( string path );
	void LoadExtensionFromXml( XmlNodeRef xml, TPendingComposites * pComposites );
	void LoadComposites( TPendingComposites * pComposites );

	void RegisterAutoTypes();
	void RegisterEntityTypes();

	bool BlacklistedFlownode(const char **editorName);

private:
	class CNodeTypeIterator;
	typedef std::map<string, TFlowNodeTypeId> TTypeNameToIdMap;
	TTypeNameToIdMap m_typeNameToIdMap;
	std::vector<STypeInfo> m_typeRegistryVec; // 0 is invalid
	std::vector<CFlowGraphBase*> m_graphs;
	std::vector<IFlowGraphInspectorPtr> m_systemInspectors; // only inspectors which watch all graphs

	IFlowGraphInspectorPtr m_pDefaultInspector;

	CFlowSystemCVars m_cVars;

	// Inspecting enabled/disabled
	bool m_bInspectingEnabled;
};


#endif
