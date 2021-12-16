/*
Copyright (c) 2007, Michael Kazhdan
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of
conditions and the following disclaimer. Redistributions in binary form must reproduce
the above copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the distribution. 

Neither the name of the Johns Hopkins University nor the names of its contributors
may be used to endorse or promote products derived from this software without specific
prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO THE IMPLIED WARRANTIES 
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.
*/
#define USE_MAX_DEPTH_SPEED_UP 1

#include <stdio.h>
#include <stdlib.h>
#include "terrain_sector.h"
#include "IsoMesh.h"
#include "DecalRenderNode.h"
#include "RoadRenderNode.h"
#include "IndexedMesh.h"
#include "MeshCompiler/MeshCompiler.h"
#include "Brush.h"
#include "MatMan.h"

#ifdef WIN32
#include <windows.h>
#endif //WIN32

#include "CryThread.h"

extern const float fPredictionDistance;
int IsoOctree::nMeshDepth = 0;
extern ETEX_Format eIsoTreeTexFormat;
extern const float fIsoTreeCurvature;
float isoValue = 0;//.01f/4096.f;

PodArray<IDynTexture*> CVoxTerrain::m_arrTexturesForRel;
PodArray<struct SIsoMesh*> CVoxTerrain::m_arrIsoMeshesForDel;
int IsoOctree::maxDepth = 19 * sizeof(ISO_KEY) / sizeof(uint64);
float IsoOctree::m_lastEditingTime=0;
extern float fPhysMeshSize;
extern int IsoOctree_nModId;

const float gEditingRenderModeDelay = 1.f;

AABB IsoOctree::m_aabbDirty = AABB(Vec3(1.f,1.f,1.f),Vec3(-1.f,-1.f,-1.f));
AABB IsoOctree::m_aabbPhysDirty = AABB(Vec3(1.f,1.f,1.f),Vec3(-1.f,-1.f,-1.f));
TFlatness IsoOctree::m_flatnessMap;

float CVoxTerrain::m_arrTimeStats[20];

namespace
{
  CryCriticalSection g_cAddForRelease;
  CryCriticalSection g_cTaksIn;
  CryCriticalSection g_cTaksOut;
  CryCriticalSection g_cIsoTreeAccess;
  CryCriticalSection g_cIsoNodesForUpdate;
  CryCriticalSection g_cIsoMeshes;
  CryCriticalSection g_cPhysMeshes;
}

SIsoMesh * pNotNULL = (SIsoMesh *)-1;

const int nMaxEdgesNum = 100;

Vec3 _RootPosition(const Vec3& p1,const Vec3& p2,SVoxValue v1,SVoxValue v2,SSurfTypeInfo &surf_type,ColorB & color)
{
  float diff = v1.val-v2.val;
  float t = diff ? ((v1.val-isoValue) / diff) : 0.5f;

  surf_type.Lerp(t, v1.surf_type, v2.surf_type);

  ColorF color1(v1.color[0],v1.color[1],v1.color[2]);
  color1 /= 255;
  ColorF color2(v2.color[0],v2.color[1],v2.color[2]);
  color2 /= 255;

  color = (color1*(float(1.0)-t)+color2*t);

  return p1*(float(1.0)-t)+p2*t;
}

/////////////
// EdgeKey //
/////////////
EdgeKey::EdgeKey(const size_t& n1,const size_t& n2)
{
  if(n1<n2)
  {
    key1=n1;
    key2=n2;
  }
  else
  {
    key1=n2;
    key2=n1;
  }
}
EdgeKey::operator size_t() const
{
  return (size_t)key1;
}
EdgeKey::operator size_t()
{
  return (size_t)key1;
}
EdgeKey& EdgeKey::operator = (const EdgeKey& key)
{
  key1=key.key1;
  key2=key.key2;
  return *this;
}
bool EdgeKey::operator < (const EdgeKey& key) const
{
  if(key1<key.key1)	return true;
  else if(key1==key.key1)
    if(key2<key.key2)
      return true;
    else
      return false;
  else
    return false;
}
bool EdgeKey::operator != (const EdgeKey& key) const
{
  if(key1!=key.key1 || key2!=key.key2)
    return true;
  else
    return false;
}
/////////////////
// NeighborKey //
/////////////////

OctNode* NeighborKey::__FaceNeighbor(OctNode* node,const int& depth,int dir,int off)
{
  if(!depth)	return NULL;
  int x,y,z;
  x=y=z=1;
  switch(dir)
  {
  case 0:
    x=off<<1;
    break;
  case 1:
    y=off<<1;
    break;
  case 2:
    z=off<<1;
    break;
  }
  for(int d=depth;d>=0;d--)
    if(neighbors[d].neighbors[x][y][z])
      return neighbors[d].neighbors[x][y][z];

  return NULL;
}

OctNode* NeighborKey::__EdgeNeighbor(OctNode* node,const int& depth,int o,int i1,int i2)
{
  if(!depth)	return NULL;
  int x,y,z,cIndex,xx,yy,zz;
  x=y=z=1;

  // Check if the edge-adjacent neighbor exists at the current depth
  switch(o)
  {
  case 0:
    y=i1<<1;
    z=i2<<1;
    break;
  case 1:
    x=i1<<1;
    z=i2<<1;
    break;
  case 2:
    x=i1<<1;
    y=i2<<1;
    break;
  }
  if(neighbors[depth].neighbors[x][y][z])
    return neighbors[depth].neighbors[x][y][z];

  cIndex=int(node-node->parent->children);
  Cube::FactorCornerIndex(cIndex,xx,yy,zz);

  // Check if the node is on the corresponding edge of the parent
  switch(o)
  {
  case 0:
    if(yy==i1 && zz==i2)	return __EdgeNeighbor(node->parent,depth-1,o,i1,i2);
    break;
  case 1:
    if(xx==i1 && zz==i2)	return __EdgeNeighbor(node->parent,depth-1,o,i1,i2);
    break;
  case 2:
    if(xx==i1 && yy==i2)	return __EdgeNeighbor(node->parent,depth-1,o,i1,i2);
    break;
  }

  // Check if the node is on a face of the parent containing the edge
  switch(o)
  {
  case 0:
    if(yy==i1)	return __FaceNeighbor(node->parent,depth-1,1,yy);
    if(zz==i2)	return __FaceNeighbor(node->parent,depth-1,2,zz);
    break;
  case 1:
    if(xx==i1)	return __FaceNeighbor(node->parent,depth-1,0,xx);
    if(zz==i2)	return __FaceNeighbor(node->parent,depth-1,2,zz);
    break;
  case 2:
    if(xx==i1)	return __FaceNeighbor(node->parent,depth-1,0,xx);
    if(yy==i2)	return __FaceNeighbor(node->parent,depth-1,1,yy);
    break;
  }
  printf("IsoOctree: We shouldn't be here: Edges");
  return NULL;
}

OctNode* NeighborKey::__CornerNeighbor(OctNode* node,const int& depth,int x,int y,int z)
{
  if(!depth)	return NULL;
  int cIndex,xx,yy,zz;

  // Check if the edge-adjacent neighbor exists at the current depth
  if(neighbors[depth].neighbors[x<<1][y<<1][z<<1])
    return neighbors[depth].neighbors[x<<1][y<<1][z<<1];

  cIndex=int(node-node->parent->children);
  Cube::FactorCornerIndex(cIndex,xx,yy,zz);

  // Check if the node is on the corresponding corner of the parent
  if(xx==x && yy==y && zz==z)	return __CornerNeighbor(node->parent,depth-1,x,y,z);

  // Check if the node is on an edge of the parent containing the corner
  if(xx==x && yy==y)			return __EdgeNeighbor(node->parent,depth-1,2,x,y);
  if(xx==x && zz==z)			return __EdgeNeighbor(node->parent,depth-1,1,x,z);
  if(yy==y && zz==z)			return __EdgeNeighbor(node->parent,depth-1,0,y,z);

  // Check if the node is on a face of the parent containing the edge
  if(xx==x)					return __FaceNeighbor(node->parent,depth-1,0,x);
  if(yy==y)					return __FaceNeighbor(node->parent,depth-1,1,y);
  if(zz==z)					return __FaceNeighbor(node->parent,depth-1,2,z);

  printf("IsoOctree: We shouldn't be here: Corners");
  return NULL;
}

void NeighborKey::__GetCornerNeighbors(OctNode* node,const int& d,const int& c,OctNode* neighbors[Cube::CORNERS])
{
  int x,y,z,xx,yy,zz,ax,ay,az;
  Cube::FactorCornerIndex(c,x,y,z);
  ax=x^1;
  ay=y^1;
  az=z^1;
  xx=x<<0;
  yy=y<<1;
  zz=z<<2;
  ax<<=0;
  ay<<=1;
  az<<=2;

  // Set the current node
  neighbors[ax|ay|az]=node;

  // Set the face adjacent neighbors
  neighbors[xx|ay|az]=__FaceNeighbor(node,d,0,x);
  neighbors[ax|yy|az]=__FaceNeighbor(node,d,1,y);
  neighbors[ax|ay|zz]=__FaceNeighbor(node,d,2,z);

  // Set the edge adjacent neighbors
  neighbors[ax|yy|zz]=__EdgeNeighbor(node,d,0,y,z);
  neighbors[xx|ay|zz]=__EdgeNeighbor(node,d,1,x,z);
  neighbors[xx|yy|az]=__EdgeNeighbor(node,d,2,x,y);

  // Set the corner adjacent neighbor
  neighbors[xx|yy|zz]=__CornerNeighbor(node,d,x,y,z);
}

void NeighborKey::GetCornerNeighbors(OctNode* node,const int& c,OctNode* neighbors[Cube::CORNERS])
{
  getNeighbors(node);
  memset(neighbors,NULL,sizeof(OctNode*)*Cube::CORNERS);
  __GetCornerNeighbors(node,depth,c,neighbors);
}

void NeighborKey::CornerIndex(const int& c,int idx[3])
{
  int x,y,z;
  Cube::FactorCornerIndex(c,x,y,z);
  idx[0]=x<<1;
  idx[1]=y<<1;
  idx[2]=z<<1;
}

void NeighborKey::EdgeIndex(const int& e,int idx[3])
{
  int o,i1,i2;
  Cube::FactorEdgeIndex(e,o,i1,i2);
  idx[0]=idx[1]=idx[2]=1;
  switch(o)
  {
  case 0:
    idx[1]=i1<<1;
    idx[2]=i2<<1;
    break;
  case 1:
    idx[0]=i1<<1;
    idx[2]=i2<<1;
    break;
  case 2:
    idx[0]=i1<<1;
    idx[1]=i2<<1;
    break;
  }
}

void NeighborKey::FaceIndex(const int& f,int idx[3])
{
  int dir,off;
  Cube::FactorFaceIndex(f,dir,off);
  idx[0]=idx[1]=idx[2]=1;
  idx[dir]=off<<1;
}
///////////////
// IsoOctree //
///////////////

int IsoOctree::HasEdgeGrandChildren(const OctNode* node,int eIndex)
{
  if(!node || !node->children)
    return 0;
  int c1,c2;
  Cube::EdgeCorners(eIndex,c1,c2);
  if(node->children[c1].children || node->children[c2].children)
    return 1;
  else
    return 0;
}

int IsoOctree::HasFaceGrandChildren(const OctNode* node,int fIndex)
{
  if(!node || !node->children)
    return 0;
  int c1,c2,c3,c4;
  Cube::FaceCorners(fIndex,c1,c2,c3,c4);
  if(node->children[c1].children || node->children[c2].children || node->children[c3].children || node->children[c4].children)
    return 1;
  else
    return 0;
}




void IsoOctree::MeshInfo::set(const std::vector<Vec3>& verts,const std::vector<std::vector<int> > & polys,const float& width,
                              Vec3& translate,float& scale,const int& noTransform)
{
  meshBox.Reset();
  if(!noTransform)
  {
    Vec3 min,max;
    for(size_t i=0;i<verts.size();i++)
    {
      for(int j=0;j<3;j++)
      {
        if(!i || Vec3(verts[i])[j]<min[j])	min[j]=Vec3(verts[i])[j];
        if(!i || Vec3(verts[i])[j]>max[j])	max[j]=Vec3(verts[i])[j];
      }
    }

    scale=max[0]-min[0];
    if( (max[1]-min[1])>scale )		scale=max[1]-min[1];
    if( (max[2]-min[2])>scale )		scale=max[2]-min[2];

    scale*=width;
    scale=float(1.0)/scale;
    Vec3 ctr;
    ctr[0]=ctr[1]=ctr[2]=0.5f;

    translate=ctr/scale-(max+min)/2;
  }
  else
  {
    translate[0]=translate[1]=translate[2]=0;
    scale=1;
  }
  vertices.resize(verts.size());
  for(size_t i=0;i<verts.size();i++)
  {
    for(int j=0;j<3;j++)
      vertices[i][j]=float((translate[j]+(Vec3(verts[i]))[j])*scale);

    meshBox.Add(vertices[i]);
  }

  size_t pSize=0;
  size_t vSize=verts.size();
  for(size_t i=0;i<polys.size();i++)
  {
    if(polys.size()<3) continue;
    if(polys[i].size()==3)
    {
      Vec3 N=Normal(vertices[polys[i][0]],vertices[polys[i][1]],vertices[polys[i][2]]);
      if(Length(N)==0) continue;
      triangles.resize(pSize+1);
      for(int j=0;j<3;j++) triangles[pSize].idx[j]=polys[i][j];
      pSize++;
    }
    else
    {
      triangles.resize(pSize+polys[i].size());
      Vec3 ctr;
      ctr[0]=ctr[1]=ctr[2]=0;
      for(size_t j=0;j<polys[i].size();j++) ctr+=vertices[polys[i][j]];
      ctr/=float(polys[i].size());
      vertices.push_back(ctr);
      for(size_t j=0;j<polys[i].size();j++)
      {
        triangles[pSize+j].idx[0]=polys[i][j];
        triangles[pSize+j].idx[1]=polys[i][(j+1)%polys[i].size()];
        triangles[pSize+j].idx[2]=int(vSize);
      }
      vSize++;
      pSize+=polys[i].size();
    }
  }
  vertexNormals.resize(vertices.size());
  triangleNormals.resize(triangles.size());

  for(size_t i=0;i<vertices.size();i++)
    vertexNormals[i][0]=vertexNormals[i][1]=vertexNormals[i][2]=0;

  for(size_t i=0;i<triangles.size();i++)
  {
    triangleNormals[i]=Normal(vertices[triangles[i].idx[0]],vertices[triangles[i].idx[1]],vertices[triangles[i].idx[2]]);
    triangleNormals[i]/=Length(triangleNormals[i]);
    for(int j=0;j<3;j++)
    {
      // accumulate pseudo normal
      Vec3 v0 = vertices[triangles[i].idx[(j+0)%3]];
      Vec3 v1 = vertices[triangles[i].idx[(j+1)%3]];
      Vec3 v2 = vertices[triangles[i].idx[(j+2)%3]];
      Vec3 a = (v1-v0).GetNormalized();
      Vec3 b = (v2-v0).GetNormalized();
      float dot = a.dot(b);
      if(dot<0.999f)
      {
        if(dot<-1.f)
          dot=-1.f;
        float angle = acos(dot);
        vertexNormals[triangles[i].idx[j]] += triangleNormals[i]*angle;
      }
    }
    for(int j=0;j<3;j++)
    {
      EdgeKey eKey(triangles[i].idx[j],triangles[i].idx[(j+1)%3]);
      if(edgeNormals.find(eKey)==edgeNormals.end())
        edgeNormals[eKey]=triangleNormals[i];
      else
        edgeNormals[eKey]+=triangleNormals[i];
    }
  }
  for(size_t i=0;i<vertices.size();i++)
    vertexNormals[i]/=Length(vertexNormals[i]);

  for(std::map<EdgeKey,Vec3 >::iterator iter=edgeNormals.begin();iter!=edgeNormals.end();iter++)
    iter->second/=Length(iter->second);
}



int IsoOctree::setFromMesh(const std::vector<Vec3>& vertices,const std::vector<std::vector<int> > & polygons,const int& maxDepth,const int& setCenter,
                           const float& flatness,const int& noTransform, int nSurfaceType, const ColorB & color, const SBrushAABB * pBrushAABB, const EVoxelEditOperation & eOperation)
{
  Vec3 t;
  float s;
  return setFromMesh(vertices,polygons,maxDepth,setCenter,flatness,t,s,noTransform, nSurfaceType, color, pBrushAABB, eOperation);
}

int IsoOctree::setFromMesh(const std::vector<Vec3>& vertices,const std::vector<std::vector<int> > & polygons,
  const int& maxDepth,const int& setCenter,const float& flatness,
  Vec3& translate,float& scale,const int& noTransform, int nSurfaceType, const ColorB & color,
  const SBrushAABB * pBrushAABB, const EVoxelEditOperation & eOperation )
{
  FUNCTION_PROFILER_3DENGINE;

  this->maxDepth=maxDepth;
  OctNode::NodeIndex nIdx;

//  TVoxValues valuesBk;
  //CollectValues( &tree,nIdx, valuesBk, &pBrushAABB->boxEps );

  MeshInfo mInfo;
  std::vector<int> myTriangles;
  mInfo.set(vertices,polygons,float(1.1),translate,scale,noTransform);
  myTriangles.resize(mInfo.triangles.size());
  for(int i=0;i<int(mInfo.triangles.size());i++) 
    myTriangles[i]=i;

  if(!myTriangles.size())
    return 1;

//  TRoots visitedValues;

  Vec3 p;
  for(int c=0;c<Cube::CORNERS;c++)
  {
    int x,y,z;
    Cube::FactorCornerIndex(c,x,y,z);
    p[0]=float(x);
    p[1]=float(y);
    p[2]=float(z);

    setDistanceFromMesh(myTriangles,mInfo,p,OctNode::CornerIndex(nIdx,c,maxDepth), nSurfaceType, color, eOperation, NULL);
  }

  setChildrenFromMesh(nSurfaceType, color, &tree,nIdx,
    myTriangles,
    myTriangles,
    mInfo,maxDepth,setCenter,flatness, pBrushAABB, NULL/*&valuesBk*/, eOperation, true, NULL);

  if(GetCVars()->e_VoxTerDebug)
    setMCIndex(0);

  return 1;
}


/*
int IsoOctree::setConforming(const std::vector<Vec3>& vertices,const std::vector<std::vector<int> > & polygons,
const int& maxDepth,const int& setCenter,const float& flatness,const int& noTransform)
{
Vec3 t;
float s;
return setConforming(vertices,polygons,maxDepth,setCenter,flatness,t,s,noTransform);
}


int IsoOctree::setConforming(const std::vector<Vec3>& vertices,const std::vector<std::vector<int> > & polygons,
const int& maxDepth,const int& setCenter,const float& flatness,
Vec3& translate,float& scale,const int& noTransform)
{
this->maxDepth=maxDepth;
OctNode::NodeIndex nIdx;
std::vector<int> myTriangles;
std::map<ISO_KEY,std::vector<int>*> triangleMap;
MeshInfo  mInfo;

mInfo.set(vertices,polygons,float(1.1),translate,scale,noTransform);
myTriangles.resize(mInfo.triangles.size());
for(int i=0;i<int(mInfo.triangles.size());i++)
myTriangles[i]=i;

(*m_pValues).clear();
float dist;
Vec3 n,p;
for(int c=0;c<Cube::CORNERS;c++)
{
int x,y,z;
Cube::FactorCornerIndex(c,x,y,z);
p[0]=float(x);
p[1]=float(y);
p[2]=float(z);

setDistanceFromMesh(myTriangles,mInfo,p,dist,n);
(*m_pValues)[OctNode::CornerIndex(nIdx,c,maxDepth)]=float(dist);
}
if(setCenter)
{
float w;
OctNode::CenterAndWidth(nIdx,tree.nodeData.center,w);
setDistanceFromMesh(myTriangles,mInfo,tree.nodeData.center,dist,n);
if(tree.nodeData.val>dist)
tree.nodeData.val = dist;
}
setChildren(&tree,nIdx,myTriangles,mInfo,maxDepth,setCenter,flatness,&triangleMap);

// Refine non-conforming nodes
int forceConforming=1;
while(forceConforming)
{
forceConforming=0;
nIdx=OctNode::NodeIndex();
for(OctNode* node=tree.nextLeaf(NULL,nIdx) ; node ; node=tree.nextLeaf(node,nIdx) )
{
int setChildren=0;
for(int i=0;i<Cube::FACES && !setChildren;i++)
if(HasFaceGrandChildren(node->faceNeighbor(i),Cube::FaceReflectFaceIndex(i,i)))
setChildren=1;
for(int i=0;i<Cube::EDGES && !setChildren;i++)
if(HasEdgeGrandChildren(node->edgeNeighbor(i),Cube::EdgeReflectEdgeIndex(i)))
setChildren=1;
if(setChildren)
{
OctNode* temp=node;
OctNode::NodeIndex pIdx=nIdx;
ISO_KEY key;
while(temp)
{
key=OctNode::CenterIndex(pIdx,maxDepth);
if(triangleMap.find(key)==triangleMap.end() || !triangleMap[key])
{
temp=temp->parent;
--pIdx;
}
else
break;
}
if(!temp)
{
printf(stderr,"Could not find ancestor with triangles");
continue;
}
node->initChildren();
forceConforming=1;

for(int i=0;i<Cube::CORNERS;i++)
{
Vec3 ctr;
float w;
OctNode::CenterAndWidth(nIdx.child(i),ctr,w);
for(int c=0;c<Cube::CORNERS;c++)
{
int x,y,z;
Cube::FactorCornerIndex(c,x,y,z);
p[0]=ctr[0]-w/2+w*x;
p[1]=ctr[1]-w/2+w*y;
p[2]=ctr[2]-w/2+w*z;

setDistanceFromMesh(*triangleMap[key],mInfo,p,dist,n);
ISO_KEY k=OctNode::CornerIndex(nIdx.child(i),c,maxDepth);
if((*m_pValues).find(k)==(*m_pValues).end() || dist<(*m_pValues)[k])
(*m_pValues)[k]=float(dist);
}
if(setCenter)
{
node->children[i].nodeData.center=ctr;
setDistanceFromMesh(*triangleMap[key],mInfo,ctr,dist,n);
node->children[i].nodeData.v=float(dist);
}
}
}
}
}

for(std::map<ISO_KEY,std::vector<int>*>::iterator iter=triangleMap.begin();iter!=triangleMap.end();iter++)
if(iter->second)
delete iter->second;
return 1;
}*/

#  pragma warning(disable: 4244) // conversion from 'long ISO_DOUBLE' to 'ISO_DOUBLE', possible loss of data



bool IsoOctree::setDistanceFromMesh( const std::vector<int>& triangles,
                                     MeshInfo& mInfo,
                                     const Vec3& p,
                                     ISO_KEY key,
                                     int nSurfaceType,
                                     const ColorB & color,
                                     const EVoxelEditOperation & eOperation,
                                     TRoots * pVisitedValues)
{
//  TRoots::iterator iterVisited = pVisitedValues->find(key);
//  if( iterVisited != pVisitedValues->end() )
  //  return iterVisited->second ? true : false;

  Vec3 pp,t[3];
  pp[0]=p[0];
  pp[1]=p[1];
  pp[2]=p[2];
  size_t closest;
  float dist = 1000000;
  for(size_t i=0;i<triangles.size();i++)
  {
    float temp;
    for(int j=0;j<3;j++)
    {
      t[0][j]=mInfo.vertices[mInfo.triangles[triangles[i]].idx[0]][j];
      t[1][j]=mInfo.vertices[mInfo.triangles[triangles[i]].idx[1]][j];
      t[2][j]=mInfo.vertices[mInfo.triangles[triangles[i]].idx[2]][j];
    }

    temp=DistanceToTriangle(pp,t);
    if(!i || temp<dist )
    {
      closest=i;
      dist=temp;
    }
  }
  Vec3 nn;
  int vFlag;

  closest=triangles[closest];

  for(int i=0;i<3;i++) 
    t[i]=mInfo.vertices[mInfo.triangles[closest].idx[i]];

  Vec3 n2=NearestPointOnTriangle(pp,t,vFlag);
  n2=(pp-n2)/CalcDistance(pp,n2);

  switch(vFlag)
  {
  case 7:
    nn=mInfo.triangleNormals[closest];
    break;
  case 1:
    nn=mInfo.vertexNormals[mInfo.triangles[closest].idx[0]];
    break;
  case 2:
    nn=mInfo.vertexNormals[mInfo.triangles[closest].idx[1]];
    break;
  case 4:
    nn=mInfo.vertexNormals[mInfo.triangles[closest].idx[2]];
    break;
  case 3:
    nn=mInfo.edgeNormals[EdgeKey(mInfo.triangles[closest].idx[0],mInfo.triangles[closest].idx[1])];
    break;
  case 5:
    nn=mInfo.edgeNormals[EdgeKey(mInfo.triangles[closest].idx[0],mInfo.triangles[closest].idx[2])];
    break;
  case 6:
    nn=mInfo.edgeNormals[EdgeKey(mInfo.triangles[closest].idx[1],mInfo.triangles[closest].idx[2])];
    break;
  }

  if(Overlap::Point_AABB(pp, mInfo.meshBox) && DotProduct(nn,n2)<0)
  {
    dist=-dist;
    n2*=-1;
  }

  if(eOperation == eveoIntegrateMeshNeg)
	{
		dist = -dist;
		if(GetCVars()->e_VoxTerOnTheFlyIntegration)
			dist += 0.025f/CVoxTerrain::m_fMapSize;
	}

  bool bValueMOdified = false;

  TVoxValues::iterator iter = (*m_pValues).find(key);

  if( iter == (*m_pValues).end() )
  {
    SVoxValue vv;
    vv.val = dist;
    vv.surf_type = nSurfaceType;
    vv.color[0] = color[0];
    vv.color[1] = color[1];
    vv.color[2] = color[2];
    (*m_pValues)[key]=vv;
    bValueMOdified = true;
  }
  else if( eOperation == eveoIntegrateMeshPos && iter->second.val >= dist )
  {
    iter->second.val = dist;
    iter->second.surf_type = nSurfaceType;
    iter->second.color[0] = color[0];
    iter->second.color[1] = color[1];
    iter->second.color[2] = color[2];
    bValueMOdified = true;
  }
  else if( eOperation == eveoIntegrateMeshNeg && iter->second.val <= dist )
  {
    iter->second.val = dist;
    iter->second.surf_type = nSurfaceType;
    iter->second.color[0] = color[0];
    iter->second.color[1] = color[1];
    iter->second.color[2] = color[2];
    bValueMOdified = true;

		if(GetCVars()->e_VoxTerOnTheFlyIntegration)
		{
			float x = p.x*CVoxTerrain::m_fMapSize;
			float y = p.y*CVoxTerrain::m_fMapSize;

			float fTerrainZ = 0;
			if(C3DEngine::m_pGetLayerIdAtCallback)
				fTerrainZ = C3DEngine::m_pGetLayerIdAtCallback->GetElevationAtPosition(y/GetTerrain()->GetHeightMapUnitSize(),x/GetTerrain()->GetHeightMapUnitSize());

			ColorB colB = Col_DarkGray;

			int surf_type = 0;
			if(C3DEngine::m_pGetLayerIdAtCallback)
			{
				surf_type = C3DEngine::m_pGetLayerIdAtCallback->GetLayerIdAtPosition((int)(y/GetTerrain()->GetHeightMapUnitSize()),(int)(x/GetTerrain()->GetHeightMapUnitSize()));
				if(surf_type<0 || surf_type>=MAX_SURFACE_TYPES_COUNT || 
					(!Get3DEngine()->m_arrBaseTextureData[surf_type].szBaseTexName[0] && !Get3DEngine()->m_arrBaseTextureData[surf_type].szDetMatName[0]))
					surf_type = 0;

				if(!surf_type)
				{
					while(surf_type < (Get3DEngine()->m_arrBaseTextureData.Count()-1) && 
						(!Get3DEngine()->m_arrBaseTextureData[surf_type].szBaseTexName[0] || Get3DEngine()->m_arrBaseTextureData[surf_type].nDetailSurfTypeId))
						surf_type++;
				}

				colB = C3DEngine::m_pGetLayerIdAtCallback->GetColorAtPosition(y,x,true);
			}

			fTerrainZ = max(fTerrainZ,4.125f);

			ColorF col;

			col.r = 1.f/255.f*colB.r;
			col.g = 1.f/255.f*colB.g;
			col.b = 1.f/255.f*colB.b;

			col.srgb2rgb();

			col *= (255.f);

			dist = p.z - fTerrainZ/CVoxTerrain::m_fMapSize;

			SVoxValue vv;
			vv.val = dist;
			vv.surf_type = surf_type;
			vv.color[0] = SATURATEB((int)col.r);
			vv.color[1] = SATURATEB((int)col.g);
			vv.color[2] = SATURATEB((int)col.b);

			if( vv.val < iter->second.val )
			{
				iter->second = vv;  
				bValueMOdified = true;
			}
		}
  }

  if(bValueMOdified)
    m_aabbDirty.Add(p);

//  (*pVisitedValues)[key] = bValueMOdified ? 1 : 0;

  return bValueMOdified;
}

void IsoOctree::setDistanceAndNormalFromSphere(const Sphere & sp, const Vec3& p, float& dist, ISO_KEY key, EVoxelEditOperation eOperation, int surf_type, ColorB color, IsoOctree * isoTreeBk, TRoots * pVisitedValues)
{
  if( pVisitedValues->find(key) != pVisitedValues->end() )
    return;
  (*pVisitedValues)[key] = 1;

  float alpha=0;

  bool bMofified = false;

  if(eOperation == eveoPaintHeightPos || eOperation == eveoPaintHeightNeg)
  {
    dist = (p - sp.center).GetLength();
  }
  else if(eOperation == eveoBlurPos || eOperation == eveoBlurNeg)
  {
    dist = p.z - sp.center.z;
    Vec3 dir = p - sp.center;
    alpha = 1.f - sqrtf(p.GetSquaredDistance(sp.center))/sp.radius;
    alpha = sqrtf(SATURATE(alpha));
  }
  else
  {
    dist = sp.center.GetDistance(p) - sp.radius;
  }

  if(eOperation == eveoBlurPos || eOperation == eveoBlurNeg)
  {
    if(alpha<=0)
      return;

    float fSumm = 0;
    float fCounter = 0;

    float fEditSize = 1.f;
    for(int d=0; d<editDepth; d++)
      fEditSize *= 0.5f;

    for(int x=-1; x<=1; x+=2)
    {
      for(int y=-1;y <=1; y+=2)
      {
        for(int z=-1; z<=1; z+=2)
        {
          OctNode::NodeIndex nIdx;     
          Vec3 offset(x,y,z);
          float fVal=0;
          isoTreeBk->GetValueByPosition(&isoTreeBk->tree, nIdx, p + offset * fEditSize, &fVal, editDepth);
          fSumm += fVal;
          fCounter ++;
        }
      }
    }

    fSumm /= fCounter;

    TVoxValues::iterator iter = m_pValues->find(key);

    if( iter == (*m_pValues).end() )
    {
      SVoxValue vv;

      if((fSumm>vv.val && eOperation == eveoBlurNeg) || (fSumm<vv.val && eOperation == eveoBlurPos))
      {
        vv.val = vv.val*(1.f-alpha) + fSumm*alpha;
        (*m_pValues)[key] = vv;
        bMofified = true;
      }
    }
    else
    {
      SVoxValue & vv = iter->second;

      if((fSumm>vv.val && eOperation == eveoBlurNeg) || (fSumm<vv.val && eOperation == eveoBlurPos))
      {
        vv.val = vv.val*(1.f-alpha) + fSumm*alpha;
      }
    }
  }
  else if(eOperation == eveoPaintHeightPos || eOperation == eveoPaintHeightNeg)
  {
    TVoxValues::iterator iter = m_pValues->find(key);

    if( iter != m_pValues->end() )
    {
      float fRatio = SATURATE(1.f - dist/sp.radius);

      if(fRatio > 0)
      {
        if(eOperation == eveoPaintHeightPos)
          iter->second.val -= (sp.radius * 0.2f) * fRatio;
        else
          iter->second.val += (sp.radius * 0.2f) * fRatio;

        bMofified = true;
      }
    }
  }
  else if(eOperation == eveoCreate)
  {
    TVoxValues::iterator iter = m_pValues->find(key);

    if( iter == m_pValues->end() || iter->second.val > dist )
    {
      SVoxValue vv;
      vv.val = dist;
      vv.surf_type = surf_type;
      vv.color[0] = color[0];
      vv.color[1] = color[1];
      vv.color[2] = color[2];

      if(iter == m_pValues->end())
        (*m_pValues)[key] = vv;
      else
        iter->second = vv;

      bMofified = true;
    }
  }
  else if(eOperation == eveoMaterial || eOperation == eveoBaseColor)
  {
    if(dist<0)
    {
      TVoxValues::iterator iter = m_pValues->find(key);
      if( iter != m_pValues->end() )
      {
        // soft painting of base color only
        float dist1 = (p - sp.center).GetLength();
        float fRatio = 0.5f*SATURATE(1.f - dist1/sp.radius);

        if(eOperation != eveoBaseColor)
        {
          // hard painting with surface type
          SSurfTypeInfo eInfo = surf_type;
          iter->second.surf_type.Lerp(fRatio, iter->second.surf_type, eInfo);          
        }
        else
        {
          iter->second.color[0] = (uint8)(fRatio*(float)color[0] + (1.f-fRatio)*(float)iter->second.color[0]);
          iter->second.color[1] = (uint8)(fRatio*(float)color[1] + (1.f-fRatio)*(float)iter->second.color[1]);
          iter->second.color[2] = (uint8)(fRatio*(float)color[2] + (1.f-fRatio)*(float)iter->second.color[2]);
        }

        bMofified = true;
      }
    }
  }
  else if(eOperation == eveoSubstract)
  {
    dist = -dist;
    if( (*m_pValues).find(key)==(*m_pValues).end() || (*m_pValues)[key]<dist )
    {
      SVoxValue vv;
      vv.val = dist;
      vv.surf_type = surf_type;
      vv.color[0] = color[0];
      vv.color[1] = color[1];
      vv.color[2] = color[2];
      (*m_pValues)[key] = vv;
      bMofified = true;
    }
  }
  else if(eOperation == eveoCopyTerrainPos || eOperation == eveoCopyTerrainNeg)
  {
    float x = p.x*CVoxTerrain::m_fMapSize;
    float y = p.y*CVoxTerrain::m_fMapSize;

    float fTerrainZ = 0;
    Vec3 vNormal(0,0,0);
    if(C3DEngine::m_pGetLayerIdAtCallback)
    {
      fTerrainZ = C3DEngine::m_pGetLayerIdAtCallback->GetElevationAtPosition(y/GetTerrain()->GetHeightMapUnitSize(),x/GetTerrain()->GetHeightMapUnitSize());
      vNormal = GetTerrain()->GetTerrainSurfaceNormal(Vec3(x,y,0),.5f);

      if(eOperation == eveoCopyTerrainPos)
      {
        if(sqrt(sp.center.GetSquaredDistance2D(p)) > sp.radius)
          fTerrainZ = 0;
      }
      else
      {
        if(sqrt(sp.center.GetSquaredDistance2D(p)) > sp.radius)
          fTerrainZ = GetTerrain()->GetTerrainSize();
      }
    }

    ColorB colB = Col_DarkGray;

    if(C3DEngine::m_pGetLayerIdAtCallback)
    {
      surf_type = C3DEngine::m_pGetLayerIdAtCallback->GetLayerIdAtPosition((int)(y/GetTerrain()->GetHeightMapUnitSize()),(int)(x/GetTerrain()->GetHeightMapUnitSize()));
      if(surf_type<0 || surf_type>=MAX_SURFACE_TYPES_COUNT || 
        (!Get3DEngine()->m_arrBaseTextureData[surf_type].szBaseTexName[0] && !Get3DEngine()->m_arrBaseTextureData[surf_type].szDetMatName[0]))
        surf_type = 0;

      if(!surf_type)
      {
        while(surf_type < (Get3DEngine()->m_arrBaseTextureData.Count()-1) && 
          (!Get3DEngine()->m_arrBaseTextureData[surf_type].szBaseTexName[0] || Get3DEngine()->m_arrBaseTextureData[surf_type].nDetailSurfTypeId))
          surf_type++;
      }

      colB = C3DEngine::m_pGetLayerIdAtCallback->GetColorAtPosition(y,x,true);

      surf_type = CLAMP(surf_type, 0, MAX_VOXTER_STYPES_NUM-1);
    }

    fTerrainZ = max(fTerrainZ,4.125f);

    ColorF col;

    col.r = 1.f/255.f*colB.r;
    col.g = 1.f/255.f*colB.g;
    col.b = 1.f/255.f*colB.b;

    col.srgb2rgb();

    col *= (255.f/**4*/);

    dist = p.z - fTerrainZ/CVoxTerrain::m_fMapSize;

    SVoxValue vv;
    vv.val = dist;
    vv.surf_type = surf_type;
    vv.color[0] = SATURATEB((int)col.r);
    vv.color[1] = SATURATEB((int)col.g);
    vv.color[2] = SATURATEB((int)col.b);
    vv.normal[0] = (uint8)SATURATEB(vNormal.x*127.f+127.f);
    vv.normal[1] = (uint8)SATURATEB(vNormal.y*127.f+127.f);
    vv.normal[2] = (uint8)SATURATEB(vNormal.z*127.f+127.f);

		TVoxValues::iterator iterOld = (*m_pValues).find(key);

    if( (iterOld == (*m_pValues).end()) || 
      (eOperation == eveoCopyTerrainPos && vv.val < iterOld->second.val) || 
      (eOperation == eveoCopyTerrainNeg && vv.val > iterOld->second.val) )
    {
      (*m_pValues)[key] = vv;  
      bMofified = true;
    }
  }

  if(bMofified)
    m_aabbDirty.Add(p);
}

bool IsoOctree::RequestMeshUpdate(OctNode* node, const OctNode::NodeIndex& nIdx, const AABB & areaBox, const AABB & brushBox)
{
#if !defined(XENON) && !defined(PS3)

  ISO_KEY key = OctNode::CenterIndex( nIdx, maxDepth );

  {
    AUTO_LOCK(g_cIsoMeshes);

    TIsoMeshes::iterator iter = threadMeshes.find(key);
    if( iter != threadMeshes.end() )
    {     
      iter->second->m_nRebuildRequestedFrameId = GetMainFrameID();
      iter->second->RequestTextureUpdate();

      bool b2DMixMask = GetCVars()->e_VoxTerHeightmapEditing && GetCVars()->e_VoxTerMixMask;

      if(GetCVars()->e_VoxTerTexBuildOnCPU && !b2DMixMask)
      {
        char szFileName[256]="";
        SIsoMesh::MakeTempTextureFileName(szFileName, sizeof(szFileName),0, "MTT", key);
        remove(szFileName);
        SIsoMesh::MakeTempTextureFileName(szFileName, sizeof(szFileName),0, "BIN", key);
        remove(szFileName);
        SIsoMesh::MakeTempTextureFileName(szFileName, sizeof(szFileName),1, "BIN", key);
        remove(szFileName);
      }
    }
  }

  {
    AUTO_LOCK(g_cIsoMeshes);

    TIsoMeshes::iterator iter = threadEmptyMeshes.find(key);
    if( iter != threadEmptyMeshes.end() )
    {
      threadEmptyMeshesDelta[iter->first] = NULL;
      threadEmptyMeshes.erase(iter);
    }
  }

  if( (nIdx.depth>=nMeshDepth) || !node->children ) // && !GetCVars()->e_VoxTerTexBuildOnCPU) )
    return true;

  for(int i=0;i<Cube::CORNERS;i++)
    if(Overlap::AABB_AABB(areaBox, GetNodeAABB(nIdx.child(i),false)))
      RequestMeshUpdate(&node->children[i], nIdx.child(i), areaBox, brushBox);

#endif

  return true;
}

void IsoOctree::CheckMemoryLimit()
{
#if !defined(XENON) && !defined(PS3) && !defined(LINUX)
  { // check memory
    static bool bCHeckMem = true;
    if(m_bEditor && bCHeckMem && GetEngineMemoryUsageMB()>1024*GetCVars()->e_VoxTerMemWarning)
    {
      bCHeckMem = false;
      char szMessage[512]="";
      sprintf(szMessage, "More than %d GB of RAM is allocated by application\nPress OK to continue or Cancel to terminate process", GetCVars()->e_VoxTerMemWarning);
      if(IDOK != CryMessageBox(szMessage, "Memory warning", MB_OKCANCEL | MB_ICONWARNING))
        exit(0);
    }
  }
#endif
}

bool IsoOctree::PrintProgress()
{
	if(GetCVars()->e_VoxTerLog)
	{
		static byte nCounter=0;
		nCounter++;

		if( !nCounter )
		{
      CheckMemoryLimit();

			float fLastTime = GetCurTimeSec(); // not Async!
			static float fTime = 0;
			if(fLastTime > fTime + 1.f)
			{
				PrintMessagePlus(".");
				fTime = fLastTime;
			}
		}
	}

  if(!Get3DEngine()->m_pVoxTerrain->IsContinue())
    return true;
 
  return false;
}

void IsoOctree::setChildrenFromSphere(OctNode* node,
                                      const OctNode::NodeIndex& nIdx,
                                      const Sphere & sp,
                                      const int& maxDepth,const int& setCenter,
                                      const float& flatness,
                                      EVoxelEditOperation eOperation, 
                                      CTerrainNode * pTerrainNode,
                                      int surfr_type, ColorB color,
                                      const SBrushAABB * pBrushAABB,
                                      IsoOctree * isoTreeBk,
                                      TRoots * pVisitedValues)
{
  if(nIdx.depth<=nMeshDepth)
    if(PrintProgress())
      return;

  float w,dist;
  Vec3 ctr;

#ifdef VOX_DVR
  node->nColor = node->nNormal = node->nOcNodeInfoId=0;
  if(node->nTmpChild)
    node->deleteChildren(m_aabbDirty, GetNodeAABB(nIdx));
#endif
  if(nIdx.depth==maxDepth)	
    return;

  OctNode::CenterAndWidth(nIdx,ctr,w);

  if(eOperation == eveoSubstract)
  {
    /*    float fDist = sp.center.GetDistance(ctr);
    if((fDist + w*0.86f) < sp.radius)
    {
    if(node->children)
    node->deleteChildren(m_aabbDirty, GetNodeAABB(nIdx));
    return;
    }
    else if(fDist > (w*0.86f + sp.radius))
    {
    return;
    }*/
  }
  else if(eOperation == eveoCreate)
  {
    float fDist = sp.center.GetDistance(ctr);
     
    if(nIdx.depth>nMeshDepth)
    {
      if((fDist + w*0.7f) < (sp.radius - 1.f/CVoxTerrain::m_fMapSize))
      {
        if(node->children)
          node->deleteChildren(m_aabbDirty, GetNodeAABB(nIdx));
        return;
      }
    }
  }
  else if(eOperation == eveoPaintHeightPos || eOperation == eveoPaintHeightNeg)
  {
/*    float fDist = sp.center.GetDistance(ctr);

    if(nIdx.depth>nMeshDepth)
    {
      if((fDist + w*0.7f) < (sp.radius - 1.f/CVoxTerrain::m_fMapSize))
      {
        if(node->children)
          node->deleteChildren(m_aabbDirty, GetNodeAABB(nIdx));
        return;
      }
    }*/
  }
  else if(eOperation == eveoCopyTerrainPos || eOperation == eveoCopyTerrainNeg)
  {
    /*if(pTerrainNode && pTerrainNode->m_pGeomErrors)
    {
      if(pTerrainNode->m_pGeomErrors[0]<1.f)
        return;
    }*/

    /*if(pTerrainNode)
    {
      AABB hmBox = pTerrainNode->GetBBox();
      hmBox.max /= CVoxTerrain::m_fMapSize;
      hmBox.min /= CVoxTerrain::m_fMapSize;

      if(nIdx.depth>nMeshDepth)
      if((ctr.z + w*0.5f) < hmBox.min.z)
      {
        if(node->children)
          node->deleteChildren(m_aabbDirty, GetNodeAABB(nIdx));
        return;
      }

      if(nIdx.depth>nMeshDepth)
      if((ctr.z - w*0.5f) > hmBox.max.z)
      {
        if(node->children)
          node->deleteChildren(m_aabbDirty, GetNodeAABB(nIdx));
        return;
      }
    }*/
  }

  //  if(flatness>0 && w < sp.radius*0.1f)
  /*  if(eOperation != eveoCopyTerrain)
  {
  float cValues[Cube::CORNERS];
  for(int i=0;i<Cube::CORNERS;i++)
  {
  if((*m_pValues).find(OctNode::CornerIndex(nIdx,i,maxDepth))==(*m_pValues).end())
  printf(stderr,"Could not find value in corner value table!");
  cValues[i]=(*m_pValues)[OctNode::CornerIndex(nIdx,i,maxDepth)];
  }

  int mcIndex = MarchingCubes::GetIndex(cValues,0);

  if(mcIndex == 255)
  {
  ISO_KEY llKey = OctNode::CenterIndex( nIdx, maxDepth );
  if( isoMeshes.find(llKey) != isoMeshes.end() )
  {
  isoMeshes[llKey]->m_bRebuildRequested = true;
  }

  node->deleteChildren(m_aabbDirty, GetNodeAABB(nIdx));
  return;
  }
  }*/

  if(!node->children)	
  {
    /*if(GetCVars()->e_VoxTerDebug)
    {
      SVoxValue childVals[8];

      if(node->children)
      { // take childs
        for(int i=0; i<8; i++)
          childVals[i] = (node->children[i].voxValue);
      }
      else
      { // take real values
        SVoxValue voxValAverage;
        for(int c=0;c<Cube::CORNERS;c++)
        {
          TVoxValues::iterator iter = (*m_pValues).find(OctNode::CornerIndex(nIdx,c,maxDepth));
          if(iter==(*m_pValues).end())
            childVals[c] = SVoxValue();
          else
            childVals[c] = iter->second;
        }
      }

      node->voxValue = SVoxValue::Average(&childVals[0], 8);
    }*/

    return; // everything is already preallocated
  }

  // Set the center
  setDistanceAndNormalFromSphere(sp,ctr,dist,OctNode::CenterIndex(nIdx,maxDepth), eOperation, surfr_type, color, isoTreeBk, pVisitedValues);

  // Set the edge mid-points
  for(int i=0;i<Cube::EDGES;i++)
  {
    int o,i1,i2;
    Cube::FactorEdgeIndex(i,o,i1,i2);
    Vec3 p=ctr;
    p[0]-=w/2;
    p[1]-=w/2;
    p[2]-=w/2;
    p[o]=ctr[o];
    switch(o)
    {
    case 0:
      p[1]+=w*i1;
      p[2]+=w*i2;
      break;
    case 1:
      p[0]+=w*i1;
      p[2]+=w*i2;
      break;
    case 2:
      p[0]+=w*i1;
      p[1]+=w*i2;
      break;
    }
    setDistanceAndNormalFromSphere(sp,p,dist,OctNode::EdgeIndex(nIdx,i,maxDepth),eOperation, surfr_type, color, isoTreeBk, pVisitedValues);
  }

  // set the face mid-points
  for(int i=0;i<Cube::FACES;i++)
  {
    int dir,off;
    Cube::FactorFaceIndex(i,dir,off);
    Vec3 p=ctr;
    p[dir]+=-w/2+w*off;
    setDistanceAndNormalFromSphere(sp,p,dist,OctNode::FaceIndex(nIdx,i,maxDepth),eOperation, surfr_type, color, isoTreeBk, pVisitedValues);
  }

  for(int i=0;i<Cube::CORNERS;i++)
  {
    OctNode::CenterAndWidth(nIdx.child(i),ctr,w);

    CTerrainNode * pChildTerrainNode = pTerrainNode ? pTerrainNode->m_arrChilds[i&3] : NULL;

    if(IsBrushAffectsNode(ctr,w,eOperation,CVoxTerrain::m_fMapSize,pChildTerrainNode,pBrushAABB)) 
    {
      setChildrenFromSphere(&node->children[i],nIdx.child(i),sp,maxDepth,setCenter,flatness,eOperation,pChildTerrainNode, surfr_type, color, pBrushAABB, isoTreeBk, pVisitedValues);
    }
  }

  /*if(GetCVars()->e_VoxTerDebug)
  {
    SVoxValue childVals[8];

    if(node->children)
    { // take childs
      for(int i=0; i<8; i++)
        childVals[i] = (node->children[i].voxValue);
    }
    else
    { // take real values
      SVoxValue voxValAverage;
      for(int c=0;c<Cube::CORNERS;c++)
      {
        TVoxValues::iterator iter = (*m_pValues).find(OctNode::CornerIndex(nIdx,c,maxDepth));
        if(iter==(*m_pValues).end())
          childVals[c] = SVoxValue();
        else
          childVals[c] = iter->second;
      }
    }

    node->voxValue = SVoxValue::Average(&childVals[0], 8);
  }*/
}

void IsoOctree::setChildrenFromMesh(int nSurfaceType, const ColorB & color,
  OctNode* node, const OctNode::NodeIndex& nIdx,
  const std::vector<int>& trianglesForThisNode,
  const std::vector<int>& trianglesForFromParent,
  MeshInfo& mInfo,
  const int& maxDepth,const int& setCenter,
  const float& flatness,
  const SBrushAABB * pBrushAABB, TVoxValues * pValuesBk, const EVoxelEditOperation & eOperation,
  bool bSetValues, TRoots * pVisitedValues)
{
  if(nIdx.depth<=nMeshDepth)
    if(PrintProgress())
      return;

  Vec3 ctr; float w;
  OctNode::CenterAndWidth(nIdx,ctr,w);

#ifdef VOX_DVR
  node->nColor = node->nNormal = node->nOcNodeInfoId=0;
  if(node->nTmpChild)
    node->deleteChildren(m_aabbDirty, GetNodeAABB(nIdx));
#endif

  if(nIdx.depth==maxDepth)	
    return;

  if(!node->children)	
  {
    if(!trianglesForThisNode.size())
      return;

    if(GetEditDepth() <= nIdx.depth)	
      return;

    if(!IsVisibleFromGamePlayArea(GetNodeAABB(nIdx,true)))
      return;
  }

  // check corners min/max distances
  float fMinCornersDistance = 100000;
  float fMaxCornersDistance =-100000;
  for(int c=0;c<Cube::CORNERS;c++)
  {
    TVoxValues::iterator iter = (*m_pValues).find(OctNode::CornerIndex(nIdx,c,maxDepth));
    if(iter != (*m_pValues).end())
    {
      fMinCornersDistance = min(iter->second.val,fMinCornersDistance);
      fMaxCornersDistance = max(iter->second.val,fMaxCornersDistance);
    }
  }

  // too far away from surface and outside
  if(fMinCornersDistance > w)
  {
    TVoxValues::iterator iter = (*m_pValues).find(OctNode::CenterIndex(nIdx,maxDepth));
    if(iter != (*m_pValues).end())
      (*m_pValues).erase(iter);

    node->deleteChildren(m_aabbDirty, GetNodeAABB(nIdx));
    return;
  }

  // too far away from surface and inside
  if(fMaxCornersDistance < -w)
  {
    TVoxValues::iterator iter = (*m_pValues).find(OctNode::CenterIndex(nIdx,maxDepth));
    if(iter != (*m_pValues).end())
      (*m_pValues).erase(iter);

    node->deleteChildren(m_aabbDirty, GetNodeAABB(nIdx));
    return;
  }

  if(!node->children)	
  {
    node->initChildren();

    // prepare prev values for this node
    if(pValuesBk)
    interpolateChilds(nIdx, node, pValuesBk, pValuesBk);

    // interpolate and merge new and prev values
    interpolateChilds(nIdx, node, m_pValues, m_pValues);
  }

  const std::vector<int> * pTris = trianglesForThisNode.size() ? &trianglesForThisNode : &trianglesForFromParent;

  bool bValuesModified = false;

  if(bSetValues)
  {
    { // Set the center
      bValuesModified |= setDistanceFromMesh(*pTris,mInfo,ctr,OctNode::CenterIndex(nIdx,maxDepth), nSurfaceType, color, eOperation, pVisitedValues);
    }

    // Set the edge mid-points
    for(int i=0;i<Cube::EDGES;i++)
    {
      Vec3 p;

      int o,i1,i2;
      Cube::FactorEdgeIndex(i,o,i1,i2);
      p=ctr;
      p[0]-=w/2;
      p[1]-=w/2;
      p[2]-=w/2;
      p[o]=ctr[o];
      switch(o)
      {
      case 0:
        p[1]+=w*i1;
        p[2]+=w*i2;
        break;
      case 1:
        p[0]+=w*i1;
        p[2]+=w*i2;
        break;
      case 2:
        p[0]+=w*i1;
        p[1]+=w*i2;
        break;
      }
      bValuesModified |= setDistanceFromMesh(*pTris,mInfo,p,OctNode::EdgeIndex(nIdx,i,maxDepth), nSurfaceType, color, eOperation, pVisitedValues);
    }

    // set the face mid-points
    for(int i=0;i<Cube::FACES;i++)
    {
      Vec3 p;
      int dir,off;
      Cube::FactorFaceIndex(i,dir,off);
      p=ctr;
      p[dir]+=-w/2+w*off;
      bValuesModified |= setDistanceFromMesh(*pTris,mInfo,p,OctNode::FaceIndex(nIdx,i,maxDepth), nSurfaceType, color, eOperation, pVisitedValues);
    }
  }

  Vec3 vW(w/3,w/3,w/3);

  for(int i=0;i<Cube::CORNERS;i++)
  {
    std::vector<int> myTriangles;

    AABB nodeBox = GetNodeAABB(nIdx.child(i));
    nodeBox.min -= vW;
    nodeBox.max += vW;

    if(Overlap::AABB_AABB(mInfo.meshBox,nodeBox) && trianglesForThisNode.size())
    {
    for(size_t j=0;j<(*pTris).size();j++)
    {
      Vec3 t[3];
      for(int k=0;k<3;k++) 
        t[k]=mInfo.vertices[mInfo.triangles[(*pTris)[j]].idx[k]];

        if(Overlap::AABB_Triangle(nodeBox, t[0], t[1], t[2]))
        myTriangles.push_back((*pTris)[j]);
    }
    }

    assert(node->children);
    if(bValuesModified || myTriangles.size())
    {
    setChildrenFromMesh(nSurfaceType, color, &node->children[i], nIdx.child(i),
      myTriangles, (*pTris), mInfo, maxDepth, setCenter, flatness, pBrushAABB, pValuesBk, eOperation, true, pVisitedValues);
  }
}
}


/*void IsoOctree::interpolateSharedValues(void)
{
SVoxValue values[Cube::CORNERS];
nKey.set(maxDepth);

ISO_KEY key;
int c1,c2,c3,c4;
int idx[3];
for(int d=0;d<maxDepth;d++)
{
OctNode::NodeIndex nIdx;
for(OctNode* temp=tree.nextNode(NULL,nIdx) ; temp ; temp=tree.nextNode(temp,nIdx) )
{
if(nIdx.depth==d && temp->children)
{
for(int c=0;c<Cube::CORNERS;c++)
values[c]=(*m_pValues)[OctNode::CornerIndex(nIdx,c,maxDepth)];

nKey.getNeighbors(temp);
for(int f=0;f<Cube::FACES;f++)
{
Cube::FaceCorners(f,c1,c2,c3,c4);
key=OctNode::FaceIndex(nIdx,f,maxDepth);
NeighborKey::FaceIndex(f,idx);
if(	!nKey.neighbors[d].neighbors[idx[0]][idx[1]][idx[2]] ||
!nKey.neighbors[d].neighbors[idx[0]][idx[1]][idx[2]]->children)
(*m_pValues)[key]=(values[c1]+values[c2]+values[c3]+values[c4]) / 4;
}
for(int e=0;e<Cube::EDGES;e++)
{
Cube::EdgeCorners(e,c1,c2);
key=OctNode::EdgeIndex(nIdx,e,maxDepth);

int f1,f2;
Cube::FacesAdjacentToEdge(e,f1,f2);
NeighborKey::FaceIndex(f1,idx);
if(	!nKey.neighbors[d].neighbors[idx[0]][idx[1]][idx[2]] ||
!nKey.neighbors[d].neighbors[idx[0]][idx[1]][idx[2]]->children)
(*m_pValues)[key]=(values[c1]+values[c2])/2;

NeighborKey::FaceIndex(f2,idx);
if(	!nKey.neighbors[d].neighbors[idx[0]][idx[1]][idx[2]] ||
!nKey.neighbors[d].neighbors[idx[0]][idx[1]][idx[2]]->children)
(*m_pValues)[key]=(values[c1]+values[c2])/2;
NeighborKey::EdgeIndex(e,idx);
if(	!nKey.neighbors[d].neighbors[idx[0]][idx[1]][idx[2]] ||
!nKey.neighbors[d].neighbors[idx[0]][idx[1]][idx[2]]->children)
(*m_pValues)[key]=(values[c1]+values[c2])/2;
}
}
}
}
}*/

Vec3_tpl<ISO_DOUBLE> Average(const Vec3_tpl<ISO_DOUBLE> * pVals, int nCount)
{
  Vec3_tpl<ISO_DOUBLE> fValSumm(0,0,0);
  for(int i=0; i<nCount; i++)
    fValSumm += pVals[i];
  return fValSumm/nCount;
}

Vec3_tpl<ISO_DOUBLE> Average(const Vec3_tpl<ISO_DOUBLE> & v0, const Vec3_tpl<ISO_DOUBLE> & v1, const Vec3_tpl<ISO_DOUBLE> & v2, const Vec3_tpl<ISO_DOUBLE> & v3)
{
  Vec3_tpl<ISO_DOUBLE> result;
  result = (v0 + v1 + v2 + v3)*.25f;
  return result;
}

Vec3_tpl<ISO_DOUBLE> Average(const Vec3_tpl<ISO_DOUBLE> & v0, const Vec3_tpl<ISO_DOUBLE> & v1)
{
  Vec3_tpl<ISO_DOUBLE> result;
  result = (v0 + v1)*.5f;
  return result;
}

void IsoOctree::interpolateChildsNormals(const OctNode::NodeIndex& nIdx, OctNode*temp, TFlatness * pSrcValues, TFlatness * pDstValues)
{
  Vec3_tpl<ISO_DOUBLE> values[Cube::CORNERS];
  nKey.set(maxDepth);

  ISO_KEY key;
  int c1,c2,c3,c4;
  int idx[3];
  int d = nIdx.depth;
  Vec3_tpl<ISO_DOUBLE> voxValAverage;
  for(int c=0;c<Cube::CORNERS;c++)
  {
    // search in prev values
    TFlatness::iterator iter = (*pSrcValues).find(OctNode::CornerIndex(nIdx,c,maxDepth));
    if(iter==(*pSrcValues).end())
    {
      Vec3_tpl<ISO_DOUBLE> vv;
      values[c] = vv;
    }
    else
    {
      values[c] = iter->second.first;
    }
  }

  voxValAverage = Average(values,Cube::CORNERS);

  Vec3_tpl<ISO_DOUBLE> fNewVal;
  nKey.getNeighbors(temp);
  for(int f=0;f<Cube::FACES;f++)
  {
    Cube::FaceCorners(f,c1,c2,c3,c4);
    key=OctNode::FaceIndex(nIdx,f,maxDepth);
    NeighborKey::FaceIndex(f,idx);
    if(	!nKey.neighbors[d].neighbors[idx[0]][idx[1]][idx[2]] ||
      !nKey.neighbors[d].neighbors[idx[0]][idx[1]][idx[2]]->children)
    {
      TFlatness::iterator iter = (*pDstValues).find(key);
      fNewVal = Average(values[c1],values[c2],values[c3],values[c4]);
      if( iter==(*pDstValues).end() )
        (*pDstValues)[key].first = fNewVal;
      else //if( iter->second>fNewVal )
        iter->second.first = fNewVal;
    }
  }

  key=OctNode::CenterIndex(nIdx,maxDepth);
  TFlatness::iterator _iter = (*pDstValues).find(key);
  fNewVal = voxValAverage;
  if( _iter==(*pDstValues).end() )
    (*pDstValues)[key].first = fNewVal;
  else //if( iter->second>fNewVal )
    _iter->second.first = fNewVal;

  for(int e=0;e<Cube::EDGES;e++)
  {
    Cube::EdgeCorners(e,c1,c2);
    key=OctNode::EdgeIndex(nIdx,e,maxDepth);

    int f1,f2;
    Cube::FacesAdjacentToEdge(e,f1,f2);
    NeighborKey::FaceIndex(f1,idx);
    if(	!nKey.neighbors[d].neighbors[idx[0]][idx[1]][idx[2]] ||
      !nKey.neighbors[d].neighbors[idx[0]][idx[1]][idx[2]]->children)
    {
      TFlatness::iterator iter = (*pDstValues).find(key);
      fNewVal = Average(values[c1],values[c2]);
      if( iter==(*pDstValues).end() )
        (*pDstValues)[key].first = fNewVal;
      else //if( iter->second>fNewVal )
        iter->second.first = fNewVal;
    }

    NeighborKey::FaceIndex(f2,idx);
    if(	!nKey.neighbors[d].neighbors[idx[0]][idx[1]][idx[2]] ||
      !nKey.neighbors[d].neighbors[idx[0]][idx[1]][idx[2]]->children)
    {
      TFlatness::iterator iter = (*pDstValues).find(key);
      fNewVal = Average(values[c1],values[c2]);
      if( iter==(*pDstValues).end() )
        (*pDstValues)[key].first = fNewVal;
      else //if( iter->second>fNewVal )
        iter->second.first = fNewVal;
    }

    NeighborKey::EdgeIndex(e,idx);
    if(	!nKey.neighbors[d].neighbors[idx[0]][idx[1]][idx[2]] ||
      !nKey.neighbors[d].neighbors[idx[0]][idx[1]][idx[2]]->children)
    {
      TFlatness::iterator iter = (*pDstValues).find(key);
      fNewVal = Average(values[c1],values[c2]);
      if( iter==(*pDstValues).end() )
        (*pDstValues)[key].first = fNewVal;
      else //if( iter->second>fNewVal )
        iter->second.first = fNewVal;
    }
  }
}

void IsoOctree::interpolateChilds(const OctNode::NodeIndex& nIdx, OctNode*temp, TVoxValues * pSrcValues, TVoxValues * pDstValues)
{
  m_aabbDirty.Add(GetNodeAABB(nIdx));

  SVoxValue values[Cube::CORNERS];
  nKey.set(maxDepth);

  ISO_KEY key;
  int c1,c2,c3,c4;
  int idx[3];
  int d = nIdx.depth;
  SVoxValue voxValAverage;
  for(int c=0;c<Cube::CORNERS;c++)
  {
    // search in prev values
    TVoxValues::iterator iter = (*pSrcValues).find(OctNode::CornerIndex(nIdx,c,maxDepth));
    if(iter==(*pSrcValues).end())
    {
      SVoxValue vv;
      values[c] = vv;
      assert(!"Parent values are not ready");
    }
    else
      values[c] = iter->second;
  }

  voxValAverage = SVoxValue::Average(values,Cube::CORNERS);

  SVoxValue fNewVal;
  nKey.getNeighbors(temp);
  for(int f=0;f<Cube::FACES;f++)
  {
    Cube::FaceCorners(f,c1,c2,c3,c4);
    key=OctNode::FaceIndex(nIdx,f,maxDepth);
    NeighborKey::FaceIndex(f,idx);
    if(	!nKey.neighbors[d].neighbors[idx[0]][idx[1]][idx[2]] ||
        !nKey.neighbors[d].neighbors[idx[0]][idx[1]][idx[2]]->children)
    {
      TVoxValues::iterator iter = (*pDstValues).find(key);
      fNewVal = SVoxValue::Average(values[c1],values[c2],values[c3],values[c4]);
      if( iter==(*pDstValues).end() )
        (*pDstValues)[key] = fNewVal;
      else //if( iter->second>fNewVal )
        iter->second = fNewVal;
    }
  }

  key=OctNode::CenterIndex(nIdx,maxDepth);
  TVoxValues::iterator _iter = (*pDstValues).find(key);
  fNewVal = voxValAverage;
  if( _iter==(*pDstValues).end() )
    (*pDstValues)[key] = fNewVal;
  else //if( iter->second>fNewVal )
    _iter->second = fNewVal;

  for(int e=0;e<Cube::EDGES;e++)
  {
    Cube::EdgeCorners(e,c1,c2);
    key=OctNode::EdgeIndex(nIdx,e,maxDepth);

    int f1,f2;
    Cube::FacesAdjacentToEdge(e,f1,f2);
    NeighborKey::FaceIndex(f1,idx);
    if(	!nKey.neighbors[d].neighbors[idx[0]][idx[1]][idx[2]] ||
        !nKey.neighbors[d].neighbors[idx[0]][idx[1]][idx[2]]->children)
    {
      TVoxValues::iterator iter = (*pDstValues).find(key);
      fNewVal = SVoxValue::Average(values[c1],values[c2]);
      if( iter==(*pDstValues).end() )
        (*pDstValues)[key] = fNewVal;
      else //if( iter->second>fNewVal )
        iter->second = fNewVal;
    }

    NeighborKey::FaceIndex(f2,idx);
    if(	!nKey.neighbors[d].neighbors[idx[0]][idx[1]][idx[2]] ||
        !nKey.neighbors[d].neighbors[idx[0]][idx[1]][idx[2]]->children)
    {
      TVoxValues::iterator iter = (*pDstValues).find(key);
      fNewVal = SVoxValue::Average(values[c1],values[c2]);
      if( iter==(*pDstValues).end() )
        (*pDstValues)[key] = fNewVal;
      else //if( iter->second>fNewVal )
        iter->second = fNewVal;
    }

    NeighborKey::EdgeIndex(e,idx);
    if(	!nKey.neighbors[d].neighbors[idx[0]][idx[1]][idx[2]] ||
      !nKey.neighbors[d].neighbors[idx[0]][idx[1]][idx[2]]->children)
    {
      TVoxValues::iterator iter = (*pDstValues).find(key);
      fNewVal = SVoxValue::Average(values[c1],values[c2]);
      if( iter==(*pDstValues).end() )
        (*pDstValues)[key] = fNewVal;
      else //if( iter->second>fNewVal )
        iter->second = fNewVal;
    }
  }
}

void IsoOctree::CleanUpValues()
{
  FUNCTION_PROFILER_3DENGINE;

  PrintMessage("Cleanup voxel values ...");

  TVoxValues tempValues;
  //  TFlatness tempNormals;

  nKey.set(maxDepth);
  OctNode::NodeIndex nIdx;

  for(OctNode* temp=tree.nextLeaf(NULL,nIdx) ; temp ; temp=tree.nextLeaf(temp,nIdx) )
  {
    for(int i=0;i<Cube::CORNERS;i++)
    {
      ISO_KEY key=OctNode::CornerIndex(nIdx,i,maxDepth);

      {
        TVoxValues::iterator iter = (*m_pValues).find(key);
        if(iter != (*m_pValues).end())
          tempValues[key] = iter->second;
      }

      /*      {
      TFlatness::iterator iter = (m_flatnessMap).find(key);
      if(iter != (m_flatnessMap).end())
      tempNormals[key] = iter->second;
      }*/
    }
  }

  int nBefore = (*m_pValues).size();
  //  int nBeforeNorm = (m_flatnessMap).size();

  (*m_pValues) = tempValues;
  //  m_flatnessMap = tempNormals;

  PrintMessagePlus(" (%d K values removed)", (nBefore -      (*m_pValues).size())/1024);
  //  PrintMessagePlus(", %d K normals removed", (nBeforeNorm - m_flatnessMap.size())/1024);

  Get3DEngine()->m_pVoxTerrain->m_bModified = false;
}
/*{
  FUNCTION_PROFILER_3DENGINE;

  PrintMessage("Cleanup voxel values ...");

  for(TVoxValues::iterator iter=(*m_pValues).begin();iter!=(*m_pValues).end();iter++)
    iter->second.color[3]=0;

  nKey.set(maxDepth);
  OctNode::NodeIndex nIdx;

  for(OctNode* temp=tree.nextLeaf(NULL,nIdx) ; temp ; temp=tree.nextLeaf(temp,nIdx) )
  {
    for(int i=0;i<Cube::CORNERS;i++)
    {
      ISO_KEY key=OctNode::CornerIndex(nIdx,i,maxDepth);
        TVoxValues::iterator iter = (*m_pValues).find(key);
        if(iter != (*m_pValues).end())
        iter->second.color[3]=255;
      }
    }

  int nBefore = (*m_pValues).size();

  TVoxValues::iterator next;
  for(TVoxValues::iterator iter=(*m_pValues).begin(); iter!=(*m_pValues).end(); iter=next)
  {
    next = iter;
    next++;

    if(iter->second.color[3]!=255)
      m_pValues->erase(iter);
  }

  PrintMessagePlus(" (%d K values removed)", (nBefore -      (*m_pValues).size())/1024);

  Get3DEngine()->m_pVoxTerrain->m_bModified = false;
}*/

//#define VOX_EXPORT_OCTREE_AND_NORMALS 1

int IsoOctree::GetCompiledData(byte * & pData, int & nDataSize, EEndian eEndian, bool bSaveMesh, bool bSaveForEditing, AABB * pAreaBox)
{
  FUNCTION_PROFILER_3DENGINE;

  int nTreeSuccess = 0;
  if(bSaveForEditing)
  {
#ifdef VOX_EXPORT_OCTREE_AND_NORMALS
      nTreeSuccess = tree.GetCompiledData(pData, nDataSize, eEndian);
#else
      OctNodeChunk chunk;
      ZeroStruct(chunk);
      AddToPtr(pData,nDataSize,&chunk,1,eEndian,true);
      nTreeSuccess = 1;
#endif
  }
  else
  {
    nTreeSuccess = renderTree.GetCompiledData(pData, nDataSize, eEndian);
  }

  if(nTreeSuccess)
  {
    AddToPtr(pData,nDataSize,&maxDepth,1,eEndian,true);

    // save values

    if(bSaveForEditing)
    {
      {
        TVoxValues * pValues = pAreaBox ? &m_tmpValues : m_pValues;

        int nValuesNum=int((*pValues).size());
        AddToPtr(pData,nDataSize,&nValuesNum,1,eEndian,true);

        for(TVoxValues::const_iterator iter=(*pValues).begin();iter!=(*pValues).end();iter++)
        {
          AddToPtr(pData,nDataSize,&iter->first,1,eEndian,true);
          AddToPtr(pData,nDataSize,&iter->second,1,eEndian,true);
        }
      }

#ifdef VOX_EXPORT_OCTREE_AND_NORMALS
      {
        int nValuesNum=int((m_flatnessMap).size());
        AddToPtr(pData,nDataSize,&nValuesNum,1,eEndian,true);

        for(TFlatness::const_iterator iter=(m_flatnessMap).begin();iter!=(m_flatnessMap).end();iter++)
        {
          AddToPtr(pData,nDataSize,&iter->first,1,eEndian,true);
          AddToPtr(pData,nDataSize,&iter->second,1,eEndian,true);
        }
      }
#endif
    }
    else
    {
      int nValuesNum=0;
      AddToPtr(pData,nDataSize,&nValuesNum,1,eEndian,true);
#ifdef VOX_EXPORT_OCTREE_AND_NORMALS
        AddToPtr(pData,nDataSize,&nValuesNum,1,eEndian,true);
#endif
    }

    // save meshes

    if(bSaveMesh)
    {
      int nMeshesNum = threadMeshes.size();

      AddToPtr(pData,nDataSize,&nMeshesNum,1,eEndian,true);

      int nSectorId=0;
      for(TIsoMeshes::const_iterator iter=threadMeshes.begin();iter!=threadMeshes.end();iter++)
      {
        AddToPtr(pData,nDataSize,(ISO_KEY*)&iter->first,1,eEndian,true);

        int nSizeOut = 0;
        SIsoMesh * pIsoMesh = iter->second;
        byte * pIsoMeshData = pIsoMesh->GetCompiledMeshData(nSizeOut, eEndian, bSaveForEditing, nSectorId++);

        AddToPtr(pData,nDataSize,pIsoMeshData,nSizeOut,eEndian,true);
      }
    }
    else
    {
      int nMeshesNum=0;
      AddToPtr(pData,nDataSize,&nMeshesNum,1,eEndian,true);
    }

    // save normals

    if(GetCVars()->e_VoxTerDebug)
    {
      int nValuesNum = int((m_flatnessMap).size());
      AddToPtr(pData,nDataSize,&nValuesNum,1,eEndian,true);

      for(TFlatness::const_iterator iter=(m_flatnessMap).begin();iter!=(m_flatnessMap).end();iter++)
      {
        AddToPtr(pData,nDataSize,&iter->first,1,eEndian,true);
        AddToPtr(pData,nDataSize,&iter->second,1,eEndian,true);
      }
    }
    else
    {
      int nValuesNum=0;
      AddToPtr(pData,nDataSize,&nValuesNum,1,eEndian,true);
    }

    // save layers

    if(bSaveMesh)
    {
      int nLayersNum = Get3DEngine()->m_arrBaseTextureData.Count();
      AddToPtr(pData,nDataSize,&nLayersNum,1,eEndian,true);

      for(int nLayerId=0; nLayerId<Get3DEngine()->m_arrBaseTextureData.Count(); nLayerId++)
      {
        if(SImageInfo * pImgInfo = &Get3DEngine()->m_arrBaseTextureData[nLayerId])
        {
          SaveLayerImageInfo(pData, nDataSize, eEndian, pImgInfo);
        }
      }
    }
    else
    {
      int nLayersNum = 0;
      AddToPtr(pData,nDataSize,&nLayersNum,1,eEndian,true);
    }

    // save empty keys

    if(bSaveMesh)
    {
      AUTO_LOCK(g_cIsoMeshes);

      int nValuesNum = threadEmptyMeshes.size();

      AddToPtr(pData,nDataSize,&nValuesNum,1,eEndian,true);

      for(TIsoMeshes::const_iterator iter=threadEmptyMeshes.begin();iter!=threadEmptyMeshes.end();iter++)
      {
        AddToPtr(pData,nDataSize,&iter->first,1,eEndian,true);
      }
    }
    else
    {
      int nValuesNum = 0;
      AddToPtr(pData,nDataSize,&nValuesNum,1,eEndian,true);
    }
  }

  assert(!nDataSize);

  return !nDataSize;
}

void IsoOctree::GetSubImageData(byte * & pData, int & nDataSize, EEndian eEndian, SImageSubInfo * pImgSubInfo, const char * pName, bool bSaveImgData)
{
  PrintMessage("Serializing image data (%dx%d) %s", pImgSubInfo->nDim, pImgSubInfo->nDim, pName);

  SImageSubInfo tmpImgSubInfo = *pImgSubInfo;
//  if(!bSaveImgData)
  memset(tmpImgSubInfo.nDummy,0,sizeof(tmpImgSubInfo.nDummy));
  AddToPtr(pData,nDataSize,&tmpImgSubInfo,1,eEndian);
/*
  if(bSaveImgData && pImgSubInfo->pImgMips[0])
  {
    int nMipDim = pImgSubInfo->nDim;
    for(int nMip=0; nMip<SImageSubInfo::nMipsNum && pImgSubInfo->pImgMips[nMip]; nMip++)
    {
      if(CMemoryBlock * pCompressed = CMemoryBlock::CompressToMemBlock(pImgSubInfo->pImgMips[nMip], nMipDim*nMipDim*4, GetSystem()))
      {
        int nComprSize = pCompressed->GetSize();
        AddToPtr(pData,nDataSize,nComprSize,eEndian);
        AddToPtr(pData,nDataSize,(byte*)pCompressed->GetData(),nComprSize,eEndian,true);
        pCompressed->Release();
      }
      else
      {
        int nComprSize = 0;
        AddToPtr(pData,nDataSize,nComprSize,eEndian);
      }

      nMipDim/=2;
    }
  }
*/
  PrintMessagePlus(" ok");
}

void IsoOctree::SaveLayerImageInfo(byte * & pData, int & nDataSize, EEndian eEndian, SImageInfo * pImgInfo)
{
  SImageInfo tmpImgSubInfo = *pImgInfo;
  memset(tmpImgSubInfo.baseInfo.nDummy,0,sizeof(tmpImgSubInfo.baseInfo.nDummy));
  memset(tmpImgSubInfo.detailInfo.nDummy,0,sizeof(tmpImgSubInfo.detailInfo.nDummy));
  ZeroStruct(tmpImgSubInfo.arrTextureId);

  // save all params and pointers
  AddToPtr(pData,nDataSize,&tmpImgSubInfo,1,eEndian,true);

  // save base texture data
  //  GetSubImageData(pData, nDataSize, eEndian, &pImgInfo->baseInfo, pImgInfo->szBaseTexName, bSaveImgData);

  // save detail texture data
  //GetSubImageData(pData, nDataSize, eEndian, &pImgInfo->detailInfo, pImgInfo->szMatName, bSaveImgData);
}

int IsoOctree::GetSubImageDataSize(SImageSubInfo & subInfo, bool bSaveImgData) const
{
  int nDataSize = 0;

  // dim
  nDataSize += sizeof(SImageSubInfo);

 /* if(bSaveImgData && subInfo.pImgMips[0])
  {
    int nMipDim = subInfo.nDim;
    for(int nMip=0; nMip<SImageSubInfo::nMipsNum && subInfo.pImgMips[nMip]; nMip++)
    {
      if(CMemoryBlock * pCompressed = CMemoryBlock::CompressToMemBlock(subInfo.pImgMips[nMip], nMipDim*nMipDim*4, GetSystem()))
      {
        int nComprSize = pCompressed->GetSize();
        pCompressed->Release();

        nDataSize += 4;
        nDataSize += nComprSize;
        while((int)nDataSize&3)
          nDataSize++;
      }
      else
        nDataSize += 4;

      nMipDim/=2;
    }
  }*/

  return nDataSize;
}

int IsoOctree::GetCompiledDataSize(bool bSaveMesh, bool bSaveForEditing, AABB * pAreaBox)
{
  FUNCTION_PROFILER_3DENGINE;

  int nDataSize = 0;

//  EEndian eEndian = GetPlatformEndian();

  if(bSaveForEditing)
  {
#ifdef VOX_EXPORT_OCTREE_AND_NORMALS
    nDataSize += tree.GetCompiledDataSize();
#else
    nDataSize += sizeof(OctNodeChunk);
#endif
  }
  else
    nDataSize += renderTree.GetCompiledDataSize();

  nDataSize += sizeof(maxDepth);

  // save values count
  nDataSize += sizeof(int);

  // save normals count
#ifdef VOX_EXPORT_OCTREE_AND_NORMALS
    nDataSize += sizeof(int);
#endif

  if(bSaveForEditing)
  {
    if(bSaveMesh && Get3DEngine()->m_pVoxTerrain->m_bModified)
      CleanUpValues();

    if(!pAreaBox)
    {
      // all
      nDataSize += m_pValues->size() * (sizeof(ISO_KEY) + sizeof(SVoxValue));

      if(GetCVars()->e_VoxTerDebug)
        nDataSize += m_flatnessMap.size() * (sizeof(ISO_KEY) + sizeof(SFlatnessItem));
    }
    else
    {
      FRAME_PROFILER( "IsoOctree::GetCompiledDataSize_CollectValues", GetSystem(), PROFILE_3DENGINE );

      OctNode::NodeIndex nIdxGlobal;
      m_tmpValues.clear();
      AABB areaBoxNorm = *pAreaBox;
      areaBoxNorm.min /= CVoxTerrain::m_fMapSize;
      areaBoxNorm.max /= CVoxTerrain::m_fMapSize;
      CollectValues(&tree, nIdxGlobal, m_tmpValues, &areaBoxNorm);
      nDataSize += m_tmpValues.size() * (sizeof(ISO_KEY) + sizeof(SVoxValue));

      if(GetCVars()->e_VoxTerDebug)
        nDataSize += m_flatnessMap.size() * (sizeof(ISO_KEY) + sizeof(SFlatnessItem));
    }
  }

  nDataSize += sizeof(int);

  // save meshes
  if(bSaveMesh)
  {
    if((bSaveForEditing && GetCVars()->e_VoxTerTexBuildOnCPU) || !GetCVars()->e_VoxTerHeightmapEditing)
      CleanUpMeshes(bSaveForEditing && GetCVars()->e_VoxTerTexBuildOnCPU);

    int nSectorId=0;
    for(TIsoMeshes::const_iterator iter=threadMeshes.begin();iter!=threadMeshes.end();iter++)
    {
      nDataSize += sizeof(ISO_KEY);
      nDataSize += iter->second->GetCompiledMeshDataSize(bSaveForEditing,nSectorId++);
    }
  }

  // save texture database data
  nDataSize += sizeof(int);

/*  if(bSaveMesh && 0)
  {
    Get3DEngine()->CheckUpdateImageInfos();

    for(std::map<string,SImageSubInfo*>::const_iterator iter=Get3DEngine()->m_imageInfos.begin(); iter!=Get3DEngine()->m_imageInfos.end(); iter++)
    {
      // save file name
      const string & strFileName = iter->first;
      int nLen = strFileName.size();
      nDataSize += sizeof(int);
      const char * pStr = strFileName.c_str();
      nDataSize += nLen;
      while((int)nDataSize&3)
        nDataSize++;

      // save image data
      SImageSubInfo * pImageSubInfo = iter->second;
      nDataSize += GetSubImageDataSize(*pImageSubInfo, true);
    }
  }*/

  // save layers
  nDataSize += sizeof(int);

  if(bSaveMesh)
  {
    // save layers
    for(int nLayerId=0; nLayerId<Get3DEngine()->m_arrBaseTextureData.Count(); nLayerId++)
    {
      if(SImageInfo * pImgInfo = &Get3DEngine()->m_arrBaseTextureData[nLayerId])
      {
        nDataSize += sizeof(*pImgInfo);
      }
    }
  }

  // save empty keys
  nDataSize += sizeof(int);

  if(bSaveMesh)
  {
    AUTO_LOCK(g_cIsoMeshes);

    for(TIsoMeshes::const_iterator iter=threadEmptyMeshes.begin();iter!=threadEmptyMeshes.end();iter++)
    {
      nDataSize += sizeof(ISO_KEY);
    }
  }

  return nDataSize;
}

int IsoOctree::GetEngineMemoryUsageMB()
{
  static IMemoryManager::SProcessMemInfo processMemInfo;
  if(GetISystem()->GetIMemoryManager())
    GetISystem()->GetIMemoryManager()->GetProcessMemInfo(processMemInfo);
  int nVirtMemMB = (int)(processMemInfo.PeakPagefileUsage/1024/1024);
  return nVirtMemMB;
}

int IsoOctree::getRootPosition(const OctNode* node,
                               const OctNode::NodeIndex& nIdx,
                               const int& eIndex,Vec3& position,SSurfTypeInfo& surf_type, ColorB & color)
{
  int c0,c1;
  Cube::EdgeCorners(eIndex,c0,c1);

  if(!MarchingCubes::HasEdgeRoots(node->nodeData.mcIndex,eIndex))
    return 0;

  float w;
  Vec3 p1,p2;
  int x,y,z;

  OctNode::CenterAndWidth(nIdx,p1,w);
  OctNode::CenterAndWidth(nIdx,p2,w);

  Cube::FactorCornerIndex(c0,x,y,z);
  p1[0]+=-w/2+w*x;
  p1[1]+=-w/2+w*y;
  p1[2]+=-w/2+w*z;
  Cube::FactorCornerIndex(c1,x,y,z);
  p2[0]+=-w/2+w*x;
  p2[1]+=-w/2+w*y;
  p2[2]+=-w/2+w*z;
  position=_RootPosition(p1,p2,
    (*m_pValues)[OctNode::CornerIndex(nIdx,c0,maxDepth)],
    (*m_pValues)[OctNode::CornerIndex(nIdx,c1,maxDepth)],
    surf_type, color);
  return 1;
}

ISO_KEY IsoOctree::getRootKey(const OctNode::NodeIndex& nIdx,const int& edgeIndex)
{
  int offset,eIndex[2],o,i1,i2;
  Cube::FactorEdgeIndex(edgeIndex,o,i1,i2);
  offset=BinaryNode::Index(nIdx.depth,nIdx.offset[o]);
  switch(o)
  {
  case 0:
    eIndex[0]=BinaryNode::CornerIndex(maxDepth+1,nIdx.depth,nIdx.offset[1],i1);
    eIndex[1]=BinaryNode::CornerIndex(maxDepth+1,nIdx.depth,nIdx.offset[2],i2);
    break;
  case 1:
    eIndex[0]=BinaryNode::CornerIndex(maxDepth+1,nIdx.depth,nIdx.offset[0],i1);
    eIndex[1]=BinaryNode::CornerIndex(maxDepth+1,nIdx.depth,nIdx.offset[2],i2);
    break;
  case 2:
    eIndex[0]=BinaryNode::CornerIndex(maxDepth+1,nIdx.depth,nIdx.offset[0],i1);
    eIndex[1]=BinaryNode::CornerIndex(maxDepth+1,nIdx.depth,nIdx.offset[1],i2);
    break;
  }

  return (ISO_KEY)(o) | (ISO_KEY)(eIndex[0])<<5 | (ISO_KEY)(eIndex[1])<<(nBitShift1+5) | (ISO_KEY)(offset)<<(nBitShift2+5);
}

int IsoOctree::getRootIndex(OctNode* node,
                            const OctNode::NodeIndex& nIdx,
                            const int& edgeIndex,RootInfo& ri){
                              int c1,c2,f1,f2;
                              const OctNode *temp,*finest;
                              int finestIndex;
                              OctNode::NodeIndex finestNIdx=nIdx;

                              // The assumption is that the super-edge has a root along it. 
                              if(!(MarchingCubes::HasEdgeRoots(node->nodeData.mcIndex,edgeIndex)))
                                return 0;
#if USE_MAX_DEPTH_SPEED_UP
                              if(nIdx.depth==maxDepth)
                              {
                                ri.node=node;
                                ri.edgeIndex=edgeIndex;
                                ri.nIdx=nIdx;
                                ri.key=getRootKey(nIdx,edgeIndex);
                                return 1;
                              }
#endif // USE_MAX_DEPTH_SPEED_UP


                              finest=node;
                              finestIndex=edgeIndex;

                              Cube::FacesAdjacentToEdge(edgeIndex,f1,f2);
                              if(nIdx.depth<maxDepth)
                              {
                                if(!node->children)
                                {
                                  temp=node->faceNeighbor(f1);
                                  if(temp && temp->children)
                                  {
                                    finest=temp;
                                    finestIndex=Cube::FaceReflectEdgeIndex(edgeIndex,f1);
                                    int dir,off;
                                    Cube::FactorFaceIndex(f1,dir,off);
                                    if(off)
                                      finestNIdx.offset[dir]++;
                                    else
                                      finestNIdx.offset[dir]--;
                                  }
                                  else
                                  {
                                    temp=node->faceNeighbor(f2);
                                    if(temp && temp->children)
                                    {
                                      finest=temp;
                                      finestIndex=Cube::FaceReflectEdgeIndex(edgeIndex,f2);
                                      int dir,off;
                                      Cube::FactorFaceIndex(f2,dir,off);
                                      if(off)
                                        finestNIdx.offset[dir]++;
                                      else
                                        finestNIdx.offset[dir]--;
                                    }
                                    else
                                    {
                                      temp=node->edgeNeighbor(edgeIndex);
                                      if(temp && temp->children)
                                      {
                                        finest=temp;
                                        finestIndex=Cube::EdgeReflectEdgeIndex(edgeIndex);
                                        int o,i1,i2;
                                        Cube::FactorEdgeIndex(edgeIndex,o,i1,i2);
                                        switch(o)
                                        {
                                        case 0:
                                          if(i1)	finestNIdx.offset[1]++;
                                          else	finestNIdx.offset[1]--;
                                          if(i2)	finestNIdx.offset[2]++;
                                          else	finestNIdx.offset[2]--;
                                          break;
                                        case 1:
                                          if(i1)	finestNIdx.offset[0]++;
                                          else	finestNIdx.offset[0]--;
                                          if(i2)	finestNIdx.offset[2]++;
                                          else	finestNIdx.offset[2]--;
                                          break;
                                        case 2:
                                          if(i1)	finestNIdx.offset[0]++;
                                          else	finestNIdx.offset[0]--;
                                          if(i2)	finestNIdx.offset[1]++;
                                          else	finestNIdx.offset[1]--;
                                          break;
                                        }
                                      }
                                    }
                                  }
                                }
                              }

                              Cube::EdgeCorners(finestIndex,c1,c2);
                              if(finest->children)
                              {
                                if		(getRootIndex(&finest->children[c1],finestNIdx.child(c1),finestIndex,ri))	{return 1;}
                                else if	(getRootIndex(&finest->children[c2],finestNIdx.child(c2),finestIndex,ri))	{return 1;}
                                else																				{
                                  printf("IsoOctree: Failed to find root index");
                                  return 0;
                                }
                              }
                              else
                              {
                                ri.nIdx=finestNIdx;
                                ri.node=finest;
                                ri.edgeIndex=finestIndex;

                                int o,i1,i2;
                                Cube::FactorEdgeIndex(finestIndex,o,i1,i2);
                                int offset,eIndex[2];
                                offset=BinaryNode::Index(finestNIdx.depth,finestNIdx.offset[o]);
                                switch(o)
                                {
                                case 0:
                                  eIndex[0]=BinaryNode::CornerIndex(maxDepth+1,finestNIdx.depth,finestNIdx.offset[1],i1);
                                  eIndex[1]=BinaryNode::CornerIndex(maxDepth+1,finestNIdx.depth,finestNIdx.offset[2],i2);
                                  break;
                                case 1:
                                  eIndex[0]=BinaryNode::CornerIndex(maxDepth+1,finestNIdx.depth,finestNIdx.offset[0],i1);
                                  eIndex[1]=BinaryNode::CornerIndex(maxDepth+1,finestNIdx.depth,finestNIdx.offset[2],i2);
                                  break;
                                case 2:
                                  eIndex[0]=BinaryNode::CornerIndex(maxDepth+1,finestNIdx.depth,finestNIdx.offset[0],i1);
                                  eIndex[1]=BinaryNode::CornerIndex(maxDepth+1,finestNIdx.depth,finestNIdx.offset[1],i2);
                                  break;
                                }

                                ri.key= (ISO_KEY)(o) | (ISO_KEY)(eIndex[0])<<5 | (ISO_KEY)(eIndex[1])<<(nBitShift1+5) | (ISO_KEY)(offset)<<(nBitShift2+5);
                                return 1;
                              }
}


void IsoOctree::getRoots(OctNode* node,
                         const OctNode::NodeIndex& nIdx,                         
                         TRoots& roots,std::vector<Vec3>& vertices,std::vector<SSurfTypeInfo> * surf_types, std::vector<ColorB> * colors )
{
  Vec3 position;

  int eIndex;
  RootInfo ri;

  if(!MarchingCubes::HasRoots(node->nodeData.mcIndex))
    return;

  for(eIndex=0;eIndex<Cube::EDGES;eIndex++)
  {
    if(!(MarchingCubes::HasEdgeRoots(node->nodeData.mcIndex,eIndex)))
      continue;

    if(getRootIndex(node,nIdx,eIndex,ri))
    {
      if(roots.find(ri.key)==roots.end())
      {
        SSurfTypeInfo suref_type;
        ColorB color;
        getRootPosition(ri.node,ri.nIdx,ri.edgeIndex,position,suref_type,color);
        vertices.push_back(position);
        if(surf_types)
          surf_types->push_back(suref_type);
        if(colors)
          colors->push_back(color);
        roots[ri.key]=int(vertices.size())-1;
      }
    }
    else
      printf("IsoOctree: Failed to get root index");
  }
}

int IsoOctree::getRootPair(const RootInfo& ri,const int& maxDepth,RootInfo& pair)
{
  const OctNode* node=ri.node;
  OctNode::NodeIndex nIdx=ri.nIdx;
  int c1,c2,c;
  Cube::EdgeCorners(ri.edgeIndex,c1,c2);
  while(node->parent)
  {
    c=int(node-node->parent->children);
    if(c!=c1 && c!=c2)
      return 0;
    if(!MarchingCubes::HasEdgeRoots(node->parent->nodeData.mcIndex,ri.edgeIndex))
    {
      if(c==c1)
        return getRootIndex(&node->parent->children[c2],nIdx.parent().child(c2),ri.edgeIndex,pair);
      else
        return getRootIndex(&node->parent->children[c1],nIdx.parent().child(c2),ri.edgeIndex,pair);
    }
    node=node->parent;
    --nIdx;
  }
  return 0;
}

void IsoOctree::getIsoFaceEdges(OctNode* node,
                                const OctNode::NodeIndex& nIdx,
                                const int& faceIndex,std::vector<std::pair<RootInfo,RootInfo> > & edges,const int& flip,const int& useFull)
{
  int c1,c2,c3,c4;
  if(node->children)
  {
    Cube::FaceCorners(faceIndex,c1,c2,c3,c4);
    getIsoFaceEdges(&node->children[c1],nIdx.child(c1),faceIndex,edges,flip,useFull);
    getIsoFaceEdges(&node->children[c2],nIdx.child(c2),faceIndex,edges,flip,useFull);
    getIsoFaceEdges(&node->children[c3],nIdx.child(c3),faceIndex,edges,flip,useFull);
    getIsoFaceEdges(&node->children[c4],nIdx.child(c4),faceIndex,edges,flip,useFull);
  }
  else
  {
    int idx=node->nodeData.mcIndex;

    RootInfo ri1,ri2;
    const std::vector<std::vector<int> > & table=MarchingCubes::caseTable(idx,useFull);
    for(size_t i=0;i<table.size();i++)
    {
      size_t pSize=table[i].size();
      for(size_t j=0;j<pSize;j++)
      {
        if(faceIndex==Cube::FaceAdjacentToEdges(table[i][j],table[i][(j+1)%pSize]))
          if(getRootIndex(node,nIdx,table[i][j],ri1) && getRootIndex(node,nIdx,table[i][(j+1)%pSize],ri2))
            if(flip)
              edges.push_back(std::pair<RootInfo,RootInfo>(ri2,ri1));
            else
              edges.push_back(std::pair<RootInfo,RootInfo>(ri1,ri2));
          else
            printf("IsoOctree: Bad Edge 1: %lld %lld",ri1.key,ri2.key);
        if(edges.size()>nMaxEdgesNum)
          return;
      }
    }
  }
}

void IsoOctree::getIsoPolygons(OctNode* node,
                               const OctNode::NodeIndex& nIdx,
                               TRoots& roots,
                               std::vector<std::vector<int> > & polygons,
                               const int& useFull)
{
  std::vector<std::pair<ISO_KEY,ISO_KEY> > edges;
  std::map<ISO_KEY,std::pair<RootInfo,int> >::iterator iter;
  std::map<ISO_KEY,std::pair<RootInfo,int> > vertexCount;
  std::vector<std::pair<RootInfo,RootInfo> > riEdges;

#if USE_MAX_DEPTH_SPEED_UP
  if(nIdx.depth==maxDepth) // Just run the standard marching cubes...
  {
    RootInfo ri;
    size_t pIndex=polygons.size();
    int idx=node->nodeData.mcIndex;
    const std::vector<std::vector<int> > & table=MarchingCubes::caseTable(idx,useFull);

    polygons.resize(pIndex+table.size());
    for(size_t i=0;i<table.size();i++)
    {
      polygons[pIndex+i].resize(table[i].size());
      for(size_t j=0;j<table[i].size();j++)
        if(getRootIndex(node,nIdx,table[i][j],ri))
          polygons[pIndex+i][j]=roots[ri.key];
        else
          printf("IsoOctree: Bad Edge 1: %lld",ri.key);
    }
    return;
  }
  else
  {
    int x[3];
    nKey.getNeighbors(node);
    x[0]=x[1]=x[2]=1;
    for(int i=0;i<3;i++)
    {
      for(int j=0;j<2;j++)
      {
        x[i]=j<<1;
        if(!nKey.neighbors[nIdx.depth].neighbors[x[0]][x[1]][x[2]] || !nKey.neighbors[nIdx.depth].neighbors[x[0]][x[1]][x[2]]->children)
          getIsoFaceEdges(node,nIdx,Cube::FaceIndex(i,j),riEdges,0,useFull);
        else
        {
          OctNode::NodeIndex idx=nIdx;
          if(j)	idx.offset[i]++;
          else	idx.offset[i]--;
          getIsoFaceEdges(nKey.neighbors[idx.depth].neighbors[x[0]][x[1]][x[2]],idx,Cube::FaceIndex(i,j^1),riEdges,1,useFull);
        }
      }
      x[i]=1;
    }
  }
  for(size_t i=0;i<riEdges.size();i++)
  {
    edges.push_back(std::pair<ISO_KEY,ISO_KEY>(riEdges[i].first.key,riEdges[i].second.key));
    if(edges.size()>nMaxEdgesNum)
      return;

    iter=vertexCount.find(riEdges[i].first.key);
    if(iter==vertexCount.end())
    {
      vertexCount[riEdges[i].first.key].first=riEdges[i].first;
      vertexCount[riEdges[i].first.key].second=0;
    }
    iter=vertexCount.find(riEdges[i].second.key);
    if(iter==vertexCount.end())
    {
      vertexCount[riEdges[i].second.key].first=riEdges[i].second;
      vertexCount[riEdges[i].second.key].second=0;
    }
    vertexCount[riEdges[i].first.key ].second++;
    vertexCount[riEdges[i].second.key].second--;
  }
#else // !USE_MAX_DEPTH_SPEED_UP
  for(fIndex=0;fIndex<Cube::FACES;fIndex++)
  {
    ref=Cube::FaceReflectFaceIndex(fIndex,fIndex);
    temp=node->faceNeighbor(fIndex);

    riEdges.clear();
    if(temp && temp->children)
      getIsoFaceEdges(temp,ref,riEdges,1);
    else
      getIsoFaceEdges(node,fIndex,riEdges,0);
    for(int i=0;i<riEdges.size();i++)
    {
      edges.push_back(std::pair<ISO_KEY,ISO_KEY>(riEdges[i].first.key,riEdges[i].second.key));
      if(edges.size()>nMaxEdgesNum)
        return;

      iter=vertexCount.find(riEdges[i].first.key);
      if(iter==vertexCount.end())
      {
        vertexCount[riEdges[i].first.key].first=riEdges[i].first;
        vertexCount[riEdges[i].first.key].second=0;
      }
      iter=vertexCount.find(riEdges[i].second.key);
      if(iter==vertexCount.end())
      {
        vertexCount[riEdges[i].second.key].first=riEdges[i].second;
        vertexCount[riEdges[i].second.key].second=0;
      }
      vertexCount[riEdges[i].first.key ].second++;
      vertexCount[riEdges[i].second.key].second--;
    }
  }
#endif // USE_MAX_DEPTH_SPEED_UP

  if(edges.size()>nMaxEdgesNum)
    return;

  for(int i=0;i<int(edges.size());i++)
  {
    iter=vertexCount.find(edges[i].first);
    if(iter==vertexCount.end())
      printf("IsoOctree: Could not find vertex: %lld",edges[i].first);
    else if(vertexCount[edges[i].first].second)
    {
      RootInfo ri;
      if(!getRootPair(vertexCount[edges[i].first].first,maxDepth,ri))
        printf("IsoOctree: Failed to get root pair 1: %lld %d",edges[i].first,vertexCount[edges[i].first].second);
      iter=vertexCount.find(ri.key);
      if(iter==vertexCount.end())
        printf("IsoOctree: Vertex pair not in list");
      else
      {
        edges.push_back(std::pair<ISO_KEY,ISO_KEY>(ri.key,edges[i].first));
        vertexCount[ri.key].second++;
        vertexCount[edges[i].first].second--;
        if(edges.size()>nMaxEdgesNum)
          return;
      }
    }

    iter=vertexCount.find(edges[i].second);
    if(iter==vertexCount.end())
      printf("IsoOctree: Could not find vertex: %lld",edges[i].second);
    else if(vertexCount[edges[i].second].second)
    {
      RootInfo ri;
      if(!getRootPair(vertexCount[edges[i].second].first,maxDepth,ri))
        printf("IsoOctree: Failed to get root pair 2: %lld %d",edges[i].second,vertexCount[edges[i].second].second);
      iter=vertexCount.find(ri.key);
      if(iter==vertexCount.end())
        printf("IsoOctree: Vertex pair not in list");
      else{
        edges.push_back(std::pair<ISO_KEY,ISO_KEY>(edges[i].second,ri.key));
        vertexCount[edges[i].second].second++;
        vertexCount[ri.key].second--;
        if(edges.size()>nMaxEdgesNum)
          return;
      }
    }
  }
  getEdgeLoops(edges,roots,polygons);
}

template<class C>
void IsoOctree::getEdgeLoops(std::vector<std::pair<C,C> > & edges,
                             TRoots& roots,
                             std::vector<std::vector<int> > & polygons)
{
  size_t polygonSize=polygons.size();
  C frontIdx,backIdx;
  std::pair<C,C> e,temp;

  while(edges.size())
  {
    std::vector<std::pair<C,C> > front,back;
    e=edges[0];
    polygons.resize(polygonSize+1);
    edges[0]=edges[edges.size()-1];
    edges.pop_back();
    frontIdx=e.second;
    backIdx=e.first;
    for(int j=int(edges.size())-1;j>=0;j--){
      if(edges[j].first==frontIdx || edges[j].second==frontIdx){
        if(edges[j].first==frontIdx)	{temp=edges[j];}
        else							{temp.first=edges[j].second;temp.second=edges[j].first;}
        frontIdx=temp.second;
        front.push_back(temp);
        edges[j]=edges[edges.size()-1];
        edges.pop_back();
        j=int(edges.size());
      }
      else if(edges[j].first==backIdx || edges[j].second==backIdx){
        if(edges[j].second==backIdx)	{temp=edges[j];}
        else							{temp.first=edges[j].second;temp.second=edges[j].first;}
        backIdx=temp.first;
        back.push_back(temp);
        edges[j]=edges[edges.size()-1];
        edges.pop_back();
        j=int(edges.size());
      }
    }
    polygons[polygonSize].resize(back.size()+front.size()+1);
    int idx=0;
    for(int j=int(back.size())-1;j>=0;j--)	polygons[polygonSize][idx++]=roots[back[j].first];
    polygons[polygonSize][idx++]=roots[e.first];
    for(int j=0;j<int(front.size());j++)	polygons[polygonSize][idx++]=roots[front[j].first];
    polygonSize++;
  }
}

template<class C>
void IsoOctree::getEdgeLoops(std::vector<std::pair<C,C> > & edges,
                             std::vector<std::vector<C> > & polygons)
{
  int polygonSize=polygons.size();
  C frontIdx,backIdx;
  std::pair<C,C> e,temp;

  while(edges.size())
  {
    std::vector<std::pair<C,C> > front,back;
    e=edges[0];
    polygons.resize(polygonSize+1);
    edges[0]=edges[edges.size()-1];
    edges.pop_back();
    frontIdx=e.second;
    backIdx=e.first;
    for(int j=int(edges.size())-1;j>=0;j--){
      if(edges[j].first==frontIdx || edges[j].second==frontIdx){
        if(edges[j].first==frontIdx)	{temp=edges[j];}
        else							{temp.first=edges[j].second;temp.second=edges[j].first;}
        frontIdx=temp.second;
        front.push_back(temp);
        edges[j]=edges[edges.size()-1];
        edges.pop_back();
        j=int(edges.size());
      }
      else if(edges[j].first==backIdx || edges[j].second==backIdx){
        if(edges[j].second==backIdx)	{temp=edges[j];}
        else							{temp.first=edges[j].second;temp.second=edges[j].first;}
        backIdx=temp.first;
        back.push_back(temp);
        edges[j]=edges[edges.size()-1];
        edges.pop_back();
        j=int(edges.size());
      }
    }
    polygons[polygonSize].resize(back.size()+front.size()+1);
    int idx=0;
    for(int j=int(back.size())-1;j>=0;j--)
      polygons[polygonSize][idx++]=back[j].first;
    polygons[polygonSize][idx++]=e.first;
    for(int j=0;j<int(front.size());j++)
      polygons[polygonSize][idx++]=front[j].first;
    polygonSize++;
  }
}

void IsoOctree::setMCIndex(const int& useFull)
{
  FUNCTION_PROFILER_3DENGINE;

  OctNode* temp;
  float cValues[Cube::CORNERS];

  // Clear the indices
  for(temp=tree.nextNode(NULL) ; temp ; temp=tree.nextNode(temp) )
    temp->nodeData.mcIndex=0;

  // Get the values at the leaf nodes and propagate up to the parents
  OctNode::NodeIndex nIdx;
  for(temp=tree.nextLeaf(NULL,nIdx) ; temp ; temp=tree.nextLeaf(temp,nIdx) )
  {
    for(int i=0;i<Cube::CORNERS;i++)
    {
      ISO_KEY key = OctNode::CornerIndex(nIdx,i,maxDepth);
      TVoxValues::iterator iter = (*m_pValues).find(key);
      if(iter==(*m_pValues).end())
        cValues[i] = 0.f;
      else
        cValues[i] = iter->second.val;
    }
    if(useFull)
      temp->nodeData.mcIndex=MarchingCubes::GetFullIndex(cValues,isoValue);
    else
      temp->nodeData.mcIndex=MarchingCubes::GetIndex(cValues,isoValue);
    if(temp->parent)
    {
      int cIndex=int(temp-temp->parent->children);
      int bitFlag = temp->nodeData.mcIndex & (1<<cIndex);
      if(bitFlag)
      {
        OctNode *parent,*child;
        child=temp;
        parent=temp->parent;
        while(parent && (child-parent->children)==cIndex)
        {
          parent->nodeData.mcIndex |= bitFlag;
          child=parent;
          parent=parent->parent;
        }
      }
    }
  }
}

/*
void IsoOctree::getDualIsoSurface(
std::vector<Vec3>& vertices,
std::vector<std::vector<int> > & polygons,
const int& useFull)
{
OctNode* temp;
OctNode* neighbors[Cube::CORNERS];
std::map<ISO_KEY,char> cornerSet;
float cValues[Cube::CORNERS];
float cVertexData[Cube::CORNERS];
Vec3 p;
int c1,c2;
EdgeKey eKey;
std::vector<std::vector<int> > polys;
std::vector<std::pair<int,int> > eList;
std::map<EdgeKey,int> vTable;

nKey.set(maxDepth);

for(int d=maxDepth;d>=0;d--)
{
OctNode::NodeIndex nIdx;
for(temp=tree.nextNode(NULL,nIdx) ; temp ; temp=tree.nextNode(temp,nIdx) )
{
if(nIdx.depth==d && !temp->children)
for(int c=0;c<Cube::CORNERS;c++)
{
ISO_KEY key=OctNode::CornerIndex(nIdx,c,maxDepth);
if(cornerSet.find(key)==cornerSet.end())
{
cornerSet[key]=1;
nKey.GetCornerNeighbors(temp,c,neighbors);
int count=0;
for(int i=0;i<Cube::CORNERS;i++)
if(neighbors[i])
{
cVertexData[i]=neighbors[i]->nodeData.val;
cValues[i]=cVertexData[i];
count++;
}
if(count==8)
{
int idx;
if(useFull)
idx=MarchingCubes::GetFullIndex(cValues);
else
idx=MarchingCubes::GetIndex(cValues);
// Add the necessary vertices
const std::vector<std::vector<int> > & table=MarchingCubes::caseTable(idx,useFull);
for(int i=0;i<int(table.size());i++)
{
for(int j=0;j<int(table[i].size());j++)
{
Cube::EdgeCorners(table[i][j],c1,c2);
eKey=EdgeKey(size_t(neighbors[c1]),size_t(neighbors[c2]));
if(vTable.find(eKey)==vTable.end())
{
vTable[eKey]=int(vertices.size());
p=_RootPosition(neighbors[c1]->nodeData.center,neighbors[c2]->nodeData.center,
cVertexData[c1],cVertexData[c2]);
vertices.push_back(p);
}
}
}
// Add the loops
for(size_t i=0;i<table.size();i++)
{
std::vector<int> polygon;
for(size_t j=0;j<table[i].size();j++)
{
Cube::EdgeCorners(table[i][j],c1,c2);
eKey=EdgeKey(size_t(neighbors[c1]),size_t(neighbors[c2]));
polygon.push_back(vTable[eKey]);
}
polygons.push_back(polygon);
}
}
}
}
}
}
}*/


void IsoOctree::getIsoSurface(std::vector<Vec3>& vertices,
                              std::vector<SSurfTypeInfo> *surface_types,
                              std::vector<ColorB> *colors,
                              std::vector<std::vector<int> > * pPolygonsIn,
                              std::vector<std::vector<int> > * pPolygonsOut,
                              const int& useFull,
                              const AABB * pAreaBox,
                              bool bSetMCIndex)
{
  FUNCTION_PROFILER_3DENGINE;

  OctNode* temp;
  TRoots roots;
  nKey.set(maxDepth);

  // Set the marching cubes values
  {
    FRAME_PROFILER( "getIsoSurface_setMCIndex", GetSystem(), PROFILE_3DENGINE );
    if(bSetMCIndex)
      setMCIndex(useFull);
  }

  {
    FRAME_PROFILER( "getIsoSurface_getRoots", GetSystem(), PROFILE_3DENGINE );

    OctNode::NodeIndex nIdx;
    for(temp=tree.nextLeaf(NULL,nIdx) ; temp ; temp=tree.nextLeaf(temp,nIdx) )
      getRoots(temp,nIdx,roots,vertices,surface_types,colors);
  }

  IsHeapValid();

  if(pPolygonsIn)
  {
    FRAME_PROFILER( "getIsoSurface_getIsoPolygons_In", GetSystem(), PROFILE_3DENGINE );

    OctNode::NodeIndex nIdx;
    nIdx=OctNode::NodeIndex();
    for(temp=tree.nextLeaf(NULL,nIdx) ; temp ; temp=tree.nextLeaf(temp,nIdx) )
    {
      AABB nodeBox = GetNodeAABB(nIdx);
      if(!pAreaBox || Overlap::AABB_AABB(nodeBox,*pAreaBox))
      {
        bool bHole = false;
        Vec3 vCenterWS = nodeBox.GetCenter()*CVoxTerrain::m_fMapSize;
        if(C3DEngine::m_pGetLayerIdAtCallback)
          bHole = C3DEngine::m_pGetLayerIdAtCallback->GetHoleAtPosition((int)(vCenterWS.y/GetTerrain()->GetHeightMapUnitSize()),(int)(vCenterWS.x/GetTerrain()->GetHeightMapUnitSize()));
        if(!bHole)
          getIsoPolygons(temp,nIdx,roots,*pPolygonsIn,useFull);
      }
    }
  }

  if(pPolygonsIn && pPolygonsIn->size() && pPolygonsOut)
  {
    FRAME_PROFILER( "getIsoSurface_getIsoPolygons_Out", GetSystem(), PROFILE_3DENGINE );

    OctNode::NodeIndex nIdx;
    nIdx=OctNode::NodeIndex();
    for(temp=tree.nextLeaf(NULL,nIdx) ; temp ; temp=tree.nextLeaf(temp,nIdx) )
    {
      AABB nodeBox = GetNodeAABB(nIdx);
      if(!pAreaBox || Overlap::AABB_AABB(nodeBox,*pAreaBox))
        ;
      else
      {
        bool bHole = false;
        Vec3 vCenterWS = nodeBox.GetCenter()*CVoxTerrain::m_fMapSize;
        if(C3DEngine::m_pGetLayerIdAtCallback)
          bHole = C3DEngine::m_pGetLayerIdAtCallback->GetHoleAtPosition((int)(vCenterWS.y/GetTerrain()->GetHeightMapUnitSize()),(int)(vCenterWS.x/GetTerrain()->GetHeightMapUnitSize()));
        if(!bHole)
          getIsoPolygons(temp,nIdx,roots,*pPolygonsOut,useFull);
      }
    }
  }

  /*
  OctNode::NodeIndex nIdx;
  BuildRenderMeshes(&tree, nIdx);*/
}
/*
void IsoOctree::_getIsoSoup(
                           std::vector<Vec3>& vertices,
                           std::vector<int>& surf_types,
                           std::vector<ColorB>& colors,
                           std::vector<std::vector<int> > & polygons,
                           const int& useFull)
{
  OctNode* temp;

  nKey.set(maxDepth);

  // Set the marching cubes values
  setMCIndex(useFull);

  OctNode::NodeIndex nIdx;
  for(temp=tree.nextLeaf(NULL,nIdx) ; temp ; temp=tree.nextLeaf(temp,nIdx) )
  {
    int pIndex=int(polygons.size());
    int idx=temp->nodeData.mcIndex;
    const std::vector<std::vector<int> > & table=MarchingCubes::caseTable(idx,useFull);
    polygons.resize(pIndex+table.size());
    for(size_t i=0;i<table.size();i++)
    {
      polygons[pIndex+i].resize(table[i].size());
      for(size_t j=0;j<table[i].size();j++)
      {
        polygons[pIndex+i][j]=int(vertices.size());
        Vec3 position;
        int surf_type=0;
        ColorB color;
        getRootPosition(temp,nIdx,table[i][j],position,surf_type,color);
        vertices.push_back(position);
        surf_types.push_back(surf_type);
        colors.push_back(color);
      }
    }
  }
}*/

void IsoOctree::setNormalFlatness(TFlatness & flatnessMap)
{
  FUNCTION_PROFILER_3DENGINE;

  TRoots roots;
  std::vector<Vec3> vertices;
  std::vector<SSurfTypeInfo> surf_types;

  nKey.set(maxDepth);

  // Set the marching cubes values
  setMCIndex(0);

  // Set the iso-vertex positions
  OctNode::NodeIndex nIdx;
  for(OctNode* temp=tree.nextLeaf(NULL,nIdx) ; temp ; temp=tree.nextLeaf(temp,nIdx) )
    getRoots(temp,nIdx,roots,vertices,&surf_types,NULL);

  // Get the polygon normals
  nIdx=OctNode::NodeIndex();
  byte nProgress=0;
  for(OctNode* temp=tree.nextLeaf(NULL,nIdx) ; temp ; temp=tree.nextLeaf(temp,nIdx) )
  {
    Vec3_tpl<ISO_DOUBLE> normal(0,0,0);
    ISO_DOUBLE area=0;

    std::vector<std::vector<int> > polygons;
    getIsoPolygons(temp,nIdx,roots,polygons,0);
    if(!polygons.size())
      continue;
    
    nProgress++;
    if(!nProgress)
      PrintProgress();

    float fSurfTransitionFound = 0;

    for(size_t i=0;i<polygons.size();i++)
    {
      Vec3_tpl<ISO_DOUBLE> n;
      n[0]=n[1]=n[2]=0;
      for(size_t j=0;j<polygons[i].size();j++)
      {      
        int i0 = polygons[i][(j+0)%polygons[i].size()];
        int i1 = polygons[i][(j+1)%polygons[i].size()];
        int i2 = polygons[i][(j+2)%polygons[i].size()];

        if(surf_types[i0] != surf_types[i1] || surf_types[i1] != surf_types[i2])
        {
          fSurfTransitionFound = 1.f;//0.000005; // not allow to simplify surface types transition
        }

        {
          Vec3_tpl<ISO_DOUBLE> vCross;
          CrossProductD(vertices[i1] - vertices[i0], vertices[i2] - vertices[i0], vCross);
          n+=vCross;
        }
      }
      area+=Length(n);
      normal+=n;
    }

    area += fSurfTransitionFound;

    ISO_KEY _key;

    _key=OctNode::CenterIndex(nIdx,maxDepth);
    flatnessMap[_key].first=normal;
    flatnessMap[_key].second=area;

    for(int c=0;c<Cube::CORNERS;c++)
    {
      ISO_KEY key = OctNode::CornerIndex(nIdx,c,maxDepth);

      TFlatness::iterator iter = flatnessMap.find(key);

      if(iter == flatnessMap.end())
      {
        flatnessMap[key].first=normal;
        flatnessMap[key].second=area;
      }
      else
      {
        iter->second.first+=normal;
        iter->second.second+=area;
      }
    }

    OctNode* parent=temp->parent;
    OctNode::NodeIndex pIdx=nIdx.parent();
    while(parent)
    {
      for(int c=0;c<Cube::CORNERS;c++)
      {
        ISO_KEY key = OctNode::CornerIndex(pIdx,c,maxDepth);

        TFlatness::iterator iter = flatnessMap.find(key);

        if(iter == flatnessMap.end())
        {
          flatnessMap[key].first=normal;
          flatnessMap[key].second=area;
        }
        else
        {
          iter->second.first+=normal;
          iter->second.second+=area;
        }
      }

      --pIdx;
      parent=parent->parent;
    }
  }
}

void IsoOctree::Render(const AABB & aabb)
{
  FUNCTION_PROFILER_3DENGINE;

  if(GetCVars()->e_VoxTer == 2 && !m_aabbDirty.IsReset())
    DrawBBox(AABB(m_aabbDirty.min*CVoxTerrain::m_fMapSize,m_aabbDirty.max*CVoxTerrain::m_fMapSize));

  OctNode::NodeIndex nIdx;
  SIsoMesh::m_nThisFrameMeshMemoryUsage=0;
  SIsoMesh::m_nThisFrameTextureMemoryUsage=0;
  Render(&renderTree, nIdx);

  if(GetCVars()->e_VoxelDebug)
  {
    Vec3 vTestPos = GetCamera().GetPosition();
    vTestPos += GetCamera().GetViewdir();

//    float fVal0 = -1000000;
  //  GetMaterialByPosition(&tree, nIdx,vTestPos/CVoxTerrain::m_fMapSize,&fVal0,100);

    ColorF resColor;
    SSurfTypeInfo surf_type;
    SVoxValueCache voxValueCache;
    Vec3 vNormal;
    CVoxTerrain::GetVoxelValueInterpolated(this, vTestPos, resColor, 0, 0, surf_type, vNormal, &voxValueCache);

//    float fVal1 = -1000000;
  //  GetValueByPosition(&tree, nIdx,(vTestPos+Vec3(0,0,1.f))/CVoxTerrain::m_fMapSize,&fVal1,100);

    ColorF col = Col_Red;

    DrawSphere(vTestPos,0.025f);
    GetRenderer()->DrawLabelEx(vTestPos, 0.25f, (float*)&col, false, true, "s%d", surf_type.GetDomSType());

    DrawSphere(vTestPos+vNormal/6,0.0125f);
//    DrawLine(vTestPos,vTestPos+vNormal*.5f);

/*    SSurfTypeInfo surf_type;
    {
      ColorF vVertColorInterp;
      m_pVoxTerrain->GetVoxelValueInterpolated(this, vTestPos, vVertColorInterp, NULL, NULL, surf_type);
      surf_type.Normalize();
    }

    GetRenderer()->DrawLabelEx(vTestPos, 0.25f, (float*)&col, false, true, 
      "W8 %d-%d,%d-%d,%d-%d,%d-%d", 
      surf_type.ty[0], 
      surf_type.we[0], 
      surf_type.ty[1], 
      surf_type.we[1], 
      surf_type.ty[2], 
      surf_type.we[2], 
      surf_type.ty[3], 
      surf_type.we[3]);*/
  }
}

void IsoOctree::GetMeshes(OctNode* node, const OctNode::NodeIndex& nIdx, TIsoMeshes & foundMeshes)
{
  FUNCTION_PROFILER_3DENGINE;

  bool bLeafMesh = (!node->children || nIdx.depth >= nMeshDepth);
  /*
  if(!bDraw)
  {
  OctNode::NodeIndex nIdxTmp = nIdx;
  if(!CheckDepth(node,nIdxTmp, nIdx.depth+nMaxMeshBuildDepth))
  bDraw = true;
  }
  */
  ISO_KEY key = OctNode::CenterIndex(nIdx,maxDepth);

  SIsoMesh * pIsoMesh = 0;
  TIsoMeshes::iterator iter = threadMeshes.find(key);
  if(iter != threadMeshes.end())
    pIsoMesh = iter->second;

  if( pIsoMesh && !pIsoMesh->m_nRebuildRequestedFrameId && pIsoMesh->IsMeshCompiled() )
    foundMeshes[key] = pIsoMesh;

  if(!bLeafMesh)
  {
    for(int i=0;i<Cube::CORNERS;i++)
    {
      GetMeshes(&node->children[i],nIdx.child(i), foundMeshes);
    }
  }
}

void SNodeUpdateRequest::UpdateSortVal_HQ(int _nMainFrameId, const Vec3 & _vCamPosNorm, TIsoMeshes & threadMeshes)
{
  // find how soon node will be visible

  fPredictionRatio = 0;

  float hw;
  Vec3 center;

  // this node bbox
  OctNode::CenterAndWidth(nIdx,center,hw);
  hw*=0.5f*CVoxTerrain::m_fMapSize;
  center*=CVoxTerrain::m_fMapSize;
  AABB boxThis(Vec3(center.x-hw,center.y-hw,center.z-hw),Vec3(center.x+hw,center.y+hw,center.z+hw));
/*
  // this node parent bbox
  OctNode::NodeIndex nIdxParent = nIdx;
  if(nIdxParent.depth>0)
    --nIdxParent;

  OctNode::CenterAndWidth(nIdxParent,center,hw);
  hw*=0.5f*CVoxTerrain::m_fMapSize;
  center*=CVoxTerrain::m_fMapSize;
  AABB boxParent(Vec3(center.x-hw,center.y-hw,center.z-hw),Vec3(center.x+hw,center.y+hw,center.z+hw));

  // dist where this node decides to split and become not visible (except mesh node size nodes)
  float dist_min = boxThis.GetSize().x / 2 / Cry3DEngineBase::GetCVars()->e_VoxTerViewDistRatio; 

  // dist where parent decides to split and this node become visible
  float dist_max = boxParent.GetSize().x / 2 / Cry3DEngineBase::GetCVars()->e_VoxTerViewDistRatio; 

  // distance to parent node
  float dist_parent = boxParent.GetDistance(_vCamPosNorm*CVoxTerrain::m_fMapSize);
*/
  // distance to this node
  float dist_this = boxThis.GetDistance(_vCamPosNorm*CVoxTerrain::m_fMapSize);
/*
  if(dist_this < dist_min)
  { // fade
    if(nIdx.depth >= IsoOctree::nMeshDepth)
      fPredictionRatio = 1.f;
    else
      fPredictionRatio = 1.f - SATURATE((dist_min - dist_this)/fPredictionDistance);
  }
  else if(dist_parent > dist_max)
  { // fade
    fPredictionRatio = 1.f - SATURATE((dist_parent - dist_max)/fPredictionDistance);
  }
  else
  { // constant maximum
    fPredictionRatio = 1.f;
  }

  // process visible first
//  if(Cry3DEngineBase::GetCamera().IsAABBVisible_F(boxThis))
  //  fPredictionRatio += 2.f;
*/
  // process near first
  fPredictionRatio = 1.f/(dist_this+.1f);//(1.f + dist_this/3000.f);

/*  nVis=0;
  TIsoMeshes::iterator iter = threadMeshes.find(centerKey);
  if( iter != threadMeshes.end() )
    if(SIsoMesh * pIsoMesh = iter->second)
      if(((int)pIsoMesh->m_nLastDrawFrameId) >= _nMainFrameId-10)
        nVis=1;*/

//  if(CVoxTerrain::IsEditing())
    fNodeSize = hw;
  //else
    //fNodeSize = 1.f / hw;
}

struct S_Cmp_OctNode
{
  bool operator()(const SNodeUpdateRequest& a, const SNodeUpdateRequest& b)
  {
    if(a.nRequestPrioriry != b.nRequestPrioriry)
      return a.nRequestPrioriry > b.nRequestPrioriry;

    if(CVoxTerrain::IsEditing())
    {
      if(a.nRequestFrameId != b.nRequestFrameId)
        return a.nRequestFrameId < b.nRequestFrameId;
    }
    else
    {
      if(a.fNodeSize != b.fNodeSize)
        return a.fNodeSize > b.fNodeSize;
    }

    if(a.fPredictionRatio != b.fPredictionRatio)
      return (a.fPredictionRatio > b.fPredictionRatio);

    if(a.fNodeSize != b.fNodeSize) // hack: start crash workaround
      return a.fNodeSize < b.fNodeSize;

    return false;
  }
};
int IsoOctree::GetProgress()
{
  return m_nProgress;
}

struct IIsoThreadMessage
{
  enum EMessageType
  {
    eEdit=0,
    eUpdate,
    ePhysicalize,
    eMakeMesh,
    eDeleteMesh,
    eUpdateMeshesMap,
    eEnablePhysEntity,
    ePrintText,
    eLast
  };

  static const char * arrMessageNames[eLast];

  virtual EMessageType GetType() = 0;
};

const char * IIsoThreadMessage::arrMessageNames[IIsoThreadMessage::eLast] =
{
  "Edit",
  "Update",
  "Physicalize",
  "MakeMesh",
  "DeleteMesh",
  "UpdateMeshesMap",
  "EnablePhysEntity",
  "PrintText",
};

struct SEditThreadMessage : public IIsoThreadMessage
{
  SEditThreadMessage() 
  {  
    vertices=0;
    indices=0;
    meshBox.Reset();
    nProgressCur=0;
    nProgressAll=0;
    editingTime=0;
    bAllowRequestTreeUpdate=true;
  }

  EMessageType GetType() { return eEdit; }

  Vec3 vWSPos;
  float fRadius;
  int nSurfaceTypeId;
  ColorB color;
  EVoxelEditOperation eOperation;
  EVoxelBrushShape eShape;
  int nDetailLevel;
  std::vector<Vec3> * vertices;
  std::vector<int> * indices;
  AABB meshBox;
  int nProgressCur;
  int nProgressAll;
  float editingTime;
  bool bAllowRequestTreeUpdate;
};

struct SUpdateThreadMessage : public IIsoThreadMessage
{
  SUpdateThreadMessage() {  }  
  EMessageType GetType() { return eUpdate; }

};

struct SPhysThreadMessage : public IIsoThreadMessage
{
  SPhysThreadMessage() {  }  
  EMessageType GetType() { return ePhysicalize; }

  AABB areaBox;
  bool bEnable;
};

struct SMakeMeshThreadMessage : public IIsoThreadMessage
{
  SMakeMeshThreadMessage() {  }  
  EMessageType GetType() { return eMakeMesh; }

  SIsoMesh * pIsoMesh;
  SIsoMesh * pNewMesh;
  OctNode::NodeIndex nIdx;
  IsoOctree * pAreaIsoOctree;
  AABB aabbCutIn;
  ISO_KEY key;
  int nItemId;
  PodArray<std::pair<ISO_KEY, SVoxValue> > * pColValues;
};

struct SDeleteMeshThreadMessage : public IIsoThreadMessage
{
  SDeleteMeshThreadMessage() {  }  
  EMessageType GetType() { return eDeleteMesh; }

  SIsoMesh * pMeshForDelete;
};

struct SUpdateMeshesMapThreadMessage : public IIsoThreadMessage
{
  SUpdateMeshesMapThreadMessage() {  }  
  EMessageType GetType() { return eUpdateMeshesMap; }

  TIsoMeshes * pThreadMeshesCopy;
  
  TIsoMeshes threadEmptyMeshesDelta;
};

struct SEnablePhysEntityThreadMessage : public IIsoThreadMessage
{
  SEnablePhysEntityThreadMessage() {  }  
  EMessageType GetType() { return eEnablePhysEntity; }

  bool bEnable;
  SIsoMesh * pIsoMesh;
};

struct SPrintTextThreadMessage : public IIsoThreadMessage
{
  SPrintTextThreadMessage() {  }  
  EMessageType GetType() { return ePrintText; }

  char * pText;
  float fStartTime;
};

struct CIsoOctreeThread : public Cry3DEngineBase, public CrySimpleThread<>
{
  CIsoOctreeThread(int nThreadSlotId)
  {	
    szCurrentMessageIn[0]=0;
    bContinue=true;
    bDone=false;
    m_nMessagesIn=m_nMessagesOut=0;
    m_nThreadSlotId = nThreadSlotId;
    
    Start(0, "IsoOctreeThread");

#ifdef XENON
    XSetThreadProcessor(GetHandle(), 2);
#endif

    if(!pRenderMeshes)
      pRenderMeshes = new TIsoMeshes;
    if(!pRenderEmptyMeshes)
      pRenderEmptyMeshes = new TIsoMeshes;

    if(!m_pIsoOctree)
      m_pIsoOctree = new IsoOctree(new TVoxValues);
  }

  ~CIsoOctreeThread()
  {
    Stop();

    Cancel();

    WaitForThread();

    while(!bDone)
      CrySleep(10);
    CrySleep(10);

    Stop();

    WaitForThread();

    SAFE_DELETE(m_pIsoOctree);

    SAFE_DELETE(pRenderMeshes);
    SAFE_DELETE(pRenderEmptyMeshes);
  }

  static bool SetCompiledData(byte * pData, int nDataSize, bool bUpdateMesh, EEndian eEndian, CVoxTerrain * pVoxTerrain, AABB * pAreaBox, int nSID)
  {
    pVoxTerrain->m_bAllowEditing = false;
    AUTO_LOCK(g_cIsoTreeAccess);

    SIsoTreeChunkHeader * pTerrainChunkHeader = (SIsoTreeChunkHeader *)pData;
    pData += sizeof(SIsoTreeChunkHeader);
    nDataSize -= sizeof(SIsoTreeChunkHeader);

    return pVoxTerrain->Load_T(pData, nDataSize, pTerrainChunkHeader, bUpdateMesh, eEndian, pAreaBox, nSID);
  }

  static IMemoryBlock * GetCompiledData(bool bSaveMesh, EEndian eEndian, CVoxTerrain * pVoxTerrain, bool bSaveForEditing, AABB * pAreaBox, int nSID)
  {
    FUNCTION_PROFILER_3DENGINE;

#ifdef WIN32
    if(!pAreaBox)
      SetProcessWorkingSetSize( GetCurrentProcess(),-1,-1 );
#endif //WIN32

    pVoxTerrain->m_bAllowEditing = false;
    AUTO_LOCK(g_cIsoTreeAccess);

    // refresh render structures
    if(bSaveMesh)
    {
      IsoOctree * pIsoTree = CIsoOctreeThread::m_pIsoOctree;

      OctNode::NodeIndex nIdx;

      if(!bSaveForEditing)
      {
        pIsoTree->PrintMessage("Preloading cached meshes ...");
        pIsoTree->PreloadCachedMeshes(&pIsoTree->tree, nIdx);
        pIsoTree->PrintMessagePlus(" ok");
      }

      {
        AUTO_LOCK(g_cIsoMeshes);

        *CIsoOctreeThread::pRenderMeshes = pIsoTree->threadMeshes;
        *CIsoOctreeThread::pRenderEmptyMeshes = pIsoTree->threadEmptyMeshes;
      }

      pIsoTree->renderTree.deleteChildren(IsoOctree::m_aabbDirty, IsoOctree::GetNodeAABB(nIdx,false));
      AABB nodeBox = pIsoTree->GetNodeAABB(nIdx);
      pIsoTree->CopyTree(NULL, nodeBox, pIsoTree->nMeshDepth, &pIsoTree->tree, nIdx, &pIsoTree->renderTree, true, 0, 0, 0);
    }

    CMemoryBlock * pData = new CMemoryBlock;
    pData->Allocate(pVoxTerrain->GetCompiledDataSize(bSaveMesh, bSaveForEditing, pAreaBox, nSID));
    pVoxTerrain->GetCompiledData((byte*)pData->GetData(), pData->GetSize(), bSaveMesh, eEndian, bSaveForEditing, pAreaBox, nSID);

    return pData;
  }

  virtual void Run()
  {
    CryThreadSetName( ~0, "IsoOctreeThread" );

    while(bContinue)
    {
      if(IIsoThreadMessage * pIMessage = GetTaskIn())
      {
        IIsoThreadMessage::EMessageType eType = pIMessage->GetType();

        strcpy(szCurrentMessageIn, IIsoThreadMessage::arrMessageNames[eType]);

        if(eType == IIsoThreadMessage::eMakeMesh)
        {
          if(GetCVars()->e_VoxTerTraceMessages==1)
            PrintMessage("In: eMakeMesh");

          // now make a mesh
          SMakeMeshThreadMessage * pNewTask = (SMakeMeshThreadMessage*)pIMessage;

          bool bLoaded = false;

          bool b2DMixMask = GetCVars()->e_VoxTerHeightmapEditing && GetCVars()->e_VoxTerMixMask;

          if(GetCVars()->e_VoxTerTexBuildOnCPU && !b2DMixMask)
          {
            ISO_KEY key = OctNode::CenterIndex( pNewTask->nIdx, IsoOctree::maxDepth );

            char szFileName[256]="";
            SIsoMesh::MakeTempTextureFileName(szFileName, sizeof(szFileName),0,"MTT",key);

            if(FILE * f = fopen(szFileName,"rb"))
            {
              strcpy(szCurrentMessageIn, "FileRead");

              // load if ready

              int nSize = 0;
              fread(&nSize, 1, sizeof(nSize), f);        
              int nModId = 0;
              fread(&nModId, 1, sizeof(nModId), f);        

              if(nModId>IsoOctree_nModId)
              {
//                if(GetCVars()->e_VoxTer == 2)
                  PrintMessage("MTT file ignored: %s, file nModId = %d, level nModId = %d", szFileName, nModId, IsoOctree_nModId);
              }
              else
              {
                pNewTask->pNewMesh = new SIsoMesh(OctNode::CenterIndex( pNewTask->nIdx, IsoOctree::maxDepth ));
                SAFE_DELETE(pNewTask->pNewMesh->m_pMeshForSerialization);
                pNewTask->pNewMesh->m_pMeshForSerialization = new CMemoryBlock();
                pNewTask->pNewMesh->m_pMeshForSerialization->Allocate(nSize);
                fread((byte*)pNewTask->pNewMesh->m_pMeshForSerialization->GetData(), 1, nSize, f);

                fread(&pNewTask->pNewMesh->m_nTexDimX, 1, sizeof(pNewTask->pNewMesh->m_nTexDimX), f);
                fread(&pNewTask->pNewMesh->m_nTexDimY, 1, sizeof(pNewTask->pNewMesh->m_nTexDimY), f);
                if(!pNewTask->pNewMesh->m_nTexDimX || !pNewTask->pNewMesh->m_nTexDimY)
                  Error(__FUNC__);

                // load zipped re-mesh texture
                int nZipSize = 0;
                int nElRead = fread(&nZipSize, 1, sizeof(nZipSize), f);
                if(nZipSize && (nZipSize < (16*1024*1024)) && nElRead==sizeof(nZipSize))
                {
                  pNewTask->pNewMesh->m_pZipData = new CMemoryBlock;
                  pNewTask->pNewMesh->m_pZipData->Allocate(nZipSize);
                  int nZipElRead = fread(pNewTask->pNewMesh->m_pZipData->GetData(), 1, pNewTask->pNewMesh->m_pZipData->GetSize(), f);
                  if(nZipElRead!=pNewTask->pNewMesh->m_pZipData->GetSize())
                  {
                    Error("%s, Zip data fread failed, %s", __FUNC__, szFileName);
                    Error("nZipSize=%d, nZipElRead=%d", nZipSize, nZipElRead);
                  }
                }
                else if(nZipSize || nElRead!=sizeof(nZipSize))
                {
                  Error("%s, nZipSize fread failed, %s", __FUNC__, szFileName);
                  Error("nZipSize=%d, nElRead=%d", nZipSize, nElRead);
                }

                bLoaded = true;
              }

              fclose(f);
            }
          }

          OctNode::NodeIndex nIdxGlobal;

          TVoxValues * pValOld = pNewTask->pAreaIsoOctree->m_pValues;

          if(pNewTask->pColValues && !bLoaded)
          {
            pNewTask->pAreaIsoOctree->m_pValues = new TVoxValues;

            for(int i=0; i<pNewTask->pColValues->Count(); i++)
            {
              const std::pair<ISO_KEY, SVoxValue> & rPair = pNewTask->pColValues->GetAt(i);
              (*pNewTask->pAreaIsoOctree->m_pValues)[rPair.first] = rPair.second;
            }

            SAFE_DELETE(pNewTask->pColValues);
          }

          if(!bLoaded)
            pNewTask->pNewMesh = pNewTask->pAreaIsoOctree->GenerateIsoMeshForRendering(pNewTask->nIdx, GetCVars()->e_VoxTerLog==2, NULL, &m_pIsoOctree->tree, &nIdxGlobal, szCurrentMessageIn);

          if(pNewTask->pIsoMesh)
            pNewTask->pNewMesh->m_nRebuildStartedFrameId = pNewTask->pIsoMesh->m_nRebuildStartedFrameId;

          if(pNewTask->pAreaIsoOctree->m_pValues != pValOld)
            SAFE_DELETE(pNewTask->pAreaIsoOctree->m_pValues);
          pNewTask->pAreaIsoOctree->m_pValues = pValOld;

          SAFE_DELETE(pNewTask->pAreaIsoOctree);

          AABB wsBox = pNewTask->aabbCutIn;
          wsBox.min *= CVoxTerrain::m_fMapSize;
          wsBox.max *= CVoxTerrain::m_fMapSize;

          //    DrawBBox(wsBox, Col_Yellow);

          pNewTask->pNewMesh->m_WSBBox = IsoOctree::GetNodeAABB(pNewTask->nIdx, true);

          bool bEmpty = true;

          if(pNewTask->pNewMesh)
          {
            if(pNewTask->pNewMesh->m_pGeneratedMesh && pNewTask->pNewMesh->m_pGeneratedMesh->GetIndexCount())
              bEmpty = false;
            if(pNewTask->pNewMesh->m_pMeshForSerialization && pNewTask->pNewMesh->m_pMeshForSerialization->GetSize())
              bEmpty = false;
          }

          ISO_KEY key = pNewTask->key;

          if( pNewTask->pIsoMesh )
          {
            assert(key == pNewTask->pIsoMesh->m_key);
            MSGO_DeleteIsoMesh(pNewTask->pIsoMesh);
          }

          m_pIsoOctree->m_nProgress = pNewTask->nItemId;

          if(bEmpty)
          {
            AUTO_LOCK(g_cIsoMeshes);

            m_pIsoOctree->threadEmptyMeshes[key] = pNewTask->pNewMesh;
            m_pIsoOctree->threadEmptyMeshesDelta[key] = pNotNULL;
            assert(key == pNewTask->pNewMesh->m_key);
            MSGO_DeleteIsoMesh(pNewTask->pNewMesh);
            SAFE_DELETE(pNewTask);
          }
          else
          {
            AUTO_LOCK(g_cIsoMeshes);

            m_pIsoOctree->threadEmptyMeshes.erase(key);
            m_pIsoOctree->threadEmptyMeshesDelta[key] = NULL;
            m_pIsoOctree->threadMeshes[key] = pNewTask->pNewMesh;
          }

          {
            AUTO_LOCK(g_cIsoNodesForUpdate);

            for(int i=0; i<m_pIsoOctree->m_nodesForUpdate.Count(); i++)
            {
              if(m_pIsoOctree->m_nodesForUpdate[i].centerKey == key)
              {
                assert(m_pIsoOctree->m_nodesForUpdate[i].bInProgress);
                m_pIsoOctree->m_nodesForUpdate[i].bInProgress = false;
                break;
              }
            }
          }

          if(pNewTask)
            AddTaskOut(pNewTask);
        }
        else if(eType == IIsoThreadMessage::ePhysicalize)
        {
          SPhysThreadMessage * pTask = (SPhysThreadMessage*)pIMessage;
          AABB areaBoxNorm = pTask->areaBox;
          areaBoxNorm.min /= CVoxTerrain::m_fMapSize;
          areaBoxNorm.max /= CVoxTerrain::m_fMapSize;
          int key = (int)(pTask->areaBox.min.x*CVoxTerrain::m_fMapSize + pTask->areaBox.min.y);

          // delete old if found
          {
            AUTO_LOCK(g_cPhysMeshes);

            if(m_pIsoOctree->physMeshes.find(key) != m_pIsoOctree->physMeshes.end())
            {
              if(GetCVars()->e_VoxTerTraceMessages==1)
                PrintMessage("In: ePhysicalize False");

              SIsoMesh * pPhysMesh = m_pIsoOctree->physMeshes[key];
              m_pIsoOctree->physMeshes[key] = NULL;
              m_pIsoOctree->physMeshes.erase(key);
              MSGO_DeleteIsoMesh(pPhysMesh);
            }
          }

          // make new if requested
          if(pTask->bEnable)
          {
            if(GetCVars()->e_VoxTerTraceMessages==1)
              PrintMessage("In: ePhysicalize True");

            OctNode::NodeIndex nIdx;
            SIsoMesh * pPhysMesh = m_pIsoOctree->PhysicalizeBox(nIdx, areaBoxNorm);

            {
              AUTO_LOCK(g_cPhysMeshes);

              m_pIsoOctree->physMeshes[key] = pPhysMesh;            
            }

            MSGO_EnablePhysEntity(true, pPhysMesh);
          }

          delete pTask;
        }
        else if(eType == IIsoThreadMessage::eUpdate)
        {
          if(GetCVars()->e_VoxTerTraceMessages==1)
            PrintMessage("In: eUpdate");

          SUpdateThreadMessage * pTask = (SUpdateThreadMessage*)pIMessage;
          m_pIsoOctree->UpdateTree();
          delete pTask;
        }       
        else if(eType == IIsoThreadMessage::eEdit)
        {
          if(GetCVars()->e_VoxTerTraceMessages==1)
            PrintMessage("In: eEdit");

          SEditThreadMessage * pTask = (SEditThreadMessage*)pIMessage;

          if(m_pVoxTerrain->m_bAllowEditing)
          {
            m_pIsoOctree->DoVoxelShapeTask( pTask->vWSPos, pTask->fRadius, pTask->nSurfaceTypeId, pTask->color, 
              pTask->eOperation, pTask->eShape, pTask->nDetailLevel, NULL, 
							pTask->vertices, pTask->indices, pTask->meshBox, pTask->nProgressCur, pTask->nProgressAll, pTask->editingTime, pTask->bAllowRequestTreeUpdate);
          }

          delete pTask->vertices;
          delete pTask->indices;
          delete pTask;
        }
        else if(eType == IIsoThreadMessage::ePrintText)
        {
          if(GetCVars()->e_VoxTerTraceMessages==1)
            PrintMessage("In: ePrintText");

          SPrintTextThreadMessage * pTask = (SPrintTextThreadMessage*)pIMessage;
          if(strstr(pTask->pText, "%.1f"))
            PrintMessage(pTask->pText, GetCurTimeSec() - pTask->fStartTime);
          else
            PrintMessage(pTask->pText);
          delete pTask->pText;
          delete pTask;
        }
        else
          assert(!"Undefined task");

				strcpy(szCurrentMessageIn, " -Done- ");
      }
      else
      {
        if(GetCVars()->e_VoxTerTraceMessages==1)
          PrintMessage("In: -Idle-");

        strcpy(szCurrentMessageIn, " -Idle- ");
        CrySleep(10);
      }
    }

    bDone = true;
  }

  virtual void Cancel()
  {
    bContinue = false;
  }

  static void AddTaskIn(struct IIsoThreadMessage * pTask) 
  { 
    AUTO_LOCK(g_cTaksIn);

    m_arrMessagesIn.Add(pTask);

    m_nMessagesIn = m_arrMessagesIn.Count();
  }

  static void AddTaskOut(struct IIsoThreadMessage * pTask) 
  { 
    AUTO_LOCK(g_cTaksOut);

    m_arrMessagesOut.Add(pTask);

    m_nMessagesOut = m_arrMessagesOut.Count();
  }

  IIsoThreadMessage * GetTaskIn() 
  { 
    AUTO_LOCK(g_cTaksIn);

    IIsoThreadMessage * pTask = 0;

    if(m_nThreadSlotId < GetCVars()->e_VoxTerThreads && m_arrMessagesIn.Count())
    {
      pTask = m_arrMessagesIn[0];
      m_arrMessagesIn.Delete((int)0);
    }

    m_nMessagesIn = m_arrMessagesIn.Count();

    return pTask; 
  }

  static IIsoThreadMessage * GetTaskOut() 
  { 
    if(!m_nMessagesOut)
      return 0;

    AUTO_LOCK(g_cTaksOut);

    IIsoThreadMessage * pTask = 0;

    if(m_arrMessagesOut.Count())
    {
      pTask = m_arrMessagesOut[0];
      m_arrMessagesOut.Delete((int)0);
    }

    m_nMessagesOut = m_arrMessagesOut.Count();

    return pTask; 
  }

  static void MSGI_PhysicalizeContentOfBBox(const AABB & areaBox, bool bEnable)
  {
    SPhysThreadMessage * pTask = new SPhysThreadMessage;
    pTask->areaBox = areaBox;
    pTask->bEnable = bEnable;

    if(GetCVars()->e_VoxTerAsyncPhysicalization && !CVoxTerrain::IsEditing())
      AddTaskIn(pTask);
    else
    {
      AUTO_LOCK(g_cPhysMeshes);

      AABB areaBoxNorm = pTask->areaBox;
      areaBoxNorm.min /= CVoxTerrain::m_fMapSize;
      areaBoxNorm.max /= CVoxTerrain::m_fMapSize;
      int key = (int)(pTask->areaBox.min.x*CVoxTerrain::m_fMapSize + pTask->areaBox.min.y);

      if(pTask->bEnable && m_pIsoOctree->physMeshes.find(key) == m_pIsoOctree->physMeshes.end())
      {
        OctNode::NodeIndex nIdx;
        SIsoMesh * pPhysMesh = m_pIsoOctree->PhysicalizeBox(nIdx, areaBoxNorm);
        int nSize = m_pIsoOctree->physMeshes.size();
        m_pIsoOctree->physMeshes[key] = pPhysMesh;            
        MSGO_EnablePhysEntity(true, pPhysMesh);
      }

      if(!pTask->bEnable && m_pIsoOctree->physMeshes.find(key) != m_pIsoOctree->physMeshes.end())
      {
        SIsoMesh * pPhysMesh = m_pIsoOctree->physMeshes[key];
        m_pIsoOctree->physMeshes[key] = NULL;
        m_pIsoOctree->physMeshes.erase(key);
        MSGO_EnablePhysEntity(false, pPhysMesh);
        MSGO_DeleteIsoMesh(pPhysMesh);
      }

      delete pTask;
    }

  }

  static void MSGI_PrintText(const char * pText)
  {
    SPrintTextThreadMessage * pTask = new SPrintTextThreadMessage;
    pTask->pText = new char[strlen(pText)+1];
    pTask->fStartTime = Get3DEngine()->GetCurTimeSec();
    strcpy(pTask->pText, pText);
    AddTaskIn(pTask);
  }

  static void MSGI_Update(bool bAppend)
  {
    AUTO_LOCK(g_cTaksIn);

    for(int i=0; i<m_arrMessagesIn.Count(); i++)
      if(m_arrMessagesIn[i]->GetType() == IIsoThreadMessage::eUpdate)
        return;

    for(int i=0; i<ISO_THREADS_NUM && i<GetCVars()->e_VoxTerThreads; i++)
      if(strstr(m_pVoxTerrain->m_pIsoOctreeThreads[i]->szCurrentMessageIn,IIsoThreadMessage::arrMessageNames[IIsoThreadMessage::eUpdate]))
        return;

    if(bAppend)
    {
      m_arrMessagesIn.Add(new SUpdateThreadMessage);
    }
    else
    {
      m_arrMessagesIn.InsertBefore(new SUpdateThreadMessage, 0);
    }

    m_nMessagesIn = m_arrMessagesIn.Count();
  }



  static void MSGO_DeleteIsoMesh(SIsoMesh * pMeshForDelete)
  {
    {
      AUTO_LOCK(g_cIsoMeshes);
      m_pIsoOctree->threadMeshes.erase(pMeshForDelete->m_key);
    }

    SDeleteMeshThreadMessage * pTask = new SDeleteMeshThreadMessage;
    pTask->pMeshForDelete = pMeshForDelete;
    AddTaskOut(pTask);
  }

  static void MSGO_UpdateMeshesMap()
  {
    SUpdateMeshesMapThreadMessage * pTask = new SUpdateMeshesMapThreadMessage;

    {
      AUTO_LOCK(g_cIsoMeshes);

      pTask->pThreadMeshesCopy = new TIsoMeshes;
      *pTask->pThreadMeshesCopy = m_pIsoOctree->threadMeshes;

      pTask->threadEmptyMeshesDelta = m_pIsoOctree->threadEmptyMeshesDelta;
      m_pIsoOctree->threadEmptyMeshesDelta.clear();
    }

    AddTaskOut(pTask);
  }

  static void MSGO_EnablePhysEntity(bool bEnable, SIsoMesh * pIsoMesh)
  {
    SEnablePhysEntityThreadMessage * pTask = new SEnablePhysEntityThreadMessage;
    pTask->bEnable = bEnable;
    pTask->pIsoMesh = pIsoMesh;
    AddTaskOut(pTask);
  }

  static void Update(C3DEngine * pEng);

  static bool bContinue;
  bool bDone;

  static PodArray<IIsoThreadMessage*> m_arrMessagesIn;
  static PodArray<IIsoThreadMessage*> m_arrMessagesOut;
  
  static int m_nMessagesIn;
  static int m_nMessagesOut;

  static class IsoOctree * m_pIsoOctree;

  static TIsoMeshes * pRenderMeshes;
  static TIsoMeshes * pRenderEmptyMeshes;

  char szCurrentMessageIn[256];

  int m_nThreadSlotId;
};

PodArray<IIsoThreadMessage*> CIsoOctreeThread::m_arrMessagesOut;
PodArray<IIsoThreadMessage*> CIsoOctreeThread::m_arrMessagesIn;
bool CIsoOctreeThread::bContinue=true;
int CIsoOctreeThread::m_nMessagesOut=0;
int CIsoOctreeThread::m_nMessagesIn=0;
class IsoOctree * CIsoOctreeThread::m_pIsoOctree=0;
TIsoMeshes * CIsoOctreeThread::pRenderMeshes=0;
TIsoMeshes * CIsoOctreeThread::pRenderEmptyMeshes=0;

void CIsoOctreeThread::Update(C3DEngine * pEng)
{
  if(m_bEditor)
  {
    FRAME_PROFILER( "IsoOctreeThread::Update_MSGI_UpdateAndSleep", GetSystem(), PROFILE_3DENGINE );

    // make sure we do check update every X ms
    if(CIsoOctreeThread::m_nMessagesIn<1024)
    {
      static float fUpdateReqTime = 0;
      if( fabs(pEng->GetCurTimeSec() - fUpdateReqTime) > 0.05f )
      {
        MSGI_Update(false);
        fUpdateReqTime = pEng->GetCurTimeSec();

        // keep editor mode during start
        static bool bStartMode = true;       
        if(m_pIsoOctree->m_nProgress > 100)
          bStartMode=false;
        if(bStartMode && !GetCVars()->e_VoxTerHeightmapEditing)
          CIsoOctreeThread::m_pIsoOctree->m_lastEditingTime = pEng->GetCurTimeSec();
      }
    }

    // give more CPU to mesh building threads when editing
    if( CVoxTerrain::IsEditing() )
      CrySleep(10);    
  }

  {
    FRAME_PROFILER( "IsoOctreeThread::Update_DelayedRelease", GetSystem(), PROFILE_3DENGINE );

    AUTO_LOCK(g_cAddForRelease);

    while(CVoxTerrain::m_arrTexturesForRel.Count())
    {
      SAFE_RELEASE(CVoxTerrain::m_arrTexturesForRel[0]);
      CVoxTerrain::m_arrTexturesForRel.Delete(0);
    }

    while(CVoxTerrain::m_arrIsoMeshesForDel.Count())
    {
      if(!m_bEditor)
        SAFE_DELETE(CVoxTerrain::m_arrIsoMeshesForDel[0]);
      CVoxTerrain::m_arrIsoMeshesForDel.Delete(0);
    }    
  }

  if(GetCVars()->e_VoxTerTraceMessages==2)
    pEng->PrintMessage("Out: *** New Frame ***");

  float fStartTime = pEng->GetCurAsyncTimeSec();

  // check if there some finished tasks
  while(IIsoThreadMessage * pMessage = GetTaskOut())
  {   
    FRAME_PROFILER( "IsoOctreeThread::Update_GetTaskOut", GetSystem(), PROFILE_3DENGINE );

    if(pMessage->GetType() == IIsoThreadMessage::eMakeMesh)
    {
      FRAME_PROFILER( "IsoOctreeThread::Update_eMakeMesh", GetSystem(), PROFILE_3DENGINE );

      if(GetCVars()->e_VoxTerTraceMessages==2)
        pEng->PrintMessage("Out: eMakeMesh");

      SMakeMeshThreadMessage * pFinishedTask = (SMakeMeshThreadMessage *)pMessage;

      assert(pFinishedTask->pNewMesh);

      ISO_KEY key = OctNode::CenterIndex( pFinishedTask->nIdx, IsoOctree::maxDepth );

      TIsoMeshes & renderMeshes = *pRenderMeshes;

      if(m_bEditor)
        pFinishedTask->pNewMesh->MakeRenderMesh();

      renderMeshes[key] = pFinishedTask->pNewMesh;

      pFinishedTask->pNewMesh = 0;

      delete pFinishedTask;
    }
    else if(pMessage->GetType() == IIsoThreadMessage::eDeleteMesh)
    {
      FRAME_PROFILER( "IsoOctreeThread::Update_eDeleteMesh", GetSystem(), PROFILE_3DENGINE );

      if(GetCVars()->e_VoxTerTraceMessages==2)
        pEng->PrintMessage("Out: eDeleteMesh");

      SDeleteMeshThreadMessage * pFinishedTask = (SDeleteMeshThreadMessage*)pMessage;
      CVoxTerrain::ReleaseIsoMeshFromMainThread(pFinishedTask->pMeshForDelete, pFinishedTask->pMeshForDelete->m_key);
      delete pFinishedTask;
    }
    else if(pMessage->GetType() == IIsoThreadMessage::eUpdateMeshesMap)
    {
      FRAME_PROFILER( "IsoOctreeThread::Update_eUpdateMeshesMap", GetSystem(), PROFILE_3DENGINE );

      if(GetCVars()->e_VoxTerTraceMessages==2)
        pEng->PrintMessage("Out: eUpdateMeshesMap");

      SUpdateMeshesMapThreadMessage * pFinishedTask = (SUpdateMeshesMapThreadMessage*)pMessage;
      
      {
        FRAME_PROFILER( "IsoOctreeThread::Update_eUpdateMeshesMap_Del", GetSystem(), PROFILE_3DENGINE );
        SAFE_DELETE(pRenderMeshes);
      }

      // copy render meshes map
      pRenderMeshes = pFinishedTask->pThreadMeshesCopy;

      {
        FRAME_PROFILER( "IsoOctreeThread::Update_eUpdateMeshesMap_update_empty", GetSystem(), PROFILE_3DENGINE );

        // update empty render meshes map
        for(TIsoMeshes::const_iterator iter = pFinishedTask->threadEmptyMeshesDelta.begin(); iter != pFinishedTask->threadEmptyMeshesDelta.end(); iter ++)
        {
          if(iter->second == pNotNULL)
            (*pRenderEmptyMeshes)[iter->first] = pNotNULL;
          else if(iter->second == NULL)
            pRenderEmptyMeshes->erase(iter->first);
          else
            assert(0);
        }
      }

      delete pFinishedTask;
    }
    else if(pMessage->GetType() == IIsoThreadMessage::eEnablePhysEntity)
    {
      FRAME_PROFILER( "IsoOctreeThread::Update_eEnablePhysEntity", GetSystem(), PROFILE_3DENGINE );

      if(GetCVars()->e_VoxTerTraceMessages==2)
        pEng->PrintMessage("Out: eEnablePhysEntity");

      SEnablePhysEntityThreadMessage * pFinishedTask = (SEnablePhysEntityThreadMessage*)pMessage;
      pFinishedTask->pIsoMesh->EnablePhysEntity(pFinishedTask->bEnable);
      delete pFinishedTask;
    }

  //  if( pEng->GetCurAsyncTimeSec() - fStartTime > 0.005f )
    //  break;
  }
}

void IsoOctree::UpdateTree()
{
  FUNCTION_PROFILER_3DENGINE;

//  PrintMessage("IsoOctree::UpdateTree: Start");

  AUTO_LOCK(g_cIsoNodesForUpdate);

  bool bMeshMapsUpdated = false;

  if(!GetCVars()->e_TerrainDrawThisSectorOnly)
  {
    FRAME_PROFILER( "IsoOctreeThread::UpdateTree_ActivateNodes", GetSystem(), PROFILE_3DENGINE );

    AUTO_LOCK(g_cIsoTreeAccess);

    OctNode::NodeIndex nIdx;
    AABB parentBoxWS = GetNodeAABB(nIdx, true);
    parentBoxWS.min *= 2;
    parentBoxWS.max *= 2;
    ActivateNodes(&tree, nIdx, parentBoxWS, GetTerrain()->GetParentNode(0));
    if(GetCVars()->e_VoxTerHeightmapEditing && threadEmptyMeshesDelta.size())
      bMeshMapsUpdated = true;

    if(GetCVars()->e_VoxTerActivateAll == 2)
    {
      if(ICVar * pVar = GetConsole()->GetCVar("e_VoxTerActivateAll"))
        pVar->ForceSet("1");
      GetCVars()->e_VoxTerActivateAll = 1;
    }
  }

  { // delete empty nodes
    FRAME_PROFILER( "IsoOctreeThread::UpdateTree_DelEmpty", GetSystem(), PROFILE_3DENGINE );

    AUTO_LOCK(g_cIsoMeshes);

    for(int t=0; t<256; t++)
    {
      static int i=0;
      i++;
      if(i>=m_nodesForUpdate.Count())
        i=0;

      if(!m_nodesForUpdate.Count())
        break;

      const SNodeUpdateRequest & info = m_nodesForUpdate[i];

      if(info.bInProgress)
        continue;

      if(threadEmptyMeshes.find(info.centerKey) != threadEmptyMeshes.end())
      {
        TIsoMeshes::iterator iter = threadMeshes.find(info.centerKey);

        if(iter != threadMeshes.end())
        {
          SIsoMesh * pIsoMesh = iter->second;
          assert(pIsoMesh->m_key == info.centerKey);
          CIsoOctreeThread::MSGO_DeleteIsoMesh(pIsoMesh);
          bMeshMapsUpdated = true;
        }
        else
        {
          SIsoMesh * pIsoMesh = new SIsoMesh(info.centerKey);
          CIsoOctreeThread::MSGO_DeleteIsoMesh(pIsoMesh);
          bMeshMapsUpdated = true;
        }

        m_nodesForUpdate.Delete(i);
        i--;
      }
    }
  }

  { // sort
    FRAME_PROFILER( "IsoOctreeThread::UpdateTree_SORT", GetSystem(), PROFILE_3DENGINE );

    Vec3 vFocusPosNorm = (GetCamera().GetPosition() + GetCamera().GetViewdir()*4.f) / CVoxTerrain::m_fMapSize;

    for(int i=0; i<m_nodesForUpdate.Count(); i++)
    {
      SNodeUpdateRequest & info = m_nodesForUpdate[i];
      info.UpdateSortVal_HQ(GetMainFrameID(),vFocusPosNorm,threadMeshes);
    }

    std::sort(m_nodesForUpdate.begin(), m_nodesForUpdate.end(), S_Cmp_OctNode());
  }

  int nMaxMeshesInMemory = GetCVars()->e_VoxTerMaxMeshesInMem;
  if(GetCVars()->e_VoxTerActivateAll)
    nMaxMeshesInMemory *= 16;

  {
    FRAME_PROFILER( "IsoOctreeThread::UpdateTree_DelIfTooMany", GetSystem(), PROFILE_3DENGINE );

    for(int i=nMaxMeshesInMemory; i<m_nodesForUpdate.Count(); i++)
    {
      const SNodeUpdateRequest & info = m_nodesForUpdate[i];

      TIsoMeshes::iterator iter = threadMeshes.find(info.centerKey);
      if( iter == threadMeshes.end() )
        continue;

      SIsoMesh * pIsoMesh = iter->second;
      assert(info.centerKey == pIsoMesh->m_key);
      CIsoOctreeThread::MSGO_DeleteIsoMesh(pIsoMesh);
      bMeshMapsUpdated = true;
    }

    if(m_nodesForUpdate.Count()>nMaxMeshesInMemory)
      m_nodesForUpdate.PreAllocate(nMaxMeshesInMemory,nMaxMeshesInMemory);
  }

  // unload not used meshes
  if((int)threadMeshes.size()>nMaxMeshesInMemory)
  {
    int nNodesToVisitNum = threadMeshes.size()/200;

    FRAME_PROFILER( "IsoOctreeThread::UpdateTree_DelOld", GetSystem(), PROFILE_3DENGINE );

    static ISO_KEY next_key = 0;

    TIsoMeshes::iterator iter;
    if(next_key && (threadMeshes.find(next_key) != threadMeshes.end()))
      iter = threadMeshes.find(next_key);
    else
      iter = threadMeshes.begin();

    TIsoMeshes::iterator next;

    for(int m=0; m<nNodesToVisitNum; m++)
    {
      next = iter;

      next++;
      if(next == threadMeshes.end())
        next = threadMeshes.begin();
      next_key = next->first;

      SIsoMesh * pIsoMesh = iter->second;

      if(pIsoMesh->m_nLastDrawFrameId < GetMainFrameID()-64)
      {
        bool bActive = false;
        for(int i=0; i<m_nodesForUpdate.Count(); i++)
        {
          const SNodeUpdateRequest & info = m_nodesForUpdate[i];
          if(info.centerKey == iter->first)
          {
            bActive = true;
            break;
          }
        }
      
        if(!bActive)
        {
          CIsoOctreeThread::MSGO_DeleteIsoMesh(pIsoMesh);
          bMeshMapsUpdated = true;
        }
      }

      iter = next;
    }
  }

  // Setup new tasks

  static bool bFirstTime = true;

  int nMaxTasksPerThread = bFirstTime ? 256 : 16;
  if(GetCVars()->e_VoxTerActivateAll)
    nMaxTasksPerThread *= 8;

  bFirstTime = false;

  bool bAllUpToDate = CIsoOctreeThread::m_nMessagesOut<64;

  int nMeakeMeshTasksInLine = 0;
  {
    AUTO_LOCK(g_cTaksIn);
    for(int i=0; i<CIsoOctreeThread::m_arrMessagesIn.Count(); i++)
      if(CIsoOctreeThread::m_arrMessagesIn[i]->GetType() == IIsoThreadMessage::eMakeMesh)
        nMeakeMeshTasksInLine++;
  }

  if(GetCVars()->e_VoxTerHeightmapEditing)
  {
    FRAME_PROFILER( "IsoOctreeThread::UpdateTree_MarkMixMasksMap2DAsUsed", GetSystem(), PROFILE_3DENGINE );

    for(int nItem=0; nItem<m_nodesForUpdate.Count(); nItem++)
    {
      SNodeUpdateRequest & info = m_nodesForUpdate[nItem];

      TIsoMeshes::iterator it = threadMeshes.find(info.centerKey);
      SIsoMesh * pIsoMesh = (it == threadMeshes.end()) ? NULL : it->second;

      if(pIsoMesh)
        pIsoMesh->MarkMixMasksMap2DAsUsed();
    }
  }

  for(int nItem=0; nItem<m_nodesForUpdate.Count() && CIsoOctreeThread::m_nMessagesOut<64; nItem++)
  {
    FRAME_PROFILER( "IsoOctreeThread::UpdateTree_SetupNewTask", GetSystem(), PROFILE_3DENGINE );

    if(IsoOctree::m_lastEditingTime > GetCurTimeSec() - gEditingRenderModeDelay/4.f)
    {
      if(nMeakeMeshTasksInLine >= GetCVars()->e_VoxTerThreads)
      {
        bAllUpToDate = false;
        break;
      }
    }
    else
    {
      if(nMeakeMeshTasksInLine >= GetCVars()->e_VoxTerThreads*nMaxTasksPerThread)
      {
        bAllUpToDate = false;
        break;
      }
    }

    SNodeUpdateRequest & info = m_nodesForUpdate[nItem];

    if(info.bInProgress)
    {
      bAllUpToDate = false;
      continue;
    }

    TIsoMeshes::iterator it = threadMeshes.find(info.centerKey);
    SIsoMesh * pIsoMesh = (it == threadMeshes.end()) ? NULL : it->second;

    if( !pIsoMesh || pIsoMesh->m_nRebuildRequestedFrameId || (!pIsoMesh->IsMeshCompiled() && !pIsoMesh->m_bInProgress) )
    {
      AUTO_LOCK(g_cIsoMeshes);

      if(threadEmptyMeshes.find(info.centerKey) != threadEmptyMeshes.end())
      {
        m_nodesForUpdate.Delete(nItem);
        nItem--;
        bMeshMapsUpdated = true;
        continue;
      }
    }
    else
      continue;

    info.bInProgress = true;

    info.nRequestFrameId = 60000000;

    if(pIsoMesh)
      pIsoMesh->m_nRebuildStartedFrameId = GetMainFrameID();

    SMakeMeshThreadMessage * pNewTask = new SMakeMeshThreadMessage;

    pNewTask->pNewMesh = NULL;
    pNewTask->nIdx = info.nIdx;

    pNewTask->pAreaIsoOctree = new IsoOctree(m_pValues);
    pNewTask->pAreaIsoOctree->nKey.set(maxDepth);
    pNewTask->pAreaIsoOctree->maxDepth = maxDepth;
    pNewTask->pAreaIsoOctree->editDepth = editDepth;
    pNewTask->key = info.centerKey;
    pNewTask->nItemId = nItem;

    // copy nodes required for triangulation

    float fBorder = 0.02f/CVoxTerrain::m_fMapSize;

    AABB aabbCutOut = GetNodeAABB(pNewTask->nIdx);
    aabbCutOut.min-=Vec3(fBorder,fBorder,fBorder);
    aabbCutOut.max+=Vec3(fBorder,fBorder,fBorder);

    pNewTask->aabbCutIn = GetNodeAABB(pNewTask->nIdx);
    pNewTask->aabbCutIn.min+=Vec3(fBorder,fBorder,fBorder);
    pNewTask->aabbCutIn.max-=Vec3(fBorder,fBorder,fBorder);
    pNewTask->pIsoMesh = pIsoMesh;

    OctNode::NodeIndex nIdxGlobal;

    {
      FRAME_PROFILER( "IsoOctreeThread::UpdateTree_CopyTree", GetSystem(), PROFILE_3DENGINE );

      AUTO_LOCK(g_cIsoTreeAccess);

      bool bOptLods = (m_flatnessMap.size() && pNewTask->nIdx.depth<nMeshDepth && GetCVars()->e_VoxTerOptimizeLods);

      pNewTask->pColValues = 0;//new PodArray<std::pair<ISO_KEY, SVoxValue> >;

      CopyTree(&pNewTask->aabbCutIn, aabbCutOut, pNewTask->nIdx.depth+GetCVars()->e_VoxTerMaxMeshBuildDepth, &tree, nIdxGlobal, &pNewTask->pAreaIsoOctree->tree, false, 
        ( bOptLods || pNewTask->pColValues ) ? this : NULL, 
        GetCVars()->e_VoxTerOptimizeLods, 
        pNewTask->pColValues);
    }

    CIsoOctreeThread::AddTaskIn(pNewTask);
    
    bMeshMapsUpdated = true;

    bAllUpToDate = false;

    nMeakeMeshTasksInLine++;
  }  

  if(bAllUpToDate)
    m_nProgress = m_nodesForUpdate.Count();
  else
    m_nProgress = min(m_nProgress, m_nodesForUpdate.Count()-1);

//  PrintMessage("IsoOctree::UpdateTree: Done");

  if(bMeshMapsUpdated)
    CIsoOctreeThread::MSGO_UpdateMeshesMap();
}

void IsoOctree::ShutDown()
{
  tree.deleteChildren(m_aabbDirty, m_aabbDirty);
  renderTree.deleteChildren(m_aabbDirty, m_aabbDirty);  
  
  CleanUpMeshes(true);

  ResetPhysics();
}

void IsoOctree::ResetPhysics()
{
  AUTO_LOCK(g_cPhysMeshes);

  for(std::map<int,SIsoMesh*>::const_iterator iter = physMeshes.begin(); iter != physMeshes.end(); iter ++)
    delete (SIsoMesh*)iter->second;
  physMeshes.clear();
}

void IsoOctree::ActivateNode(const OctNode::NodeIndex& nIdx, int nPriority)
{
  FUNCTION_PROFILER_3DENGINE;

  if(nIdx.depth>nMeshDepth)	
    return;

  if(nIdx.depth<nMeshDepth-GetCVars()->e_VoxTerMaxMeshLods)	
    return;

  ISO_KEY key = OctNode::CenterIndex( nIdx, maxDepth );

  {
    AUTO_LOCK(g_cIsoMeshes);

    if(threadEmptyMeshes.find(key) != threadEmptyMeshes.end())
      return;
  }

  for(int i=0; i<m_nodesForUpdate.Count(); i++)
  {
    if(m_nodesForUpdate[i].centerKey == key)
    {
      m_nodesForUpdate[i].nRequestFrameId = min( m_nodesForUpdate[i].nRequestFrameId, (int)GetMainFrameID());
      m_nodesForUpdate[i].nRequestPrioriry = nPriority;
      return;
    }
  }

  SNodeUpdateRequest info;
  info.nIdx = nIdx;
  info.nRequestFrameId = GetMainFrameID();
  info.nRequestPrioriry = nPriority;
  info.centerKey = key;
  info.bInProgress = false;
  m_nodesForUpdate.Add(info);
}

bool IsoOctree::GetTrianglesInSphere(OctNode* node, const OctNode::NodeIndex& nIdx, const Sphere & sp, 
                                   PodArray<Vec3> & lstVerts, PodArray<Vec3> & lstNorms, PodArray<uint16> & lstIndices)
{
  FUNCTION_PROFILER_3DENGINE;

  ISO_KEY key;
  float hw;
  Vec3 center,n,p;

  OctNode::CenterAndWidth(nIdx,center,hw);
  hw*=CVoxTerrain::m_fMapSize*0.5f;
  center*=CVoxTerrain::m_fMapSize;
  AABB WSBBox(Vec3(center.x-hw,center.y-hw,center.z-hw),Vec3(center.x+hw,center.y+hw,center.z+hw));

  bool bSuccess = true;

  if(!Overlap::Sphere_AABB(sp,WSBBox))
    return bSuccess;

  if(!GetCamera().IsAABBVisible_F(WSBBox))
    return bSuccess;

  float fDistance = sqrt(Distance::Point_AABBSq(GetCamera().GetPosition(), WSBBox));

  bool bDraw = (fDistance*GetCVars()->e_VoxTerViewDistRatio>hw || !node->children || nIdx.depth >= nMeshDepth);

  if(!bDraw)
  {
    for(int i=0;i<Cube::CORNERS;i++)
    {
      if(!GetTrianglesInSphere(&node->children[i], nIdx.child(i), sp, lstVerts, lstNorms, lstIndices))
        bDraw = true;
    }
  }

  if(bDraw)
  {
    key=OctNode::CenterIndex(nIdx,maxDepth);

    TIsoMeshes & renderMeshes = *CIsoOctreeThread::pRenderMeshes;
    TIsoMeshes & renderEmptyMeshes = *CIsoOctreeThread::pRenderEmptyMeshes;

    if( renderMeshes.find(key) != renderMeshes.end() )
    {
      renderMeshes[key]->GetTrianglesInSphere(sp, lstVerts, lstNorms, lstIndices);
    }
    else if(renderEmptyMeshes.find(key) == renderEmptyMeshes.end())
    {
      bSuccess = false;
    }
  }

  return bSuccess;
}

bool IsoOctree::GetTrianglesInAABB(OctNode* node, const OctNode::NodeIndex& nIdx, const AABB & aabb, 
                                   PodArray<Vec3> & lstVerts, PodArray<Vec3> & lstNorms, PodArray<uint16> & lstIndices)
{
  FUNCTION_PROFILER_3DENGINE;

  ISO_KEY key;
  float hw;
  Vec3 center,n,p;

  OctNode::CenterAndWidth(nIdx,center,hw);
  hw*=CVoxTerrain::m_fMapSize*0.5f;
  center*=CVoxTerrain::m_fMapSize;
  AABB WSBBox(Vec3(center.x-hw,center.y-hw,center.z-hw),Vec3(center.x+hw,center.y+hw,center.z+hw));

  bool bSuccess = true;

  if(!Overlap::AABB_AABB(aabb,WSBBox))
    return bSuccess;

  if(!GetCamera().IsAABBVisible_F(WSBBox))
    return bSuccess;

  float fDistance = sqrt(Distance::Point_AABBSq(GetCamera().GetPosition(), WSBBox));

  bool bDraw = (fDistance*GetCVars()->e_VoxTerViewDistRatio>hw || !node->children || nIdx.depth >= nMeshDepth);

  if(!bDraw)
  {
    for(int i=0;i<Cube::CORNERS;i++)
    {
      if(!GetTrianglesInAABB(&node->children[i], nIdx.child(i), aabb, lstVerts, lstNorms, lstIndices))
        bDraw = true;
    }
  }

  if(bDraw)
  {
    key=OctNode::CenterIndex(nIdx,maxDepth);

    TIsoMeshes & renderMeshes = *CIsoOctreeThread::pRenderMeshes;
    TIsoMeshes & renderEmptyMeshes = *CIsoOctreeThread::pRenderEmptyMeshes;

    if( renderMeshes.find(key) != renderMeshes.end() )
    {
      renderMeshes[key]->GetTrianglesInAABB(aabb, lstVerts, lstNorms, lstIndices);
    }
    else if(renderEmptyMeshes.find(key) == renderEmptyMeshes.end())
    {
      bSuccess = false;
    }
  }

  return bSuccess;
}


bool IsoOctree::TestGetElevationChilds(OctNode* node, const OctNode::NodeIndex& nIdx, const Ray & ray)
{
  FUNCTION_PROFILER_3DENGINE;

  ISO_KEY key;
  float hw;
  Vec3 center,n,p;

  OctNode::CenterAndWidth(nIdx,center,hw);
  hw*=CVoxTerrain::m_fMapSize*0.5f;
  center*=CVoxTerrain::m_fMapSize;
  AABB WSBBox(Vec3(center.x-hw,center.y-hw,center.z-hw),Vec3(center.x+hw,center.y+hw,center.z+hw));

  bool bSuccess = true;

  Vec3 vOut;
  if(!Intersect::Ray_AABB(ray,WSBBox,vOut))
    return bSuccess;

  float fDistance = sqrtf(Distance::Point_AABBSq(GetCamera().GetPosition(), WSBBox));

  bool bDraw = (fDistance*GetCVars()->e_VoxTerViewDistRatio>hw || !node->children || nIdx.depth >= nMeshDepth);

  if(bDraw)
  {
    key=OctNode::CenterIndex(nIdx,maxDepth);

    TIsoMeshes & renderMeshes = *CIsoOctreeThread::pRenderMeshes;
    TIsoMeshes & renderEmptyMeshes = *CIsoOctreeThread::pRenderEmptyMeshes;

    TIsoMeshes::iterator iter = renderMeshes.find(key);

    if( iter != renderMeshes.end() )
    {
      if(!iter->second->IsMeshCompiled() && (renderEmptyMeshes.find(key) == renderEmptyMeshes.end()))
        bSuccess = false;
    }
    else if(renderEmptyMeshes.find(key) == renderEmptyMeshes.end())
    {
      bSuccess = false;
    }
  }

  return bSuccess;
}

void IsoOctree::RegisterBBoxInPODGrid(OctNode* node, const OctNode::NodeIndex& nIdx)
{
  if(!node->children || nIdx.depth >= nMeshDepth)
  {
    AABB WSBBox = GetNodeAABB(nIdx);
    WSBBox.min *= CVoxTerrain::m_fMapSize;
    WSBBox.max *= CVoxTerrain::m_fMapSize;
    gEnv->pPhysicalWorld->RegisterBBoxInPODGrid(&WSBBox.min);
  }
  else
  {
    for(int i=0;i<Cube::CORNERS;i++)
      RegisterBBoxInPODGrid(&node->children[i],nIdx.child(i));
  }
}

void IsoOctree::ActivateNodes(OctNode* node, const OctNode::NodeIndex& nIdx, AABB parentBoxWS, CTerrainNode * pTerrainNode)
{
  AABB nodeBoxWS = GetNodeAABB(nIdx, true);
  float hw = nodeBoxWS.GetSize().x/2;
  float fDistance = sqrtf(Distance::Point_AABBSq(GetCamera().GetPosition(), nodeBoxWS));
  float fParentDistance = sqrtf(Distance::Point_AABBSq(GetCamera().GetPosition(), parentBoxWS));

  bool bMeshIsVisible = (fParentDistance*GetCVars()->e_VoxTerViewDistRatio <= hw*2) 
    && (fDistance*GetCVars()->e_VoxTerViewDistRatio >= hw || !node->children || nIdx.depth >= nMeshDepth || !CVoxTerrain::IsEditing());

  bool bInFrustum = GetCamera().IsAABBVisible_F(nodeBoxWS);

  if(!GetCVars()->e_VoxTerHeightmapEditing || (pTerrainNode && Overlap::AABB_AABB(nodeBoxWS, pTerrainNode->GetBBox())))
  {
    if(bMeshIsVisible && bInFrustum)// && CVoxTerrain::IsEditing())
    {
      /*uint32 nPriorityPlus = 0;

      ISO_KEY key = OctNode::CenterIndex(nIdx,maxDepth);
      TIsoMeshes::iterator iter = threadMeshes.find(key);
      if(iter != threadMeshes.end())
      {
        SIsoMesh * pIsoMesh = iter->second;

        if(((int)pIsoMesh->m_nLastDrawFrameId) >= GetMainFrameID()-50)
        {
          nPriorityPlus += 4;
  //        if(pIsoMesh->m_nRebuildRequestedFrameId)
    //        nPriorityPlus += min(1000u, GetMainFrameID() - (uint32) pIsoMesh->m_nRebuildRequestedFrameId);
        }
      }*/

      ActivateNode(nIdx, 4);
    }
    else //if(/*!GetCVars()->e_VoxTerTexBuildOnCPU && */!CVoxTerrain::IsEditing())
    {
      if(bInFrustum)
        ActivateNode(nIdx, 2);
//#ifndef _DEBUG
      else
        ActivateNode(nIdx, 0);
//#endif
    }
  }
  else
  {
    ISO_KEY key = OctNode::CenterIndex( nIdx, maxDepth );
    TIsoMeshes::iterator iter = threadEmptyMeshes.find(key);
    if( iter == threadEmptyMeshes.end() )
    {
      threadEmptyMeshes[key] = NULL;
      threadEmptyMeshesDelta[key] = pNotNULL;
    }
  }

  if(!IsVisibleFromGamePlayArea(nodeBoxWS, GetCVars()->e_VoxTerViewDistRatioMeshActivate))
    return;

  // build child nodes if necessary
  if(GetCVars()->e_VoxTerHeightmapEditing && !node->children)
    if(nIdx.depth < nMeshDepth)
      if((fDistance - hw/16 - fPredictionDistance)*GetCVars()->e_VoxTerViewDistRatio < hw || GetCVars()->e_VoxTerActivateAll==2)
        node->initChildren();

  if((GetCVars()->e_VoxTerActivateAll==2 || (fDistance - hw/16 - fPredictionDistance)*GetCVars()->e_VoxTerViewDistRatio < hw) && node->children && nIdx.depth < nMeshDepth)
  {
    for(int i=0;i<Cube::CORNERS;i++)
    {
      CTerrainNode * pChildTerrainNode = (pTerrainNode && pTerrainNode->m_arrChilds[i&3]) ? pTerrainNode->m_arrChilds[i&3] : pTerrainNode;

      ActivateNodes(&node->children[i],nIdx.child(i), nodeBoxWS, pChildTerrainNode);
    }
  }
}
/*
bool IsoOctree::ActivatePhysics(OctNode* node, const OctNode::NodeIndex& nIdx, const AABB & areaBox)
{
  // get node box
  AABB nodeBox = GetNodeAABB(nIdx);

  bool bLeafMeshNode = !node->children || nIdx.depth >= (nMeshDepth-1);

  ISO_KEY key = OctNode::CenterIndex(nIdx,maxDepth);

  TIsoMeshes::iterator iter = isoMeshes.find(key);

  SIsoMesh * pIsoMesh = 0;
  if( iter != isoMeshes.end() )
    pIsoMesh = iter->second;

  if(pIsoMesh && Overlap::AABB_AABB(areaBox,nodeBox) && pIsoMesh->m_pPhysGeom)
  {
    bool bGoodChildFound;

    if(bLeafMeshNode)
      bGoodChildFound = false;
    else
    {
      bGoodChildFound = true;
      for(int i=0;i<Cube::CORNERS;i++)
        if(!ActivatePhysics(&node->children[i],nIdx.child(i), areaBox))
          bGoodChildFound = false;
    }

    if(!bGoodChildFound)
    { // we are in good node
      pIsoMesh->EnablePhysEntity(true);
      pIsoMesh->m_nLastPhysFrameId = GetMainFrameID();
      RegisterPotentialyVisibleNode(node,nIdx,16);
    }
  }
  else
  {
    if(Overlap::AABB_AABB(areaBox,nodeBox))
      return false;
    else
      return true;
  }

  return true;
}*/

bool IsoOctree::Render(OctNode* node, const OctNode::NodeIndex& nIdx)
{
  FUNCTION_PROFILER_3DENGINE;

  ISO_KEY key;
  float hw;
  Vec3 center,n,p;

  OctNode::CenterAndWidth(nIdx,center,hw);
  hw*=CVoxTerrain::m_fMapSize*0.5f;
  center*=CVoxTerrain::m_fMapSize;
  AABB WSBBox(Vec3(center.x-hw,center.y-hw,center.z-hw),Vec3(center.x+hw,center.y+hw,center.z+hw));

  bool bSuccess = true;

  if(!GetCamera().IsAABBVisible_F(WSBBox))
    return bSuccess;

  float fDistance = sqrtf(Distance::Point_AABBSq(GetCamera().GetPosition(), WSBBox));

  bool bDraw = (fDistance*GetCVars()->e_VoxTerViewDistRatio>hw || !node->children || nIdx.depth >= nMeshDepth);

  if(!bDraw && !IsVisibleFromGamePlayArea(WSBBox, GetCVars()->e_VoxTerViewDistRatioMeshActivate))
    bDraw = true;

  if(/*!m_bEditor && */!bDraw && !CVoxTerrain::IsEditing() && (nIdx.depth>=nMeshDepth-GetCVars()->e_VoxTerMaxMeshLods) && !GetCVars()->e_TerrainDrawThisSectorOnly)
  {
    bool bChHasMeshes = true;
    if(node->children)
    {
      TIsoMeshes & renderMeshes = *CIsoOctreeThread::pRenderMeshes;
      TIsoMeshes & renderEmptyMeshes = *CIsoOctreeThread::pRenderEmptyMeshes;

      for(int i=0; i<Cube::CORNERS; i++)
      {
        ISO_KEY keyChild = OctNode::CenterIndex(nIdx.child(i),maxDepth);
        AABB childBoxWS = GetNodeAABB(nIdx.child(i), true);
        if(GetCamera().IsAABBVisible_F(childBoxWS))
        {
          if( renderMeshes.find(keyChild) == renderMeshes.end() && renderEmptyMeshes.find(keyChild) == renderEmptyMeshes.end() )
          {
            bChHasMeshes = false;
            break;
          }
        }
      }
    }
    if(!bChHasMeshes)
      bDraw = true;
  }

  if(m_bEditor && !node->children)
  {
    bool bDrawWithoutChildsCheck = (fDistance*GetCVars()->e_VoxTerViewDistRatio>hw || nIdx.depth >= nMeshDepth);
    if(!bDrawWithoutChildsCheck)
      node->initChildren();
  }

  if(!bDraw)
  {
    Vec3 vCamPos = GetCamera().GetPosition();

    Vec3 vNodeCenter = WSBBox.GetCenter();

    int nFirst = 
      ((vCamPos.x > vNodeCenter.x) ? 1 : 0) |
      ((vCamPos.y > vNodeCenter.y) ? 2 : 0) |
      ((vCamPos.z > vNodeCenter.z) ? 4 : 0);

    if(!Render(&node->children[nFirst  ],nIdx.child(nFirst  )))
      bDraw = true;

    if(!Render(&node->children[nFirst^1],nIdx.child(nFirst^1)))
      bDraw = true;

    if(!Render(&node->children[nFirst^2],nIdx.child(nFirst^2)))
      bDraw = true;

    if(!Render(&node->children[nFirst^4],nIdx.child(nFirst^4)))
      bDraw = true;

    if(!Render(&node->children[nFirst^3],nIdx.child(nFirst^3)))
      bDraw = true;

    if(!Render(&node->children[nFirst^5],nIdx.child(nFirst^5)))
      bDraw = true;

    if(!Render(&node->children[nFirst^6],nIdx.child(nFirst^6)))
      bDraw = true;

    if(!Render(&node->children[nFirst^7],nIdx.child(nFirst^7)))
      bDraw = true;
  }

  if(bDraw)
  {
    key = OctNode::CenterIndex(nIdx,maxDepth);

    TIsoMeshes & renderMeshes = *CIsoOctreeThread::pRenderMeshes;
    TIsoMeshes & renderEmptyMeshes = *CIsoOctreeThread::pRenderEmptyMeshes;

    if( renderMeshes.find(key) != renderMeshes.end() )
    {
      SRendParams rendParams;
      rendParams.fDistance = fDistance;
      rendParams.nDLightMask = 1;
      SIsoMesh * pIsoMesh = renderMeshes[key];
      pIsoMesh->m_WSBBox = WSBBox;
/*
      pIsoMesh->m_nTexDimScale = 0;
      if(!node->children)
        pIsoMesh->m_nTexDimScale = nMeshDepth-nIdx.depth;
*/
      Get3DEngine()->CheckCreateRNTmpData(&pIsoMesh->m_pRNTmpData, pIsoMesh);

			if( GetObjManager()->IsBoxOccluded(WSBBox, fDistance, &pIsoMesh->m_pRNTmpData->userData.m_OcclState, false, eoot_OBJECT) )
				return bSuccess;

      if(pIsoMesh)// && pIsoMesh == (SIsoMesh*)0x229091c8)
      {
        if(!m_bEditor && (pIsoMesh->m_eStreamingStatus == ecss_NotLoaded) && GetCVars()->e_VoxTerViewDistRatioPrediction>=0)
          pIsoMesh->StartStreaming( Get3DEngine()->IsStatObjSyncLoad() || (WSBBox.GetSize().x == fPhysMeshSize && Overlap::Point_AABB2D(GetCamera().GetPosition(),WSBBox)), 0 );

        if(m_bEditor || (pIsoMesh->m_eStreamingStatus == ecss_Ready))
          pIsoMesh->Render(rendParams);
        else if(!GetCVars()->e_TerrainDrawThisSectorOnly)
          bSuccess = false;
      }

      if((!pIsoMesh->IsMeshCompiled() || !pIsoMesh->m_bRenderSuccess) && (m_bEditor && renderEmptyMeshes.find(key) == renderEmptyMeshes.end()) && (nIdx.depth > nMeshDepth-GetCVars()->e_VoxTerMaxMeshLods))
      {
        if(GetCVars()->e_VoxTer == 5)
          DrawBBox(WSBBox, Col_Blue);

//        if(!CVoxTerrain::IsEditing() && !GetCVars()->e_TerrainDrawThisSectorOnly)
  //        bSuccess = false;
      }
    }
    else if( m_bEditor && renderEmptyMeshes.find(key) == renderEmptyMeshes.end() && (nIdx.depth > nMeshDepth-GetCVars()->e_VoxTerMaxMeshLods))
    {
      if(GetCVars()->e_VoxTer == 5)
        DrawBBox(WSBBox, Col_Red);

    //  if(!CVoxTerrain::IsEditing() && !GetCVars()->e_TerrainDrawThisSectorOnly)
      //  bSuccess = false;
    }
  }

  return bSuccess;
}

void IsoOctree::ActivateStreaming(OctNode* node, const OctNode::NodeIndex& nIdx, const AABB & parentBoxWS)
{
  AABB nodeBoxWS = GetNodeAABB(nIdx, true);
  float hw = nodeBoxWS.GetSize().x/2;
  float fDistance = sqrtf(Distance::Point_AABBSq(GetCamera().GetPosition(), nodeBoxWS));

  TIsoMeshes::iterator iter = CIsoOctreeThread::pRenderMeshes->find(OctNode::CenterIndex(nIdx,maxDepth));
  if( iter != CIsoOctreeThread::pRenderMeshes->end() )
  {
    if(SIsoMesh * pIsoMesh = iter->second)
    {
      pIsoMesh->m_nLastDrawFrameId = GetMainFrameID();
			pIsoMesh->m_fLastDrawTime = GetCurTimeSec();

      if(pIsoMesh->m_eStreamingStatus == ecss_NotLoaded)
      {
        float fTime = 0;

        float fParentDistance = sqrtf(Distance::Point_AABBSq(GetCamera().GetPosition(), parentBoxWS));

        bool bInFrustum = GetCamera().IsAABBVisible_F(nodeBoxWS);

        bool bMeshIsVisible = (fParentDistance*GetCVars()->e_VoxTerViewDistRatio <= hw*2) 
          && (fDistance*GetCVars()->e_VoxTerViewDistRatio >= hw || !node->children || nIdx.depth >= nMeshDepth);

        if(bMeshIsVisible && bInFrustum)
          fTime = 0;
        else if(bInFrustum)
          fTime = 2;
        else
          fTime = 4;

        pIsoMesh->StartStreaming( Get3DEngine()->IsStatObjSyncLoad() || (nodeBoxWS.GetSize().x == fPhysMeshSize && Overlap::Point_AABB2D(GetCamera().GetPosition(),nodeBoxWS)), fTime );
      }
    }
  }

  if(node->children && nIdx.depth < (nMeshDepth-2))
    if((fDistance/* - hw/10*/)*GetCVars()->e_VoxTerViewDistRatio*fabs(GetCVars()->e_VoxTerViewDistRatioPrediction) < hw)// || nodeBoxWS.GetSize().x > fPhysMeshSize)
  {
    for(int i=0;i<Cube::CORNERS;i++)
      ActivateStreaming(&node->children[i], nIdx.child(i), nodeBoxWS);
  }
}

void IsoOctree::RequestTextureUpdate(OctNode* node, const OctNode::NodeIndex& nIdx, const AABB & areaBox)
{
  FUNCTION_PROFILER_3DENGINE;

  ISO_KEY key;
  float hw;
  Vec3 center,n,p;

  OctNode::CenterAndWidth(nIdx,center,hw);
  hw*=CVoxTerrain::m_fMapSize*0.5f;
  center*=CVoxTerrain::m_fMapSize;
  AABB WSBBox(Vec3(center.x-hw,center.y-hw,center.z-hw),Vec3(center.x+hw,center.y+hw,center.z+hw));

  if(!Overlap::AABB_AABB(areaBox,WSBBox))
    return;

  bool bDraw = !node->children || nIdx.depth >= nMeshDepth;

  key = OctNode::CenterIndex(nIdx,maxDepth);

  TIsoMeshes & renderMeshes = *CIsoOctreeThread::pRenderMeshes;

  if( renderMeshes.find(key) != renderMeshes.end() )
  {
    SIsoMesh * pIsoMesh = renderMeshes[key];
    pIsoMesh->ReleaseTextures(true);
  }

  if(!bDraw)
  {
    for(int i=0;i<Cube::CORNERS;i++)
      RequestTextureUpdate(&node->children[i], nIdx.child(i), areaBox);
  }
}

bool IsAABBInsideHull(SPlaneObject * pHullPlanes, int nPlanesNum, const AABB & aabbBox);

bool IsoOctree::FillShadowCastersList(OctNode* node, const OctNode::NodeIndex& nIdx,
                                      bool bAllIn, CDLight * pLight, ShadowMapFrustum * pFr, PodArray<SPlaneObject> * pShadowHull, bool bUseFrustumTest)
{
  FUNCTION_PROFILER_3DENGINE;

  AABB WSBBox = GetNodeAABB(nIdx,true);

  bool bSuccess = true;

  if(pFr->bForSubSurfScattering)
    return bSuccess;

  if(bUseFrustumTest && !bAllIn && !pFr->IntersectAABB(WSBBox, &bAllIn))
    return bSuccess;

  if(pShadowHull && !pFr->bUseAdditiveBlending && !IsAABBInsideHull(pShadowHull->GetElements(), pShadowHull->Count(), WSBBox))
    return bSuccess;

  assert(pFr->pLightOwner == pLight->m_pOwner);

  float fGSMBoxDiameter = 2.0f / (pFr->fFrustrumSize * GetCVars()->e_GsmRange);

  float fNodeHalfSize = (WSBBox.max.x-WSBBox.min.x)*0.5f;
  bool bDraw = ( !node->children 
    || fGSMBoxDiameter*.5f > fNodeHalfSize*2 
    || nIdx.depth >= nMeshDepth 
    || sqrtf(Distance::Point_AABBSq(GetCamera().GetPosition(), WSBBox))*GetCVars()->e_VoxTerViewDistRatio > fNodeHalfSize
    );

  if(!bDraw && !IsVisibleFromGamePlayArea(WSBBox, GetCVars()->e_VoxTerViewDistRatioMeshActivate))
    bDraw = true;

  if(/*!m_bEditor && */!bDraw && !CVoxTerrain::IsEditing() && (nIdx.depth>=nMeshDepth-GetCVars()->e_VoxTerMaxMeshLods))
  {
    bool bChHasMeshes = true;
    if(node->children)
    {
      TIsoMeshes & renderMeshes = *CIsoOctreeThread::pRenderMeshes;
      TIsoMeshes & renderEmptyMeshes = *CIsoOctreeThread::pRenderEmptyMeshes;

      for(int i=0; i<Cube::CORNERS; i++)
      {
        ISO_KEY keyChild = OctNode::CenterIndex(nIdx.child(i),maxDepth);
        AABB childBoxWS = GetNodeAABB(nIdx.child(i), true);
        if(GetCamera().IsAABBVisible_F(childBoxWS))
        {
          if( renderMeshes.find(keyChild) == renderMeshes.end() && renderEmptyMeshes.find(keyChild) == renderEmptyMeshes.end() )
          {
            bChHasMeshes = false;
            break;
          }
        }
      }
    }
    if(!bChHasMeshes)
      bDraw = true;
  }

  if(!bDraw)
  {
    for(int i=0;i<Cube::CORNERS;i++)
      if(!FillShadowCastersList(&node->children[i], nIdx.child(i), bAllIn, pLight, pFr, pShadowHull, bUseFrustumTest))
        bDraw = true;
  }

  if(bDraw)
  {
    ISO_KEY key=OctNode::CenterIndex(nIdx,maxDepth);

    TIsoMeshes & renderMeshes = *CIsoOctreeThread::pRenderMeshes;
    TIsoMeshes & renderEmptyMeshes = *CIsoOctreeThread::pRenderEmptyMeshes;

    if( renderMeshes.find(key) != renderMeshes.end() )
    {
      SIsoMesh * pIsoMesh = renderMeshes[key];
      if(pIsoMesh->m_pRenderMesh)
      {
        pIsoMesh->m_WSBBox = WSBBox;
        
        if(!m_bEditor && (pIsoMesh->m_eStreamingStatus == ecss_NotLoaded) && GetCVars()->e_VoxTerViewDistRatioPrediction>=0)
          pIsoMesh->StartStreaming( false, 10.f );

        if(m_bEditor || (pIsoMesh->m_eStreamingStatus == ecss_Ready))
          pFr->pCastersList->Add(pIsoMesh);
        else
          bSuccess = false;

        if(!CVoxTerrain::IsEditing())
        {
          pIsoMesh->m_nLastDrawFrameId = GetMainFrameID();
          pIsoMesh->m_fLastDrawTime = GetCurTimeSec();
        }
      }
      else if(m_bEditor && renderEmptyMeshes.find(key) == renderEmptyMeshes.end())
        bSuccess = false;
    }
    else if(m_bEditor && renderEmptyMeshes.find(key) == renderEmptyMeshes.end())
    {
//      renderMeshes[key] = new SIsoMesh;
      bSuccess = false;
    }
  }

  return bSuccess;
}

/*
void IsoOctree::InvalidateRenderMeshes(const AABB & modifiedArea)
{
OctNode::NodeIndex nIdx;

InvalidateRenderMeshes(&tree,nIdx, modifiedArea);
}

void IsoOctree::InvalidateRenderMeshes(OctNode* node, const OctNode::NodeIndex& nIdx, const AABB & modifiedArea)
{
FUNCTION_PROFILER_3DENGINE;

ISO_KEY key;
float hw;
Vec3 center,n,p;

float fDistance = 0;

OctNode::CenterAndWidth(nIdx,center,hw);
hw*=fMapSize*0.5;
center*=fMapSize;
AABB WSBBox(Vec3(center.x-hw,center.y-hw,center.z-hw),Vec3(center.x+hw,center.y+hw,center.z+hw));

if(!Overlap::AABB_AABB(modifiedArea, WSBBox))
return;

key = OctNode::CenterIndex( nIdx, maxDepth );
SIsoMesh * pIsoMesh = NULL;
if( isoMeshes.find(key) != isoMeshes.end() )
{
pIsoMesh = isoMeshes[key];
SAFE_DELETE(pIsoMesh); 
isoMeshes.erase(key);
}

if(nIdx.depth == nMeshDepth || !node->children)
{
return;
}
else 
{
for(int i=0;i<Cube::CORNERS;i++)
{
InvalidateRenderMeshes(&node->children[i],nIdx.child(i), modifiedArea);
}
}
}
*/

void IsoOctree::CutChilds(OctNode* node, const OctNode::NodeIndex& nIdx, std::map<OctNode*,OctNode*> & detachedChilds, int nMaxDepth)
{
  if(!node->children)
    return;

  if(nIdx.depth >= nMaxDepth)
  {
    detachedChilds[node] = node->children;
    node->children = NULL;
  }
  else
  {
    for(int i=0;i<Cube::CORNERS;i++)
    {
      CutChilds(&node->children[i], nIdx.child(i), detachedChilds, nMaxDepth);
    }
  }
}


AABB IsoOctree::GetNodeAABB(const OctNode::NodeIndex& nIdx, bool bWS)
{
  Vec3 ctr; float w;
  OctNode::CenterAndWidth(nIdx,ctr,w);
  Vec3 hw(w*.5f,w*.5f,w*.5f);
  AABB nodeBox(ctr-hw,ctr+hw);
  if(bWS)
  {
    nodeBox.min *= CVoxTerrain::m_fMapSize;
    nodeBox.max *= CVoxTerrain::m_fMapSize;
  }
  return nodeBox;
}

void IsoOctree::CollectValues(OctNode* node, OctNode::NodeIndex nIdx, TVoxValues & outValues, const AABB * pAreaBox)
{
  AABB nodeBox(GetNodeAABB(nIdx));

  if(!Overlap::AABB_AABB(nodeBox,*pAreaBox))
    return;

  if(node->children)
  {
    for(int i=0;i<Cube::CORNERS;i++)
      CollectValues(&node->children[i], nIdx.child(i), outValues, pAreaBox);
  }
  else
  {
    for(int i=0;i<Cube::CORNERS;i++)
    {
      ISO_KEY key=OctNode::CornerIndex(nIdx,i,maxDepth);
      TVoxValues::iterator iter = (*m_pValues).find(key);
      if(iter != (*m_pValues).end())
        outValues[key] = iter->second;
    }
  }
}

void IsoOctree::CutOutsideOfBox(const AABB & aabbCut, OctNode* node, const OctNode::NodeIndex& nIdx, std::map<OctNode*,OctNode*> & detachedChilds)
{
  if(!node->children)
    return;

  AABB nodeBox(GetNodeAABB(nIdx));

  if(!Overlap::AABB_AABB(nodeBox,aabbCut))
  {
    detachedChilds[node] = node->children;
    node->children = NULL;
  }
  else
  {
    for(int i=0;i<Cube::CORNERS;i++)
      CutOutsideOfBox(aabbCut, &node->children[i], nIdx.child(i), detachedChilds);
  }
}

void IsoOctree::UpdateNormals(OctNode* node, const OctNode::NodeIndex& nIdx, TFlatness & flatnessMap)
{
  if(node->children)
  {
    if(nIdx.depth<=nMeshDepth)
      if(PrintProgress())
        return;

    for(int c=0;c<Cube::CORNERS;c++)
      UpdateNormals(&node->children[c], nIdx.child(c), flatnessMap);
  }
  else
  {
    float values[2][2][2];

    for(int c=0;c<Cube::CORNERS;c++)
    {
      int x,y,z;
      Cube::FactorCornerIndex(c,x,y,z);

      TVoxValues::iterator iter = (*m_pValues).find(OctNode::CornerIndex(nIdx,c,maxDepth));

      if(iter!=(*m_pValues).end())
        values[x][y][z] = iter->second.val;
      else
        values[x][y][z] = 0;
    }

    Vec3 vNormal(0,0,0);
    for(int z=0; z<=1; z++)
    {
      vNormal += (Vec3(0.f,0.f,z) - Vec3(0.5f,0.5f,0.5f)) * values[0][0][z];
      vNormal += (Vec3(1.f,0.f,z) - Vec3(0.5f,0.5f,0.5f)) * values[1][0][z];
      vNormal += (Vec3(0.f,1.f,z) - Vec3(0.5f,0.5f,0.5f)) * values[0][1][z];
      vNormal += (Vec3(1.f,1.f,z) - Vec3(0.5f,0.5f,0.5f)) * values[1][1][z];
    }
    vNormal.Normalize();

    for(int c=0;c<Cube::CORNERS;c++)
    {
      int x,y,z;
      Cube::FactorCornerIndex(c,x,y,z);
      flatnessMap[OctNode::CornerIndex(nIdx,c,maxDepth)].first += vNormal;
    }
  }
}

int IsoOctree::ReconstructTree(int nMaxDepth, OctNode* node, const OctNode::NodeIndex& nIdx)
{
  int nNodesNum = 1;

  node->nodeData.mcIndex = 0;

  {
    TVoxValues::iterator iter = m_pValues->find(OctNode::CenterIndex(nIdx,maxDepth));
    if(iter != (*m_pValues).end())
    {
      node->initChildren();
      for(int i=0; i<Cube::CORNERS; i++)
      {
        nNodesNum += ReconstructTree(nMaxDepth, &node->children[i], nIdx.child(i));
        node->children[i].parent = node;
      }
    }
  }
/*
  SVoxValue childVals[8];

  if(node->children)
  { // take childs
    for(int i=0; i<8; i++)
      childVals[i] = (node->children[i].voxValue);
  }
  else
  { // take real values
    SVoxValue voxValAverage;
    for(int c=0;c<Cube::CORNERS;c++)
    {
      TVoxValues::iterator iter = (*m_pValues).find(OctNode::CornerIndex(nIdx,c,maxDepth));
      if(iter==(*m_pValues).end())
        childVals[c] = SVoxValue();
      else
        childVals[c] = iter->second;
    }
  }

  node->voxValue = SVoxValue::Average(&childVals[0], 8);
*/
  return nNodesNum;
}

void IsoOctree::CopyTree(const AABB * pAabbCutIn, const AABB & aabbCutOut, int nMaxDepthToCopy, OctNode* srcNode, const OctNode::NodeIndex& nIdx, OctNode* dstNode, bool bCopyOnlyIfMeshExist, const IsoOctree * pIsoTree, float fLodCurvature, PodArray<std::pair<ISO_KEY, SVoxValue> > * pColValues)
{
  AABB nodeBox(GetNodeAABB(nIdx));

  dstNode->nodeData.mcIndex = 0;//srcNode->nodeData.mcIndex;

  if(pColValues)
  {
    for(int c=0;c<Cube::CORNERS;c++)
    {
      std::pair<ISO_KEY, SVoxValue> pair;
      pair.first = OctNode::CornerIndex(nIdx,c,maxDepth);
      TVoxValues::iterator iter = (*pIsoTree->m_pValues).find(pair.first);
      if( iter != (*pIsoTree->m_pValues).end() )
      {
        pair.second = iter->second;
        pColValues->Add(pair);
      }
    }
  }

  if(!srcNode->children)
    return; // nothing more to copy

  if(nIdx.depth >= nMaxDepthToCopy && nodeBox.min.z <= 0)
      return;

  if(nIdx.depth >= nMaxDepthToCopy+GetCVars()->e_VoxTerBorderLodPlus)
    return; // +1 is needed for compatibility with neighbors

  if(nIdx.depth >= nMeshDepth+GetCVars()->e_VoxTerMaxMeshBuildDepthLimit)
    return;

  if(!Overlap::AABB_AABB(nodeBox,aabbCutOut))
    return; // cut if outside of aabbCutOut

  if(nIdx.depth >= nMaxDepthToCopy) // cut by specified depth if inside but not on the border
  {
    if(!pAabbCutIn || 2==Overlap::AABB_AABB_Inside(nodeBox,*pAabbCutIn))
    {
      if(!GetCVars()->e_VoxTerOptimizeLods || !pIsoTree)
      return;

      if(nIdx.depth >= nMaxDepthToCopy+1)
        return;

      if(pIsoTree)
      {
        if(eHasImportantDetails != IsNodeClippable(*pIsoTree,srcNode,nIdx,m_flatnessMap,fLodCurvature,0))
          return;
      }
    }
  }

  if(bCopyOnlyIfMeshExist)
  {
    if(nIdx.depth >= nMeshDepth-GetCVars()->e_VoxTerMaxMeshLods)	
      if(TIsoMeshes * renderMeshes = CIsoOctreeThread::pRenderMeshes)
    {
      ISO_KEY key = OctNode::CenterIndex(nIdx,maxDepth);
      if( renderMeshes->find(key) == renderMeshes->end() )
        return;
    }

    if((nIdx.depth+1) >= nMeshDepth-GetCVars()->e_VoxTerMaxMeshLods)	
      if(srcNode->children)
    {
      bool bChHasMeshes = false;

      assert(CIsoOctreeThread::pRenderMeshes);
      TIsoMeshes & renderMeshes = *CIsoOctreeThread::pRenderMeshes;
      assert(CIsoOctreeThread::pRenderEmptyMeshes);
      TIsoMeshes & renderEmptyMeshes = *CIsoOctreeThread::pRenderEmptyMeshes;

      for(int i=0; i<Cube::CORNERS; i++)
      {
        ISO_KEY keyChild = OctNode::CenterIndex(nIdx.child(i),maxDepth);
        if( renderMeshes.find(keyChild) != renderMeshes.end() || renderEmptyMeshes.find(keyChild) != renderEmptyMeshes.end() )
        {
          bChHasMeshes = true;
          break;
        }
      }

      if(!bChHasMeshes)
        return;
    }
  }
/*
  if(pAabbCutIn && pIsoTree && (nIdx.depth >= nMaxDepthToCopy-2) && (2==Overlap::AABB_AABB_Inside(nodeBox,*pAabbCutIn)))
  {
    if(IsClippable(*pIsoTree,srcNode,nIdx,m_flatnessMap,fLodCurvature,0))
      return;
  }
*/
  dstNode->initChildren();

  for(int i=0; i<Cube::CORNERS; i++)
    CopyTree(pAabbCutIn, aabbCutOut, nMaxDepthToCopy, &srcNode->children[i], nIdx.child(i), &dstNode->children[i], bCopyOnlyIfMeshExist, pIsoTree, fLodCurvature, pColValues);
}

void IsoOctree::RestoreChilds(std::map<OctNode*,OctNode*> & detachedChilds)
{
  for(std::map<OctNode*,OctNode*>::iterator iter=detachedChilds.begin();iter!=detachedChilds.end();iter++)
  {
    OctNode* node = iter->first;
    assert(!node->children);
    node->children = iter->second;
  }
}

SIsoMesh * IsoOctree::GenerateIsoMeshForPhysics( const OctNode::NodeIndex& nIdx, bool bLog, const AABB & clipBoxWS )
{
  FUNCTION_PROFILER_3DENGINE;

  std::vector< std::vector<int> > polygonsIn; polygonsIn.clear();
  std::vector<uint16> indices; indices.clear();
  std::vector<Vec3> vertices; vertices.clear();
  std::vector<SSurfTypeInfo> surf_types; surf_types.clear();

  if(bLog)
    PrintMessage("Compiling mesh (%d,%d,%d-%d) ...", (int)clipBoxWS.min.x, (int)clipBoxWS.min.y, (int)clipBoxWS.min.z, (int)clipBoxWS.GetSize().x);

  AABB clipBoxNorm = clipBoxWS;
  clipBoxNorm.min /= CVoxTerrain::m_fMapSize;
  clipBoxNorm.max /= CVoxTerrain::m_fMapSize;
  getIsoSurface(vertices,&surf_types,NULL,&polygonsIn,NULL,0,&clipBoxNorm,m_bEditor);

  CVoxTerrain::PolygonToTriangleMeshForPhysics(vertices, surf_types, polygonsIn, indices);

  ClipMeshByAABB(clipBoxNorm, indices, vertices, surf_types);

  for(uint32 i=0; i<vertices.size(); i++)
    vertices[i] *= CVoxTerrain::m_fMapSize;

  if(bLog)
    PrintMessagePlus(" %d tris produced ...", indices.size()/3);

  SIsoMesh * pIsoMesh = new SIsoMesh(0);
  pIsoMesh->m_WSBBox = clipBoxWS;
  pIsoMesh->BildIndexedMeshForPhysics(indices,vertices,surf_types);

  if(bLog)
    PrintMessagePlus(" ok");

  return pIsoMesh;
}

int GetTerrainVertIndex(int _x, int _y, int _step,  int nSectorSize)
{
  unsigned short id = _x/_step*(nSectorSize/_step+1) + _y/_step;
  return id;
}

SIsoMesh * IsoOctree::GenerateIsoMeshForRendering(const OctNode::NodeIndex& nIdx, bool bLog, SIsoMesh * pParentMesh, void * node, void * pIdx, char * szCurrentMessageIn)
{
  FUNCTION_PROFILER_3DENGINE;

  float fStartTime = GetCurTimeSec();

  std::vector< std::vector<int> > polygonsIn; polygonsIn.clear();
  std::vector< std::vector<int> > polygonsOut; polygonsOut.clear();
  std::vector<TriangleIndex> triangles; triangles.clear();
  std::vector<Vec3> vertices; vertices.clear();
  std::vector<SSurfTypeInfo> surf_types; surf_types.clear();
  std::vector<ColorB> colors; colors.clear();

  AABB WSBBox = GetNodeAABB(nIdx, true);

  if(bLog)
    PrintMessage("Compiling mesh (%d,%d,%d-%d) ...", (int)WSBBox.min.x, (int)WSBBox.min.y, (int)WSBBox.min.z, (int)WSBBox.GetSize().x);

  if(!GetCVars()->e_VoxTerHeightmapEditing)
  {
    AABB nodeBoxIn = GetNodeAABB(nIdx);
    float fBorder = 0.02f / CVoxTerrain::m_fMapSize;
    nodeBoxIn.min += Vec3(fBorder,fBorder,fBorder);
    nodeBoxIn.max -= Vec3(fBorder,fBorder,fBorder);
    getIsoSurface(vertices,&surf_types,&colors,&polygonsIn,&polygonsOut,0,&nodeBoxIn,m_bEditor);
    CVoxTerrain::PolygonToTriangleMeshForRendering(vertices, surf_types,  polygonsIn, triangles, 1);
    CVoxTerrain::PolygonToTriangleMeshForRendering(vertices, surf_types, polygonsOut, triangles, 0);
  }

  int nBrushTrisAdded = 0;

  if(GetCVars()->e_VoxTerHeightmapEditing && C3DEngine::m_pGetLayerIdAtCallback)
  {
    int nOriginX = (int)WSBBox.min.x;
    int nOriginY = (int)WSBBox.min.y;
    int nSectorSize = (int)WSBBox.GetSize().x;
    int nStep = nSectorSize/32;
    int nUnitSize = GetTerrain()->GetHeightMapUnitSize();
    nStep = max(nUnitSize, nStep);

    int bUB = nStep>nUnitSize;

    float fMin=1000000;
    float fMax=0;

    for( int _x=nOriginX-nStep*bUB; _x<=nOriginX+nSectorSize+nStep*bUB; _x+=nStep )
    {
      for( int _y=nOriginY-nStep*bUB; _y<=nOriginY+nSectorSize+nStep*bUB; _y+=nStep )
      {
        int x = CLAMP(_x, nOriginX, nOriginX+nSectorSize);
        int y = CLAMP(_y, nOriginY, nOriginY+nSectorSize);

        { // position
          Vec3 vPos;
          vPos.x = x;
          vPos.y = y;
          vPos.z = C3DEngine::m_pGetLayerIdAtCallback->GetElevationAtPosition(y/nUnitSize,x/nUnitSize);
          if(x!=_x || y!=_y)
            vPos.z -= 0.01f*nSectorSize;
          vertices.push_back(vPos);
          fMin = min(fMin,vPos.z);
          fMax = max(fMax,vPos.z);
        }

        { // surface type
          int surf_type;
          if(GetCVars()->e_VoxTerHeightmapEditingCustomLayerInfo)
            surf_type = m_pVoxTerrain->GetLayerAtPosition((int)(y/nUnitSize),(int)(x/nUnitSize)).GetDomSType();
          else
            surf_type = C3DEngine::m_pGetLayerIdAtCallback->GetLayerIdAtPosition((int)(y/nUnitSize),(int)(x/nUnitSize));
          surf_types.push_back(surf_type);
        }

        { // color
//          ColorB colB = Col_White;//C3DEngine::m_pGetLayerIdAtCallback->GetColorAtPosition(y,x);
  //        colors.push_back(colB);
        }
      }
    }
  
    if(fMin < WSBBox.max.z && fMax > WSBBox.min.z)
    {
      int nSectorSizeB = nSectorSize+nStep*2*bUB;

      int nWrap = (GetTerrain()->GetTerrainSize()/nUnitSize-1u);

      for( int x=0; x<nSectorSizeB; x+=nStep )
      {   
        for( int y=0; y<nSectorSizeB; y+=nStep )
        {
          if(!C3DEngine::m_pGetLayerIdAtCallback->GetHoleAtPosition(nWrap&((int)((nOriginY+y)/nUnitSize)),nWrap&((int)((nOriginX+x)/nUnitSize))))
          {
            TriangleIndex t0;
            t0[0] = GetTerrainVertIndex(x       ,y        ,nStep,nSectorSizeB);
            t0[1] = GetTerrainVertIndex(x+nStep ,y        ,nStep,nSectorSizeB);
            t0[2] = GetTerrainVertIndex(x       ,y+nStep  ,nStep,nSectorSizeB);

            TriangleIndex t1;
            t1[0] = t0[1];
            t1[1] = GetTerrainVertIndex(x+nStep ,y+nStep  ,nStep,nSectorSizeB);
            t1[2] = t0[2];

            if(GetCVars()->e_VoxTerRoadsCheck == 2)
            {
              AABB triBox; 
              triBox.min = triBox.max = vertices[t0[0]];
              triBox.Add(vertices[t0[1]]);
              triBox.Add(vertices[t0[2]]);
              triBox.Add(vertices[t1[1]]);

              Vec3 vArea(GetCVars()->e_VoxTerRoadsCullDist,GetCVars()->e_VoxTerRoadsCullDist,GetCVars()->e_VoxTerRoadsCullDist);
              triBox.min -= vArea;
              triBox.max += vArea;

              if(!Get3DEngine()->m_pObjectsTree[0]->IsObjectTypeInTheBox(eERType_Road,triBox))
                continue;
            }

            triangles.push_back(t0);
            triangles.push_back(t1);
          }
        }
      }

      AABB WSBoxEx = WSBBox;

      bool b2DMixMask = GetCVars()->e_VoxTerHeightmapEditing && GetCVars()->e_VoxTerMixMask;

      if(b2DMixMask)
      {
        float fBorder = 0.01f*nSectorSize;
        WSBoxEx.min -= Vec3(fBorder,fBorder,fBorder);
        WSBoxEx.max += Vec3(fBorder,fBorder,fBorder);
      }

      ClipMeshByAABB(WSBoxEx, triangles, vertices, surf_types);

      colors.resize(vertices.size());

      if(GetCVars()->e_VoxTerMixMask)
      {
        for(uint32 v=0; v<vertices.size(); v++)
        {
          colors[v] = Col_White;
          vertices[v] /= CVoxTerrain::m_fMapSize;
        }
      }
      else
      {
        float fClamp = 1.f/C3DEngine::m_pGetLayerIdAtCallback->GetRGBMultiplier();

        for(uint32 v=0; v<vertices.size(); v++)
        {
          ColorB colB = C3DEngine::m_pGetLayerIdAtCallback->GetColorAtPosition(vertices[v].y,vertices[v].x,true);
          ColorF colF;
          colF.r = 1.f/255.f*colB.r;
          colF.g = 1.f/255.f*colB.g;
          colF.b = 1.f/255.f*colB.b;
          colF.srgb2rgb();
          float fBr = (colF.r+colF.g+colF.b)*0.3333f;
          colF.lerpFloat(ColorF(fBr,fBr,fBr,1.f), colF, SATURATE(fBr*4.f));
          colF.clamp(0,fClamp);
          colors[v] = colF;
          vertices[v] /= CVoxTerrain::m_fMapSize;
        }
      }

      if(triangles.size())
        nBrushTrisAdded = 1;
    }
  }

  if(!triangles.size())
  {
    triangles.clear();
    vertices.clear();
    surf_types.clear();
    colors.clear();
  }

  if(GetCVars()->e_VoxTerAddBrushes)
  {
    std::vector<TriangleIndex> br_triangles; br_triangles.clear();
    std::vector<Vec3> br_vertices; br_vertices.clear();
    std::vector<SSurfTypeInfo> br_surf_types; br_surf_types.clear();

    AddBrushesIntoMesh( GetNodeAABB(nIdx,true),
      br_triangles,
      br_vertices,
      br_surf_types);

    // add verts
    int nInitVertCount = vertices.size();
    vertices.resize(vertices.size() + br_vertices.size());
    surf_types.resize(surf_types.size() + br_vertices.size());
    colors.resize(colors.size() + br_vertices.size());

    for(uint32 v=0; v<br_vertices.size(); v++)
    {
      vertices[nInitVertCount+v] = br_vertices[v] / CVoxTerrain::m_fMapSize;
      surf_types[nInitVertCount+v] = br_surf_types[v];
      colors[nInitVertCount+v] = Col_White;
    }

    if(GetCVars()->e_VoxTerRoadsCheck == 2)
      for(uint32 t=0; t<br_triangles.size(); t++)
    {
      TriangleIndex & t0 = br_triangles[t];

      AABB triBox; 
      triBox.min = triBox.max = br_vertices[t0[0]];
      triBox.Add(br_vertices[t0[1]]);
      triBox.Add(br_vertices[t0[2]]);

      Vec3 vArea(GetCVars()->e_VoxTerRoadsCullDist+4.f,GetCVars()->e_VoxTerRoadsCullDist+4.f,GetCVars()->e_VoxTerRoadsCullDist+4.f);
      triBox.min -= vArea;
      triBox.max += vArea;

      Vec3 vTriNorm = (br_vertices[t0[1]]-br_vertices[t0[0]]).Cross(br_vertices[t0[2]]-br_vertices[t0[0]]);

      if(!Get3DEngine()->m_pObjectsTree[0]->IsObjectTypeInTheBox(eERType_Road,triBox) || vTriNorm.z<0)
      {
        std::vector<TriangleIndex>::iterator remove_first = br_triangles.begin()+t;
        br_triangles.erase(remove_first);
        t--;
      }
    }

    // add tris
    int nInitTrisCount = triangles.size();
    triangles.resize(triangles.size() + br_triangles.size());

    for(uint32 t=0; t<br_triangles.size(); t++)
    {
      triangles[nInitTrisCount+t][0] = br_triangles[t][0]+nInitVertCount;
      triangles[nInitTrisCount+t][1] = br_triangles[t][1]+nInitVertCount;
      triangles[nInitTrisCount+t][2] = br_triangles[t][2]+nInitVertCount;
      triangles[nInitTrisCount+t].nCull = 0;
      nBrushTrisAdded++;
    }
  }

  if(bLog)
    PrintMessagePlus(" %d tris produced ...", triangles.size());

  if(triangles.size()>65536/3)
  {
    Warning("IsoOctree::GenerateIsoMeshForRendering: Too many tris produced for sector (%d), mesh size = %d", triangles.size(), (int)WSBBox.GetSize().x);
    triangles.resize(65536/3);
    assert(!"Currently only 65536/3 tris per mesh is supported by texture coordinates unwrapper");
  }

  SIsoMesh * pIsoMesh = new SIsoMesh(OctNode::CenterIndex( nIdx, IsoOctree::maxDepth ));

  if(polygonsIn.size() || nBrushTrisAdded)
  {
    pIsoMesh->BildIndexedMeshForRendering(triangles,vertices,surf_types,colors,
      CVoxTerrain::m_fMapSize,
      WSBBox,
      bLog, 
      false, node, pIdx, szCurrentMessageIn );
  }
  else if(bLog)
    PrintMessagePlus(" polygonsIn=0 ...");

  if(bLog)
    PrintMessagePlus(" ok");

  CVoxTerrain::m_arrTimeStats[nIdx.depth] += (GetCurTimeSec() - fStartTime);

  return pIsoMesh;
}

void IsoOctree::AddStatObjIntoMesh(Matrix34 objMat, IRenderMesh * pRM, IMaterial * pMat,
                                   const AABB & nodeBoxWS,
                                   std::vector<TriangleIndex> & triangles,
                                   std::vector<Vec3> & vertices,
                                   std::vector<SSurfTypeInfo> & surf_types,
                                   int nSurfType)
{
  int nPosStride=0;
  int nIndCount = pRM->GetIndicesCount();
  const uint16 * pIndices = pRM->GetIndexPtr(FSL_READ);
  const byte * pPos = pRM->GetPosPtr(nPosStride, FSL_READ);
  const int nVertCount = pRM->GetVerticesCount();
  assert(pIndices && pPos);

  // copy verts
  int nInitVertCount = vertices.size();
  vertices.resize(nInitVertCount+nVertCount);
  surf_types.resize(nInitVertCount+nVertCount);

  nSurfType = CLAMP(nSurfType, 0, Get3DEngine()->m_arrBaseTextureData.Count()-1);

  for(int v=0; v<nVertCount; v++)
  {
    Vec3 vPos; memcpy(&vPos, &pPos[nPosStride*v], sizeof(vPos));
    vPos = objMat.TransformPoint(vPos);
    vertices[nInitVertCount+v] = vPos;
    surf_types[nInitVertCount+v] = nSurfType;
  }

  // copy indices

  PodArray<CRenderChunk>& Chunks = pRM->GetChunks();

  for(int i=0; i<Chunks.Count(); i++)
  {
    CRenderChunk *pChunk = &Chunks[i];

    if(pChunk->m_nMatFlags & MTL_FLAG_NODRAW || !pChunk->pRE)
      continue;

    // skip transparent and alpha test
    const SShaderItem &shaderItem = pMat->GetShaderItem(pChunk->m_nMatID);
    if (!shaderItem.IsZWrite() || !shaderItem.m_pShaderResources || shaderItem.m_pShaderResources->GetAlphaRef())
      continue;

    if(shaderItem.m_pShader && shaderItem.m_pShader->GetFlags()&EF_DECAL)
      continue;

    int nDoubleSide = 0;//((shaderItem.m_pShader->GetShaderType() == eST_Vegetation) && shaderItem.m_pShaderResources->GetAlphaRef()) ? 1 : 0;

    int nInitTrisCount = triangles.size();
    triangles.resize(nInitTrisCount + (pChunk->nNumIndices/3)*(1+nDoubleSide));

    int nTrisAdded = 0;
    for(int t=pChunk->nFirstIndexId; t<pChunk->nFirstIndexId+pChunk->nNumIndices; t+=3)
    {
      triangles[nInitTrisCount+nTrisAdded][0] = pIndices[t+0]+nInitVertCount;
      triangles[nInitTrisCount+nTrisAdded][2] = pIndices[t+2]+nInitVertCount;
      triangles[nInitTrisCount+nTrisAdded][1] = pIndices[t+1]+nInitVertCount;
      triangles[nInitTrisCount+nTrisAdded].nCull = 0;
      nTrisAdded++;

      if(nDoubleSide)
      {
        triangles[nInitTrisCount+nTrisAdded][0] = pIndices[t+0]+nInitVertCount;
        triangles[nInitTrisCount+nTrisAdded][1] = pIndices[t+2]+nInitVertCount;
        triangles[nInitTrisCount+nTrisAdded][2] = pIndices[t+1]+nInitVertCount;
        triangles[nInitTrisCount+nTrisAdded].nCull = 0;
        nTrisAdded++;
      }
    }
  }
}

void IsoOctree::AddBrushesIntoMesh( const AABB & nodeBoxWS,
                                    std::vector<TriangleIndex> & triangles,
                                    std::vector<Vec3> & vertices,
                                    std::vector<SSurfTypeInfo> & surf_types)
{
  PodArray<IRenderNode*> lstBrushes; lstBrushes.Clear();

  for(int nSID=0; nSID<Get3DEngine()->m_pObjectsTree.Count(); nSID++)
  {
    if(!Get3DEngine()->m_pObjectsTree[nSID])
      continue;

    Get3DEngine()->m_pObjectsTree[nSID]->GetObjectsByType(lstBrushes, eERType_Brush, &nodeBoxWS, nodeBoxWS.GetRadius()*2.f);
    Get3DEngine()->m_pObjectsTree[nSID]->GetObjectsByType(lstBrushes, eERType_Vegetation, &nodeBoxWS, nodeBoxWS.GetRadius()*2.f);
  }

  for(int i=0; i<lstBrushes.Count(); i++)
  {
    if(lstBrushes[i]->GetIntegrationType() != eIT_VoxelMesh)
      continue;

    uint32 nFlags = lstBrushes[i]->GetRndFlags();

    if(nFlags&ERF_COLLISION_PROXY || nFlags&ERF_RAYCAST_PROXY)
      continue;

    if(lstBrushes[i]->m_fWSMaxViewDist*GetCVars()->e_VoxTerViewDistRatio < nodeBoxWS.GetSize().x)
      continue;

    int nLod = 0;
 
    float fLodDist = 8.f;
    while((fLodDist < nodeBoxWS.GetSize().x) || nLod < GetCVars()->e_LodMin)// && lstBrushes[i]->GetRenderMesh(nLod+1))
    {
      if(!lstBrushes[i]->GetRenderMesh(nLod+1))
        break;

      if(lstBrushes[i]->GetRenderMesh(nLod) && lstBrushes[i]->GetRenderMesh(nLod)->GetIndicesCount()/3 < 500)
        break;

      fLodDist *= 2;
      nLod++;
    }

    Matrix34A objMat;
    CStatObj * pStatObj = (CStatObj *)lstBrushes[i]->GetEntityStatObj(0,0,&objMat);

    int nLayerId=0;

    // find layer for re-meshing
    for(nLayerId=0; nLayerId<Get3DEngine()->m_arrBaseTextureData.Count(); nLayerId++)
    {
      SImageInfo & rImgInfo = Get3DEngine()->m_arrBaseTextureData[nLayerId];
      if(rImgInfo.fUseRemeshing)
        break;
    }

    // if failed - find any valid layer
    if(nLayerId >= Get3DEngine()->m_arrBaseTextureData.Count())
    {
      for(nLayerId=0; nLayerId<Get3DEngine()->m_arrBaseTextureData.Count(); nLayerId++)
      {
        SImageInfo & rImgInfo = Get3DEngine()->m_arrBaseTextureData[nLayerId];
        if(rImgInfo.szBaseTexName[0])
          break;
      }
    }

    if(nLayerId >= Get3DEngine()->m_arrBaseTextureData.Count())
    {
      Error("IsoOctree::AddBrushesIntoMesh: appropriate terrain layer not found");
      continue;
    }

    IMaterial * pMat = lstBrushes[i]->GetMaterial();

    if(pStatObj->GetSubObjectCount() && (pStatObj->GetFlags() & STATIC_OBJECT_COMPOUND))
    {
      for(int s=0; s<pStatObj->GetSubObjectCount(); s++)
      {
        IStatObj::SSubObject &subObj = pStatObj->SubObject(s);
        if (subObj.pStatObj && !subObj.bHidden && subObj.nType == STATIC_SUB_OBJECT_MESH)
        if(IStatObj * pSubObj = pStatObj->GetSubObject(s)->pStatObj)
        {
          Matrix34A subObjMatrix = objMat * subObj.tm;
          if(pSubObj = pSubObj->GetLodObject(nLod))
            if(IRenderMesh * pRM = pSubObj->GetRenderMesh())
              AddStatObjIntoMesh(subObjMatrix, pRM, pMat, nodeBoxWS, triangles, vertices, surf_types, nLayerId);
        }
      }
    }
    else
    {
      if(CStatObj * pLodObj = (CStatObj *)pStatObj->GetLodObject(nLod))
        if(IRenderMesh * pRM = pLodObj->GetRenderMesh())
          AddStatObjIntoMesh(objMat, pRM, pMat, nodeBoxWS, triangles, vertices, surf_types, nLayerId);
    }

    ClipMeshByAABB(nodeBoxWS, triangles, vertices, surf_types);
  }
}

int IsoOctree::ClipEdge(const Vec4 & v1, const Vec4 & v2, const Plane & ClipPlane, Vec4 & vRes1, Vec4 & vRes2)
{
  float d1 = -ClipPlane.DistFromPlane(*(Vec3*)&v1);
  float d2 = -ClipPlane.DistFromPlane(*(Vec3*)&v2);
  if(d1<0 && d2<0)
    return 0; // all clipped = do not add any vertices

  if(d1>=0 && d2>=0)
  {
    vRes1 = v2;
    return 1; // both not clipped - add second vertex
  }

  // calculate new vertex
  float t = Ffabs(d1)/(Ffabs(d2)+Ffabs(d1));
  Vec4 vIntersectionPoint = v1 + (v2-v1)*t;
  if(t<0.5)
    vIntersectionPoint.w = v1.w;
  else
    vIntersectionPoint.w = v2.w;

#ifdef _DEBUG
  float fNewDist = -ClipPlane.DistFromPlane(*(Vec3*)&vIntersectionPoint);
  assert(Ffabs(fNewDist)<0.01f);
#endif

  if(d1>=0 && d2<0)
  { // from vis to no vis
    vRes1 = vIntersectionPoint;
    return 1;
  }
  else if(d1<0 && d2>=0)
  { // from not vis to vis
    vRes1 = vIntersectionPoint;
    vRes2 = v2;
    return 2;
  }

  assert(0);
  return 0;
}

void IsoOctree::ClipPolygon(PodArray<Vec4> * pPolygon, const Plane & ClipPlane)
{
  PodArray<Vec4> PolygonOut; PolygonOut.Clear();

  // clip edges, make list of new vertices
  for(int i=0; i<pPolygon->Count(); i++)
  {
    Vec4 vNewVert1(0,0,0,0), vNewVert2(0,0,0,0);
    if(int nNewVertNum = ClipEdge(pPolygon->GetAt(i), pPolygon->GetAt((i+1)%pPolygon->Count()), ClipPlane, vNewVert1, vNewVert2))
    {
      PolygonOut.Add(vNewVert1);
      if(nNewVertNum>1)
        PolygonOut.Add(vNewVert2);
    }
  }

  // check result
  for(int i=0; i<PolygonOut.Count(); i++)
  {
    float d1 = -ClipPlane.DistFromPlane(*(Vec3*)&PolygonOut.GetAt(i));
    assert(d1>=-0.01f);
  }

  assert(PolygonOut.Count()==0 || PolygonOut.Count() >= 3);

  pPolygon->Clear();
  pPolygon->AddList( PolygonOut );
}

bool IsoOctree::ClipTriangle(std::vector<uint16> & indices, std::vector<Vec3> & vertices, std::vector<SSurfTypeInfo> & surf_types, int nStartIdxId, Plane * pPlanes, int nPlanesNum)
{
  PodArray<Vec4> lstPolygon; lstPolygon.Clear();
  lstPolygon.Add(Vec4(vertices[indices[nStartIdxId+0]], surf_types[indices[nStartIdxId+0]].GetDomSType()));
  lstPolygon.Add(Vec4(vertices[indices[nStartIdxId+1]], surf_types[indices[nStartIdxId+1]].GetDomSType()));
  lstPolygon.Add(Vec4(vertices[indices[nStartIdxId+2]], surf_types[indices[nStartIdxId+2]].GetDomSType()));

  for(int i=0; i<nPlanesNum && lstPolygon.Count()>=3; i++)
    ClipPolygon(&lstPolygon, pPlanes[i]);

  if(lstPolygon.Count()>32)
  {
    assert(0);
    return false;
  }

  if(lstPolygon.Count() < 3)
  {
    std::vector<uint16>::iterator remove_first = indices.begin()+nStartIdxId;
    std::vector<uint16>::iterator remove_last  = indices.begin()+nStartIdxId+3;
    indices.erase(remove_first,remove_last);
    return true; // entire triangle is clipped away
  }

  if(lstPolygon.Count() == 3)
    if((*(Vec3*)&lstPolygon[0]).IsEquivalent(vertices[indices[nStartIdxId+0]]))
      if((*(Vec3*)&lstPolygon[1]).IsEquivalent(vertices[indices[nStartIdxId+1]]))
        if((*(Vec3*)&lstPolygon[2]).IsEquivalent(vertices[indices[nStartIdxId+2]]))
          return false; // entire triangle is in

  // replace old triangle with several new triangles
  int nStartId = vertices.size();
//  vertices.AddList(lstPolygon);
  for(uint32 i=0; i<lstPolygon.size() ; i++)
  {
    vertices.push_back((*(Vec3*)&lstPolygon[i]));
    SSurfTypeInfo stype;
    stype = (int)lstPolygon[i].w;
    surf_types.push_back(stype);
  }

  // put first new triangle into position of original one
  indices[nStartIdxId+0] = nStartId+0;
  indices[nStartIdxId+1] = nStartId+1;
  indices[nStartIdxId+2] = nStartId+2;

  // put others in the end
  for(uint32 i=1; i<lstPolygon.size()-2 ; i++)
  {
    indices.push_back(nStartId+0);
    indices.push_back(nStartId+i+1);
    indices.push_back(nStartId+i+2);
  }

  return false;
}

void IsoOctree::DrawDebugValues(OctNode* node, const OctNode::NodeIndex& nIdx)
{
  AUTO_LOCK(g_cIsoTreeAccess);

  OctNode* temp = node;

  AABB aabbNode = GetNodeAABB(nIdx, true);
  if(!GetCamera().IsAABBVisible_F(aabbNode))
    return;

  float w=0;
  Vec3 center(0,0,0);
  OctNode::CenterAndWidth(nIdx,center,w);
  w*=CVoxTerrain::m_fMapSize;
  center*=CVoxTerrain::m_fMapSize;

  float fDistance = sqrt(Distance::Point_AABBSq(GetCamera().GetPosition(), aabbNode));

  float hw = w/2;

  bool bDraw = (fDistance*GetCVars()->e_VoxTerViewDistRatio>hw || !node->children || nIdx.depth >= nMeshDepth);

  if(bDraw)
  {
    for(int c=0;c<Cube::CORNERS;c++)
    {
      int x,y,z;
      Cube::FactorCornerIndex(c,x,y,z);
      Vec3 p = (center - Vec3(w/2,w/2,w/2) + Vec3(w*x,w*y,w*z));
//      float fDist = GetCamera().GetPosition().GetDistance(p);

  //    if(fDist<8)
      {
        ISO_KEY key = OctNode::CornerIndex(nIdx,c,maxDepth);
        if( (*m_pValues).find(key)!=(*m_pValues).end() )
        {
          float fVal = CVoxTerrain::m_fMapSize*(*m_pValues)[key].val;
          int surf_type = (*m_pValues)[key].surf_type.GetDomSType();
          uint8 * color = &(*m_pValues)[key].color[0];
          ColorF col = fVal>0 ? Col_Red : Col_Green;
          gEnv->pRenderer->DrawLabelEx(p, 1.f, (float*)&col, false, true, "v%.1f", fVal);//, surf_type, color[0], color[1], color[2]);
        }
      }
    }
  }
  else
  {
    for(int i=0;i<Cube::CORNERS;i++)
    {
      DrawDebugValues(&node->children[i], nIdx.child(i));
    }
  }
}

int IsoOctree::setFromSphere(const Sphere & sp, const int& maxDepth,const int& setCenter,const float& flatness,EVoxelEditOperation eOperation, bool bConforming, int surf_type, ColorB color, const SBrushAABB * pBrushAABB, IsoOctree * isoTreeBk)
{
  this->maxDepth=maxDepth;
  OctNode::NodeIndex nIdx;

  TRoots visitedValues;

  float dist;
  Vec3 n,p;
  for(int c=0;c<Cube::CORNERS;c++)
  {
    int x,y,z;
    Cube::FactorCornerIndex(c,x,y,z);
    p[0]=float(x);
    p[1]=float(y);
    p[2]=float(z);

    setDistanceAndNormalFromSphere(sp,p,dist,OctNode::CornerIndex(nIdx,c,maxDepth), eOperation, surf_type, color, isoTreeBk, &visitedValues);
  }

  setChildrenFromSphere(&tree,nIdx,sp,maxDepth,setCenter,flatness,eOperation,NULL/*GetTerrain()->GetParentNode(0)*/, surf_type, color, pBrushAABB, isoTreeBk, &visitedValues);

  // Refine non-conforming nodes
/*  int forceConforming = 1;

  while(bConforming && forceConforming)
  {
    forceConforming=0;
    nIdx=OctNode::NodeIndex();
    for(OctNode* node=tree.nextLeaf(NULL,nIdx) ; node ; node=tree.nextLeaf(node,nIdx) )
    {
      int setChildren=0;

      for(int i=0;i<Cube::FACES && !setChildren;i++)
        if(HasFaceGrandChildren(node->faceNeighbor(i),Cube::FaceReflectFaceIndex(i,i)))
          setChildren=1;
      for(int i=0;i<Cube::EDGES && !setChildren;i++)
        if(HasEdgeGrandChildren(node->edgeNeighbor(i),Cube::EdgeReflectEdgeIndex(i)))
          setChildren=1;

      if(setChildren)
      {
        node->initChildren();
        interpolateChilds(nIdx, node);
        forceConforming=1;
      }
    }
  }
*/

  if(GetCVars()->e_VoxTerDebug)
    setMCIndex(0);

  return 1;
}

float IsoOctree::GetElevation3D(OctNode* node, const OctNode::NodeIndex& nIdx, Vec3 vPos, float & fMaxZ, Vec3 & vMaxZNorm)
{
  Vec3 vStart = vPos; 
  if(vStart.z == 0.f) 
    vStart.z = Get3DEngine()->GetTerrainSize() + 100;
  Vec3 vEnd = vPos; vEnd.z = fMaxZ - 0.1f;
  Ray ray(vStart, vEnd-vStart);
  Vec3 vOut;

  ISO_KEY key;
  float hw;
  Vec3 center,p;

  OctNode::CenterAndWidth(nIdx,center,hw);
  hw*=CVoxTerrain::m_fMapSize*0.5f;
  center*=CVoxTerrain::m_fMapSize;
  AABB WSBBox(Vec3(center.x-hw,center.y-hw,center.z-hw),Vec3(center.x+hw,center.y+hw,center.z+hw));

  if(!Intersect::Ray_AABB(ray,WSBBox,vOut) || !node)
    return 0;

  bool bDraw = (!node->children || nIdx.depth >= (nMeshDepth-2));

  if(!bDraw && nIdx.depth < nMeshDepth)
  {
    for(int i=0;i<Cube::CORNERS;i++)
    {
      if(!TestGetElevationChilds(&node->children[i],nIdx.child(i), ray))
        bDraw = true;
    }
  }

  if(bDraw)
  {
    key=OctNode::CenterIndex(nIdx,maxDepth);

    TIsoMeshes & renderMeshes = *CIsoOctreeThread::pRenderMeshes;
//    TIsoMeshes & renderEmptyMeshes = *CIsoOctreeThread::pRenderEmptyMeshes;

    if( renderMeshes.find(key) != renderMeshes.end() )
    {
      SIsoMesh * pIsoMesh = renderMeshes[key];
      if(IRenderMesh * pRenderMesh = pIsoMesh->m_pRenderMesh)
      {
        Vec3 vHitPoint(0,0,0), vHitNorm(0,0,0);
        vEnd.z = fMaxZ-1.f;
        if(CObjManager::RayRenderMeshIntersection(pRenderMesh, vStart, vEnd-vStart, vHitPoint, vHitNorm, false, NULL))
        {
          if(fMaxZ < vHitPoint.z)
          {
            fMaxZ = vHitPoint.z;
            vMaxZNorm = vHitNorm;
          }
        }
      }
    }
  }
  else
  {
    for(int i=0;i<Cube::CORNERS;i++)
      GetElevation3D(&node->children[i], nIdx.child(i), vPos, fMaxZ, vMaxZNorm);
  }

  return fMaxZ;
}

void IsoOctree::LimitDepth(OctNode* node, const OctNode::NodeIndex& nIdx, const Sphere & sp, const SBrushAABB * pBrushAABB)
{
  if(nIdx.depth<=nMeshDepth)
    if(PrintProgress())
      return;

#ifdef VOX_DVR
  node->nColor = node->nNormal = node->nOcNodeInfoId = 0;
  if(node->nTmpChild)
    node->deleteChildren(m_aabbDirty, GetNodeAABB(nIdx));
#endif

  if(nIdx.depth==maxDepth || nIdx.depth >= GetEditDepth() || !IsVisibleFromGamePlayArea(GetNodeAABB(nIdx,true)))	
  {
    node->deleteChildren(m_aabbDirty, GetNodeAABB(nIdx));
    return;
  }

  if(!node->children)
    return;

  for(int i=0;i<Cube::CORNERS;i++)
  {
    float w;
    Vec3 ctr,p;
    OctNode::CenterAndWidth(nIdx.child(i),ctr,w);

    if(IsBrushAffectsNode(ctr,w,eveoLimitLod,CVoxTerrain::m_fMapSize,NULL,pBrushAABB)) 
    {
      LimitDepth(&node->children[i],nIdx.child(i),sp,pBrushAABB);
    }
  }
}


void IsoOctree::ForceDepth(OctNode* node, const OctNode::NodeIndex& nIdx, const Sphere & sp, const SBrushAABB * pBrushAABB, EVoxelEditOperation eOperation, CTerrainNode * pTerrainNode)
{
  assert(node);

  if(nIdx.depth<=nMeshDepth)
    if(PrintProgress())
      return;

  if(nIdx.depth>nMeshDepth)
  if(nIdx.depth==maxDepth)	
  {
    if(node->children)
      node->deleteChildren(m_aabbDirty, GetNodeAABB(nIdx));
    return;
  }

  AABB nodeAABB = GetNodeAABB(nIdx);
/*
  if( !pBrushAABB->sol.IsReset() && Overlap::AABB_AABB_Inside(nodeAABB, pBrushAABB->sol) == 2 )
  {
    if(node->children)
      node->deleteChildren(m_aabbDirty, GetNodeAABB(nIdx));
    return;
  }

  if( !pBrushAABB->emp.IsReset() && Overlap::AABB_AABB_Inside(nodeAABB, pBrushAABB->emp) == 2 )
  {
    if(node->children)
      node->deleteChildren(m_aabbDirty, GetNodeAABB(nIdx));
    return;
  }
*/

  if(!node->children)	
  {
    if(GetCVars()->e_VoxTerDebug)
      if(node->nodeData.mcIndex <= 0 || node->nodeData.mcIndex >= 255)
        return;

    if(GetEditDepth() <= nIdx.depth)	
      return;

    if(!IsVisibleFromGamePlayArea(GetNodeAABB(nIdx,true)))
      return;

    if(eOperation != eveoCopyTerrainPos && eOperation != eveoCopyTerrainNeg && !GetCVars()->e_VoxTerHeightmapEditing)
    {
      float cValues[Cube::CORNERS];

      for(int i=0;i<Cube::CORNERS;i++)
      {
        ISO_KEY key = OctNode::CornerIndex(nIdx,i,maxDepth);
        TVoxValues::iterator iter = (*m_pValues).find(key);
        if(iter==(*m_pValues).end())
          cValues[i] = 0.f;
        else
          cValues[i] = iter->second.val;
      }

      node->nodeData.mcIndex = MarchingCubes::GetIndex(cValues,0);

      if(node->nodeData.mcIndex == 0 || node->nodeData.mcIndex == 255)
        return;
    }
    else
    {
      /*if(pTerrainNode && !pTerrainNode->m_arrChilds[0])
      {
        float fNodeSize = nodeAABB.GetSize().x*CVoxTerrain::m_fMapSize;
        
        static int nMinNodeToCopy = 0;
        if(!nMinNodeToCopy)
        {
          nMinNodeToCopy = GetTerrain()->GetHeightMapUnitSize();
          for(int l=0; l<GetTerrain()->m_nUnitsToSectorBitShift; l++)
            nMinNodeToCopy *= 2;
        }

        if(fNodeSize < nMinNodeToCopy)
        {
          int nErrSlot = 0;
          float fTerrAreaSize = GetTerrain()->GetHeightMapUnitSize();

          while(fTerrAreaSize < fNodeSize)
          {
            nErrSlot ++;
            fTerrAreaSize *= 2;
          }

          if(pTerrainNode->m_pGeomErrors[nErrSlot]<2.f)
          {
            uint8 cVal=255;
            bool bTransition=0;

            for(int i=0;i<Cube::CORNERS;i++)
            {
              ISO_KEY key = OctNode::CornerIndex(nIdx,i,maxDepth);
              TVoxValues::iterator iter = (*m_pValues).find(key);
              if(iter!=(*m_pValues).end())
              {
                if(cVal!=255)
                  if(cVal != iter->second.surf_type.GetDomSType())
                  {
                    bTransition = true;
                    break;
                  }

                cVal = iter->second.surf_type.GetDomSType();
              }
            }

            if(!bTransition)
              return;
          }
        }
      }*/
    }

    node->initChildren();
    m_aabbDirty.Add(GetNodeAABB(nIdx,false));

    assert(node->children);

    if((eOperation != eveoCopyTerrainPos && eOperation != eveoCopyTerrainNeg) || pBrushAABB->spEps.radius<0.1f)
    {
      interpolateChilds(nIdx, node, m_pValues, m_pValues);

      if(GetCVars()->e_VoxTerDebug)
      {
        for(int childId=0; childId<8; childId++)
        {
          float cValues[8];
          for(int i=0;i<Cube::CORNERS;i++)
          {
            ISO_KEY key = OctNode::CornerIndex(nIdx.child(childId),i,maxDepth);
            TVoxValues::iterator iter = (*m_pValues).find(key);
            if(iter==(*m_pValues).end())
              cValues[i] = 0.f;
            else
              cValues[i] = iter->second.val;
          }

          node->children[childId].nodeData.mcIndex = MarchingCubes::GetIndex(cValues,isoValue);
        }
      }
    }
  }

  for(int i=0;i<Cube::CORNERS;i++)
  {
    float w;
    Vec3 ctr,p;
    OctNode::CenterAndWidth(nIdx.child(i),ctr,w);

    CTerrainNode * pChildTerrainNode = (pTerrainNode && pTerrainNode->m_arrChilds[i&3]) ? pTerrainNode->m_arrChilds[i&3] : pTerrainNode;

    if(IsBrushAffectsNode(ctr,w,eOperation,CVoxTerrain::m_fMapSize,pChildTerrainNode,pBrushAABB,true)) 
    {
      assert(node->children);
      ForceDepth(&node->children[i],nIdx.child(i),sp,pBrushAABB,eOperation,pChildTerrainNode);
    }
  }
}
/*
bool IsoOctree::CheckDepth(OctNode* node, const OctNode::NodeIndex& nIdx, int nDepth)
{
  FUNCTION_PROFILER_3DENGINE;

  if(nIdx.depth>nDepth)
    return true;

  if(node->children)	
    for(int i=0;i<Cube::CORNERS;i++)
      if(CheckDepth(&node->children[i],nIdx.child(i),nDepth))
        return true;

  return false;
}
*/
/*
void IsoOctree::GetMinDepth(OctNode* node, const OctNode::NodeIndex& nIdx, int & nMinDepth)
{
if(node->children)	
{
for(int i=0;i<Cube::CORNERS;i++)
{
GetMinDepth(&node->children[i],nIdx.child(i),nMinDepth);
}
}
else
nMinDepth = min(nIdx.depth, nMinDepth);
}
*/

int IsoOctree::GetEditDepth()
{
  return min(editDepth, maxDepth-1);
}

void IsoOctree::CleanUpMeshes(bool bRemoveAll)
{
  if(!threadMeshes.size())
    return;

  TIsoMeshes usedMeshes;

  PrintMessage("Cleanup meshes ...");

  if(!bRemoveAll)
  {
    AUTO_LOCK(g_cIsoTreeAccess);

    OctNode::NodeIndex nIdx;
    if(GetCVars()->e_VoxTerHeightmapEditing)
      GetMeshes(&renderTree, nIdx, usedMeshes);
    else
      GetMeshes(&tree, nIdx, usedMeshes);
  }

  PrintMessagePlus("  %d meshes found ...", usedMeshes.size());

  int nDeleted = 0;

  TIsoMeshes::const_iterator next;
  for(TIsoMeshes::const_iterator iter = threadMeshes.begin(); iter != threadMeshes.end(); iter = next)
  {
    next = iter;
    next++;

    if(usedMeshes.find(iter->first) == usedMeshes.end())
    {
      SIsoMesh * pIsoMesh = iter->second;
      assert(iter->first == pIsoMesh->m_key);
      CVoxTerrain::ReleaseIsoMeshFromMainThread(pIsoMesh, iter->first);
      threadMeshes.erase(iter->first);
      nDeleted++;
    }
  }

  PrintMessagePlus(" %d meshes deleted", nDeleted);
}

IsoOctree::ENodeClipState IsoOctree::IsNodeClippable(const IsoOctree& isoTree,
                           OctNode* node,const OctNode::NodeIndex& nIndex,
                           TFlatness & flatnessMap,
                           const float& clipValue,const int& forceConforming)
{
  /*
  if(forceConforming)
  {
    for(int i=0;i<Cube::FACES;i++)
      if(IsoOctree::HasFaceGrandChildren(node->faceNeighbor(i),Cube::FaceReflectFaceIndex(i,i)))
        return 0;
    for(int i=0;i<Cube::EDGES;i++)
      if(IsoOctree::HasEdgeGrandChildren(node->edgeNeighbor(i),Cube::EdgeReflectEdgeIndex(i)))
        return 0;
  }
*/
  if(node->nodeData.mcIndex > 0 && node->nodeData.mcIndex < 255)
  {
//    if(nIndex.depth<=nMeshDepth)
  //    return 0;
/*
    int nType = -1;
    ColorB color = Col_Black;
    for(int c=0;c<Cube::CORNERS;c++)
    {
      TVoxValues::iterator iter = (*m_pValues).find(OctNode::CornerIndex(nIndex,c,maxDepth));
      if(iter!=(*m_pValues).end())
      {
        if(nType>=0 && (nType!=iter->second.surf_type 
          || abs((int)color[0]-(int)iter->second.color[0])>16
          || abs((int)color[1]-(int)iter->second.color[1])>16
          || abs((int)color[2]-(int)iter->second.color[2])>16
          ))
          return 0;

        nType = iter->second.surf_type;
        color[0] = iter->second.color[0];
        color[1] = iter->second.color[1];
        color[2] = iter->second.color[2];
      }
    }*/
  }

  Vec3_tpl<ISO_DOUBLE> vNormal(0,0,0);
  ISO_DOUBLE fArea = 0;

  bool bFound = false;

  for(int c=0;c<Cube::CORNERS;c++)
  {
    ISO_KEY key = OctNode::CornerIndex(nIndex,c,maxDepth);

    if(flatnessMap.find(key) != flatnessMap.end())
    {
      vNormal += flatnessMap[key].first;
      fArea += flatnessMap[key].second;
      bFound = true;
    }
  }

//  ISO_KEY key=OctNode::CenterIndex(nIndex,isoTree.maxDepth);
//  if(flatnessMap.find(key)!=flatnessMap.end())
  if(bFound)
  {
    ISO_DOUBLE normalSize=vNormal.GetLength();
    ISO_DOUBLE areaSize=fArea;
    if(areaSize<=0.000000001 || normalSize/areaSize>clipValue)
      return eNotEmpty;
    else
      return eHasImportantDetails;
  }
  else
    return eEmpty;
}


void IsoOctree::OptimizeTree(AABB * pAABB, float fCurvature, std::map<OctNode*,OctNode*> * pDetachedChilds, int nTopLevelToKeep, bool bRebuildFlatnessMap)
{
  AUTO_LOCK(g_cIsoTreeAccess);

  float fStartTime = GetCurAsyncTimeSec();

  if(m_bEditor && bRebuildFlatnessMap)
    PrintMessagePlus(" Compute flatness ...");

  FUNCTION_PROFILER_3DENGINE;

  if(bRebuildFlatnessMap)
  {
    m_flatnessMap.clear();
    setNormalFlatness(m_flatnessMap);
  }

  int nChildsDeleted = 0;

  if(m_bEditor && bRebuildFlatnessMap)
    PrintMessagePlus(" Cleanup tree ...");

  //if(1)
  {
    for(int i=maxDepth-1; i > nTopLevelToKeep; i--)
    {
      OctNode::NodeIndex nIdx;
      for(OctNode* temp=tree.nextNode(NULL,nIdx) ; temp ; temp=tree.nextNode(temp,nIdx))
      {
        if(nIdx.depth==i && temp->children)
        {
          IsoOctree::ENodeClipState eClipState = IsNodeClippable(*this,temp,nIdx,m_flatnessMap,fCurvature,0);

          if( eClipState == IsoOctree::eEmpty || !IsVisibleFromGamePlayArea(GetNodeAABB(nIdx,true)) || (nIdx.depth>nMeshDepth && eClipState!=IsoOctree::eHasImportantDetails) )
          {
            if(!pDetachedChilds)
            {
              if(!pAABB || Overlap::AABB_AABB(GetNodeAABB(nIdx),*pAABB))
              {
                bool bDelete = true;

                if(temp->children)
                {
                  for(int c=0; c<8; c++)
                  {
                    if(temp->children[c].children)
                    {
                      bDelete = false;
                      break;
                    }
                  }
                }

                if(bDelete)
                {
                  temp->deleteChildren(m_aabbDirty, GetNodeAABB(nIdx));
                  nChildsDeleted++;
                }
              }
            }
            else
            {             
              if(!pAABB || Overlap::AABB_AABB(GetNodeAABB(nIdx),*pAABB))
              {                  
                bool bDelete = true;

                if(temp->children)
                {
                  for(int c=0; c<8; c++)
                  {
                    if(pDetachedChilds->find(&temp->children[c]) != pDetachedChilds->end() || temp->children[c].children)
                    {
                      bDelete = false;
                      break;
                    }
                  }
                }

                if(pDetachedChilds->find(temp) != pDetachedChilds->end())
                  bDelete = false;

                if(bDelete)
                {
                  temp->deleteChildren(m_aabbDirty, GetNodeAABB(nIdx));
                  nChildsDeleted++;
                }
              }
            }
          }
        }
      }
    }
  }
  /*else
  {
    OctNode::NodeIndex nIdx;
    for(OctNode* temp=tree.nextNode(NULL,nIdx) ; temp ; temp=tree.nextNode(temp,nIdx) )
    {
      if(temp->children && IsClippable(*this,temp,nIdx,m_flatnessMap,fCurvature,0))
      {
        temp->deleteChildren(m_aabbDirty, GetNodeAABB(nIdx));
        nChildsDeleted++;
      }
    }
  }*/

  if(m_bEditor && bRebuildFlatnessMap)
    PrintMessagePlus(" (%.1f sec) %d K nodes removed", GetCurAsyncTimeSec()-fStartTime, nChildsDeleted/1024);
}

bool IsoOctree::GetColorByPosition(OctNode* node, const OctNode::NodeIndex& nIdx, Vec3 vPos, ColorB & resColor)
{
  if(node->children)
  {
    Vec3 ctr; float w;
    OctNode::CenterAndWidth(nIdx,ctr,w);
    int nFirst = ((vPos.x > ctr.x) ? 1 : 0) | ((vPos.y > ctr.y) ? 2 : 0) | ((vPos.z > ctr.z) ? 4 : 0);
    return GetColorByPosition(&node->children[nFirst],nIdx.child(nFirst), vPos, resColor);
  }
  else
  {
    ColorF values[2][2][2];

    for(int c=0;c<Cube::CORNERS;c++)
    {
      int x,y,z;
      Cube::FactorCornerIndex(c,x,y,z);

      TVoxValues::iterator iter = (*m_pValues).find(OctNode::CornerIndex(nIdx,c,maxDepth));

      if(iter==(*m_pValues).end())
      {
        values[x][y][z] = Col_DarkGray;
      }
      else
      {
        values[x][y][z][0] = iter->second.color[0];
        values[x][y][z][1] = iter->second.color[1];
        values[x][y][z][2] = iter->second.color[2];
      }
    }

    {
#define VOX_EPS VEC_EPSILON
      AABB aabbNode = GetNodeAABB(nIdx);

      Vec3 vT = (vPos-aabbNode.min);
      Vec3 vNodeSize = aabbNode.GetSize();
      vT.x /= vNodeSize.x;
      vT.y /= vNodeSize.y;
      vT.z /= vNodeSize.z;

      assert(vT.x>=0 && vT.x<=1.f);
      assert(vT.y>=0 && vT.y<=1.f);
      assert(vT.z>=0 && vT.z<=1.f);

      ColorF arrfValues1D[2];

      for(int nX=0; nX<2; nX++)
      {
        ColorF fValue0 = 
          (1.f - vT.z)*values[nX][0][0] +
          (			 vT.z)*values[nX][0][1];

        ColorF fValue1 = 
          (1.f - vT.z)*values[nX][1][0] +
          (			 vT.z)*values[nX][1][1];

        arrfValues1D[nX] = (1.f - vT.y)*fValue0 + (vT.y)*fValue1;
      }

      resColor = (1.f - vT.x)*arrfValues1D[0] +(vT.x)*arrfValues1D[1];

      if(resColor.r || resColor.g || resColor.b)
        resColor.r = resColor.r;
    }

    return true;
  }
}


bool IsoOctree::GetMaterialByPosition(OctNode* node, const OctNode::NodeIndex& nIdx, Vec3 vPos, ColorF & resColor, SSurfTypeInfo & resSType, Vec3 & resNormal, SVoxValueCache * pVVC)
{
  if(node->children && (!GetCVars()->e_VoxTerHeightmapEditing || (m_pValues->find(OctNode::CenterIndex(nIdx,maxDepth)) != m_pValues->end())))
  {
    Vec3 ctr; float w;
    OctNode::CenterAndWidth(nIdx,ctr,w);
    int nFirst = ((vPos.x > ctr.x) ? 1 : 0) | ((vPos.y > ctr.y) ? 2 : 0) | ((vPos.z > ctr.z) ? 4 : 0);
    return GetMaterialByPosition(&node->children[nFirst],nIdx.child(nFirst), vPos, resColor, resSType, resNormal, pVVC);
  }
  else
  {
    if(pVVC->node != node)
    {
      pVVC->node = node;

      for(int c=0;c<Cube::CORNERS;c++)
      {
        int x,y,z;
        Cube::FactorCornerIndex(c,x,y,z);

        TVoxValues::iterator iter = (*m_pValues).find(OctNode::CornerIndex(nIdx,c,maxDepth));

        if(iter!=(*m_pValues).end())
        {
          pVVC->values[x][y][z] = iter->second;
        }
      }

      pVVC->vNormal.Set(0,0,0);
      for(int z=0; z<=1; z++)
      {
        pVVC->vNormal += (Vec3(0.f,0.f,z) - Vec3(0.5f,0.5f,0.5f)) * pVVC->values[0][0][z].val;
        pVVC->vNormal += (Vec3(1.f,0.f,z) - Vec3(0.5f,0.5f,0.5f)) * pVVC->values[1][0][z].val;
        pVVC->vNormal += (Vec3(0.f,1.f,z) - Vec3(0.5f,0.5f,0.5f)) * pVVC->values[0][1][z].val;
        pVVC->vNormal += (Vec3(1.f,1.f,z) - Vec3(0.5f,0.5f,0.5f)) * pVVC->values[1][1][z].val;
    }
      pVVC->vNormal.Normalize();
    }

    {
#define VOX_EPS VEC_EPSILON
      AABB aabbNode = GetNodeAABB(nIdx);

      Vec3 vT = (vPos-aabbNode.min);
      Vec3 vNodeSize = aabbNode.GetSize();
      vT.x /= vNodeSize.x;
      vT.y /= vNodeSize.y;
      vT.z /= vNodeSize.z;
/*
      assert(vT.x>=-0.5f && vT.x<=2.f);
      assert(vT.y>=-0.5f && vT.y<=2.f);
      assert(vT.z>=-0.5f && vT.z<=2.f);
*/
/*
      // must be static for now 
      PodArray<int> arrMatsNoDup; arrMatsNoDup.Clear();
      PodArray<float> arrWeightsNoDup; arrWeightsNoDup.Clear();

      { // materials
        for(int n=0; n<8; n++)
        {
          int surf_type = (&values[0][0][0])[n].surf_type.GetDomSType();

          assert(surf_type>=0 && surf_type<MAX_SURFACE_TYPES_COUNT);

          if(surf_type>=0)
            if(arrMatsNoDup.Find(surf_type)<0)
              arrMatsNoDup.Add(surf_type);
        }

        arrWeightsNoDup.PreAllocate(arrMatsNoDup.Count(),arrMatsNoDup.Count());

        float arrfValues1D[2];

        for(int i=0; i<arrMatsNoDup.Count(); i++)
        {
          int nMatId = arrMatsNoDup[i];

          for(int nX=0; nX<2; nX++)
          {
            float fValue0 = (1.f - vT.z)*(nMatId == values[nX][0][0].surf_type.GetDomSType()) + (vT.z)*(nMatId == values[nX][0][1].surf_type.GetDomSType());
            float fValue1 = (1.f - vT.z)*(nMatId == values[nX][1][0].surf_type.GetDomSType()) + (vT.z)*(nMatId == values[nX][1][1].surf_type.GetDomSType());
            arrfValues1D[nX] = (1.f - vT.y)*fValue0 + (vT.y)*fValue1;
          }

          arrWeightsNoDup[i] = (1.f - vT.x)*arrfValues1D[0] +(vT.x)*arrfValues1D[1];
        }

        if(pMats)
        {
          float fSumW = 0;
          for(int i=0; i<arrWeightsNoDup.Count(); i++)
            fSumW += arrWeightsNoDup[i];
          for(int i=0; i<arrWeightsNoDup.Count(); i++)
            arrWeightsNoDup[i] /= fSumW;

          memcpy(pWeig, arrWeightsNoDup.GetElements(), arrWeightsNoDup.GetDataSize());

          memcpy(pMats, arrMatsNoDup.GetElements(), arrMatsNoDup.GetDataSize());

          *pMatsNum = arrMatsNoDup.Count();
        }
      }*/

      {
        ColorF arrfValues1D[2];
//        ColorF arrfNorm1D[2];
        SSurfTypeInfo arrSType1D[2];

        for(int nX=0; nX<2; nX++)
        {
          {
            SSurfTypeInfo fValue0; fValue0.Lerp(vT.z, pVVC->values[nX][0][0].surf_type, pVVC->values[nX][0][1].surf_type);

            SSurfTypeInfo fValue1; fValue1.Lerp(vT.z, pVVC->values[nX][1][0].surf_type, pVVC->values[nX][1][1].surf_type);

            arrSType1D[nX].Lerp(vT.y, fValue0, fValue1);
          }
          {
            ColorF fValue0 = 
              (1.f - vT.z)*ColorF(pVVC->values[nX][0][0].color[0],pVVC->values[nX][0][0].color[1],pVVC->values[nX][0][0].color[2]) +
              (			 vT.z)*ColorF(pVVC->values[nX][0][1].color[0],pVVC->values[nX][0][1].color[1],pVVC->values[nX][0][1].color[2]);

            ColorF fValue1 = 
              (1.f - vT.z)*ColorF(pVVC->values[nX][1][0].color[0],pVVC->values[nX][1][0].color[1],pVVC->values[nX][1][0].color[2]) +
              (			 vT.z)*ColorF(pVVC->values[nX][1][1].color[0],pVVC->values[nX][1][1].color[1],pVVC->values[nX][1][1].color[2]);

            arrfValues1D[nX] = (1.f - vT.y)*fValue0 + (vT.y)*fValue1;
          }
/*          {
            ColorF fValue0 = 
              (1.f - vT.z)*ColorF(pVVC->values[nX][0][0].normal[0],pVVC->values[nX][0][0].normal[1],pVVC->values[nX][0][0].normal[2]) +
              (			 vT.z)*ColorF(pVVC->values[nX][0][1].normal[0],pVVC->values[nX][0][1].normal[1],pVVC->values[nX][0][1].normal[2]);

            ColorF fValue1 = 
              (1.f - vT.z)*ColorF(pVVC->values[nX][1][0].normal[0],pVVC->values[nX][1][0].normal[1],pVVC->values[nX][1][0].normal[2]) +
              (			 vT.z)*ColorF(pVVC->values[nX][1][1].normal[0],pVVC->values[nX][1][1].normal[1],pVVC->values[nX][1][1].normal[2]);

            arrfNorm1D[nX] = (1.f - vT.y)*fValue0 + (vT.y)*fValue1;
          }*/
        }

        resColor = (1.f - vT.x)*arrfValues1D[0] +(vT.x)*arrfValues1D[1];
/*        
        ColorF resNormalCol = (1.f - vT.x)*arrfNorm1D[0] +(vT.x)*arrfNorm1D[1];
        resNormal.x = resNormalCol.r/127 - 1;
        resNormal.y = resNormalCol.g/127 - 1;
        resNormal.z = resNormalCol.b/127 - 1;
*/
        resSType.Lerp(vT.x, arrSType1D[0], arrSType1D[1]);

        resNormal = pVVC->vNormal;
      }
    }

    return true;
  }
}

OctNode * IsoOctree::GetValueByPosition_BIN(OctNode* node, const OctNode::NodeIndex& nIdx, const Vec3 & vPos, int nMaxDepth)
{
  FUNCTION_PROFILER_3DENGINE;

  if(!node->children && nIdx.depth < nMaxDepth && node->nodeData.mcIndex>0 && node->nodeData.mcIndex<255)
  {
    node->initChildren();

#ifdef VOX_DVR
    node->nTmpChild = true;
#endif

    interpolateChilds(nIdx, node, m_pValues, m_pValues);

#ifdef VOX_EXPORT_OCTREE_AND_NORMALS
    interpolateChildsNormals(nIdx, node, &m_flatnessMap, &m_flatnessMap);
#endif

    for(int childId=0; childId<8; childId++)
    {
      float cValues[8];

      for(int i=0;i<Cube::CORNERS;i++)
      {
        ISO_KEY key = OctNode::CornerIndex(nIdx.child(childId),i,maxDepth);
        TVoxValues::iterator iter = (*m_pValues).find(key);
        if(iter==(*m_pValues).end())
          cValues[i] = 0.f;
        else
          cValues[i] = iter->second.val;
      }

      node->children[childId].nodeData.mcIndex = MarchingCubes::GetIndex(cValues,isoValue);
    }
  }

  if(node->children && nIdx.depth < nMaxDepth && node->nodeData.mcIndex>0 && node->nodeData.mcIndex<255)
  {
    Vec3 ctr; float w;
    OctNode::CenterAndWidth(nIdx,ctr,w);
    int nFirst = ((vPos.x > ctr.x) ? 1 : 0) | ((vPos.y > ctr.y) ? 2 : 0) | ((vPos.z > ctr.z) ? 4 : 0);
    return GetValueByPosition_BIN(&node->children[nFirst],nIdx.child(nFirst), vPos, nMaxDepth);
  }
  else
  {
    return (node->nodeData.mcIndex>0) ? node : NULL;// && node->nodeData.mcIndex<255;
  }
}

bool IsoOctree::GetValueByPosition(OctNode* node, const OctNode::NodeIndex& nIdx, const Vec3 & vPos, float * pVal, int nMaxDepth)
{
  if(node->children && nIdx.depth < nMaxDepth)
  {
    Vec3 ctr; float w;
    OctNode::CenterAndWidth(nIdx,ctr,w);
    int nFirst = ((vPos.x > ctr.x) ? 1 : 0) | ((vPos.y > ctr.y) ? 2 : 0) | ((vPos.z > ctr.z) ? 4 : 0);
    return GetValueByPosition(&node->children[nFirst],nIdx.child(nFirst), vPos, pVal, nMaxDepth);
  }
  else
  {
    float values[2][2][2];

    for(int c=0;c<Cube::CORNERS;c++)
    {
      int x,y,z;
      Cube::FactorCornerIndex(c,x,y,z);

      TVoxValues::iterator iter = (*m_pValues).find(OctNode::CornerIndex(nIdx,c,maxDepth));

      if(iter==(*m_pValues).end())
        values[x][y][z] = 0;
      else
        values[x][y][z] = iter->second.val;
    }

    {
#define VOX_EPS VEC_EPSILON
      AABB aabbNode = GetNodeAABB(nIdx);

      Vec3 vT = (vPos-aabbNode.min);
      Vec3 vNodeSize = aabbNode.GetSize();
      vT.x /= vNodeSize.x;
      vT.y /= vNodeSize.y;
      vT.z /= vNodeSize.z;

      float arrfValues1D[2];

      for(int nX=0; nX<2; nX++)
      {
        float fValue0 = 
          (1.f - vT.z)*values[nX][0][0] +
          (			 vT.z)*values[nX][0][1];

        float fValue1 = 
          (1.f - vT.z)*values[nX][1][0] +
          (			 vT.z)*values[nX][1][1];

        arrfValues1D[nX] = (1.f - vT.y)*fValue0 + (vT.y)*fValue1;
      }

      *pVal = (1.f - vT.x)*arrfValues1D[0] +(vT.x)*arrfValues1D[1];
    }

    return true;
  }
}

IsoOctree::IsoOctree(TVoxValues * pValues) 
{ 
  m_nSID=0;
  m_nCurrentNodeSlotId=0;
  m_pValues = pValues;
  m_nProgress = 0;
  editDepth = 0; 

  if(!nMeshDepth)
  {
    if(GetCVars()->e_VoxTerHeightmapEditing && GetCVars()->e_VoxTerMixMask)
    {
      GetCVars()->e_VoxTerTexResInternal = min(8, GetCVars()->e_VoxTerTexResInternal);
      GetCVars()->e_VoxTerMeshSize = max(4, GetCVars()->e_VoxTerMeshSize);
    }

    int nTerrainSize = Get3DEngine()->GetTerrainSize();
    while(nTerrainSize > GetCVars()->e_VoxTerMeshSize)
    {
      nTerrainSize /= 2;
      nMeshDepth = nMeshDepth+1;
    }
    PrintMessage("IsoOctree::IsoOctree: MinMeshSize=%d, MeshDepth=%d, TexResInternal=%d",(int)GetCVars()->e_VoxTerMeshSize,nMeshDepth,GetCVars()->e_VoxTerTexResInternal);
  }
/*
  maxDepth = 19;

  if(strstr(Get3DEngine()->GetLevelFilePath(""), "VoxelTerrainTest") || strstr(Get3DEngine()->GetLevelFilePath(""), "_MaxDepth14"))
    maxDepth = 14;

  if(strstr(Get3DEngine()->GetLevelFilePath(""), "ts_fxp") || strstr(Get3DEngine()->GetLevelFilePath(""), "_MaxDepth20") || strstr(Get3DEngine()->GetLevelFilePath(""), "CXP_benchmark_redesign_voxel") || strstr(Get3DEngine()->GetLevelFilePath(""), "VoxTerrainDemo"))
    maxDepth = 20;
*/
}

void IsoOctree::GetMemoryUsage(ICrySizer * pSizer)
{
  {
    SIZER_COMPONENT_NAME(pSizer, "IsoOctree_ValuesMap");
    pSizer->AddObject(&(*m_pValues),stl::size_of_map((*m_pValues)));
  }

  {
    SIZER_COMPONENT_NAME(pSizer, "IsoOctree_TmpValuesMap");
    pSizer->AddObject(&(m_tmpValues),stl::size_of_map((m_tmpValues)));
  }

  {
    SIZER_COMPONENT_NAME(pSizer, "IsoOctree_loadingCache");
    pSizer->AddObject(&m_loadingCache,m_loadingCache.GetDataSize());
  }

  {
    SIZER_COMPONENT_NAME(pSizer, "IsoOctree_threadTree");

    AUTO_LOCK(g_cIsoTreeAccess);

    int nSize = 0;
    tree.GetMemoryUsage(nSize);
    pSizer->AddObject(&tree,nSize);
  }

  {
    SIZER_COMPONENT_NAME(pSizer, "IsoOctree_RenderTree");
    int nSize = 0;
    renderTree.GetMemoryUsage(nSize);
    pSizer->AddObject(&renderTree,nSize);
  }

  {
    SIZER_COMPONENT_NAME(pSizer, "IsoOctree_threadMeshesMap");
    pSizer->AddObject(&threadMeshes,stl::size_of_map(threadMeshes));
  }

  {
    SIZER_COMPONENT_NAME(pSizer, "IsoOctree_threadEmptyMeshesMap");
    AUTO_LOCK(g_cIsoMeshes);
    pSizer->AddObject(&threadEmptyMeshes,stl::size_of_map(threadEmptyMeshes));
  }
}

void IsoOctree::RegisterLightSource(OctNode* node, const OctNode::NodeIndex& nIdx, CDLight * pLight)
{
  FUNCTION_PROFILER_3DENGINE;

  AABB WSBBox = GetNodeAABB(nIdx);
  WSBBox.min *= CVoxTerrain::m_fMapSize;
  WSBBox.max *= CVoxTerrain::m_fMapSize;

  if(!GetCamera().IsAABBVisible_F(WSBBox))
    return;

  if(!Overlap::Sphere_AABB(Sphere(pLight->m_Origin, pLight->m_fRadius), WSBBox))
    return;

  float fDistance = sqrt(Distance::Point_AABBSq(GetCamera().GetPosition(), WSBBox));

  float hw = (WSBBox.max.x-WSBBox.min.x)*0.5;

  bool bDraw = (fDistance*GetCVars()->e_VoxTerViewDistRatio>hw || !node->children || nIdx.depth >= nMeshDepth);

  ISO_KEY key=OctNode::CenterIndex(nIdx,maxDepth);

  TIsoMeshes & renderMeshes = *CIsoOctreeThread::pRenderMeshes;
//  TIsoMeshes & renderEmptyMeshes = *CIsoOctreeThread::pRenderEmptyMeshes;

  if( renderMeshes.find(key) != renderMeshes.end() )
  {
    SIsoMesh * pIsoMesh = renderMeshes[key];
    if(pIsoMesh->m_pRenderMesh)
      pIsoMesh->AddLightSource(pLight);
  }

  if(bDraw)
  {
  }
  else
  {
    for(int i=0;i<Cube::CORNERS;i++)
      RegisterLightSource(&node->children[i], nIdx.child(i), pLight);
  }
}

SIsoMesh * IsoOctree::PhysicalizeBox(const OctNode::NodeIndex& nIdx, const AABB & areaBox)
{
  FUNCTION_PROFILER_3DENGINE;

  SIsoMesh * pNewMesh = 0;

  IsoOctree * pNewIsoOctree = new IsoOctree(m_pValues);
  pNewIsoOctree->nKey.set(maxDepth);
  pNewIsoOctree->maxDepth = maxDepth;
  pNewIsoOctree->editDepth = editDepth;

  // copy only in nodes
  AABB aabbCutIn = areaBox;
  aabbCutIn.min+=Vec3(0.01f/CVoxTerrain::m_fMapSize,0.01f/CVoxTerrain::m_fMapSize,0.01f/CVoxTerrain::m_fMapSize);
  aabbCutIn.max-=Vec3(0.01f/CVoxTerrain::m_fMapSize,0.01f/CVoxTerrain::m_fMapSize,0.01f/CVoxTerrain::m_fMapSize);

  OctNode::NodeIndex nIdxGlobal;

  {
    FRAME_PROFILER( "PhysicalizeBox_CopyTree", GetSystem(), PROFILE_3DENGINE );

    AUTO_LOCK(g_cIsoTreeAccess);

    CopyTree(NULL, aabbCutIn, IsoOctree::nMeshDepth + (m_bEditor ? 2 : 1), &tree, nIdxGlobal, &pNewIsoOctree->tree, false, 0, 0, 0);
  }

  {
    FRAME_PROFILER( "PhysicalizeBox_CopyValues", GetSystem(), PROFILE_3DENGINE );

    AABB areaWSBox = aabbCutIn;
    areaWSBox.min *= CVoxTerrain::m_fMapSize;
    areaWSBox.max *= CVoxTerrain::m_fMapSize;

    pNewMesh = pNewIsoOctree->GenerateIsoMeshForPhysics(nIdx, false, areaWSBox);

    SAFE_DELETE(pNewIsoOctree);
  }

  return pNewMesh;
}

void IsoOctree::ClipMeshByAABB(const AABB & clipBox, std::vector<TriangleIndex> & tris, std::vector<Vec3> & vertices, std::vector<SSurfTypeInfo> & surf_types)
{
  Plane planes[6];

  Vec3 arrVerts[4] = 
  {
    Vec3(clipBox.min.x,clipBox.min.y,0),
    Vec3(clipBox.min.x,clipBox.max.y,0),
    Vec3(clipBox.max.x,clipBox.max.y,0),
    Vec3(clipBox.max.x,clipBox.min.y,0),
  };

  planes[0].SetPlane(arrVerts[0],arrVerts[1],arrVerts[1]+Vec3(0,0,-1));
  planes[1].SetPlane(arrVerts[1],arrVerts[2],arrVerts[2]+Vec3(0,0,-1));
  planes[2].SetPlane(arrVerts[2],arrVerts[3],arrVerts[3]+Vec3(0,0,-1));
  planes[3].SetPlane(arrVerts[3],arrVerts[0],arrVerts[0]+Vec3(0,0,-1));

  float fMinZ = clipBox.min.z;
  float fMaxZ = clipBox.max.z;

  planes[4].SetPlane(arrVerts[0]+Vec3(0,0,fMinZ),arrVerts[1]+Vec3(0,0,fMinZ),arrVerts[2]+Vec3(0,0,fMinZ));
  planes[5].SetPlane(arrVerts[0]+Vec3(0,0,fMaxZ),arrVerts[2]+Vec3(0,0,fMaxZ),arrVerts[1]+Vec3(0,0,fMaxZ));

  std::vector<uint16> indices;
  for(uint32 t=0; t<tris.size(); t++)
  {
    indices.push_back(tris[t][0]);
    indices.push_back(tris[t][1]);
    indices.push_back(tris[t][2]);
  }

  // clip triangles
  int nOrigCount = indices.size();
  for(int i=0; i<nOrigCount; i+=3)
  {
    if(ClipTriangle(indices, vertices, surf_types, i, &planes[0], 6))
    {
      i-=3;
      nOrigCount-=3;
    }
  }

  tris.resize(indices.size()/3);

  for(uint32 t=0; t<tris.size(); t++)
  {
    tris[t][0] = indices[t*3+0];
    tris[t][1] = indices[t*3+1];
    tris[t][2] = indices[t*3+2];
  }
}

void IsoOctree::ClipMeshByAABB(const AABB & clipBox, std::vector<uint16> & indices, std::vector<Vec3> & vertices, std::vector<SSurfTypeInfo> & surf_types)
{
  Plane planes[6];

  Vec3 arrVerts[4] = 
  {
    Vec3(clipBox.min.x,clipBox.min.y,0),
    Vec3(clipBox.min.x,clipBox.max.y,0),
    Vec3(clipBox.max.x,clipBox.max.y,0),
    Vec3(clipBox.max.x,clipBox.min.y,0),
  };

  planes[0].SetPlane(arrVerts[0],arrVerts[1],arrVerts[1]+Vec3(0,0,-1));
  planes[1].SetPlane(arrVerts[1],arrVerts[2],arrVerts[2]+Vec3(0,0,-1));
  planes[2].SetPlane(arrVerts[2],arrVerts[3],arrVerts[3]+Vec3(0,0,-1));
  planes[3].SetPlane(arrVerts[3],arrVerts[0],arrVerts[0]+Vec3(0,0,-1));

  float fMinZ = clipBox.min.z;
  float fMaxZ = clipBox.max.z;

  planes[4].SetPlane(arrVerts[0]+Vec3(0,0,fMinZ),arrVerts[1]+Vec3(0,0,fMinZ),arrVerts[2]+Vec3(0,0,fMinZ));
  planes[5].SetPlane(arrVerts[0]+Vec3(0,0,fMaxZ),arrVerts[2]+Vec3(0,0,fMaxZ),arrVerts[1]+Vec3(0,0,fMaxZ));

  // clip triangles
  int nOrigCount = indices.size();
  for(int i=0; i<nOrigCount; i+=3)
  {
    if(ClipTriangle(indices, vertices, surf_types, i, &planes[0], 6))
    {
      i-=3;
      nOrigCount-=3;
    }
  }
}

void CVoxTerrain::AddTextureForRelease(IDynTexture * pTex)
{
  AUTO_LOCK(g_cAddForRelease);

  if(pTex && m_arrTexturesForRel.Find(pTex)<0)
    m_arrTexturesForRel.Add(pTex);
}

void CVoxTerrain::ReleaseIsoMeshFromMainThread(SIsoMesh * pMesh, ISO_KEY key)
{
  AUTO_LOCK(g_cAddForRelease);

  assert(pMesh->m_key == key);

  if(!key)
    pMesh->Dephysicalize();

  pMesh->ReleaseTextures();
  //SAFE_DELETE(pMesh->m_pMeshForSerialization);
  SAFE_DELETE(pMesh->m_pGeneratedMesh);

  if(pMesh && m_arrIsoMeshesForDel.Find(pMesh)<0)
    m_arrIsoMeshesForDel.Add(pMesh);

  if(!GetCVars()->e_VoxTerHeightmapEditing)
    SAFE_DELETE(pMesh->m_pZipData);
}

extern SBrushAABB * GetBrushAABB(const Sphere & sp, EVoxelEditOperation eOperation, float fMapSize, float fEpsilon);

void IsoOctree::DoVoxelShapeTask(Vec3 vWSPos, float fRadius, int nSurfaceTypeId, ColorB color, EVoxelEditOperation eOperation, EVoxelBrushShape eShape, 
  int nDetailLevel, const char * szCGF, std::vector<Vec3> * vertices, std::vector<int> * indices, AABB meshBox, int nProgressCur, int nProgressAll, float editingTime, bool bAllowRequestTreeUpdate)
{
  FUNCTION_PROFILER_3DENGINE;

  AUTO_LOCK(g_cIsoTreeAccess);

  m_flatnessMap.clear();

  float fOperationStartTime = GetCurAsyncTimeSec();
  
  IsoOctree_nModId++;

  IsoOctree & isoTree = *this;

  Sphere sp(vWSPos/CVoxTerrain::m_fMapSize, fRadius/CVoxTerrain::m_fMapSize);

  float fEpsilon = 1.f;
  for(int d=0; d<IsoOctree::maxDepth; d++)
    fEpsilon /= 2;
  fEpsilon /= 2;

  SBrushAABB brushAABB = *GetBrushAABB(sp, eOperation, 1.f, fEpsilon);

  if(eOperation == eveoIntegrateMeshNeg || eOperation == eveoIntegrateMeshPos)
    m_aabbDirty.Add(brushAABB.boxEps);
  else
    m_aabbDirty = brushAABB.boxEps;

  if(nSurfaceTypeId<0)
  {
    ColorF resColor;
    SSurfTypeInfo surf_type;
    SVoxValueCache voxValueCache;
    Vec3 vNormal;
    CVoxTerrain::GetVoxelValueInterpolated(&isoTree, sp.center*CVoxTerrain::m_fMapSize, resColor, 0, 0, surf_type, vNormal, &voxValueCache);
    nSurfaceTypeId = surf_type.GetDomSType();
  }

  bool bOldValuesCollected = false;
  TVoxValues valuesBefore;
  OctNode::NodeIndex nIdxGlobal;
  AABB aabbAreaExEx = brushAABB.boxEps;
  aabbAreaExEx.min -= Vec3(brushAABB.spEps.radius,brushAABB.spEps.radius,brushAABB.spEps.radius);
  aabbAreaExEx.max += Vec3(brushAABB.spEps.radius,brushAABB.spEps.radius,brushAABB.spEps.radius);

  if( brushAABB.spEps.radius<0.1f )
    if(!m_bEditor || (CVoxTerrain::m_bAllowEditing && (eOperation == eveoLimitLod) && GetCVars()->e_VoxelLodsNum))
  {
    PrintMessage("Collect previous values ...");
    isoTree.CollectValues(&isoTree.tree, nIdxGlobal, valuesBefore, &aabbAreaExEx);
    bOldValuesCollected = true;
    PrintMessagePlus(" ok");
  }

  bool bPrint = (!GetCVars()->e_VoxTerHeightmapEditing || brushAABB.spEps.radius>0.5f);

  if(m_bEditor && (eOperation != eveoForceDepth || !(nProgressCur&15)) && (GetCVars()->e_VoxTerLog))
    if(bPrint)
      PrintMessage("Executing operation: %s (%d) ...", Get3DEngine()->GetVoxelEditOperationName(eOperation), nDetailLevel);

  {
    isoTree.editDepth = nDetailLevel;

    if(eOperation == eveoLimitLod)
    {
      OctNode::NodeIndex nIdx;
      isoTree.LimitDepth(&tree, nIdx, sp, &brushAABB);
    }
    else if(eOperation == eveoIntegrateMeshPos || eOperation == eveoIntegrateMeshNeg)
    {
      mesh_compiler::CMeshCompiler meshCompiler;
      int nVertNum = vertices->size();
      meshBox.min -= Vec3(0.1f,0.1f,0.1f);
      meshBox.max += Vec3(0.1f,0.1f,0.1f);
      meshCompiler.WeldPositions( &((*vertices)[0]), nVertNum, *indices, 0.025f, meshBox );
      vertices->resize(nVertNum);

      for(uint32 i=0; i<vertices->size(); i++)
        (*vertices)[i] = (*vertices)[i] / CVoxTerrain::m_fMapSize;

      meshBox.min /= CVoxTerrain::m_fMapSize;
      meshBox.max /= CVoxTerrain::m_fMapSize;

      std::vector< std::vector<int> > * polygons = new std::vector< std::vector<int> >;

      for(uint32 i=0; i<(*indices).size(); i+=3)
      {
        assert((*indices)[i+0]>=0 && (*indices)[i+0]<nVertNum);
        assert((*indices)[i+1]>=0 && (*indices)[i+1]<nVertNum);
        assert((*indices)[i+2]>=0 && (*indices)[i+2]<nVertNum);

        int i0 = CLAMP((*indices)[i+0],0,nVertNum-1);
        int i1 = CLAMP((*indices)[i+1],0,nVertNum-1);
        int i2 = CLAMP((*indices)[i+2],0,nVertNum-1);

        if(i0==i1 || i1==i2 || i2==i0)
          continue;

        std::vector<int> triangle;
        triangle.push_back(i0);
        triangle.push_back(i1);
        triangle.push_back(i2);

        polygons->push_back(triangle);
      }

      PrintMessagePlus(" %d tris ...", polygons->size());

      if(CVoxTerrain::m_bAllowEditing)
      {
    /*    if(nProgressCur > (nProgressAll/6) && GetCurTimeSec()-editingTime > 60)
        {
          float fTimeLeft = (GetCurTimeSec()-editingTime) / nProgressCur * (nProgressAll - nProgressCur);
          PrintMessagePlus(" Integrating (%d/%d, %d min left) ...", nProgressCur, nProgressAll, (int)(fTimeLeft/60));
        }
        else*/
          PrintMessagePlus(" Integrating (%d/%d) ...", nProgressCur, nProgressAll);
				
				if(polygons->size())
					isoTree.setFromMesh(*vertices, *polygons, isoTree.maxDepth, 0, fIsoTreeCurvature, true, nSurfaceTypeId, color, &brushAABB, eOperation);
      }

      delete polygons;
    }
    else if(eOperation == eveoForceDepth)
    {
      OctNode::NodeIndex nIdx;
      isoTree.ForceDepth(&tree, nIdx, sp, &brushAABB, eOperation, NULL);
    }
    else
    {
      IsoOctree * isoTreeBk = NULL;

      if(eOperation == eveoBlurPos || eOperation == eveoBlurNeg)
      {
        FRAME_PROFILER( "DoVoxelShape_isoTreeBk", GetSystem(), PROFILE_3DENGINE );

        isoTreeBk = new IsoOctree(new TVoxValues);
        isoTreeBk->nKey.set(isoTree.maxDepth);
        isoTreeBk->maxDepth = isoTree.maxDepth;
        isoTreeBk->editDepth = isoTree.editDepth;

        // copy nodes required for triangulation

        AABB aabbArea = brushAABB.boxEps;
        isoTree.CopyTree(NULL, aabbArea, isoTree.editDepth, &isoTree.tree, nIdxGlobal, &isoTreeBk->tree, false, 0, 0, 0);

        // copy values
        TVoxValues tempValues;
        isoTreeBk->nKey.set(isoTree.maxDepth);
        OctNode::NodeIndex nIdx;
        for(OctNode* temp=isoTreeBk->tree.nextLeaf(NULL,nIdx) ; temp ; temp=isoTreeBk->tree.nextLeaf(temp,nIdx) )
        {
          for(int i=0;i<Cube::CORNERS;i++)
          {
            ISO_KEY key=OctNode::CornerIndex(nIdx,i,isoTree.maxDepth);
            (*isoTreeBk->m_pValues)[key] = (*isoTree.m_pValues)[key];
          }
        }
      }

      {
        FRAME_PROFILER( "DoVoxelShape_ForceDepth", GetSystem(), PROFILE_3DENGINE );

        OctNode::NodeIndex nIdx;
        isoTree.ForceDepth(&tree, nIdx, sp, &brushAABB, eOperation, GetTerrain()->GetParentNode(0));
      }

      if(m_bEditor && (eOperation != eveoForceDepth || !(nProgressCur&15)) && (GetCVars()->e_VoxTerLog))
      {
        if(bPrint)
          PrintMessagePlus(" (%.1f sec)", GetCurAsyncTimeSec()-fOperationStartTime);
        fOperationStartTime = GetCurAsyncTimeSec();
      }

      {
        FRAME_PROFILER( "DoVoxelShape_setFromSphere", GetSystem(), PROFILE_3DENGINE );
        isoTree.setFromSphere(sp, maxDepth, 0, fIsoTreeCurvature, eOperation, 0, nSurfaceTypeId, color, &brushAABB, isoTreeBk);
      }

      SAFE_DELETE(isoTreeBk);
    }
  }

  if(m_bEditor && (eOperation != eveoForceDepth || !(nProgressCur&15)) && (GetCVars()->e_VoxTerLog))
    if(bPrint)
      PrintMessagePlus(" (%.1f sec)", GetCurAsyncTimeSec()-fOperationStartTime);

  // free wasted elements
  if(!m_bEditor || (CVoxTerrain::m_bAllowEditing && (eOperation == eveoLimitLod) && GetCVars()->e_VoxelLodsNum))
  {
    FRAME_PROFILER( "DoVoxelShape::OptimizeTree", GetSystem(), PROFILE_3DENGINE );

    std::map<OctNode*,OctNode*> detachedChilds;
    AABB aabbBrushEx = brushAABB.boxEps;

    isoTree.CutOutsideOfBox(aabbBrushEx, &isoTree.tree, nIdxGlobal, detachedChilds);
    isoTree.OptimizeTree(&aabbBrushEx, fIsoTreeCurvature, &detachedChilds,0/*min(isoTree.nMeshDepth,isoTree.editDepth)*/,true);
    isoTree.RestoreChilds(detachedChilds);

    if(bOldValuesCollected)
    {
      TVoxValues valuesAfter;

      PrintMessage("Collect new values ...");

      isoTree.CollectValues(&isoTree.tree, nIdxGlobal, valuesAfter, &aabbAreaExEx);

      PrintMessagePlus(" cleanup ...");

      int nDeleted = 0;
      for(TVoxValues::const_iterator iter=valuesBefore.begin(); iter!=valuesBefore.end(); iter++)
      {
        if(valuesAfter.find(iter->first) == valuesAfter.end())
        {
          m_pValues->erase(iter->first);
          nDeleted++;
        }
      }

      int nDiff = ((ISO_KEY)valuesBefore.size() - (ISO_KEY)valuesAfter.size())/1024;

      if(nDiff>0)
        PrintMessagePlus(" (%d K values removed)", nDiff);
      else
        PrintMessagePlus(" (%d K values added)", -nDiff);
    }
    else
    {
      CleanUpValues();

#ifdef WIN32
      SetProcessWorkingSetSize( GetCurrentProcess(),-1,-1 );
#endif //WIN32
    }
  }

  // refresh mesh
  if(!m_aabbDirty.IsReset() && bAllowRequestTreeUpdate)
  {
    m_aabbDirty.min -= Vec3(brushAABB.fEps,brushAABB.fEps,brushAABB.fEps);
    m_aabbDirty.max += Vec3(brushAABB.fEps,brushAABB.fEps,brushAABB.fEps);

    if(GetCVars()->e_VoxTerHeightmapEditing)
    {
      m_aabbDirty = brushAABB.boxEps;

      if(eOperation == eveoCopyTerrainPos)
      {
        m_aabbDirty.min.z -= 40.f;
        m_aabbDirty.max.z += 40.f;
      }
    }

    if(m_aabbDirty.GetRadius() > Get3DEngine()->GetTerrainSize()/2)
    {
      PrintMessagePlus(" Removing all cache files ...");
      char szFileName[256]="";
      SIsoMesh::MakeTempTextureFileName(szFileName, sizeof(szFileName),0, "MTT", 0);
      DeleteFolder(szFileName,true);
      SIsoMesh::MakeTempTextureFileName(szFileName, sizeof(szFileName),0, "BIN", 0);
      DeleteFolder(szFileName,true);
    }

    if((GetCVars()->e_VoxTerLog))
      if(bPrint)
  			PrintMessagePlus(" Request mesh update (%.1f) ...", m_aabbDirty.GetRadius()*CVoxTerrain::m_fMapSize);

    isoTree.RequestMeshUpdate(&isoTree.tree, nIdxGlobal, m_aabbDirty, brushAABB.boxEps);

    if((GetCVars()->e_VoxTerLog))
      if(bPrint)
        PrintMessagePlus(" ok");

    m_lastEditingTime = GetCurTimeSec();

    if(CIsoOctreeThread::m_nMessagesIn<1024 && m_nodesForUpdate.Count())
      CIsoOctreeThread::MSGI_Update(true);
  }

  // refresh physics
  if(&brushAABB && GetCVars()->e_VoxelUpdatePhysics)
  {
    if(eOperation != eveoMaterial && eOperation != eveoBaseColor)
    {
      AABB WSBBox = brushAABB.boxEps;
      WSBBox.min *= CVoxTerrain::m_fMapSize;
      WSBBox.max *= CVoxTerrain::m_fMapSize;

      m_aabbPhysDirty.Add(WSBBox);
//      gEnv->pPhysicalWorld->RegisterBBoxInPODGrid(&WSBBox.min);
    }
  }
}

IsoOctree::~IsoOctree()
{
  IsoOctree * pMainIsoOctree = CIsoOctreeThread::m_pIsoOctree;

  if(pMainIsoOctree == this)
    CVoxTerrain::m_bAllowEditing = false;

  if(this != pMainIsoOctree)
  {
    ShutDown();
    m_pValues = NULL;
  }
  else
  {
    AUTO_LOCK(g_cIsoTreeAccess);
    ShutDown();

		if(m_pValues)
		{
			if(m_pValues->size())
				PrintMessage("Deleting values");
			SAFE_DELETE(m_pValues)
		}
  }
}

template <class T> 
bool IsoOctree::Load_T(T * & f, int & nDataSize, SIsoTreeChunkHeader * pTerrainChunkHeader, bool bUpdateMesh, EEndian eEndian, AABB * pAreaBox, int nSID)
{
  LOADING_TIME_PROFILE_SECTION(GetSystem());

  CVoxTerrain::SetStreamingActivityMax(nDataSize);

  if(pTerrainChunkHeader->nVersion != ISOTREE_CHUNK_VERSION_BC && pTerrainChunkHeader->nVersion != ISOTREE_CHUNK_VERSION)
  {
    assert(!"Version error");
    Error("CIsoTree::Load_T: version of file is %d, expected version is %d", pTerrainChunkHeader->nVersion, (int)ISOTREE_CHUNK_VERSION);
    return 0;
  }

  if(pTerrainChunkHeader->nChunkSize != nDataSize+sizeof(SIsoTreeChunkHeader))
    return 0;

  IsoOctree_nModId = pTerrainChunkHeader->TerrainInfo.nModId;

  if(m_bEditor)
	  DeleteTooNewCacheItems();

  if(!pAreaBox)
  { // delete all
    (*m_pValues).clear();
//    (m_flatnessMap).clear();
  }
  else
  { // delete only in area
    OctNode::NodeIndex nIdx;
    m_tmpValues.clear();
    AABB areaBoxNorm = *pAreaBox;
    areaBoxNorm.min /= CVoxTerrain::m_fMapSize;
    areaBoxNorm.max /= CVoxTerrain::m_fMapSize;
    CollectValues(&tree,nIdx,m_tmpValues,&areaBoxNorm);
    for(TVoxValues::const_iterator iter=(m_tmpValues).begin();iter!=(m_tmpValues).end();iter++)
      m_pValues->erase(iter->first);
//    (m_flatnessMap).clear();

    PrintMessage("Request mesh update ...");
    RequestMeshUpdate(&tree, nIdx, areaBoxNorm, areaBoxNorm);
    PrintMessagePlus(" ok");
  }

  ShutDown();

  int nInitMemUsageMB=0;
  int nDataNodesLoaded=0;

  { // tree loading (only for game)
    if(!m_bEditor)
      PrintMessage("Loading tree ...");
    nInitMemUsageMB = GetEngineMemoryUsageMB();
    nDataNodesLoaded = tree.Load_T(f,nDataSize,eEndian);
    if(!m_bEditor)
      PrintMessagePlus(" %d MB allocated in %d K items", GetEngineMemoryUsageMB()-nInitMemUsageMB, nDataNodesLoaded/1024);
    nInitMemUsageMB = GetEngineMemoryUsageMB();
  }

  maxDepth = 0;
  if(!CTerrain::LoadDataFromFile(&maxDepth, 1, f, nDataSize, eEndian))
    return 0;

  {
    int nValuesNum=0;
    if(!CTerrain::LoadDataFromFile(&nValuesNum, 1, f, nDataSize, eEndian))
      return 0;

    {
      PrintMessage("Loading values (item size = %d, %d M items) ...", pTerrainChunkHeader->TerrainInfo.nSVoxValueSize, nValuesNum/1024/1024);
      nInitMemUsageMB = GetEngineMemoryUsageMB();
			float fTime = GetCurAsyncTimeSec();

      if(pTerrainChunkHeader->TerrainInfo.nSVoxValueSize==sizeof(SVoxValue))
      {
        for(int i=0;i<nValuesNum;i++)
        {
          ISO_KEY key;
          if(!CTerrain::LoadDataFromFile(&key, 1, f, nDataSize, eEndian))
            return 0;
          SVoxValue voxVal;
          if(!CTerrain::LoadDataFromFile(&voxVal, 1, f, nDataSize, eEndian))
            return 0;
          (*m_pValues)[key] = voxVal;

          if((i&(1024*1024-1))==0 && i)
            PrintMessage(" %d M of %d M items loaded", i/1024/1024, nValuesNum/1024/1024);
        }
      }
      else 
      {
        // old format
      if(pTerrainChunkHeader->nVersion == ISOTREE_CHUNK_VERSION_BC)
      {
        for(int i=0;i<nValuesNum;i++)
        {
          ISO_KEY key;
          if(!CTerrain::LoadDataFromFile(&key, 1, f, nDataSize, eEndian))
            return 0;
          SVoxValueOld voxValOld;
          if(!CTerrain::LoadDataFromFile(&voxValOld, 1, f, nDataSize, eEndian))
            return 0;
          SVoxValue voxVal;
          voxVal.color[0] = voxValOld.color[0];
          voxVal.color[1] = voxValOld.color[1];
          voxVal.color[2] = voxValOld.color[2];
          voxVal.surf_type = voxValOld.surf_type;
          voxVal.val = voxValOld.val;
          (*m_pValues)[key] = voxVal;
        }
      }
      else
      {
        for(int i=0;i<nValuesNum;i++)
        {
          ISO_KEY key;
          if(!CTerrain::LoadDataFromFile(&key, 1, f, nDataSize, eEndian))
            return 0;
            SVoxValueOld2 voxValOld2;
            if(!CTerrain::LoadDataFromFile(&voxValOld2, 1, f, nDataSize, eEndian))
              return 0;
          SVoxValue voxVal;
            voxVal.color[0] = voxValOld2.color[0];
            voxVal.color[1] = voxValOld2.color[1];
            voxVal.color[2] = voxValOld2.color[2];
            voxVal.normal[0] = 127;
            voxVal.normal[1] = 127;
            voxVal.normal[2] = 127;
            voxVal.surf_type = voxValOld2.surf_type;
            voxVal.val = voxValOld2.val;
          (*m_pValues)[key] = voxVal;
        }
      }
      }

      PrintMessagePlus(" %d MB allocated in %d K items (%.1f sec)", GetEngineMemoryUsageMB()-nInitMemUsageMB, nValuesNum/1024, GetCurAsyncTimeSec()-fTime);
      nInitMemUsageMB = GetEngineMemoryUsageMB();
    }
  }

#ifdef VOX_EXPORT_OCTREE_AND_NORMALS
  if(tree.children && (*m_pValues).size())
  {
    int nValuesNum=0;
    if(!CTerrain::LoadDataFromFile(&nValuesNum, 1, f, nDataSize, eEndian))
      return 0;

    PrintMessage("Loading normals ...");
    nInitMemUsageMB = GetEngineMemoryUsageMB();
    for(int i=0;i<nValuesNum;i++)
    {
      ISO_KEY key;
      if(!CTerrain::LoadDataFromFile(&key, 1, f, nDataSize, eEndian))
        return 0;
      SFlatnessItem voxVal;
      if(!CTerrain::LoadDataFromFile(&voxVal, 1, f, nDataSize, eEndian))
        return 0;
      (m_flatnessMap)[key] = voxVal;
    }
    PrintMessagePlus(" %d MB allocated in %d K items", GetEngineMemoryUsageMB()-nInitMemUsageMB, nValuesNum/1024);
    nInitMemUsageMB = GetEngineMemoryUsageMB();
  }
#endif

  if(m_bEditor && !tree.children)
  { // build tree from values (only for editor)
    PrintMessage("Reconstructing tree ...");
    nInitMemUsageMB = GetEngineMemoryUsageMB();
    OctNode::NodeIndex nIdx;
    tree.deleteChildren(m_aabbDirty, GetNodeAABB(nIdx));
    nDataNodesLoaded = ReconstructTree(maxDepth, &tree, nIdx);   
    if(GetCVars()->e_VoxTerDebug)
    {
/*      if(GetCVars()->e_VoxTerTexBuildOnCPU)
      {
        PrintMessagePlus(" setMCIndex and normals ...");
        m_flatnessMap.clear();
        setNormalFlatness(m_flatnessMap);
      }
      else*/
      {
        PrintMessagePlus(" setMCIndex ...");
        setMCIndex(0);
      }
    }
    PrintMessagePlus(" %d MB allocated in %d K items", GetEngineMemoryUsageMB()-nInitMemUsageMB, nDataNodesLoaded/1024);
    nInitMemUsageMB = GetEngineMemoryUsageMB();
  }

  if(m_bEditor && !GetCVars()->e_VoxTerHeightmapEditing)
  {
    OctNode::NodeIndex _nIdx;
    RegisterBBoxInPODGrid(&tree, _nIdx);
  }

  int nMeshesNum=0;
  if(!CTerrain::LoadDataFromFile(&nMeshesNum, 1, f, nDataSize, eEndian))
    return 0;

  {
    PrintMessage("Indexing meshes ..");
    nInitMemUsageMB = GetEngineMemoryUsageMB();

    threadMeshes.clear();

    for(int i=0;i<nMeshesNum;i++)
    {
      ISO_KEY key;

      if(!CTerrain::LoadDataFromFile(&key, 1, f, nDataSize, eEndian))
        return 0;

      SIsoMesh * pIsoMesh = NULL;

      if( threadMeshes.find(key) != threadMeshes.end() )
      {
        pIsoMesh = threadMeshes[key];
        SAFE_DELETE(pIsoMesh); 
        threadMeshes.erase(key);
      }

      pIsoMesh = new SIsoMesh(key);

      pIsoMesh->m_nMeshFileOffset = 0;
      pIsoMesh->m_nMeshFileDataSize = nDataSize;

      int nMeshDataSize=0;
      if(!CTerrain::LoadDataFromFile(&nMeshDataSize, 1, f, nDataSize, eEndian,&pIsoMesh->m_nMeshFileOffset))
        return 0;

      if(nMeshDataSize)
      {
        // PrintMessage("Loading node mesh (%d kb) ... ", (int)(nMeshDataSize)/1024);

        if(m_bEditor)
        {
          m_loadingCache.PreAllocate(nMeshDataSize,nMeshDataSize);

          if(!CTerrain::LoadDataFromFile((uint8*)m_loadingCache.GetElements(), nMeshDataSize, f, nDataSize, eEndian))
            return 0;

          CTerrain::LoadDataFromFile_FixAllignemt(f,nDataSize);

          CMemoryBlock * pMB = new CMemoryBlock();
          pMB->SetData(m_loadingCache.GetElements(), nMeshDataSize);
          pIsoMesh->m_pMeshForSerialization = pMB;
        }
        else
        {
          if(!CTerrain::LoadDataFromFile_Seek(nMeshDataSize, f, nDataSize, eEndian))
            return 0;

          CTerrain::LoadDataFromFile_FixAllignemt(f,nDataSize);
        }

        pIsoMesh->InitMaterials(nSID);

        if(m_bEditor)
          pIsoMesh->MakeRenderMesh();

        // process texture info

        uint32 arrSize[2]={0,0};

        if(!CTerrain::LoadDataFromFile(arrSize, 2, f, nDataSize, eEndian))
          return 0;

        for(int t=0; t<2; t++)
        {
          if(arrSize[t])
          {            
            if(!CTerrain::LoadDataFromFile_Seek(arrSize[t], f, nDataSize, eEndian))
              return 0;

            CTerrain::LoadDataFromFile_FixAllignemt(f,nDataSize);
          }
        }

        if(!CTerrain::LoadDataFromFile(&pIsoMesh->m_nTexDimX, 1, f, nDataSize, eEndian))
          return 0;

        if(!CTerrain::LoadDataFromFile(&pIsoMesh->m_nTexDimY, 1, f, nDataSize, eEndian))
          return 0;       

        if(!CTerrain::LoadDataFromFile(&pIsoMesh->m_WSBBox, 1, f, nDataSize, eEndian))
          return 0;  

        if(!CTerrain::LoadDataFromFile(&pIsoMesh->m_arrTexRangeInfo[0][0], sizeof(pIsoMesh->m_arrTexRangeInfo)/sizeof(float), f, nDataSize, eEndian))
          return 0;  
      }

      pIsoMesh->m_nMeshFileDataSize -= nDataSize;

      threadMeshes[key] = pIsoMesh;

      PrintProgress();
    }

    PrintMessagePlus(" %d MB allocated in %d items", GetEngineMemoryUsageMB()-nInitMemUsageMB, nMeshesNum);
    nInitMemUsageMB = GetEngineMemoryUsageMB();
  }

  {
    if(GetCVars()->e_VoxTerDebug)
      m_flatnessMap.clear();

    int nValuesNum=0;
    if(!CTerrain::LoadDataFromFile(&nValuesNum, 1, f, nDataSize, eEndian))
      return 0;

    PrintMessage("Loading normals ...");
    nInitMemUsageMB = GetEngineMemoryUsageMB();
    for(int i=0;i<nValuesNum;i++)
    {
      ISO_KEY key;
      if(!CTerrain::LoadDataFromFile(&key, 1, f, nDataSize, eEndian))
        return 0;
      SFlatnessItem voxVal;
      if(!CTerrain::LoadDataFromFile(&voxVal, 1, f, nDataSize, eEndian))
        return 0;

      if(GetCVars()->e_VoxTerDebug)
        (m_flatnessMap)[key] = voxVal;
    }
    PrintMessagePlus(" %d MB allocated in %d K items", GetEngineMemoryUsageMB()-nInitMemUsageMB, nValuesNum/1024);
    nInitMemUsageMB = GetEngineMemoryUsageMB();
  }

  {
    PrintMessage("Loading layers ...");
    nInitMemUsageMB = GetEngineMemoryUsageMB();

    int nLayersNum=0;
    if(!CTerrain::LoadDataFromFile(&nLayersNum, 1, f, nDataSize, eEndian))
      return 0;

    if(nLayersNum)
    {
      Get3DEngine()->m_arrBaseTextureData.PreAllocate(nLayersNum,nLayersNum);

      for(int nLayerId=0; nLayerId<nLayersNum; nLayerId++)
      {
        SImageInfo & rImgInfo = Get3DEngine()->m_arrBaseTextureData[nLayerId];

        // release old data
//        for(int nMip=0; nMip<SImageSubInfo::nMipsNum; nMip++)
  //        SAFE_DELETE_ARRAY(rImgInfo.detailInfo.pImgMips[nMip]);

        // load all params and fake pointers
        int oldId = rImgInfo.nPhysSurfaceType;
        if(!CTerrain::LoadDataFromFile(&rImgInfo, 1, f, nDataSize, eEndian))
          return 0;
        if(m_bEditor)
          rImgInfo.nPhysSurfaceType = oldId;
        ZeroStruct(rImgInfo.arrTextureId);

        ZeroStruct(rImgInfo.baseInfo.pImgMips);
        ZeroStruct(rImgInfo.detailInfo.pImgMips);
      }
    }

    PrintMessagePlus(" %d MB allocated in %d items", GetEngineMemoryUsageMB()-nInitMemUsageMB, nLayersNum);
    nInitMemUsageMB = GetEngineMemoryUsageMB();
  }

  {
    AUTO_LOCK(g_cIsoMeshes);

    PrintMessage("Loading empty keys ...");
    nInitMemUsageMB = GetEngineMemoryUsageMB();

    int nEmptyKeysNum=0;
    if(!CTerrain::LoadDataFromFile(&nEmptyKeysNum, 1, f, nDataSize, eEndian))
      return 0;

    threadEmptyMeshes.clear();
    for(int i=0;i<nEmptyKeysNum;i++)
    {
      ISO_KEY key;
      if(!CTerrain::LoadDataFromFile(&key, 1, f, nDataSize, eEndian))
        break;

      threadEmptyMeshes[key] = NULL;
    }

    PrintMessagePlus(" %d MB allocated in %d items", GetEngineMemoryUsageMB()-nInitMemUsageMB, nEmptyKeysNum);
    nInitMemUsageMB = GetEngineMemoryUsageMB();
  }

  SAFE_DELETE(CIsoOctreeThread::pRenderMeshes);
  SAFE_DELETE(CIsoOctreeThread::pRenderEmptyMeshes);

  CIsoOctreeThread::pRenderMeshes = new TIsoMeshes;
  CIsoOctreeThread::pRenderEmptyMeshes = new TIsoMeshes;

  {
    AUTO_LOCK(g_cIsoMeshes);

    *CIsoOctreeThread::pRenderMeshes = threadMeshes;
    *CIsoOctreeThread::pRenderEmptyMeshes = threadEmptyMeshes;
  }

  OctNode::NodeIndex nIdx;
  renderTree.deleteChildren(m_aabbDirty, GetNodeAABB(nIdx));
  AABB nodeBox = GetNodeAABB(nIdx);
  CopyTree(NULL, nodeBox, nMeshDepth, &tree, nIdx, &renderTree, true, 0, 0, 0);

  assert(nDataNodesLoaded && nDataSize==0);

  if(nDataSize)
  {
    const char * szMessage = "Non critical data corruption detected during voxel terrain loading\nEngine will try to load data anyway";
    if(m_bEditor)
      CryMessageBox(szMessage,"Voxel terrain loading error",0x00000000L);
    else
      Error(szMessage);

    nDataSize=0;
  }
/*
  {
    PrintMessage("Update normals ...");

    OctNode::NodeIndex nIdx;
    m_flatnessMap.clear();
    UpdateNormals(&tree, nIdx, m_flatnessMap);

    for(TFlatness::iterator iterFlat=m_flatnessMap.begin(); iterFlat!=m_flatnessMap.end(); iterFlat++)
    {
      TVoxValues::iterator iterVal = (*m_pValues).find(iterFlat->first);
      if(iterVal != (*m_pValues).end())
      {
        iterFlat->second.first.Normalize();
        SVoxValue & r = iterVal->second;
        r.normal[0] = (uint8)SATURATEB(iterFlat->second.first[0]*127.f+127.f);
        r.normal[1] = (uint8)SATURATEB(iterFlat->second.first[1]*127.f+127.f);
        r.normal[2] = (uint8)SATURATEB(iterFlat->second.first[2]*127.f+127.f);
      }
    }

    m_flatnessMap.clear();
    PrintMessagePlus(" ok");
  }
*/
  return (nDataNodesLoaded && nDataSize==0);
}

bool CVoxTerrain::IsContinue()
{
  return CIsoOctreeThread::m_pIsoOctree && CIsoOctreeThread::bContinue && !CIsoOctreeThread::IsEscapePressed() && m_bAllowEditing;
}

bool IsoOctree::RayIntersection(OctNode* node, const OctNode::NodeIndex& nIdx, const Ray & r, Vec3 & vHitPoint)
{
  const AABB & nodeBox = GetNodeAABB(nIdx);
  AABB nodeBoxEx = nodeBox;
  float fBorder = 0.025f/CVoxTerrain::m_fMapSize;
  nodeBoxEx.Expand(Vec3(fBorder,fBorder,fBorder));

  Vec3 arrP[2];
  if(!Intersect::Ray_AABB(r,nodeBoxEx,arrP[0]))
    return 0;

  Vec3 ctr; float w;
  OctNode::CenterAndWidth(nIdx,ctr,w);

  if(node->children)
  {
    int nFirst = 
      ((r.origin.x > ctr.x) ? 1 : 0) |
      ((r.origin.y > ctr.y) ? 2 : 0) |
      ((r.origin.z > ctr.z) ? 4 : 0);

    if(RayIntersection(&node->children[nFirst  ],nIdx.child(nFirst  ), r, vHitPoint))
      return true;

    if(RayIntersection(&node->children[nFirst^1],nIdx.child(nFirst^1), r, vHitPoint))
      return true;

    if(RayIntersection(&node->children[nFirst^2],nIdx.child(nFirst^2), r, vHitPoint))
      return true;

    if(RayIntersection(&node->children[nFirst^4],nIdx.child(nFirst^4), r, vHitPoint))
      return true;

    if(RayIntersection(&node->children[nFirst^3],nIdx.child(nFirst^3), r, vHitPoint))
      return true;

    if(RayIntersection(&node->children[nFirst^5],nIdx.child(nFirst^5), r, vHitPoint))
      return true;

    if(RayIntersection(&node->children[nFirst^6],nIdx.child(nFirst^6), r, vHitPoint))
      return true;

    if(RayIntersection(&node->children[nFirst^7],nIdx.child(nFirst^7), r, vHitPoint))
      return true;
  }
  else
  {
    Ray r2;
    r2.direction = -r.direction;
    r2.origin = arrP[0] + r.direction * w * 4;

    if(!Intersect::Ray_AABB(r2,nodeBoxEx,arrP[1]))
      return 0; // error

    float values[2][2][2];

    for(int c=0;c<Cube::CORNERS;c++)
    {
      int x,y,z;
      Cube::FactorCornerIndex(c,x,y,z);

      TVoxValues::iterator iter = m_pValues->find(OctNode::CornerIndex(nIdx,c,maxDepth));
      if(iter==(*m_pValues).end())
        values[x][y][z] = 0;
      else
        values[x][y][z] = iter->second.val;
    }

    float arrV[2];

    for(int i=0; i<2; i++)
    {
      Vec3 & vPos = arrP[i];

      Vec3 vT = (vPos-nodeBox.min);
      Vec3 vNodeSize = nodeBox.GetSize();

      vT.x /= vNodeSize.x;
      vT.y /= vNodeSize.y;
      vT.z /= vNodeSize.z;

      float arrfValues1D[2];

      for(int nX=0; nX<2; nX++)
      {
        float fValue0 = 
          (1.f - vT.z)*values[nX][0][0] +
          (			 vT.z)*values[nX][0][1];

        float fValue1 = 
          (1.f - vT.z)*values[nX][1][0] +
          (			 vT.z)*values[nX][1][1];

        arrfValues1D[nX] = (1.f - vT.y)*fValue0 + (vT.y)*fValue1;
      }

      arrV[i] = (1.f - vT.x)*arrfValues1D[0] +(vT.x)*arrfValues1D[1];
    }

    if((arrV[0] >= 0) && (arrV[1] < 0))
    { // maybe not correct some cases
      float diff = arrV[0]-arrV[1];
      float t = diff ? ((arrV[0]-0) / diff) : 0.5f;
      vHitPoint = arrP[0]*(1.f-t) + arrP[1]*t;
      return true;
    }
  }

  return false;
}

void IsoOctree::PreloadCachedMeshes(OctNode* node, const OctNode::NodeIndex& nIdx)
{
  if(nIdx.depth<=nMeshDepth)
    PrintProgress();

  ISO_KEY key = OctNode::CenterIndex(nIdx,maxDepth);

  TIsoMeshes::iterator iter = threadMeshes.find(key);

  bool bExist = true;

  if(nIdx.depth >= nMeshDepth-GetCVars()->e_VoxTerMaxMeshLods)	
    if(iter == threadMeshes.end())
  {
    char szFileName[256]="";
    SIsoMesh::MakeTempTextureFileName(szFileName, sizeof(szFileName),0,"MTT",key);

    bExist = false;

    if(FILE * f = fopen(szFileName,"rb"))
    {
      // load if ready

      SIsoMesh * pNewMesh = new SIsoMesh(OctNode::CenterIndex( nIdx, IsoOctree::maxDepth ) );

      int nSize = 0;
      fread(&nSize, 1, sizeof(nSize), f);        
      int nModId = 0;
      fread(&nModId, 1, sizeof(nModId), f);        

      pNewMesh->m_pMeshForSerialization = new CMemoryBlock();
      pNewMesh->m_pMeshForSerialization->Allocate(nSize);
      fread((byte*)pNewMesh->m_pMeshForSerialization->GetData(), 1, nSize, f);

      fread(&pNewMesh->m_nTexDimX, 1, sizeof(pNewMesh->m_nTexDimX), f);
      fread(&pNewMesh->m_nTexDimY, 1, sizeof(pNewMesh->m_nTexDimY), f);
      if(!pNewMesh->m_nTexDimX || !pNewMesh->m_nTexDimY)
        Error(__FUNC__);
     
      // load zipped re-mesh texture
      int nZipSize = 0;
      int nElRead = fread(&nZipSize, 1, sizeof(nZipSize), f);
      if(nZipSize && (nZipSize < (16*1024*1024)) && nElRead==sizeof(nZipSize))
      {
        pNewMesh->m_pZipData = new CMemoryBlock;
        pNewMesh->m_pZipData->Allocate(nZipSize);
        fread(pNewMesh->m_pZipData->GetData(), 1, pNewMesh->m_pZipData->GetSize(), f);
      }
      else if(nZipSize || nElRead!=sizeof(nZipSize))
        Error("%s, fread failed, %s", __FUNC__, szFileName);

      fclose(f);

      threadMeshes[key] = pNewMesh;

      bExist = true;
    }
  }

  if(bExist && /*node->children && */nIdx.depth<nMeshDepth)
  {
    for(int i=0;i<Cube::CORNERS;i++)
    {
      PreloadCachedMeshes(NULL, nIdx.child(i));
    }
  }
}

bool IsoOctree::IsVisibleFromGamePlayArea(AABB WSBox, float fScale)
{
//  if(WSBox.GetSize().x >= 2.f || !GetCVars()->e_VoxTerTexBuildOnCPU)
  //  return true;

  if( !GetCVars()->e_VoxTerRoadsCheck || WSBox.GetSize().x >= 256.f )
    return true;

  float fVisRange = (fScale * WSBox.GetSize().x / GetCVars()->e_VoxTerViewDistRatio) + fPredictionDistance; 
  AABB WSBoxEx = WSBox;
  WSBoxEx.min -= Vec3(fVisRange,fVisRange,fVisRange);
  WSBoxEx.max += Vec3(fVisRange,fVisRange,fVisRange);

  for(int nSID=0; nSID<Get3DEngine()->m_pObjectsTree.Count(); nSID++)
  {
    if(Get3DEngine()->m_pObjectsTree[nSID])
      if(Get3DEngine()->m_pObjectsTree[nSID]->IsObjectTypeInTheBox(eERType_Road,WSBoxEx))
        return true;
  }

  return false;
}

byte IsoOctree_a = 0; 
Vec3 vHitT(0,0,0);

int IsoOctree::RayIntersectionChildsFindFirstNode(float& tx0, float& ty0, float& tz0, float& txm, float& tym, float& tzm) const 
{
  int x[3] = {0,0,0};

  /* find max{tx0, ty0, tz0} */
  int max = tx0 > ty0 ? (tx0 > tz0 ? 0 : 2) : (ty0 > tz0 ? 1 : 2);

  switch(max) 
  {
    case 2:
      /* entry plane XY */
      x[0] |= (txm < tz0) & 1;
      x[1] |= (tym < tz0) & 1;
      break;

    case 1:
      /* entry plane XZ */
      x[0] |= (txm < ty0) & 1;
      x[2] |= (tzm < ty0) & 1;
      break;

    case 0:
      /* entry plane YZ*/
      x[1] |= (tym < tx0) & 1;
      x[2] |= (tzm < tx0) & 1;
      break;

    default:
    /* really strange error. */
      break;
  }

  int res = ((x[0]&1) | ((x[1]&1)<<1) | ((x[2]&1)<<2));

  return res;
}

bool IsoOctree::RayIntersectionChilds(  float tx0, float ty0, float tz0, float tx1, float ty1, float tz1, OctNode *n ) 
{ 
  if ( (tx1 <= 0.0f ) || (ty1 <= 0.0f) || (tz1 <= 0.0f) ) 
    return false;

  if ( !(max(max(tx0,ty0),tz0) < min(min(tx1,ty1),tz1)) ) 
    return false;

  float txM = 0.5f * (tx0 + tx1); 
  float tyM = 0.5f * (ty0 + ty1); 
  float tzM = 0.5f * (tz0 + tz1);

  if (!n->children) 
  { 
    if(n->nodeData.mcIndex>0)
    {
      vHitT.x = txM;
      vHitT.y = tyM;
      vHitT.z = tzM;
      return true;
    }
    else
    {
      return false;
    }
  }

  int nFirst = RayIntersectionChildsFindFirstNode(tx0, ty0, tz0, txM, tyM, tzM);

  const float arrBounds[8][6]=
  {
    {tx0,ty0,tz0,txM,tyM,tzM},
    {txM,ty0,tz0,tx1,tyM,tzM},
    {tx0,tyM,tz0,txM,ty1,tzM},
    {txM,tyM,tz0,tx1,ty1,tzM},
    {tx0,ty0,tzM,txM,tyM,tz1},
    {txM,ty0,tzM,tx1,tyM,tz1},
    {tx0,tyM,tzM,txM,ty1,tz1},
    {txM,tyM,tzM,tx1,ty1,tz1},
  };

  for(int ii=0; ii<8; ii++)
  {
    int i = nFirst^ii;
    if(RayIntersectionChilds(arrBounds[i][0],arrBounds[i][1],arrBounds[i][2],arrBounds[i][3],arrBounds[i][4],arrBounds[i][5],&n->children[i^IsoOctree_a])) 
    {
      return true; 
    }
  }

  return false;
}

// In practice, it may be worth passing in the ray by value or passing in a copy of the ray 
// because of the fact the ray_step() function is destructive to the ray data. 
bool IsoOctree::RayIntersectionNew( Ray r, Vec3 & vHitPos ) 
{ 
  IsoOctree_a = 0;

  if (r.direction.x < 0) 
  { 
    r.origin.x = 1.f - r.origin.x; 
    r.direction.x = -(r.direction.x); 
    IsoOctree_a |= 1; 
  } 
  if (r.direction.y < 0) 
  { 
    r.origin.y = 1.f - r.origin.y; 
    r.direction.y = -(r.direction.y); 
    IsoOctree_a |= 2; 
  } 
  if (r.direction.z < 0) 
  { 
    r.origin.z = 1.f - r.origin.z; 
    r.direction.z = -(r.direction.z); 
    IsoOctree_a |= 4; 
  }

  float  tx0 = (0.0 - r.origin.x) / r.direction.x; 
  float  tx1 = (1.f - r.origin.x) / r.direction.x; 

  float  ty0 = (0.0 - r.origin.y) / r.direction.y; 
  float  ty1 = (1.f - r.origin.y) / r.direction.y; 

  float  tz0 = (0.0 - r.origin.z) / r.direction.z; 
  float  tz1 = (1.f - r.origin.z) / r.direction.z; 

  float tmin = max(max(tx0,ty0),tz0); 
  float tmax = min(min(tx1,ty1),tz1);

  if ( (tmin < tmax) ) 
  {
    if(RayIntersectionChilds(tx0,ty0,tz0,tx1,ty1,tz1, &tree))
    {
      vHitPos = vHitT;
      return true;
    }
  }

  return false;
}

Vec3 IsoOctree::ProjectToScreen(const Vec3 & vIn, float * pMatrix)
{
  Vec3 res;

  float _in[4], out[4];
  _in[0] = vIn.x;
  _in[1] = vIn.y;
  _in[2] = vIn.z;
  _in[3] = 1.0;

  CCoverageBuffer::TransformPoint(out, pMatrix, _in);

  res.x = out[0] / out[3];
  res.y = out[1] / out[3];
  res.z = out[2];// / out[3];

  return res;
}

void IsoOctree_TransformPoint(float out[4], const float m[16], const Vec3 in)
{
#define M(row,col)  m[col*4+row]
  out[0] = M(0, 0) * in[0] + M(0, 1) * in[1] + M(0, 2) * in[2] + M(0, 3);
  out[1] = M(1, 0) * in[0] + M(1, 1) * in[1] + M(1, 2) * in[2] + M(1, 3);
  out[2] = M(2, 0) * in[0] + M(2, 1) * in[1] + M(2, 2) * in[2] + M(2, 3);
  out[3] = M(3, 0) * in[0] + M(3, 1) * in[1] + M(3, 2) * in[2] + M(3, 3);
#undef M
}

CVoxTerrain::STargetMipData * IsoOctree_pTargets=0;
Array2d<uint8> * IsoOctree_pTargetsCounter=0;
float * IsoOctree_pMatCombined=0;
CCamera IsoOctree_camNorm;
Vec3 IsoOctree_camNormPos(0,0,0);
PodArray<SOcNodeInfo> IsoOctree_visNodes;
int nIsoOctree_visNodesSlotId=0;

uint16 EncodeNormalIntoUINT16(Vec3 vNorm)
{
  const int nCubeDim = 40;
  vNorm.Normalize();
  vNorm *= 0.5f;
  vNorm += Vec3(0.5f,0.5f,0.5f);
  vNorm *= nCubeDim;
  int x = CLAMP((int)vNorm.x,0,nCubeDim-1);
  int y = CLAMP((int)vNorm.y,0,nCubeDim-1);
  int z = CLAMP((int)vNorm.z,0,nCubeDim-1);
  int nId = x*nCubeDim*nCubeDim + y*nCubeDim + z;
  return nId;
}

uint16 EncodeColorIntoUINT16(Vec3 vColor)
{
  const int nCubeDim = 40;
  vColor *= nCubeDim;
  int x = CLAMP((int)vColor.x,0,nCubeDim-1);
  int y = CLAMP((int)vColor.y,0,nCubeDim-1);
  int z = CLAMP((int)vColor.z,0,nCubeDim-1);
  int nId = x*nCubeDim*nCubeDim + y*nCubeDim + z;
  return nId;
}

bool IsoOctree_RayObjectsIntersection(Ray & WSRay, PodArray<SDecalProjectionInfo> & decalProjectionInfo, Vec3 &vSummNorm, ColorF &colSumm, float fRayLen, int nMip)
{
  FUNCTION_PROFILER_3DENGINE;

  SRayHitInfo hitInfo;
  hitInfo.bInFirstHit = false;
  hitInfo.bUseCache = false;
  hitInfo.bGetVertColorAndTC = true;

  bool bHit = 0;

  for(int d=0; d<decalProjectionInfo.Count(); d++)
  {
    SDecalProjectionInfo & info = decalProjectionInfo[d];

    //        if(info.objType == eERType_Brush)
    {
      hitInfo.inRay.origin = info.matObjInv.TransformPoint(WSRay.origin);
      hitInfo.inRay.direction = info.matObjInv.TransformVector(WSRay.direction);
      hitInfo.inReferencePoint = hitInfo.inRay.origin + hitInfo.inRay.direction*0.5f;
      hitInfo.fMaxHitDistance = fRayLen / info.fScale;

      if(info.pStatObj->RayIntersection(hitInfo, info.pMat))
      {
        assert(hitInfo.fDistance <= hitInfo.fMaxHitDistance);

        fRayLen = hitInfo.fDistance * info.fScale;

        CIndexedMesh::ProcessMaterial(info.pMat, hitInfo.nHitMatID, hitInfo.vHitColor, hitInfo.vHitTC, nMip, colSumm, vSummNorm, 
          hitInfo.vHitTangent, hitInfo.vHitBinormal, &info.matObj);

        bHit = true;
      }
    }
    /*else if(info.objType == eERType_Decal)
    {
    Vec3 vDecalTC = info.matObjInv.TransformPoint(vPos)*-0.5f + Vec3(0.5f,0.5f,0.5f);

    if(vDecalTC.x<1 && vDecalTC.x>0 && vDecalTC.y<1 && vDecalTC.y>0)
    {
    ColorF colDecal = GetBilinearFilteredAt( 1.f-vDecalTC.x, vDecalTC.y, info.pImgMips, info.nImgDim, info.nImgDim, 1.f/255.f, nMip ) * info.materialDiffuse;

    float fAlpha = SATURATE((1.f - fabs(vDecalTC.z-0.5f)*4.f) * 3.f);

    fAlpha *= colDecal.a;

    colSumm = (colSumm*(1.f-fAlpha) + colDecal*fAlpha);
    }
    }
    else
    assert(!"Undefined type");*/
  }

  return bHit;
}

int GetVecProjId(const Vec3 & vNorm)
{
  int nOpenId=0;

  Vec3 vNormAbs = vNorm.abs();

  if(     vNormAbs.x>=vNormAbs.y && vNormAbs.x>=vNormAbs.z)
    nOpenId = 0;
  else if(vNormAbs.y>=vNormAbs.x && vNormAbs.y>=vNormAbs.z)
    nOpenId = 1;
  else 
    nOpenId = 2;

  return nOpenId;
}

#ifdef VOX_DVR
void IsoOctree::RasterizeVolume( OctNode* node, const OctNode::NodeIndex& nIdx, bool bHasSurface, bool bAllIn )
{ 
  // get node center and size
  Vec3 center_n; float w_n;
  OctNode::CenterAndWidth(nIdx,center_n,w_n);

  // check frustum
  if(!bAllIn)   
  {
    uint8 nVis = IsoOctree_camNorm.IsSphereVisible_FH(Sphere(center_n,w_n));
    if(nVis == CULL_EXCLUSION)
      return;
    if(nVis == CULL_INCLUSION)
      bAllIn = true;
  }

  // check distance to camera and voxel size
  bool bLeaf = ((w_n*CVoxTerrain::m_fMapSize) < 0.16f)  || // !node->children ||
    (IsoOctree_camNormPos.GetSquaredDistance(center_n) > pow(w_n*GetCVars()->e_VoxTerDebugLodRatio,2));

  // register node
  if(node && ((bLeaf && bHasSurface) || !node->children) && bHasSurface)
  {
    Vec4_tpl<uint16> nodeBox;
    nodeBox.x = uint16(center_n.x*65536.f);
    nodeBox.y = uint16(center_n.y*65536.f);
    nodeBox.z = uint16(center_n.z*65536.f);
    nodeBox.w = uint16(w_n*65536.f);

    if(node->nOcNodeInfoId == 0 || nodeBox != IsoOctree_visNodes[node->nOcNodeInfoId].nodeBox)
    {
      for(int i=0; i<IsoOctree_visNodes.Count(); i++)
      {
        m_nCurrentNodeSlotId++;
        if(m_nCurrentNodeSlotId>=IsoOctree_visNodes.Count())
          m_nCurrentNodeSlotId=1;

        if(IsoOctree_visNodes[m_nCurrentNodeSlotId].lastUsedFrameId.x < (GetMainFrameID()-4))
          break;
      }

      node->nOcNodeInfoId = m_nCurrentNodeSlotId;
      
      SOcNodeInfo & rNodeInfo = IsoOctree_visNodes[m_nCurrentNodeSlotId];
      
      rNodeInfo.nodeBox = nodeBox;

      Vec3 vAverNormal(0,0,0);

      int surf_type = 0;

      bool bUseRemesh = false;
      
      FillCornerValues( nIdx,  &rNodeInfo.nodeValues[0], &rNodeInfo.nodeEncodedNormals[0], vAverNormal, surf_type, bUseRemesh );

//      float fDot = max(0.01f,fabs(vAverNormal.Dot((IsoOctree_camNormPos-center_n).GetNormalized())));

      int nMip = 0;
      float node_w = w_n*CVoxTerrain::m_fMapSize;
      float texel_w = 0.01f;
      while(texel_w*4 < node_w)
        nMip++, texel_w*=2;

      TVoxValues::iterator iterCenter = (*m_pValues).find(OctNode::CenterIndex( nIdx, maxDepth ));
      if(iterCenter == m_pValues->end())
        iterCenter = (*m_pValues).find(OctNode::CenterIndex( nIdx.parent(), maxDepth ));

      ColorF nodeColor = Col_White;
      if(iterCenter != m_pValues->end())
      {
        nodeColor = ColorF(iterCenter->second.color[2],iterCenter->second.color[1],iterCenter->second.color[0]);
        nodeColor /= 255;
        nodeColor.srgb2rgb();
      }

      for(int x=0; x<4; x++) for(int y=0; y<4; y++) for(int z=0; z<4; z++)
      {
        if(!GetCVars()->e_VoxTerDebugBuildLeafs)
        {
          rNodeInfo.arrLeafData[x][y][z] = EncodeColorIntoUINT16(vAverNormal);
          rNodeInfo.arrLeafNorm[x][y][z] = EncodeNormalIntoUINT16(vAverNormal);
          continue;
        }

        Vec3 vPos = center_n - Vec3(w_n*.5f,w_n*.5f,w_n*.5f) + Vec3(w_n*.125f,w_n*.125f,w_n*.125f) + Vec3(float(x)/4.f, float(y)/4.f, float(z)/4.f)*w_n;

//      float fVal = 0;
//      GetValueByPosition(node, nIdx, vPos, &fVal, nIdx.depth+2);
//      bool bFound = (fVal<0.025f/CVoxTerrain::m_fMapSize);
//      rNodeInfo.arrLeafData[x][y][z] = (fVal<0.025f/CVoxTerrain::m_fMapSize) ? uint16(-1) : 0;

        OctNode * pFoundNode = GetValueByPosition_BIN(node, nIdx, vPos, nIdx.depth+2);

        if( pFoundNode && pFoundNode->nColor == 0 && pFoundNode->nNormal == 0 )
        {
          Vec3 vColor(0,0,1);
	        Vec3 vSummNorm(0,0,0.5f);

          if(GetCVars()->e_VoxTerTexBuildOnCPU)
          {
            static PodArray<SDecalProjectionInfo> decalProjectionInfo; // decalProjectionInfo.Clear();

            float fRayLen = CVoxTerrain::m_fMapSize*w_n*2;

            if(!decalProjectionInfo.Count())
            {
              static PodArray<IRenderNode*> arrRenderNodes; arrRenderNodes.Clear();

              for(int nSID=0; nSID<Get3DEngine()->m_pObjectsTree.Count(); nSID++)
                if(Get3DEngine()->m_pObjectsTree[nSID] && GetCVars()->e_VoxTerTexBuildOnCPU)
              {
                AABB aabbWSEx = AABB(Vec3(0,0,0),Vec3(512,512,512));
                aabbWSEx.min -= Vec3(fRayLen,fRayLen,fRayLen);
                aabbWSEx.max += Vec3(fRayLen,fRayLen,fRayLen);
                Get3DEngine()->m_pObjectsTree[nSID]->GetObjectsByType(arrRenderNodes, eERType_Brush, &aabbWSEx, -1);
              }

              AABB nodeBoxWS = GetNodeAABB(nIdx,true);

              for(int d=0; d<arrRenderNodes.Count(); d++)
              {
                IRenderNode * pNode = arrRenderNodes.GetAt(d);

                AABB nodeBoxEx = pNode->GetBBox();
                nodeBoxEx.min -= Vec3(fRayLen,fRayLen,fRayLen);
                nodeBoxEx.max += Vec3(fRayLen,fRayLen,fRayLen);

                if(!Overlap::AABB_AABB(nodeBoxEx,nodeBoxWS))
                  continue;

                SDecalProjectionInfo info;

                if(IMaterial * pMaterial = pNode->GetMaterial())
                {
                  Matrix34A mat;
                  pNode->GetEntityStatObj(0,0,&mat);
                  info.matObjInv = mat.GetInverted();
                  info.matObj = mat;
                  info.pStatObj = (CStatObj *)pNode->GetEntityStatObj(0,0,NULL);

                  info.fScale = ((CBrush*)pNode)->m_fMatrixScale;

                  info.pMat = pMaterial;
                  decalProjectionInfo.Add(info);
                }
              }
            }

            Vec3 vNorm = vAverNormal;

/*              TFlatness::iterator iter = m_flatnessMap.find(OctNode::CenterIndex( nIdx, maxDepth ));
            if(iter != m_flatnessMap.end())
            {
              SFlatnessItem & val = iter->second;
              vNorm = val.first;
            }
            else
              vNorm = Vec3(0,0,1.f);*/

            vNorm.Normalize();
            vNorm *= -fRayLen;
            Ray WSRay(vPos*CVoxTerrain::m_fMapSize-vNorm, vNorm*2.f); 

            ColorF colSumm = nodeColor;

            SImageInfo & rImgInfo = Get3DEngine()->m_arrBaseTextureData[surf_type];

            {
              Vec4 vTang0, vTang1;

              vTang0 = Vec4(0,1,0,0), vTang1 = Vec4(1,0,0,0);

              Vec2 vTC;
              vTC = Vec2(vPos.y,vPos.x)*CVoxTerrain::m_fMapSize;
/*
              switch(GetVecProjId(vAverNormal))
              {
              case 0:
//                vTang0 = Vec4(0,1,0,0), vTang1 = Vec4(0,0,1,0);
                vTC = Vec2(vPos.y,vPos.z)*CVoxTerrain::m_fMapSize;
                break;
              case 1:
  //              vTang0 = Vec4(0,0,1,0), vTang1 = Vec4(1,0,0,0);
                vTC = Vec2(vPos.z,vPos.x)*CVoxTerrain::m_fMapSize;
                break;
              case 2:
    //            vTang0 = Vec4(0,1,0,0), vTang1 = Vec4(1,0,0,0);
                vTC = Vec2(vPos.y,vPos.x)*CVoxTerrain::m_fMapSize;
                break;
              }
*/
              // project tangent and binormal to plane perpendicular to the normal
              Vec4 vAverNormal4;
              vAverNormal4.x = vAverNormal.x;
              vAverNormal4.y = vAverNormal.y;
              vAverNormal4.z = vAverNormal.z;
              vAverNormal4.w = 0;

              Vec4 vS;
              vS = vAverNormal4*vTang0.Dot(vAverNormal4);
              vTang0.x -= vS.x;
              vTang0.y -= vS.y;
              vTang0.z -= vS.z;
              vTang0.w -= vS.w;

              vS = vAverNormal4*vTang1.Dot(vAverNormal4);
              vTang1.x -= vS.x;
              vTang1.y -= vS.y;
              vTang1.z -= vS.z;
              vTang1.w -= vS.w;

              vTang0.Normalize();
              vTang0.w=-1;
              vTang1.Normalize();
              vTang1.w=-1;
              /*
              if(IMaterial * pMat = rImgInfo.szDetMatName[0] ? Get3DEngine()->GetMatMan()->LoadMaterial(rImgInfo.szDetMatName) : NULL)
              {
                ColorF detMat = Col_DarkGray;
                CIndexedMesh::ProcessMaterial(pMat, 0, Vec4(1,1,1,1), vTC, nMip, detMat, vSummNorm, vTang0, vTang1, NULL);
                detMat -= .5f;
                colSumm = detMat*.5f + colSumm;
              }
              */             
            }

            if(bUseRemesh)
              IsoOctree_RayObjectsIntersection(WSRay, decalProjectionInfo, vSummNorm, colSumm, fRayLen, nMip);

            colSumm = colSumm*6;
				    colSumm += 1.f/40;
            colSumm.Clamp();

            vColor.x = colSumm.b;
            vColor.y = colSumm.g;
            vColor.z = colSumm.r;
          }
          else
          {
            vColor.Set(1,0,0);
            vSummNorm.Set(0,0,0.5f);
          }

          pFoundNode->nColor = EncodeColorIntoUINT16(vColor);
          pFoundNode->nNormal = EncodeNormalIntoUINT16(vSummNorm);
        }

        rNodeInfo.arrLeafData[x][y][z] = (pFoundNode) ? pFoundNode->nColor : 0;
        rNodeInfo.arrLeafNorm[x][y][z] = (pFoundNode) ? pFoundNode->nNormal : 0;
      }
    }

    if(node->nOcNodeInfoId<IsoOctree_visNodes.Count())
    {
      IsoOctree_visNodes[node->nOcNodeInfoId].lastUsedFrameId.x = GetMainFrameID();
      nIsoOctree_visNodesSlotId = node->nOcNodeInfoId+1;
    }
  }

  if(bLeaf)
  { 
    if(bHasSurface)
    {
      // get screen space position
      Vec2 vPos2D;
      {
        float out[4];
        IsoOctree_TransformPoint(out, IsoOctree_pMatCombined, center_n);
        vPos2D.x = out[0] / out[3];
        vPos2D.y = out[1] / out[3];
      }

      {
        int nRes = IsoOctree_pTargets[0].arrData.GetSize();

        int sxC = (int)(((float)(nRes>>1) + (float)(vPos2D.x * (float)(nRes>>1))));
        int syC = (int)(((float)(nRes>>1) + (float)(vPos2D.y * (float)(nRes>>1))));

        float fNearPlaneDist = -IsoOctree_camNorm.GetFrustumPlane(FR_PLANE_NEAR)->DistFromPlane(center_n);

        int area = (int)min((float)GetCVars()->e_VoxTerDebugAreaSizeMax, w_n * (float)GetCVars()->e_VoxTerDebugAreaSizeRatio / max(fNearPlaneDist,0.01f/CVoxTerrain::m_fMapSize));

        int sx_min = max(sxC-area, 0);
        int sx_max = min(sxC+area, nRes-1);
        int sy_min = max(syC-area, 0);
        int sy_max = min(syC+area, nRes-1);

        int area2 = area*area;

        for(int sy = sy_min; sy<=sy_max; sy++)
        {
          for(int sx = sx_min; sx<=sx_max; sx++)
          {
            if(((sx-sxC)*(sx-sxC) + (sy-syC)*(sy-syC)) > area2)
              continue;

            int nOffset = (nRes-1-sy)*nRes + sx;

            uint8 & rCounter = IsoOctree_pTargetsCounter->GetData()[nOffset];

            if(rCounter < INDEX_TARGETS_NUM*4)
            {
              // check if current node is already added
              if(!node && rCounter)
              {
                int l = ((rCounter-1)/4);
                int c = (rCounter-1) - l;
                Vec4_tpl<uint16> & val = IsoOctree_pTargets[l&(INDEX_TARGETS_NUM-1)].arrData.GetData()[nOffset];
                if(val[c&3] == nIsoOctree_visNodesSlotId)
                  continue;
              }

              int l = (rCounter/4);
              int c = rCounter - l;
              rCounter++;
              Vec4_tpl<uint16> & val0 = IsoOctree_pTargets[l&(INDEX_TARGETS_NUM-1)].arrData.GetData()[nOffset];
              val0[c&3] = nIsoOctree_visNodesSlotId;
            }
          }
        }
      }
    }

    return;
  }

  /*if(node && !node->children && (nIdx.depth < (nMeshDepth+GetCVars()->e_VoxTerDebugNodesGenDepth)) && bHasSurface)
  {
    node->initChildren();

    interpolateChilds(nIdx, node, m_pValues, m_pValues);

    interpolateChildsNormals(nIdx, node, &m_flatnessMap, &m_flatnessMap);

    for(int childId=0; childId<8; childId++)
    {
      float cValues[8];

      for(int i=0;i<Cube::CORNERS;i++)
      {
        ISO_KEY key = OctNode::CornerIndex(nIdx.child(childId),i,maxDepth);
        TVoxValues::iterator iter = (*m_pValues).find(key);
        if(iter==(*m_pValues).end())
          cValues[i] = 0.f;
        else
          cValues[i] = iter->second.val;
      }

      node->children[childId].nodeData.mcIndex = MarchingCubes::GetIndex(cValues,isoValue);
    }
  }*/

  int nFirst = 
    ((IsoOctree_camNormPos.x > center_n.x) ? 1 : 0) |
    ((IsoOctree_camNormPos.y > center_n.y) ? 2 : 0) |
    ((IsoOctree_camNormPos.z > center_n.z) ? 4 : 0);

  for(int ii=0; ii<8; ii++)
  {
    int i = nFirst^ii;

    OctNode * child = (node && node->children) ? (&node->children[i]) : NULL;

    bool bChildHasSurface = child ? (child->nodeData.mcIndex>0 && child->nodeData.mcIndex<255) : bHasSurface; 

    if(node && !node->children && (nIdx.depth<(nMeshDepth+GetCVars()->e_VoxTerDebugNodesGenDepth)))
      bChildHasSurface = false;

    if(child || bChildHasSurface)
      RasterizeVolume( child, nIdx.child(i), bChildHasSurface, bAllIn ) ;
  }
}

void IsoOctree::FillCornerValues( const OctNode::NodeIndex& nIdx,  float * values )
{
  FUNCTION_PROFILER_3DENGINE;

  for(int c=0;c<Cube::CORNERS;c++)
  {
    int x,y,z;
    Cube::FactorCornerIndex(c,x,y,z);
    float & v = values[x*4+y*2+z];
    TVoxValues::iterator iter = m_pValues->find(OctNode::CornerIndex(nIdx,c,maxDepth));
    if(iter==(*m_pValues).end())
      v = 0;
    else
      v = iter->second.val;
  }
}

void IsoOctree::FillCornerValues( const OctNode::NodeIndex& nIdx, Vec4_tpl<uint16> * nodeValues, Vec4_tpl<uint16> * nodeEncodedNormals, Vec3 & vAverNormal, int & nSurfType, bool & bUseRemesh )
{
  FUNCTION_PROFILER_3DENGINE;

  vAverNormal.zero();

  float values[2][2][2];

  for(int c=0;c<Cube::CORNERS;c++)
  {
    int x,y,z;
    Cube::FactorCornerIndex(c,x,y,z);

    {
      float & v = values[x][y][z] = 0;

      TVoxValues::iterator iter = m_pValues->find(OctNode::CornerIndex(nIdx,c,maxDepth));
      if(iter!=(*m_pValues).end())
      {
        v = iter->second.val;

        SImageInfo & rImgInfo = Get3DEngine()->m_arrBaseTextureData[iter->second.surf_type.GetDomSType()];

        if(rImgInfo.fUseRemeshing > 0)
          bUseRemesh = true;
        
        nSurfType = iter->second.surf_type.GetDomSType();
      }
    }

    {
      Vec3 vNormal(0,0,1);

      TFlatness::iterator iter = m_flatnessMap.find(OctNode::CornerIndex(nIdx,c,maxDepth));
      if(iter != m_flatnessMap.end())
      {
        SFlatnessItem & val = iter->second;
        vNormal = val.first;
        vNormal.Normalize();
        vAverNormal += vNormal;
      }

      nodeEncodedNormals[(c/4)&1][c&3] = EncodeNormalIntoUINT16(vNormal);
    }
  }

  for(int l=0; l<2; l++)
  {
    nodeValues[l].x = (uint16)CLAMP(values[0][0][l]*32768.f*32.f + 32768.f, 0.f, 65536.f);
    nodeValues[l].y = (uint16)CLAMP(values[1][0][l]*32768.f*32.f + 32768.f, 0.f, 65536.f);
    nodeValues[l].z = (uint16)CLAMP(values[0][1][l]*32768.f*32.f + 32768.f, 0.f, 65536.f);
    nodeValues[l].w = (uint16)CLAMP(values[1][1][l]*32768.f*32.f + 32768.f, 0.f, 65536.f);
  }

  vAverNormal.Normalize();
}
#endif
bool IsoOctree::DeleteFolder(const char *szFolder,bool bRecurse)
{
  bool							boRemoveResult(false);

  __finddata64_t		fd;
  string						filespec = szFolder;

  filespec += "/";
  filespec += "*.*";

  intptr_t hfil = 0;
  if ((hfil = _findfirst64(filespec.c_str(), &fd)) == -1)
  {
    return false;
  }

  do
  {
    if (fd.attrib & _A_SUBDIR)
    {
      string name = fd.name;

      if ((name != ".") && (name != ".."))
      {
        if (bRecurse)
        {
          name = szFolder;
          name += "/";
          name += fd.name;
          name += "/";

          DeleteFolder(name.c_str(), bRecurse);
        }
      }
    }
    else
    {
      string name = szFolder;

      name += "/";
      name += fd.name;

      int res = remove(name);
    }

  } while(!_findnext64(hfil, &fd));

  _findclose(hfil);

  ::RemoveDirectory(szFolder);

  return boRemoveResult;
}


void IsoOctree::DeleteTooNewCacheItems()
{
	char szFolder[256]="";
	SIsoMesh::MakeTempTextureFileName(szFolder, sizeof(szFolder),0,"MTT",0);

	__finddata64_t		fd;
	string						filespec = szFolder;

	filespec += "/";
	filespec += "*.MTT";

	intptr_t hfil = 0;
	if ((hfil = _findfirst64(filespec.c_str(), &fd)) == -1)
	{
		return;
	}

	PrintMessage("Validating cache files (ModId=%d) ...", IsoOctree_nModId);

	int nDeleted=0, nValid=0;

	do
	{
		if (fd.attrib & _A_SUBDIR)
		{
			string name = fd.name;

			if ((name != ".") && (name != ".."))
			{
			}
		}
		else
		{
			string name = szFolder;
			name += "/";
			name += fd.name;
		
			if(FILE * f = fopen(name,"rb"))
			{
				int nSize = 0;
				fread(&nSize, 1, sizeof(nSize), f);        
				int nModId = IsoOctree_nModId+1;
				fread(&nModId, 1, sizeof(nModId), f);        
				fclose(f);

				if(nModId>IsoOctree_nModId)
				{
					int res = remove(name);

					string bin_name = name;
					bin_name.replace("MTT","BIN");
					res = remove(bin_name);

					nDeleted++;
				}
				else
				{
					nValid++;
				}
			}

			PrintProgress();
		}

	} while(!_findnext64(hfil, &fd));

	_findclose(hfil);

	PrintMessagePlus(" %d - %d = %d", nDeleted+nValid, nDeleted, nValid);
}
