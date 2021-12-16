#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File name:   LensFlareUndo.h
//  Created:     12/Dec/2012 by Jaesik.
////////////////////////////////////////////////////////////////////////////

class CLensFlareItem;

class CUndoLensFlareItem : public IUndoObject
{
public:
	CUndoLensFlareItem( CLensFlareItem* pGroupItem, const CString& undoDescription = "Undo Lens Flare Tree" );
	~CUndoLensFlareItem();

protected:
	int GetSize()
	{
		return sizeof(*this);
	}
	const char* GetDescription() { return m_undoDescription; };
	void Undo( bool bUndo );
	void Redo();

private:	

	CString m_undoDescription;

	struct SData
	{
		SData()
		{
			m_pOptics = NULL;
		}

		CString m_selectedFlareItemName;
		bool m_bRestoreSelectInfo;
		IOpticsElementBasePtr m_pOptics;
	};

	CString m_flarePathName;
	SData m_Undo;
	SData m_Redo;

	void Restore( const SData& data );
};

class CUndoRenameLensFlareItem : public IUndoObject
{
public:	

	CUndoRenameLensFlareItem( const CString& oldFullName, const CString& newFullName, bool bRefreshItemTreeWhenUndo=false, bool bRefreshItemTreeWhenRedo=false);

protected:

	int GetSize()
	{
		return sizeof(*this);
	}
	const char* GetDescription() { return m_undoDescription; };
	void Undo( bool bUndo );
	void Redo();

private:

	CString m_undoDescription;

	struct SUndoDataStruct
	{
		CString m_oldFullItemName;
		CString m_newFullItemName;
		bool m_bRefreshItemTreeWhenUndo;
		bool m_bRefreshItemTreeWhenRedo;
	};

	void Rename( const SUndoDataStruct& data, bool bRefreshItemTree );

	SUndoDataStruct m_undo;
	SUndoDataStruct m_redo;
};

class CUndoLensFlareElementSelection : public IUndoObject
{
public:
	CUndoLensFlareElementSelection( CLensFlareItem* pLensFlareItem, const CString& flareTreeItemFullName, const CString& undoDescription = "Undo Lens Flare Element Tree" );
	~CUndoLensFlareElementSelection(){}

protected:
	int GetSize()
	{
		return sizeof(*this);
	}
	const char* GetDescription() { return m_undoDescription; };
	void Undo( bool bUndo );
	void Redo();

private:

	CString m_undoDescription;

	CString m_flarePathNameForUndo;
	CString m_flareTreeItemFullNameForUndo;

	CString m_flarePathNameForRedo;
	CString m_flareTreeItemFullNameForRedo;
};

class CUndoLensFlareItemSelectionChange : public IUndoObject
{
public:
	CUndoLensFlareItemSelectionChange( const CString& fullLensFlareItemName, const CString& undoDescription = "Undo Lens Flare element selection");
	~CUndoLensFlareItemSelectionChange(){}

protected:
	int GetSize()
	{
		return sizeof(*this);
	}
	const char* GetDescription() { return m_undoDescription; };

	void Undo( bool bUndo );
	void Redo();

private:
	CString m_undoDescription;
	CString m_FullLensFlareItemNameForUndo;
	CString m_FullLensFlareItemNameForRedo;
};