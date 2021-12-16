//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2010.
// -------------------------------------------------------------------------
//  File Name        : ToolBox.cpp
//  Author           : Jaewon Jung
//  Time of creation : 6/30/2010   15:52
//  Compilers        : VS2008
//  Description      : ToolBox Macro System 
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "ToolBox.h"

//////////////////////////////////////////////////////////////////////////
// CToolBoxCommand
//////////////////////////////////////////////////////////////////////////
void CToolBoxCommand::Save(XmlNodeRef commandNode) const
{
	commandNode->setAttr("type", (int)m_type);
	commandNode->setAttr("text", m_text);
	commandNode->setAttr("bVariableToggle", m_bVariableToggle);
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxCommand::Load(XmlNodeRef commandNode)
{
	int type = 0;
	commandNode->getAttr("type", type);
	m_type = CToolBoxCommand::EType(type);
	m_text = commandNode->getAttr("text");
	commandNode->getAttr("bVariableToggle", m_bVariableToggle);
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxCommand::Execute() const
{
	if(m_type == CToolBoxCommand::eT_EDITOR_COMMAND)
	{
		GetIEditor()->ExecuteCommand(m_text);
	}
	else if (m_type == CToolBoxCommand::eT_CONSOLE_COMMAND)
	{
		if (m_bVariableToggle)
		{
			// Toggle the variable.
			float val = GetIEditor()->GetConsoleVar(m_text);
			bool bOn = val != 0;
			GetIEditor()->SetConsoleVar(m_text, (bOn)?0:1);
		}
		else
		{
			GetIEditor()->GetSystem()->GetIConsole()->ExecuteString(m_text);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// CToolBoxMacro
//////////////////////////////////////////////////////////////////////////
void CToolBoxMacro::Save(XmlNodeRef macroNode) const
{
	for(size_t i=0; i<m_commands.size(); ++i)
	{
		XmlNodeRef commandNode = macroNode->newChild("command");
		m_commands[i]->Save(commandNode);
	}
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxMacro::Load(XmlNodeRef macroNode)
{
	for(int i=0; i<macroNode->getChildCount(); ++i)
	{
		XmlNodeRef commandNode = macroNode->getChild(i);
		m_commands.push_back(new CToolBoxCommand);
		m_commands[i]->Load(commandNode);
	}
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxMacro::AddCommand(CToolBoxCommand::EType type, const CString &command, bool bVariableToggle)
{
	CToolBoxCommand *pNewCommand = new CToolBoxCommand;
	pNewCommand->m_type = type;
	pNewCommand->m_text = command;
	pNewCommand->m_bVariableToggle = bVariableToggle;
	m_commands.push_back(pNewCommand);
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxMacro::Clear()
{
	for(size_t i=0; i<m_commands.size(); ++i)
		delete m_commands[i];

	m_commands.clear();
}

//////////////////////////////////////////////////////////////////////////
const CToolBoxCommand *CToolBoxMacro::GetCommandAt(int index) const
{
	assert(0 <= index && index < m_commands.size());

	return m_commands[index];
}

//////////////////////////////////////////////////////////////////////////
CToolBoxCommand *CToolBoxMacro::GetCommandAt(int index)
{
	assert(0 <= index && index < m_commands.size());

	return m_commands[index];
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxMacro::SwapCommand(int index1, int index2)
{
	assert(0 <= index1 && index1 < m_commands.size());
	assert(0 <= index2 && index2 < m_commands.size());
	std::swap(m_commands[index1], m_commands[index2]);
}

void CToolBoxMacro::RemoveCommand(int index)
{
	assert(0 <= index && index < m_commands.size());

	m_commands.erase(m_commands.begin()+index);
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxMacro::Execute() const
{
	for(size_t i=0; i<m_commands.size(); ++i)
		m_commands[i]->Execute();
}

//////////////////////////////////////////////////////////////////////////
// CToolBoxManager
//////////////////////////////////////////////////////////////////////////
int CToolBoxManager::GetMacroCount() const
{
	return int(m_macros.size());
}

//////////////////////////////////////////////////////////////////////////
const CToolBoxMacro *CToolBoxManager::GetMacro( int iIndex ) const
{
	assert(0 <= iIndex && iIndex < m_macros.size());

	return m_macros[iIndex];
}

//////////////////////////////////////////////////////////////////////////
CToolBoxMacro *CToolBoxManager::GetMacro( int iIndex )
{
	assert(0 <= iIndex && iIndex < m_macros.size());

	return m_macros[iIndex];
}

//////////////////////////////////////////////////////////////////////////
int CToolBoxManager::GetMacroIndex(const CString& title) const
{
	for(size_t i=0; i<m_macros.size(); ++i)
	{
		if(stricmp(m_macros[i]->GetTitle(), title) == 0)
			return int(i);
	}

	return -1;
}

//////////////////////////////////////////////////////////////////////////
CToolBoxMacro *CToolBoxManager::NewMacro(const CString& title)
{
	if( m_macros.size() > ID_TOOL_LAST-ID_TOOL1+1 )
		return NULL;

	for(size_t i=0; i<m_macros.size(); ++i)
	{
		if(stricmp(m_macros[i]->GetTitle(), title) == 0)
			return NULL;
	}

	CToolBoxMacro *pNewTool = new CToolBoxMacro(title);
	m_macros.push_back(pNewTool);
	return pNewTool;
}

//////////////////////////////////////////////////////////////////////////
bool CToolBoxManager::SetMacroTitle(int index, const CString& title)
{
	assert(0 <= index && index < m_macros.size());

	for(size_t i=0; i<m_macros.size(); ++i)
	{
		if(i==index)
			continue;

		if(stricmp(m_macros[i]->GetTitle(), title) == 0)
			return false;
	}

	m_macros[index]->m_title = title;
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::Load()
{
	Clear();

	CString path;
	GetSaveFilePath(path);
	XmlNodeRef toolBoxNode = XmlHelpers::LoadXmlFromFile(path);
	if(toolBoxNode == NULL)
		return;
	GetIEditor()->GetSettingsManager()->AddSettingsNode(toolBoxNode);
	for(int i = 0; i < toolBoxNode->getChildCount(); ++i)
	{
		XmlNodeRef macroNode = toolBoxNode->getChild(i);
		CString title = macroNode->getAttr("title");
		CString shortcutName = macroNode->getAttr("shortcut");
		CString iconPath = macroNode->getAttr("icon");
		m_macros.push_back(new CToolBoxMacro(title));
		m_macros[i]->Load(macroNode);
		m_macros[i]->SetShortcutName(shortcutName);
		m_macros[i]->SetIconPath(iconPath);
	}

	UpdateShortcutsAndIcons();
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::UpdateShortcutsAndIcons()
{
	/// Shortcuts
	CXTPShortcutManager* pShortcutMgr = ((CMainFrame*)AfxGetMainWnd())->XTPShortcutManager();
	if( pShortcutMgr == NULL )
		return;

	CToolBoxManager* pToolBoxMgr(GetIEditor()->GetToolBoxManager());
	if( pToolBoxMgr == NULL )
		return;

	CXTPShortcutManagerAccelTable* pAccelTable = pShortcutMgr->GetDefaultAccelerator();
	for( int i = 0; i < pAccelTable->GetCount();  )
	{
		CXTPShortcutManagerAccel* pAccel = pAccelTable->GetAt(i);
		if( pAccel->cmd >= ID_TOOL1 &&  pAccel->cmd <= ID_TOOL_LAST )
		{
			pAccelTable->RemoveAt(i);			
			continue;
		}
		++i;
	} 

	for( int i = 0; i < pToolBoxMgr->GetMacroCount(); ++i )
	{
		CToolBoxMacro* pMacro = pToolBoxMgr->GetMacro(i);
		CString shortcutName(pMacro->GetShortcutName());
		if( shortcutName.IsEmpty() )
			continue;

		if( !CToolBoxManager::AddShortcut( ID_TOOL1+i, shortcutName ) )
			pMacro->SetShortcutName("");
	}

	/// Icons
	for( int i = 0; i < pToolBoxMgr->GetMacroCount(); ++i )
	{
		pToolBoxMgr->UpdateMacroIcon(i);
	}
}


//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::Save() const
{
	XmlNodeRef toolBoxNode = XmlHelpers::CreateXmlNode(TOOLBOXMACROS_NODE);
	for(size_t i = 0; i < m_macros.size(); ++i)
	{
		XmlNodeRef macroNode = toolBoxNode->newChild("macro");
		macroNode->setAttr( "title", m_macros[i]->GetTitle() );
		macroNode->setAttr( "shortcut", m_macros[i]->GetShortcutName() );
		macroNode->setAttr( "icon", m_macros[i]->GetIconPath() );
		m_macros[i]->Save(macroNode);
	}
	CString path;
	GetSaveFilePath(path);
	XmlHelpers::SaveXmlNode(toolBoxNode, path);
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::Clear()
{
	for(size_t i=0; i<m_macros.size(); ++i)
	{
		RemoveMarcoShortcut(i);
		delete m_macros[i];
	}
	m_macros.clear();
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::ExecuteMacro( int iIndex ) const
{
	assert(0 <= iIndex && iIndex < m_macros.size());
	if (iIndex < 0 || iIndex >= m_macros.size())
		return;

	m_macros[iIndex]->Execute();
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::ExecuteMacro( const CString &name ) const
{
	// Find tool with this name.
	for (size_t i = 0; i < m_macros.size(); ++i)
	{
		if (stricmp(m_macros[i]->GetTitle(), name) == 0)
		{
			ExecuteMacro(int(i));
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::SwapMacro(int index1, int index2)
{
	assert(0 <= index1 && index1 < m_macros.size());
	assert(0 <= index2 && index2 < m_macros.size());
	std::swap(m_macros[index1], m_macros[index2]);
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::RemoveMacro(int index)
{
	assert(0 <= index && index < m_macros.size());

	RemoveMarcoShortcut(index);
	m_macros.erase(m_macros.begin()+index);
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::RemoveMarcoShortcut( int index )
{
	if( index >= m_macros.size() )
		return;

	CXTPShortcutManager* pShortcutMgr = ((CMainFrame*)AfxGetMainWnd())->XTPShortcutManager();
	if( pShortcutMgr == NULL )
		return;

	CXTPShortcutManagerAccelTable* pAccelTable = pShortcutMgr->GetDefaultAccelerator();
	if( pAccelTable == NULL )
		return;

	pAccelTable->RemoveAt(ID_TOOL1+index);
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::GetSaveFilePath(CString& outPath) const
{
	outPath = Path::GetUserSandboxFolder();
	outPath += "Macros.xml";
}

//////////////////////////////////////////////////////////////////////////
bool CToolBoxManager::AddShortcut( CXTPShortcutManagerAccel& accel )
{
	CXTPShortcutManager* pShortcutMgr = ((CMainFrame*)AfxGetMainWnd())->XTPShortcutManager();
	if( pShortcutMgr == NULL )
		return false;

	CString shortcutName(pShortcutMgr->Format( &accel, NULL ));
	bool isPossible = IsPossibleToAddShortcut(shortcutName);

	if( isPossible )
	{
		CXTPShortcutManagerAccelTable* pAccelTable = pShortcutMgr->GetDefaultAccelerator();
		pAccelTable->Add(accel);
	}

	return isPossible;
}

//////////////////////////////////////////////////////////////////////////
bool CToolBoxManager::AddShortcut( int cmdID, const CString& shortcutName )
{
	CXTPShortcutManager* pShortcutMgr = ((CMainFrame*)AfxGetMainWnd())->XTPShortcutManager();
	if( pShortcutMgr == NULL )
		return false;
	
	bool isPossible = IsPossibleToAddShortcut(shortcutName);

	if( isPossible )
		pShortcutMgr->AddShortcut( cmdID, shortcutName );

	return isPossible;
}

//////////////////////////////////////////////////////////////////////////
bool CToolBoxManager::IsPossibleToAddShortcut( const CString& shortcutName )
{
	CXTPShortcutManager* pShortcutMgr = ((CMainFrame*)AfxGetMainWnd())->XTPShortcutManager();
	if( pShortcutMgr == NULL )
		return false;
	
	CXTPShortcutManagerAccel accel;
	return pShortcutMgr->ParseShortcut(shortcutName, &accel);
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::UpdateMacroIcon(int macroIndex)
{
	CXTPCommandBars *pCmdBars = static_cast<CMainFrame*>(AfxGetMainWnd())->GetCommandBars();
	CToolBoxMacro* pMacro = GetMacro(macroIndex);
	if( pMacro == NULL )
		return;
	CString iconPath(pMacro->GetIconPath());
	if( iconPath.IsEmpty() )
		pCmdBars->GetImageManager()->RemoveIcon(ID_TOOL1+macroIndex);
	else
		GetIEditor()->GetIconManager()->RegisterCommandIcon(iconPath, ID_TOOL1+macroIndex);

	// Refresh the torn-off toolbar, if exists.
	CXTPToolBar *pToolBar = pCmdBars->GetToolBar(ID_TOOLS_TOOL1);
	if( pToolBar )
	{
		CXTPControl *pControl = pToolBar->GetControl(macroIndex);
		if( pControl )
			pControl->SetCaption(pMacro->GetTitle());
		pToolBar->Invalidate();
	}
}