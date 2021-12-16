#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2013 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushDesignerSpotManager.h
//  Created:     May/6/2013 by Jaesik.
////////////////////////////////////////////////////////////////////////////
#include "BrushRegion.h"

class CBrushDesigner;

class CBrushDesignerSpotManager
{

protected:

	enum ESpotPositionState
	{
		eSpotPosState_InRegion,
		eSpotPosState_Edge,
		eSpotPosState_CenterOfEdge,
		eSpotPosState_CenterOfRegion,
		eSpotPosState_EitherPointOfEdge,
		eSpotPosState_FirstPointOfRegion,
		eSpotPosState_LastPointOfRegion,
		eSpotPosState_AtFirstSpot,
		eSpotPosState_OnVirtualLine,
		eSpotPosState_OutsideDesigner,
		eSpotPosState_Invalid
	};

	struct SSpot
	{
		SSpot() : m_Pos(BrushVec3(0,0,0))
		{
			Reset();
		}

		explicit SSpot( const BrushVec3& pos ) : m_Pos(pos)
		{
			Reset();
		}

		SSpot( const BrushVec3& pos, ESpotPositionState posState, CBrushRegion::RegionPtr pRegion ) : m_Pos(pos), m_PosState(posState), m_pRegion(pRegion)
		{
			if( m_pRegion )
				m_Plane = m_pRegion->GetPlane();
			else
				m_Plane = BrushPlane(BrushVec3(0,0,0),0);
			m_bProcessed = false;
		}

		SSpot( const BrushVec3& pos, ESpotPositionState posState, const BrushPlane& plane ) : m_Pos(pos), m_PosState(posState), m_Plane(plane)
		{
			m_pRegion = NULL;
			m_bProcessed = false;
		}

		bool IsAtEndPoint() const
		{
			return m_PosState == eSpotPosState_FirstPointOfRegion || m_PosState == eSpotPosState_LastPointOfRegion;
		}

		bool IsEquivalentPos( const SSpot& spot ) const
		{
			return m_Pos.IsEquivalent(spot.m_Pos,kDesignerEpsilon);
		}

		bool IsOnEdge() const
		{
			return IsAtEndPoint() || m_PosState == eSpotPosState_Edge || m_PosState == eSpotPosState_CenterOfEdge || m_PosState == eSpotPosState_EitherPointOfEdge;
		}

		bool IsCenterOfEdge() const
		{
			return m_PosState == eSpotPosState_CenterOfEdge;
		}

		bool IsAtEitherPointOnEdge() const
		{
			return IsAtEndPoint() || m_PosState == eSpotPosState_EitherPointOfEdge;
		}

		bool IsInRegion() const
		{
			return m_PosState == eSpotPosState_InRegion || m_PosState == eSpotPosState_CenterOfRegion;
		}

		bool IsSamePos( const SSpot& spot ) const
		{
			return m_Pos.IsEquivalent(spot.m_Pos,kDesignerEpsilon);
		}

		void Reset()
		{
			m_PosState = eSpotPosState_InRegion;
			m_pRegion = NULL;
			m_bProcessed = false;
			m_Plane = BrushPlane(BrushVec3(0,0,0),0);
		}

		ESpotPositionState m_PosState;
		bool m_bProcessed;
		BrushVec3 m_Pos;
		BrushPlane m_Plane;
		CBrushRegion::RegionPtr m_pRegion;
	};

	struct SSpotPair
	{
		SSpotPair(){}
		SSpotPair( const SSpot& spot0, const SSpot& spot1 )
		{
			m_Spot[0] = spot0;
			m_Spot[1] = spot1;
		}
		SSpot m_Spot[2];
	};

	typedef std::vector<SSpot> SpotList;
	typedef std::vector<SSpotPair> SpotPairList;

protected:

	CBrushDesignerSpotManager()
	{
		ResetAllSpots();
		m_bBuiltInSnap = false;
		m_BuiltInSnapSize = (BrushFloat)0.5;
		m_bEnableMagnetic = true;
	}
	~CBrushDesignerSpotManager(){}

	void DrawPolyline( DisplayContext& dc ) const;
	void DrawCurrentSpot( DisplayContext& dc, const BrushMatrix34& worldTM ) const;	
	void ResetAllSpots(){
		m_SpotList.clear();
		m_CurrentSpot.Reset();
		m_StartSpot.Reset();
	}

	virtual void CreateRegionFromSpots( bool bClosedRegion, const SpotList& spotList )		{	assert(0); };

	static void GenerateVertexListFromSpotList( const SpotList& spotList, std::vector<BrushVec3>& outVList );

	void RegisterSpotList( CBrushDesigner* pDesigner, const SpotList& spotList );

	const BrushVec3& GetCurrentSpotPos() const{return m_CurrentSpot.m_Pos;}
	const BrushVec3& GetStartSpotPos() const{return m_StartSpot.m_Pos;}

	void SetCurrentSpotPos( const BrushVec3& vPos ){m_CurrentSpot.m_Pos = vPos;}
	void SetStartSpotPos( const BrushVec3& vPos ){m_StartSpot.m_Pos	= vPos;}

	void SetCurrentSpotPosState( ESpotPositionState state ){m_CurrentSpot.m_PosState = state;}
	ESpotPositionState GetCurrentSpotPosState() const{return m_CurrentSpot.m_PosState;}
	void ResetCurrentSpot(){
		m_CurrentSpot.Reset();
	}
	void ResetCurrentSpotWeakly(){
		m_CurrentSpot.m_PosState = eSpotPosState_InRegion;
	}

	const SSpot& GetCurrentSpot() const{return m_CurrentSpot;}
	const SSpot& GetStartSpot() const{return m_StartSpot;}

	void SetCurrentSpot( const SSpot& spot ){m_CurrentSpot = spot;}
	void SetStartSpot( const SSpot& spot ){m_StartSpot = spot;}
	void SetSpotProcessed( int nPos, bool bProcessed ){ m_SpotList[nPos].m_bProcessed = bProcessed;}
	void SetPosState( int nPos, ESpotPositionState state ){m_SpotList[nPos].m_PosState = state;}

	void SwapCurrentAndStartSpots(){std::swap( m_CurrentSpot, m_StartSpot );}

	int GetSpotListCount() const{return m_SpotList.size();}
	const BrushVec3& GetSpotPos( int nPos ) const {return m_SpotList[nPos].m_Pos;}
	void AddSpotToSpotList( const SSpot& spot );
	void ReplaceSpotList( const SpotList& spotList );

	void ClearSpotList(){m_SpotList.clear();}

	const SSpot& GetSpot( int nIndex ) const { return m_SpotList[nIndex]; }
	const SpotList& GetSpotList() const { return m_SpotList; }

	void SplitSpotList( CBrushDesigner* pDesigner, const SpotList& spotList, std::vector<SpotList>& outSpotLists );
	void SplitSpot( CBrushDesigner* pDesigner, const SSpotPair& spotPair, SpotPairList& outSpotPairs );

	bool UpdateCurrentSpotPosition( CBrushDesigner* pDesigner, const BrushMatrix34& worldTM, const BrushPlane& plane, IDisplayViewport *view, CPoint point, bool bKeepInitialPlane, bool bSearchAllShelves = false );
	SSpot Convert2Spot( CBrushDesigner* pDesigner, const BrushVec3& pos ) const;

	bool GetPlaneBeginEndPoints( const BrushPlane& plane, BrushVec2& outProjectedStartPt, BrushVec2& outProjectedEndPt ) const;

	bool GetPosAndPlaneBasedOnWorld( IDisplayViewport* view, const CPoint& point, const BrushMatrix34& worldTM, BrushVec3& outPos, BrushPlane& outPlane );

	bool IsSnapEnabled() const;
	BrushVec3 Snap(const BrushVec3& vPos) const;
	
	void EnableBuiltInSnap( bool bEnabled ) { m_bBuiltInSnap = bEnabled; }
	void SetBuiltInSnapSize( BrushFloat fSnapSize ) { m_BuiltInSnapSize = fSnapSize; }

	void EnableMagnetic( bool bEnabled ) {  m_bEnableMagnetic = bEnabled; }

private:

	struct SCandidateInfo
	{
		SCandidateInfo( const BrushVec3& pos, ESpotPositionState spotPosState ) : m_Pos(pos), m_SpotPosState(spotPosState){}
		BrushVec3 m_Pos;
		ESpotPositionState m_SpotPosState;
	};

	bool FindBestPlane( CBrushDesigner* pDesigner, const SSpot& s0, const SSpot& s1, BrushPlane& outPlane );
	bool AddRegionToDesignerFromSpotList( CBrushDesigner* pDesigner, const SpotList& spotList );
	bool FindSnappedSpot( const BrushMatrix34& worldTM, CBrushRegion::RegionPtr pPickedRegion, const BrushVec3& pickedPos, SSpot& outSpot ) const;
	bool FindNicestSpot( IDisplayViewport* pViewport, const std::vector<SCandidateInfo>& candidates, const CBrushDesigner* pDesigner, const BrushMatrix34& worldTM, const BrushVec3& pickedPos, CBrushRegion::RegionPtr pPickedRegion, const BrushPlane& plane, SSpot& outSpot ) const;
	bool FindSpotNearAxisAlignedLine( IDisplayViewport* pViewport, CBrushRegion::RegionPtr pRegion, const BrushMatrix34& worldTM, SSpot& outSpot );

	struct SIntersectionInfo
	{
		int nRegionIndex;
		BrushVec3 vIntersection;
	};

	SSpot m_CurrentSpot;
	SSpot m_StartSpot;
	SpotList m_SpotList;

	bool m_bEnableMagnetic;
	bool m_bBuiltInSnap;
	BrushFloat m_BuiltInSnapSize;
};