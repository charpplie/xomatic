#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2012 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushUtil.h
//  Created:     6/7/2010 by Jaesik
////////////////////////////////////////////////////////////////////////////
#include "BrushPlane.h"
#include "BrushLine.h"
#include "Controls/PropertyCtrl.h"
#include "IIndexedMesh.h"

// #pragma optimize("",off)
// #define DEBUG 
#ifdef DEBUG
#define ENABLE_DESIGNERASSERTION
//#define ENABLE_DRAWDEBUGHELPER
//#define ENABLE_OUTPUT_DEBUGINFO
#endif

#ifdef ENABLE_DESIGNERASSERTION
#define DESIGNER_ASSERT(condition)	assert(condition)
#else
#define DESIGNER_ASSERT(condition)	assert(1)
#endif

class CResourceHandlerReconstructor
{
public:
	CResourceHandlerReconstructor()
	{
		g_hPrevInst = AfxGetResourceHandle();
		AfxSetResourceHandle(g_hInst);
	}
	~CResourceHandlerReconstructor()
	{
		AfxSetResourceHandle(g_hPrevInst);
	}
private:
	HINSTANCE g_hPrevInst;
};
#define RESOURCEHANDLER_RECONSTRUCTOR CResourceHandlerReconstructor resourceHandlerReconstructor

static const BrushFloat kDesignerEpsilon((BrushFloat)1.0e-4);
static const BrushFloat kDesignerLooseEpsilon((BrushFloat)1.0e-3);
static const BrushFloat kDistanceLimitation((BrushFloat)0.001);
static const BrushFloat kLimitForMagnetic((BrushFloat)8.0);
static const BrushFloat kInitialPrimitiveHeight((BrushFloat)1.0e-5);

class CBrushRegion;
class CBaseBrush;
class CBrushDesigner;
class CBrushDesignerElementManager;
class CBrushDesignerHalfEdgeMesh;

namespace BUtil
{
	struct STexInfo
	{
		float shift[2];
		float scale[2];
		float rotate;

		STexInfo()
		{
			shift[0] = shift[1] = 0;
			scale[0] = scale[1] = 1.0f;
			rotate = 0;
		}

		STexInfo(const STexInfo& ti )
		{
			shift[0] = ti.shift[0];
			shift[1] = ti.shift[1];

			scale[0] = ti.scale[0];
			scale[1] = ti.scale[1];

			rotate = ti.rotate;
		}

		void Load( XmlNodeRef &xmlNode );
		void Save( XmlNodeRef &xmlNode );
	};

	enum ESolidBrushCreateType
	{
		eBrushCreateType_Box,
		eBrushCreateType_Cone,
		eBrushCreateType_Sphere,
		eBrushCreateType_Cylinder,
		eBrushCreateType_Rectangle,
		eBrushCreateType_Disc,
		eBrushCreateType_Custom,
		eBrushCreateType_Max,
	};

	enum EBooleanOperationEnum
	{
		eBOE_Union,
		eBOE_Intersection,
		eBOE_Difference,
	};

	enum EPointPosEnum
	{
		ePP_BORDER,
		ePP_INSIDE,
		ePP_OUTSIDE
	};

	enum EClipType
	{
		eCT_Positive,
		eCT_Negative,
	};

	enum EBrushFlags 
	{
		BRF_MESH_VALID = 0x40,
		BRF_MODIFIED = 0x80,
		BRF_SUB_OBJ_SEL = 0x100,
		BRF_SELECTED    = 0x200,
	};

	struct SSelectedInfo
	{
		SSelectedInfo()
		{
			memset(this,0,sizeof(*this));
		}
		CBaseObject* m_pObj;
		CBaseBrush* m_pBrush;
		CBrushDesigner* m_pDesigner;
	};

	typedef uint16 VertexFaceIndexType;

	static const char* BrushDesignerDefaultMaterial("EngineAssets/TextureMsg/DefaultSolids");
	static const char* BrushSolidDefaultMaterial("EngineAssets/TextureMsg/DefaultSolids");
	static VertexFaceIndexType VertexFaceMax = std::numeric_limits<VertexFaceIndexType>::max();
	static const int kLineThickness(2);
	static const int kChosenLineThickness(7);
	static const ColorB kSelectedColor(214,150,0,255);
	static const ColorB kResizedRegionColor(100,100,200,255);
	static const ColorB RegionLineColor(100,100,200,255);
	static const ColorB RegionInvalidLineColor(255,100,0,255);
	static const ColorB RegionParallelToAxis(100,200,200,255);
	static uint32 s_nGlobalBrushFileId = 0;
	static uint32 s_nGlobalBrushDesignerFileId = 0;
	static const BrushFloat PI = 3.14159265358979323846;
	static const BrushFloat PI2 = BUtil::PI * 2.0;
	static const BrushFloat kNormalGap(0.01);
	static const BrushFloat kEnoughBigNumber(3e10);
	static const char* BrushDesignerCameraName("Crydesigner_camera");
	static const int kDefaultSubdivisionNum = 6;
	static const int kDefaultCurveEdgeNum = 18;
	static const int kDefaultViewDist = 100;
	static const ColorB kElementBoxColor(0xFFFFAAAA);
	static const BrushFloat kElementEdgeThickness = 6;
	static const int kSmoothingGroupIDNumber = 32;
	static const int kMaximumSubdivisionLevel = 5;

	typedef int ShelfID;
	static const ShelfID kMaxShelfCount = 2;

	// BrushEdge3D - A picked Edge, BrushVec3 - A picked point in the middle of the edge
	typedef std::vector< std::pair<BrushEdge3D,BrushVec3> > EdgeQueryResult;

	struct SVertex : public CRefCountBase
	{
		SVertex() :
	m_Normal(0,0,0)
	{
	}
	SVertex( const BrushVec3& position ) :
	m_Pos(position)
	{
		m_Normal = BrushVec3(0,0,0);
	}
	SVertex( const BrushVec3& position, const BrushVec3& normal ) :
	m_Pos(position),
		m_Normal(normal)
	{
	}
	SVertex( const SVertex& vertex ) :
	m_Pos(vertex.m_Pos),
		m_Normal(vertex.m_Normal)
	{
	}
		bool IsEquivalent(const SVertex& vertex, const BrushFloat& epsilon=VEC_EPSILON) const
		{
			return m_Pos.IsEquivalent(vertex.m_Pos,epsilon) && m_Normal.IsEquivalent(vertex.m_Normal,epsilon);
		}
		BrushVec3 m_Pos;
		BrushVec3 m_Normal;
	};
	typedef _smart_ptr<SVertex> VertexPtr;
	typedef std::vector<VertexPtr> VertexList;

	struct SFace : public CRefCountBase
	{
		SFace() :
		m_MaterialID(0)
		{
		}
		SFace( const SFace& face )
		{
			m_IndexList = face.m_IndexList;
			m_Plane = face.m_Plane;
			m_MaterialID = face.m_MaterialID;
			m_TextureInfo = face.m_TextureInfo;
		}
		std::vector<BUtil::VertexFaceIndexType> m_IndexList;
		BrushPlane m_Plane;
		short m_MaterialID;
		BUtil::STexInfo	 m_TextureInfo;
	};
	typedef _smart_ptr<SFace> FacePtr;
	typedef std::vector<FacePtr> FaceList;

	struct SEdge
	{
		SEdge(){}

		SEdge( int index0, int index1 ) 
		{
			m_i[0] = index0;
			m_i[1] = index1;
		}
		SEdge( const SEdge& edge )
		{
			m_i[0] = edge.m_i[0];
			m_i[1] = edge.m_i[1];
		}
		bool operator == (const SEdge& edge)
		{			
			return m_i[0] == edge.m_i[0] && m_i[1] == edge.m_i[1];
		}

		bool operator < ( const SEdge& e ) const
		{
			return m_i[0] < e.m_i[0] || m_i[0] == e.m_i[0] && m_i[1] < e.m_i[1];
		}

		int m_i[2];
	};
	typedef std::vector<SEdge> EdgeList;
	typedef std::set<SEdge> EdgeSet;
	typedef std::set<int> EdgeIndexSet;
	typedef std::map<int,EdgeIndexSet> EdgeMap;

	enum EPickingFlag
	{
		ePF_Vertex = BIT(0),
		ePF_Edge = BIT(1),
		ePF_Face = BIT(2),
		ePF_All = ePF_Vertex | ePF_Edge | ePF_Face
	};

	enum EPushPull
	{
		ePP_Push,
		ePP_Pull,
		ePP_None,
	};

	enum EClipObjective
	{
		eCO_JustClip,
		eCO_Union0,
		eCO_Union1,
		eCO_Subtract,
		eCO_Intersection0,
		eCO_Intersection0IncludingCoSame,
		eCO_Intersection1,
		eCO_Intersection1IncludingCoDiff,
	};

	enum EIntersectionType
	{
		eIT_None,
		eIT_Intersection,
		eIT_JustTouch,
	};

	enum EDesignerMode
	{
		eDesigner_Select_Vertex = BIT(0),
		eDesigner_Select_Edge = BIT(1),
		eDesigner_Select_Face = BIT(2),
		eDesigner_Select_VertexEdge = eDesigner_Select_Vertex | eDesigner_Select_Edge,
		eDesigner_Select_VertexFace = eDesigner_Select_Vertex | eDesigner_Select_Face,
		eDesigner_Select_EdgeFace = eDesigner_Select_Edge | eDesigner_Select_Face,
		eDesigner_Select_VertexEdgeFace = eDesigner_Select_Vertex | eDesigner_Select_Edge | eDesigner_Select_Face,
		eDesigner_Select_AllNone,
		eDesigner_Select_Connected,
		eDesigner_Select_Grow,
		eDesigner_Select_Loop,
		eDesigner_Select_Ring,
		eDesigner_Select_Invert,
		eDesigner_Magnet,
		eDesigner_Box,
		eDesigner_Sphere,
		eDesigner_Cylinder,
		eDesigner_Cone,
		eDesigner_Rectangle,
		eDesigner_Disc,
		eDesigner_Line,
		eDesigner_Curve,
		eDesigner_Weld,
		eDesigner_Slice,
		eDesigner_Remove,
		eDesigner_Fill,
		eDesigner_Extrude,
		eDesigner_Offset,
		eDesigner_Seprate,
		eDesigner_Merge,
		eDesigner_Copy,
		eDesigner_Flip,
		eDesigner_Bevel,
		eDesigner_Stair,
		eDesigner_StairProfile,
		eDesigner_Array,
		eDesigner_Clone,
		eDesigner_Mirror,
		eDesigner_Lathe,
		eDesigner_Mapping,
		eDesigner_Debugger,
		eDesigner_ResetXForm,
		eDesigner_Export,
		eDesigner_Pivot,
		eDesigner_Pivot2Bottom,
		eDesigner_Boolean,
		eDesigner_ObjectMode,
		eDesigner_SnapToGrid,
		eDesigner_SmoothingGroup,
		eDesigner_HideFace,
		eDesigner_RemoveDoubles,
		eDesigner_CubeEditor,
		eDesigner_Subdivision,
		eDesigner_Max
	};

	static bool IsCreationTool( BUtil::EDesignerMode designerMode )
	{ 
		return designerMode == BUtil::eDesigner_Box || designerMode == BUtil::eDesigner_Sphere ||
			designerMode == BUtil::eDesigner_Cylinder || designerMode == BUtil::eDesigner_Cone ||
			designerMode == BUtil::eDesigner_Rectangle || designerMode == BUtil::eDesigner_Disc ||
			designerMode == BUtil::eDesigner_Stair || designerMode == BUtil::eDesigner_Line || 
			designerMode == BUtil::eDesigner_Curve || designerMode == eDesigner_StairProfile;
	}

	static bool IsSelectElementMode( BUtil::EDesignerMode mode )
	{
		return mode == BUtil::eDesigner_Select_Vertex || mode == BUtil::eDesigner_Select_Edge || mode == BUtil::eDesigner_Select_Face ||
			mode == BUtil::eDesigner_Select_VertexEdge || mode == BUtil::eDesigner_Select_VertexFace || mode == BUtil::eDesigner_Select_EdgeFace ||
			mode == BUtil::eDesigner_Select_VertexEdgeFace;
	}

	static bool IsEdgeSelectMode( BUtil::EDesignerMode mode )
	{
		return mode == BUtil::eDesigner_Select_Edge ||
			mode == BUtil::eDesigner_Select_VertexEdge || mode == BUtil::eDesigner_Select_EdgeFace ||
			mode == BUtil::eDesigner_Select_VertexEdgeFace;
	}

	enum EPivotPosition
	{
		ePivotPos_BottomCenter,
		ePivotPos_Center,
		ePivotPos_TopCenter,
		ePivotPos_Max
	};
	static const char* sPivotPosName[ePivotPos_Max] =  { "bottomcenter", "center", "topcenter" };

	enum EStairHeightCalculationWayMode
	{
		eStairHeightCalculationWay_StepRise,
		eStairHeightCalculationWay_StepNumber
	};

	enum EResetXForm
	{
		eResetXForm_Rotation = BIT(0),
		eResetXForm_Scale = BIT(1),
		eResetXForm_Position = BIT(2),
		eResetXForm_All = eResetXForm_Rotation|eResetXForm_Scale|eResetXForm_Position
	};

	enum EDBResetFlag
	{
		eDBRF_Plane	 = BIT(0),
		eDBRF_Vertex = BIT(1),
		eDBRF_ALL    = 0xFFFFFFFF
	};

	static const float kDefaultStepRise = 0.33f;
	static const int kDefaultStepNumber = 16;

	BrushVec3 WorldPos2ScreenPos( const BrushVec3& worldPos );
	BrushVec3 ScreenPos2WorldPos( const BrushVec3& screenPos );
	void DrawSpot( DisplayContext& dc, const BrushMatrix34& worldTM, const BrushVec3& pos, const ColorB& color, float fSize = 5.0f );

	EOperationResult SubtractEdge3D( const BrushEdge3D& inEdge0, const BrushEdge3D& inEdge1, BrushEdge3D outEdge[2], const BrushFloat& kEpsilon );
	EOperationResult IntersectEdge3D( const BrushEdge3D& inEdge0, const BrushEdge3D& inEdge1, BrushEdge3D& outEdge, const BrushFloat& kEpsilon );
	ESplitResult Split(
		const BrushPlane& plane,
		const BrushLine& splitLine,
		const std::vector<BrushEdge3D>& splitEdges,
		const BrushEdge3D& inEdge,
		BrushEdge3D& positiveEdge,
		BrushEdge3D& negativeEdge,
		const BrushFloat& kEpsilon );
	void CalcTextureBasis(	const SBrushPlane<float>& p, const BUtil::STexInfo& ti, Vec3& u, Vec3& v );
	void CalcTexCoords(	const SBrushPlane<float>& p, const BUtil::STexInfo& ti, const Vec3& pos, float& tu, float &tv );
	void FitTexture( const SBrushPlane<float>& plane, const Vec3& vMin, const Vec3& vMax, float tileU, float tileV, STexInfo& outTexInfo );

	struct SMeshInfo
	{
		std::vector<BrushVec3> vertexList;
		std::vector<BrushVec3> normalList;
		std::vector<SMeshTexCoord> uvList;
		std::vector<SMeshFace> faceList;
		std::map<int,int> matId2SubsetMap;

		int FindMatIdFromSubsetNum( int nSubsetNum ) const
		{
			std::map<int,int>::const_iterator ii = matId2SubsetMap.begin();
			for(; ii != matId2SubsetMap.end(); ++ii )
			{
				if( ii->second == nSubsetNum )
					return ii->first;
			}
			return -1;
		}

		int AddMatID( int nMatID )
		{
			int nSubsetNumber = 0;
			if( matId2SubsetMap.find(nMatID) != matId2SubsetMap.end() )
			{
				nSubsetNumber = matId2SubsetMap[nMatID];
			}
			else
			{
				nSubsetNumber = matId2SubsetMap.size();
				matId2SubsetMap[nMatID] = nSubsetNumber;
			}
			return nSubsetNumber;
		}

		void Reserve( int nSize )
		{
			vertexList.reserve(nSize);
			normalList.reserve(nSize);
			uvList.reserve(nSize);
			faceList.reserve(nSize);
		}

		void Clear()
		{
			vertexList.clear();
			normalList.clear();
			uvList.clear();
			faceList.clear();
			matId2SubsetMap.clear();
		}

		bool IsValid() const
		{
			return !vertexList.empty() && !faceList.empty() && !uvList.empty() && !normalList.empty();
		}
	};
	void CreateMeshFacesFromRegion( CBrushRegion* pRegion, BUtil::SMeshInfo& mesh, bool bGenerateBackFaces );	
	void FillMesh( const SMeshInfo& mesh, IIndexedMesh* pMesh );
	void JoinTwoMeshes( SMeshInfo& mesh0, const SMeshInfo& mesh1 );	
	void MakeSectorOfCircle( BrushFloat fRadius, const BrushVec2& vCenter, BrushFloat startRadian, BrushFloat diffRadian, int nSegmentCount, std::vector<BrushVec2>& outVertexList );
	float ComputeAnglePointedByPos( const BrushVec2& vCenter, const BrushVec2& vPointedPos );
	template<class T>
	bool FindVertexWithMinimum( std::vector<T> vertexList, int nElementIndex, int& outMinimumIndex )
	{
		int nMinimumIndex = -1;
		BrushFloat minimumValue = 3e10f;
		for( int i = 0, iVertexSize(vertexList.size()); i < iVertexSize; ++i )
		{
			if( vertexList[i][nElementIndex] < minimumValue )
			{
				nMinimumIndex = i;
				minimumValue = vertexList[i][nElementIndex];
			}
		}
		if( nMinimumIndex == -1 )
			return false;
		outMinimumIndex = nMinimumIndex;
		return true;
	}

	template<class T>
	T GetAdjustedFloatToAvoidZero( T x )
	{
		T _x = std::abs(x) < kDesignerEpsilon ? kDesignerEpsilon : x;
		if( x < 0 )
			_x *= -1;
		return _x;
	}

	bool DoesEquivalentExist( std::vector<BrushEdge3D>& edgeList, const BrushEdge3D& edge, int* pOutIndex = NULL );
	bool DoesEquivalentExist( std::vector<BrushVec3>& vertexList, const BrushVec3& vertex );
	bool ComputePlane( const std::vector<BrushVec3>& vList, BrushPlane& outPlane );
	void GetLocalViewRay( const BrushMatrix34& worldTM, IDisplayViewport *view, CPoint point, BrushVec3& outRaySrc, BrushVec3& outRayDir );
	void DrawPlane( DisplayContext &dc, const BrushVec3& vPivot, const BrushPlane& plane, float size = 12.0f );
	BrushFloat SnapGrid( BrushFloat fValue );
	BrushFloat Snap( BrushFloat pos );

	void Write2Buffer( std::vector<char>& buffer, const void* pData, int nDataSize );
	int ReadFromBuffer( std::vector<char>& buffer, int nPos, void* pData, int nDataSize );	

	int GetPolygonCountUsedInAllDesignerObjects();
	bool GetIntersectionOfRayAndAABB( const BrushVec3& vPoint, const BrushVec3& vDir, const AABB& aabb, BrushFloat* pOutDistance );

	int FindShortestEdge( std::vector<BrushEdge3D>& edges );
	std::vector<BrushEdge3D> FindNearestEdges( CViewport* pViewport, const BrushMatrix34& worldTM, std::vector< std::pair<BrushEdge3D,BrushVec3> >& edges );

	BrushVec3 GetElementBoxSize( IDisplayViewport* pView, bool b2DViewport, const BrushVec3& vWorldPos );
	bool AreTwoPositionsNear( const BrushVec3& modelPos0, const BrushVec3& modelPos1, const BrushMatrix34& worldTM, IDisplayViewport* pViewport, BrushFloat fBoundRadiuse, BrushFloat* pOutDistance = NULL);
	bool HitTestEdge( const BrushVec3& raySrc, const BrushVec3& rayDir, const BrushVec3& vNormal, const BrushEdge3D& edge, IDisplayViewport* pView = NULL );
	bool PickPosFromWorld( IDisplayViewport* view, const CPoint& point, Vec3& outPickedPos );
	BrushVec3 CorrectVec3( const BrushVec3& v );
	bool IsConvex( std::vector<BrushVec2>& vList );

	class CBrushDesignerBasicPanel : public CXTResizeDialog
	{
	protected:

		enum { IDD = IDD_PANEL_DESIGNER_EMPTY };

		void InitEmptyPanel()
		{
			m_bAllowChangeVariable = true;
			CWnd* pFrameWnd = GetDlgItem(IDC_STATIC);
			CRect clientRect;
			pFrameWnd->GetClientRect(&clientRect);
			CRect frameWndRect;
			pFrameWnd->GetWindowRect(&frameWndRect);
			CRect wndRect;
			GetWindowRect(wndRect);
			clientRect.OffsetRect(frameWndRect.left-wndRect.left,frameWndRect.top-wndRect.top);
			m_PropertyCtrl.Create(WS_CHILD|WS_VISIBLE, &clientRect, this);
		}

		void DestroyVariables()
		{
			for( int i = 0, iCount(m_Variables.size()); i < iCount; ++i )
				m_Variables[i]->RemoveOnSetCallback(m_ValueCallBack);			
		}

		void SetCallBack( IVariable::OnSetCallback& callback ) { m_ValueCallBack = callback; }
		void AddVariable( IVariable* pVar )
		{ 
			pVar->AddOnSetCallback(m_ValueCallBack);
			m_Variables.push_back(pVar);
		}

		void PostNcDestroy(){ delete this; }

		std::vector< _smart_ptr<IVariable> >  m_Variables;

		CPropertyCtrl m_PropertyCtrl;
		IVariable::OnSetCallback m_ValueCallBack;
		bool m_bAllowChangeVariable;
	};

	struct SMainContext
	{
		SMainContext() : pObject(NULL),pBrush(NULL),pDesigner(NULL) {}
		SMainContext( CBaseObject* _pObject, CBaseBrush* _pBrush, CBrushDesigner* _pDesigner ) : pObject(_pObject), pBrush(_pBrush), pDesigner(_pDesigner) {}
		CBaseObject* pObject;
		CBaseBrush* pBrush;
		CBrushDesigner* pDesigner;
		CBrushDesignerElementManager* pSelected;
	};

	enum EPlacementType
	{
		ePlacementType_Divide,
		ePlacementType_Multiply
	};
};
