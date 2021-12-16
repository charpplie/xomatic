


#include "stdafx.h"
#include "PhotoBump10.h"
#include "PhotoBump10Doc.h"
#include "PhotoBump10View.h"
#include "Video.h"
#include "PhotoFrame.h"
#include "PVertex.h"
#include "PQuadtree.h"
#include "PhotoImage.h"

#include "WmlDelaunay2a.h"

using namespace Wml;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10Doc::PolygonReduction()
{	
	int w=m_lstFrames[KEY_FRAME]->m_pImage->m_nWidth;
	int h=m_lstFrames[KEY_FRAME]->m_pImage->m_nHeight;
	CPhotoFrame *pFrame=m_lstFrames[KEY_FRAME];

	int nLimit=m_nNumVertices/5;

#ifdef USE_GL_LIGHT
	int nStride=8;
#else
	int nStride=5;
#endif

	pFrame->m_pImage->ExtractEdges(false,false);

	// remove excessive points
	for (int k=0;k<w*h;k++)
	{
		if (!pFrame->m_pImage->m_pEdges[k])
			continue;

		if (m_IndexMap[k]<0)
			continue;
		
		int y=k/w;
		int x=k-(y*w);
		
		for (int k3=0;k3<15*15;k3++)
		{
			int x1=x+dirs1515[k3][0];
			int y1=y+dirs1515[k3][1];

			if (x1<0 || y1<0 || x1>=w || y1>=h)
				continue;

			if (x1==x && y1==y)
				continue;

			m_IndexMap[y1*w+x1]=-1;
		} //k3		
	} //k

	// 2d triangulation	
	Vector2f *akVertex=new Vector2f [w*h];	
	int nVertices=0;
	for (int k=0;k<w*h;k++)
	{
		if (!pFrame->m_pImage->m_pEdges[k])
			continue;

		if (m_IndexMap[k]<0)
			continue;

		int y=k/w;
		int x=k-(y*w);

		akVertex[nVertices]=Vector2f((float)(x),(float)(y));			
		m_IndexMap[k]=nVertices;

		nVertices++;
	} //i	

	MyOutputDebugString("Vertices to triangulate: %d \n",nVertices);

	m_nNumVertices=nVertices;

	m_nNumIndexes=0;	
	SAFE_DELETE_ARRAY(m_Indices);	
	SAFE_DELETE_ARRAY(m_Vertices);

	int *OutVerts,*OutAdj;
	int nOutTriang;

	Delaunay2af *delaunay=new Delaunay2af(nVertices,akVertex,nOutTriang,OutVerts,OutAdj);

	MyOutputDebugString("Generated %d triangles \n",nOutTriang);

	m_nNumIndexes=nOutTriang*3;
	
	m_Indices=new uint [m_nNumIndexes];
	m_Vertices=new float [m_nNumVertices*nStride];

	//The i-th triangle has edge index pairs
	//   edge0 = <raiTVertex[3*i],raiTVertex[3*i+1]>
	//   edge1 = <raiTVertex[3*i+1],raiTVertex[3*i+2]>
	//   edge2 = <raiTVertex[3*i+2],raiTVertex[3*i]>
	m_nNumIndexes=0;
	for (int k=0;k<nOutTriang;k++)	
	{
		// get triangle's edges
		for (int j=0;j<3;j++)
		{					
			int idx1=OutVerts[k*3+(2-j)];	// ccw for ogl		
			// get vert based on 2d edges used for triangulation
			Vector2f *v11=&akVertex[idx1];
			
			int x1=(int)(v11->X()),y1=(int)(v11->Y());			

			vector3f vSource=pFrame->m_pPoints[y1*w+x1];
			int nVert=m_IndexMap[y1*w+x1];

			m_Vertices[nVert*nStride+0]=(float)(vSource.x);
			m_Vertices[nVert*nStride+1]=(float)(vSource.y);
			m_Vertices[nVert*nStride+2]=(float)(vSource.z);
			m_Vertices[nVert*nStride+3]=(float)(x1)/(float)(w);
			m_Vertices[nVert*nStride+4]=(float)(y1)/(float)(h);

#ifdef USE_GL_LIGHT
			m_Vertices[nVert*nStride+5]=0;
			m_Vertices[nVert*nStride+6]=0;					
			m_Vertices[nVert*nStride+7]=0;					
#endif

			m_Indices[m_nNumIndexes++]=nVert;
		} //j
	} //k

	delete delaunay;
	SAFE_DELETE_ARRAY(OutVerts);
	SAFE_DELETE_ARRAY(OutAdj);
	SAFE_DELETE_ARRAY(akVertex);

#ifdef USE_GL_LIGHT
	// calc normals
	for (int k=0;k<m_nNumIndexes;k+=3)
	{
		int nVerts[3];
		vector3f rkV[3];
		for (int j=0;j<3;j++)		
		{
			nVerts[j]=m_Indices[k+j];			
			for (int j1=0;j1<3;j1++)
				rkV[j][j1]=m_Vertices[nVerts[j]*nStride+j1];
		}		

		vector3f kEdge1 = rkV[1] - rkV[0];
		vector3f kEdge2 = rkV[2] - rkV[0];
		vector3f kNormal = kEdge1.Cross(kEdge2);

		for (int j=0;j<3;j++)
		{		
			for (int j1=0;j1<3;j1++)
				m_Vertices[nVerts[j]*nStride+5+j1]+=(float)(kNormal[j1]);			
		} //j
	} //k

	//for (int k=0;k<(w1/nScale*h1/nScale);k++)
	for (int k=0;k<m_nNumVertices;k++)
	{
		vector3f vNormal;
		for (int j1=0;j1<3;j1++)
			vNormal[j1]=m_Vertices[k*nStride+5+j1];		
		vNormal.Normalize();		
		for (int j1=0;j1<3;j1++)
			m_Vertices[k*nStride+5+j1]=(float)(vNormal[j1]);		
	} //k			
#endif

	SAFE_DELETE_ARRAY(m_IndexMap);
}



