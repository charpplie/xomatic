

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

#include "klt/klt.h"

//#include "GaussianBlur.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif 

#define NUM_FEATURES_TRACK	1000

//////////////////////////////////////////////////////////////////////////
bool CPhotoBump10Doc::DetectFeatures(CPhotoFrame *pFrame,int nFrames)
{			
	// find features for this image
	
	int w=pFrame->m_pImage->m_nWidth;
	int h=pFrame->m_pImage->m_nHeight;

	pFrame->m_pImage->GrayScale();
	pFrame->m_pImage->ExtractEdges(false);
	//pFrame->m_pImage->ExtractEdges(true);

	uchar *pGray1=pFrame->m_pImage->m_pRawPixels;		
			
	if (!m_bLoadedFromBojou)
	{	
		KLT_TrackingContext m_tc;
		KLT_FeatureList m_fl;
		KLT_FeatureTable m_ft;

		m_tc = KLTCreateTrackingContext();		
		m_fl = KLTCreateFeatureList(NUM_FEATURES_TRACK);
		m_ft = KLTCreateFeatureTable(nFrames, NUM_FEATURES_TRACK);  

		m_tc->sequentialMode = TRUE;
		//m_tc->borderx=32;
		//m_tc->bordery=32; 
		//m_tc->mindist=32;

		KLTSelectGoodFeatures(m_tc, pGray1, w, h, m_fl);
		KLTStoreFeatureList(m_fl, m_ft, 0);  		

		for (int k=0;k<m_ft->nFeatures;k++)  
		{				
			if (m_ft->feature[k][0]->val<0)
				continue;		
			if (m_ft->feature[k][0]->x<=32 || m_ft->feature[k][0]->x>=w-32)
				continue;
			if (m_ft->feature[k][0]->y<=32 || m_ft->feature[k][0]->y>=h-32)
				continue;

			CPVertex *pVert=new CPVertex(nFrames);

			pVert->m_v2DPos[pFrame->m_nFrameNum].x=m_ft->feature[k][0]->x;
			pVert->m_v2DPos[pFrame->m_nFrameNum].y=m_ft->feature[k][0]->y;						
			pVert->m_v2DPos[pFrame->m_nFrameNum].z=1.0;

			pFrame->m_lstFeatures.push_back(pVert);

			vector3f v0=pVert->m_v2DPos[pFrame->m_nFrameNum];

			//pFrame->m_pImage->DrawPoint(v0,5,255,255,255);
			//pFrame->m_pImage->DrawLine(v0+vector3f(-5,-5,1.0),v0+vector3f(5,-5,1.0),vector3f(255,255,255),vector3f(255,255,255));
			//pFrame->m_pImage->DrawLine(v0+vector3f(5,-5,1.0),v0+vector3f(5,5,1.0),vector3f(255,255,255),vector3f(255,255,255));
			//pFrame->m_pImage->DrawLine(v0+vector3f(5,5,1.0),v0+vector3f(-5,5,1.0),vector3f(255,255,255),vector3f(255,255,255));
			//pFrame->m_pImage->DrawLine(v0+vector3f(-5,5,1.0),v0+vector3f(-5,-5,1.0),vector3f(255,255,255),vector3f(255,255,255));					
		} //k

		KLTFreeFeatureTable(m_ft);	
		KLTFreeFeatureList(m_fl);	
		KLTFreeTrackingContext(m_tc);		
	}
	else
	{
		for (int k=0;k<(int)(m_lstVertices.size());k++)
		{
			CPVertex *pVert1=m_lstVertices[k];
			if (pVert1->m_v2DPos[pFrame->m_nFrameNum].x<0 || pVert1->m_v2DPos[pFrame->m_nFrameNum].y<0 
				|| pVert1->m_v2DPos[pFrame->m_nFrameNum].x>=w || pVert1->m_v2DPos[pFrame->m_nFrameNum].y>=h)
			{
				pVert1->m_v2DPos[pFrame->m_nFrameNum].Set(-1,-1,-1);
				continue;
			}

			pVert1->m_v2DPos[pFrame->m_nFrameNum].z=1.0;

			CPVertex *pVert=new CPVertex(nFrames);
			memcpy(pVert->m_v2DPos,pVert1->m_v2DPos,sizeof(vector3f)*nFrames);
			pVert->m_vPos=pVert1->m_vPos;
			pVert->m_vColor=pVert1->m_vColor;
			
			pFrame->m_lstFeatures.push_back(pVert);
		} //k
	}

	//pFrame->m_pImage->SaveAsTga(NULL,"Features");

	return (true);
}

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10Doc::FollowEdge(int x11,int y11,int x22,int y22,CPhotoImage *pImg1,CPhotoImage *pImg2,lstPhotoVertices *tempVerts,int nFrame1,int nFrame2,int w,int h,int nFrames)
{
	if (x11<32 || y11<32 || x22<32 || y22<32)
		return;

	if ((x11>w-32) || (y11>h-32) || (x22>w-32) || (y22>h-32))
		return;


	uchar *pEdge1=pImg1->m_pEdges;
	uchar *pEdge2=pImg2->m_pEdges;

	if (!pEdge1[y11*w+x11])
		return;

	if (!pEdge2[y22*w+x22])
		return;

	vector3float *vNorms1=pImg1->m_vNormals;
	vector3float *vNorms2=pImg2->m_vNormals;

	vector3float v1=vNorms1[y11*w+x11];
	vector3f v11(v1.x,v1.y,v1.z);

	vector3float v2=vNorms2[y22*w+x22];
	vector3f v22(v2.x,v2.y,v2.z);

	ftype fDist=v11.Distance2(v22);
	if (fDist>=0.01)
		return;


	/*
	int nX=-1,nY=-1;
	ftype fBestDist=0.01;
	for (int k=0;k<9;k++)
	{
		int x222=x22+dirs[k][0];
		int y222=y22+dirs[k][1];

		if (!pEdge2[y222*w+x222])
			continue;
		
		vector3float v2=vNorms2[y222*w+x222];
		vector3f v22(v2.x,v2.y,v2.z);

		ftype fDist=v11.Distance2(v22);
		if (fDist<fBestDist)
		{
			nX=x222;
			nY=y222;
			fBestDist=fDist;
		}
	} //k	

	if (nX<0)
		return;

	x22=nX;
	y22=nY;
	*/

	CPVertex *pVert=new CPVertex(nFrames);

	// store for this pair of frames				
	pVert->m_v2DPos[nFrame1].Set(x11,y11,1.0);
	pVert->m_v2DPos[nFrame2].Set(x22,y22,1.0);

	tempVerts->push_back(pVert);

	// remove it
	vNorms1[y11*w+x11].Clear();
	vNorms2[y22*w+x22].Clear();
	pEdge1[y11*w+x11]=0;
	pEdge2[y22*w+x22]=0;

	for (int k=1;k<9;k++)
	{
		int x111=x11+dirs[k][0];
		int y111=y11+dirs[k][1];

		int x222=x22+dirs[k][0];
		int y222=y22+dirs[k][1];

		FollowEdge(x111,y111,x222,y222,pImg1,pImg2,tempVerts,nFrame1,nFrame2,w,h,nFrames);
	} //k
}

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10Doc::FollowEdge(const vector3f &v1,const vector3f &v2,CPhotoImage *pImg1,CPhotoImage *pImg2,lstPhotoVertices *tempVerts,int nFrame1,int nFrame2,int w,int h,int nFrames)
{
	int x1=(int)(v1.x);
	int y1=(int)(v1.y);
	int x2=(int)(v2.x);
	int y2=(int)(v2.y);

	FollowEdge(x1,y1,x2,y2,pImg1,pImg2,tempVerts,nFrame1,nFrame2,w,h,nFrames);
}

//////////////////////////////////////////////////////////////////////////
vector3f CalcAverageDirection(int _x,int _y,int w,int nRadius,uchar *pPixels)
{
	int tempdirs[3][2]={{0,0},{1,0},{0,1}};
	ftype fC[3];

	vector3f vSum(0,0,0);
	ftype fVecSum=0;
	
	for (int y=_y-nRadius;y<_y+nRadius;y++)
	{		
		for (int x=_x-nRadius;x<_x+nRadius;x++)
		{
		
			for (int k=0;k<3;k++)
			{
				int x1=x+tempdirs[k][0];
				int y1=y+tempdirs[k][1];

				ftype fSum=0;
				for (int k1=0;k1<9;k1++)
				{
					int x2=x1+dirs[k1][0];
					int y2=y1+dirs[k1][1];
					fSum+=pPixels[y2*w+x2];
				} //k1
				fC[k]=(fSum/9.0);				
			} //k

			ftype dcx=32.0*(fC[0]-fC[1]);
			ftype dcy=32.0*(fC[0]-fC[2]);

			// Normalize the vector 
			ftype sqlen = dcx*dcx + dcy*dcy + 1;
			ftype reciplen = 1.0/(ftype)sqrt(sqlen);			
			ftype nx = dcy*reciplen;
			ftype ny = -dcx*reciplen;
			ftype nz = reciplen;				

			//m_vNormals[y*w+x].Set((float)(nx),(float)(ny),(float)(nz));
			vSum+=vector3f(nx,ny,nz);

			fVecSum+=1.0f;
		}
	}

	vSum/=fVecSum;
	return (vSum);
}

//////////////////////////////////////////////////////////////////////////
int CPhotoBump10Doc::MatchFeatures(CPhotoFrame *pFrame1,CPhotoFrame *pFrame2,lstPhotoVertices *tempVerts,int nFrames)
{
	int w=pFrame1->m_pImage->m_nWidth;
	int h=pFrame1->m_pImage->m_nHeight;
	int nRadius1=5;	
	int nwLen1=(nRadius1*2+1);
	ftype fMaxDist=ftype(max(w,h))*0.15;

	/*
	//////////////////////////////////////////////////////////////////////////
	// uncomment to draw matches
	//CPhotoImage *pImg1=pFrame1->m_pImage->Resize(w,h,pFrame1->m_pImage->m_nBpp,false);
	//CPhotoImage *pImg2=pFrame2->m_pImage->Resize(w,h,pFrame2->m_pImage->m_nBpp,false);
	//////////////////////////////////////////////////////////////////////////

	uchar *ptr1=pFrame1->m_pImage->m_pRawPixels;
	uchar *ptr2=pFrame2->m_pImage->m_pRawPixels;

	for (lstPhotoVerticesIt i1=pFrame1->m_lstFeatures.begin();i1!=pFrame1->m_lstFeatures.end();i1++)
	{
		CPVertex *pVert1=(*i1);		

		int x1=(int)(pVert1->m_v2DPos[pFrame1->m_nFrameNum].x);
		int y1=(int)(pVert1->m_v2DPos[pFrame1->m_nFrameNum].y);		

		pVert1->m_vPos=CalcAverageDirection(x1,y1,w,nRadius1,ptr1);
	} //i1

	for (lstPhotoVerticesIt i2=pFrame2->m_lstFeatures.begin();i2!=pFrame2->m_lstFeatures.end();i2++)		
	{
		CPVertex *pVert2=(*i2);
		int x1=(int)(pVert2->m_v2DPos[pFrame2->m_nFrameNum].x);
		int y1=(int)(pVert2->m_v2DPos[pFrame2->m_nFrameNum].y);		

		pVert2->m_vPos=CalcAverageDirection(x1,y1,w,nRadius1,ptr2);
	}

	for (lstPhotoVerticesIt i1=pFrame1->m_lstFeatures.begin();i1!=pFrame1->m_lstFeatures.end();i1++)
	{
		CPVertex *pVert1=(*i1);		

		float fBestError=10;
		CPVertex *pBest=NULL;

		int x1=(int)(pVert1->m_v2DPos[pFrame1->m_nFrameNum].x);
		int y1=(int)(pVert1->m_v2DPos[pFrame1->m_nFrameNum].y);		

		if (!m_bLoadedFromBojou)
		{		
			for (lstPhotoVerticesIt i2=pFrame2->m_lstFeatures.begin();i2!=pFrame2->m_lstFeatures.end();i2++)		
			{
				CPVertex *pVert2=(*i2);

				if (pVert2->m_vColor.x<0)
					continue;

				ftype fDist=pVert2->m_v2DPos[pFrame2->m_nFrameNum].Distance(pVert1->m_v2DPos[pFrame1->m_nFrameNum]);
				if (fDist>fMaxDist)
					continue;

				ftype fError=pVert1->m_vPos.Distance2(pVert2->m_vPos);

				if (fError<fBestError)
				{
					fBestError=fError;
					pBest=pVert2;
				}		
			} //i2

			if (fBestError>=0.0005)
				continue;

			//if (nBestError>1000)
			//	continue;
		}
		else
		{
			float fBestDist=0;
			for (lstPhotoVerticesIt i2=pFrame2->m_lstFeatures.begin();i2!=pFrame2->m_lstFeatures.end();i2++)		
			{
				CPVertex *pVert2=(*i2);

				if (pVert2->m_vColor.x<0)
					continue;

				// compare in the same frame
				float fDist=pVert1->m_v2DPos[pFrame1->m_nFrameNum].Distance(pVert2->m_v2DPos[pFrame1->m_nFrameNum]);
				if (!pBest || fDist<fBestDist)
				{
					pBest=pVert2;
					fBestDist=fDist;
				}
			} //i2
		}

		if (!pBest)
			continue;

		CPVertex *pVert=new CPVertex(nFrames);

		// store for this pair of frames				
		pVert->m_v2DPos[pFrame1->m_nFrameNum]=pVert1->m_v2DPos[pFrame1->m_nFrameNum];
		pVert->m_v2DPos[pFrame2->m_nFrameNum]=pBest->m_v2DPos[pFrame2->m_nFrameNum];

		//////////////////////////////////////////////////////////////////////////	
		// uncomment to draw matches
		//pImg1->DrawPoint(pVert->m_v2DPos[pFrame1->m_nFrameNum],14,255,255,255);
		//pImg2->DrawPoint(pVert->m_v2DPos[pFrame2->m_nFrameNum],14,255,255,255);
		//////////////////////////////////////////////////////////////////////////		

		tempVerts->push_back(pVert);

		pBest->m_vColor.x=-1;

		//////////////////////////////////////////////////////////////////////////
		// match edges! start within the window where the match was found

		int x2=(int)(pVert->m_v2DPos[pFrame2->m_nFrameNum].x);
		int y2=(int)(pVert->m_v2DPos[pFrame2->m_nFrameNum].y);

		vector3float *vNorms1=pFrame1->m_pImage->m_vNormals;
		vector3float *vNorms2=pFrame2->m_pImage->m_vNormals;

		for (int y=-nRadius1*2;y<=nRadius1*2;y++)
		{
			for (int x=-nRadius1*2;x<=nRadius1*2;x++)
			{						
				int x11=x+x1;
				int y11=y+y1;
				int x22=x+x2;
				int y22=y+y2;

				FollowEdge(x11,y11,x22,y22,pFrame1->m_pImage,pFrame2->m_pImage,tempVerts,pFrame1->m_nFrameNum,pFrame2->m_nFrameNum,w,h,nFrames);
			} //x
		} //y
	} //i1	

	pFrame1->RemoveVertices();
	SAFE_DELETE_ARRAY(pFrame1->m_pImage->m_vNormals);

	// clear up vPos as it is gonna be used for 3d pos later
	for (lstPhotoVerticesIt i1=pFrame1->m_lstFeatures.begin();i1!=pFrame1->m_lstFeatures.end();i1++)
	{
		CPVertex *pVert1=(*i1);		
		pVert1->m_vPos.Clear();
	}

	for (lstPhotoVerticesIt i2=pFrame2->m_lstFeatures.begin();i2!=pFrame2->m_lstFeatures.end();i2++)		
	{
		CPVertex *pVert2=(*i2);
		pVert2->m_vPos.Clear();
	}

	//////////////////////////////////////////////////////////////////////////	
	// uncomment to draw matches
	//pImg1->SaveAsTga(NULL,"Features_%d_%d",pFrame1->m_nFrameNum,pFrame2->m_nFrameNum);	
	//pImg2->SaveAsTga(NULL,"Features_%d_%d",pFrame1->m_nFrameNum,pFrame2->m_nFrameNum);	
	//delete pImg1;
	//delete pImg2;
	//////////////////////////////////////////////////////////////////////////

	return ((int)(tempVerts->size()));
	*/

	
	//////////////////////////////////////////////////////////////////////////
	// uncomment to draw matches
	//CPhotoImage *pImg1=pFrame1->m_pImage->Resize(w,h,pFrame1->m_pImage->m_nBpp,false);
	//CPhotoImage *pImg2=pFrame2->m_pImage->Resize(w,h,pFrame2->m_pImage->m_nBpp,false);
	//////////////////////////////////////////////////////////////////////////

	uchar *ptr1=pFrame1->m_pImage->m_pRawPixels;
	uchar *ptr2=pFrame2->m_pImage->m_pRawPixels;
	
	for (lstPhotoVerticesIt i1=pFrame1->m_lstFeatures.begin();i1!=pFrame1->m_lstFeatures.end();i1++)
	{
		CPVertex *pVert1=(*i1);		

		ftype fBestError=99999;
		CPVertex *pBest=NULL;

		int x1=(int)(pVert1->m_v2DPos[pFrame1->m_nFrameNum].x);
		int y1=(int)(pVert1->m_v2DPos[pFrame1->m_nFrameNum].y);		

		if (!m_bLoadedFromBojou)
		{		
			for (lstPhotoVerticesIt i2=pFrame2->m_lstFeatures.begin();i2!=pFrame2->m_lstFeatures.end();i2++)		
			{
				CPVertex *pVert2=(*i2);

				if (pVert2->m_vColor.x<0)
					continue;

				ftype fDist=pVert2->m_v2DPos[pFrame2->m_nFrameNum].Distance(pVert1->m_v2DPos[pFrame1->m_nFrameNum]);
				if (fDist>fMaxDist)
					continue;

				int x2=(int)(pVert2->m_v2DPos[pFrame2->m_nFrameNum].x);
				int y2=(int)(pVert2->m_v2DPos[pFrame2->m_nFrameNum].y);			
				
				ftype	fError=0;
				for (int y=-nRadius1;y<=nRadius1;y++)
				{
					for (int x=-nRadius1;x<=nRadius1;x++)
					{						
						//////////////////////////////////////////////////////////////////////////
						// new code
						ftype fSum1=0;
						ftype fSum2=0;
						for (int k1=0;k1<9;k1++)
						{
							int x3=x+x1+dirs[k1][0];
							int y3=y+y1+dirs[k1][1];
							fSum1+=ptr1[y3*w+x3];
							int x4=x+x2+dirs[k1][0];
							int y4=y+y2+dirs[k1][1];
							fSum2+=ptr2[y4*w+x4];
						} //k1
						fSum1/=9.0;
						fSum2/=9.0;
						fError+=fabs(fSum1-fSum2);

						//////////////////////////////////////////////////////////////////////////
						
						// old code
						//nError+=abs(((int)ptr2[(y+y2)*w+(x+x2)])-
						//(int)(ptr1[(y+y1)*w+(x+x1)]));
					} //x
					if (fError>fBestError)
						break;
				} //y

				if (fError<fBestError)
				{
					fBestError=fError;
					pBest=pVert2;
				}		
			} //i2
			
			if (fBestError>1000)
				continue;
		}
		else
		{
			float fBestDist=0;
			for (lstPhotoVerticesIt i2=pFrame2->m_lstFeatures.begin();i2!=pFrame2->m_lstFeatures.end();i2++)		
			{
				CPVertex *pVert2=(*i2);

				if (pVert2->m_vColor.x<0)
					continue;

				// compare in the same frame
				float fDist=(float)pVert1->m_v2DPos[pFrame1->m_nFrameNum].Distance(pVert2->m_v2DPos[pFrame1->m_nFrameNum]);
				if (!pBest || fDist<fBestDist)
				{
					pBest=pVert2;
					fBestDist=fDist;
				}
			} //i2
		}

		if (!pBest)
			continue;

		CPVertex *pVert=new CPVertex(nFrames);

		// store for this pair of frames				
		pVert->m_v2DPos[pFrame1->m_nFrameNum]=pVert1->m_v2DPos[pFrame1->m_nFrameNum];
		pVert->m_v2DPos[pFrame2->m_nFrameNum]=pBest->m_v2DPos[pFrame2->m_nFrameNum];

		//////////////////////////////////////////////////////////////////////////	
		// uncomment to draw matches
		//pImg1->DrawPoint(pVert->m_v2DPos[pFrame1->m_nFrameNum],14,255,255,255);
		//pImg2->DrawPoint(pVert->m_v2DPos[pFrame2->m_nFrameNum],14,255,255,255);
		//////////////////////////////////////////////////////////////////////////		

		tempVerts->push_back(pVert);

		pBest->m_vColor.x=-1;

		//////////////////////////////////////////////////////////////////////////
		// match edges! start within the window where the match was found

		int x2=(int)(pVert->m_v2DPos[pFrame2->m_nFrameNum].x);
		int y2=(int)(pVert->m_v2DPos[pFrame2->m_nFrameNum].y);

		vector3float *vNorms1=pFrame1->m_pImage->m_vNormals;
		vector3float *vNorms2=pFrame2->m_pImage->m_vNormals;
			
		for (int y=-nRadius1*2;y<=nRadius1*2;y++)
		{
			for (int x=-nRadius1*2;x<=nRadius1*2;x++)
			{						
				int x11=x+x1;
				int y11=y+y1;
				int x22=x+x2;
				int y22=y+y2;
				
				FollowEdge(x11,y11,x22,y22,pFrame1->m_pImage,pFrame2->m_pImage,tempVerts,pFrame1->m_nFrameNum,pFrame2->m_nFrameNum,w,h,nFrames);
			} //x
		} //y
	} //i1	

	pFrame1->RemoveVertices();
	SAFE_DELETE_ARRAY(pFrame1->m_pImage->m_vNormals);

	//////////////////////////////////////////////////////////////////////////	
	// uncomment to draw matches
	//pImg1->SaveAsTga(NULL,"Features_%d_%d",pFrame1->m_nFrameNum,pFrame2->m_nFrameNum);	
	//pImg2->SaveAsTga(NULL,"Features_%d_%d",pFrame1->m_nFrameNum,pFrame2->m_nFrameNum);	
	//delete pImg1;
	//delete pImg2;
	//////////////////////////////////////////////////////////////////////////

	return ((int)(tempVerts->size()));
	
}

//////////////////////////////////////////////////////////////////////////
ftype CPhotoBump10Doc::CalcNormalizedCrossCorrelation(int x1,int y1,int x2,int y2,uchar *ptr1,uchar *ptr2,int w,int h,int nWindow)
{
	ftype fSums[2]={0,0};
	ftype fSumNorm[2]={0,0};
	ftype fArea[2]={0,0};
	CvPoint coords[2];
	coords[0].x=x1;coords[0].y=y1;
	coords[1].x=x2;coords[1].y=y2;
	uchar *ptrs[2]={ptr1,ptr2};

	if (nWindow>5)
	{	
		for (int k=0;k<2;k++)
		{	
			for (int k1=0;k1<49;k1++)
			{					
				int x3=coords[k].x+dirs77[k1][0];
				int y3=coords[k].y+dirs77[k1][1];
				if (x3<0 || y3<0 || x3>=w || y3>=h)
					continue;
				fSums[k]+=ptrs[k][y3*w+x3];
				fArea[k]+=1.0;			
			} //k1

			if (fSums[k]==0 || fArea[k]==0)
				return (0);		
			fSumNorm[k]=fSums[k]/fArea[k];
		} //k

		ftype xy=0,xx=0,yy=0;	
		ftype Intens[2];
		for (int k1=0;k1<49;k1++)
		{					
			int k;
			for (k=0;k<2;k++)
			{		
				int x3=coords[k].x+dirs77[k1][0];
				int y3=coords[k].y+dirs77[k1][1];
				if (x3<0 || y3<0 || x3>=w || y3>=h)
					break;
				Intens[k]=ptrs[k][y3*w+x3];
			} //k
			if (k<2)		
				continue;		
			xy+=(Intens[0]-fSumNorm[0])*(Intens[1]-fSumNorm[1]);
			xx+=(Intens[0]-fSumNorm[0])*(Intens[0]-fSumNorm[0]);		
			yy+=(Intens[1]-fSumNorm[1])*(Intens[1]-fSumNorm[1]);		
		} //k1

		ftype fRoot=sqrt(xx*yy);
		if	(fRoot<=0.00000001)
			return (-1);	
		xy/=fRoot;

		return (xy);

	}
	else 
	if (nWindow>3)
	{	
		for (int k=0;k<2;k++)
		{	
			for (int k1=0;k1<25;k1++)
			{					
				int x3=coords[k].x+dirs55[k1][0];
				int y3=coords[k].y+dirs55[k1][1];
				if (x3<0 || y3<0 || x3>=w || y3>=h)
					continue;
				fSums[k]+=ptrs[k][y3*w+x3];
				fArea[k]+=1.0;			
			} //k1

			if (fSums[k]==0 || fArea[k]==0)
				return (0);		
			fSumNorm[k]=fSums[k]/fArea[k];
		} //k

		ftype xy=0,xx=0,yy=0;	
		ftype Intens[2];
		for (int k1=0;k1<25;k1++)
		{					
			int k;
			for (k=0;k<2;k++)
			{		
				int x3=coords[k].x+dirs55[k1][0];
				int y3=coords[k].y+dirs55[k1][1];
				if (x3<0 || y3<0 || x3>=w || y3>=h)
					break;
				Intens[k]=ptrs[k][y3*w+x3];
			} //k
			if (k<2)		
				continue;		
			xy+=(Intens[0]-fSumNorm[0])*(Intens[1]-fSumNorm[1]);
			xx+=(Intens[0]-fSumNorm[0])*(Intens[0]-fSumNorm[0]);		
			yy+=(Intens[1]-fSumNorm[1])*(Intens[1]-fSumNorm[1]);		
		} //k1

		ftype fRoot=sqrt(xx*yy);
		if	(fRoot<=0.00000001)
			return (-1);	
		xy/=fRoot;

		return (xy);

	}
	else 
	//if (nWindow==3)
	{	
		for (int k=0;k<2;k++)
		{	
			for (int k1=0;k1<9;k1++)
			{					
				int x3=coords[k].x+dirs[k1][0];
				int y3=coords[k].y+dirs[k1][1];
				if (x3<0 || y3<0 || x3>=w || y3>=h)
					continue;
				fSums[k]+=ptrs[k][y3*w+x3];
				fArea[k]+=1.0;			
			} //k1

			if (fSums[k]==0 || fArea[k]==0)
				return (0);		
			fSumNorm[k]=fSums[k]/fArea[k];
		} //k

		ftype xy=0,xx=0,yy=0;	
		ftype Intens[2];
		for (int k1=0;k1<9;k1++)
		{					
			int k;
			for (k=0;k<2;k++)
			{		
				int x3=coords[k].x+dirs[k1][0];
				int y3=coords[k].y+dirs[k1][1];
				if (x3<0 || y3<0 || x3>=w || y3>=h)
					break;
				Intens[k]=ptrs[k][y3*w+x3];
			} //k
			if (k<2)		
				continue;		
			xy+=(Intens[0]-fSumNorm[0])*(Intens[1]-fSumNorm[1]);
			xx+=(Intens[0]-fSumNorm[0])*(Intens[0]-fSumNorm[0]);		
			yy+=(Intens[1]-fSumNorm[1])*(Intens[1]-fSumNorm[1]);		
		} //k1

		ftype fRoot=sqrt(xx*yy);
		if	(fRoot<=0.00000001)
			return (-1);	
		xy/=fRoot;

		return (xy);
	}


	// TODO: add RGB distance as in van gool

	return (-1);
}

#define BORDER_X	16
#define BORDER_Y	16

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10Doc::MatchAdditionalFeatures()
{
	int w=m_lstFrames[KEY_FRAME]->m_pImage->m_nWidth;
	int h=m_lstFrames[KEY_FRAME]->m_pImage->m_nHeight;

	int nFrames=(int)(m_lstFrames.size());

	// find min/max z
	m_fMinZ=99999;
	m_fMaxZ=0;	
	for (lstPhotoVerticesIt it=m_lstVertices.begin();it!=m_lstVertices.end();it++)
	{
		CPVertex *pVert=(*it);

		for (int k1=0;k1<nFrames;k1++)
		{		

			CCamera *pCam1=&m_lstFrames[k1]->m_Camera;
			//CCamera *pCam2=&m_lstFrames[k1+1]->m_Camera;
			CPhotoFrame *pFrame1=m_lstFrames[pCam1->m_nFrame];
			//CPhotoFrame *pFrame2=m_lstFrames[pCam2->m_nFrame];

			int x1=pVert->GetX(k1);
			int y1=pVert->GetY(k1);

			if (x1<0 || y1<0)
				continue;

			vector3f vPos;
			m_pVideo->ProjectToScreen(pVert->m_vPos,vPos,&pFrame1->m_Camera);

			if (vPos.z>m_fMaxZ)
				m_fMaxZ=vPos.z;
			if (vPos.z<m_fMinZ)
				m_fMinZ=vPos.z;			
		} //k1
	} //it

	// TODO: get depth maps from all views, then combine them - no more
	// plain 2d but fully 3d

	//////////////////////////////////////////////////////////////////////////
	// match more edges, following epipolar lines

	//CPhotoImage *pTest[3]={0,0,0};

	lstPhotoVertices alledges;
	
	int k1;
	for (k1=0;k1<nFrames-1;k1++)
	{
		CPhotoFrame *pFrame1=m_lstFrames[k1];
		CPhotoFrame *pFrame2=m_lstFrames[k1+1];

		CCamera *pCam1=&pFrame1->m_Camera;
		CCamera *pCam2=&pFrame2->m_Camera;
		
		pFrame1->m_pImage->ExtractEdges(false);
		pFrame2->m_pImage->ExtractEdges(false);

		uchar *ptr1=pFrame1->m_pImage->m_pRawPixels;
		uchar *ptr2=pFrame2->m_pImage->m_pRawPixels;

		/*
		if (!pTest[k1])
		{		
			pTest[k1]=pFrame1->m_pImage->Resize(w,h,24,false);
			for (int k=0;k<w*h;k++) 
			{
				if (!pFrame1->m_pImage->m_pEdges[k])
					continue;
				vector3float vNorm=pFrame1->m_pImage->m_vNormals[k];
				for (int j=0;j<3;j++)
					pTest[k1]->m_pRawPixels[k*3+j]=(uchar) (128.0 + 127.0*vNorm[j]);
			} //k
		}

		if (!pTest[k1+1])
		{		
			pTest[k1+1]=pFrame2->m_pImage->Resize(w,h,24,false);
			for (int k=0;k<w*h;k++)
			{
				if (!pFrame2->m_pImage->m_pEdges[k])
					continue;
				vector3float vNorm=pFrame2->m_pImage->m_vNormals[k];
				for (int j=0;j<3;j++)
					pTest[k1+1]->m_pRawPixels[k*3+j]=(uchar) (128.0 + 127.0*vNorm[j]);
			} //k
		}
		*/

		lstPhotoVertices edges;
		ftype fMaxDist=ftype(max(w,h))*0.05;

		for (lstPhotoVerticesIt it=m_lstVertices.begin();it!=m_lstVertices.end();it++)
		{
			CPVertex *pVert=(*it);

			pVert->m_vColor.Set(0,1,0);

			if (pVert->m_v2DPos[k1].x<0 || pVert->m_v2DPos[k1].y<0)
				continue;		

			if (pVert->m_v2DPos[k1+1].x<0 || pVert->m_v2DPos[k1+1].y<0)
				continue;		

			// remove it
			int x11=(int)(pVert->m_v2DPos[k1].x);
			int y11=(int)(pVert->m_v2DPos[k1].y);
			int x22=(int)(pVert->m_v2DPos[k1+1].x);
			int y22=(int)(pVert->m_v2DPos[k1+1].y);

			pFrame1->m_pImage->m_vNormals[y11*w+x11].Clear();
			pFrame1->m_pImage->m_pEdges[y11*w+x11]=0;
			pFrame2->m_pImage->m_vNormals[y22*w+x22].Clear();
			pFrame2->m_pImage->m_pEdges[y22*w+x22]=0;

			vector3f vEpiLines[2];			
			vEpiLines[0]=pVert->m_vEpiLines[0][k1];
			vEpiLines[1]=pVert->m_vEpiLines[1][k1];
			int nIncValues[2];

			// clip within the window and calc best inc values
			// do it for the first epiline only...
			for (int k=0;k<1;k++)
			{
				vector3f vLine1=vEpiLines[k];				
				vector3f v1(0,0,1.0);					
				v1.y=-(vLine1.x*v1.x+vLine1.z)/vLine1.y;
				if (v1.y<0)						
				{
					// a*x + b*y + c = 0
					v1.y=0;v1.x=-(vLine1.y*v1.y+vLine1.z)/vLine1.x;		
					ASSERT(v1.x>=0 && v1.x<w);
				}
				else
				if (v1.y>=(h))									
				{
					v1.y=h-1;v1.x=-(vLine1.y*v1.y+vLine1.z)/vLine1.x;						
					ASSERT(v1.x>=0 && v1.x<w);
				}
				
				vector3f v2(w,0,1.0);
				v2.y=-(vLine1.x*v2.x+vLine1.z)/vLine1.y;					
				if (v2.y<0)						
				{
					// a*x + b*y + c = 0
					v2.y=0;v2.x=-(vLine1.y*v2.y+vLine1.z)/vLine1.x;						
					ASSERT(v2.x>=0 && v2.x<w);
				}
				else
				if (v2.y>=(h))									
				{
					v2.y=h-1;v2.x=-(vLine1.y*v2.y+vLine1.z)/vLine1.x;						
					ASSERT(v2.x>=0 && v2.x<w);
				}				

				// calc best inc values
				if (fabs(v2.x-v1.x)>=fabs(v2.y-v1.y))
				{				
					nIncValues[0]=1;
					nIncValues[1]=0;
				}
				else
				{
					nIncValues[0]=0;
					nIncValues[1]=1;
				}
			}//k

			// proceed in both directions away from the feature match
			for (int k=0;k<2;k++)
			{				
				if (k==1)
				{
					nIncValues[0]=-nIncValues[0];
					nIncValues[1]=-nIncValues[1];
				}

				vector3f vPos[2],vLastPos[2];

				// move them away a bit the first time or they
				// will match again the same edges
				for (int j=0;j<2;j++)
				{				
					vLastPos[j]=pVert->m_v2DPos[k1+j];
					vLastPos[j].x+=nIncValues[0];
					vLastPos[j].y+=nIncValues[1];
					vPos[j]=vLastPos[j];					
				}

				vector3f vNorm1;
				while (1)
				{

					while (1)
					{
						// find a new epiline-edge intersection
						if (nIncValues[0]==0)
						{								
							// increase y 
							vPos[0].y+=(ftype)(nIncValues[1]);
							vPos[0].x=-(vEpiLines[0].y*vPos[0].y+vEpiLines[0].z)/vEpiLines[0].x;
						}
						else
						{
							vPos[0].x+=(ftype)(nIncValues[0]);
							vPos[0].y=-(vEpiLines[0].x*vPos[0].x+vEpiLines[0].z)/vEpiLines[0].y;
						}

						if (vPos[0].x<BORDER_X || vPos[0].x>=(w-BORDER_X) || vPos[0].y<BORDER_Y || vPos[0].y>=(h-BORDER_Y))						
							break;						
						
						int x=(int)(vPos[0].x);
						int y=(int)(vPos[0].y);

						if (!pFrame1->m_pImage->m_pEdges[y*w+x])
							continue;

						vector3float vNorm=pFrame1->m_pImage->m_vNormals[y*w+x];
						vNorm1.Set(vNorm.x,vNorm.y,vNorm.z);
						break;
					} //1					
	
					if (vPos[0].x<BORDER_X || vPos[0].x>=(w-BORDER_X) || vPos[0].y<BORDER_Y || vPos[0].y>=(h-BORDER_Y))
						break;						

					//ftype fBestMatch=0.001;
					//ftype fBestMatch=0.001;
					bool bFound=false;
					ftype fBestError=64.0;
					//ftype fBestError=128.0;
					vector3f vNorm2(0,0,0);
					vector3f vBestPos(0,0,0);
					while (1)
					{
						// find the best match
						if (nIncValues[0]==0)
						{								
							// increase y 
							vPos[1].y+=(ftype)(nIncValues[1]);
							vPos[1].x=-(vEpiLines[1].y*vPos[1].y+vEpiLines[1].z)/vEpiLines[1].x;
						}
						else
						{
							vPos[1].x+=(ftype)(nIncValues[0]);
							vPos[1].y=-(vEpiLines[1].x*vPos[1].x+vEpiLines[1].z)/vEpiLines[1].y;
						}

						if (vPos[1].x<BORDER_X || vPos[1].x>=(w-BORDER_X) || vPos[1].y<BORDER_Y || vPos[1].y>=(h-BORDER_Y))						
							break;					

						if (vPos[1].Distance(vLastPos[1])>fMaxDist)					
							break;

						int x1=(int)(vPos[0].x);
						int y1=(int)(vPos[0].y);

						int x2=(int)(vPos[1].x);
						int y2=(int)(vPos[1].y);

						if (!pFrame2->m_pImage->m_pEdges[y2*w+x2])
							continue;

						vector3float vTest=pFrame2->m_pImage->m_vNormals[y2*w+x2];
						vector3f vNorm(vTest.x,vTest.y,vTest.z);
						ftype fDist=vNorm1.Distance2(vNorm);

						if (fDist>0.01)
							continue;

						ftype fError=0;
						for (int k=0;k<25;k++)
						{
							int x11=x1+dirs55[k][0];
							int y11=y1+dirs55[k][1];

							int x22=x2+dirs55[k][0];
							int y22=y2+dirs55[k][1];
							
							fError+=abs(((int)ptr2[y22*w+x22])-
								(int)(ptr1[y11*w+x11]));

							//vector3float v1=pFrame1->m_pImage->m_vNormals[y11*w+x11];
							//vector3float v2=pFrame2->m_pImage->m_vNormals[y22*w+x22];
							//fError+=v1.Distance2(v2);

							if (fError>fBestError)
								break;
						} //k

						if (fError<fBestError)
						{													
							fBestError=fError;
							vBestPos=vPos[1];
							bFound=true;
						}	

						/*
						vector3float vTest=pFrame2->m_pImage->m_vNormals[y*w+x];
						if (vTest.x==0 && vTest.y==0 && vTest.z==0)
							continue;					
						vector3f vNorm(vTest.x,vTest.y,vTest.z);
						ftype fDist=vNorm1.Distance2(vNorm);
						if (fDist<fBestMatch)
						{						
							vNorm2.Set(vNorm.x,vNorm.y,vNorm.z);
							fBestMatch=fDist;
							vBestPos=vPos[1];
						}	
						*/
					} //1									
						
					if (!bFound)
					{
						vPos[1]=vLastPos[1];					
						continue; // find next edge in view 1
					}

					vPos[1]=vBestPos;
					
					int x11=(int)(vPos[0].x);
					int y11=(int)(vPos[0].y);
					int x22=(int)(vPos[1].x);
					int y22=(int)(vPos[1].y);
										
					for (int k=0;k<49;k++)
					{					
						int x111=x11+dirs77[k][0];
						int y111=y11+dirs77[k][1];
						int x222=x22+dirs77[k][0];
						int y222=y22+dirs77[k][1];
						// found a match, follow the edges
						FollowEdge(x111,y111,x222,y222,pFrame1->m_pImage,pFrame2->m_pImage,
							&edges,k1,k1+1,w,h,nFrames);					
					}
					// set last pos - preserve order 
					vLastPos[1]=vPos[1];

				}// while1
			} //k (both dirs) 		 
		} //it		

		SAFE_DELETE_ARRAY(pFrame1->m_pImage->m_vNormals);
		
		// they have been cleared - must be regenerated 
		// no, so it will match only new edges
		//SAFE_DELETE_ARRAY(pFrame2->m_pImage->m_vNormals);
		
		int nInvalid=0;
		lstPhotoVertices tempVerts2; 
		for (lstPhotoVerticesIt it=edges.begin();it!=edges.end();it++)
		{
			CPVertex *pVert=(*it);

			//find 3d pos
			ftype fError=FindBest3DPos(pVert,pFrame1,pFrame2,pVert->m_vPos);						
			if (fError>1)
			{
				// wrong match?
				delete pVert;
				nInvalid++;
				continue;
			}
			
			vector3f vSource=pVert->m_v2DPos[k1];
			ftype fCol=pFrame1->m_pImage->m_pRawPixels[(int)(vSource.y)*w+(int)(vSource.x)];
			fCol/=255.0;
			pVert->m_vColor.Set(fCol,fCol,fCol);			

			tempVerts2.push_back(pVert);
		} //i
		
		int nFeatures=(int)(tempVerts2.size());

		if (!nFeatures)
		{
			edges.clear();
			continue;
		}

		CvMat* points1  = cvCreateMat(2,nFeatures,CV_MAT64D);
		CvMat* points2  = cvCreateMat(2,nFeatures,CV_MAT64D);		

		for (int k=0;k<nFeatures;k++)		
		{		
			CPVertex *pVert=tempVerts2[k];
			ftype x1=pVert->m_v2DPos[k1].x;
			ftype y1=pVert->m_v2DPos[k1].y;

			ftype x2=pVert->m_v2DPos[k1+1].x;
			ftype y2=pVert->m_v2DPos[k1+1].y;

			cvmSet(points1,0,k,x1);
			cvmSet(points1,1,k,y1);

			cvmSet(points2,0,k,x2);
			cvmSet(points2,1,k,y2);			
		}	

		CvMat* wpoints1 = cvCreateMat(3,nFeatures,CV_MAT64D);
		CvMat* wpoints2 = cvCreateMat(3,nFeatures,CV_MAT64D);
		CvMat* corrLines1 = cvCreateMat(3,nFeatures,CV_MAT64D);
		CvMat* corrLines2 = cvCreateMat(3,nFeatures,CV_MAT64D);

		cvMake3DPoints(points1,wpoints1);
		cvMake3DPoints(points2,wpoints2);

		cvReleaseMat(&points1);
		cvReleaseMat(&points2);

		cvComputeCorrespondEpilines(wpoints1,1,m_lstFrames[k1]->m_FundMatrix,corrLines2);
		cvComputeCorrespondEpilines(wpoints2,2,m_lstFrames[k1]->m_FundMatrix,corrLines1);		

		CvMat pnt1,pnt2;
		CvMat lin1,lin2;

		for (int i = 0; i < nFeatures; i++ )
		{
			cvGetCol(wpoints1,&pnt1,i);
			cvGetCol(corrLines1,&lin1,i);
			cvGetCol(wpoints2,&pnt2,i);
			cvGetCol(corrLines2,&lin2,i);

			double dist1,dist2;
			dist1 = fabs(cvDotProduct(&pnt1,&lin1));
			dist2 = fabs(cvDotProduct(&pnt2,&lin2));

			CPVertex *pVert=tempVerts2[i];

			if (dist1<1.0 && dist2<1.0)
			{				
				double x1=cvmGet(&pnt1,0,0);
				double x2=cvmGet(&pnt2,0,0);

				double y1=cvmGet(&pnt1,1,0);
				double y2=cvmGet(&pnt2,1,0);

				// Each epipolar line is present by coefficients a,b,c of line equation: 
				// a*x + b*y + c = 0				
				double a=cvmGet(&lin1,0,0);
				double b=cvmGet(&lin1,1,0);
				double c=cvmGet(&lin1,2,0);				
				pVert->m_vEpiLines[0][k1].Set(a,b,c);				

				a=cvmGet(&lin2,0,0);
				b=cvmGet(&lin2,1,0);
				c=cvmGet(&lin2,2,0);
				pVert->m_vEpiLines[1][k1].Set(a,b,c);

				alledges.push_back(pVert);		
			}
			else
			{
				delete pVert;
				nInvalid++;
			}			
		} //i
			
		cvReleaseMat(&wpoints1);
		cvReleaseMat(&wpoints2);
		cvReleaseMat(&corrLines1);
		cvReleaseMat(&corrLines2);	

		MyOutputDebugString("Found %d edges matches between %d and %d (%d wrong matches), total=%d \n",edges.size(),k1,k1+1,nInvalid,alledges.size());		

		edges.clear();		
	} //k1
	SAFE_DELETE_ARRAY(m_lstFrames[k1]->m_pImage->m_vNormals);

	for (lstPhotoVerticesIt it=alledges.begin();it!=alledges.end();it++)
	{
		CPVertex *pVert=(*it);
		m_lstVertices.push_back(pVert);
	}
}

			/*
			if ((!(pVert->m_vColor.x==0 && pVert->m_vColor.y==1)))
			{			
				uchar r=(GetRandomNumber(x1*y1*17731))&255;
				uchar g=((int)(r)*18923)&255;
				uchar b=((int)(r+g)*7249)&255;

				pTest[k1]->DrawPoint(pVert->m_v2DPos[pFrame1->m_nFrameNum],3,r,g,b);

				m_pVideo->ProjectToScreen(pVert->m_vPos,vPos,&pFrame2->m_Camera);
				pVert->m_v2DPos[pFrame2->m_nFrameNum].z=vPos.z;
				if (pFrame2->m_pBestDepth)
					pFrame2->m_pBestDepth[y2*w+x2]=vPos.z;

				if (pTest[k1+1])
					pTest[k1+1]->DrawPoint(pVert->m_v2DPos[pFrame2->m_nFrameNum],3,r,g,b);

				//pTest[k1]->DrawLine(pVert->m_v2DPos[k1],pVert->m_v2DPos[k1+1],vector3f(255,255,255),vector3f(255,255,255));
				*/
/*
	for (int k1=0;k1<nFrames;k1++)	
	{
		if (pTest[k1])
			pTest[k1]->SaveAsTga(NULL,"Corresp");
		CPhotoFrame *pFrame=m_lstFrames[k1];
		uchar *pTemp=new uchar [w*h*3];
		if (pFrame->m_pBestDepth)
		{	
			char szFilename[512];
			sprintf(szFilename,"%s-BestDepth.tga",pFrame->m_pImage->m_szName);

			//ftype fScale=255.0/(fMaxz-fMinz);
			ftype fScale=8192.0;

			for (int k=0;k<w*h;k++)
			{
				int nRes=(int)((pFrame->m_pBestDepth[k]-m_fMinZ)*fScale);		
				if (nRes<0)
					nRes=0;
				if (nRes>255)
					nRes=255;

				for (int j=0;j<3;j++)
				{
					pTemp[k*3+j]=nRes;
				} //j
			}//k
			SaveTga(pTemp,3,w,h,szFilename,true);
		}
		delete [] pTemp;
	} //k1
*/