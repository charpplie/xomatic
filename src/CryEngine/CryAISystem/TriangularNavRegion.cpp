/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   TriangularNavRegion.cpp
$Id$
Description: 

-------------------------------------------------------------------------
History:
- ?
- 4 May 2009  : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/

#include "StdAfx.h"
#include "TriangularNavRegion.h"
#include "AICollision.h"
#include "CAISystem.h"
#include "ISystem.h"
#include "I3DEngine.h"
#include "ISerialize.h"
#include "IConsole.h"
#include "DebugDrawContext.h"

#include <limits>

#define PI 3.141592653589793f

//====================================================================
// CTriangularNavRegion
//====================================================================
CTriangularNavRegion::CTriangularNavRegion(CGraph* pGraph)
{
  AIAssert(pGraph);
  m_pGraph = pGraph;
}

//====================================================================
// UglifyPath
//====================================================================
void CTriangularNavRegion::UglifyPath(const VectorConstNodeIndices& inPath, TPathPoints& outPath, 
                                      const Vec3& startPos, const Vec3& startDir, 
                                      const Vec3& endPos, const Vec3 & endDir)
{
  outPath.push_back(PathPointDescriptor(IAISystem::NAV_TRIANGULAR, startPos));
  for(VectorConstNodeIndices::const_iterator itrCurNode=inPath.begin() ; itrCurNode != inPath.end() ; ++itrCurNode)
  {
    const GraphNode& curNode=*m_pGraph->GetNodeManager().GetNode(*itrCurNode);
    outPath.push_back(PathPointDescriptor(IAISystem::NAV_TRIANGULAR, curNode.GetPos()));
  }
  outPath.push_back(PathPointDescriptor(IAISystem::NAV_TRIANGULAR, endPos));
}

//====================================================================
// SimplifiedLink
//====================================================================
class SimplifiedLink
{
public:
  Vec3	left;
  Vec3	right;
  SimplifiedLink(const Vec3& _left, const Vec3& _right) : left(_left), right(_right) {}
  SimplifiedLink() {}
};

typedef std::vector< SimplifiedLink > tLinks;
static tLinks debugLinks;

//====================================================================
// BeautifyPath
//====================================================================
void CTriangularNavRegion::BeautifyPath(const VectorConstNodeIndices& inPath, TPathPoints& outPath, 
                                        const Vec3& startPos, const Vec3& startDir, 
                                        const Vec3& endPos, const Vec3 & endDir,
                                        float radius,
                                        const AgentMovementAbility & movementAbility,
                                        const NavigationBlockers& navigationBlockers)
{
  FUNCTION_PROFILER( GetISystem(), PROFILE_AI );
  float extraR = gAIEnv.CVars.ExtraRadiusDuringBeautification;
  float extraFR = gAIEnv.CVars.ExtraForbiddenRadiusDuringBeautification;
  radius += extraR;

  static float extraNonCollRadius = 0.0f;
  if (movementAbility.pathfindingProperties.minWaterDepth > 0.0f)
    extraNonCollRadius = 20.0f; // for boats etc
  radius += extraNonCollRadius;

  if (inPath.size() <= 1)
  {
    return;
  }

  static tLinks links;
  links.resize(0);

  // Fix our starting position
  Vec3 _start(startPos);
  _start.z = 0;
  links.push_back(SimplifiedLink(_start, _start));

  // Track previous obstacles over iterations to help fill out the path
  unsigned int prevLeftObstIdx;
  unsigned int prevRightObstIdx;

  VectorConstNodeIndices::const_iterator itNodes, itNodesEnd = inPath.end();
  for (itNodes = inPath.begin(); itNodes != itNodesEnd; ++itNodes)
  {
    unsigned nodeIndex = *itNodes;
		GraphNode* node = m_pGraph->GetNodeManager().GetNode(nodeIndex);

    AIAssert(node->navType == IAISystem::NAV_TRIANGULAR);

    // _start is _supposed_ to be inside the first node anyway. If it's not, then
    // if we assumed that it is, it would generate a backwards segment, and this
    // messes up the subsequent beautification. So - skip the first node anyway - the link to
    // the next node will always be safe.
    // MTJ: Investigate this
    //if (itNodes == inPath.begin())
    //  continue;

    VectorConstNodeIndices::const_iterator itNext = itNodes;
    ++itNext;
    if (itNext == itNodesEnd)
      break;

    unsigned nodeNextIndex = *itNext;
    if (m_pGraph->GetNodeManager().GetNode(nodeNextIndex)->GetTriangularNavData()->vertices.empty())
      continue;

    // Find the link that connects this node to the next in the path
		unsigned link;
    for (link = node->firstLinkIndex; link; link = m_pGraph->GetLinkManager().GetNextLink(link))
      if (m_pGraph->GetLinkManager().GetNextNode(link) == nodeNextIndex)
        break;
    if (node->GetTriangularNavData()->vertices.empty() ||
      (m_pGraph->GetLinkManager().GetEndIndex(link) >= (uint32)node->GetTriangularNavData()->vertices.size()) ||
      (m_pGraph->GetLinkManager().GetStartIndex(link) >= (uint32)node->GetTriangularNavData()->vertices.size()) )
    {
      AIWarning("Error during path beautification - check for errors during AI loading");
      continue;
    }

    // Fetch indices of obstacles either side of this link
    const unsigned int startObstIdx = node->GetTriangularNavData()->vertices[m_pGraph->GetLinkManager().GetStartIndex(link)];
    const unsigned int endObstIdx = node->GetTriangularNavData()->vertices[m_pGraph->GetLinkManager().GetEndIndex(link)];

    // Fetch _2D_ positions of those obstacles, the node we are coming from and the centre of the edge we will pass thru
    const Vec2 startPos2( GetAISystem()->m_VertexList.GetVertex(startObstIdx).vPos );
    const Vec2 endPos2( GetAISystem()->m_VertexList.GetVertex(endObstIdx).vPos );
    const Vec2 nodePos( node->GetPos() );
    const Vec2 center( m_pGraph->GetLinkManager().GetEdgeCenter(link) );

    // Identify left and right obstacles, with respect to the direction we are walking the link
    bool inLinkDir = ((startPos2 - endPos2)^(center - nodePos)) < 0.0f;
    unsigned int leftObstIdx = (inLinkDir ? startObstIdx : endObstIdx);
    unsigned int rightObstIdx = (inLinkDir ? endObstIdx : startObstIdx);

    // Fetch data for left and right
    const ObstacleData &leftObst = GetAISystem()->m_VertexList.GetVertex(leftObstIdx);
    const ObstacleData &rightObst = GetAISystem()->m_VertexList.GetVertex(rightObstIdx);
    Vec2 left( leftObst.vPos );
    Vec2 right( rightObst.vPos );
    Vec2 dir = left - right;
    
    // if this triangle is next to forbidden, then increase the radius, but if neither is 
    // collidable then set radius = 0 so that we "hug" it
    float workingRadius = radius;
    float linkRadius= m_pGraph->GetLinkManager().GetRadius(link);
    float ptDist = (left - right).GetLength();
    if (linkRadius > ptDist)
      linkRadius = ptDist;

    if (!leftObst.IsCollidable() && !rightObst.IsCollidable())
    {
      workingRadius = extraNonCollRadius;
      dir.SetLength(linkRadius - workingRadius);
    }
    else
    {
      // Danny todo - this check must be very slow - speed it up!
      if (gAIEnv.pNavigation->IsPointOnForbiddenEdge(left, 0.0001f, 0, 0, false) || gAIEnv.pNavigation->IsPointOnForbiddenEdge(right, 0.0001f, 0, 0, false))
        workingRadius += extraFR;
      if (workingRadius > linkRadius)
        workingRadius = linkRadius;
      dir.SetLength(linkRadius - workingRadius);
    }
    left = center + dir;
    right = center - dir;

    // The radius of normal collidable things will have been incorporated into the link radius. For non-collidable things
    // we want to avoid them now (e.g. to avoid bushes) even though they shouldn't affect the unbeautified path - i.e.
    // they weren't included in the pass actually - radius. So, tweak the radius/pass center.
    Vec3 dirDir = dir.GetNormalizedSafe();
    if (!leftObst.IsCollidable() && leftObst.fApproxRadius > 0.0f)
    {
      left = Vec2(leftObst.vPos - leftObst.fApproxRadius * dirDir);
    }
    if (!rightObst.IsCollidable() && rightObst.fApproxRadius > 0.0f)
    {
      right = Vec2(rightObst.vPos + rightObst.fApproxRadius * dirDir);
    }

    // Create our new link, but don't yet push it
    SimplifiedLink newLink(left, right);

    // Check if we should insert an extra guiding link (relatively rare)
    if (itNodes != inPath.begin())
    {
      const SimplifiedLink &prevLink = links.back();
      Vec2 prevDir(prevLink.left - prevLink.right);
      
      bool shareLeft = ( prevLeftObstIdx == leftObstIdx );
      bool shareRight = ( prevRightObstIdx == rightObstIdx );
      bool dotSign = prevDir.Dot( dir ) > 0.0f;  // Length of the vectors isn't relevant

      if ( !dotSign && (shareLeft || shareRight))
      {
        Vec2 vSharedObst( (shareLeft ? leftObst : rightObst).vPos );
        SimplifiedLink extraLink;
        if (CreateExtraLink( vSharedObst, shareLeft, prevLink, newLink, extraLink ))
          links.push_back(extraLink);
      }
    }

    // Keep the left and right obstacle indices
    prevLeftObstIdx = leftObstIdx;
    prevRightObstIdx = rightObstIdx;
  
    // Push this link
    links.push_back(newLink);
  }

  // Fix end position
  Vec3 _end = endPos;
  _end.z = 0;
  links.push_back(SimplifiedLink(_end, _end));

  // MTJ: Show the simplified link corridor that is key to this approach
  if (gAIEnv.CVars.DebugDraw >= 74)
    debugLinks = links;
    else
    debugLinks.clear();


  bool more = true;
  while (more)
  {
    more = false;
    tLinks::iterator i1, i2, i3, iEnd = links.end();
    for (i1 = links.begin(); i1 != iEnd;)
    {
      i2 = i1;
      ++i2;

			if (i2 == iEnd)
				break;
			
			i3 = i2;
			++i3;
      if (i3 == iEnd)
        break;

      Vec3& i2left = i2->left;
      Vec3& i2right = i2->right;
      if (i2left == i2right)
      {
        ++i1;
        continue;
      }
      else if ((i2left-i2right).GetLengthSquared() < 0.5f)
      {
        i2left = i2right = (i2left+i2right)*0.5f;
        ++i1;
        continue;
      }

      Vec3& i1left = i1->left;
      Vec3& i1right = i1->right;
      Vec3& i3left = i3->left;
      Vec3& i3right = i3->right;

      Vec3 segAStart = i1left;
      Vec3 segBStart = i2left;
      Vec3 segADir = i3left - segAStart;
      Vec3 segBDir = i2right - segBStart;

      bool shortenLeft = false;

      Vec3 delta = segBStart - segAStart;
      float crossD = segADir.x*segBDir.y - segADir.y*segBDir.x;

      if ( fabs(crossD) > 0.01f )
      {
        // there is an intersection
        float crossDelta2 = delta.x*segADir.y - delta.y*segADir.x;
        float cut = crossDelta2/crossD;
        if (cut > 0.001f)
        {
          if (cut > 0.999f)
          {
            i2left = i2right;
            more = true;
            ++i1;
            continue;
          }
          else
          {
            i2left += segBDir * cut * 0.99f;
            more = true;
            shortenLeft = true;
          }
        }
      }

      segAStart = i1right;
      segBStart = i2right;
      segADir = i3right - segAStart;
      segBDir = i2left - segBStart;

      delta = segBStart - segAStart;
      crossD = segADir.x*segBDir.y - segADir.y*segBDir.x;

      if ( fabs(crossD) > 0.01f )
      {
        // there is an intersection
        float crossDelta2 = delta.x*segADir.y - delta.y*segADir.x;
        float cut = crossDelta2/crossD;
        if (cut > 0.001f)
        {
          if (cut > 0.999f)
          {
            i2right = i2left;
            more = true;
          }
          else if (shortenLeft)
          {
            // discard whole segment if both sides should be shortened
            links.erase(i2);
						iEnd = links.end();
            continue;
          }
          else
          {
            i2right += segBDir * cut * 0.99f;
            more = true;
          }
        }
      }
      ++i1;
    }
  }

  static bool doTidyup = true;
  if (doTidyup)
  {
    // At this point we can still have "corridors" where there is a sequence that starts with a 
    // co-located pair, and then after some spans finishes with a co-located pair.
    // If we identify those sequences then we can use the algorithm in AIPW3 (p 134) since (a)
    // the "corridor" will be basically a straight line (from the start to end of the sequence, which
    // may be unrelated to the start/end of the whole path) and (b) the AIPW algorithm only fails
    // if the path turns significfiantly away from the start/end.
    tLinks::iterator iEnd = links.end();
    for (tLinks::iterator iSeqStart= links.begin(); iSeqStart != iEnd; ++iSeqStart)
    {
      const SimplifiedLink &slStart = *iSeqStart;
      if (slStart.left.IsEquivalent(slStart.right))
        continue;

      int numDiff = 0;
      tLinks::iterator iSeqEnd;
      for (iSeqEnd = iSeqStart; iSeqEnd != iEnd; ++iSeqEnd)
      {
        const SimplifiedLink &slEnd = *iSeqEnd;
        if (slEnd.left.IsEquivalent(slEnd.right))
          break;
        ++numDiff;
      }

      AIAssert(iSeqEnd != iEnd);
      const SimplifiedLink &slEnd = *iSeqEnd;
      if (numDiff == 0)
        continue;

      Vec3 posEnd = slEnd.left;
      // slStart and slEnd represent the start and end of a corridor sequence
      for (tLinks::iterator i = iSeqStart-1 ; i != iSeqEnd ; ++i)
      {
        tLinks::iterator iNext = i+1;
        if (iNext == iSeqEnd)
          break;

        Lineseg pathSeg(i->left, posEnd);
        Lineseg line(iNext->left, iNext->right);

        float a, b;
        Intersect::Lineseg_Lineseg2D(pathSeg, line, a, b);
        Limit(b, 0.0f, 1.0f);

        iNext->left = line.GetPoint(b);
        iNext->right = iNext->left;
      }
      iSeqStart = iSeqEnd;
    }
  }

  const tLinks::iterator itLinksEnd = links.end();
  for (tLinks::iterator itLinks = links.begin() + 1; itLinks != itLinksEnd; ++itLinks)
  {
    const SimplifiedLink &link = *itLinks;
    Vec3	thePoint=(link.left+link.right) * 0.5f;
    thePoint.z = gEnv->p3DEngine->GetTerrainElevation(thePoint.x, thePoint.y);
    outPath.push_back(PathPointDescriptor(IAISystem::NAV_TRIANGULAR, thePoint));
  }

  return;
}

//===================================================================
// CreateExtraLink
// Where two edges have an angle between them approaching 180 degrees, our course will take us through the shared obstacle.
// Based on the distance of the two existing edges from the obstacle, we create a new edge to guide us around.
// The rest of the system implicitly assumes that obstacles are roughly round (ideally circular) - so we exploit that here.
// Params:  
//   pos of obstacle shared by both links, flag denoting whether this is on the left or the right,
//   the previous link, the new link we are about to add, output link that will be inserted between them 
//===================================================================
bool CTriangularNavRegion::CreateExtraLink( const Vec2 &vSharedObst, bool shareLeft,
  const SimplifiedLink &prevLink, const SimplifiedLink &newLink, SimplifiedLink &extraLink )
{
  // Find vectors from the shared obstacle to the link points
  Vec2 vPrevObstToLeft = Vec2(prevLink.left) - vSharedObst;
  Vec2 vPrevObstToRight = Vec2(prevLink.right) - vSharedObst;
  Vec2 vObstToLeft = Vec2(newLink.left) - vSharedObst;
  Vec2 vObstToRight = Vec2(newLink.right) - vSharedObst;

  // Find their lengths
  float fPrevObstToLeftDist = vPrevObstToLeft.GetLength();
  float fPrevObstToRightDist = vPrevObstToRight.GetLength();
  float fObstToLeftDist = vObstToLeft.GetLength();
  float fObstToRightDist = vObstToRight.GetLength();

  // Interpolate between normalised vectors to create new normalised vector. Using right would be equivalent.
  // Must be from normalised vectors to find the mid-angle even if sizes are different.
  // UPDATE Sep 3, 2009: <pvl> fixing a divide-by-zero bug: try to use right if
  // left is degenerated, fail altogether if both sides are bad
  Vec2 vNewObstDir;
  if (fPrevObstToLeftDist != 0.0f && fObstToLeftDist != 0.0f)
    vNewObstDir = (vPrevObstToLeft / fPrevObstToLeftDist + vObstToLeft / fObstToLeftDist) * 0.5f;
  else if (fPrevObstToRightDist != 0.0f && fObstToRightDist != 0.0f)
    vNewObstDir = (vPrevObstToRight / fPrevObstToRightDist + vObstToRight / fObstToRightDist) * 0.5f;
  else
    return false;

  vNewObstDir.Normalize();

  // Scale the interpolated vector to create a new near point and far point
  // Note that the far point should on the straight line between the old far points -
  // otherwise this extra link will not fit inside the polygon formed by the old links, which would be risky.
  if (shareLeft)
  {
    // Create a new left point that is at half the angle between the considered links, and
    // has the average distance from the obstacle. Ideal for circles, conservative for others.
    // Create a new farside link that is a simple interpolation.
    extraLink.left = vSharedObst + vNewObstDir * ( fPrevObstToLeftDist + fObstToLeftDist ) * 0.5f;
    extraLink.right = vSharedObst + (vPrevObstToRight * fObstToRightDist + vObstToRight * fPrevObstToRightDist)
      / (fPrevObstToRightDist + fObstToRightDist);

    // Check that near point is closer than far point
    // Note that as it is, a large obstacle connected to a thin triangle can potentially push points outside of
    // the triangle - hence this check. Ideally, we shouldn't have these thin triangles in the first place. 
    if ((extraLink.right - extraLink.left).Dot(vNewObstDir) < 0.0f) 
      extraLink.left = extraLink.right;
  }
  else
  {
    // Mirror image of above
    extraLink.left = vSharedObst + (vPrevObstToLeft * fObstToLeftDist + vObstToLeft * fPrevObstToLeftDist) 
      / (fPrevObstToLeftDist + fObstToLeftDist);
    extraLink.right = vSharedObst + vNewObstDir * ( fPrevObstToRightDist + fObstToRightDist ) * 0.5f;

    if ((extraLink.left - extraLink.right).Dot(vNewObstDir) < 0.0f) 
      extraLink.right = extraLink.left;
  }
  return true;
}


//===================================================================
// DebugDrawPathLinks
//===================================================================
void CTriangularNavRegion::DebugDrawPathLinks()
{
  const tLinks::iterator itLinksEnd = debugLinks.end();

  CDebugDrawContext dc;
  ColorB col(24, 56, 129);
  ColorB colL(129, 56, 56);  // Left
  ColorB colR(56, 129, 56);  // Right
  for (tLinks::iterator itLinks = debugLinks.begin(); itLinks != itLinksEnd; ++itLinks)
  {
    SimplifiedLink link = *itLinks;
    link.left.z = gEnv->p3DEngine->GetTerrainElevation(link.left.x, link.left.y);
    link.right.z = gEnv->p3DEngine->GetTerrainElevation(link.right.x, link.right.y);
    dc->DrawLine(link.left, col, link.right, col);
    if (link.left.IsEquivalent(link.right))
      dc->DrawSphere(link.left, 0.4f, col);
    else
    {
      dc->DrawSphere(link.left, 0.4f, colL);
      dc->DrawSphere(link.right, 0.4f, colR);
    }
  }
}

//====================================================================
// GreedyStep1
// iterative function to quickly converge on the target position in the graph
//====================================================================
unsigned CTriangularNavRegion::GreedyStep1(unsigned beginIndex, const Vec3 & pos)
{
	GraphNode* pBegin = m_pGraph->GetNodeManager().GetNode(beginIndex);
  AIAssert(pBegin && pBegin->navType == IAISystem::NAV_TRIANGULAR);

  if (PointInTriangle(pos,pBegin))
    return beginIndex;		// we have arrived

  m_pGraph->MarkNode(beginIndex);

  Vec3r triPos = pBegin->GetPos();
  for (unsigned link = pBegin->firstLinkIndex; link; link = m_pGraph->GetLinkManager().GetNextLink(link))
  {
		unsigned nextNodeIndex = m_pGraph->GetLinkManager().GetNextNode(link);
    GraphNode *pNextNode = m_pGraph->GetNodeManager().GetNode(nextNodeIndex);

    if (pNextNode->mark || pNextNode->navType != IAISystem::NAV_TRIANGULAR)
      continue;

    // cross to the next triangle if this edge splits space such that the current triangle and the
    // destination are on opposite sides.
    const ObstacleData &startObst = GetAISystem()->m_VertexList.GetVertex(pBegin->GetTriangularNavData()->vertices[m_pGraph->GetLinkManager().GetStartIndex(link)]);
    const ObstacleData &endObst = GetAISystem()->m_VertexList.GetVertex(pBegin->GetTriangularNavData()->vertices[m_pGraph->GetLinkManager().GetEndIndex(link)]);
    Vec3r edgeStart = startObst.vPos;
    Vec3r edgeEnd = endObst.vPos;
    Vec3r edgeDelta = edgeEnd - edgeStart;
    Vec3r edgeNormal(edgeDelta.y, -edgeDelta.x, 0.0f); // could point either way

    real dotDest = edgeNormal.Dot(pos - edgeStart);
    real dotTri = edgeNormal.Dot(triPos - edgeStart);
    if (sgn(dotDest) != sgn(dotTri))
    {
      return nextNodeIndex;
    }
  }
  return 0;
}

//===================================================================
// GreedyStep2
//===================================================================
unsigned CTriangularNavRegion::GreedyStep2(unsigned beginIndex, const Vec3 & pos)
{
	GraphNode* pBegin = m_pGraph->GetNodeManager().GetNode(beginIndex);
  AIAssert(pBegin && pBegin->navType == IAISystem::NAV_TRIANGULAR);

  if (PointInTriangle(pos,pBegin))
    return beginIndex;		// we have arrived

  m_pGraph->MarkNode(beginIndex);

  real bestDot = -1.0;
  unsigned bestLink = 0;

  Vec3r dirToPos = pos - pBegin->GetPos();
  dirToPos.z = 0.0f;
  dirToPos.NormalizeSafe();

  for (unsigned link = pBegin->firstLinkIndex; link; link = m_pGraph->GetLinkManager().GetNextLink(link))
  {
		unsigned nextNodeIndex = m_pGraph->GetLinkManager().GetNextNode(link);
    const GraphNode *pNextNode = m_pGraph->GetNodeManager().GetNode(nextNodeIndex);

    if (pNextNode->mark || pNextNode->navType != IAISystem::NAV_TRIANGULAR)
      continue;

    const ObstacleData &startObst = GetAISystem()->m_VertexList.GetVertex(pBegin->GetTriangularNavData()->vertices[m_pGraph->GetLinkManager().GetStartIndex(link)]);
    const ObstacleData &endObst = GetAISystem()->m_VertexList.GetVertex(pBegin->GetTriangularNavData()->vertices[m_pGraph->GetLinkManager().GetEndIndex(link)]);
    Vec3r edgeStart = startObst.vPos;
    Vec3r edgeEnd = endObst.vPos;
    Vec3r edgeDelta = (edgeEnd - edgeStart);
    Vec3r edgeNormal(edgeDelta.y, -edgeDelta.x, 0.0f); // points out - assumes correct winding
    edgeNormal.NormalizeSafe();
    if (edgeNormal.Dot(edgeStart - pBegin->GetPos()) < 0.0f)
      edgeNormal *= -1.0f;

    real thisDot = dirToPos.Dot(edgeNormal);
    if (thisDot > bestDot)
    {
      bestLink = link;
      bestDot = thisDot;
    }
  }

  if (bestLink)
  {
    return m_pGraph->GetLinkManager().GetNextNode(bestLink);
  }
  else
  {
    return 0;
  }
}

//====================================================================
// GetGoodPositionInTriangle
//====================================================================
Vec3 CTriangularNavRegion::GetGoodPositionInTriangle(const GraphNode *pNode, const Vec3 &pos) const
{
  Vec3 newPos(pos);
	const STriangularNavData *triangNavData = pNode->GetTriangularNavData();
	uint32	count = triangNavData->vertices.size();

  for (unsigned iVertex = 0 ; iVertex < count; ++iVertex)
  {
    int obIndex = triangNavData->vertices[iVertex];
    const ObstacleData& od = GetAISystem()->m_VertexList.GetVertex(obIndex);
    float distToObSq = Distance::Point_Point2DSq(newPos, od.vPos);
    if (distToObSq > 0.0001f && distToObSq < square(max(od.fApproxRadius, 0.0f)))
    {
      Vec3 dirToMove = newPos - od.vPos;
      dirToMove.z = 0.0f;
      dirToMove.Normalize();
      newPos = od.vPos + max(od.fApproxRadius, 0.0f) * dirToMove;
    }
  }
  return newPos;
}


//====================================================================
// GetEnclosing
//====================================================================
unsigned CTriangularNavRegion::GetEnclosing(const Vec3 &pos, float passRadius, unsigned startIndex, 
                                              float /*range*/, Vec3 * closestValid, bool returnSuspect, const char *requesterName)
{
  FUNCTION_PROFILER( GetISystem(), PROFILE_AI );
//Diesel cut
/*
  if (!m_pGraph->InsideOfBBox(pos))
  {
    AIWarning("CTriangularNavRegion::GetEnclosing (%5.2f %5.2f %5.2f) %s is outside of navigation bounding box",
      pos.x, pos.y, pos.z, requesterName);
    return 0;
  }
*/
//Diesel cut
  m_pGraph->ClearMarks();

  if (!startIndex || (m_pGraph->GetNodeManager().GetNode(startIndex)->navType != IAISystem::NAV_TRIANGULAR))
  {
		FRAME_PROFILER("CTriangularNavRegion::GetEnclosing Frame 0", GetISystem(), PROFILE_AI);
		CAllNodesContainer &allNodes = m_pGraph->GetAllNodes();
		std::pair<float, unsigned> node;

		//Simon says: use the new GetNodeWithinRange since only the first node is used
		if (allNodes.GetNodeWithinRange(node, pos, 15.0f, IAISystem::NAV_TRIANGULAR, m_pGraph))
		{
			startIndex = node.second;
		}
    else
    {
      CAllNodesContainer::Iterator it(allNodes, IAISystem::NAV_TRIANGULAR);
      startIndex = it.Increment();
//Diesel cut
/*
      if (!startIndex)
      {
        AIWarning("CTriangularNavRegion::GetEnclosing No triangular nodes in navigation");
        return 0;
      }
*/
//Diesel cut
    }
  }

  AIAssert(startIndex && m_pGraph->GetNodeManager().GetNode(startIndex)->navType == IAISystem::NAV_TRIANGULAR);

	unsigned nextNodeIndex = startIndex;
	unsigned prevNodeIndex = 0;
	int iterations = 0;

	{
		FRAME_PROFILER("CTriangularNavRegion::GetEnclosing Frame 1", GetISystem(), PROFILE_AI);
		while (nextNodeIndex && prevNodeIndex != nextNodeIndex)
		{
			++iterations;
			prevNodeIndex = nextNodeIndex;
			nextNodeIndex = GreedyStep1(prevNodeIndex, pos);
		}
		m_pGraph->ClearMarks();
	}
  // plan B - should not get called very much
  if (!nextNodeIndex)
  {
		FRAME_PROFILER("CTriangularNavRegion::GetEnclosing Frame 2", GetISystem(), PROFILE_AI);
    //AILogProgress("CTriangularNavRegion::GetEnclosing (%5.2f %5.2f %5.2f %s radius = %5.2f) Resorting to slower algorithm", 
    //pos.x, pos.y, pos.z, requesterName, passRadius);
    static int planBTimes = 0;
    ++planBTimes;
    nextNodeIndex = startIndex;
    prevNodeIndex = 0;
    iterations = 0;
		{
			FRAME_PROFILER("CTriangularNavRegion::GetEnclosing Frame 2.1", GetISystem(), PROFILE_AI);
			while (nextNodeIndex && prevNodeIndex != nextNodeIndex)
			{
				++iterations;
				prevNodeIndex = nextNodeIndex;
				nextNodeIndex = GreedyStep2(prevNodeIndex, pos);
			}
			m_pGraph->ClearMarks();
		}
  }
  // plan C - bad news if this gets called
  if(!nextNodeIndex)
  {
		//FRAME_PROFILER("CTriangularNavRegion::GetEnclosing Frame 3", GetISystem(), PROFILE_AI);
    //AILogProgress("CTriangularNavRegion::GetEnclosing (%5.2f %5.2f %5.2f %s radius = %5.2f) Resorting to brute-force algorithm",
    // pos.x, pos.y, pos.z, requesterName, passRadius);
    CAllNodesContainer &allNodes = m_pGraph->GetAllNodes();
    CAllNodesContainer::Iterator it(allNodes, IAISystem::NAV_TRIANGULAR);
    while (nextNodeIndex = it.Increment())
    {
      if (PointInTriangle(pos, m_pGraph->GetNodeManager().GetNode(nextNodeIndex)))
        break;
      else
        nextNodeIndex = 0;
    }
  }

	{
		FRAME_PROFILER("CTriangularNavRegion::GetEnclosing Frame 4", GetISystem(), PROFILE_AI);
		if (nextNodeIndex && PointInTriangle(pos, m_pGraph->GetNodeManager().GetNode(nextNodeIndex)))
		{
			if (closestValid)
				*closestValid = GetGoodPositionInTriangle(m_pGraph->GetNodeManager().GetNode(nextNodeIndex), pos);
			return nextNodeIndex; // or pPrevNode, they are the same
		}
/* //Diesel cut
		else
		{
			// silently fail if there's no navigation at all
			if (nextNodeIndex == m_pGraph->m_safeFirstIndex)
				return 0;
			AIWarning("CTriangularNavRegion::GetEnclosing DebugWalk bad result from looking for node containing %s position (%5.2f, %5.2f, %5.2f)", 
				requesterName, pos.x, pos.y, pos.z);
			return 0;
		}
*/ //Diesel cut
return nextNodeIndex; //Diesel new
	}
}

//====================================================================
// CheckPassability
//====================================================================
bool CTriangularNavRegion::CheckPassability(const Vec3& from, const Vec3& to, float radius, const NavigationBlockers& navigationBlockers, IAISystem::tNavCapMask ) const
{
	bool passible = false;

	Vec3 closestPoint;

	// Check forbidden areas and boundaries (or falls back on attempting to escape from one)
	if (!gAIEnv.pNavigation->IntersectsForbidden(from, to, closestPoint) || gAIEnv.pNavigation->IsPointForbidden(from, 0.0f))
	{
		// Ensure clearance by agent radius TODO: Does not appear to respect radius parameter...
		//if (!gAIEnv.pNavigation->IsPointOnForbiddenEdge(closestPoint, radius))
		{
			ListPositions boundary;
			passible = CheckWalkability(from, to, 0.0f, false, boundary, AICE_ALL);
		}
	}

	return passible;
}

//====================================================================
// Serialize
//====================================================================
void CTriangularNavRegion::Serialize(TSerialize ser, CObjectTracker& objectTracker)
{
  ser.BeginGroup("TriangularNavRegion");

  ser.EndGroup();
}

//====================================================================
// Reset
//====================================================================
void CTriangularNavRegion::Clear()
{
}

//====================================================================
// AttemptStraightPath
//====================================================================
float CTriangularNavRegion::AttemptStraightPath(TPathPoints &straightPathOut,
												CAStarSolver * m_pAStarSolver,
                                                const CHeuristic* pHeuristic, const Vec3& start, const Vec3& end, 
                                                unsigned startHintIndex,
                                                float maxCost,
                                                const NavigationBlockers& navigationBlockers, bool returnPartialPath)
{
  if (!pHeuristic)
    return -1.0f;

  const PathfindingHeuristicProperties *reqProperties = pHeuristic->GetProperties();
  if (!reqProperties)
    return -1.0f;

  unsigned startIndex = GetEnclosing(start, 0, startHintIndex);
	GraphNode* pStart = m_pGraph->GetNodeManager().GetNode(startIndex);
  if (!pStart || pStart->navType != IAISystem::NAV_TRIANGULAR)
    return -1.0f;
  if (pStart->GetTriangularNavData()->isForbidden)
    return -1.0f;

  unsigned endIndex = GetEnclosing(end, 0, startHintIndex);
	GraphNode* pEnd = m_pGraph->GetNodeManager().GetNode(endIndex);
  if (!pEnd || pEnd->navType != IAISystem::NAV_TRIANGULAR)
    return -1.0f;
  if (pEnd->GetTriangularNavData()->isForbidden)
    return -1.0f;

  if (pStart == pEnd)
  {
    straightPathOut.push_back(PathPointDescriptor(IAISystem::NAV_TRIANGULAR, end));
    return (end - start).GetLength();
  }

  float extraR = gAIEnv.CVars.ExtraRadiusDuringBeautification;
  float extraFR = gAIEnv.CVars.ExtraForbiddenRadiusDuringBeautification;

  // so - start and end are triangular - need to walk from one to the other
  straightPathOut.push_back(PathPointDescriptor(IAISystem::NAV_TRIANGULAR, start));
  unsigned currentNodeIndex = startIndex;
  SMarkClearer markClearer(m_pGraph);

  float pathCost = 0.0f;

  float heuristicZScale = 1.0f;
  if (pHeuristic->GetProperties())
    heuristicZScale *= pHeuristic->GetProperties()->agentproperties.zScale;
  Vec3 heuristicZScaleVec(1, 1, heuristicZScale);

  while (true)
  {
    const Vec3& currentPos = straightPathOut.back().vPos;

    m_pGraph->MarkNode(currentNodeIndex);

    if (currentNodeIndex == startIndex)
    {
      Vec3 delta = start - currentPos;
      pathCost += delta.CompMul(heuristicZScaleVec).GetLength();
    }

    // check if we're there
    if (currentNodeIndex == endIndex)
    {
      Vec3 delta = end - currentPos;
      pathCost += delta.CompMul(heuristicZScaleVec).GetLength();
      straightPathOut.push_back(PathPointDescriptor(IAISystem::NAV_TRIANGULAR, end));
      return pathCost;
    }

    // currentNode != pEnd

    // Find the triangle edge that we would cross if going to the destination
		unsigned link;
    for (link = m_pGraph->GetNodeManager().GetNode(currentNodeIndex)->firstLinkIndex; link; link = m_pGraph->GetLinkManager().GetNextLink(link))
    {
      if (m_pGraph->GetNodeManager().GetNode(m_pGraph->GetLinkManager().GetNextNode(link))->mark)
        continue;

			float heuristicCost = pHeuristic->CalculateCost(
				m_pGraph,
				*m_pAStarSolver->GetAStarNode(currentNodeIndex),
				link,
				*m_pAStarSolver->GetAStarNode(m_pGraph->GetLinkManager().GetNextNode(link)),
				navigationBlockers
				);
      if (heuristicCost < 0.0f)
        continue;
      float heuristicDist = (m_pGraph->GetNodeManager().GetNode(m_pGraph->GetLinkManager().GetNextNode(link))->GetPos() - m_pGraph->GetNodeManager().GetNode(currentNodeIndex)->GetPos()).CompMul(heuristicZScaleVec).GetLength();
      if (heuristicDist < 0.01f)
        continue;

      // Now find the point on the edge containing the link that intersects the straight path
      // in 2D.
      unsigned vertexStartIndex = m_pGraph->GetLinkManager().GetStartIndex(link);
      unsigned vertexEndIndex = m_pGraph->GetLinkManager().GetEndIndex(link);
      if (vertexStartIndex == vertexEndIndex)
        continue;
      const ObstacleData &startObst = GetAISystem()->m_VertexList.GetVertex(m_pGraph->GetNodeManager().GetNode(currentNodeIndex)->GetTriangularNavData()->vertices[vertexStartIndex]);
      const ObstacleData &endObst = GetAISystem()->m_VertexList.GetVertex(m_pGraph->GetNodeManager().GetNode(currentNodeIndex)->GetTriangularNavData()->vertices[vertexEndIndex]);
      Vec3 edgeStart = startObst.vPos;
      Vec3 edgeEnd = endObst.vPos;
      Vec3 edgeDir = (edgeEnd - edgeStart).GetNormalizedSafe(Vec3(1, 0, 0));
      Lineseg edgeSegment(edgeStart, edgeEnd);
      Lineseg segmentToEnd(currentPos, end);
      float linkRadius= m_pGraph->GetLinkManager().GetRadius(link);
      float ptDist = Distance::Point_Point2D(edgeStart, edgeEnd);
      if (linkRadius > ptDist)
        linkRadius = ptDist;
      float tA, tB;
      if (Intersect::Lineseg_Lineseg2D(segmentToEnd, edgeSegment, tA, tB))
      {
        Vec3 edgePt = edgeSegment.start + tB * (edgeSegment.end - edgeSegment.start);
        // constrain this edge point to be in a valid region
        float workingRadius = reqProperties->agentproperties.radius + extraR;
        if (!startObst.IsCollidable() && !endObst.IsCollidable())
        {
          workingRadius = 0.0f;
        }
        else if (gAIEnv.pNavigation->IsPointOnForbiddenEdge(edgeStart, 0.0001f, 0, 0, false) || gAIEnv.pNavigation->IsPointOnForbiddenEdge(edgeEnd, 0.0001f, 0, 0, false))
        {
            workingRadius += extraFR;
        }
        if (workingRadius > linkRadius)
          workingRadius = linkRadius;
        Vec3 edgeStartOK = m_pGraph->GetLinkManager().GetEdgeCenter(link) - edgeDir * (linkRadius - workingRadius);
        Vec3 edgeEndOK = m_pGraph->GetLinkManager().GetEdgeCenter(link) + edgeDir * (linkRadius - workingRadius);

        if (!startObst.IsCollidable() && startObst.fApproxRadius > 0.0f)
        {
          edgeStartOK = startObst.vPos + startObst.fApproxRadius * edgeDir;
        }
        if (!endObst.IsCollidable() && endObst.fApproxRadius > 0.0f)
        {
          edgeEndOK = endObst.vPos - endObst.fApproxRadius * edgeDir;
        }

        if ( ( (edgePt - edgeStartOK) | edgeDir ) < 0.0f)
          edgePt = edgeStartOK;
        else if ( ( (edgePt - edgeEndOK) | edgeDir ) > 0.0f)
          edgePt = edgeEndOK;

        // Estimate the cost of getting there
        Vec3 pathSegment = edgePt - currentPos;
        float pathSegmentDist = pathSegment.CompMul(heuristicZScaleVec).GetLength();
        float pathSegmentCost = heuristicCost * (pathSegmentDist / heuristicDist);

        // OK - move!
        straightPathOut.push_back(PathPointDescriptor(IAISystem::NAV_TRIANGULAR, edgePt));
        float oldCost = pathCost;
        pathCost += pathSegmentCost;
        currentNodeIndex = m_pGraph->GetLinkManager().GetNextNode(link);

        if (pathCost > maxCost)
        {
          if (pathCost == oldCost)
            return pathCost;
          unsigned nPts = straightPathOut.size();
          if (nPts < 2)
            return pathCost;
          // fraction where to truncate the path
          float frac = (maxCost - oldCost) / (pathCost - oldCost);
          TPathPoints::reverse_iterator it1 = straightPathOut.rbegin();
          TPathPoints::reverse_iterator it2 = it1;
          ++it2;
          Vec3 endPos = frac * it1->vPos + (1.0f - frac) * it2->vPos;
          it1->vPos = endPos;
          return maxCost;
        }

        break;
      }
    }
    if (!link)
      return returnPartialPath ? pathCost : -1.0f;
  }

  return pathCost;
}

//====================================================================
// Reset
//====================================================================
void CTriangularNavRegion::Reset(IAISystem::EResetReason reason)
{
}

//====================================================================
// Update
//====================================================================
void CTriangularNavRegion::Update(class CCalculationStopper& stopper)
{
  FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	// NOTE Feb 6, 2008: <pvl> there used to be "dynamic triangulation" update
	// code here, now it's unused
}

//===================================================================
// GetTeleportPosition
// Implement this by trying random positions away from curPos - checking
// the offsets against physics and forbidden areas etc
//===================================================================
bool CTriangularNavRegion::GetTeleportPosition(const Vec3 &curPos, Vec3 &teleportPos, const char *requesterName)
{
  Vec3 startPos = curPos; // may want some height offset too
  static int numRays = 10;
  static float minDist = 3.0f;
  static float maxDist = 25.0f;
  for (int iRay = 0 ; iRay != numRays ; ++iRay)
  {
    float angle = ai_frand() * gf_PI2;
    Vec3 dir(ZERO);
    cry_sincosf(angle, &dir.x, &dir.y);
    float dist = minDist + (maxDist - minDist) * iRay / (numRays - 1.0f);
    Vec3 delta = dist * dir;

    Lineseg seg(startPos, startPos+delta);

    // check forbidden
    Vec3 newPt = seg.end;
    if (gAIEnv.pNavigation->IntersectsForbidden(seg.start, seg.end, newPt))
    {
      seg.end = newPt;
      dist = Distance::Point_Point(seg.start, seg.end);
      float offset = min(dist, 0.5f);
      seg.end = newPt - dir * offset;
    }

    // check physics
    Vec3 hitPos(seg.end);
    if (IntersectSegment(hitPos, seg, AICE_ALL))
    {
      float offset = min(dist, 0.5f);
      hitPos -= dir * offset;
    }

    Vec3 floorPos;
    if (!GetFloorPos(floorPos, hitPos, 1.0f, 4.0f, walkabilityDownRadius, AICE_STATIC))
      continue;

    if (!CheckBodyPos(floorPos, AICE_ALL))
      continue;

    if (GetAISystem()->WouldHumanBeVisible(floorPos, false))
      continue;

    if (!GetEnclosing(floorPos))
      continue;

    teleportPos = floorPos;
    return true;
  }

  return false;
}
