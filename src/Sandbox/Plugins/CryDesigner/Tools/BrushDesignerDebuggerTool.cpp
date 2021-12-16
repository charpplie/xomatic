#include "StdAfx.h"
#include "BrushDesignerDebuggerTool.h"
#include "BrushDesignerSelectTool.h"
#include "IBaseToolPanel.h"
#include "BrushDesignerEditTool.h"

void CBrushDesignerDebuggerTool::Enter()
{
	IDesignerRegionDebuggerDlg* dlg = CreateRegionDebuggerDlg();
	CBrushDesignerElementManager* pSelected = GetEditTool()->GetSelectedElements();

	for( int i = 0, iElementCount(pSelected->GetSize()); i < iElementCount; ++i )
	{
		if( !(*pSelected)[i].IsFace() || (*pSelected)[i].m_pRegion == NULL )
			continue;

		CString buffer;
		buffer.Format("Region #%d",i);
		dlg->AddRegion((*pSelected)[i].m_pRegion.get(),buffer);
	}

	if( dlg->GetRegionCount() > 0 )
		dlg->Open();
	else
		AfxMessageBox("At least one face need to be selected.",MB_OK);

	GetEditTool()->GoToSelectDesignerMode();
}