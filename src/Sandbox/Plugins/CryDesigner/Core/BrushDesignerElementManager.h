#pragma once

#include "BrushRegion.h"
#include "BrushDesignerDB.h"

class CBaseObject;

struct SDesignerElement
{
	SDesignerElement() : m_bIsolated(false) {}
	SDesignerElement( CBaseObject* pObject, CBrushRegion::RegionPtr pRegion ) : m_bIsolated(false)
	{
		SetFace(pObject,pRegion);
	}
	SDesignerElement( CBaseObject* pObject, const BrushEdge3D& edge ) : m_bIsolated(false)
	{
		SetEdge(pObject,edge);
	}
	SDesignerElement( CBaseObject* pObject, const BrushVec3& vertex ) : m_bIsolated(false)
	{
		SetVertex(pObject,vertex);
	}
	SDesignerElement GetMirroredElement( const BrushPlane& mirrorPlane ) const;
	bool operator == ( const SDesignerElement& info );
	void Invalidate();
	bool IsEquivalent( const SDesignerElement& elementInfo ) const;

	bool IsVertex()	const {return m_Vertices.size() == 1;}
	bool IsEdge()	const {return m_Vertices.size() == 2;}
	bool IsFace()	const {return m_Vertices.size() >= 3;}

	BrushVec3 GetVertex() const
	{
		assert(IsVertex());
		if( !IsVertex() )
			return BrushVec3(0,0,0);
		return m_Vertices[0];
	}

	BrushEdge3D GetEdge() const
	{
		assert(IsEdge());
		if( !IsEdge() )
			return BrushEdge3D(BrushVec3(0,0,0), BrushVec3(0,0,0));
		return BrushEdge3D(m_Vertices[0], m_Vertices[1]);
	}

	void SetFace( CBaseObject* pObject, CBrushRegion::RegionPtr pRegion )
	{
		m_pRegion = pRegion;
		int iVertexSize(pRegion->GetVertexListSize());
		m_Vertices.reserve(iVertexSize);
		for( int i = 0; i < iVertexSize; ++i )
			m_Vertices.push_back(pRegion->GetVertex(i));
		m_pObject = pObject;
	}

	void SetEdge( CBaseObject* pObject, const BrushEdge3D& edge )
	{
		m_pRegion = NULL;
		m_Vertices.resize(2);
		m_Vertices[0] = edge.m_v[0];
		m_Vertices[1] = edge.m_v[1];
		m_pObject = pObject;
	}

	void SetVertex( CBaseObject* pObject, const BrushVec3& vertex )
	{
		m_pRegion = NULL;
		m_Vertices.resize(1);
		m_Vertices[0] = vertex;
		m_pObject = pObject;
	}

	~SDesignerElement(){}

	std::vector<BrushVec3> m_Vertices;
	CBrushRegion::RegionPtr m_pRegion;
	_smart_ptr<CBaseObject> m_pObject;
	bool m_bIsolated;
};

class CBrushDesignerElementManager : public CRefCountBase
{
public:

	void Clear()
	{
		m_Elements.clear();
	}

	int GetSize() const
	{
		return (int)m_Elements.size();
	}

	CBrushDesignerElementManager(){}
	~CBrushDesignerElementManager(){}

	CBrushDesignerElementManager( const CBrushDesignerElementManager& elementManager )
	{
		operator =(elementManager);
	}

	CBrushDesignerElementManager& operator =( const CBrushDesignerElementManager& elementManager )
	{
		m_Elements = elementManager.m_Elements;
		return *this;
	}

	const SDesignerElement& Get( int nIndex ) const { return m_Elements[nIndex]; }
	SDesignerElement& operator [] ( int nIndex ) { return m_Elements[nIndex]; }

	void Set( int nIndex, const SDesignerElement& element )
	{
		m_Elements[nIndex] = element;
	}

	void Set( const CBrushDesignerElementManager& elements ) 
	{
		operator = (elements);
	}

	bool IsEmpty() const { return m_Elements.empty(); }

	bool Add( CBrushDesignerElementManager& elements );
	bool Add( const SDesignerElement& element );

	bool Pick( CBaseObject* pObject, CBrushDesigner* pDesigner, CViewport* viewport, CPoint point, int nFlag, bool bOnlyIncludeCube, BrushVec3* pOutPickedPos );

	void Display( CBaseObject* pObject, DisplayContext &dc ) const;
	void DisplayHighlightElements( CBaseObject* pObject, CBrushDesigner* pDesigner, DisplayContext& dc, int nPickFlag ) const;
	void DisplayVertexElements( CBaseObject* pObject, CBrushDesigner* pDesigner, DisplayContext& dc, int nShelf = -1, std::vector<BrushVec3>* pExcludedVertices = NULL ) const;
	void DisplayFaceElements( CBaseObject* pObject, CBrushDesigner* pDesigner, DisplayContext& dc ) const;	

	void PickAdjacentCurvedEdges( CBaseObject* pObject, CBrushRegion::RegionPtr pRegion, const BrushEdge3D& edge, std::vector<SDesignerElement>* outPickInfo ) const;

	BrushVec3 GetNormal( CBrushDesigner* pDesigner ) const;	
	void Erase( int nElementFlags );
	void Erase( const SDesignerElement& element );
	bool Erase( CBrushDesignerElementManager& elements );

	bool Has( const SDesignerElement& elementInfo ) const;
	bool HasVertex( const BrushVec3& vertex ) const;
	CString GetElementsInfoText();
	void RemoveInvalidElements();

	CBrushDesignerDB::QueryResult QueryFromElements( CBrushDesigner* pDesigner ) const;
	bool QueryNearestVertex( CBaseObject* pObject, CBrushDesigner* pDesigner, CViewport* pView, CPoint point, const BrushVec3& rayLocalSrc, const BrushVec3& rayLocalDir, BrushVec3& outPos, BrushVec3* pOutNormal = NULL ) const;

private:
	
	bool HasRegionSelected( CBrushRegion::RegionPtr pRegion ) const;
	CBrushRegion::RegionPtr PickRegionFromRepresentativeBox( CBaseObject* pObject, CBrushDesigner* pDesigner, CViewport* pView, CPoint point, const BrushVec3& rayLocalSrc, const BrushVec3& rayLocalDir, BrushVec3& outPickedPos ) const;	

	std::vector<SDesignerElement> m_Elements;
};

typedef _smart_ptr<CBrushDesignerElementManager> DesignerElementsPtr;