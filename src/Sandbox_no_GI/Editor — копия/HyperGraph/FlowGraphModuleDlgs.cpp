////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2006.
// -------------------------------------------------------------------------
//  File name:   FlowGraphProperties.cpp
//  Version:     v1.00
// 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////


#include "StdAfx.h"
#include "FlowGraphModuleDlgs.h"
#include "HyperGraphDialog.h"
#include "FlowGraphModuleManager.h"
#include "FlowGraphManager.h"
#include "FlowGraph.h"

namespace
{
	CString typeNames[] = 
	{
		"Void",			// 	eFDT_Void
		"Int",			// 	eFDT_Int
		"Float",		// 	eFDT_Float
		"EntityId",	// 	eFDT_EntityId
		"Vec3",			// 	eFDT_Vec3
		"String",		// 	eFDT_String
		"Bool",			// 	eFDT_Bool
	};
};

//////////////////////////////////////////////////////////////////////////
// CFlowGraphEditModuleDlg
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CFlowGraphEditModuleDlg, CDialog)

CFlowGraphEditModuleDlg::CFlowGraphEditModuleDlg(IFlowGraphModule* pModule, CWnd* pParent /*=NULL*/)
	: CDialog(CFlowGraphEditModuleDlg::IDD, pParent)
	, m_pModule(pModule)
{
	assert(m_pModule);

	// extract copy of ports from graph, so we can edit locally yet still cancel changes
	for(size_t i=0; i<m_pModule->GetModulePortCount(); ++i)
	{
		const IFlowGraphModule::SModulePortConfig* pPort = m_pModule->GetModulePort(i);
		if(pPort->input)
			m_inputs.push_back(*pPort);
		else
			m_outputs.push_back(*pPort);
	}
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphEditModuleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FG_MODULE_INPUT_LIST,				m_inputListCtrl);
	DDX_Control(pDX, IDC_FG_MODULE_OUTPUT_LIST,				m_outputListCtrl);
}

//////////////////////////////////////////////////////////////////////////
BOOL CFlowGraphEditModuleDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	RefreshControl();

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CFlowGraphEditModuleDlg, CDialog)
	ON_COMMAND(IDC_ADD_MODULE_INPUT, OnCommand_NewInput)
	ON_COMMAND(IDC_DELETE_MODULE_INPUT, OnCommand_DeleteInput)
	ON_COMMAND(IDC_EDIT_MODULE_INPUT, OnCommand_EditInput)
	ON_COMMAND(IDC_ADD_MODULE_OUTPUT, OnCommand_NewOutput)
	ON_COMMAND(IDC_DELETE_MODULE_OUTPUT, OnCommand_DeleteOutput)
	ON_COMMAND(IDC_EDIT_MODULE_OUTPUT, OnCommand_EditOutput)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
void CFlowGraphEditModuleDlg::OnCommand_NewInput()
{
	IFlowGraphModule::SModulePortConfig input;
	input.input = true;
	input.type = eFDT_Bool;
 	CFlowGraphNewModuleInputDlg dlg(&input);
 	if(dlg.DoModal() == IDOK && input.name != "" && input.type != eFDT_Void)
 	{
		if (isdigit(input.name[0]))
		{
			CryMessageBox("Input name should not start with a number!", "Error", MB_OK|MB_ICONERROR);
			return;
		}
 		m_inputs.push_back(input);
 	}

	RefreshControl();
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphEditModuleDlg::OnCommand_DeleteInput()
{
  int selected = m_inputListCtrl.GetSelCount();
	CArray<int, int> listSelections;
	listSelections.SetSize(selected);
	m_inputListCtrl.GetSelItems(selected, listSelections.GetData());

	for(int i=0; i<selected; ++i)
	{
		int item = listSelections[i];

		CString temp;
		m_inputListCtrl.GetText(item, temp);

		m_inputs[item] = IFlowGraphModule::SModulePortConfig();
	}

	stl::find_and_erase_all(m_inputs, IFlowGraphModule::SModulePortConfig());

	RefreshControl();
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphEditModuleDlg::OnCommand_EditInput()
{
 	int selected = m_inputListCtrl.GetSelCount();
 
 	if(selected == 1)
 	{
 		CArray<int, int> listSelections;
 		listSelections.SetSize(selected);
 		m_inputListCtrl.GetSelItems(selected, listSelections.GetData());
 
 		int item = listSelections[0];
 
 		CFlowGraphNewModuleInputDlg dlg(&m_inputs[item]);
 		dlg.DoModal();
 
 		RefreshControl();
 	}
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphEditModuleDlg::OnCommand_NewOutput()
{
	IFlowGraphModule::SModulePortConfig output;
	output.input = false;
	output.type = eFDT_Bool;
	CFlowGraphNewModuleInputDlg dlg(&output);
	if(dlg.DoModal() == IDOK && output.name != "" && output.type != eFDT_Void)
	{
		if (isdigit(output.name[0]))
		{
			CryMessageBox("Output name should not start with a number!", "Error", MB_OK|MB_ICONERROR);
			return;
		}
		m_outputs.push_back(output);
	}

	RefreshControl();
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphEditModuleDlg::OnCommand_DeleteOutput()
{
	int selected = m_outputListCtrl.GetSelCount();
	CArray<int, int> listSelections;
	listSelections.SetSize(selected);
	m_outputListCtrl.GetSelItems(selected, listSelections.GetData());

	for(int i=0; i<selected; ++i)
	{
		int item = listSelections[i];

		CString temp;
		m_outputListCtrl.GetText(item, temp);

		m_outputs[item] = IFlowGraphModule::SModulePortConfig();
	}

	stl::find_and_erase_all(m_outputs, IFlowGraphModule::SModulePortConfig());

	RefreshControl();
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphEditModuleDlg::OnCommand_EditOutput()
{
	int selected = m_outputListCtrl.GetSelCount();

	if(selected == 1)
	{
		CArray<int, int> listSelections;
		listSelections.SetSize(selected);
		m_outputListCtrl.GetSelItems(selected, listSelections.GetData());

		int item = listSelections[0];

		CFlowGraphNewModuleInputDlg dlg(&m_outputs[item]);
		dlg.DoModal();

		RefreshControl();
	}
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphEditModuleDlg::RefreshControl()
{
	m_inputListCtrl.ResetContent();
	m_outputListCtrl.ResetContent();

	for(TPorts::const_iterator it = m_inputs.begin(), end = m_inputs.end(); it != end; ++it)
	{
		assert(it->input);

		CString text = it->name;
		text += " (";
		text += typeNames[it->type];
		text += ")";
		m_inputListCtrl.AddString(text.GetBuffer());
	}

	for(TPorts::const_iterator it = m_outputs.begin(), end = m_outputs.end(); it != end; ++it)
	{
		assert(!it->input);

		CString text = it->name;
		text += " (";
		text += typeNames[it->type];
		text += ")";
		m_outputListCtrl.AddString(text.GetBuffer());
	}
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphEditModuleDlg::OnOK()
{
	if(m_pModule)
	{
		// write inputs/outputs back to graph
		m_pModule->RemoveModulePorts();

		for(TPorts::const_iterator it = m_inputs.begin(), end = m_inputs.end(); it != end; ++it)
		{
			assert(it->input);

			m_pModule->AddModulePort(*it);
		}

		for(TPorts::const_iterator it = m_outputs.begin(), end = m_outputs.end(); it != end; ++it)
		{
			assert(!it->input);

			m_pModule->AddModulePort(*it);
		}
	}
	
	GetIEditor()->GetFlowGraphModuleManager()->CreateModuleNodes(m_pModule->GetName());
	
	CDialog::OnOK();
}

//////////////////////////////////////////////////////////////////////////
// CFlowGraphNewModuleInputDlg
//////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CFlowGraphNewModuleInputDlg, CDialog)

CFlowGraphNewModuleInputDlg::CFlowGraphNewModuleInputDlg(IFlowGraphModule::SModulePortConfig* pPort, CWnd* pParent /*=NULL*/)
	: CDialog(CFlowGraphNewModuleInputDlg::IDD, pParent)
	, m_pPort(pPort)
{
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraphNewModuleInputDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MODULE_INPUT_TYPE, m_inputTypesCtrl);
	DDX_Control(pDX, IDC_MODULE_INPUT_NAME, m_inputNameCtrl);
}

//////////////////////////////////////////////////////////////////////////
BOOL CFlowGraphNewModuleInputDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	for(int i=1; i<sizeof(typeNames) / sizeof(CString); ++i)
	{
		m_inputTypesCtrl.AddString(typeNames[i]);
		m_inputTypesCtrl.SetItemData(i,i);
	}

	m_inputNameCtrl.SetWindowText(m_pPort->name.c_str());

	m_inputTypesCtrl.SetWindowText(typeNames[m_pPort->type]);

	if(m_pPort->input)
		SetWindowText("Edit Module Input");
	else
		SetWindowText("Edit Module Output");

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CFlowGraphNewModuleInputDlg, CDialog)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
void CFlowGraphNewModuleInputDlg::OnOK()
{
 	if(m_pPort)
 	{
		CString itemName;
 		m_inputNameCtrl.GetWindowText(itemName);
		m_pPort->name = itemName.GetBuffer();
 
 		CString itemType;
 		m_inputTypesCtrl.GetWindowText(itemType);
 
 		for(int i=0; i<sizeof(typeNames) / sizeof(CString); ++i)
 		{
 			if(itemType == typeNames[i])
 			{
 				m_pPort->type = (EFlowDataTypes)i;
 				break;
 			}
 		}
 	}
	
	CDialog::OnOK();
}