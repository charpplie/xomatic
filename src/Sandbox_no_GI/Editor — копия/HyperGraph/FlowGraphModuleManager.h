/********************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2011.
-------------------------------------------------------------------------
File name:   FlowGraphModuleManager.h
$Id$
Description: 

-------------------------------------------------------------------------
History:
- 08/15/11   : Sascha Hoba

*********************************************************************/

#ifndef __FLOWGRAPHMODULEMANAGER_H__
#define __FLOWGRAPHMODULEMANAGER_H__

#include "LevelIndependentFileMan.h"
#include "HyperGraph/HyperGraph.h"
#include "../HyperGraph/FlowGraph.h"
#include <IFlowGraphModuleManager.h>

class CEditorFlowGraphModuleManager : public ILevelIndependentFileModule, public IFlowGraphModuleListener
{
public:
	CEditorFlowGraphModuleManager();
	virtual ~CEditorFlowGraphModuleManager();

	//ILevelIndependentFileModule
	virtual bool PromptChanges();
	//~ILevelIndependentFileModule

	//IFlowGraphModuleListener
	virtual void OnModuleInstanceCreated(IFlowGraphModule* module, TModuleInstanceId instanceID){}
	virtual void OnModuleInstanceDestroyed(IFlowGraphModule* module, TModuleInstanceId instanceID){}
	virtual void OnPostModuleDestroyed(){}
	virtual void OnModuleDestroyed(IFlowGraphModule* module);
	virtual void OnRootGraphChanged(IFlowGraphModule* module);
	virtual void OnScannedForModules();
	//~IFlowGraphModuleListener

	void Init();

	bool NewModule( CString& filename, bool bGlobal = true, CHyperGraph** pHyperGraph = NULL );

	IFlowGraphPtr GetModuleFlowGraph(int index) const;
	IFlowGraphPtr GetModuleFlowGraph(const char* name) const;
	void DeleteModuleFlowGraph(CFlowGraph* pGraph);

	void CreateModuleNodes(const char* moduleName);

	void SaveModuleGraph(CFlowGraph* pFG);

private:

	bool HasModifications();
	void SaveChangedGraphs();
	void ReloadHypergraphDialog();
	void CreateEditorFlowgraphs();

	typedef std::vector<CFlowGraph*> TFlowGraphs;
	TFlowGraphs m_FlowGraphs;
};

#endif
