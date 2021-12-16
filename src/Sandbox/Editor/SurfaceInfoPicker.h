////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2012.
// -------------------------------------------------------------------------
//  File name:   SurfaceInfoPicker.h
//  Version:     v1.00
//  Created:     Feb/21/2012 by Jaesik.
//  Compilers:   Visual Studio 2010
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SurfaceInfoPicker_h__
#define __SurfaceInfoPicker_h__

#if _MSC_VER > 1000
#pragma once
#endif

class CKDTree;

//////////////////////////////////////////////////////////////////////////
class SANDBOX_API CSurfaceInfoPicker
{
public:

	CSurfaceInfoPicker();

	class CExcludedObjects
	{
	public:

		CExcludedObjects(){}
		~CExcludedObjects(){}
		CExcludedObjects( const CExcludedObjects& excluded )
		{
			objects = excluded.objects;
		}

		void Add( CBaseObject* pObject )
		{
			objects.insert(pObject);
		}

		void Clear()
		{
			objects.clear();
		}

		bool Contains( CBaseObject* pObject ) const
		{
			return objects.find(pObject) != objects.end();
		}

	private:
		std::set<CBaseObject*> objects;
	};

	enum EPickedObjectGroup
	{
		ePOG_BrushObject = BIT(0), 
		ePOG_Prefabs = BIT(1),
		ePOG_Solid = BIT(2),
		ePOG_DesignerObject	= BIT(3),
		ePOG_Vegetation = BIT(4),
		ePOG_Entity = BIT(5),
		ePOG_Terrain = BIT(6),
		ePOG_GeneralObjects = ePOG_BrushObject|ePOG_Prefabs|ePOG_Solid|ePOG_DesignerObject|ePOG_Entity,
		ePOG_All = 0xFFFFFFFF,
	};


public:

	bool PickObject( const CPoint& point,
		SRayHitInfo& outHitInfo,
		CBaseObject* pObject );

	bool PickObject( const Vec3& vWorldRaySrc, const Vec3& vWorldRayDir,
		SRayHitInfo& outHitInfo,
		CBaseObject* pObject );

	bool Pick( const Vec3& vWorldRaySrc, const Vec3& vWorldRayDir,
		IMaterial*& ppOutLastMaterial,
		SRayHitInfo& outHitInfo,
		CExcludedObjects* pExcludedObjects = NULL,
		int nFlag = ePOG_All )
	{
		return PickImpl(vWorldRaySrc, vWorldRayDir, &ppOutLastMaterial, outHitInfo, pExcludedObjects, nFlag);
	}

	bool Pick( const Vec3& vWorldRaySrc, const Vec3& vWorldRayDir,
		SRayHitInfo& outHitInfo,
		CExcludedObjects* pExcludedObjects = NULL,
		int nFlag = ePOG_All )
	{
		return PickImpl(vWorldRaySrc, vWorldRayDir, NULL, outHitInfo, pExcludedObjects, nFlag);
	}

	bool Pick( const CPoint& point,
		SRayHitInfo& outHitInfo,
		CExcludedObjects* pExcludedObjects = NULL,
		int nFlag = ePOG_All )
	{
		return PickImpl(point, NULL, outHitInfo, pExcludedObjects, nFlag);
	}

	bool Pick( const CPoint& point,
		IMaterial*& ppOutLastMaterial,
		SRayHitInfo& outHitInfo,
		CExcludedObjects* pExcludedObjects = NULL,
		int nFlag = ePOG_All )
	{
		return PickImpl(point, &ppOutLastMaterial, outHitInfo, pExcludedObjects, nFlag);
	}

	bool PickByAABB( const CPoint& point, int nFlag = ePOG_All, IDisplayViewport* pView = NULL, CExcludedObjects* pExcludedObjects = NULL, std::vector<CBaseObjectPtr>* pOutObjects = NULL );

	void SetObjects(CBaseObjectsArray* pSetObjects)	{	m_pSetObjects = pSetObjects; }

	CBaseObjectPtr GetPickedObject(){
		return m_pPickedObject;
	}

	void SetActiveView( IDisplayViewport* view );

public:

	static bool RayWorldToLocal( 
		const Matrix34A& WorldTM, 
		const Vec3& vWorldRaySrc, 
		const Vec3& vWorldRayDir, 
		Vec3& outRaySrc, 
		Vec3& outRayDir );

private:

	bool PickImpl( const CPoint& point,
		IMaterial** ppOutLastMaterial,
		SRayHitInfo& outHitInfo,
		CExcludedObjects* pExcludedObjects = NULL,
		int nFlag = ePOG_All );

	bool PickImpl( const Vec3& vWorldRaySrc, const Vec3& vWorldRayDir,
		IMaterial** ppOutLastMaterial,
		SRayHitInfo& outHitInfo,
		CExcludedObjects* pExcludedObjects = NULL,
		int nFlag = ePOG_All );

	void FindNearestInfoFromBrushObjects( 
		const Vec3& vWorldRaySrc, 
		const Vec3& vWorldRayDir, 		
		IMaterial** ppOutLastMaterial, 
		SRayHitInfo& outHitInfo ) const;

	void FindNearestInfoFromPrefabs( 
		const Vec3& vWorldRaySrc, 
		const Vec3& vWorldRayDir, 		
		IMaterial** ppOutLastMaterial, 
		SRayHitInfo& outHitInfo ) const;

	void FindNearestInfoFromSolids( int nFlag, 
		const Vec3& vWorldRaySrc, 
		const Vec3& vWorldRayDir,
		IMaterial** ppOutLastMaterial,
		SRayHitInfo& outHitInfo ) const;

	void FindNearestInfoFromVegetations(		
		const Vec3& vWorldRaySrc, 
		const Vec3& vWorldRayDir, 
		IMaterial** ppOutLastMaterial, 
		SRayHitInfo& outHitInfo ) const;

	void FindNearestInfoFromDecals( 
		const Vec3& vWorldRaySrc, 
		const Vec3& vWorldRayDir, 		
		IMaterial** ppOutLastMaterial, 
		SRayHitInfo& outHitInfo ) const;

	void FindNearestInfoFromEntities( 
		const Vec3& vWorldRaySrc, 
		const Vec3& vWorldRayDir, 		
		IMaterial** ppOutLastMaterial, 
		SRayHitInfo& outHitInfo) const;

	void FindNearestInfoFromTerrain( 
		const Vec3& vWorldRaySrc, 
		const Vec3& vWorldRayDir, 
		IMaterial** ppOutLastMaterial, 
		SRayHitInfo& outHitInfo ) const;

	static bool RayIntersection( 
		const Vec3& vWorldRaySrc, 
		const Vec3& vWorldRayDir, 
		IRenderNode* pRenderNode,
		IEntity* pEntity,
		IStatObj* pStatObj, 
		const Matrix34A& WorldTM, 
		SRayHitInfo& outHitInfo,
		IMaterial** ppOutLastMaterial );

	static bool RayIntersection( 
		const Vec3& vWorldRaySrc, 
		const Vec3& vWorldRayDir, 
		IStatObj* pStatObj, 
		IMaterial** ppOutLastMaterial, 
		const Matrix34A& WorldTM, 
		SRayHitInfo& outHitInfo );

	static bool RayIntersection(
		const Vec3& vWorldRaySrc,
		const Vec3& vWorldRayDir,
		IGeomCacheRenderNode* pGeomCacheRenderNode,
		IMaterial** ppOutLastMaterial, 
		const Matrix34A& worldTM,
		SRayHitInfo& outHitInfo );

	static bool RayIntersection( 
		const Vec3& vWorldRaySrc, 
		const Vec3& vWorldRayDir,
		IRenderNode* pRenderNode,
		IMaterial** ppOutLastMaterial, 
		const Matrix34A& WorldTM, 
		SRayHitInfo& outHitInfo );	

	static bool RayIntersection(
		const Vec3& vWorldRaySrc,
		const Vec3& vWorldRayDir,
		IEntity* pEntity,
		IMaterial** ppOutLastMaterial, 
		const Matrix34A& WorldTM,
		SRayHitInfo& outHitInfo );

	static bool RayIntersection( 
		const Vec3& vWorldRaySrc, 
		const Vec3& vWorldRayDir, 
		CBaseObject* pBaseObject,
		IMaterial** ppOutLastMaterial, 
		SRayHitInfo& outHitInfo );

	static void AssignObjectMaterial( CBaseObject* pObject, const SRayHitInfo& outHitInfo, IMaterial** pOutMaterial );
	static bool IsMaterialValid( CMaterial* pMaterial );	

private:
	CBaseObjectsArray* m_pObjects;
	CBaseObjectsArray* m_pSetObjects;
	CBaseObjectsArray m_objects;
	IDisplayViewport* m_pActiveView;
	CExcludedObjects m_ExcludedObjects;
	mutable CBaseObjectPtr m_pPickedObject;

};

#endif __SurfaceInfoPicker_h__
