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
#include "Util/BoostPythonHelpers.h"
#include "IconManager.h"

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
	if(m_type == CToolBoxCommand::eT_SCRIPT_COMMAND)
	{
		PyScript::Execute(m_text);
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
int CToolBoxManager::GetMacroCount(bool bToolbox) const
{
	if (bToolbox)
		return int(m_macros.size());
	return int(m_shelveMacros.size());
}

//////////////////////////////////////////////////////////////////////////
const CToolBoxMacro *CToolBoxManager::GetMacro( int iIndex , bool bToolbox) const
{
	if (bToolbox)
	{
		assert(0 <= iIndex && iIndex < m_macros.size());
		return m_macros[iIndex];
	}
	else
	{
		assert(0 <= iIndex && iIndex < m_shelveMacros.size());
		return m_shelveMacros[iIndex];
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////
CToolBoxMacro *CToolBoxManager::GetMacro( int iIndex , bool bToolbox)
{
	if (bToolbox)
	{
		assert(0 <= iIndex && iIndex < m_macros.size());
		return m_macros[iIndex];
	}
	else
	{
		assert(0 <= iIndex && iIndex < m_shelveMacros.size());
		return m_shelveMacros[iIndex];
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////
int CToolBoxManager::GetMacroIndex(const CString& title, bool bToolbox) const
{
	if (bToolbox)
	{
		for(size_t i=0; i<m_macros.size(); ++i)
		{
			if(stricmp(m_macros[i]->GetTitle(), title) == 0)
				return int(i);
		}
	}
	else
	{
		for(size_t i=0; i<m_shelveMacros.size(); ++i)
		{
			if(stricmp(m_shelveMacros[i]->GetTitle(), title) == 0)
				return int(i);
		}
	}

	return -1;
}

//////////////////////////////////////////////////////////////////////////
CToolBoxMacro *CToolBoxManager::NewMacro(const CString& title, bool bToolbox, int * newIdx)
{
	if ( bToolbox )
	{
		const int macroCount = m_macros.size();
		if( macroCount > ID_TOOL_LAST-ID_TOOL_FIRST+1 )
			return NULL;

		for(size_t i=0; i<macroCount; ++i)
		{
			if(stricmp(m_macros[i]->GetTitle(), title) == 0)
				return NULL;
		}

		CToolBoxMacro *pNewTool = new CToolBoxMacro(title);
		if (newIdx)
			*newIdx = macroCount; 
		m_macros.push_back(pNewTool);
		return pNewTool;
	}
	else
	{
		const int shelveMacroCount = m_shelveMacros.size();
		if( shelveMacroCount > ID_TOOL_SHELVE_LAST-ID_TOOL_SHELVE_FIRST+1 )
			return NULL;

		CToolBoxMacro *pNewTool = new CToolBoxMacro(title);
		if (newIdx)
			*newIdx = shelveMacroCount;
		m_shelveMacros.push_back(pNewTool);
		return pNewTool;
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////
bool CToolBoxManager::SetMacroTitle(int index, const CString& title, bool bToolbox)
{
	if ( bToolbox )
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
	}
	else
	{
		assert(0 <= index && index < m_shelveMacros.size());
		for(size_t i=0; i<m_shelveMacros.size(); ++i)
		{
			if(i==index)
				continue;

			if(stricmp(m_shelveMacros[i]->GetTitle(), title) == 0)
				return false;
		}

		m_shelveMacros[index]->m_title = title;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
byte CToolBoxManager::HashName(CString name)
{
	const int len = name.GetLength();
	byte h = 0;
	for ( int idx = 0; idx < len; ++idx )
	{
		byte index = h ^ name[idx];
		h = index;
	}
	return h;
}

bool CToolBoxManager::CheckForCollision(int hash)
{
	for ( std::vector<CXTPToolBar*>::iterator item = m_toolbars.begin(), end = m_toolbars.end(); item != end; ++item )
	{
		if ( (*item)->GetBarID() == hash+ID_SHELF_RESERVED_FIRST )
			return true;
	}
	return false;
}

byte CToolBoxManager::GenerateHashId(CString hashName)
{
	bool collision = false;
	byte hash = 0;
	do 
	{
		hash = HashName(hashName);
		collision = CheckForCollision(hash);
		if ( collision )
			hashName.Append("@");
	} while (collision != false);
	return hash;
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::Load(CXTPCommandBars* pCommandBars)
{
	Clear();

	CString path;
	GetSaveFilePath(path);
	Load(path,NULL,true);

	if (pCommandBars)
	{
		XmlNodeRef envNode = XmlHelpers::LoadXmlFromFile(gSettings.strEditorEnv);
		if ( envNode )
		{
			int childrenCount = envNode->getChildCount();
			for(int idx = 0; idx < childrenCount; ++idx )
			{
				XmlNodeRef child = envNode->getChild(idx);
				if ( child->haveAttr("scriptPath") && child->haveAttr("shelvesPath") )
				{
					LoadShelves(child->getAttr("scriptPath"),child->getAttr("shelvesPath"),pCommandBars);
				}
			}
		}
	}

	UpdateShortcutsAndIcons();
}

void CToolBoxManager::LoadShelves(CString scriptPath, CString shelvesPath, CXTPCommandBars* pCommandBars)
{
	GetIEditor()->ExecuteCommand("general.run_file_parameters 'addToSysPath.py' '%s'", scriptPath);	
	CFileUtil::FileArray files;
	CFileUtil::ScanDirectory(shelvesPath,"*.xml",files);

	string shelfName;
	const int shelfCount = files.size();
	for ( int idx = 0; idx < shelfCount; ++idx )
	{
		if ( Path::GetExt(files[idx].filename) != "xml" )
			continue;

		shelfName = PathUtil::GetFileName(string(files[idx].filename));
		int hash = GenerateHashId(CString(shelfName.c_str()));

		CXTPToolBar * pToolbar = GenerateShelf(pCommandBars, shelfName.c_str(), hash);
		if ( !pToolbar )
			continue;

		m_toolbars.push_back(pToolbar);

		Load(shelvesPath+CString("/")+files[idx].filename, pToolbar, false);
	}
}

void CToolBoxManager::Load(CString xmlpath, CXTPToolBar * pToolbar, bool bToolbox)
{
	XmlNodeRef toolBoxNode = XmlHelpers::LoadXmlFromFile(xmlpath);
	if(toolBoxNode == NULL)
		return;

	if ( !pToolbar )
		GetIEditor()->GetSettingsManager()->AddSettingsNode(toolBoxNode);

	for(int i = 0; i < toolBoxNode->getChildCount(); ++i)
	{
		XmlNodeRef macroNode = toolBoxNode->getChild(i);
		CString title = macroNode->getAttr("title");
		CString shortcutName = macroNode->getAttr("shortcut");
		CString iconPath = macroNode->getAttr("icon");

		int idx = -1;
		CToolBoxMacro * pMacro = NewMacro(title,bToolbox,&idx);
		if(!pMacro || idx == -1)
			continue;

		pMacro->Load(macroNode);
		pMacro->SetShortcutName(shortcutName);
		pMacro->SetIconPath(iconPath);
		pMacro->SetToolbarId(-1);

		if ( !pToolbar )
			continue;

		string shelfPath = PathUtil::GetParentDirectory(string(xmlpath));
		string fullIconPath = PathUtil::AddSlash(shelfPath.c_str());
		fullIconPath.append(iconPath);

		pMacro->SetIconPath(fullIconPath);
		pMacro->SetToolbarId(pToolbar->GetBarID());

		CXTPControls * pControls = pToolbar->GetControls();
		if ( !pControls )
			continue;

		CXTPControlButton *pButton = (CXTPControlButton*)pControls->Add(new CXTPControlButton(),bToolbox ? ID_TOOL_FIRST+idx : ID_TOOL_SHELVE_FIRST+idx);
		if ( !pButton )
			continue;

		CString toolTip = macroNode->getAttr("tooltip");

		pButton->SetCaption(title);
		pButton->SetTooltip(toolTip);
		pButton->SetShortcutText(shortcutName);

		UpdateMacroIcon(idx,bToolbox);
	}
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
		if((pAccel->cmd >= ID_TOOL_FIRST && pAccel->cmd <= ID_TOOL_LAST) || (pAccel->cmd >= ID_TOOL_SHELVE_FIRST && pAccel->cmd <= ID_TOOL_SHELVE_LAST) )
		{
			pAccelTable->RemoveAt(i);			
			continue;
		}
		++i;
	} 


	const int macroCount = pToolBoxMgr->GetMacroCount(true);
	for( int i = 0; i < macroCount; ++i )
	{
		CToolBoxMacro* pMacro = pToolBoxMgr->GetMacro(i,true);
		if (!pMacro)
			continue;

		CString shortcutName(pMacro->GetShortcutName());
		if( shortcutName.IsEmpty() )
			continue;

		if( !CToolBoxManager::AddShortcut( ID_TOOL_FIRST+i, shortcutName ) )
			pMacro->SetShortcutName("");

		if( pMacro->GetToolbarId() == -1)
			pToolBoxMgr->UpdateMacroIcon(i,true);
	}

	const int shelveMacroCount = pToolBoxMgr->GetMacroCount(false);
	for( int i = 0; i < shelveMacroCount; ++i )
	{
		CToolBoxMacro* pMacro = pToolBoxMgr->GetMacro(i,false);
		if (!pMacro)
			continue;

		CString shortcutName(pMacro->GetShortcutName());
		if( shortcutName.IsEmpty() )
			continue;

		if( !CToolBoxManager::AddShortcut( ID_TOOL_SHELVE_FIRST+i, shortcutName ) )
			pMacro->SetShortcutName("");

		if( pMacro->GetToolbarId() == -1)
			pToolBoxMgr->UpdateMacroIcon(i,false);
	}
}


//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::Save() const
{
	XmlNodeRef toolBoxNode = XmlHelpers::CreateXmlNode(TOOLBOXMACROS_NODE);
	for(size_t i = 0; i < m_macros.size(); ++i)
	{
		if ( m_macros[i]->GetToolbarId() != -1 )
			continue;

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
		RemoveMacroShortcut(i,true);
		delete m_macros[i];
	}
	m_macros.clear();
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::ExecuteMacro( int iIndex, bool bToolbox ) const
{
	if ( iIndex >= 0 && iIndex < GetMacroCount(bToolbox) && GetMacro(iIndex,bToolbox))
		GetMacro(iIndex,bToolbox)->Execute();
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::ExecuteMacro( const CString &name , bool bToolbox) const
{
	if ( bToolbox )
	{
		// Find tool with this name.
		for (size_t i = 0; i < m_macros.size(); ++i)
		{
			if (stricmp(m_macros[i]->GetTitle(), name) == 0)
			{
				ExecuteMacro(int(i),bToolbox);
				break;
			}
		}
	}
	else
	{
		// Find tool with this name.
		for (size_t i = 0; i < m_shelveMacros.size(); ++i)
		{
			if (stricmp(m_shelveMacros[i]->GetTitle(), name) == 0)
			{
				ExecuteMacro(int(i),bToolbox);
				break;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::SwapMacro(int index1, int index2, bool bToolbox)
{
	assert(0 <= index1 && index1 < GetMacroCount(bToolbox));
	assert(0 <= index2 && index2 < GetMacroCount(bToolbox));
	if ( bToolbox )
	{
		std::swap(m_macros[index1], m_macros[index2]);
	}
	else
	{
		std::swap(m_shelveMacros[index1], m_shelveMacros[index2]);
	}
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::RemoveMacro(int index, bool bToolbox)
{
	assert(0 <= index && index < GetMacroCount(bToolbox));

	RemoveMacroShortcut(index,bToolbox);
	if (bToolbox)
	{
		m_macros.erase(m_macros.begin()+index);
	}
	else
	{
		m_shelveMacros.erase(m_shelveMacros.begin()+index);
	}
}

//////////////////////////////////////////////////////////////////////////
void CToolBoxManager::RemoveMacroShortcut( int index , bool bToolbox)
{
	if( index >= GetMacroCount(bToolbox) )
		return;

	CXTPShortcutManager* pShortcutMgr = ((CMainFrame*)AfxGetMainWnd())->XTPShortcutManager();
	if( pShortcutMgr == NULL )
		return;

	CXTPShortcutManagerAccelTable* pAccelTable = pShortcutMgr->GetDefaultAccelerator();
	if( pAccelTable == NULL )
		return;

	pAccelTable->RemoveAt(bToolbox?ID_TOOL_FIRST+index:ID_TOOL_SHELVE_FIRST+index);
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
void CToolBoxManager::UpdateMacroIcon(int macroIndex, bool bToolbox)
{
	CXTPCommandBars *pCmdBars = static_cast<CMainFrame*>(AfxGetMainWnd())->GetCommandBars();
	CToolBoxMacro* pMacro = GetMacro(macroIndex,bToolbox);
	if( pMacro == NULL )
		return;

	int nToolBarID =  pMacro->GetToolbarId();
	if ( nToolBarID == -1 )
		nToolBarID = ID_TOOLS_TOOL1;

	// Refresh the torn-off toolbar, if exists.
	CXTPToolBar *pToolBar = pCmdBars->GetToolBar(nToolBarID);
	if( pToolBar )
	{
		CXTPControl *pControl = pToolBar->GetControl(macroIndex);
		if( pControl )
		{
			pControl->SetCaption(pMacro->GetTitle());
		}
		pToolBar->Invalidate();
	}

	CString iconPath(pMacro->GetIconPath());
	if( iconPath.IsEmpty() )
	{
		pCmdBars->GetImageManager()->RemoveIcon(bToolbox ? ID_TOOL_FIRST+macroIndex : ID_TOOL_SHELVE_FIRST+macroIndex);
	}
	else
	{
		if ( Path::GetExt(iconPath) == "ico" )
		{
			pCmdBars->GetImageManager()->SetIconFromIcoFile(iconPath,bToolbox ? ID_TOOL_FIRST+macroIndex : ID_TOOL_SHELVE_FIRST+macroIndex,CSize(0,0),xtpImageNormal);
		}
		else
		{
			GetIEditor()->GetIconManager()->RegisterCommandIcon(iconPath, bToolbox ? ID_TOOL_FIRST+macroIndex : ID_TOOL_SHELVE_FIRST+macroIndex);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CXTPToolBar * CToolBoxManager::GenerateShelf(CXTPCommandBars * pCmdBars, CString shelfName, int idx)
{
	int iD = ID_SHELF_RESERVED_FIRST+idx;
	if ( iD > ID_SHELF_RESERVED_LAST )
	{
		CryLogAlways("Ran out of available Shelve IDs, no more shelves can be added.");
		return NULL;
	}

	CXTPToolBar *pToolBar = pCmdBars->Add(shelfName, xtpBarTop);
	if ( !pToolBar )
		return NULL;

	pToolBar->SetTemporary(TRUE);
	pToolBar->SetBarID(iD);
	pToolBar->SetVisible(FALSE);
	return pToolBar;
}
