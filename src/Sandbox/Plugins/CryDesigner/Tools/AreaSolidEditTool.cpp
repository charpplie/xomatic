#include "StdAfx.h"
#include "AreaSolidEditTool.h"
#include "Objects/AreaSolidObject.h"
#include "EditMode/ObjectMode.h"
#include "Tools/BrushDesignerBaseTool.h"
#include "IBaseToolPanel.h"

namespace
{
	IBrushDesignerEditToolPanel* s_pMenuPanel = NULL;
}

IMPLEMENT_DYNCREATE(CAreaSolidEditTool,CBrushDesignerEditTool)

class CAreaSolidEditTool_ClassDesc : public CRefCountClassDesc
{
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_EDITTOOL; }
	virtual REFGUID ClassID()
	{ 
		// {df8d2e5b-597a-40d4-8f1b-7590ae7c9071}
		static const GUID AREASOLID_TOOL_GUID = { 0xdf8d2e5b,0x597a,0x40d4, {0x8f, 0x1b, 0x75, 0x90, 0xae, 0x7c, 0x90, 0x71} };
		return AREASOLID_TOOL_GUID; 
	}
	virtual const char* ClassName() { return "EditTool.AreaSolidTool"; };
	virtual const char* Category() { return "Brush"; };
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CAreaSolidEditTool); }
};

void CAreaSolidEditTool::RegisterTool( CRegistrationContext &rc )
{
	rc.pClassFactory->RegisterClass( new CAreaSolidEditTool_ClassDesc );
}

void CAreaSolidEditTool::SetUserData( const char *key,void *userData )
{
	CAreaSolid* pAreaSolid = (CAreaSolid*)userData;
	if(!pAreaSolid)
		return;

	SetBaseObject(pAreaSolid);
	CBrushDesignerEditTool::SetPanelOwner(NULL);

	if (GetIEditor()->IsUndoRecording())
		GetIEditor()->AcceptUndo( CString("New ")+pAreaSolid->GetTypeName() );

	InitializeEventHandlers();
}

IBrushDesignerEditToolPanel* CAreaSolidEditTool::GetDesignerToolPanel() const
{
	return s_pMenuPanel;
}

void CAreaSolidEditTool::BeginEditParams( IEditor *ie,int flags )
{
	if( !s_pMenuPanel )
	{
		s_pMenuPanel = CreateDesignerEditToolPanel();

		s_pMenuPanel->DisableButton(IDC_DESIGNER_OBJECTMODE);
		s_pMenuPanel->DisableButton(IDC_DESIGNER_SEPARATETOOL);
		s_pMenuPanel->DisableButton(IDC_DESIGNER_COPYTOOL);
		s_pMenuPanel->DisableButton(IDC_DESIGNER_MERGETOOL);
		s_pMenuPanel->DisableButton(IDC_DESIGNER_ARRAYCLONETOOL);
		s_pMenuPanel->DisableButton(IDC_DESIGNER_CLONETOOL);
		s_pMenuPanel->DisableButton(IDC_DESIGNER_MAPPINGTOOL);
		s_pMenuPanel->DisableButton(IDC_DESIGNER_EXPORTTOOL);
		s_pMenuPanel->DisableButton(IDC_DESIGNER_BOOLEANTOOL);
		s_pMenuPanel->DisableButton(IDC_DESIGNER_DEBUGGERTOOL);
		s_pMenuPanel->DisableButton(IDC_DESIGNER_EXCLUSIVEMODE);
		s_pMenuPanel->DisableButton(IDC_DESIGNER_BEVELTOOL);
		s_pMenuPanel->DisableButton(IDC_DESIGNER_MAGNETTOOL);
		s_pMenuPanel->DisableButton(IDC_DESIGNER_LATHETOOL);
		s_pMenuPanel->DisableButton(IDC_DESIGNER_BEVELTOOL);
		s_pMenuPanel->DisableButton(IDC_DESIGNER_SUBDIVISION);
		s_pMenuPanel->DisableButton(IDC_DESIGNER_SMOOTHINGGROUP);
	}

	CreateMappingTableBetweenToolIdAndButtonId();

	if( s_pMenuPanel )
		s_pMenuPanel->SetEditTool(this,BUtil::eDesigner_Box);
}

void CAreaSolidEditTool::EndEditParams()
{
	if( GetCurrentTool() )
		GetCurrentTool()->Leave();

	if (s_pMenuPanel)
		s_pMenuPanel->DestroyPanel();
}

bool CAreaSolidEditTool::OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{
	if( nChar == VK_ESCAPE )
	{
		if( BUtil::IsSelectElementMode(m_DesignerMode) )
		{
			if( GetCurrentTool() )
				GetCurrentTool()->Leave();
			return false;
		}
		else if( m_DesignerMode == BUtil::eDesigner_Pivot )
		{
			GoToSelectDesignerMode();
			return true;
		}
	}

	CBrushDesignerBaseTool* pHandler = m_ToolMap[m_DesignerMode];
	if( pHandler == NULL )
		return false;

	bool bContinueEdit = pHandler->OnKeyDown(view,nChar,nRepCnt,nFlags);
	if( !bContinueEdit )
	{
		if( GetCurrentTool() )
			GetCurrentTool()->ReleaseObjectGizmo();
	}

	return bContinueEdit;
}

void CAreaSolidEditTool::Finish()
{
	if( m_pBaseObject && m_pBaseObject->IsKindOf(RUNTIME_CLASS(CAreaSolid)) )
	{
		CBrushDesigner* pDesigner = ((CAreaSolid*)&*m_pBaseObject)->GetDesigner();
		if( !pDesigner )
			return;

		bool bEmptyShelf0 = pDesigner->IsEmpty(0);
		bool bEmptyShelf1 = pDesigner->IsEmpty(1);

		if( bEmptyShelf0 && bEmptyShelf1 )
		{
			GetIEditor()->SuspendUndo();
			GetIEditor()->DeleteObject(m_pBaseObject);
			GetIEditor()->ResumeUndo();
		}
		else if( !bEmptyShelf1 )
		{
			if( GetCurrentTool() )
				GetCurrentTool()->FreezeDesigner();			
		}
	}
}

int CAreaSolidEditTool::GetPanelIndex() const
{
	if( s_pMenuPanel )
		s_pMenuPanel->GetPanelIndex();
	return -1;
}