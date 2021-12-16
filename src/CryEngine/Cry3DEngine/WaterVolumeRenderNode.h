#ifndef _WATERVOLUME_RENDERNODE_
#define _WATERVOLUME_RENDERNODE_

#pragma once


#include "VertexFormats.h"


struct SWaterVolumeSerialize
{
	// volume type and id
	int32 m_volumeType;
	uint64 m_volumeID;

	// material
	IMaterial * m_pMaterial;	

	// fog properties
	f32 m_fogDensity;
	Vec3 m_fogColor;
	Plane m_fogPlane;

	f32 m_volumeDepth;
	f32 m_streamSpeed;
	bool m_capFogAtVolumeDepth;

	// render geometry 
	f32 m_uTexCoordBegin;
	f32 m_uTexCoordEnd;
	f32 m_surfUScale;
	f32 m_surfVScale;
	typedef std::vector< Vec3 > VertexArraySerialize;
	VertexArraySerialize m_vertices;

	// physics properties
	typedef std::vector< Vec3 > PhysicsAreaContourSerialize;
	PhysicsAreaContourSerialize m_physicsAreaContour;


	size_t GetMemoryUsage() const
	{
		size_t dynSize( sizeof( *this ) );
		dynSize += m_vertices.capacity() * sizeof( VertexArraySerialize::value_type );
		dynSize += m_physicsAreaContour.capacity() * sizeof( PhysicsAreaContourSerialize::value_type );
		return dynSize;
	}
};


class CWaterVolumeRenderNode : public IWaterVolumeRenderNode, public Cry3DEngineBase
{
public:
	// implements IWaterVolumeRenderNode	
	VIRTUAL void SetFogDensity( float fogDensity );
	VIRTUAL float GetFogDensity() const;
	VIRTUAL void SetFogColor( const Vec3& fogColor );

	VIRTUAL void SetCapFogAtVolumeDepth(bool capFog);
	VIRTUAL void SetVolumeDepth(float volumeDepth);
	VIRTUAL void SetStreamSpeed(float streamSpeed);

	VIRTUAL void CreateOcean( uint64 volumeID, /* TBD */ bool keepSerializationParams = false );
	VIRTUAL void CreateArea( uint64 volumeID, const Vec3* pVertices, unsigned int numVertices, const Vec2& surfUVScale, const Plane& fogPlane, bool keepSerializationParams = false );
	VIRTUAL void CreateRiver( uint64 volumeID, const Vec3* pVertices, unsigned int numVertices, float uTexCoordBegin, float uTexCoordEnd, const Vec2& surfUVScale, const Plane& fogPlane, bool keepSerializationParams = false );	

	VIRTUAL void SetAreaPhysicsArea( const Vec3* pVertices, unsigned int numVertices, bool keepSerializationParams = false );
	VIRTUAL void SetRiverPhysicsArea( const Vec3* pVertices, unsigned int numVertices, bool keepSerializationParams = false );

	// implements IRenderNode
	virtual EERType GetRenderNodeType();
	virtual const char* GetEntityClassName() const;
	virtual const char* GetName() const;
	virtual Vec3 GetPos( bool bWorldOnly = true ) const;
	virtual void Render( const SRendParams &rParam );
	virtual void SetMaterial( IMaterial* pMat );
	virtual IMaterial* GetMaterial( Vec3* pHitPos );
	virtual IMaterial* GetMaterialOverride() { return m_pMaterial; }
	virtual float GetMaxViewDist();
	virtual void GetMemoryUsage( ICrySizer* pSizer );
	virtual void Precache();

	virtual IPhysicalEntity* GetPhysics() const;
	virtual void SetPhysics( IPhysicalEntity* );

	virtual void CheckPhysicalized();
	virtual void Physicalize( bool bInstant = false );
	virtual void Dephysicalize( bool bKeepIfReferenced=false );

  virtual const AABB GetBBox() const { return m_WSBBox; }
  virtual void SetBBox( const AABB& WSBBox ) { m_WSBBox = WSBBox; }

public:
	CWaterVolumeRenderNode();
	const SWaterVolumeSerialize* GetSerializationParams();

private:
	typedef std::vector< SVF_P3F_C4B_T2F > WaterSurfaceVertices;
	typedef std::vector< uint16 > WaterSurfaceIndices;
		
	struct SPhysAreaInput
	{
		typedef std::vector< Vec3 > PhysicsVertices;
		typedef std::vector< int > PhysicsIndices;

		PhysicsVertices m_contour;
		PhysicsVertices m_flowContour;
		PhysicsIndices m_indices;

		size_t GetMemoryUsage() const
		{
			size_t dynSize( sizeof( *this ) );
			dynSize += m_contour.capacity() * sizeof( PhysicsVertices::value_type );
			dynSize += m_flowContour.capacity() * sizeof( PhysicsVertices::value_type );
			dynSize += m_indices.capacity() * sizeof( PhysicsIndices::value_type );
			return dynSize;
		}
	};

private:
	~CWaterVolumeRenderNode();

	float GetCameraDistToWaterVolumeSurface() const;
	bool IsCameraInsideWaterVolumeSurface2D() const;

	void UpdateBoundingBox();
	
	void CopyVolatilePhysicsAreaContourSerParams( const Vec3* pVertices, unsigned int numVertices );
	void CopyVolatileRiverSerParams( const Vec3* pVertices, unsigned int numVertices, float uTexCoordBegin, float uTexCoordEnd, const Vec2& surfUVScale );

	void CopyVolatileAreaSerParams( const Vec3* pVertices, unsigned int numVertices, const Vec2& surfUVScale );

private:
	IWaterVolumeRenderNode::EWaterVolumeType m_volumeType;
	uint64 m_volumeID;

	float m_volumeDepth;
	float m_streamSpeed;

	bool m_capFogAtVolumeDepth;
	
	CREWaterVolume::SParams m_wvParams;	
	
	_smart_ptr< IMaterial > m_pMaterial;
	_smart_ptr< IMaterial > m_pWaterBodyIntoMat;
	_smart_ptr< IMaterial > m_pWaterBodyOutofMat;
	
	CREWaterVolume* m_pVolumeRE;
	CREWaterVolume* m_pSurfaceRE;
	SWaterVolumeSerialize* m_pSerParams;
	
	SPhysAreaInput* m_pPhysAreaInput;
	IPhysicalEntity* m_pPhysArea;

	WaterSurfaceVertices m_waterSurfaceVertices;
	WaterSurfaceIndices m_waterSurfaceIndices;
  AABB m_WSBBox;
};


#endif // #ifndef _DECAL_RENDERNODE_
