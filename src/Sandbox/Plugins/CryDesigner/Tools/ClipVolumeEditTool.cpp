#include "StdAfx.h"
#include "ClipVolumeEditTool.h"
#include "EditMode/ObjectMode.h"
#include "Core/BrushCommonInterface.h"
#include "BrushDesignerBaseTool.h"
#include "IBaseToolPanel.h"

IMPLEMENT_DYNCREATE(CClipVolumeEditTool,CBrushDesignerEditTool)

class CClipVolumeEditTool_ClassDesc : public CRefCountClassDesc
{
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_EDITTOOL; }
	virtual REFGUID ClassID()
	{ 
		// {0FFA00A8-0476-4398-9938-7AEC7C736BEB}
		static const GUID CLIPVOLUME_TOOL_GUID = 
		{ 0xffa00a8, 0x476, 0x4398, { 0x99, 0x38, 0x7a, 0xec, 0x7c, 0x73, 0x6b, 0xeb } };

		return CLIPVOLUME_TOOL_GUID; 
	}
	virtual const char* ClassName() { return "EditTool.ClipVolumeEditTool"; };
	virtual const char* Category() { return "Brush"; };
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CClipVolumeEditTool); }
};

IBrushDesignerEditToolPanel* CClipVolumeEditTool::s_pMenuPanel = NULL;

void CClipVolumeEditTool::RegisterTool( CRegistrationContext &rc )
{
	rc.pClassFactory->RegisterClass( new CClipVolumeEditTool_ClassDesc );
}

void CClipVolumeEditTool::SetUserData( const char *key,void *userData )
{
	CBaseObject* pClipVolume = (CBaseObject*)userData;
	if(!pClipVolume)
		return;

	SetBaseObject(pClipVolume);
	CBrushDesignerEditTool::SetPanelOwner(NULL);

	if (GetIEditor()->IsUndoRecording())
		GetIEditor()->AcceptUndo( CString("New ")+pClipVolume->GetTypeName() );

	InitializeEventHandlers();
}

IBrushDesignerEditToolPanel* CClipVolumeEditTool::GetDesignerToolPanel() const
{
	return s_pMenuPanel;
}

void CClipVolumeEditTool::BeginEditParams( IEditor *ie,int flags )
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
		s_pMenuPanel->DisableButton(IDC_DESIGNER_REMOVETOOL);
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

void CClipVolumeEditTool::EndEditParams()
{
	if( GetCurrentTool() )
		GetCurrentTool()->Leave();

	if (s_pMenuPanel)
	{
		s_pMenuPanel->DestroyPanel();
		s_pMenuPanel = 0;
		CBrushCommonInterface::UpdateGameResource(m_pBaseObject);
	}
}

bool CClipVolumeEditTool::OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
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

void CClipVolumeEditTool::OnManipulatorMouseEvent( CViewport *view, ITransformManipulator *pManipulator, EMouseEvent event, CPoint &point, int flags, bool bHitGizmo )
{
	__super::OnManipulatorMouseEvent(view, pManipulator, event, point, flags, bHitGizmo);

	if(CBrushDesignerBaseTool* pHandler = GetCurrentTool())
	{
		if(event == eMouseLUp)
			CBrushCommonInterface::UpdateGameResource(m_pBaseObject);
	}
}

void CClipVolumeEditTool::Finish()
{
	if( m_pBaseObject )
	{
		CBrushDesigner* pDesigner;
		if(CBrushCommonInterface::GetDesigner(m_pBaseObject, pDesigner))
		{
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
}

int CClipVolumeEditTool::GetPanelIndex() const
{
	if( s_pMenuPanel )
		return s_pMenuPanel->GetPanelIndex();
	return -1;
}