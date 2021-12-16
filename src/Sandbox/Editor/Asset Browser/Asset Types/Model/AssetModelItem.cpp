#include "StdAfx.h"
#include "AssetModelItem.h"
#include "AssetModelDatabase.h"
#include "..\Util/PathUtil.h"
#include "..\Include/IAssetViewer.h"
#include "StringUtils.h"
#include "IMaterial.h"

const float kAssetViewer_InteractiveRenderModelTranslateMultiplier = 0.02f; 
const float kAssetViewer_InteractiveRenderModelRotateMultiplier = 0.02f; 
const float kAssetViewer_InteractiveRenderModelZoomMultiplier = 0.006f;
const UINT	kAssetViewer_MaxDependencyFilesInTooltip = 5;

static const int kAssetViewer_MinWidthForOverlayText = 256;
static const int kAssetViewer_OverlayTextTopMargin = 10;
static const int kAssetViewer_OverlayTextLeftMargin = 10;

//! used in GetAssetFieldValue to replace the ugly strcmp
//! fieldName is constant string, used in the 'switch' of IAssetDisplay::GetAssetFieldValue
#define isAssetField( fieldName ) !strncmp( pFieldName, fieldName, strlen( pFieldName ) )

UINT CAssetModelItem::m_frameBufferScreenshot[CAssetModelItem::kAssetDisplay_MaxThumbImageBufferSize];

CAssetModelItem::CAssetModelItem():
m_strFilename(""),
m_strExtension(""),
m_strRelativePath(""),
m_bUsedInLevel(false),
m_nFileSize(0),
m_oDrawingRectangle( 0, 0, 0, 0 ),
m_piOwnerDatabase(NULL),
m_ref(1),
m_camTarget(0,0,0),
m_camRadius(1),
m_pObject(NULL),
m_pRenderer(gEnv->pRenderer),
m_bGrid(false),
m_bAxis(true),
m_clearColor( 0.47f, 0.63f, 0.8f ),
m_bRotate(false),
m_rotateAngle(0),
m_bShowObject(true),
m_fov(60),
m_flags( eAssetFlags_InteractiveRenderSupported ),
m_rotationX(10),
m_rotationY(70),
m_translateX(0),
m_translateY(0),
m_camZoom(1),
m_assetIndex(0),
m_lodCount(0),
m_triangleCount(0), 
m_vertexCount(0), 
m_submeshCount(0),
m_textureSize(0), 
m_nRef(0), 
m_physicsTriCount(0), 
m_physicsSize(0),
m_bSplitLODs(false)
{
	m_aabb.Reset();
	m_crtRotation.CreateIdentity();
	memset(m_triCountLOD, 0, sizeof(m_triCountLOD));

	CDLight light;

	light.m_Origin = Vec3( 1, 1, -1 );
	light.SetLightColor( ColorF( 1, 1, 1, 1 ) );
	light.m_SpecMult = 1.0f;
	light.m_fRadius = 1000;
	light.m_Flags |= DLF_DIRECTIONAL;
	m_lights.push_back( light );
}

CAssetModelItem::~CAssetModelItem()
{
}

HRESULT STDMETHODCALLTYPE CAssetModelItem::QueryInterface( const IID &riid, void **ppvObj ) 
{ 
	if( riid == __uuidof(IAssetDisplay)/* && m_pIntegrator*/ )
	{
		*ppvObj = this;
		return S_OK;
	}
	
	return E_NOINTERFACE; 
}

ULONG STDMETHODCALLTYPE CAssetModelItem::AddRef()
{
	return ++m_ref; 
}

ULONG STDMETHODCALLTYPE CAssetModelItem::Release() 
{ 
	if( (--m_ref) == 0 )
	{
		FreeData();
		delete this;
		return 0; 
	}
	else
		return m_ref;
}

void CAssetModelItem::FreeData()
{
	UnCache();
	UnCacheThumbnail();
}

IAssetDisplayDatabase*	CAssetModelItem::GetOwnerDisplayDatabase()
{
	return m_piOwnerDatabase;
}

void CAssetModelItem::SetOwnerDisplayDatabase( IAssetDisplayDatabase* piOwnerDisplayDatabase )
{
	if( !m_piOwnerDatabase )
		m_piOwnerDatabase = piOwnerDisplayDatabase;
}

const std::vector<string>& CAssetModelItem::GetDependencies()
{
	return m_dependencies;
}

bool CAssetModelItem::Cache()
{
	if( m_flags & eAssetFlags_Cached )
		return true;

	if( !m_pObject )
	{
		IStatObj::SSubObject* pSubObj = NULL;

		m_pObject = gEnv->p3DEngine->LoadStatObj( m_strRelativePath + m_strFilename, NULL, &pSubObj, FALSE );

		if( !m_pObject )
		{
			return false;
		}

		m_pObject->AddRef();
		m_aabb.min = m_pObject->GetBoxMin();
		m_aabb.max = m_pObject->GetBoxMax();

		IStatObj::SStatistics stats;
		stats.pTextureSizer = gEnv->pSystem->CreateSizer();

		m_pObject->GetStatistics( stats );

		if( !stats.nIndices || !stats.nVertices )
		{
			SAFE_RELEASE(m_pObject);
			return false;
		}

		m_lodCount = stats.nLods;
		m_triangleCount = stats.nIndices / 3;
		m_vertexCount = stats.nVertices;
		m_submeshCount = stats.nSubMeshCount;
		m_nRef = stats.nNumRefs;
		m_physicsTriCount = stats.nPhysPrimitives;
		m_physicsSize = (stats.nPhysProxySize + 512) / 1024;	// In KBs
		m_bSplitLODs = stats.bSplitLods;
		m_textureSize = stats.pTextureSizer->GetTotalSize() / 1024;	// In KBs
		for( int i = 0; i < MAX_STATOBJ_LODS_NUM; ++i )
			m_triCountLOD[i] = stats.nIndicesPerLod[i] / 3;
		stats.pTextureSizer->Release();

		size_t subMtls = m_pObject->GetMaterial()->GetSubMtlCount();
		IMaterial* pMtl = NULL;
		m_dependencies.resize( 0 );

		if( !subMtls )
		{
			pMtl = m_pObject->GetMaterial();

			// fill up dependencies
			if( pMtl )
			{
				IRenderShaderResources* pShaderRes = pMtl->GetShaderItem().m_pShaderResources;

				if( pShaderRes )
				{
					for( size_t j = 0; SEfResTexture* pTex = pShaderRes->GetTexture( j ); ++j )
					{
						m_dependencies.push_back( pTex->m_Name );
					}
				}
			}
		}
		else
		for( size_t s = 0; s < subMtls; ++s )
		{
			IMaterial* pMtl = m_pObject->GetMaterial()->GetSubMtl( s );
			
			// fill up dependencies
			if( pMtl )
			{
				IRenderShaderResources* pShaderRes = pMtl->GetShaderItem().m_pShaderResources;
				
				if( pShaderRes )
				{
					for( size_t j = 0; SEfResTexture* pTex = pShaderRes->GetTexture( j ); ++j )
					{
						m_dependencies.push_back( pTex->m_Name );
					}
				}
			}
		}
	}

	SetFlag( IAssetDisplay::eAssetFlags_Cached, true );
	SetFlag( IAssetDisplay::eAssetFlags_CachedFieldsInfo, true );

	return true;
}

bool CAssetModelItem::CacheFieldsInfo()
{
	if( m_flags & eAssetFlags_CachedFieldsInfo )
	{
		return true;
	}

	Cache();
	UnCache();

	return true;
}

bool CAssetModelItem::UnCache()
{
	if( !( m_flags & eAssetFlags_Cached ) )
	{
		return true;
	}

	SetFlag( IAssetDisplay::eAssetFlags_Cached, false );
	SAFE_RELEASE( m_pObject );

	return true;
}

bool CAssetModelItem::UnCacheThumbnail()
{
	if( !( m_flags & eAssetFlags_ThumbnailCached ) )
	{
		return true;
	}

	SetFlag( eAssetFlags_ThumbnailCached, false );
	m_oCachedBmp.Free();

	return true;
}

void CAssetModelItem::InteractiveRender( HWND hRenderWindow, const CRect& rstViewport, int aMouseX, int aMouseY, int aMouseDeltaX, int aMouseDeltaY, UINT aKeyFlags )
{
	// this way we can zoom and pan in the same time
	if( ( aKeyFlags & MK_SHIFT ) || ( aKeyFlags & MK_CONTROL ) )
	{
		if( aKeyFlags & MK_SHIFT )
		{
			m_camZoom += (float) aMouseDeltaX * kAssetViewer_InteractiveRenderModelZoomMultiplier;
		}

		if( aKeyFlags & MK_CONTROL )
		{
			m_translateX += (float) aMouseDeltaX * kAssetViewer_InteractiveRenderModelTranslateMultiplier;
			m_translateY += (float) aMouseDeltaY * kAssetViewer_InteractiveRenderModelTranslateMultiplier;
		}
	}
	else
	{
		m_rotationY += (float) aMouseDeltaX * kAssetViewer_InteractiveRenderModelRotateMultiplier;
		m_rotationX += (float) aMouseDeltaY * kAssetViewer_InteractiveRenderModelRotateMultiplier;
	}

	Render( hRenderWindow, rstViewport, false );
}

bool CAssetModelItem::Render( HWND hRenderWindow, const CRect& rstViewport, bool bCacheThumbnail )
{
	if( !( m_flags & eAssetFlags_Cached ) )
	{
		return false;
	}

	if( !m_pObject )
	{
		return false;
	}

	m_pRenderer->SetCurrentContext( hRenderWindow );
	m_pRenderer->ChangeViewport( rstViewport.left, rstViewport.top, rstViewport.Width(), rstViewport.Height() );
	m_pRenderer->SetClearColor( Vec3( m_clearColor.r, m_clearColor.g, m_clearColor.b ) );
	m_pRenderer->ClearBuffer( FRT_CLEAR, &m_clearColor );
	m_pRenderer->BeginFrame();

	CalculateCameraPosition();
	SetCamera( m_camera, rstViewport);

	if( m_bGrid )
	{
		DrawGrid();
		m_pRenderer->GetIRenderAuxGeom()->Flush();
	}

	// Render object.
	m_pRenderer->EF_StartEf();
	m_pRenderer->ResetToDefault();
	m_pRenderer->SetWireframeMode( R_SOLID_MODE );

	// Add lights.
	for( int i = 0; i < m_lights.size(); i++ )
	{
		m_pRenderer->EF_ADDDlight( &m_lights[i] );
	}

	_smart_ptr<IMaterial> pMaterial = NULL;

	SRendParams rp;
	rp.nDLightMask = 0x3;
	rp.AmbientColor = ColorF( 1, 1, 1, 1 );
	rp.dwFObjFlags |= FOB_TRANS_MASK;
	rp.pMaterial = pMaterial;

	Matrix34 tm;
	tm.SetIdentity();
	rp.pMatrix = &tm;

	if( m_bShowObject )
	{
		if( m_pObject )
		{
			m_pObject->Render( rp );
		}

		//if (m_entity)
		//	m_entity->Render( rp );

		//if (m_character)
		//	m_character->Render( rp, QuatTS(IDENTITY)  );

		//if (m_pEmitter)
		//{
		//	m_pEmitter->Update();
		//	m_pEmitter->Render( rp );
		//}
	}

	m_pRenderer->EF_EndEf3D( SHDF_SORT | SHDF_NOASYNC | SHDF_STREAM_SYNC, -1 );
	m_pRenderer->FlushTextMessages();
	m_pRenderer->RenderDebug();
	m_pRenderer->EndFrame();

	if( bCacheThumbnail )
	{
		m_pRenderer->ReadFrameBufferFast( m_frameBufferScreenshot, gSettings.sAssetBrowserSettings.nThumbSize, gSettings.sAssetBrowserSettings.nThumbSize );
		m_oCachedBmp.Create( m_frameBufferScreenshot, gSettings.sAssetBrowserSettings.nThumbSize, gSettings.sAssetBrowserSettings.nThumbSize, true );
		m_flags |= eAssetFlags_ThumbnailCached;
		
		// lets push this asset to the thumbs pool/queue, so we can have a pool of thumbs kept in memory until they are too old
		// this help browsing the assets, no need to recache each time a row of assets thumbs previously 
		if( m_piOwnerDatabase && m_piOwnerDatabase->GetAssociatedViewer() )
		{
			m_piOwnerDatabase->GetAssociatedViewer()->PushToThumbsCacheQueue( this );
		}
	}

	// Restore main context.
	m_pRenderer->MakeMainContextActive();

	return true;
}

void	CAssetModelItem::CalculateCameraPosition()
{
	AABB		box(m_aabb);
	Vec3		stCameraDirection(0.0f,0.0f,0.0f);
	float		fCameraDistance(0);
	float		afAABBDimensions[3];
	float		afRatios[3];
	float   fSmallestRatio(FLT_MAX);
	float		afCameraDistances[2];
	float		fSign(-1.0f);
	int32		nCount(0);
	int32		nSmallestRationIndex(0);
	int32		nCameraDimensionIndex(0);
	// If m_fov==45+180*K, FPE.
	float		fHalfFovTangent(tanf(DEG2RAD(m_fov/2.0f)));

	for (nCount=0;nCount<3;++nCount)
	{
		afAABBDimensions[nCount]=box.max[nCount]-box.min[nCount];
	}

	for (nCount=0;nCount<3;++nCount)
	{
		if (afAABBDimensions[(nCount+1)%3]!=0)
		{
			afRatios[nCount]=fabs(fabs(afAABBDimensions[nCount]/afAABBDimensions[(nCount+1)%3])-1.0f);
		}
		else
		{
			afRatios[nCount]=FLT_MAX;
		}
	}

	for (nCount=0;nCount<3;++nCount)
	{
		if (fSmallestRatio<afRatios[nCount])
		{
			fSmallestRatio=afRatios[nCount];
			nSmallestRationIndex=nCount;
		}
	}

	nSmallestRationIndex = 2;
	nCameraDimensionIndex = (nSmallestRationIndex + 2) % 3;
	
	if( ( ( nCameraDimensionIndex + 1 ) % 3 ) == 2 )
	{
		fSign *= fSign;
	}

	stCameraDirection[nCameraDimensionIndex] = 1.0f * fSign;

	// The placement of the camera must be calculated from a point
	// on the top of the bounding box.
	//if (box.min[nCameraDimensionIndex]>box.max[nCameraDimensionIndex])
	//{
	//	box.min[nCameraDimensionIndex]=box.max[nCameraDimensionIndex];
	//}
	//else
	//{
	//	box.max[nCameraDimensionIndex]=box.min[nCameraDimensionIndex];
	//}

	// We want to look at the center of the front face of the AABB.
	m_camTarget = ( box.max + box.min ) * 0.5f;

	afCameraDistances[0] = m_camTarget[(nCameraDimensionIndex+1) % 3] - m_aabb.min[(nCameraDimensionIndex + 1)%3];
	afCameraDistances[1] = m_camTarget[(nCameraDimensionIndex+2) % 3] - m_aabb.min[(nCameraDimensionIndex + 2)%3];
	fCameraDistance = MAX( afCameraDistances[0], afCameraDistances[1] ) / fHalfFovTangent;
	fCameraDistance = box.GetRadius();
	stCameraDirection.Set( 0.0f, -1.0f, 0.0f );

	//fCameraDistance = box.max.GetDistance( box.min );
	//stCameraDirection = box.max + Vec3( 0, 0, fCameraDistance*0.3f ) - box.min;
	//stCameraDirection.Normalize();

	if( fCameraDistance < m_camera.GetNearPlane() )
	{
		fCameraDistance = m_camera.GetNearPlane();
	}

	// grow a little bit, further from camera center, add padding
	fCameraDistance *= 1.8f;
	Vec3 pos = stCameraDirection;
	Matrix33 rot;

	rot.SetIdentity();
	rot.SetRotationXYZ( Ang3( m_rotationX, 0, -m_rotationY ) );
	stCameraDirection = -rot.TransformVector( pos );
	Matrix34 tm = Matrix33::CreateRotationVDir( stCameraDirection, 0 );
	
	Vec3 vLeft = Vec3( -1, 0, 0 );
	Vec3 vUp = Vec3( 0, 0, 1 );
	
	vLeft = tm.TransformVector( vLeft );
	vUp = tm.TransformVector( vUp );

	Vec3 panVector = vUp*m_translateY + vLeft*m_translateX;
	m_camTarget += panVector;
	tm.SetTranslation( m_camTarget - stCameraDirection*fCameraDistance*m_camZoom + panVector );
	m_camera.SetMatrix( tm );
}

void CAssetModelItem::DrawGrid()
{
	float step = 0.1f;
	float XR = 5;
	float YR = 5;

	IRenderAuxGeom * pRag = m_pRenderer->GetIRenderAuxGeom();
	SAuxGeomRenderFlags nRendFlags = pRag->GetRenderFlags();

	pRag->SetRenderFlags( e_Def3DPublicRenderflags );
	SAuxGeomRenderFlags nNewFlags = pRag->GetRenderFlags();
	nNewFlags.SetAlphaBlendMode( e_AlphaBlended );
	pRag->SetRenderFlags( nNewFlags );

	int nGridAlpha = 40;
	if (m_bGrid)
	{
		// Draw grid.
		for (float x = -XR; x < XR; x+=step)
		{
			if (fabs(x) > 0.01)
				//m_pRenderer->DrawLine( Vec3(x,-YR,0),Vec3(x,YR,0) );
				pRag->DrawLine( Vec3(x,-YR,0), ColorB(150, 150, 150 , nGridAlpha), Vec3(x,YR,0) , ColorB(150, 150, 150 , nGridAlpha) );
		}
		for (float y = -YR; y < YR; y+=step)
		{
			if (fabs(y) > 0.01)
				//m_pRenderer->DrawLine( Vec3(-XR,y,0),Vec3(XR,y,0) );
				pRag->DrawLine( Vec3(-XR,y,0), ColorB(150, 150, 150 , nGridAlpha), Vec3(XR,y,0) , ColorB(150, 150, 150 , nGridAlpha) );
		}
	}

	nGridAlpha = 60;
	if (m_bAxis)
	{
		// Draw axis.
		//m_pRenderer->SetMaterialColor( 1,0,0,1.0f );
		//m_pRenderer->DrawLine( Vec3(-XR,0,0),Vec3(XR,0,0) );
		pRag->DrawLine( Vec3(0,0,0), ColorB(255, 0, 0 ,nGridAlpha), Vec3(XR,0,0) , ColorB(255, 0, 0 ,nGridAlpha) );

		//m_pRenderer->SetMaterialColor( 0,1,0,1.0f );
		//m_pRenderer->DrawLine( Vec3(0,-YR,0),Vec3(0,YR,0) );
		pRag->DrawLine( Vec3(0,0,0), ColorB(0, 255, 0 ,nGridAlpha), Vec3(0,YR,0) , ColorB(0, 255, 0 ,nGridAlpha) );

		//m_pRenderer->SetMaterialColor( 0,0,1,1.0f );
		//m_pRenderer->DrawLine( Vec3(0,0,-YR),Vec3(0,0,YR) );
		pRag->DrawLine(Vec3(0,0,0), ColorB(0, 0, 255 ,nGridAlpha), Vec3(0,0,YR) , ColorB(0, 0, 255 ,nGridAlpha) );
	}
	pRag->SetRenderFlags( nRendFlags );
}

void	CAssetModelItem::SetCamera( CCamera &cam,const CRect &rcViewportRect )
{
	int w = rcViewportRect.Width();
	int h = rcViewportRect.Height();

	m_camera.SetPosition( cam.GetPosition() );
	m_camera.SetFrustum( w, h, DEG2RAD( m_fov ), m_camera.GetNearPlane(), m_camera.GetFarPlane() );
	m_camera.SetViewPort( rcViewportRect.left, rcViewportRect.top, rcViewportRect.Width(), rcViewportRect.Height() );
	m_pRenderer->SetCamera( m_camera );
}

void CAssetModelItem::SetFileSize( unsigned __int64 aSize )
{
	m_nFileSize = aSize;
}

void CAssetModelItem::SetFileExtension( const char* pExt )
{
	m_strExtension = pExt;
}

const char* CAssetModelItem::GetFileExtension()
{
	return m_strExtension.c_str();
}

unsigned __int64 CAssetModelItem::GetFileSize()
{
	return m_nFileSize;
}

void CAssetModelItem::SetFilename( const char* pName )
{
	m_strFilename = pName;
}

const char* CAssetModelItem::GetFilename()
{
	return m_strFilename.c_str();
}

void CAssetModelItem::SetRelativePath( const char* pPath )
{
	m_strRelativePath = pPath;
}

const char* CAssetModelItem::GetRelativePath()
{
	return m_strRelativePath;
}

UINT CAssetModelItem::GetFlags() const
{
	return m_flags;
}

void CAssetModelItem::SetFlags( UINT aFlags )
{
	m_flags = aFlags;
}

void CAssetModelItem::SetFlag( EAssetFlags aFlag, bool bSet )
{
	m_flags = bSet ? ( m_flags | (UINT)aFlag ) : ( m_flags & (~(UINT)aFlag) );
}

bool CAssetModelItem::IsFlagSet( EAssetFlags aFlag )
{
	return m_flags & (UINT) aFlag;
}

void CAssetModelItem::SetIndex( UINT aIndex )
{
	m_assetIndex = aIndex;
}

UINT CAssetModelItem::GetIndex()
{
	return m_assetIndex;
}

bool CAssetModelItem::GetAssetFieldValue( const char* pFieldName, void* pDest )
{
	if( isAssetField( "thumbTipInfo" ) )
	{
		stack_string str;
		string dependencies = "\n    ", filename, fileExt;

		for( size_t i = 0, iCount = min( kAssetViewer_MaxDependencyFilesInTooltip, m_dependencies.size() ); i < iCount; ++i )
		{
			filename = PathUtil::GetFile( m_dependencies[i].c_str() );
			dependencies += filename + string( "\n    " );
		}

		if( kAssetViewer_MaxDependencyFilesInTooltip - (int)m_dependencies.size() < 0 )
		{
			dependencies += "(more in dependency viewer)";
		}

		str.Format( "Path: %s\nLODs: %d\nTriangles: %d\nVertices: %d\nSubMeshes: %d\nBBox.X: %.4f\nBBox.Y: %.4f\nBBox.Z: %.4f\nFilesize: %.2f kB\nTextures (%d): %s", m_strRelativePath.c_str(),
								m_lodCount, m_triangleCount, m_vertexCount, m_submeshCount, m_aabb.GetSize().x, m_aabb.GetSize().y, m_aabb.GetSize().z, (float) m_nFileSize / 1024.0f, m_dependencies.size(), dependencies.c_str() );
		*(string*)pDest = str.c_str();
		return true;
	}
	else
	if( isAssetField( "thumbOneLineInfo" ) )
	{
		stack_string str;
		str.Format( "[ %d LODs %d tris ]", m_lodCount, m_triangleCount, m_vertexCount, m_submeshCount );
		*(string*)pDest = str.c_str();
		return true;
	}
	else
	if( isAssetField( "filename" ) )
	{
		*(string*)pDest = m_strFilename;
		return true;
	}
	else
	if( isAssetField( "relativepath" ) )
	{
		*(string*)pDest = m_strRelativePath;
		return true;
	}
	else
	if( isAssetField( "extension" ) )
	{
		*(string*)pDest = m_strExtension;
		return true;
	}
	else
	if( isAssetField( "filesize" ) )
	{
		*(int*)pDest = m_nFileSize;
		return true;
	}
	else
	if( isAssetField( "trianglecount" ) )
	{
		*(int*)pDest = m_triangleCount;
		return true;
	}
	else
	if( isAssetField( "vertexcount" ) )
	{
		*(int*)pDest = m_vertexCount;
		return true;
	}
	else
	if( isAssetField( "lodcount" ) )
	{
		*(int*)pDest = m_lodCount;
		return true;
	}
	else
	if( isAssetField( "submeshcount" ) )
	{
		*(int*)pDest = m_submeshCount;
		return true;
	}
	else
	if( isAssetField( "bbox_x" ) )
	{
		*(float*)pDest = m_aabb.GetSize().x;
		return true;
	}
	else
	if( isAssetField( "bbox_y" ) )
	{
		*(float*)pDest = m_aabb.GetSize().y;
		return true;
	}
	else
	if( isAssetField( "bbox_z" ) )
	{
		*(float*)pDest = m_aabb.GetSize().z;
		return true;
	}
	else
	if( isAssetField( "usedinlevel" ) )
	{
		*(string*)pDest = m_bUsedInLevel ? "Yes" : "No";
		return true;
	}
	else
	if( isAssetField( "nref" ) )
	{
		*(int*)pDest = m_nRef;
		return true;
	}
	else
	if( isAssetField( "texturesize" ) )
	{
		*(int*)pDest = m_textureSize;
		return true;
	}
	else
	if( isAssetField( "phytricount" ) )
	{
		*(int*)pDest = m_physicsTriCount;
		return true;
	}
	else
	if( isAssetField( "physize" ) )
	{
		*(int*)pDest = m_physicsSize;
		return true;
	}
	else
	if( isAssetField( "splitlods" ) )
	{
		*(string*)pDest = m_bSplitLODs ? "Yes" : "No";
		return true;
	}
	else
	if( isAssetField( "lodstricount" ) )
	{
		string lodsTris = MakeLODsTrisString();
		*(string*)pDest = lodsTris;
		return true;
	}

	return false;
}

void CAssetModelItem::GetDrawingRectangle( CRect& rstDrawingRectangle ) const
{ 
	rstDrawingRectangle = m_oDrawingRectangle;
}

void	CAssetModelItem::SetDrawingRectangle( const CRect& crstDrawingRectangle )
{
	m_oDrawingRectangle = crstDrawingRectangle;
}

bool CAssetModelItem::DrawThumbImage( HDC hDC, const CRect& rRect )
{
	if( m_flags & eAssetFlags_ThumbnailCached )
	{
		CDC dc;

		dc.Attach( hDC );
		dc.BitBlt( rRect.left, rRect.top, rRect.Width(), rRect.Height(), &m_oCachedBmp.GetDC(), 0, 0, SRCCOPY );
		dc.Detach();

		return true;
	}

	return false;
}

bool CAssetModelItem::HitTest( int nX, int nY )
{
	return m_oDrawingRectangle.PtInRect( CPoint( nX, nY ) ) == TRUE;
}

bool CAssetModelItem::HitTest( const CRect& roTestRect )
{
	CRect oIntersection;

	return oIntersection.IntersectRect( &m_oDrawingRectangle, &roTestRect ) == TRUE;
}

void CAssetModelItem::CacheFieldsInfoForLoadedStatObj( IStatObj *pStatObj )
{
	if( m_flags & eAssetFlags_CachedFieldsInfo )
		return;

	IStatObj::SStatistics stats;
	stats.pTextureSizer = gEnv->pSystem->CreateSizer();

	pStatObj->GetStatistics( stats );

	if( !stats.nIndices || !stats.nVertices )
		return;

	m_aabb.min = pStatObj->GetBoxMin();
	m_aabb.max = pStatObj->GetBoxMax();

	m_lodCount = stats.nLods;
	m_triangleCount = stats.nIndices / 3;
	m_vertexCount = stats.nVertices;
	m_submeshCount = stats.nSubMeshCount;
	m_nRef = stats.nNumRefs;
	m_physicsTriCount = stats.nPhysPrimitives;
	m_physicsSize = (stats.nPhysProxySize + 512) / 1024;	// In KBs
	m_bSplitLODs = stats.bSplitLods;
	m_textureSize = stats.pTextureSizer->GetTotalSize() / 1024;		// In KBs
	for( int i = 0; i < MAX_STATOBJ_LODS_NUM; ++i )
		m_triCountLOD[i] = stats.nIndicesPerLod[i] / 3;
	stats.pTextureSizer->Release();

	size_t subMtls = pStatObj->GetMaterial()->GetSubMtlCount();
	IMaterial* pMtl = NULL;
	m_dependencies.resize( 0 );

	if( !subMtls )
	{
		pMtl = pStatObj->GetMaterial();

		// fill up dependencies
		if( pMtl )
		{
			IRenderShaderResources* pShaderRes = pMtl->GetShaderItem().m_pShaderResources;

			if( pShaderRes )
			{
				for( size_t j = 0; SEfResTexture* pTex = pShaderRes->GetTexture( j ); ++j )
				{
					m_dependencies.push_back( pTex->m_Name );
				}
			}
		}
	}
	else
		for( size_t s = 0; s < subMtls; ++s )
		{
			IMaterial* pMtl = pStatObj->GetMaterial()->GetSubMtl( s );

			// fill up dependencies
			if( pMtl )
			{
				IRenderShaderResources* pShaderRes = pMtl->GetShaderItem().m_pShaderResources;

				if( pShaderRes )
				{
					for( size_t j = 0; SEfResTexture* pTex = pShaderRes->GetTexture( j ); ++j )
					{
						m_dependencies.push_back( pTex->m_Name );
					}
				}
			}
		}

	SetFlag( IAssetDisplay::eAssetFlags_CachedFieldsInfo, true );
}

void CAssetModelItem::DrawTextOnReportImage( CAlphaBitmap &abm ) const
{
	const COLORREF filenameShadowColor = RGB( 0, 0, 0 );
	const COLORREF filenameColor = RGB( 255, 255, 0 );
	const COLORREF otherInfosColor = RGB( 0, 0, 0 );
	CFont fontInfoTitle, fontInfo;
	fontInfoTitle.CreatePointFont(95, "Arial Bold");
	fontInfo.CreatePointFont(80, "Arial");

	CDC dc;
	dc.Attach(abm.GetDC());
	dc.SetBkMode(TRANSPARENT);
	dc.SelectObject( fontInfoTitle );
	dc.SetTextColor(filenameShadowColor);
	dc.TextOut(kAssetViewer_OverlayTextLeftMargin+1, 
						kAssetViewer_OverlayTextTopMargin+1, m_strFilename.c_str());
	dc.SetTextColor(filenameColor);
	dc.TextOut(kAssetViewer_OverlayTextLeftMargin, 
						kAssetViewer_OverlayTextTopMargin, m_strFilename.c_str());

	string lodsTris = MakeLODsTrisString();

	string reportInfos;
	reportInfos.Format( 
		"Path: %s\nTrianges: %d\nVertices: %d\nLODs: %d\nLODs tris: %s\nSplit LODs: %s\nPhysics tris: %d\nRefs: %d\n",
		m_strRelativePath,
		m_triangleCount,
		m_vertexCount,
		m_lodCount,
		lodsTris.c_str(),
		m_bSplitLODs ? "Yes" : "No",
		m_physicsTriCount,
		m_nRef
		);

	CSize	titleSize;
	titleSize = dc.GetTextExtent(m_strFilename.c_str());
	CRect rcTextInfo;
	rcTextInfo.left = kAssetViewer_OverlayTextLeftMargin;
	rcTextInfo.right = abm.GetWidth();
	rcTextInfo.top = titleSize.cy + 15;
	rcTextInfo.bottom = abm.GetHeight();
	dc.SetTextColor(otherInfosColor);
	dc.SelectObject(fontInfo);
	dc.DrawText(reportInfos.c_str(), &rcTextInfo, DT_WORDBREAK);
	dc.Detach();

	fontInfoTitle.DeleteObject();
	fontInfo.DeleteObject();
}

bool CAssetModelItem::SaveReportImage( const char *filePath ) const
{
	if((m_flags & eAssetFlags_ThumbnailCached) == 0)
		return false;

	CAlphaBitmap& abm = const_cast<CAlphaBitmap&>(m_oCachedBmp);
	CBitmap& bitmap = abm.GetBitmap();
	BITMAP bmpInfo;
	bitmap.GetBitmap(&bmpInfo);
	int byteCount = bmpInfo.bmWidth*bmpInfo.bmWidth*(bmpInfo.bmBitsPixel/8);

	bool bImageIsWideEnough = bmpInfo.bmWidth >= kAssetViewer_MinWidthForOverlayText;

	std::vector<unsigned char> bitsBackUp;
	if(bImageIsWideEnough)					// Back up the bitmap and draw the overlay text.
	{
		bitsBackUp.resize(byteCount);
		bitmap.GetBitmapBits(byteCount, &bitsBackUp[0]);
		DrawTextOnReportImage(abm);
	}

	CImage image;
	image.Allocate(bmpInfo.bmWidth, bmpInfo.bmWidth);
	bitmap.GetBitmapBits(byteCount, image.GetData());
	image.SwapRedAndBlue();

	if(bImageIsWideEnough)				// Restore the bitmap.
		bitmap.SetBitmapBits(byteCount, &bitsBackUp[0]);

	return CImageUtil::SaveBitmap(filePath, image);
}

bool CAssetModelItem::SaveReportText( const char *filePath ) const
{
	
	FILE *file = fopen(filePath, "wt");
	if(file == NULL)
		return false;

	string lodsTris = MakeLODsTrisString();

	fprintf(file, 
					"Filename: %s\nPath: %s\nTrianges: %d\nVertices: %d\nLODs: %d\nLODs tris: %s\nSplit LODs: %s\nPhysics tris: %d\nRefs: %d\n",
					m_strFilename,
					m_strRelativePath,
					m_triangleCount,
					m_vertexCount,
					m_lodCount,
					lodsTris.c_str(),
					m_bSplitLODs ? "Yes" : "No",
					m_physicsTriCount,
					m_nRef);

	fclose(file);

	return true;
}

string CAssetModelItem::MakeLODsTrisString() const
{
	string lodsTris;
	for(int lod = 0; lod < MAX_STATOBJ_LODS_NUM; ++lod)
	{
		if(m_triCountLOD[lod] <= 0)
			break;

		string count;
		count.Format("%d", m_triCountLOD[lod]);
		if(lod > 0)
			lodsTris += " / ";
		lodsTris += count;
	}	
	return lodsTris;
}