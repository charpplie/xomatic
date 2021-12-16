
#include "stdafx.h"
#include "PhotoBump10.h"
#include "PhotoBump10Doc.h"
#include "PhotoBump10View.h"
#include "Video.h"
#include "PhotoFrame.h"
#include "PhotoImage.h"
#include "PVertex.h"
#include "ImageProcessing.h"

#include "opencv/cv.h"
//#include "opencv/cvaux.h"
#include "opencv/highgui.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif 

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10Doc::Reconstruct5(CPaintDC *dc)
{	
	if (m_lstFrames.empty() || m_bDone)
		return;

	int w=m_lstFrames[KEY_FRAME]->m_pImage->m_nWidth;
	int h=m_lstFrames[KEY_FRAME]->m_pImage->m_nHeight;

	if (m_bInitialUpdate)
	{	

		int w=m_lstFrames[KEY_FRAME]->m_pImage->m_nWidth;
		int h=m_lstFrames[KEY_FRAME]->m_pImage->m_nHeight;
		CPhotoFrame *pFrame=m_lstFrames[KEY_FRAME];		

		if (!ReconstructInitialUpdate5(dc))					
		{
			m_bDone=true;		
			if (theApp.m_bStandalone)
			{
		#ifdef _DEBUG
				PostQuitMessage(0);
		#else
				exit(0);
		#endif				
			}			
		}			
		else
		{
			if (!m_bShowFeaturesOnly)
				DenseMatching(dc);
		}

		m_bInitialUpdate=false;
				
		return;
	}

	if (theApp.m_bStandalone)
	{
#ifdef _DEBUG
		PostQuitMessage(0);
#else
		exit(0);
#endif
		
		m_bDone=true;
		return;
	}

#ifdef CREATE_RENDERING_DATA
	// create rendering data	
	int nScale=2;
	CPhotoFrame *pKeyFrame=m_lstFrames[KEY_FRAME];
	//CPhotoFrame *pKeyFrame=m_lstFrames[1];
	if (!pKeyFrame->m_pPoints)
	{
		m_bDone=true;
		return;
	}	
	//CCamera *pCam=&pKeyFrame->m_Camera;
#ifdef USE_GL_LIGHT
	int nStride=8;
#else
	int nStride=5;
#endif

	int x1=0;int y1=0;
	int x2=w;int y2=h;

	//int x1=827;int y1=180;
	//int x2=952;int y2=297;

	//int x1=876;int y1=180;
	//int x2=x1+5;int y2=y1+5;

	int w1=x2-x1;
	int h1=y2-y1;
					
	m_nNumVertices=0;

	// allocate maximum
	m_Vertices=new float [((w1/nScale)+1)*((h1/nScale)+1)*nStride];		

	m_IndexMap=new int [w*h];
	memset(m_IndexMap,(int)(-1),w*h*sizeof(int));

	m_Indices=new uint[(w1/nScale*h1/nScale)*6]; // allocate maximum
	m_nNumIndexes=0;
	int nw=w1/nScale;
	vector3f vMins,vMaxs;
	vMins=m_vMins;
	vMaxs=m_vMaxs;
	vector3f vDiag=m_vMaxs-((vMins+vMaxs)/2.0);
	ftype fLen=vDiag.Length(); 
	fLen/=15.0;
	fLen*=fLen;
	for (int y=y1;y<y2-nScale;y+=nScale)
	{
		for (int x=x1;x<(x2-nScale);x+=nScale)
		{
			uint nVerts[4]; // in the vertex buffer above
			//nVerts[0]=(y-y1)/nScale*nw+(x-x1)/nScale;
			
			nVerts[0]=y*w+x;
			nVerts[1]=y*w+x+nScale;
			nVerts[2]=(y+nScale)*w+x+nScale;
			nVerts[3]=(y+nScale)*w+x;

			vector3f v1=pKeyFrame->m_pPoints[nVerts[0]];
			vector3f v2=v1;

			int k;
			for (k=0;k<4;k++)
			{			
				vector3f v2=pKeyFrame->m_pPoints[nVerts[k]];
				if (v2.w<0)
					break;
				if (!(v2.InsideBBox(vMins,vMaxs)))				
					break;				
				if (v1.Distance2(v2)>fLen)
					break;				
			} //k

			if (k<4)
				continue;

			for (int j=0;j<4;j++)
			{			
				int idx=m_IndexMap[nVerts[j]];
				if (idx>0)				
					nVerts[j]=idx;				
				else
				{
					vector3f vSource=pKeyFrame->m_pPoints[nVerts[j]];

					m_Vertices[m_nNumVertices*nStride+0]=(float)(vSource.x);
					m_Vertices[m_nNumVertices*nStride+1]=(float)(vSource.y);
					m_Vertices[m_nNumVertices*nStride+2]=(float)(vSource.z);
					m_Vertices[m_nNumVertices*nStride+3]=(float)(x)/(float)(w);
					m_Vertices[m_nNumVertices*nStride+4]=(float)(y)/(float)(h);					
#ifdef USE_GL_LIGHT
					m_Vertices[m_nNumVertices*nStride+5]=0;
					m_Vertices[m_nNumVertices*nStride+6]=0;					
					m_Vertices[m_nNumVertices*nStride+7]=0;					
#endif
					m_IndexMap[nVerts[j]]=m_nNumVertices;
					nVerts[j]=m_nNumVertices;			
					m_nNumVertices++;
				}
			} //j

			m_Indices[m_nNumIndexes++]=nVerts[0];
			m_Indices[m_nNumIndexes++]=nVerts[3];
			m_Indices[m_nNumIndexes++]=nVerts[2];
			m_Indices[m_nNumIndexes++]=nVerts[0];
			m_Indices[m_nNumIndexes++]=nVerts[2];
			m_Indices[m_nNumIndexes++]=nVerts[1];
		} //x
	}//y		
		

	//////////////////////////////////////////////////////////////////////////
	// calc normals

#ifdef USE_GL_LIGHT	
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
#endif
	
	m_bDone=true;
}

//////////////////////////////////////////////////////////////////////////
bool CPhotoBump10Doc::Iterate5()
{
	
	return (false);	
}

//////////////////////////////////////////////////////////////////////////
bool CPhotoBump10Doc::ReconstructInitialUpdate5(CPaintDC *dc)
{		
	InitSinTable();

	if (!SelfCalibration(dc))
	{
		if (!theApp.m_bStandalone)
			AfxMessageBox("Cannot calibrate cameras");
		return (false);
	}

	return (true);
}