#include "StdAfx.h"
#include "SelectionTreeModifier.h"

#include "SelectionTreeManager.h"

#include "CustomMessageBox.h"
#include "Dialogs/AddNewTreeDialog.h"
#include "Dialogs/AddNewBlockGroupDialog.h"
#include "Dialogs/DialogCommon.h"

#include "Views/SelectionTreeVariablesView.h"
#include "Views/SelectionTreeSignalsView.h"

CSelectionTreeModifier::CSelectionTreeModifier(CSelectionTreeManager* pManager)
: m_pManager(pManager)
{

}

CSelectionTreeModifier::~CSelectionTreeModifier()
{

}

void CSelectionTreeModifier::ShowErrorMsg(const char* title, const char* textFormat, ...)
{
	va_list args;	
	va_start( args, textFormat );
	char tmp[5096];
	vsnprintf_s( tmp, 5096, 5095, textFormat, args );
	va_end( args );

	CString error( tmp );
	CCustomMessageBox::Show( error, title, "OK" );
}

CSelectionTreeModifier::EMsgBoxResult CSelectionTreeModifier::ShowMsgBox(const char* title, const char* userButton1, const char* userButton2, const char* textFormat, ...)
{
	va_list args;	
	va_start( args, textFormat );
	char tmp[5096];
	vsnprintf_s( tmp, 5096, 5095, textFormat, args );
	va_end( args );

	CString msg( tmp );
	int idx = CCustomMessageBox::Show( msg, title, "Cancel" /*idx 1*/, userButton2 /*idx 2*/, userButton1 /*idx 3*/);
	switch (idx)
	{
	case 1: return eMBR_Cancel;
	case 2: return eMBR_UserBtn2;
	case 3: return eMBR_UserBtn1;
	}
	return eMBR_Cancel;
}

void CSelectionTreeModifier::CreateNewTree()
{
	CAddNewTreeDialog newDlg;
	newDlg.Init( "New Selection Tree", "NewTree" );
	if(newDlg.DoModal() == IDOK)
	{
		CSelectionTreeManager::STreeDefinition def;
		def.name = newDlg.GetName().c_str();
		def.type = newDlg.GetType().c_str();
		string newFile = string().Format("Scripts\\AI\\SelectionTrees\\%s.xml", def.name);
		def.filename = newFile.c_str();
		if (strlen(def.name) == 0)
		{
			CString error;
			error.Format( _T("Tree \"%s\" could not created, invalid name!" ), def.name );
			CCustomMessageBox::Show( error,_T("Error: Faild to create new SelectionTree!"), "OK" );
			return;
		}
		if (!m_pManager->AddNewTree( def ))
		{
			CString error;
			error.Format( _T("Tree \"%s\" could not created, tree with same name alredy exist!" ), def.name );
			CCustomMessageBox::Show( error,_T("Error: Faild to create new SelectionTree!"), "OK" );
		}
	}
}

void CSelectionTreeModifier::CreateNewRefBlock()
{
	CAddNewBlockGroupDialog newDlg;
	newDlg.Init( "New Reference Block", "NewRef", true, true, true, "NewRefVars", "NewRefSignals", "NewRef" );
	if(newDlg.DoModal() == IDOK)
	{
		CSelectionTreeManager::SRefGroupDefinition def;
		string groupName = newDlg.GetName();
		string varsName = newDlg.GetVarsName();
		string signalsName = newDlg.GetSignalsName();
		string treeName = newDlg.GetTreeName();
		if (newDlg.IsAdvanced())
		{
			def.vars = newDlg.HasVars();
			def.signals = newDlg.HasSignals();
			def.tree = newDlg.HasTree();
		}
		else
		{
			varsName = groupName + "Vars";
			signalsName = groupName + "Signals";
			treeName = groupName;
			def.vars = true;
			def.signals = true;
			def.tree = true;
		}
		string newFile = string().Format("Scripts\\AI\\SelectionTrees\\References\\%s.xml", groupName.c_str());

		def.name = groupName.c_str();
		def.varsname = varsName.c_str();
		def.signalsname = signalsName.c_str();
		def.treename = treeName.c_str();
		def.filename = newFile.c_str();

		if (!(def.vars || def.signals || def.tree))
		{
			CString error;
			error.Format( _T("Reference block \"%s\" could not be created, need at least one block!" ), def.name );
			CCustomMessageBox::Show( error,_T("Error: Faild to create Reference block!"), "OK" );
			return;
		}
		if (def.vars && strlen(def.varsname) == 0)
		{
			CString error;
			error.Format( _T("Reference block \"%s\" could not be created, Invalid name for variable block!" ), def.name );
			CCustomMessageBox::Show( error,_T("Error: Faild to create Reference block!"), "OK" );
			return;
		}
		if (def.signals && strlen(def.signalsname) == 0)
		{
			CString error;
			error.Format( _T("Reference block \"%s\" could not be created, Invalid name for signal block!" ), def.name );
			CCustomMessageBox::Show( error,_T("Error: Faild to create Reference block!"), "OK" );
			return;
		}
		if (def.tree && strlen(def.treename) == 0)
		{
			CString error;
			error.Format( _T("Reference block \"%s\" could not be created, Invalid name for tree block!" ), def.name );
			CCustomMessageBox::Show( error,_T("Error: Faild to create Reference block!"), "OK" );
			return;
		}
		if (!m_pManager->AddNewRef( def ))
		{
			CString error;
			error.Format( _T("Reference block \"%s\" could not be created, block with same name alredy exist!" ), def.name );
			CCustomMessageBox::Show( error,_T("Error: Faild to create Reference block!"), "OK" );
		}
	}
}

void CSelectionTreeModifier::EditTree(const char* tree)
{
	CAddNewTreeDialog newDlg;

	SSelectionTreeInfo info;
	if (m_pManager->GetTreeInfoByName(info, tree))
	{
		newDlg.Init( "Edit Selection Tree", info.Name, info.TreeType );
		if(newDlg.DoModal() == IDOK)
		{
			CSelectionTreeManager::STreeDefinition def;
			def.name = newDlg.GetName().c_str();
			def.type = newDlg.GetType().c_str();
			string newFile = string().Format("Scripts\\AI\\SelectionTrees\\%s.xml", def.name);
			def.filename = newFile.c_str();
			if (strlen(def.name) == 0)
			{
				CString error;
				error.Format( _T("Tree \"%s\" could not edited, invalid name!" ), def.name );
				CCustomMessageBox::Show( error,_T("Error: Faild to edit new SelectionTree!"), "OK" );
				return;
			}
			if (!m_pManager->EditTree( info.Name, def ))
			{
				CString error;
				error.Format( _T("Tree \"%s\" could not be renamed, tree with same name alredy exist!" ), def.name );
				CCustomMessageBox::Show( error,_T("Error: Faild to rename SelectionTree!"), "OK" );
			}
		}
		return;
	}
	assert(false);
}

void CSelectionTreeModifier::EditBlock(const char* refGroup)
{
	CAddNewBlockGroupDialog newDlg;

	SSelectionTreeInfo info;
	if (m_pManager->GetRefGroupInfoByName(info, refGroup))
	{
		SSelectionTreeBlockInfo varsInfo;
		SSelectionTreeBlockInfo signalsInfo;
		SSelectionTreeBlockInfo treeInfo;
		const bool hasVars = info.GetBlockById(varsInfo, eSTTI_Variables);
		const bool hasSignals = info.GetBlockById(signalsInfo, eSTTI_Signals);
		const bool hasTree = info.GetBlockById(treeInfo, eSTTI_Tree);
		newDlg.Init( "Edit Reference Block", info.Name, hasVars, hasSignals, hasTree, varsInfo.Name, signalsInfo.Name, treeInfo.Name );
		if(newDlg.DoModal() == IDOK)
		{
			CSelectionTreeManager::SRefGroupDefinition def;
			string groupName = newDlg.GetName();
			string varsName = newDlg.GetVarsName();
			string signalsName = newDlg.GetSignalsName();
			string treeName = newDlg.GetTreeName();

			def.name = newDlg.GetName().c_str();
			if (newDlg.IsAdvanced())
			{
				def.vars = newDlg.HasVars();
				def.signals = newDlg.HasSignals();
				def.tree = newDlg.HasTree();
			}
			else
			{
				varsName = groupName + "Vars";
				signalsName = groupName + "Signals";
				treeName = groupName;
				def.vars = true;
				def.signals = true;
				def.tree = true;
			}
			string newFile = string().Format("Scripts\\AI\\SelectionTrees\\References\\%s.xml", groupName.c_str());

			def.name = groupName.c_str();
			def.varsname = varsName.c_str();
			def.signalsname = signalsName.c_str();
			def.treename = treeName.c_str();
			def.filename = newFile.c_str();

			if (!(def.vars || def.signals || def.tree))
			{
				CString error;
				error.Format( _T("Reference block \"%s\" could not be edited, need at least one block!" ), def.name );
				CCustomMessageBox::Show( error,_T("Error: Faild to edit Reference block!"), "OK" );
				return;
			}
			if (def.vars && strlen(def.varsname) == 0)
			{
				CString error;
				error.Format( _T("Reference block \"%s\" could not be edited, Invalid name for variable block!" ), def.name );
				CCustomMessageBox::Show( error,_T("Error: Faild to edit Reference block!"), "OK" );
				return;
			}
			if (def.signals && strlen(def.signalsname) == 0)
			{
				CString error;
				error.Format( _T("Reference block \"%s\" could not be edited, Invalid name for signal block!" ), def.name );
				CCustomMessageBox::Show( error,_T("Error: Faild to edit Reference block!"), "OK" );
				return;
			}
			if (def.tree && strlen(def.treename) == 0)
			{
				CString error;
				error.Format( _T("Reference block \"%s\" could not be edited, Invalid name for tree block!" ), def.name );
				CCustomMessageBox::Show( error,_T("Error: Faild to edit Reference block!"), "OK" );
				return;
			}
			if (!m_pManager->EditRef( refGroup, def ))
			{
				CString error;
				error.Format( _T("Reference block \"%s\" could not be edited, block with same name alredy exist!" ), def.name );
				CCustomMessageBox::Show( error,_T("Error: Faild to edit Reference block!"), "OK" );
			}
		}
		return;
	}
	assert(false);
}

void CSelectionTreeModifier::RenameTreeFromBlock(const char* tree, const char* group)
{
	CBSTStringDlg dlg ( _T("Reference Tree name") );
	dlg.SetString( tree );
	if ( dlg.DoModal() == IDOK )
	{
		string newName = dlg.GetString().GetString();
		if (newName.length() == 0)
		{
			CString error;
			error.Format( _T("Reference tree could not be edited, invalid name!" ), newName.c_str() );
			CCustomMessageBox::Show( error,_T("Error: Faild to edit Reference tree!"), "OK" );
			return;
		}
		if (!m_pManager->RenameTreeForBlock(tree, newName.c_str(), group))
		{
			CString error;
			error.Format( _T("Reference tree \"%s\" could not be edited, block with same name alredy exist!" ), newName.c_str() );
			CCustomMessageBox::Show( error,_T("Error: Faild to edit Reference tree!"), "OK" );
		}
	}
}

void CSelectionTreeModifier::AddTreeToBlock(const char* group)
{
	CBSTStringDlg dlg ( _T("Reference Tree name") );
	dlg.SetString( string().Format("%sNewTree", group).c_str() );
	if ( dlg.DoModal() == IDOK )
	{
		string newName = dlg.GetString().GetString();
		if (newName.length() == 0)
		{
			CString error;
			error.Format( _T("Reference tree could not be added, invalid name!" ), newName.c_str() );
			CCustomMessageBox::Show( error,_T("Error: Faild to create Reference tree!"), "OK" );
			return;
		}
		if (!m_pManager->CreateTreeForBlock(newName.c_str(), group))
		{
			CString error;
			error.Format( _T("Reference tree \"%s\" could not be added, block with same name alredy exist!" ), newName.c_str() );
			CCustomMessageBox::Show( error,_T("Error: Faild to add Reference tree!"), "OK" );
		}
	}
}

bool CSelectionTreeModifier::OnReferenceAdded(const SSelectionTreeInfo& groupInfo, ESelectionTreeTypeId type, const char* refName)
{
	SSelectionTreeInfo info;
	int index;
	m_pManager->GetCurrentTreeInfo(info, &index);
	bool res = true;

	SSelectionTreeBlockInfo treeInfo;
	if (info.GetBlockById(treeInfo, eSTTI_Tree, index))
	{
		SSelectionTreeBlockInfo sigInfo;
		SSelectionTreeBlockInfo varInfo;

		CSelectionTreeSignalsView* pSigView = GetIEditor()->GetSelectionTreeManager()->GetSigView();
		CSelectionTreeVariablesView* pVarView = GetIEditor()->GetSelectionTreeManager()->GetVarView();

		switch (type)
		{
		case eSTTI_Tree:
			if (groupInfo.GetBlockById(sigInfo, eSTTI_Signals) && pSigView)
			{
				if (pSigView->ShouldAddRef(sigInfo.Name))
					res &= pSigView->AddNewRefEx(sigInfo.Name, false);
			}
		case eSTTI_Signals:
			if (groupInfo.GetBlockById(varInfo, eSTTI_Variables) && pVarView)
			{
				if (pVarView->ShouldAddRef(varInfo.Name))
					res &= pVarView->AddNewRefEx(varInfo.Name, false);
			}
		}
		return res;
	}
	assert(false);
	return false;
}

void CSelectionTreeModifier::OnReferenceRemoved(const SSelectionTreeInfo& groupInfo, ESelectionTreeTypeId type, const char* refName)
{
// 	SSelectionTreeInfo info;
// 	int index;
// 	m_pManager->GetCurrentTreeInfo(info, &index);
// 
// 	SSelectionTreeBlockInfo treeInfo;
// 	if (info.GetBlockById(treeInfo, eSTTI_Tree, index))
// 	{
// 		CryLogAlways("Reference %s removed from tree %s", refName, treeInfo.Name);
// 		CryLogAlways("  Type: %s", m_pManager->GetBlockStrByType(type));
// 		CryLogAlways("  Group: %s", groupInfo.Name);
// 		return;
// 	}
// 	assert(false);
}
