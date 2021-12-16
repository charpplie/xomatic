/********************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2011.
-------------------------------------------------------------------------
File name:   FlowGraphModuleManager.cpp
$Id$
Description: 

-------------------------------------------------------------------------
History:
- 08/15/11   : Sascha Hoba

*********************************************************************/

#include "StdAfx.h"
#include "FlowGraphModuleManager.h"

#include "../HyperGraph/FlowGraphManager.h"
#include "CustomMessageBox.h"
#include "HyperGraphDialog.h"

#include <IFlowGraphDebugger.h>


static const char* FLOWGRAPH_MODULE_PATH_GLOBAL = "Libs/FlowgraphModules";
static const char* FLOWGRAPH_MODULE_PATH_LEVEL = "FlowgraphModules";
static const char* GRAPH_FILE_FILTER = "Graph XML Files (*.xml)|*.xml";

//////////////////////////////////////////////////////////////////////////
CEditorFlowGraphModuleManager::CEditorFlowGraphModuleManager()
{
}

//////////////////////////////////////////////////////////////////////////
CEditorFlowGraphModuleManager::~CEditorFlowGraphModuleManager()
{
	GetIEditor()->GetLevelIndependentFileMan()->UnregisterModule(this);

	IFlowGraphModuleManager* pModuleManager = gEnv->pFlowSystem->GetIModuleManager();
	if(pModuleManager)
	{
		pModuleManager->UnregisterListener(this);
	}
}

//////////////////////////////////////////////////////////////////////////
void CEditorFlowGraphModuleManager::Init()
{
	GetIEditor()->GetLevelIndependentFileMan()->RegisterModule(this);

	if (gEnv->pFlowSystem == NULL)
		return;

	// get modules from engine module manager, create editor-side graphs to match
	IFlowGraphModuleManager* pModuleManager = gEnv->pFlowSystem->GetIModuleManager();
	if(pModuleManager == NULL)
		return;

	pModuleManager->RegisterListener(this, "CEditorFlowGraphModuleManager");

	CreateEditorFlowgraphs();
}

//////////////////////////////////////////////////////////////////////////
void CEditorFlowGraphModuleManager::SaveChangedGraphs()
{
	for (int i = 0; i < m_FlowGraphs.size(); ++i)
	{
		IFlowGraphPtr pGraph = GetModuleFlowGraph(i);
		CFlowGraph* pFG = GetIEditor()->GetFlowGraphManager()->FindGraph(pGraph);
		assert(pFG);
		if (pFG && pFG->IsModified())
		{
			SaveModuleGraph(pFG);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CEditorFlowGraphModuleManager::SaveModuleGraph(CFlowGraph* pFG)
{
	// Saves from the editor have to save the editor flowgraph (not the game side one as
	//	it is not updated with new nodes). So allow the game-side module manager to 
	//	add the ports to the xml from the editor flowgraph, then write to file.

	const char* filename = gEnv->pFlowSystem->GetIModuleManager()->GetModulePath(pFG->GetName());

	XmlNodeRef rootNode = gEnv->pSystem->CreateXmlNode("Graph");

	IFlowGraphModuleManager* pModuleManager = gEnv->pFlowSystem->GetIModuleManager();
	if(pModuleManager)
	{
		pModuleManager->SaveModule(pFG->GetName(), rootNode);

		pFG->Serialize(rootNode, false);

		rootNode->saveToFile(filename);
	}
}

//////////////////////////////////////////////////////////////////////////
bool CEditorFlowGraphModuleManager::HasModifications()
{
	if (!gEnv->pFlowSystem)
		return false;

	IFlowGraphModuleManager* pModuleManager = gEnv->pFlowSystem->GetIModuleManager();

	if(pModuleManager == NULL)
		return false;

	for (int i = 0; i < m_FlowGraphs.size(); ++i)
	{
		IFlowGraphPtr pGraph = GetModuleFlowGraph(i);
		CFlowGraph* pFG = GetIEditor()->GetFlowGraphManager()->FindGraph(pGraph);
		assert(pFG);
		if (pFG && pFG->IsModified())
			return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CEditorFlowGraphModuleManager::NewModule( CString& filename, bool bGlobal/*=true*/, CHyperGraph** pHyperGraph )
{
	CString path(Path::Make(bGlobal ? Path::GetGameFolder() : GetIEditor()->GetLevelFolder(), bGlobal ? FLOWGRAPH_MODULE_PATH_GLOBAL : FLOWGRAPH_MODULE_PATH_LEVEL));

	CFileUtil::CreateDirectory(Path::AddBackslash(path));
		
	if (!CFileUtil::SelectSaveFile(GRAPH_FILE_FILTER, "xml", path, filename))
		return false;

	// create (temporary) default graph for module 
	CHyperGraph* pGraph = GetIEditor()->GetFlowGraphManager()->CreateGraph();
	pGraph->AddRef();
	IFlowGraphModuleManager* pModuleManager = gEnv->pFlowSystem->GetIModuleManager();

	if(pModuleManager == NULL)
		return false;

	CString moduleName = Path::GetFileName(filename);

	if (IFlowGraphModule* pModule = pModuleManager->GetModule(moduleName))
	{
		CString msg;
		msg.Format( _T("Flowgraph Module: %s already exist. %s Module"), moduleName, pModule->GetType() == IFlowGraphModule::eT_Global ? "Global" : "Level");
		CCustomMessageBox::Show(msg, _T("Can not create Flowgraph Module"), "Close");
		return false;
	}
	
	// serialize graph to file, with module flag set
	XmlNodeRef root = gEnv->pSystem->CreateXmlNode("Graph");
	root->setAttr("isModule", true);
	root->setAttr("moduleName", moduleName);
	pGraph->Serialize(root, false);
	bool saved = root->saveToFile(filename.GetString());

	pGraph->Release();
	pGraph = NULL;

	if (saved)
	{
		// load module into module manager
		IFlowGraphModule* pModule = pModuleManager->LoadModuleFile(moduleName, filename.GetString(), bGlobal);

		CFlowGraph* pFG = GetIEditor()->GetFlowGraphManager()->FindGraph(pModule->GetRootGraph());
		if (pHyperGraph) 
			*pHyperGraph = pFG;

		GetIEditor()->GetFlowGraphManager()->ReloadClasses();

		// now create a start node
		string startName = pModuleManager->GetStartNodeName(pFG->GetName());
		CHyperNode* pStartNode = (CHyperNode*)pFG->CreateNode(startName.c_str());
		pStartNode->SetPos(Gdiplus::PointF(80,10));

		// and end node
		string returnName = pModuleManager->GetReturnNodeName(pFG->GetName());
		CHyperNode* pReturnNode = (CHyperNode*)pFG->CreateNode(returnName.c_str());
		pReturnNode->SetPos( Gdiplus::PointF(400,10) );

		// and connect them
		pFG->UnselectAll();
		pFG->ConnectPorts( pStartNode, &pStartNode->GetOutputs()->at(0), pReturnNode, &pReturnNode->GetInputs()->at(0), false );
		pFG->ConnectPorts( pStartNode, &pStartNode->GetOutputs()->at(2), pReturnNode, &pReturnNode->GetInputs()->at(1), false );

		SaveModuleGraph(pFG);

		return true;
	}
	else
	{
		AfxMessageBox( "Can't create Flowgraph Module!\nCould not save new file!\n\nCreation canceled..." );
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CEditorFlowGraphModuleManager::PromptChanges()
{
	if (HasModifications())
	{
		CString msg(_T("Some Flowgraph Modules were modified!\nDo you want to save your changes?" ) );
		int idx = CCustomMessageBox::Show( msg,_T("Flowgraph Module(s) not saved!"), "Cancel" /*idx 1*/, "Don't save" /*idx 2*/, "Save" /*idx 3*/);
		if (idx == 3)
		{
			SaveChangedGraphs();
		}
		else if (idx == 1)
		{
			return false;
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
IFlowGraphPtr CEditorFlowGraphModuleManager::GetModuleFlowGraph(int index) const
{
	if (index >= 0 && index < m_FlowGraphs.size())
	{
		CFlowGraph* pGraph = m_FlowGraphs[index];

		if(pGraph == NULL)
			return NULL;

		return pGraph->GetIFlowGraph();
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////
IFlowGraphPtr CEditorFlowGraphModuleManager::GetModuleFlowGraph(const char* name) const
{
	assert( name != NULL );
	if ( name == NULL ) 
		return NULL;

	for ( TFlowGraphs::const_iterator iter = m_FlowGraphs.begin(); iter != m_FlowGraphs.end(); ++iter )
	{
		CFlowGraph* pGraph = (*iter);
		if (pGraph == NULL)
			continue;

		if ( string( pGraph->GetName()).MakeUpper() ==  string(name).MakeUpper() )
			return pGraph->GetIFlowGraph();
	}

	return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CEditorFlowGraphModuleManager::DeleteModuleFlowGraph(CFlowGraph* pGraph)
{
	assert(pGraph != NULL);
	if (!pGraph) 
		return;

	// 1. delete all caller nodes used in any of the flowgraphs
	// 2. delete the CFlowGraph from the internal list
	// 3. release our CFlowGraph reference
	if (IFlowGraphModuleManager* pModuleManager = gEnv->pFlowSystem->GetIModuleManager())
	{
		const char* moduleGraphName = pGraph->GetName();
				
		if (CFlowGraphManager* pFlowGraphManager = GetIEditor()->GetFlowGraphManager())
		{
			const int numFlowgraphs = pFlowGraphManager->GetFlowGraphCount();
			const char* callerNodeName = pModuleManager->GetCallerNodeName(moduleGraphName);
		
			for (int i=0; i<numFlowgraphs;++i)
			{
				CFlowGraph* pFlowgraph = pFlowGraphManager->GetFlowGraph(i);
				IHyperGraphEnumerator* pEnum = pFlowgraph->GetNodesEnumerator();
				
				for (IHyperNode* pINode = pEnum->GetFirst(); pINode;)
				{
					CHyperNode* pHyperNode = reinterpret_cast<CHyperNode*>(pINode);
					if (!stricmp(pHyperNode->GetClassName(), callerNodeName))
					{
						IHyperNode* pTempNode = pEnum->GetNext();
						pFlowgraph->RemoveNode(pINode);
						pINode = pTempNode;
					}
					else
						pINode = pEnum->GetNext();
				}
			}
		}
	}

	if (stl::find_and_erase(m_FlowGraphs, pGraph))
		SAFE_RELEASE(pGraph);
}

//////////////////////////////////////////////////////////////////////////
void CEditorFlowGraphModuleManager::CreateModuleNodes(const char* moduleName)
{
	IFlowGraphModuleManager* pManager = gEnv->pFlowSystem->GetIModuleManager();
	IFlowGraphModule* pModule = pManager->GetModule(moduleName);
	if(pModule)
	{
		pManager->CreateModuleNodes(moduleName, pModule->GetId());
		GetIEditor()->GetFlowGraphManager()->ReloadClasses();
	}
}

void CEditorFlowGraphModuleManager::OnModuleDestroyed( IFlowGraphModule* module )
{
	IFlowGraphPtr pRootGraph = module->GetRootGraph();
	CFlowGraph* pFlowgraph = GetIEditor()->GetFlowGraphManager()->FindGraph(pRootGraph);

	if(pFlowgraph)
	{
		DeleteModuleFlowGraph(pFlowgraph);
	}
}

void CEditorFlowGraphModuleManager::OnRootGraphChanged(IFlowGraphModule* module)
{	
	CFlowGraph* pFlowgraph = GetIEditor()->GetFlowGraphManager()->FindGraph(module->GetRootGraph());

	if(pFlowgraph)
		return;

	GetIEditor()->GetFlowGraphManager()->ReloadClasses();

	CFlowGraph* pFG = GetIEditor()->GetFlowGraphManager()->CreateGraphForModule(module);
	assert(pFG);
	if (pFG) 
	{
		pFG->AddRef();
		m_FlowGraphs.push_back(pFG);
	}
}

void CEditorFlowGraphModuleManager::OnScannedForModules()
{
	CreateEditorFlowgraphs();
	ReloadHypergraphDialog();
}

void CEditorFlowGraphModuleManager::ReloadHypergraphDialog()
{
	CWnd* pWnd = GetIEditor()->FindView( "Flow Graph" );
	if (pWnd && pWnd->IsKindOf(RUNTIME_CLASS(CHyperGraphDialog)))
	{
		CHyperGraphDialog* pHyperGraphDialog = static_cast<CHyperGraphDialog*>(pWnd);
		GetIEditor()->GetFlowGraphManager()->ReloadClasses();
		pHyperGraphDialog->ReloadGraphs();
		pHyperGraphDialog->ReloadComponents();
		pHyperGraphDialog->ReloadBreakpoints();
	}
}

void CEditorFlowGraphModuleManager::CreateEditorFlowgraphs()
{
	if(m_FlowGraphs.size() > 0)
	{
		TFlowGraphs::iterator iter = m_FlowGraphs.begin();
		for (iter;iter!=m_FlowGraphs.end();++iter)
		{
			SAFE_RELEASE((*iter))
		}

		m_FlowGraphs.clear();
	}

	IModuleIteratorPtr pIter = gEnv->pFlowSystem->GetIModuleManager()->CreateModuleIterator();
	if(pIter == NULL)
		return;

	IFlowGraphModule* pModule = pIter->Next();
	const int count = pIter->Count();

	if(count == 0)
		return;

	m_FlowGraphs.reserve(count);

	for (int i=0; i < count; ++i)
	{
		CFlowGraph* pFG = GetIEditor()->GetFlowGraphManager()->CreateGraphForModule(pModule);
		assert(pFG);
		if (pFG) 
		{
			pFG->AddRef();

			m_FlowGraphs.push_back(pFG);
		}

		pModule = pIter->Next();
	}
}
