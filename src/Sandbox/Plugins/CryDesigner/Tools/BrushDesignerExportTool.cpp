#include "StdAfx.h"
#include "BrushDesignerExportTool.h"
#include "BrushDesignerEditTool.h"
#include "CryEdit.h"
#include "GameEngine.h"
#include "IBaseToolPanel.h"

namespace 
{
	IBaseToolPanel* s_pExportToolPanel = NULL;
}

void CBrushDesignerExportTool::BeginEditParams()
{
	if( !s_pExportToolPanel )
		s_pExportToolPanel = CreateExportToolPanel(this);
}

void CBrushDesignerExportTool::EndEditParams()
{
	if( s_pExportToolPanel )
	{
		s_pExportToolPanel->DestroyPanel();
		s_pExportToolPanel = NULL;
	}
}

void CBrushDesignerExportTool::ExportToCgf()
{
	std::vector<BUtil::SSelectedInfo> selections;
	GetEditTool()->GetSelectedObjectList(selections);	

	if( selections.size() == 1 )
	{
		CString filename;
		CString levelPath = GetIEditor()->GetGameEngine()->GetLevelPath();
		if( CFileUtil::SelectSaveFile( "CGF Files|*.cgf","*.cgf",levelPath,filename ))
			selections[0].m_pBrush->SaveToCgf( filename );
	}
	else
	{
		AfxMessageBox( "Only one object must be selected to save it to cgf file.", MB_OK );
	}
}

void CBrushDesignerExportTool::ExportToGrp()
{
	((CCryEditApp*)AfxGetApp())->SaveSelectedObjects(CFileUtil::FormatInitialFolderForFileDialog(GetIEditor()->GetGameEngine()->GetLevelPath()));
}

void CBrushDesignerExportTool::ExportToObj()
{
	((CCryEditApp*)AfxGetApp())->OnExportSelectedObjects();
}