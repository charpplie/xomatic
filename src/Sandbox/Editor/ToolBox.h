//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2010.
// -------------------------------------------------------------------------
//  File Name        : ToolBox.h
//  Author           : Jaewon Jung
//  Time of creation : 6/30/2010   15:51
//  Compilers        : VS2008
//  Description      : ToolBox Macro System 
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////
#ifndef __TOOLBOX_H__
#define __TOOLBOX_H__
#pragma once

#define TOOLBOXMACROS_NODE "toolboxmacros"
#define INVALID_TOOLBAR_ID -1

class CToolBoxManager;

	/** Represents a single ToolBox command.
*/
class CToolBoxCommand
{
public:
	enum EType
	{
		eT_INVALID_COMMAND = 0,
		eT_SCRIPT_COMMAND,
		eT_CONSOLE_COMMAND,
	};

	CToolBoxCommand()
		: m_bVariableToggle(false), m_type(eT_INVALID_COMMAND)
	{
	}

	void Save(XmlNodeRef commandNode) const;
	void Load(XmlNodeRef commandNode);

	void Execute() const;

	CString m_text;
	
	bool m_bVariableToggle;
	EType m_type;
};

/** Represents a sequence of ToolBox commands.
*/
class CToolBoxMacro
{
	friend class CToolBoxManager;
public:
	CToolBoxMacro(const CString& title) : m_title(title), m_toolbarId(INVALID_TOOLBAR_ID) {}
	~CToolBoxMacro()
	{ Clear(); }

	void Save(XmlNodeRef macroNode) const;
	void Load(XmlNodeRef macroNode);

	void AddCommand(CToolBoxCommand::EType type, const CString &command, bool bVariableToggle = false);
	void Clear();
	const CString& GetTitle() const
	{ return m_title; }
	int GetCommandCount() const
	{ return int(m_commands.size()); }
	const CToolBoxCommand *GetCommandAt(int index) const;
	CToolBoxCommand *GetCommandAt(int index);
	void SwapCommand(int index1, int index2);
	void RemoveCommand(int index);

	void Execute() const;

	void SetShortcutName( const char* name )
	{	m_shortcutName = name;	}
	const CString& GetShortcutName()	const
	{	return m_shortcutName;			}

	void SetIconPath(const char *path)
	{ m_iconPath = path; }
	const CString& GetIconPath() const
	{ return m_iconPath; }
	
	void SetToolbarId(int nID)
	{ m_toolbarId = nID; }
	int GetToolbarId()
	{ return m_toolbarId; }

private:
	std::vector<CToolBoxCommand*> m_commands;
	CString m_title;
	CString m_shortcutName;
	CString m_iconPath;
	int m_toolbarId;
};

/** Manages user defined macros.
*/
class CToolBoxManager
{
public:
	~CToolBoxManager()
	{ Clear(); }

	// Save macros configuration to registry.
	void Save() const;
	// Load macros configuration from registry.
	void Load(CXTPCommandBars* pCommandBars = NULL);
	void LoadShelves(CString scriptPath, CString shelvesPath, CXTPCommandBars* pCommandBars);
	void Load(CString xmlpath, CXTPToolBar * pToolbar, bool bToolbox);

	//! Get the number of managed macros.
	int GetMacroCount(bool bToolbox) const;
	//! Get a macro by index.
	const CToolBoxMacro *GetMacro( int iIndex, bool bToolbox) const;
	CToolBoxMacro *GetMacro( int iIndex, bool bToolbox);
	//! Get the index of a macro from its title.
	int GetMacroIndex(const CString& title, bool bToolbox) const;
	//! Creates a new macro in the manager. If the title is duplicate, this returns NULL. 
	CToolBoxMacro *NewMacro(const CString& title, bool bToolbox, int * newIdx);
	//! Try to change the title of a macro. If the title is duplicate, the change is aborted and this returns false. 
	bool SetMacroTitle(int index, const CString& title, bool bToolbox);
	//! Delete all macros.
	void Clear();

	void SwapMacro(int index1, int index2, bool bToolbox);
	void RemoveMacro(int index, bool bToolbox);

	//! Execute the macro with specified id.
	void ExecuteMacro( int iIndex, bool bToolbox) const;
	void ExecuteMacro( const CString& name, bool bToolbox ) const;

	void GetSaveFilePath(CString& path) const;

	void UpdateMacroIcon(int macroIndex, bool bToolbox);
	static bool AddShortcut( CXTPShortcutManagerAccel& accel );
	static bool AddShortcut( int cmdID, const CString& shortcutName );
	static void UpdateShortcutsAndIcons();

	CXTPToolBar * GenerateShelf(CXTPCommandBars * pCmdBars, CString shelfName, int idx);
	byte HashName(CString name);
	bool CheckForCollision(int hash);
	byte GenerateHashId(CString hashName);

private:
	
	static bool IsPossibleToAddShortcut( const CString& shortcutName );
	void RemoveMacroShortcut( int index, bool bToolbox );

	std::vector<CToolBoxMacro*> m_macros;
	std::vector<CToolBoxMacro*> m_shelveMacros;
	std::vector<CXTPToolBar*> m_toolbars;
};

#endif // __TOOLBOX_H__
