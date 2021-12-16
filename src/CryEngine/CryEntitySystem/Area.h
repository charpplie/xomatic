////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   Area.h
//  Version:     v1.00
//  Created:     27/9/2004 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __Area_h__
#define __Area_h__
#pragma once

#include "AreaManager.h"
#include "EntitySystem.h"

// Position type of a position compared relative to the given area
enum EAreaPosType
{
	AREA_POS_TYPE_2DINSIDE_ZABOVE,		// Above an area
	AREA_POS_TYPE_2DINSIDE_ZINSIDE,		// Inside an area
	AREA_POS_TYPE_2DINSIDE_ZBELOW,		// Below an area
	AREA_POS_TYPE_2DOUTSIDE_ZABOVE,		// Next to an area but above it
	AREA_POS_TYPE_2DOUTSIDE_ZINSIDE,	// Next to an area but inside its min and max Z
	AREA_POS_TYPE_2DOUTSIDE_ZBELOW,		// Next to an area but below it

	AREA_POS_TYPE_COUNT
};

enum ECachedAreaData
{
	eCachedAreaData_None							= 0x00000000,
	eCachedAreaData_PosTypeValid			= BIT(0),
	eCachedAreaData_PosOnHullValid		= BIT(1),
	eCachedAreaData_DistWithinSqValid	= BIT(2),
	eCachedAreaData_DistNearSqValid		= BIT(3),
	eCachedAreaData_SoundObstructed		= BIT(4)
};

class CArea : public IArea
{

public:

	struct a2DPoint
	{
		float x, y;
		a2DPoint(void):x(0.0f),y(0.0f) { }
		a2DPoint( const Vec3& pos3D ) { x=pos3D.x; y=pos3D.y; }
		a2DPoint(float x, float y):x(x),y(y){}
		float	DistSqr( const struct a2DPoint& point ) const
		{
			float xx = x-point.x;
			float yy = y-point.y;
			return (xx*xx + yy*yy);
		}
		float	DistSqr( const float px, const float py ) const
		{
			float xx = x-px;
			float yy = y-py;
			return (xx*xx + yy*yy);
		}
	};
	struct a2DBBox
	{
		a2DPoint	min;	// 2D BBox min 
		a2DPoint	max;	// 2D BBox max
		bool	PointOutBBox2D (const a2DPoint& point) const
		{
			return (point.x<min.x || point.x>max.x || point.y<min.y || point.y>max.y);
		}
		bool	PointOutBBox2DVertical (const a2DPoint& point) const
		{
			return (point.y<=min.y || point.y>max.y || point.x>max.x);
		}
		bool	BBoxOutBBox2D (const a2DBBox& box) const
		{
			return (box.max.x<min.x || box.min.x>max.x || box.max.y<min.y || box.min.y>max.y);
		}

	};
	struct a2DSegment
	{
		a2DSegment()
			:	isHorizontal(false),
				bObstructSound(false),
				k(0.0f),
				b(0.0f){}

		bool		bObstructSound;	// Does it obstruct sounds?
		bool		isHorizontal;		//horizontal flag
		float		k, b;						//line parameters y=kx+b
		a2DBBox	bbox;						// segment's BBox 

		bool IntersectsXPos( const a2DPoint& point ) const
		{
			return ( point.x < (point.y - b)/k );
		}

		float GetIntersectX( const a2DPoint& point ) const
		{
			return (point.y - b)/k;
		}

		bool IntersectsXPosVertical( const a2DPoint& point ) const
		{
			if(k==0.0f)
				return ( point.x <= b );
			return false;
		}

		a2DPoint GetStart() const
		{
			return a2DPoint (
				          bbox.min.x             ,
				(float) __fsel(k, bbox.min.y, bbox.max.y));
		}

		a2DPoint GetEnd() const
		{
			return a2DPoint(
				bbox.max.x,
				(float) __fsel(k, bbox.max.y, bbox.min.y));
		}
	};

	//////////////////////////////////////////////////////////////////////////
	CArea( CAreaManager *pManager );

	//IArea
	VIRTUAL int GetEntityAmount() const { return m_vEntityID.size(); }
	VIRTUAL const EntityId GetEntityByIdx(int index) const { return m_vEntityID[index]; }
	VIRTUAL void GetMinMax(Vec3 **min, Vec3 **max) const
	{ 
		(*min)->x = m_AreaBBox.min.x; (*min)->y = m_AreaBBox.min.y; (*min)->z = m_VOrigin;
		(*max)->x = m_AreaBBox.max.x; (*max)->y = m_AreaBBox.max.y; (*max)->z = m_VOrigin+m_VSize;
	}
	//~IArea

	// Releases area.
	void Release();

	void	SetID( const int id ) { m_AreaID = id; }
	int		GetID() const { return m_AreaID; }

	void	SetEntityID( const EntityId id ) { m_EntityID = id; }
	int		GetEntityID() const { return m_EntityID; }

	void	SetGroup( const int id) { m_AreaGroupID = id; } 
	int		GetGroup( ) const { return m_AreaGroupID; } 

	void SetPriority ( const int nPriority) { m_nPriority = nPriority; }
	int  GetPriority ( ) const { return m_nPriority; }

	// Description:
	//    Sets if the Area should fully obstruct it's Roof or Floor (on Box or Shape)
	void	SetSoundObstruction( bool const bRoof, bool const bFloor ) { m_bObstructRoof = bRoof; m_bObstructFloor = bFloor; }

	// Description:
	//    Sets the indicated area side's sound obstruction
	void	SetSoundObstructionOnSegment(unsigned int const nSegmentIndex, bool const bObstructs);

	void SetAreaType( EEntityAreaType type );
	EEntityAreaType GetAreaType( ) const { return m_AreaType; } 

	//////////////////////////////////////////////////////////////////////////
	// These functions also switch area type.
	//////////////////////////////////////////////////////////////////////////
	void	SetPoints( const Vec3* const vPoints, const bool* const pabSoundObstructionSegments, const int nPointsCount );
	void	SetBox( const Vec3& min,const Vec3& max,const Matrix34 &tm );
	void	SetSphere( const Vec3& vCenter,float fRadius );
	//////////////////////////////////////////////////////////////////////////

	void	SetBoxMatrix( const Matrix34 &tm );
	void GetBoxMatrix( Matrix34& tm );

	void GetBox( Vec3 &min,Vec3 &max ) { min = m_BoxMin; max = m_BoxMax; }
	void GetSphere( Vec3& vCenter,float &fRadius ) { vCenter = m_SphereCenter; fRadius = m_SphereRadius; }
	
	void  SetHeight( float fHeight ) { m_VSize = fHeight; }
	float GetHeight() const { return m_VSize; }

	void	SetFade( const float fFade) { m_PrevFade = fFade; }
	float GetFade( ) const { return m_PrevFade; }

	void	AddEntity( const EntityId entId );
	void	AddEntites( const std::vector<EntityId> &entIDs );
	void	GetEntites( std::vector<EntityId> &entIDs );
	const std::vector<EntityId>* GetEntities() const { return &m_vEntityID; }

	void	ClearEntities( );

	void	SetProximity( float prx ) { m_fProximity = prx; }
	float	GetProximity( ) { return m_fProximity;}

	float GetMaximumEffectRadius();

	// Calculations
	void	CalcCachedPointPosType(Vec3 const& rv3Pos);
	float	CalcPointWithinDist(const Vec3& point3d, bool const bIgnoreSoundObstruction = true);
	bool	CalcPointWithin(const Vec3& point3d, bool bIgnoreHeight=false) const;
	float	CalcDistToPoint(const a2DPoint& point) const;
	
	// squared-distance returned works only if point32 is not within the area
	float	CalcPointNearDistSq(const Vec3 &Point3d, Vec3 &OnHull3d, bool const bIgnoreSoundObstruction = true);
	float CalcPointNearDistSq(const Vec3 &Point3d, bool bIgnoreHeight = false, bool const bIgnoreSoundObstruction = true);
	float const	ClosestPointOnHullDistSq(const Vec3 &Point3d, Vec3 &OnHull3d, bool const bIgnoreSoundObstruction = true);

	// Get cached data
	void	InvalidateCachedAreaData();
	inline ECachedAreaData const	GetFlags() const { return m_oCachedAreaData.eFlags; }
	inline float const	GetCachedPointWithinDistSq() const { return m_oCachedAreaData.fDistanceWithinSq; }
	inline bool const		GetCachedPointWithin() const { return m_oCachedAreaData.ePosType == AREA_POS_TYPE_2DINSIDE_ZINSIDE; }
	inline float const	GetCachedPointNearDistSq() const { return m_oCachedAreaData.fDistanceNearSq; }

	// events
	void					SetCachedEvent	(const SEntityEvent* pNewEvent);
	SEntityEvent* GetCachedEvent	() { return &m_CachedEvent; };
	void					SendCachedEvent	();

	// Inside
	void	EnterArea( IEntity const* const __restrict pEntity );
	void	LeaveArea( IEntity const* const __restrict pEntity );
	void	UpdateArea( const Vec3 &vPos, IEntity const* const pEntity );
	void	UpdateAreaInside( IEntity const* const __restrict pEntity);
	void	ExclusiveUpdateAreaInside( IEntity const* const __restrict pEntity, EntityId const AreaHighID);
	float	CalculateFade( const Vec3& pos3D );
	void	ProceedFade( IEntity const* const __restrict pEntity, float const fadeValue );

	//Near
	void	EnterNearArea( IEntity const* const __restrict pEntity , float const fDistanceSq);
	void	LeaveNearArea( IEntity const* const __restrict pEntity );
	void	UpdateAreaNear( IEntity const* const __restrict pEntity, float const fDistanceSq );

	void	Draw( const int idx );

	unsigned MemStat();

	void	SetStepId( int id ) { m_stepID = id;}
	int		GetStepId() const { return m_stepID; }
	void	SetActive( bool bActive ) { m_bIsActive = bActive; }
	bool	IsActive() const { return m_bIsActive; }
	bool	HasSoundAttached();

	void	GetBBox(Vec2& vMin, Vec2& vMax);

private:

	struct SCachedAreaData
	{
		SCachedAreaData()
			:	eFlags(eCachedAreaData_None),
				ePosType(AREA_POS_TYPE_COUNT),
				v3Pos(0),
				fDistanceWithinSq(FLT_MAX),
				fDistanceNearSq(FLT_MAX){}

		ECachedAreaData	eFlags;
		EAreaPosType		ePosType;
		Vec3						v3Pos;
		float						fDistanceWithinSq;
		float						fDistanceNearSq;
	};

	~CArea();

	void	AddSegment(const a2DPoint& p0, const a2DPoint& p1, bool const nObstructSound);
	void	CalcBBox();
	void	ClearPoints();
	IEntitySystem* GetEntitySystem() { return m_pAreaManager->GetEntitySystem(); }
	void	CalcClosestPointToObstructedShape(Vec3& rv3ClosestPos, float& rfClosestDistSq, Vec3 const& rv3SourcePos)const;

private:

	CAreaManager *m_pAreaManager;
	float	m_fProximity;
	float m_fMaximumEffectRadius;

	//	attached entities IDs list
	std::vector<EntityId>			m_vEntityID;
	SEntityEvent							m_CachedEvent;

	float	m_PrevFade;

	int				m_AreaID;
	int				m_AreaGroupID;
	EntityId	m_EntityID;
	int				m_nPriority;

	Matrix34	m_InvMatrix;

	EEntityAreaType	m_AreaType;
	SCachedAreaData m_oCachedAreaData;

	// for shape areas ----------------------------------------------------------------------
	// area's bbox
	a2DBBox	m_AreaBBox;
	// the area segments
	std::vector<a2DSegment*>	m_vpSegments;

	// for sector areas ----------------------------------------------------------------------
	//	int	m_Building;
	//	int	m_Sector;
	//	IVisArea *m_Sector;

	// for box areas ----------------------------------------------------------------------
	Vec3 m_BoxMin;
	Vec3 m_BoxMax;

	// for sphere areas ----------------------------------------------------------------------
	Vec3 m_SphereCenter;
	float	m_SphereRadius;
	float	m_SphereRadius2;


	//	area vertical origin - the lowest point of area
	float	m_VOrigin;
	//	area height (vertical size). If (m_VSize<=0) - not used, only 2D check is done. Otherwise 
	//	additional check for Z to be in [m_VOrigin, m_VOrigin + m_VSize] range is done
	float	m_VSize;

	int		m_stepID;

	bool		m_bIsActive:1;
	bool		m_bInitialized:1;
	bool		m_bHasSoundAttached:1;
	bool		m_bAttachedSoundTested:1; // can be replaced later with an OnAfterLoad
	bool		m_bObstructRoof:1;
	bool		m_bObstructFloor:1;
};

#endif //__Area_h__
