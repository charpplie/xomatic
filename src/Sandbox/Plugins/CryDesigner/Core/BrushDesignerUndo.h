#pragma once
////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2012
// -------------------------------------------------------------------------
//  File name:   BrushDesignerUndo.h
//  Created:     Mar/25/2012 by Jaesik
//  Description: 
//
////////////////////////////////////////////////////////////////////////////
#include "BrushDesigner.h"
#include "Tools/BrushDesignerSelectTool.h"

class CBrushDesigner;

class CUndoDesigner : public IUndoObject
{
public:
	CUndoDesigner(){}
	CUndoDesigner( CBaseObject* pObj, const CBrushDesigner* pDesigner, const char *undoDescription = NULL );
	virtual ~CUndoDesigner()
	{
	}

	static bool IsAKindOfDesignerTool( CEditTool * pEditTool );
	static CBrushDesigner* GetDesigner( const GUID& objGUID );
	static CBaseObject* GetBaseObject( const GUID& objGUID );
	static void RestoreEditTool( CBrushDesigner* pDesigner, REFGUID objGUID, BUtil::EDesignerMode designerMode );
	static CBrushDesignerEditTool* GetEditTool();

protected:
	int GetSize()
	{
		return sizeof(*this);
	}

	const char* GetDescription()
	{ 
		return m_undoDescription;
	};
	
	void SetDescription( const char* description )
	{
		if( description )
			m_undoDescription = description;
	}

	CBrushDesigner* GetDesigner()
	{
		return GetDesigner(m_ObjGUID);
	}

	void Sync();

	void RestoreEditTool( CBrushDesigner* pDesigner )
	{
		RestoreEditTool( pDesigner, m_ObjGUID, m_DesignerMode );
	}

	virtual void Undo( bool bUndo );
	virtual void Redo();

	void StoreEditorTool();
	void SetDesignerMode( BUtil::EDesignerMode designerMode ) { m_DesignerMode = designerMode; }

	void SetObjGUID( REFGUID guid ){
		m_ObjGUID = guid;
	}

	void UpdateBrush();	

private:	
	CString m_undoDescription; 
	GUID m_ObjGUID;
	BUtil::EDesignerMode m_DesignerMode;
	_smart_ptr<CBrushDesigner> m_undo;
	Matrix34 m_UndoWorldTM;
	_smart_ptr<CBrushDesigner> m_redo;
	Matrix34 m_RedoWorldTM;
};

class CUndoDesigneSelection : public CUndoDesigner
{
public:

	CUndoDesigneSelection( CBrushDesignerElementManager& selectionContext, CBaseObject* pObj, const char *undoDescription = NULL );
	~CUndoDesigneSelection(){}

	int GetSize(){return sizeof(*this);}

	CBrushDesignerSelectTool* GetSelectToolHandler();
	void Undo( bool bUndo );
	void Redo();

private:

	void CopyElements( CBrushDesignerElementManager& sourceElements, CBrushDesignerElementManager& destElements );
	void ReplaceRegionsWithExistingRegionsInDesigner( CBrushDesignerElementManager& elements );

	CBrushDesignerElementManager m_SelectionContextForUndo;
	CBrushDesignerElementManager m_SelectionContextForRedo;

};

class CUndoDesignerTextureMapping : public IUndoObject
{
public:
	CUndoDesignerTextureMapping(){}
	CUndoDesignerTextureMapping( CBaseObject* pObj, const char *undoDescription = NULL ) : m_ObjGUID(pObj->GetId())
	{
		SetDescription(undoDescription);
		SaveDesignerTexInfoContext(m_UndoContext);
	}
	virtual ~CUndoDesignerTextureMapping()
	{
	}

protected:
	int GetSize()
	{
		return sizeof(*this);
	}

	const char* GetDescription()
	{ 
		return m_undoDescription;
	};

	void SetDescription( const char* description )
	{
		if( description )
			m_undoDescription = description;
	}

	void Undo( bool bUndo );
	void Redo();

	void SetObjGUID( REFGUID guid ){ m_ObjGUID = guid; }

private:

	struct SContextInfo
	{
		GUID m_RegionGUID;
		BUtil::STexInfo m_TexInfo;
		int m_MatID;
	};

	void RestoreTexInfo( const std::vector<SContextInfo>& contextList );
	void SaveDesignerTexInfoContext( std::vector<SContextInfo>& contextList );

	CString m_undoDescription;
	GUID m_ObjGUID;

	std::vector<SContextInfo> m_UndoContext;
	std::vector<SContextInfo> m_RedoContext;
};
