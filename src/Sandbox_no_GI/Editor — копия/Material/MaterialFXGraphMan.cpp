#include "StdAfx.h"
#include "MaterialFXGraphMan.h"

#include "../HyperGraph/FlowGraph.h"
#include "../HyperGraph/FlowGraphManager.h"
#include "CustomMessageBox.h"
#include <IMaterialEffects.h>

#define MATERIAL_FX_PATH ( "Libs/MaterialEffects/FlowGraphs" )
#define GRAPH_FILE_FILTER "Graph XML Files (*.xml)|*.xml"

CMaterialFXGraphMan::CMaterialFXGraphMan()
{

}

CMaterialFXGraphMan::~CMaterialFXGraphMan()
{
	GetIEditor()->GetLevelIndependentFileMan()->UnregisterModule(this);
}

void CMaterialFXGraphMan::Init()
{
	GetIEditor()->GetLevelIndependentFileMan()->RegisterModule(this);
	ReloadFXGraphs();
}


void CMaterialFXGraphMan::ReloadFXGraphs()
{
	if (!gEnv->pMaterialEffects) return;

	for (int i = 0; i < gEnv->pMaterialEffects->GetMatFXFlowGraphCount(); ++i)
	{
		IFlowGraphPtr pGraph = gEnv->pMaterialEffects->GetMatFXFlowGraph(i);
		CFlowGraph* pFG = GetIEditor()->GetFlowGraphManager()->FindGraph(pGraph);
		SAFE_RELEASE(pFG);
	}

	gEnv->pMaterialEffects->ReloadMatFXFlowGraphs();

	for (int i = 0; i < gEnv->pMaterialEffects->GetMatFXFlowGraphCount(); ++i)
	{
		string filename;
		IFlowGraphPtr pGraph = gEnv->pMaterialEffects->GetMatFXFlowGraph(i, &filename);
		CFlowGraph* pFG = GetIEditor()->GetFlowGraphManager()->CreateGraphForMatFX(pGraph, CString(filename.c_str()));
		assert(pFG);
		if (pFG) pFG->AddRef();
	}
}

void CMaterialFXGraphMan::SaveChangedGraphs()
{
	if (!gEnv->pMaterialEffects) return;

	for (int i = 0; i < gEnv->pMaterialEffects->GetMatFXFlowGraphCount(); ++i)
	{
		IFlowGraphPtr pGraph = gEnv->pMaterialEffects->GetMatFXFlowGraph(i);
		CFlowGraph* pFG = GetIEditor()->GetFlowGraphManager()->FindGraph(pGraph);
		assert(pFG);
		if (pFG && pFG->IsModified())
		{
			CString filename( MATERIAL_FX_PATH );
			filename += '/';
			filename += pFG->GetName();
			filename += ".xml";
			pFG->Save( filename );
		}
	}
}

bool CMaterialFXGraphMan::HasModifications()
{
	if (!gEnv->pMaterialEffects) return false;

	for (int i = 0; i < gEnv->pMaterialEffects->GetMatFXFlowGraphCount(); ++i)
	{
		IFlowGraphPtr pGraph = gEnv->pMaterialEffects->GetMatFXFlowGraph(i);
		CFlowGraph* pFG = GetIEditor()->GetFlowGraphManager()->FindGraph(pGraph);
		assert(pFG);
		if (pFG && pFG->IsModified())
			return true;
	}
	return false;
}

bool CMaterialFXGraphMan::NewMaterialFx( CString& filename, CHyperGraph** pHyperGraph )
{
	if (!gEnv->pMaterialEffects) return false;

	CFileUtil::CreateDirectory( Path::GetGameFolder() + CString("/") + MATERIAL_FX_PATH );

	if ( !CFileUtil::SelectSaveFile( GRAPH_FILE_FILTER, "xml", Path::GetGameFolder() + CString("/") + MATERIAL_FX_PATH, filename ) )
		return false;

	// check if file exists.
	FILE *file = fopen( filename, "rb" );
	if ( file )
	{
		fclose(file);
		AfxMessageBox( "Can't create Material FX Graph because another Material FX Graph with this name already exists!\n\nCreation canceled..." );
		return false;
	}

	// create default MatFX Graph
	CHyperGraph* pGraph = GetIEditor()->GetFlowGraphManager()->CreateGraph();

	CHyperNode* pStartNode = (CHyperNode*) pGraph->CreateNode( "MaterialFX:HUDStartFX" );
	pStartNode->SetPos( Gdiplus::PointF(80,10) );
	CHyperNode* pEndNode = (CHyperNode*) pGraph->CreateNode( "MaterialFX:HUDEndFX" );
	pEndNode->SetPos( Gdiplus::PointF(400,10) );

	pGraph->UnselectAll();
	pGraph->ConnectPorts( pStartNode, &pStartNode->GetOutputs()->at(0), pEndNode, &pEndNode->GetInputs()->at(0), false );

	bool saved = pGraph->Save( filename.GetString() );
	delete pGraph;

	if (saved)
	{
		IFlowGraphPtr pNewGraph = gEnv->pMaterialEffects->LoadNewMatFXFlowGraph(filename.GetString());
		assert(pNewGraph);
		if (pNewGraph)
		{
			CFlowGraph* pFG = GetIEditor()->GetFlowGraphManager()->CreateGraphForMatFX(pNewGraph, filename);
			assert(pFG);
			if (pFG) pFG->AddRef();
			if (pHyperGraph) *pHyperGraph = pFG;
			return true;
		}
	}
	else
	{
		AfxMessageBox( "Can't create Material FX Graph!\nCould not save new file!\n\nCreation canceled..." );
	}
	return false;
}

bool CMaterialFXGraphMan::PromptChanges()
{
	if (HasModifications())
	{
		CString msg(_T("Some Material FX flowgraphs are modified!\nDo you want to save your changes?" ) );
		int idx = CCustomMessageBox::Show( msg,_T("Material FX Graph(s) not saved!"), "Cancel" /*idx 1*/, "Don't save" /*idx 2*/, "Save" /*idx 3*/);
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