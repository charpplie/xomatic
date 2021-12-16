#pragma once
////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File name:   BaseBrush.h
//  Created:     14/9/2011 by Jaesik.
//  Description: The base brush class
////////////////////////////////////////////////////////////////////////////

class CBrushDesigner;

class CBaseBrush : public CRefCountBase
{
public:

	enum EBaseBrushFlag
	{
		eBaseBrushFlag_CastShadow = BIT(1),
		eBaseBrushFlag_Physicalize = BIT(2),
		eBaseBrushFlag_General = eBaseBrushFlag_CastShadow|eBaseBrushFlag_Physicalize
	};
	CBaseBrush( int nBaseBrushFlag );
	CBaseBrush( const CBaseBrush& brush );
	virtual ~CBaseBrush();

	bool IsValid() const;
	
	void Update( CBaseObject* pBaseObject, CBrushDesigner* pDesigner, BUtil::ShelfID shelfID = -1, bool bUpdateOnlyRenderNode = false );

	bool HitTest( CBaseObject* pBaseObject, CBrushDesigner* pDesigner, HitContext &hit ) const;

	void DisplayTriangulation( CBaseObject* pBaseObject, CBrushDesigner* pDesigner, DisplayContext& dc );
	void DeleteRenderAllNodes();
	void DeleteRenderNode( BUtil::ShelfID shelfID );
	IRenderNode* GetRenderNode(){ return m_pRenderNode[0]; }
	bool GetIStatObj( _smart_ptr<IStatObj>* pStatObj );

	bool GenerateIndexMesh(CBrushDesigner* pDesigner, IIndexedMesh *pMesh, bool bGenerateBackFaces);

	static void OptimizeMesh(IIndexedMesh* pMesh);

	void SaveToCgf( const char* filename );

	void PivotToCenter( CBaseObject* pObject, CBrushDesigner* pDesigner );
	void PivotToPos( CBaseObject* pObject, CBrushDesigner* pDesigner, const BrushVec3& vPivot );
	void ResetXForm( CBaseObject* pBaseObject, CBrushDesigner* pDesigner, int nResetFlag = BUtil::eResetXForm_All );

	void SetViewDistRatio( int nViewDistRatio ) { m_viewDistRatio = nViewDistRatio; }
	int GetViewDistRatio() const { return m_viewDistRatio; }

	void SetRenderFlags( int nRenderFlag );
	int GetRenderFlags() const { return m_RenderFlags; }

	void SetStaticObjFlags( int nStaticObjFlag );
	int GetStaticObjFlags() const;

	void AddFlags( int nFlags ) { m_nBrushFlag |= nFlags; }
	void RemoveFlags( int nFlags ) { m_nBrushFlag &= (~nFlags); }
	bool CheckFlags( int nFlags ) const { return (m_nBrushFlag & nFlags) ? true : false; }

	void SaveMesh( CArchive& ar, CBaseObject* pObj, CBrushDesigner* pDesigner );
	bool LoadMesh( CArchive& ar, CBaseObject* pObj, CBrushDesigner* pDesigner );

	bool SaveMesh( int nVersion, std::vector<char>& buffer, CBaseObject* pObj, CBrushDesigner* pDesigner );
	bool LoadMesh( int nVersion, std::vector<char>& buffer, CBaseObject* pObj, CBrushDesigner* pDesigner );

	int GetPolygonCount() const;

private:

	bool UpdateMesh( CBaseObject* pBaseObject, CBrushDesigner* pDesigner );
	void UpdateRenderNode( CBaseObject* pBaseObject, CBrushDesigner* pDesigner );

	void RemoveStatObj();
	void CreateStatObj( int nShelf ) const;
	static IMaterial* GetMaterialFromBaseObj( CBaseObject* pObj );
	void InvalidateStatObj( IStatObj* pStatObj, bool bPhysics );	

	void OutputMeshInfo( IIndexedMesh* pMesh );

private:

	mutable IStatObj* m_pStatObj[BUtil::kMaxShelfCount];
	mutable IRenderNode* m_pRenderNode[BUtil::kMaxShelfCount];	

	int m_RenderFlags;
	int m_viewDistRatio;
	int m_nBrushFlag;
};