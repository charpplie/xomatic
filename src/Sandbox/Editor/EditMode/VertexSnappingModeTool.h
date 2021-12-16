#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2012 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   VertexSnappingMode.h
//  Created:     Sep/12/2012 by Jaesik.
////////////////////////////////////////////////////////////////////////////

class CKDTree;

class CVertexSnappingModeTool : public CEditTool
{
public:
	DECLARE_DYNCREATE(CVertexSnappingModeTool);

	CVertexSnappingModeTool();
	~CVertexSnappingModeTool();

	static void RegisterTool( CRegistrationContext &rc );

	void Display( DisplayContext &dc );
	bool MouseCallback( CViewport *view,EMouseEvent event,CPoint &point,int flags );

protected:

	void DrawVertexCubes( DisplayContext &dc, const Matrix34& tm, IStatObj* pStatObj );
	void DeleteThis(){ delete this; }
	Vec3 GetCubeSize( IDisplayViewport* pView, const Vec3& pos ) const;

private:

	bool HitTest( CViewport *view, const CPoint &point, CBaseObject* pExcludedObj, Vec3* outHitPos, CBaseObjectPtr& pHitObject, std::vector<CBaseObjectPtr>& outObjects );
	CKDTree* GetKDTree( CBaseObject* pObject );

	enum EVertexSnappingStatus
	{
		eVSS_SelectFirstVertex,
		eVSS_MoveSelectVertexToAnotherVertex
	};
	EVertexSnappingStatus m_modeStatus;	

	struct SSelectionInfo
	{
		SSelectionInfo()
		{
			m_pObject = NULL;
			m_vPos = Vec3(0,0,0);
		}
		CBaseObjectPtr m_pObject;
		Vec3 m_vPos;
	};
	SSelectionInfo m_SelectionInfo;	

	std::vector<CBaseObjectPtr> m_Objects;
	Vec3 m_vHitVertex;
	bool m_bHit;
	CBaseObjectPtr m_pHitObject;

	std::vector<AABB> m_DebugBoxes;
	std::map<CBaseObjectPtr,CKDTree*> m_ObjectKdTreeMap;
};