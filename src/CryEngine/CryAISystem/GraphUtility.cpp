/********************************************************************
CryGame Source File.
Copyright (C), StalTech Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   GraphUtility.cpp
Version:     v1.00
Description: 

-------------------------------------------------------------------------
History:
- ?
- 4 May 2009  : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/

#include "StdAfx.h"
#include "Graph.h"
#include "Heuristic.h"
#include "CAISystem.h"
#include "AILog.h"

#include <ISystem.h>

#include <ILog.h>
#include <I3DEngine.h>
#include <algorithm>
#include <IConsole.h>
#include "IPhysics.h"


//====================================================================
// GetDegeneracyValue
//====================================================================
real	STriangularNavData::GetDegeneracyValue()
{
  CVertexList *pVList = &GetAISystem()->m_VertexList;

  Vec3r	firstE = pVList->GetVertex(vertices[1]).vPos - pVList->GetVertex(vertices[0]).vPos;
  Vec3r	secondE = pVList->GetVertex(vertices[2]).vPos - pVList->GetVertex(vertices[0]).vPos;

  firstE.Normalize();
  secondE.Normalize();
  real	cosAngle0 = 1.0f - firstE.Dot( secondE );

  firstE = pVList->GetVertex(vertices[0]).vPos - pVList->GetVertex(vertices[1]).vPos;
  secondE = pVList->GetVertex(vertices[2]).vPos - pVList->GetVertex(vertices[1]).vPos;
  firstE.Normalize();
  secondE.Normalize();
  real	cosAngle1 = 1.0f - firstE.Dot( secondE );

  firstE = pVList->GetVertex(vertices[0]).vPos - pVList->GetVertex(vertices[2]).vPos;
  secondE = pVList->GetVertex(vertices[1]).vPos - pVList->GetVertex(vertices[2]).vPos;
  firstE.Normalize();
  secondE.Normalize();
  real	cosAngle2 = 1.0f - firstE.Dot( secondE );

  if( cosAngle0<=cosAngle1 && cosAngle0<=cosAngle2 )
    return cosAngle0;
  if( cosAngle1<=cosAngle2 && cosAngle1<=cosAngle0 )
    return cosAngle1;
  return cosAngle2;
}

//====================================================================
// MakeAntiClockwise
//====================================================================
void STriangularNavData::MakeAntiClockwise()
{
  if (vertices.size() < 3) 
    return;

  CVertexList *pVList = &GetAISystem()->m_VertexList;

  int od1 = vertices[0];
  int od2 = vertices[1];
  int od3 = vertices[2];

  Vec3r one = pVList->GetVertex(od2).vPos - pVList->GetVertex(od1).vPos;
  Vec3r two = pVList->GetVertex(od3).vPos - pVList->GetVertex(od2).vPos;

  real fcrossz = one.x * two.y - two.x * one.y;

  if (fcrossz<0)
  {
    // rearrange the first and second
    vertices.clear();

    vertices.push_back(od2);
    vertices.push_back(od1);
    vertices.push_back(od3);

    //		MakeAntiClockwise();
  }
}


//====================================================================
// IsAntiClockwise
//====================================================================
bool STriangularNavData::IsAntiClockwise()
{
  if (vertices.size() < 3) 
    return false;

  CVertexList *pVList = &GetAISystem()->m_VertexList;

  int od1 = vertices[0];
  int od2 = vertices[1];
  int od3 = vertices[2];

  Vec3r one = pVList->GetVertex(od2).vPos - pVList->GetVertex(od1).vPos;
  Vec3r two = pVList->GetVertex(od3).vPos - pVList->GetVertex(od2).vPos;

  real fcrossz = one.x * two.y - two.x * one.y;

  if (fcrossz<0)
    return false;
  return true;
}


//====================================================================
// DbgCheckList
//====================================================================
bool CGraph::DbgCheckList( ListNodeIds& nodesList )	const
{
  bool isGood = true;
  int ndNumber=0;

  for(ListNodeIds::iterator it=nodesList.begin(); it!=nodesList.end(); it++, ndNumber++)
  {
    int counter=0;
		unsigned curNodeIndex = *it;
		const GraphNode* curNode = GetNodeManager().GetNode(curNodeIndex);
    for (unsigned link = curNode->firstLinkIndex; link; link = GetLinkManager().GetNextLink(link))
      if( GetLinkManager().IsNewLink(link) ) // it's on the edge - it's a new node
        counter++;
    if(counter > 1)
      isGood = false;
    if( curNode->GetTriangularNavData()->vertices.size()<3 )
      isGood = false;

    if (curNode->nRefCount>1 && curNode->nRefCount<4)
      isGood = false;
    ListNodeIds::iterator li;
    ListNodeIds::iterator itnext = it;
    if (++itnext != nodesList.end())
    {
      li = std::find(itnext,nodesList.end(),(*it));
      if (li!=nodesList.end())
        isGood = false;

    }
  }

  return isGood;
}

//====================================================================
// GetCross
//====================================================================
real STriangularNavData::GetCross(CGraphLinkManager& linkManager, const Vec3r &vCutStart, const Vec3r &vDir, unsigned theLink )
{
  Vec3r	theOtherVertex = GetAISystem()->m_VertexList.GetVertex(vertices[3-(linkManager.GetStartIndex(theLink) + linkManager.GetEndIndex(theLink))]).vPos 
    - vCutStart;
  theOtherVertex.normalize();
  return vDir.x * theOtherVertex.y - vDir.y * theOtherVertex.x;
}

