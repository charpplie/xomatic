/********************************************************************
CryGame Source File.
Copyright (C), StalTech Studios, 2001-2004.
-------------------------------------------------------------------------
File name:   FlightNavRegion.cpp
Version:     v1.00
Description: 

-------------------------------------------------------------------------
History:
- ?
- 4 May 2009  : Evgeny Adamenkov: Replaced IRenderer with CDebugDrawContext

*********************************************************************/

#include "StdAfx.h"
#include "CAISystem.h"
#include "AILog.h"
#include <IPhysics.h>
#include <algorithm>
#include <limits>
#include <ISystem.h>
#include "DebugDrawContext.h"
//#include <CryFile.h>
#include <ITimer.h>
#include <I3DEngine.h>
#include <Cry_GeoOverlap.h>
#include "FlightNavRegion.h"
#include "AICollision.h"
#include "WorldOctree.h"
#include "ISerialize.h"

#define BAI_FNAV_FILE_VERSION_READ 8
#define BAI_FNAV_FILE_VERSION_WRITE 9

#define	MAX_CLIP_POINT	12

inline int
clamp( int val, int minVal, int maxVal )
{
  if( val < minVal )
    return minVal;
  if( val > maxVal )
    return maxVal;
  return val;
}

inline
int fsgn( float a )
{
  if( a < 0 )
    return -1;
  return 1;
}

// The particle structure used for the path beautification.
struct SPart
{
  SPart( const Vec3& newPos ) : pos( newPos ), vel( 0, 0, 0 ), force( 0, 0, 0 ) { };
  Vec3	pos;
  Vec3	vel;
  Vec3	force;
};

struct SObstacle
{
  IGeometry*	geom;
  Vec3				pos;
  Matrix33		mat;
  AABB				bbox;
  int					minx, miny, maxx, maxy;
  bool				isSmall;
  std::vector<Vec3>	vertices;
};

CFlightNavRegion::CFlightNavRegion(IPhysicalWorld *pPhysWorld, CGraph *pGraph) :
m_pPhysWorld( pPhysWorld ),
m_pGraph(pGraph),
m_childSubDiv( 0 ),
m_terrainDownSample( 0 ),
m_heightFieldDimX( 0 ),
m_heightFieldDimY( 0 )
{
  AIAssert(m_pPhysWorld);
  AIAssert(m_pGraph);
  Clear();
}

CFlightNavRegion::~CFlightNavRegion()
{
  Clear();
}

void CFlightNavRegion::Clear()
{
  // Delete the nodes.
  for( unsigned i = 0; i < m_spans.size(); i++ )
  {
    if( m_spans[i].m_graphNodeIndex )
      m_pGraph->Disconnect(m_spans[i].m_graphNodeIndex, true);
  }

  m_spans.clear();

  m_heightFieldOriginX = 0;
  m_heightFieldOriginY = 0;
  m_heightFieldDimX = 0;
  m_heightFieldDimY = 0;
  m_childSubDiv = 2;
  m_terrainDownSample = 16;
}

bool CFlightNavRegion::ReadFromFile(const char *pName)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Navigation, 0, "Flight navigation regions");

  float fStartTime = gEnv->pTimer->GetAsyncCurTime();

  CCryFile file;
  if( !file.Open( pName, "rb" ) )
  {
    AIWarning("could not read AI flight nav. [%s]", pName);
    return false;
  }
  int nFileVersion = BAI_FNAV_FILE_VERSION_READ;
  file.ReadType( &nFileVersion );

  if (nFileVersion < BAI_FNAV_FILE_VERSION_READ)
  {
    AIWarning("Wrong BAI file version (found %d expected at least %d)!! Regenerate Flight navigation in the editor.",
      nFileVersion, BAI_FNAV_FILE_VERSION_READ);
    return false;
  }

  file.ReadType( &m_heightFieldOriginX );
  file.ReadType( &m_heightFieldOriginY );
  file.ReadType( &m_heightFieldDimX );
  file.ReadType( &m_heightFieldDimY );
  file.ReadType( &m_childSubDiv );
  file.ReadType( &m_terrainDownSample );

  uint32	spanCount = 0;
  file.ReadType( &spanCount );
  m_spans.clear(); // free memory - resize won't
  m_spans.resize( spanCount );

  if (nFileVersion == 8)
  {
    for( uint32 i = 0; i < spanCount; i++ )
    {
      SSpan&	span = m_spans[i];

      float	x, y, minz, maxz, maxradius;
      int	classification, childIdx, nextIdx;

      file.ReadType( &x );
      file.ReadType( &y );
      file.ReadType( &minz );
      file.ReadType( &maxz );
      file.ReadType( &maxradius );
      file.ReadType( &classification );
      file.ReadType( &childIdx );
      file.ReadType( &nextIdx );

      span.m_x = x;
      span.m_y  = y;
      span.m_minz = minz;
      span.m_maxz = maxz;
      span.m_maxRadius = maxradius;
      span.m_classification = classification;
      span.m_childIdx = childIdx;
      span.m_nextIdx = nextIdx;
    }
  }
  else
  {
    std::vector<SFlightDesc> flightDescs(spanCount);
    if (spanCount > 0)
      file.ReadType(&flightDescs[0], spanCount);
    for( uint32 i = 0; i < spanCount; ++i )
    {
      SSpan&	span = m_spans[i];
      SFlightDesc &fd = flightDescs[i];
      span.m_x = fd.x;
      span.m_y = fd.y;
      span.m_minz = fd.minz;
      span.m_maxz = fd.maxz;
      span.m_maxRadius = fd.maxRadius;
      span.m_classification = fd.classification;
      span.m_childIdx = fd.childIdx;
      span.m_nextIdx = fd.nextIdx;
    }
  }

  float fLinkStartTime = gEnv->pTimer->GetAsyncCurTime();

  // Create nodes.
	std::size_t nSpans = m_spans.size();

	//if (!gEnv->IsEditor() && nSpans)
	//{
	//	if (!m_pGraph->NodesPool.AddPool(IAISystem::NAV_FLIGHT, nSpans))
	//		AILogLoading("[CGraph::ReadNodes] Nodes pool already initialized!");
	//}

  for( uint32 i = 0; i < nSpans; i++ )
  {
    SSpan&	span = m_spans[i];
    span.m_graphNodeIndex = m_pGraph->CreateNewNode(IAISystem::NAV_FLIGHT, Vec3( span.m_x, span.m_y, span.m_maxz ));
		GraphNode* pSpanGraphNode = m_pGraph->GetNodeManager().GetNode(span.m_graphNodeIndex);
		if (pSpanGraphNode)
	    pSpanGraphNode->GetFlightNavData()->nSpanIdx = (int)i;
  }

  // read links
  uint32 linkCount = 0;
  file.ReadType( &linkCount );
  AILogLoading("Loading %d links descriptors.", linkCount);
  std::vector<SFlightLinkDesc> linkDescs(linkCount);
  if (linkCount> 0)
    file.ReadType(&linkDescs[0], linkCount);
  AILogLoading("Connecting %d links.", linkCount);
  for( uint32 i = 0; i < linkCount; i++ )
  {
    int	startIdx = linkDescs[i].index1;
    int	endIdx = linkDescs[i].index2;
    AIAssert( startIdx != INVALID_IDX && endIdx != INVALID_IDX );
    float	rad = min( m_spans[startIdx].m_maxRadius, m_spans[endIdx].m_maxRadius );
    m_pGraph->Connect( m_spans[startIdx].m_graphNodeIndex, m_spans[endIdx].m_graphNodeIndex, rad, rad );
  }

  float fEndTime = gEnv->pTimer->GetAsyncCurTime();

  AILogLoading("AI flight nav loaded successfully in %6.3f (spans = %6.3f, links = %6.3f) sec. [%s]", 
    fEndTime - fStartTime,
    fLinkStartTime - fStartTime,
    fEndTime - fLinkStartTime,
    pName);

  DumpLinkRadii( "flightnav_readfromfile.txt" );

  return true;
}

Vec3 CFlightNavRegion::AdjustToFlightHeight( const Vec3& start, const Vec3& end, const Vec3& pos, float radius, float optimalHeight, float minHeight, float maxHeight ) const
{
  Vec3	startToPos = pos - start;
  Vec3	endToPos = pos - end;
  startToPos.z = 0;
  endToPos.z = 0;

  float	startToPosDist = startToPos.GetLength();
  float	endToPosDist = endToPos.GetLength();
  float	minDist = min( startToPosDist, endToPosDist );

  float	a = startToPosDist / (startToPosDist + endToPosDist);
  Vec3	mid = (1.0f - a) * start + a * end;

  float	navHeight = 0, navHeightAbove = 0;
  GetHeightAndSpaceAt( pos, navHeight, navHeightAbove );

  float	navSpace = (navHeightAbove - navHeight);

  // adjust the space based on the current radius.
  float	padding = radius;
  if( padding * 2 > navSpace )
    padding = navSpace * 0.5f;

  navHeight += padding;
  navHeightAbove -= padding;
  navHeight += min( minHeight, (navHeightAbove - navHeight) );

  float	slope = 1.0f - min( 1.0f, (fabsf(start.z - end.z) + optimalHeight) / (optimalHeight * 5.0f) );

  float	distMod = min( minDist / (optimalHeight * 5.0f), 1.0f ) * slope;

  float	h = mid.z + distMod * optimalHeight;

  // Fit the desired height to the currently available space.
  if( h < navHeight )
    h = navHeight;
  if( h > navHeightAbove )
    h = navHeightAbove;

  return Vec3( pos.x, pos.y, h );
}

void CFlightNavRegion::DebugDrawField( const Vec3& start, const Vec3& end, const Vec3& camPos, float drawDist ) const
{
  // Don't do anything if the system is not initialised.
  if( m_spans.empty() )
    return;

  const float	cellSize = (float)(m_childSubDiv * m_terrainDownSample);
//  const float	childCellSize = (float)m_terrainDownSample;

  CDebugDrawContext dc;

  Matrix44	mat;
  AABB			bbox;
  mat.SetIdentity();
  const ColorB		col( 255, 255, 255, 255 );

  float	radius = 3.2f;
  float	optimalHeight = 15.0f;
  float	minHeight = 8.0f;
  float	maxHeight = 45.0f;

  int	idx = 0;
  for( int y = 0; y < m_heightFieldDimY; y++ )
  {
    for( int x = 0; x < m_heightFieldDimX; x++ )
    {
      float	worldX = (float)(x + m_heightFieldOriginX) * cellSize;
      float	worldY = (float)(y + m_heightFieldOriginY) * cellSize;

      // Skip cells too far.
      Vec3	cellPos( worldX + cellSize * 0.5f, worldY + cellSize * 0.5f, camPos.z );
      if( (cellPos - camPos).GetLength() < drawDist )
      {

        if( m_spans[idx].m_childIdx != INVALID_IDX )
        {
          int	childIdx = m_spans[idx].m_childIdx;
          for( int childY = 0; childY < m_childSubDiv; childY++ )
          {
            for( int childX = 0; childX < m_childSubDiv; childX++ )
            {
              for( int spanIdx = childIdx; spanIdx != INVALID_IDX; spanIdx = m_spans[spanIdx].m_nextIdx )
              {
                const SSpan&	span = m_spans[spanIdx];
                Vec3	pos = AdjustToFlightHeight( start, end, Vec3(span.m_x, span.m_y, span.m_maxz), radius, optimalHeight, minHeight, maxHeight );
                dc->DrawSphere( pos, 1.5f, ColorB( 255, 255, 255, 128 ) );
              }

              childIdx++;
            }
          }
        }
        else
        {
          for( int spanIdx = idx; spanIdx != INVALID_IDX; spanIdx = m_spans[spanIdx].m_nextIdx )
          {
            const SSpan&	span = m_spans[spanIdx];
            Vec3	pos = AdjustToFlightHeight( start, end, Vec3(span.m_x, span.m_y, span.m_maxz), radius, optimalHeight, minHeight, maxHeight );
            dc->DrawSphere( pos, 1.5f, ColorB( 255, 255, 255, 128 ) );
          }
        }

      }

      idx++;
    }
  }

}

void CFlightNavRegion::DebugDraw(const Vec3& camPos, float drawDist) const
{
  // Don't do anything if the system is not initialised.
  if( m_spans.empty() )
    return;

  const float	cellSize = (float)(m_childSubDiv * m_terrainDownSample);
  const float	childCellSize = (float)m_terrainDownSample;

  CDebugDrawContext dc;

  Matrix44	mat;
  AABB			bbox;
  mat.SetIdentity();
  const ColorB		col( 255, 255, 255, 255 );
  const ColorB		colObstOBB( 255, 0, 0, 255 );
  const ColorB		colLandingGoodAABB( 0, 255, 0, 255 );
  const ColorB		colLandingOkAABB(		128, 128, 0, 255 );
  const ColorB		colLandingBadAABB(	255, 0, 0, 255 );
  const ColorB		colNoGoAABB(	128, 128, 128, 255 );
  const ColorB		colChildAABB( 255, 255, 0, 255 );
  const ColorB		colLink( 255, 0, 0, 255 );

	const float labelDistSq = sqr(drawDist*0.2f);

  int	idx = 0;
  for( int y = 0; y < m_heightFieldDimY; y++ )
  {
    for( int x = 0; x < m_heightFieldDimX; x++ )
    {
      float	worldX = (float)(x + m_heightFieldOriginX) * cellSize;
      float	worldY = (float)(y + m_heightFieldOriginY) * cellSize;

      // Skip cells too far.
      Vec3	cellPos( worldX + cellSize * 0.5f, worldY + cellSize * 0.5f, camPos.z );

      if( (cellPos - camPos).GetLength() < drawDist )
      {

        if( m_spans[idx].m_childIdx != INVALID_IDX )
        {
          int	childIdx = m_spans[idx].m_childIdx;
          for( int childY = 0; childY < m_childSubDiv; childY++ )
          {
            for( int childX = 0; childX < m_childSubDiv; childX++ )
            {
              bbox.min.x = worldX + (float)childX * childCellSize;
              bbox.min.y = worldY + (float)childY * childCellSize;
              bbox.max.x = bbox.min.x + childCellSize;
              bbox.max.y = bbox.min.y + childCellSize;

              for( int spanIdx = childIdx; spanIdx != INVALID_IDX; spanIdx = m_spans[spanIdx].m_nextIdx )
              {
                const SSpan&	span = m_spans[spanIdx];
                bbox.min.z = span.m_minz;
                bbox.max.z = span.m_maxz;

                /*								ColorB	c = colLandingBadAABB;
                if( span.m_classification & SSpan::LANDING_GOOD )
                c = colLandingGoodAABB;
                else if( span.m_classification & SSpan::LANDING_OK )
                c = colLandingOkAABB;
                if( !(span.m_classification & SSpan::LINKABLE) )
                c = colNoGoAABB;*/
                ColorB	c = colNoGoAABB;
								GraphNode* pSpanGraphNode = m_pGraph->GetNodeManager().GetNode(span.m_graphNodeIndex);
                if( pSpanGraphNode && pSpanGraphNode->firstLinkIndex )
                  c = colLandingGoodAABB;

                //								pRend->DrawAABB( bbox, true, c, eBBD_Faceted );
                dc->DrawAABB( bbox, true, c, eBBD_Faceted );

#ifdef FLIGHTNAV_DEBUG_SPHERES
                dc->DrawSphere( span.m_spherePos, span.m_maxRadius, ColorB( 255, 255, 255, 128 ) );
#endif

                // Draw links
                if( pSpanGraphNode )
                {
                  for( unsigned link = pSpanGraphNode->firstLinkIndex; link; link = m_pGraph->GetLinkManager().GetNextLink(link))
                  {
										unsigned nextNodeIndex = m_pGraph->GetLinkManager().GetNextNode(link);
                    GraphNode* pNextNode = m_pGraph->GetNodeManager().GetNode(nextNodeIndex);
										if (pNextNode)
										{
											const SSpan&	toSpan = m_spans[pNextNode->GetFlightNavData()->nSpanIdx];
											dc->DrawLine( Vec3( span.m_x, span.m_y, span.m_maxz + 1 ), ColorB( 255, 255, 255 ), Vec3( toSpan.m_x, toSpan.m_y, toSpan.m_maxz + 1 ), ColorB( 255, 255, 255 ) );
										}
                  }
                }
              }

              childIdx++;
            }
          }
        }
        else
        {
          // Draw big cell.
          bbox.min.x = worldX;
          bbox.min.y = worldY;
          bbox.max.x = bbox.min.x + cellSize;
          bbox.max.y = bbox.min.y + cellSize;

          for( int spanIdx = idx; spanIdx != INVALID_IDX; spanIdx = m_spans[spanIdx].m_nextIdx )
          {
            const SSpan&	span = m_spans[spanIdx];
            bbox.min.z = span.m_minz;
            bbox.max.z = span.m_maxz;

            /*						ColorB	c = colLandingBadAABB;
            if( span.m_classification & SSpan::LANDING_GOOD )
            c = colLandingGoodAABB;
            else if( span.m_classification & SSpan::LANDING_OK )
            c = colLandingOkAABB;
            if( !(span.m_classification & SSpan::LINKABLE) )
            c = colNoGoAABB;*/
            ColorB	c = colNoGoAABB;
						GraphNode* pSpanGraphNode = m_pGraph->GetNodeManager().GetNode(span.m_graphNodeIndex);
            if( pSpanGraphNode && pSpanGraphNode->firstLinkIndex )
              c = colLandingGoodAABB;


            dc->DrawAABB( bbox, true, c, eBBD_Faceted );
            //pRend->DrawAABB( bbox, false, c, eBBD_Faceted );

            // Draw links
            if( pSpanGraphNode )
            {
              for( unsigned link = pSpanGraphNode->firstLinkIndex; link; link = m_pGraph->GetLinkManager().GetNextLink(link))
              {
								unsigned nextNodeIndex = m_pGraph->GetLinkManager().GetNextNode(link);
                GraphNode* pNextNode = m_pGraph->GetNodeManager().GetNode(nextNodeIndex);
								if (pNextNode)
								{
									const SSpan&	toSpan = m_spans[pNextNode->GetFlightNavData()->nSpanIdx];
									dc->DrawLine( Vec3( span.m_x, span.m_y, span.m_maxz + 1 ), ColorB( 255, 255, 255 ), Vec3( toSpan.m_x, toSpan.m_y, toSpan.m_maxz + 1 ), ColorB( 255, 255, 255 ) );
								}
              }
            }

#ifdef FLIGHTNAV_DEBUG_SPHERES
            pRend->DrawSphere( span.m_spherePos, span.m_maxRadius, ColorB( 255, 255, 255, 128 ) );
#endif
          }
        }

      }

      idx++;
    }
  }

  // Draw labels
  idx = 0;
  for( int y = 0; y < m_heightFieldDimY; y++ )
  {
    for( int x = 0; x < m_heightFieldDimX; x++ )
    {
      float	worldX = (float)(x + m_heightFieldOriginX) * cellSize;
      float	worldY = (float)(y + m_heightFieldOriginY) * cellSize;

      // Skip cells too far.
      Vec3	cellPos( worldX + cellSize * 0.5f, worldY + cellSize * 0.5f, camPos.z );

      if( (cellPos - camPos).GetLength() < drawDist )
      {

        if( m_spans[idx].m_childIdx != INVALID_IDX )
        {
          int	childIdx = m_spans[idx].m_childIdx;
          for( int childY = 0; childY < m_childSubDiv; childY++ )
          {
            for( int childX = 0; childX < m_childSubDiv; childX++ )
            {
              for( int spanIdx = childIdx; spanIdx != INVALID_IDX; spanIdx = m_spans[spanIdx].m_nextIdx )
              {
                const SSpan&	span = m_spans[spanIdx];
								GraphNode* pSpanGraphNode = m_pGraph->GetNodeManager().GetNode(span.m_graphNodeIndex);
								if (pSpanGraphNode)
								{
									if (Distance::Point_Point2DSq(cellPos, camPos) < labelDistSq)
		                dc->Draw3dLabel( Vec3( span.m_x, span.m_y, span.m_maxz + 1.0f ), 1.5, "(%d, %d)(%d, %d) %d\n%d", x, y, childX, childY, pSpanGraphNode->GetLinkCount(m_pGraph->GetLinkManager()), pSpanGraphNode->GetFlightNavData()->nSpanIdx );
								}
              }

              childIdx++;
            }
          }
        }
        else
        {
          // Draw big cell.
          for( int spanIdx = idx; spanIdx != INVALID_IDX; spanIdx = m_spans[spanIdx].m_nextIdx )
          {
            const SSpan&	span = m_spans[spanIdx];
						GraphNode* pSpanGraphNode = m_pGraph->GetNodeManager().GetNode(span.m_graphNodeIndex);
						if (pSpanGraphNode)
						{
							if (Distance::Point_Point2DSq(cellPos, camPos) < labelDistSq)
		            dc->Draw3dLabel( Vec3( span.m_x, span.m_y, span.m_maxz + 1.0f ), 1.5f, "(%d, %d) %d\n%d", x, y, pSpanGraphNode->GetLinkCount(m_pGraph->GetLinkManager()), pSpanGraphNode->GetFlightNavData()->nSpanIdx );
						}
          }
        }

      }

      idx++;
    }
  }


#ifdef FLIGHTNAV_DEBUG_SPHERES
  // Draw labels
  CDebugDrawContext dc;
  idx = 0;
  for( int y = 0; y < m_heightFieldDimY; y++ )
  {
    for( int x = 0; x < m_heightFieldDimX; x++ )
    {
      float	worldX = (float)(x + m_heightFieldOriginX) * cellSize;
      float	worldY = (float)(y + m_heightFieldOriginY) * cellSize;

      // Skip cells too far.
      Vec3	cellPos( worldX + cellSize * 0.5f, worldY + cellSize * 0.5f, camPos.z );

      if( (cellPos - camPos).GetLength() < drawDist )
      {

        if( m_spans[idx].m_childIdx != INVALID_IDX )
        {
          int	childIdx = m_spans[idx].m_childIdx;
          for( int childY = 0; childY < m_childSubDiv; childY++ )
          {
            for( int childX = 0; childX < m_childSubDiv; childX++ )
            {
              bbox.min.x = worldX + (float)childX * childCellSize;
              bbox.min.y = worldY + (float)childY * childCellSize;
              bbox.max.x = bbox.min.x + childCellSize;
              bbox.max.y = bbox.min.y + childCellSize;

              for( int spanIdx = childIdx; spanIdx != INVALID_IDX; spanIdx = m_spans[spanIdx].m_nextIdx )
              {
                const SSpan&	span = m_spans[spanIdx];
                dc->Draw3dLabel( span.m_spherePos, 1, "%.2f", span.m_maxRadius );
              }

              childIdx++;
            }
          }
        }
        else
        {
          // Draw big cell.
          bbox.min.x = worldX;
          bbox.min.y = worldY;
          bbox.max.x = bbox.min.x + cellSize;
          bbox.max.y = bbox.min.y + cellSize;

          for( int spanIdx = idx; spanIdx != INVALID_IDX; spanIdx = m_spans[spanIdx].m_nextIdx )
          {
            const SSpan&	span = m_spans[spanIdx];
            dc->Draw3dLabel( span.m_spherePos, 1, "%.2f", span.m_maxRadius );
          }
        }

      }

      idx++;
    }
  }
#endif
}

bool CFlightNavRegion::CheckAndAdjustSphere( Vec3& center, float rad )
{
  // Don't do anything if the system is not initialised.
  if( m_spans.empty() )
    return false;

  const float	cellSize = (float)(m_childSubDiv * m_terrainDownSample);
  const float	childCellSize = (float)m_terrainDownSample;

  int	minx = clamp( (int)floor( (center.x - rad) / cellSize ) - m_heightFieldOriginX, 0, m_heightFieldDimX - 1 );
  int	miny = clamp( (int)floor( (center.y - rad) / cellSize ) - m_heightFieldOriginY, 0, m_heightFieldDimY - 1 );
  int	maxx = clamp( (int)ceil( (center.x + rad) / cellSize ) - m_heightFieldOriginX, 0, m_heightFieldDimX - 1 );
  int	maxy = clamp( (int)ceil( (center.y + rad) / cellSize ) - m_heightFieldOriginY, 0, m_heightFieldDimY - 1 );

  bool	hit = false;

  for( int y = miny; y < maxy; y++ )
  {
    for( int x = minx; x < maxx; x++ )
    {
      int	idx = x + y * m_heightFieldDimX;

      float	worldX = (float)(x + m_heightFieldOriginX) * cellSize;
      float	worldY = (float)(y + m_heightFieldOriginY) * cellSize;

      if( m_spans[idx].m_childIdx != INVALID_IDX )
      {
        // test all childs.
        int	childIdx = m_spans[idx].m_childIdx;
        for( int childY = 0; childY < m_childSubDiv; childY++ )
        {
          for( int childX = 0; childX < m_childSubDiv; childX++ )
          {
            Vec3	boxMin( worldX + (float)childX * childCellSize, worldY + (float)childY * childCellSize, 0 );
            Vec3	boxMax( boxMin.x + childCellSize, boxMin.y + childCellSize, 0 );

            for( int spanIdx = childIdx; spanIdx != INVALID_IDX; spanIdx = m_spans[spanIdx].m_nextIdx )
            {
              boxMin.z = m_spans[spanIdx].m_minz;
              boxMax.z = m_spans[spanIdx].m_maxz;
              if( BoxSphereAdjust( boxMin, boxMax, center, rad ) )
                hit = true;
            }

            childIdx++;
          }
        }
      }
      else
      {
        Vec3	boxMin( worldX, worldY, 0 );
        Vec3	boxMax( boxMin.x + cellSize, boxMin.y + cellSize, 0 );

        for( int spanIdx = idx; spanIdx != INVALID_IDX; spanIdx = m_spans[spanIdx].m_nextIdx )
        {
          boxMin.z = m_spans[spanIdx].m_minz;
          boxMax.z = m_spans[spanIdx].m_maxz;
          if( BoxSphereAdjust( boxMin, boxMax, center, rad ) )
            hit = true;
        }
      }
    }
  }

  return hit;
}

bool CFlightNavRegion::BoxSphereAdjust( const Vec3& boxMin, const Vec3& boxMax, Vec3& center, float rad )
{
  Vec3	dmin( 0, 0, 0 );
  float	r2 = rad * rad;

  if( center.x < boxMin.x )
    dmin.x += center.x - boxMin.x;
  else if( center.x > boxMax.x )
    dmin.x += center.x - boxMax.x;    

  if( center.y < boxMin.y )
    dmin.y += center.y - boxMin.y;
  else if( center.y > boxMax.y )
    dmin.y += center.y - boxMax.y;     

  if( center.z < boxMin.z )
    dmin.z += center.z - boxMin.z;
  else if( center.z > boxMax.z )
    dmin.z += center.z - boxMax.z;     

  float	lenSqr = dmin.GetLengthSquared();
  if( lenSqr <= r2 )
  {
    float len = (float)sqrtf( lenSqr );
    if( len > 0 )
    {
      len = (1.0f / len) * (-len + rad);
      center += dmin * len;
    }
    else
    {
      // If we get here, the center of the sphere is inside the
      // box, this is tricky case, and is handled in special way which
      // will behefit the way the boxes are used.
      // The sphere will be lifted up, and pushed down. Lifting is biased.
      float	deltaz = 0;
      // Adjust height only.
      if( (center.z - boxMin.z) < (boxMax.z - center.z) * 0.8f )
        deltaz = boxMin.z - rad - center.z;
      else
        deltaz = boxMax.z + rad - center.z;

      center.z += deltaz;

    }
    return true;
  }

  return false;
}

unsigned CFlightNavRegion::GetEnclosing(const Vec3 &pos, float passRadius, unsigned startIndex, float /*range*/, 
                                          Vec3 * closestValid, bool returnSuspect, const char *requesterName)
{
  FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

  // Don't do anything if the system is not initialised.
  if( m_spans.empty() )
  {
    AIWarning("CFlightNavRegion::GetEnclosing No flight navigation data available [Design bug]");
    return 0;
  }

//  range = 20.0f;
//  float	halfRange = range / 2.0f;

  const float	cellSize = (float)(m_childSubDiv * m_terrainDownSample);
//  const float	childCellSize = (float)m_terrainDownSample;

  /*
  int	sx = clamp( (int)floor( (pos.x - halfRange) / cellSize ) - m_heightFieldOriginX, 0, m_heightFieldDimX - 1 );
  int	sy = clamp( (int)floor( (pos.y - halfRange) / cellSize ) - m_heightFieldOriginY, 0, m_heightFieldDimY - 1 );
  int	ex = clamp( (int)ceil( (pos.x + halfRange) / cellSize ) - m_heightFieldOriginX, 0, m_heightFieldDimX - 1 );
  int	ey = clamp( (int)ceil( (pos.y + halfRange) / cellSize ) - m_heightFieldOriginY, 0, m_heightFieldDimY - 1 );
  */

  /*
  int	sx = (int)floor( pos.x / cellSize ) - m_heightFieldOriginX;
  int	sy = (int)floor( pos.y / cellSize ) - m_heightFieldOriginY;

  Vec3				bestValid = pos;
  GraphNode*	bestNode = 0;
  float				bestDist = FLT_MAX;

  for( int y = sy - 1; y <= sy + 1; y++ )
  {
  for( int x = sx - 1; x <= sx + 1; x++ )
  {
  int	xx = clamp( x, 0, m_heightFieldDimX - 1 );
  int	yy = clamp( y, 0, m_heightFieldDimY - 1 );
  Vec3				valid;
  GraphNode*	node = GetEnclosing( xx, yy, pos, &valid );
  if( node && !node->links.empty() )
  {
  float	dist = (node->GetPos() - pos).GetLengthSquared();
  if( dist < bestDist )
  {
  bestNode = node;
  bestValid = valid;
  bestDist = dist;
  }
  }
  }
  }

  if( bestNode && closestValid )
  *closestValid = bestValid;

  return bestNode;*/

  int	x = clamp( (int)floor( pos.x / cellSize ) - m_heightFieldOriginX, 0, m_heightFieldDimX - 1 );
  int	y = clamp( (int)floor( pos.y / cellSize ) - m_heightFieldOriginY, 0, m_heightFieldDimY - 1 );

  Vec3 valid = pos;
	unsigned nodeIndex = GetEnclosing( x, y, pos, &valid );
  GraphNode*	node = m_pGraph->GetNodeManager().GetNode(nodeIndex);

  if( node && closestValid )
    *closestValid = valid;

  return nodeIndex;
}

unsigned CFlightNavRegion::GetEnclosing( int x, int y, const Vec3 &pos, Vec3 * closestValid, Vec3 * spanPos )
{
  const float	cellSize = (float)(m_childSubDiv * m_terrainDownSample);
  const float	childCellSize = (float)m_terrainDownSample;

  int startIdx = INVALID_IDX;
  int	idx = x + y * m_heightFieldDimX;

  if( m_spans[idx].m_childIdx != INVALID_IDX )
  {
    int	childX = clamp( (int)floor( (pos.x - (float)(x + m_heightFieldOriginX) * cellSize) / childCellSize ), 0, m_childSubDiv - 1 );
    int	childY = clamp( (int)floor( (pos.y - (float)(y + m_heightFieldOriginY) * cellSize) / childCellSize ), 0, m_childSubDiv - 1 );

    startIdx = m_spans[idx].m_childIdx + childX + childY * m_childSubDiv;
  }
  else
  {
    startIdx = idx;
  }

  for( int srcIdx = startIdx; srcIdx != INVALID_IDX; srcIdx = m_spans[srcIdx].m_nextIdx )
  {
    float	srcMin, srcMax;
    int srcNextIdx = m_spans[srcIdx].m_nextIdx;

    // Test against the current span, plus the space above it.
    srcMin = m_spans[srcIdx].m_minz;
    if( srcNextIdx != INVALID_IDX )
      srcMax = m_spans[srcNextIdx].m_minz;
    else
      srcMax = srcMin + 100000;

    if( pos.z >= srcMin && pos.z < srcMax )
    {
      if( closestValid )
      {
        if( pos.z < m_spans[srcIdx].m_maxz )
        {
          *closestValid = pos;
          (*closestValid).z = m_spans[srcIdx].m_maxz;
        }
      }

      if( spanPos )
        (*spanPos).Set( m_spans[srcIdx].m_x, m_spans[srcIdx].m_y, m_spans[srcIdx].m_maxz );

      return m_spans[srcIdx].m_graphNodeIndex;
    }
  }

  AIAssert( startIdx != INVALID_IDX );
  return m_spans[startIdx].m_graphNodeIndex;
}

Vec3 CFlightNavRegion::IsSpaceVoid( const Vec3& vPos, const Vec3& vForward, const Vec3& vWing, const Vec3& vUp ) const
{
	// check if the space is void inside of the box which is made by these 8 point
	// (vPos)      (vPos+vForward)     (vPos+vWing)     (vPos+vForward+vWing) 
	// (vPos+vUp)  (vPos+vForward+vUp) (vPos+vWing+vUp) (vPos+vForward+vWing+vUp) 

	// returns the first point which is not void, as (x,y,max hight(or max hightAbove))
	// otherwize returns 0 vector;

	if (m_spans.empty())
	{
	AIWarning("CFlightNavRegion::IsSpaceVoid No flight navigation data available [Design bug]");
	return ZERO;
	}
/*
	
	Vec3 vCapCenter = vPos + vForward * 0.75f;
	Vec3 vCapFwd = vForward * 0.5;

	primitives::capsule capsulePrim;
	capsulePrim.center = 0.5f * ( vPos + vForward );
	capsulePrim.axis = vForward;
	capsulePrim.hh = 0.5f * capsulePrim.axis.NormalizeSafe(Vec3Constants<float>::fVec3_OneZ);
	capsulePrim.r = vWing.GetLength() * 0.5f;
	IPhysicalWorld* pPhysics = gEnv->pPhysicalWorld;
	float d = pPhysics->PrimitiveWorldIntersection(capsulePrim.type, &capsulePrim, Vec3(ZERO), 
	ent_static | ent_terrain | ent_ignore_noncolliding, 0, 0, geom_colltype0);
*/
 

	const float	alomostzero		= 0.0001f;	//(normaliy 5 disits has almost no error for float)
	const float	cellSize		= (float)(m_childSubDiv * m_terrainDownSample);
	const float	childCellSize	= (float)m_terrainDownSample;

	float ls = vForward.GetLength();
	float lt = vWing.GetLength();
	float lu = vUp.GetLength();

	float ds = ( ls < alomostzero ) ? 1.0f : childCellSize/ls;
	float dt = ( lt < alomostzero ) ? 1.0f : childCellSize/lt;
	float du = ( lu < alomostzero ) ? 1.0f : childCellSize/lu;

	Vec3	vCheckPoint;
	Vec3	vReturnVec(ZERO);

	float	terrainHeight = gEnv->p3DEngine->GetTerrainZ( (int)vPos.x, (int)vPos.y );
	float	waterHeight = gEnv->p3DEngine->GetWaterLevel(&vPos);
	float	height =  max( terrainHeight, waterHeight ) +20.0f;
	float	heightAbove = height +1000.0f;

	int		lastIndex = -1;
	bool	bCantGo = false;

	Vec3	ddd[8];
	{
		ddd[0] = vPos ;
		ddd[1] = vPos + vForward ;
		ddd[2] = vPos + vForward + vWing ;
		ddd[3] = vPos + vWing;
		ddd[4] = vPos + vUp;
		ddd[5] = vPos + vForward + vUp;
		ddd[6] = vPos + vForward + vWing + vUp;
		ddd[7] = vPos + vWing + vUp;
	}


	for ( float s = 0.0f; s < 1.0f  ; s += ds )
	{
		for ( float t = 0.0f; t < 1.0f  ; t += dt )
		{
			for ( float u = 0.0f; u < 1.0f  ; u += du )
			{
				// for using GetHeightAndSpaceAt, I avoided making a kind of the same function.
				// but when the cell is the same, it is better skip calling.

				vCheckPoint = vPos + vForward * s + vWing * t + vUp * u;

				int	x = clamp( (int)floor( vCheckPoint.x / cellSize ) - m_heightFieldOriginX, 0, m_heightFieldDimX - 1 );
				int	y = clamp( (int)floor( vCheckPoint.y / cellSize ) - m_heightFieldOriginY, 0, m_heightFieldDimY - 1 );

				int startIdx= INVALID_IDX;
				int	idx		= x + y * m_heightFieldDimX;

				if( m_spans[idx].m_childIdx != INVALID_IDX )
				{
					int	childX	= clamp( (int)floor( (vCheckPoint.x - (float)(x + m_heightFieldOriginX) * cellSize) / childCellSize ), 0, m_childSubDiv - 1 );
					int	childY	= clamp( (int)floor( (vCheckPoint.y - (float)(y + m_heightFieldOriginY) * cellSize) / childCellSize ), 0, m_childSubDiv - 1 );
					startIdx	= m_spans[idx].m_childIdx + childX + childY * m_childSubDiv;
				}
				else
				{
					startIdx = idx;
				}

				if ( startIdx != lastIndex )
				{
					lastIndex = startIdx;
					GetHeightAndSpaceAt( vCheckPoint, height, heightAbove );
					if ( waterHeight > height )
						height = waterHeight+1.0f;
				}

				if ( vCheckPoint.z > height && vCheckPoint.z < heightAbove )
				{
					// this cell is void :)
				}
				else
				{
					// when this point is not void :(
					if ( vCheckPoint.z > heightAbove )
					{
						// treat as closed space
						vReturnVec.x = vCheckPoint.x;
						vReturnVec.y = vCheckPoint.y;
						vReturnVec.z = 10000.0f;
						return vReturnVec;
					}

					if ( vCheckPoint.z <= height )
					{
						if ( bCantGo == false )
						{
							bCantGo = true;
							vReturnVec.x = vCheckPoint.x;
							vReturnVec.y = vCheckPoint.y;
							vReturnVec.z = height;
						}
						else if ( height > vReturnVec.z )
						{
							vReturnVec.x = vCheckPoint.x;
							vReturnVec.y = vCheckPoint.y;
							vReturnVec.z = height;
						}
					}
				}
			}
		}
	}

	unsigned char	r,g,b;
	if ( vReturnVec.z > 0 )
		r =255, g =0, b=0;
	else if ( vPos.z + vUp.z/2.0f +5.0f < vReturnVec.z  )
		r =0, g =255, b=255;
	else
		r =255, g =255, b=255;

	gEnv->pAISystem->AddDebugLine( ddd[0], ddd[1], r, g, b, 1.0f);
	gEnv->pAISystem->AddDebugLine( ddd[1], ddd[2], r, g, b, 1.0f);
	gEnv->pAISystem->AddDebugLine( ddd[2], ddd[3], r, g, b, 1.0f);
	gEnv->pAISystem->AddDebugLine( ddd[3], ddd[0], r, g, b, 1.0f);

	gEnv->pAISystem->AddDebugLine( ddd[4], ddd[5], r, g, b, 1.0f);
	gEnv->pAISystem->AddDebugLine( ddd[5], ddd[6], r, g, b, 1.0f);
	gEnv->pAISystem->AddDebugLine( ddd[6], ddd[7], r, g, b, 1.0f);
	gEnv->pAISystem->AddDebugLine( ddd[7], ddd[4], r, g, b, 1.0f);

	gEnv->pAISystem->AddDebugLine( ddd[0], ddd[4], r, g, b, 1.0f);
	gEnv->pAISystem->AddDebugLine( ddd[1], ddd[5], r, g, b, 1.0f);
	gEnv->pAISystem->AddDebugLine( ddd[2], ddd[6], r, g, b, 1.0f);
	gEnv->pAISystem->AddDebugLine( ddd[3], ddd[7], r, g, b, 1.0f);

	return vReturnVec;

}

void CFlightNavRegion::GetHeightAndSpaceAt( const Vec3& pos, float& height, float& heightAbove ) const
{
  const float	cellSize = (float)(m_childSubDiv * m_terrainDownSample);
  const float	childCellSize = (float)m_terrainDownSample;

  int	x = clamp( (int)floor( pos.x / cellSize ) - m_heightFieldOriginX, 0, m_heightFieldDimX - 1 );
  int	y = clamp( (int)floor( pos.y / cellSize ) - m_heightFieldOriginY, 0, m_heightFieldDimY - 1 );

  int startIdx = INVALID_IDX;
  int	idx = x + y * m_heightFieldDimX;

  if( m_spans[idx].m_childIdx != INVALID_IDX )
  {
    int	childX = clamp( (int)floor( (pos.x - (float)(x + m_heightFieldOriginX) * cellSize) / childCellSize ), 0, m_childSubDiv - 1 );
    int	childY = clamp( (int)floor( (pos.y - (float)(y + m_heightFieldOriginY) * cellSize) / childCellSize ), 0, m_childSubDiv - 1 );

    startIdx = m_spans[idx].m_childIdx + childX + childY * m_childSubDiv;
  }
  else
  {
    startIdx = idx;
  }

  for( int srcIdx = startIdx; srcIdx != INVALID_IDX; srcIdx = m_spans[srcIdx].m_nextIdx )
  {
    float	srcMin, srcMax;
    int srcNextIdx = m_spans[srcIdx].m_nextIdx;

    // Test against the current span, plus the space above it.
    srcMin = m_spans[srcIdx].m_minz;
    if( srcNextIdx != INVALID_IDX )
      srcMax = m_spans[srcNextIdx].m_minz;
    else
      srcMax = srcMin + 100000;

    // Test against the current span, plus the space above it.
    srcMin = (m_spans[srcIdx].m_minz + m_spans[srcIdx].m_maxz) * 0.5f;
    float	bot = 0, top = 0;
    if( srcNextIdx != INVALID_IDX )
    {
      srcMax = (m_spans[srcNextIdx].m_minz + m_spans[srcNextIdx].m_maxz) * 0.5f;
      bot = m_spans[srcIdx].m_maxz;
      top = m_spans[srcNextIdx].m_minz;
    }
    else
    {
      srcMax = srcMin + 100000;
      bot = m_spans[srcIdx].m_maxz;
      top = bot + 100000;
    }

    if( pos.z >= srcMin && pos.z < srcMax )
    {
      height = bot;
      heightAbove = top;
      return;
    }
  }

  if (startIdx != INVALID_IDX)
  {
    // we're not in a valid span, so return the terrain (lowest span) height
    height = m_spans[startIdx].m_maxz;;
    heightAbove = 100000;
    return;
  }

  // We should never get here. 
  AIError("CFlightNavRegion::GetHeightAndSpaceAt failed for pos (%5.2f, %5.2f, %5.2f)",
    pos.x, pos.y, pos.z);
  height = pos.z;
  heightAbove = 1000.0f;
}

//====================================================================
// UglifyPath
//====================================================================
void CFlightNavRegion::UglifyPath(const VectorConstNodeIndices& inPath, TPathPoints& outPath, 
                                  const Vec3& startPos, const Vec3& startDir, 
                                  const Vec3& endPos, const Vec3 & endDir)
{
  outPath.push_back(PathPointDescriptor(IAISystem::NAV_FLIGHT, startPos));
  for(VectorConstNodeIndices::const_iterator itrCurNode=inPath.begin() ; itrCurNode != inPath.end() ; ++itrCurNode)
  {
		const GraphNode* curNode = m_pGraph->GetNodeManager().GetNode(*itrCurNode);
		if (curNode)
	    outPath.push_back(PathPointDescriptor(IAISystem::NAV_FLIGHT, curNode->GetPos()));
  }
  outPath.push_back(PathPointDescriptor(IAISystem::NAV_FLIGHT, endPos));
}


//====================================================================
// BeautifyPath
//====================================================================
void CFlightNavRegion::BeautifyPath(const VectorConstNodeIndices& inPath, TPathPoints& outPath, 
                                    const Vec3& startPos, const Vec3& startDir, 
                                    const Vec3& endPos, const Vec3 & endDir,
                                    float radius,
                                    const AgentMovementAbility & movementAbility,
                                    const NavigationBlockers& navigationBlockers)
{
  FUNCTION_PROFILER( GetISystem(), PROFILE_AI );
  // Don't do anything if the system is not initialised.
  if( m_spans.empty() )
    return;

  float optimalHeight = movementAbility.optimalFlightHeight;
  float minHeight = movementAbility.minFlightHeight;
  float maxHeight = movementAbility.maxFlightHeight;

//  const float	cellSize = (float)(m_childSubDiv * m_terrainDownSample);
  const float	childCellSize = (float)m_terrainDownSample;
  const float	minRadius = childCellSize * 0.4f;

  if( radius < minRadius )
    radius = minRadius;

  std::list<SPart>	points;

  //
  // Create initial particles.
  //

  Vec3	lastPos = startPos;
  float	pathLen = 0;

  for(VectorConstNodeIndices::const_iterator nodeIt = inPath.begin(); nodeIt != inPath.end() ; ++nodeIt)
  {
    const GraphNode* pCurNode = m_pGraph->GetNodeManager().GetNode(*nodeIt);
		if (!pCurNode) continue;
    Vec3	curPos = pCurNode->GetPos();
    Vec3	diff = curPos - lastPos;
    float	len = diff.GetLength();
    pathLen += len;
    if( len > radius )
    {
      int nNewPt = (int)ceil( len / (radius * 2.0f) );
      for( int i = 1; i < nNewPt; i++ )
      {
        float	a = (float)i / (float)nNewPt;
        Vec3	pt = lastPos + a * diff;
        points.push_back( SPart( pt ) );
      }
    }
    points.push_back( SPart( curPos ) );
    lastPos = curPos;
  }

  for( std::list<SPart>::iterator poIt = points.begin(); poIt != points.end(); ++poIt )
  {
    Vec3&	pos = (*poIt).pos;
    (*poIt).pos = AdjustToFlightHeight( startPos, endPos, pos, radius, optimalHeight, minHeight, maxHeight );
  }

  points.push_front( SPart( startPos ) );
  points.push_back( SPart( endPos ) );

  // Update particle system iteratively. Pseudocode:
  //
  //	For each particle
  //	{
  //		Prev = previous particle, Cur = current particle, Next = next particle
  //		Add forces to Cur, Next to minimize the distance between them
  //		Add forces to Prev, Next to minimize the curvature the at Cur
  //		Integrate Cur
  //		Damp the Cur velocity by factor of 0.4
  //	}
  //	For each particle
  //	{
  //		Cur = current particle
  //		Collide Cur against the spans and cells
  //	}

  static int nIter = 10;
  for( int iter = 0; iter < nIter; iter++ )
  {
    for( std::list<SPart>::iterator poIt = points.begin(); poIt != points.end(); ++poIt )
      (*poIt).force.Set( 0, 0, 0 );

    std::list<SPart>::iterator poPrevIt = points.end(), poCurIt = points.end(), poNextIt = points.end();

    poCurIt = points.begin();
    if( poCurIt != points.end() )
    {
      poNextIt = poCurIt;
      ++poNextIt;
    }

    int	i = 0;
    int	nPoints = points.size();
    int	curveLen = nPoints / 3;

    if( curveLen < 1 )
      curveLen = 1;
    else if( curveLen > 20 )
      curveLen = 20;

    while( poNextIt != points.end() )
    {
      SPart*	prev = 0;
      SPart*	cur = &(*poCurIt);
      SPart*	next = &(*poNextIt);

      if( poPrevIt != points.end() )
        prev = &(*poPrevIt);

      // minimize distance
      Vec3	deltaCN = next->pos - cur->pos;
      float	dist = deltaCN.GetLength();

      // desired length.
      float	desired = 0; //radius * 0.1f;

      if( poCurIt != points.begin() && dist < radius * 0.5f )
      {
        points.erase( poCurIt++ );
        poNextIt = poCurIt;
        ++poNextIt;
        nPoints--;
        continue;
      }

      if( dist > 0 )
      {
        float	a = ((dist - desired) / dist) * 0.3f;

        cur->force +=  deltaCN * a;
        next->force += -deltaCN * a;
      }

      // Handle start and end direction.
      if( nPoints > 3 )
      {
        if( i < curveLen )
        {
          float	a = 1.0f - (float)i / (float)curveLen;
          a = 1.0f - (1.0f - a) * (1.0f - a);
          cur->force += startDir * radius * a * 0.4f;
        }
        else if( i > nPoints - curveLen )
        {
          float a = (float)(i - (nPoints - curveLen)) / (float)curveLen;
          a = 1.0f - (1.0f - a) * (1.0f - a);
          cur->force += -endDir * radius * a * 0.4f;
        }
      }

      // minimize angle
      if( prev && cur && next )
      {
        Vec3	deltaPC = cur->pos - prev->pos;

        deltaCN.NormalizeSafe();
        deltaPC.NormalizeSafe();

        Vec3	tan = deltaPC + deltaCN;
        tan.NormalizeSafe();

        Vec3	pt;
        Vec3	diff;
        Vec3	force;
        float	u;

        // Project previous point to the tangent.
        diff = prev->pos - cur->pos;
        u = diff.Dot( tan );
        pt = cur->pos + tan * u;
        force = (pt - prev->pos) * 0.2f;
        prev->force += force;

        // Project previous point to the tangent.
        diff = next->pos - cur->pos;
        u = diff.Dot( tan );
        pt = cur->pos + tan * u;
        force = (pt - next->pos) * 0.2f;
        next->force += force;
      }

      poPrevIt = poCurIt;
      poCurIt = poNextIt;
      ++poNextIt;
      i++;
    }

    // Dont move start and end points.
    points.front().force.Set( 0, 0, 0 );
    points.back().force.Set( 0, 0, 0 );

    for( std::list<SPart>::iterator poIt = points.begin(); poIt != points.end(); ++poIt )
    {
      SPart&	p = (*poIt);
      p.vel += p.force;
      p.pos += p.vel;
      p.vel *= 0.3f;
    }

    // TODO: Tesselate the links if they get too long (>2.5f*rad).
    i = 0;
    for( std::list<SPart>::iterator poIt = points.begin(); poIt != points.end(); ++poIt )
    {
      if( i >= 2 && i < nPoints - 2 )
      {
        Vec3	oldPos = (*poIt).pos;
        CheckAndAdjustSphere( (*poIt).pos, radius * 1.05f );
      }
      i++;
    }
  }

  // Store the path.
  for( std::list<SPart>::iterator poIt = points.begin(); poIt != points.end(); ++poIt )
    outPath.push_back(PathPointDescriptor(IAISystem::NAV_FLIGHT, poIt->pos ));

  return;
}

//====================================================================
// Serialize
//====================================================================
void CFlightNavRegion::Serialize(TSerialize ser, CObjectTracker& objectTracker)
{
  ser.BeginGroup("FlightNavRegion");

  ser.EndGroup();
}

//===================================================================
// DumpLinkRadii
//===================================================================
void CFlightNavRegion::DumpLinkRadii( const char* filename )
{
  /*
  FILE*	pStream = fopen( filename, "w" );
  if( pStream )
  fprintf( pStream, "Flight navigation span links\n\n" );

  float	minRad = FLT_MAX;
  float	maxRad = FLT_MIN;

  for( unsigned i = 0; i < m_spans.size(); i++ )
  {
  GraphNode*	node = m_spans[i].m_pGraphNode;
  AIAssert( node );

  if( pStream )
  fprintf( pStream, "%d r:%.2f\n", i, m_spans[i].m_maxRadius );

  int j = 0;
  for( VectorOfLinks::iterator linkIt = node->links.begin(); linkIt != node->links.end(); ++linkIt )
  {
  GraphLink&	link = (*linkIt);
  GraphNode*	nextNode = link.pNextNode;
  int	nextIdx = nextNode->GetFlightNavData()->nSpanIdx;
  if( nextIdx != INVALID_IDX )
  {
  float	rad = link.GetRadius();
  if( pStream )
  fprintf( pStream, " -> %d r:%.2f = min(%.2f, %.2f) \n", j, rad, m_spans[i].m_maxRadius, m_spans[nextIdx].m_maxRadius );
  minRad = min( rad, minRad );
  maxRad = max( rad, maxRad );
  }
  j++;
  }
  }

  if( pStream )
  fprintf( pStream, "\n** minRad=%f  maxRad=%f\n\n", minRad, maxRad );

  fclose( pStream );
  */
}

//===================================================================
// CheckFlightNavLine
//===================================================================
bool CFlightNavRegion::CheckFlightNavLine(const Vec3 &start,const Vec3 &end) const
{
  const SpecialArea *pArea = gAIEnv.pNavigation->GetSpecialArea(start, SpecialArea::TYPE_FLIGHT);

  if (!pArea)
    return false;

  if (!gAIEnv.pNavigation->IsPointInSpecialArea(end, *pArea))
    return false;

  const Lineseg navSeg( start, end );
  if ( Overlap::Lineseg_Polygon2D(navSeg, pArea->GetPolygon(), &pArea->GetAABB()) )
    return false;

	IAIPathAgent* pRequester = gAIEnv.pPathfinder->GetPathfindCurrentRequest()->pRequester;
  if ( pRequester && !OverlapCylinder(navSeg, pRequester->GetPathAgentPassRadius(), AICE_STATIC) )
    return true;

  return	false;

}

//===================================================================
// AttemptStraightPath
//===================================================================
float CFlightNavRegion::AttemptStraightPath(TPathPoints& straightPathOut,
											CAStarSolver * m_pAStarSolver,
                                            const CHeuristic* pHeuristic, const Vec3& start, const Vec3& end, 
                                            unsigned startHintIndex,
                                            float maxCost,
                                            const NavigationBlockers& navigationBlockers, bool returnPartialPath)
{

  float length = (end-start).GetLength();

  if ( length < 40.0 && CheckFlightNavLine(start,end) )
  {
    straightPathOut.push_back(PathPointDescriptor(IAISystem::NAV_FLIGHT, start));
    straightPathOut.push_back(PathPointDescriptor(IAISystem::NAV_FLIGHT, end));
    return	length;
  }

  return -1;

}
