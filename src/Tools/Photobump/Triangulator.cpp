
#include "stdafx.h"
#include "PhotoBump10.h"
#include "PhotoBump10Doc.h"
#include "PhotoBump10View.h"
#include "Video.h"
#include "PhotoFrame.h"
#include "PhotoImage.h"
#include "PVertex.h"

#include "opencv/cv.h"
//#include "opencv/cvaux.h"
#include "opencv/highgui.h"

#include "WmlDelaunay2a.h"
#include "wmlConvexHull2.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////////
typedef struct 
{
	int nPos;
	int x[2];
	vector3f vPos[2];
}tRast;

using namespace Wml;

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10Doc::TriangulateAllFrames()
{
	lstPhotoVerticesIt i;

	int w=m_lstFrames[KEY_FRAME]->m_pImage->m_nWidth;
	int h=m_lstFrames[KEY_FRAME]->m_pImage->m_nHeight;	

	//////////////////////////////////////////////////////////////////////////	
	// delaunay triangulation		
	Vector2f *akVertex=new Vector2f [m_lstVertices.size()];
	CPVertex	**pTemp=new CPVertex*[w*h];
	memset(pTemp,0,w*h*sizeof(CPVertex*));

	int k=0;
	int nVertices=0;
	for (i=m_lstVertices.begin();i!=m_lstVertices.end();i++,k++)
	{
		CPVertex *pVert=(*i);
		//akVertex[k]=Vector2f((float)(pVert->m_X[0]),(float)(pVert->m_Y[0]));
		int x=pVert->GetX(KEY_FRAME);
		int y=pVert->GetY(KEY_FRAME);

		if (x<0 || y<0 || x>=w || y>=h)
			continue; // do not draw triangles with vertices outside the screen (in the key frame)
		
		akVertex[nVertices]=Vector2f((float)(pVert->GetX(KEY_FRAME)),(float)(pVert->GetY(KEY_FRAME)));	
		pTemp[y*w+x]=pVert;
		nVertices++;
	} //i	

	// calc convex hull
	m_ConvexHull2D=new ConvexHull2f(nVertices,akVertex,false);

	/*
	const int *indices=m_ConvexHull2D->GetIndices();
	for (k=0;k<m_ConvexHull2D->GetQuantity();k++)
	{	
    int idx0=indices[k];
		int idx1=indices[(k+1)%(m_ConvexHull2D->GetQuantity())];
		vector3f v1(akVertex[idx0].X(),akVertex[idx0].Y(),0);
		vector3f v2(akVertex[idx1].X(),akVertex[idx1].Y(),0);
		m_lstFrames[KEY_FRAME]->m_pImage->DrawLine(v1,v2,vector3f(1,1,1),vector3f(0,0,0));
	} //k
	DumpError(m_lstFrames[KEY_FRAME]);
	*/

	int *OutVerts,*OutAdj;
	int nOutTriang;

	Delaunay2af *delaunay=new Delaunay2af(nVertices,akVertex,nOutTriang,OutVerts,OutAdj);
	m_lstTriangles=new CtTriangle [nOutTriang];

	// dummy image for opencv
	IplImage *pImg=cvCreateImage(cvSize(w,h),8,1);
	tRast *pTempRast=new tRast[h];

	for (int iFrame=0;iFrame<(int)(m_lstFrames.size());iFrame++)
	{	
		vector3f *vPoints=m_lstFrames[iFrame]->m_pPoints;
		CtTriangle	**pTris=m_lstFrames[iFrame]->m_pTriangles;

		//The i-th triangle has edge index pairs
		//   edge0 = <raiTVertex[3*i],raiTVertex[3*i+1]>
		//   edge1 = <raiTVertex[3*i+1],raiTVertex[3*i+2]>
		//   edge2 = <raiTVertex[3*i+2],raiTVertex[3*i]>
		for (k=0;k<nOutTriang;k++)	
		{
			// get triangle's edges
			vector3f	vEdges[3][2];
			Vector2f	v2DEdges[3][2];
			CPVertex	*verts[3];
			for (int j=0;j<3;j++)
			{					
				int idx1=OutVerts[k*3+j];			
				int idx2=OutVerts[k*3+((j+1)%3)];

				// get pvert based on 2d edges used for triangulation
				Vector2f *v11=&akVertex[idx1];
				Vector2f *v12=&akVertex[idx2];
				v2DEdges[j][0]=*v11;
				v2DEdges[j][1]=*v12;
				int x1=(int)(v11->X()),y1=(int)(v11->Y());
				int x2=(int)(v12->X()),y2=(int)(v12->Y());
				CPVertex *vp1=pTemp[y1*w+x1];
				CPVertex *vp2=pTemp[y2*w+x2];
				vEdges[j][0]=vp1->m_vPos;
				vEdges[j][1]=vp2->m_vPos;
				verts[j]=vp1;

				// get again 2d edges based on frame
				vector3f v2DPos=vp1->m_v2DPos[iFrame];
				v2DEdges[j][0]=Vector2f((float)(v2DPos.x),(float)(v2DPos.y));
				v2DPos=vp2->m_v2DPos[iFrame];
				v2DEdges[j][1]=Vector2f((float)(v2DPos.x),(float)(v2DPos.y));

				//vector3f v1(v11->X(),v11->Y(),0);
				//vector3f v2(v12->X(),v12->Y(),0);
				//m_lstFrames[KEY_FRAME]->m_pImage->DrawLine(v1,v2,vector3f(0,0,0),vector3f(0,0,0));
			} //j

			if (iFrame==KEY_FRAME)
			{
				for (int j=0;j<3;j++)
					m_lstTriangles[k].m_pVerts[j]=verts[j];
				m_lstTriangles[k].RefreshInternals();
			}

			// draw only tri pointers on the 2d screen
			// get bary coords during ray tracing
			memset(pTempRast,0,sizeof(tRast)*h);
			int ymin=h,ymax=0;
			for (int j=0;j<3;j++)
			{ 
				CvLineIterator cviter;
				CvPoint pt1=cvPoint((int)(v2DEdges[j][0].X()),(int)(v2DEdges[j][0].Y()));
				CvPoint pt2=cvPoint((int)(v2DEdges[j][1].X()),(int)(v2DEdges[j][1].Y()));
				if (pt1.y==pt2.y)
					continue;
				if (pt1.x<0 || pt1.y<0 || pt1.x>=w || pt1.y>=h)
					continue;
				if (pt2.x<0 || pt2.y<0 || pt2.x>=w || pt2.y>=h)
					continue;
				int count=cvInitLineIterator(pImg, pt1, pt2, &cviter, 8 );
				//vector3f vPos=vEdges[j][0];
				//vector3f vStep=(vEdges[j][1]-vEdges[j][0]);
				//vStep/=(ftype)(count);
				int lasty=-1;
				//for (int i=0;i<count;i++,vPos+=vStep)
				for (int i=0;i<count;i++)
				{
					int offset, x, y;
					// assume that ROI is not set, otherwise need to take it into account. 
					offset=(int)(cviter.ptr-(uchar*)(pImg->imageData));
					y=offset/w;
					//if (y<0 || y>=h || 
					if	(y==lasty || pTempRast[y].nPos>1)
					{
						CV_NEXT_LINE_POINT(cviter);
						continue;
					}
					x=(offset-y*w);				
					/*
					if (x<0 || x>=w)
					{
						CV_NEXT_LINE_POINT(cviter);
						continue;
					}
					*/

					// triangle vertices could end up outside the screen 
					// in the other views
					
					pTempRast[y].x[pTempRast[y].nPos]=x;
					//pTempRast[y].vPos[pTempRast[y].nPos]=vPos;
					pTempRast[y].nPos++;				

					if (y<ymin) ymin=y;
					if (y>ymax) ymax=y;
					lasty=y;
					CV_NEXT_LINE_POINT(cviter);
				} //i			
			} //j

			if (ymin==h || ymax==0)
				continue;

			for (int y=ymin;y<=ymax;y++)
			{
				vector3f v1,v2;
				if (pTempRast[y].nPos<1)
					continue;

				// triangle vertices could end up outside the screen 
				// in the other views
				if (y<0 || y>=h)
					continue;

				int len=iabs(pTempRast[y].x[1]-pTempRast[y].x[0])+1;
				int xstep; //=(pTempRast[y].x[1]-pTempRast[y].x[0])/len;
				if (pTempRast[y].x[1]>=pTempRast[y].x[0])
					xstep=1;
				else
					xstep=-1;
				//vector3f vRes,vPos=pTempRast[y].vPos[0];
				//vector3f vStep=(pTempRast[y].vPos[1]-pTempRast[y].vPos[0]);
				//vStep/=(ftype)(len);
				int x=pTempRast[y].x[0];
				//for (int i=0;i<len;i++,x+=xstep,vPos+=vStep)	
				for (int i=0;i<len;i++,x+=xstep)	
				{
					// triangle vertices could end up outside the screen 
					// in the other views
					if (x<0 || x>=w)
						continue;
					//vPoints[y*w+x]=vPos;	
					//if (iFrame==KEY_FRAME)
					{
						pTris[y*w+x]=&m_lstTriangles[k];
					}
					
					/*
					if (i==0 || i==len-1)
						m_lstFrames[KEY_FRAME]->m_pImage->m_pRawPixels[y*w*3+x*3+0]=255;
					else
						m_lstFrames[KEY_FRAME]->m_pImage->m_pRawPixels[y*w*3+x*3+2]=255;				
						*/
				} //i
			} //y
		} //k
		MyOutputDebugString("Frame %d completed \n",iFrame);
	} //iframe

	//SaveTga(m_lstFrames[KEY_FRAME]->m_pImage->m_pRawPixels,3,w,h,"Test.tga",true);

	delete [] pTemp;
	delete [] pTempRast;
	cvReleaseImage(&pImg);

	delete delaunay;

	SAFE_DELETE_ARRAY(OutVerts);
	SAFE_DELETE_ARRAY(OutAdj);
	SAFE_DELETE_ARRAY(akVertex);
}


//////////////////////////////////////////////////////////////////////////
void CPhotoBump10Doc::InitialTriangulation()
{
	lstPhotoVerticesIt i;

	int w=m_lstFrames[KEY_FRAME]->m_pImage->m_nWidth;
	int h=m_lstFrames[KEY_FRAME]->m_pImage->m_nHeight;
	vector3f *vPoints=m_lstFrames[KEY_FRAME]->m_pPoints;
	CCamera *pCam1=&m_lstFrames[KEY_FRAME]->m_Camera;
	ftype		*pDepth=m_lstFrames[KEY_FRAME]->m_pBestDepth;

	//////////////////////////////////////////////////////////////////////////	
	// delaunay triangulation
		
	Vector2f *akVertex=new Vector2f [m_lstVertices.size()];
	CPVertex	**pTemp=new CPVertex*[w*h];
	memset(pTemp,0,w*h*sizeof(CPVertex*));

	int k=0;
	int nVertices=0;
	for (i=m_lstVertices.begin();i!=m_lstVertices.end();i++,k++)
	{
		CPVertex *pVert=(*i);
		//akVertex[k]=Vector2f((float)(pVert->m_X[0]),(float)(pVert->m_Y[0]));
		int x=pVert->GetX(0);
		int y=pVert->GetY(0);

		if (x<0 || y<0 || x>=w || y>=h)
			continue;
		
		akVertex[nVertices]=Vector2f((float)(pVert->GetX(0)),(float)(pVert->GetY(0)));	
		pTemp[y*w+x]=pVert;
		nVertices++;
	} //i	

	int *OutVerts,*OutAdj;
	int nOutTriang;

	Delaunay2af *delaunay=new Delaunay2af(nVertices,akVertex,nOutTriang,OutVerts,OutAdj);


	// dummy image for opencv
	IplImage *pImg=cvCreateImage(cvSize(w,h),8,1);
	tRast *pTempRast=new tRast[h];

	//The i-th triangle has edge index pairs
  //   edge0 = <raiTVertex[3*i],raiTVertex[3*i+1]>
  //   edge1 = <raiTVertex[3*i+1],raiTVertex[3*i+2]>
  //   edge2 = <raiTVertex[3*i+2],raiTVertex[3*i]>
	for (k=0;k<nOutTriang;k++)	
	{
		vector3f	vEdges[3][2];
		Vector2f	v2DEdges[3][2];
		for (int j=0;j<3;j++)
		{					
			int idx1=OutVerts[k*3+j];			
			int idx2=OutVerts[k*3+((j+1)%3)];
			Vector2f *v11=&akVertex[idx1];
			Vector2f *v12=&akVertex[idx2];
			v2DEdges[j][0]=*v11;
			v2DEdges[j][1]=*v12;
			int x1=(int)(v11->X()),y1=(int)(v11->Y());
			int x2=(int)(v12->X()),y2=(int)(v12->Y());
			vEdges[j][0]=pTemp[y1*w+x1]->m_vPos;
			vEdges[j][1]=pTemp[y2*w+x2]->m_vPos;
			//vector3f v1(v11->X(),v11->Y(),0);
			//vector3f v2(v12->X(),v12->Y(),0);
			//m_lstFrames[KEY_FRAME]->m_pImage->DrawLine(v1,v2,vector3f(0,0,0),vector3f(0,0,0));
		} //j
		//CPlanef tPlane;
		//if (!tPlane.CalcPlane(vEdges[0][0],vEdges[0][1],vEdges[1][1]))
		//	Break();		
		memset(pTempRast,0,sizeof(tRast)*h);
		int ymin=h,ymax=0;
		for (int j=0;j<3;j++)
		{ 
			CvLineIterator cviter;
			CvPoint pt1=cvPoint((int)(v2DEdges[j][0].X()),(int)(v2DEdges[j][0].Y()));
			CvPoint pt2=cvPoint((int)(v2DEdges[j][1].X()),(int)(v2DEdges[j][1].Y()));
			if (pt1.y==pt2.y)
				continue;
			int count=cvInitLineIterator(pImg, pt1, pt2, &cviter, 8 );
			vector3f vPos=vEdges[j][0];
			vector3f vStep=(vEdges[j][1]-vEdges[j][0]);
			vStep/=(ftype)(count);
			int lasty=-1;
      for (int i=0;i<count;i++,vPos+=vStep)
			{
				int offset, x, y;
        // assume that ROI is not set, otherwise need to take it into account. 
        offset=(int)(cviter.ptr-(uchar*)(pImg->imageData));
        y=offset/w;
				if (y<0 || y>=h || y==lasty || pTempRast[y].nPos>1)
				{
					CV_NEXT_LINE_POINT(cviter);
					continue;
				}
        x=(offset-y*w);				
				if (x<0 || x>=w)
				{
					CV_NEXT_LINE_POINT(cviter);
					continue;
				}
				
				pTempRast[y].x[pTempRast[y].nPos]=x;
				pTempRast[y].vPos[pTempRast[y].nPos]=vPos;
				pTempRast[y].nPos++;				

				if (y<ymin) ymin=y;
				if (y>ymax) ymax=y;
				lasty=y;
				CV_NEXT_LINE_POINT(cviter);
			} //i			
		} //j

		if (ymin==h || ymax==0)
			continue;

		for (int y=ymin;y<=ymax;y++)
		{
			vector3f v1,v2;
			if (pTempRast[y].nPos<1)
				continue;
			int len=iabs(pTempRast[y].x[1]-pTempRast[y].x[0])+1;
			int xstep; //=(pTempRast[y].x[1]-pTempRast[y].x[0])/len;
			if (pTempRast[y].x[1]>=pTempRast[y].x[0])
				xstep=1;
			else
				xstep=-1;
			vector3f vRes,vPos=pTempRast[y].vPos[0];
			vector3f vStep=(pTempRast[y].vPos[1]-pTempRast[y].vPos[0]);
			vStep/=(ftype)(len);
			int x=pTempRast[y].x[0];
			for (int i=0;i<len;i++,x+=xstep)	
			{
				vPoints[y*w+x]=vPos;
				vPos+=vStep;
				m_pVideo->ProjectToScreen(vPos,vRes,pCam1);
				pDepth[y*w+x]=vRes.z;
				
				/*
				if (i==0 || i==len-1)
					m_lstFrames[KEY_FRAME]->m_pImage->m_pRawPixels[y*w*3+x*3+0]=255;
				else
					m_lstFrames[KEY_FRAME]->m_pImage->m_pRawPixels[y*w*3+x*3+2]=255;				
					*/
			} //i
		} //y

	} //k

	//SaveTga(m_lstFrames[KEY_FRAME]->m_pImage->m_pRawPixels,3,w,h,"Test.tga",true);

	delete [] pTemp;
	delete [] pTempRast;
	cvReleaseImage(&pImg);

	delete delaunay;

	SAFE_DELETE_ARRAY(OutVerts);
	SAFE_DELETE_ARRAY(OutAdj);
	SAFE_DELETE_ARRAY(akVertex);
}
