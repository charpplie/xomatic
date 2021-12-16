


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

#include "LoadingBar.h"

using namespace Wml;
#include "WmlIntrPln3Pln3.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif 

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10Doc::GetSampleVerts(lstPhotoVertices *vertsTest,int w,int h,int k1,bool bAll,int nLimit)
{

	if (bAll)
	{
		int nVerts=0;
		for (lstPhotoVerticesIt i1=m_lstVertices.begin();i1!=m_lstVertices.end();i1++)
		{	
			CPVertex *pVert=(*i1);
			if (pVert->m_v2DPos[k1].x<0 || pVert->m_v2DPos[k1].y<0)
				continue; // feature not present in this view

			if (pVert->m_vPos.x==0 && pVert->m_vPos.y==0)
				continue; // point not valid				
							
			vertsTest->push_back(pVert);
			nVerts++;
			if (nVerts>=nLimit)
				break;
		} //i1
		return;
	}

	vector3f vGoodPos[8];
	vGoodPos[0].Set(0,0,1.0);
	vGoodPos[1].Set(w,0,1.0);
	vGoodPos[2].Set(w,h,1.0);
	vGoodPos[3].Set(0,h,1.0);
	vGoodPos[4].Set(w/2,h/2,1.0);
	// choose 8 vertices - 5 from above and other 3 random
	for (int k=0;k<8;k++)
	{		
		CPVertex *pBest=NULL;
		ftype fBestDist=0;
		for (lstPhotoVerticesIt i1=m_lstVertices.begin();i1!=m_lstVertices.end();i1++)
		{	
			CPVertex *pVert=(*i1);
			if (pVert->m_v2DPos[k1].x<0 || pVert->m_v2DPos[k1].y<0)
				continue; // feature not present in this view

			if (pVert->m_vPos.x==0 && pVert->m_vPos.y==0)
				continue; // point not valid				
							
			if (k>4)
			{
				vertsTest->push_back(pVert);
				break;
			}
			else
			{
				ftype fDist=pVert->m_v2DPos[k1].Distance2(vGoodPos[k]);
				if (!pBest || (fDist<fBestDist))
				{
					pBest=pVert;
					fBestDist=fDist;
				}
			}
		} //i1
		if (k<=4)
		{
			if (pBest)
				vertsTest->push_back(pBest);
		}
	} //k
}

//////////////////////////////////////////////////////////////////////////
ftype	CPhotoBump10Doc::CalcBestRot(CCamera *pCam,vector3f &vBestRot,const vector3f &vPos,ftype fRange,ftype fStep,const vector3f &vRotStart,ftype fBestError,int nFrame,lstPhotoVertices *vertsTest)
{
	CMatrixf matrix1,matrix2,matrix3;

	for (ftype xrot=vRotStart.x-fRange;xrot<vRotStart.x+fRange;xrot+=fStep)
	{	
		matrix1.Identity();		
		matrix1.RotateMatrixf(xrot,1,0,0);	//pitch	
		for (ftype zrot=vRotStart.z-fRange;zrot<vRotStart.z+fRange;zrot+=fStep)
		{			
			memcpy(&matrix3,&matrix1,sizeof(CMatrixf));
			matrix3.RotateMatrixf(zrot,0,1,0);	//roll
			for (ftype yrot=vRotStart.y-fRange;yrot<vRotStart.y+fRange;yrot+=fStep)
			{						
				ftype fErrorSum=0;
										
				memcpy(&matrix2,&matrix3,sizeof(CMatrixf));
				matrix2.RotateMatrixf(yrot,0,0,1);	//yaw
				
				//pCam->SetAngles(vector3f(xrot,yrot,zrot));
				//pCam2->m_Matrix=matrix2;
				pCam->m_OrigMatrix=matrix2;
				//combine the two matrices		
				//pCam2->m_Matrix.Identity();
				//pCam2->m_Matrix.MultMatrixf(pCam2->m_dGLProjMatrix);					
				// view matrix doesn't need projection - so final
				// camer matirx is worng, but is not used by project
				//pCam2->m_Matrix.MultMatrixf(&matrix2.m_Tvalues[0][0]);
				//pCam2->m_Matrix.TranslateMatrix(vector3f(-xpos,-ypos,-zpos));
				//matrix2.TranslateMatrix(vector3f(-xpos,-ypos,-zpos));
				matrix2.TranslateMatrix(-vPos);
				memcpy(pCam->m_dGLViewMatrix,&matrix2.m_Tvalues[0][0],sizeof(CMatrixf));
				pCam->m_bProjectSet=false;

				//m_pVideo->SetCamera(pCam2,false,false);

				for (lstPhotoVerticesIt it=vertsTest->begin();it!=vertsTest->end();it++)
				{
					CPVertex *pVert=(*it);

					vector3f vProj;
					m_pVideo->ProjectToScreen(pVert->m_vPos,vProj,pCam);
					vProj.z=1.0;						
					fErrorSum+=pVert->m_v2DPos[nFrame].Distance2(vProj);
					if (fErrorSum>fBestError)
						break;
				} //it
				
				if (fErrorSum<fBestError)
				{
					fBestError=fErrorSum;
					vBestRot.Set(xrot,yrot,zrot);
				}
			} // xrot
		} //yrot
	} //zrot	
	return (fBestError);
}

//#define DEBUG_EPIPOLAR

//////////////////////////////////////////////////////////////////////////
bool CPhotoBump10Doc::SelfCalibration(CPaintDC *dc)
{
	
	if (LoadCalibData(dc))
		return (true);
	
	int w=m_lstFrames[KEY_FRAME]->m_pImage->m_nWidth;
	int h=m_lstFrames[KEY_FRAME]->m_pImage->m_nHeight;
	CPhotoFrame *pKeyFrame=m_lstFrames[KEY_FRAME];
	int nFrames=(int)(m_lstFrames.size());	

	//////////////////////////////////////////////////////////////////////////
	// find features	
	for (int k1=0;k1<nFrames;k1++)  
	{		
		MyOutputDebugString("Finding features for %d \n",k1);

		CLoadingBar tLoadingBar(m_pVideo,dc,"Step 1 of 4: Detecting features");
	
		//if (k1<nFrames-1)
		if (k1==KEY_FRAME)
			m_lstFrames[k1]->m_pImage->SegmentImage(false);
			//m_lstFrames[k1]->m_pImage->SegmentImage(true);
		m_lstFrames[k1]->m_pImage->GrayScale();

		if (!DetectFeatures(m_lstFrames[k1],nFrames))
			return (false);

		tLoadingBar.Tick(100.0/nFrames);
	} //k1	
		
	for (int k1=0;k1<nFrames-1;k1++)
	{	
		lstPhotoVertices tempVerts;

		char szBuf[256];
		sprintf(szBuf,"Step 2 of 4: Calibrating camera %d - ",k1);
		CLoadingBar tLoadingBar(m_pVideo,dc,szBuf);

		MyOutputDebugString("Finding matches between %d and %d...",k1,k1+1);

		MatchFeatures(m_lstFrames[k1],m_lstFrames[k1+1],&tempVerts,nFrames);

		tLoadingBar.Tick(100.0/8.0);

		MyOutputDebugString("Found %d matches \n",(int)(tempVerts.size()));

		if (tempVerts.size()<8)
		{
			char szBuffer[1024];
			sprintf(szBuffer,"Can't estabilish relationship between frames %d(%s) and (%d)%s",k1,m_lstFrames[k1]->m_pImage->GetName(),k1+1,m_lstFrames[k1+1]->m_pImage->GetName());
			if (!theApp.m_bStandalone)
				AfxMessageBox(szBuffer);
			return (false);
		}		

		//////////////////////////////////////////////////////////////////////////		
		// calc fundamental matrix
		MyOutputDebugString("Calibrating shots %d and %d - step 2\n",k1,k1+1);
	
		int nFeatures=(int)(tempVerts.size());
		int nVerts=nFeatures;
		// put a limit on fund matrix otherwise takes too long
		// to compute
		if (nVerts>3000)
			nVerts=3000; 
		CvMat* points1  = cvCreateMat(2,nVerts,CV_MAT64D);
		CvMat* points2  = cvCreateMat(2,nVerts,CV_MAT64D);		

		m_lstFrames[k1]->m_FundMatrix=cvCreateMat(3,3,CV_MAT64D);				
		for (int k=0;k<nVerts;k++)		
		{		
			CPVertex *pVert=tempVerts[k];
			ftype x1=pVert->m_v2DPos[k1].x;
			ftype y1=pVert->m_v2DPos[k1].y;

			ftype x2=pVert->m_v2DPos[k1+1].x;
			ftype y2=pVert->m_v2DPos[k1+1].y;

			cvmSet(points1,0,k,x1);
			cvmSet(points1,1,k,y1);

			cvmSet(points2,0,k,x2);
			cvmSet(points2,1,k,y2);			
		}	

		int numMatrs = cvFindFundamentalMat(points1,points2,m_lstFrames[k1]->m_FundMatrix,CV_FM_RANSAC,1,0.99,0);	
		CvMat *pMat=m_lstFrames[k1]->m_FundMatrix;

		if (nFeatures>nVerts)
		{		
			cvReleaseMat(&points1);
			cvReleaseMat(&points2);
			points1  = cvCreateMat(2,nFeatures,CV_MAT64D);
			points2  = cvCreateMat(2,nFeatures,CV_MAT64D);		
			for (int k=0;k<nFeatures;k++)		
			{		
				CPVertex *pVert=tempVerts[k];

				cvmSet(points1,0,k,pVert->m_v2DPos[k1].x);
				cvmSet(points1,1,k,pVert->m_v2DPos[k1].y);

				cvmSet(points2,0,k,pVert->m_v2DPos[k1+1].x);
				cvmSet(points2,1,k,pVert->m_v2DPos[k1+1].y);			
			}	//k
		}	
	
		if (numMatrs==0)
			return (false);	

		// remove points that are not on the epipolar lines
		MyOutputDebugString("Calibrating shots %d and %d - removing false matches \n",k1,k1+1);

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

		if (m_bStereoMode)
		{
			// 1) prendere tutte le linee, calcolare la media e usare solo quella

			//ftype aSum1=0,bSum1=0,cSum1=0;
			//ftype aSum2=0,bSum2=0,cSum2=0;
			ftype fSlopeSumA1=0,fSlopeSumA2=0;
			ftype fSlopeSumB1=0,fSlopeSumB2=0;			
			ftype	a,b;
			int		nDiv=0;
			for (int i = 0; i < nFeatures; i++ )
			{	
				CPVertex *pVert=tempVerts[i];
				// a*x + b*y + c = 0			
				// They are normalized (a2+b2=1) are stored into correspondent_lines. 
				ftype x1=pVert->m_v2DPos[k1].x;
				ftype y1=pVert->m_v2DPos[k1].y;
				ftype x2=pVert->m_v2DPos[k1+1].x;
				ftype y2=pVert->m_v2DPos[k1+1].y;

				b= x1-x2;
				a= -(y1-y2);					
				ftype fLen=sqrt(a*a+b*b);
				if (fLen<1)
					continue;
				a/=fLen;b/=fLen;														
				fSlopeSumA1+=a;
				fSlopeSumB1+=b;

				// swap
				b = x2 - x1;
				a = -(y2 - y1);									
				fLen=sqrt(a*a+b*b);
				if (fLen<1)
					continue;
				a/=fLen;b/=fLen;		
				fSlopeSumA2+=a;
				fSlopeSumB2+=b;

				nDiv++;
			}
			if (!nDiv)
				return (false);

			fSlopeSumA1/=(ftype)(nDiv);
			fSlopeSumB1/=(ftype)(nDiv);
			fSlopeSumA2/=(ftype)(nDiv);
			fSlopeSumB2/=(ftype)(nDiv);
			
			m_lstFrames[k1]->m_fSlopeA=fSlopeSumA1;
			m_lstFrames[k1]->m_fSlopeB=fSlopeSumB1;
			m_lstFrames[k1+1]->m_fSlopeA=fSlopeSumA2;
			m_lstFrames[k1+1]->m_fSlopeB=fSlopeSumB2;			

			// 3) in densematching2, sostituire cvComputeCorrespondEpilines con la linea

			// 4) aggiungere photo di normal maps super sardegna
		}

		double threshold=1.0;
		if (m_bStereoMode)
			threshold*=0.05;
		lstPhotoVertices tempVerts2;
		for (int i = 0; i < nFeatures; i++ )
		{
			double dist1,dist2;

			CPVertex *pVert=tempVerts[i];

			cvGetCol(wpoints1,&pnt1,i);
			cvGetCol(corrLines1,&lin1,i);
			cvGetCol(wpoints2,&pnt2,i);
			cvGetCol(corrLines2,&lin2,i);

			if (m_bStereoMode)
			{
				ftype x1=pVert->m_v2DPos[k1].x;
				ftype y1=pVert->m_v2DPos[k1].y;				
				ftype x2=pVert->m_v2DPos[k1+1].x;
				ftype y2=pVert->m_v2DPos[k1+1].y;
				ftype a,b;

				b= x1-x2;
				if (Sign(b)!=Sign(m_lstFrames[k1]->m_fSlopeB))
				{
					delete pVert;
					continue;
				}
				a= -(y1-y2);					
				if (Sign(a)!=Sign(m_lstFrames[k1]->m_fSlopeA))
				{
					delete pVert;
					continue;
				}
				ftype fLen=sqrt(a*a+b*b);
				if (fLen<1)
				{
					delete pVert;
					continue;
				}
				a/=fLen;b/=fLen;
				dist1=1.0f-(a*m_lstFrames[k1]->m_fSlopeA+b*m_lstFrames[k1]->m_fSlopeB);

				// swap
				b = x2 - x1;
				if (Sign(b)!=Sign(m_lstFrames[k1+1]->m_fSlopeB))
				{
					delete pVert;
					continue;
				}
				a = -(y2 - y1);									
				if (Sign(a)!=Sign(m_lstFrames[k1+1]->m_fSlopeA))
				{
					delete pVert;
					continue;
				}
				fLen=sqrt(a*a+b*b);
				if (fLen<1)
				{
					delete pVert;
					continue;
				}
				a/=fLen;b/=fLen;		
				dist2=1.0f-(a*m_lstFrames[k1+1]->m_fSlopeA+b*m_lstFrames[k1+1]->m_fSlopeB);

				//dist1=x1*fA1+y1*fB1+fC1;
				//dist2=x2*fA2+y2*fB2+fC2;
			}
			else
			{			
				dist1 = fabs(cvDotProduct(&pnt1,&lin1));
				dist2 = fabs(cvDotProduct(&pnt2,&lin2));
			}

			if (dist1<threshold && dist2<threshold)
			{				
				double x1=cvmGet(&pnt1,0,0);
				double x2=cvmGet(&pnt2,0,0);

				double y1=cvmGet(&pnt1,1,0);
				double y2=cvmGet(&pnt2,1,0);
							
				/*
				if (CalcNormalizedCrossCorrelation((int)(x1),(int)(y1),(int)(x2),(int)(y2),
					m_lstFrames[k1]->m_pImage->m_pRawPixels,
					m_lstFrames[k1+1]->m_pImage->m_pRawPixels,
					w,h,7)<0.7)
				{
					delete pVert;
					continue;
				}
				*/

				// Each epipolar line is present by coefficients a,b,c of line equation: 
				// a*x + b*y + c = 0				
				// They are normalized (a2+b2=1) are stored into correspondent_lines. 
				double a=cvmGet(&lin1,0,0);
				double b=cvmGet(&lin1,1,0);
				double c=cvmGet(&lin1,2,0);
				if (m_bStereoMode)
				{
					b= x1-x2;
					a= -(y1-y2);					
					ftype fLen=sqrt(a*a+b*b);
					a/=fLen;b/=fLen;										
					c = -(a*x1 + b*y1);					
				}				
				//pVert->m_vEpiLines[k1].Set(a,b,c);				
				pVert->m_vEpiLines[0][k1].Set(a,b,c);				

				//////////////////////////////////////////////////////////////////////////				
#ifdef DEBUG_EPIPOLAR
				//_asm { int 3 }				
				double tx1=0;double ty1=-(a*tx1+c)/b;
				double tx2=w-1;double ty2=-(a*tx2+c)/b;
				if ((i&31)==1)
				{
					m_lstFrames[KEY_FRAME]->m_pImage->DrawLine(vector3f(tx1,ty1,0),vector3f(tx2,ty2,0),vector3f(255,255,255),vector3f(1,1,1));
					m_lstFrames[KEY_FRAME]->m_pImage->DrawPoint(pVert->m_v2DPos[KEY_FRAME],6,255,255,255);
					m_lstFrames[KEY_FRAME]->m_pImage->DrawPoint(pVert->m_v2DPos[KEY_FRAME+1],6,255,0,255);
				}				
#endif
				//////////////////////////////////////////////////////////////////////////				

				a=cvmGet(&lin2,0,0);
				b=cvmGet(&lin2,1,0);
				c=cvmGet(&lin2,2,0);
				if (m_bStereoMode)
				{
					// swap
					b = x2 - x1;
					a = -(y2 - y1);									
					ftype fLen=sqrt(a*a+b*b);
					a/=fLen;b/=fLen;		
					c = -(a*x2 + b*y2);										
				}				
				//pVert->m_vEpiLines[k1+1].Set(a,b,c);
				pVert->m_vEpiLines[1][k1].Set(a,b,c);

				//////////////////////////////////////////////////////////////////////////				
				//_asm { int 3 }
#ifdef DEBUG_EPIPOLAR
				tx1=0;ty1=-(a*tx1+c)/b;
				tx2=w-1;ty2=-(a*tx2+c)/b;				
				if ((i&31)==1)
				{
					m_lstFrames[k1+1]->m_pImage->DrawLine(vector3f(tx1,ty1,0),vector3f(tx2,ty2,0),vector3f(255,255,255),vector3f(1,1,1));								
					m_lstFrames[k1+1]->m_pImage->DrawPoint(pVert->m_v2DPos[k1+1],6,255,255,255);	
					m_lstFrames[k1+1]->m_pImage->DrawPoint(pVert->m_v2DPos[KEY_FRAME],6,255,0,255);
				}
#endif
				//////////////////////////////////////////////////////////////////////////
				

				tempVerts2.push_back(pVert);				
			}
			else
			{	
				//////////////////////////////////////////////////////////////////////////
#ifdef DEBUG_EPIPOLAR
				m_lstFrames[KEY_FRAME]->m_pImage->DrawPoint(pVert->m_v2DPos[KEY_FRAME],2,0,0,0);
				m_lstFrames[k1+1]->m_pImage->DrawPoint(pVert->m_v2DPos[k1+1],2,0,0,0);
#endif
				//////////////////////////////////////////////////////////////////////////

				delete pVert;
			}
		} // nfeatures
		
		cvReleaseMat(&wpoints1);
		cvReleaseMat(&wpoints2);
		cvReleaseMat(&corrLines1);
		cvReleaseMat(&corrLines2);	

		tempVerts.clear();
		nFeatures=(int)(tempVerts2.size());
				
		//////////////////////////////////////////////////////////////////////////		
		// add common features for this pair of images in the global 
		// feature list

		for (lstPhotoVerticesIt i1=tempVerts2.begin();i1!=tempVerts2.end();i1++)
		{		
			CPVertex *p1=(*i1);
			vector3f v1=p1->m_v2DPos[k1];
			v1.IntValues();
			// check if it was already in
			CPVertex *pVert=NULL;
			for (lstPhotoVerticesIt i2=m_lstVertices.begin();i2!=m_lstVertices.end();i2++)
			{
				CPVertex *p2=(*i2);				
				vector3f v2=p2->m_v2DPos[k1];
				v2.IntValues();
				if (v1.Distance(v2)<1)
				{
					pVert=p2;
					break;
				}
			} //i2

			if (!pVert)
				m_lstVertices.push_back(p1);
			else
			{
				pVert->m_v2DPos[k1+1]=p1->m_v2DPos[k1+1];
				//pVert->m_vEpiLines[k1+1]=p1->m_vEpiLines[k1+1];
				pVert->m_vEpiLines[0][k1]=p1->m_vEpiLines[0][k1];
				pVert->m_vEpiLines[1][k1]=p1->m_vEpiLines[1][k1];
				delete p1;
			}
		} //i1		
	}//k1
	
	//////////////////////////////////////////////////////////////////////////	
#ifdef DEBUG_EPIPOLAR
	m_lstFrames[KEY_FRAME]->m_pImage->SaveAsTga("Epipolar0.tga",NULL);
	m_lstFrames[1]->m_pImage->SaveAsTga("Epipolar1.tga",NULL);
#endif
	//////////////////////////////////////////////////////////////////////////	

	MyOutputDebugString("Total points found: %d \n",m_lstVertices.size());

	if (m_lstVertices.size()<8)
	{
		char szBuffer[1024];
		sprintf(szBuffer,"Couldn't find enough total features for key frame %s\n, total points found: %d",m_lstFrames[KEY_FRAME]->m_pImage->GetName(),m_lstVertices.size());
		if (!theApp.m_bStandalone)
			AfxMessageBox(szBuffer);
		return (false);
	}		

	if (!m_bLoadedFromBojou)
	{	
		//////////////////////////////////////////////////////////////////////////
		// find cameras rotation and translation

		// set first camera
		vector3f vOrigin(1.0,1.70,9.0);
		CCamera *pKeyCam=&m_lstFrames[0]->m_Camera;
		pKeyCam->m_nViewport[0]=0;
		pKeyCam->m_nViewport[1]=0;
		pKeyCam->m_nViewport[2]=w;
		pKeyCam->m_nViewport[3]=h;		
		pKeyCam->SetPos(vector3f(0,0,0)+vOrigin);
		pKeyCam->SetAngles(vector3f(0,0,0));
		pKeyCam->m_Matrix.Identity();
		pKeyCam->m_OrigMatrix.Identity();
		m_pVideo->SetCamera(pKeyCam,false,false);

		for (int k1=1;k1<nFrames;k1++)
		{
			char szBuf[512];
			sprintf(szBuf,"Step 2 of 4: Calibrating camera %d - ",k1);
			CLoadingBar tLoadingBar(m_pVideo,dc,szBuf);
			tLoadingBar.Set(100/8.0);

			MyOutputDebugString("Calculating parameters for camera %d\n",k1);

			Line3d rkLine;
			vector3f vPlanes[2][3];
			CPVertex *pVertMinMax[2]={NULL,NULL};
			CPVertex *pMiddle=NULL;

			CPhotoFrame *pFrame=m_lstFrames[k1];
			CCamera			*pCam=&pFrame->m_Camera;

			int k2=k1-1;
			CPhotoFrame *pPrevFrame=m_lstFrames[k2];
			CCamera			*pPrevCam=&pPrevFrame->m_Camera;
			
			// find middle, mix and max verts in previous view
			for (lstPhotoVerticesIt i1=m_lstVertices.begin();i1!=m_lstVertices.end();i1++)
			{		
				CPVertex *pVert=(*i1);

				int x1=pVert->GetX(k2);
				int y1=pVert->GetY(k2);

				int x2=pVert->GetX(k1);
				int y2=pVert->GetY(k1);

				if (x1<0 || y1<0 || x2<0 || y2<0)
					continue; // feature not present in this view pair

				if (!pVertMinMax[0] || pVert->m_v2DPos[k2].y<pVertMinMax[0]->m_v2DPos[k2].y)
					pVertMinMax[0]=pVert;

				if (!pVertMinMax[1] || pVert->m_v2DPos[k2].y>pVertMinMax[1]->m_v2DPos[k2].y)
					pVertMinMax[1]=pVert;

				if (!pMiddle)
				{
					pMiddle=pVert;
				}
				else
				{		
					vector3f vCenter(w/2,h/2,1.0);
					vector3f v1=pMiddle->m_v2DPos[k2];
					vector3f v2=pVert->m_v2DPos[k2];
					if (v2.Distance2(vCenter)<v1.Distance2(vCenter))
						pMiddle=pVert;
				}
			} //i1

			// find 2d planes projection
			vector3f vExtremePoints[4];
			Plane3d Planes[2];
			for (int k=0;k<2;k++)
			{				
				//vector3f vLine=pVertMinMax[k]->m_vEpiLines[k2];

				// center
				vPlanes[k][0]=pPrevCam->GetPos();

				// previous baseline in previous view
				vector3f vLine=pVertMinMax[k]->m_vEpiLines[0][k2];

				// left
				double tx1=10;double ty1=-(vLine.x*tx1+vLine.z)/vLine.y;
				if (ty1<10)
				{
					// a*x + b*y + c = 0
					ty1=10;tx1=-(vLine.y*ty1+vLine.z)/vLine.x;
				}
				else
				if (ty1>(h-10))
				{			
					ty1=h-10;tx1=-(vLine.y*ty1+vLine.z)/vLine.x;
				}
				vector3f vSource;
				vSource.Set(tx1,ty1,0.99);
				//vSource.PrintDebug();		
				m_pVideo->UnProjectFromScreen(vSource,vPlanes[k][1],pPrevCam);
				vExtremePoints[k*2+0]=vector3f(vPlanes[k][1]);
				//vPlanes[k][1].PrintDebug();

				// right
				tx1=w-10;ty1=-(vLine.x*tx1+vLine.z)/vLine.y;
				if (ty1<10)
				{
					// a*x + b*y + c = 0
					ty1=10;tx1=-(vLine.y*ty1+vLine.z)/vLine.x;
				}
				else
				if (ty1>(h-10))
				{			
					ty1=h-10;tx1=-(vLine.y*ty1+vLine.z)/vLine.x;
				}				
				vSource.Set(tx1,ty1,0.99);

				//vSource.PrintDebug();		
				m_pVideo->UnProjectFromScreen(vSource,vPlanes[k][2],pPrevCam);
				vExtremePoints[k*2+1]=vector3f(vPlanes[k][2]);
				//vPlanes[k][2].PrintDebug();

				Planes[k].CalcPlaneUnnormalized(
					Vector3d(vPlanes[k][0].x,vPlanes[k][0].y,vPlanes[k][0].z),
					Vector3d(vPlanes[k][1].x,vPlanes[k][1].y,vPlanes[k][1].z),
					Vector3d(vPlanes[k][2].x,vPlanes[k][2].y,vPlanes[k][2].z));
			} //k

			ftype fGlobalScale=1.0;
			ftype fCamDist=2.0*fGlobalScale;					
			vector3f v1,v2,v3;

			//if (!m_bUseSinglePhoto)
			{			
				// find baseline for previous view
				bool bRes=FindIntersection(Planes[0],Planes[1],rkLine);	
				ASSERT(bRes);

				v1.Set(rkLine.Origin().X(),rkLine.Origin().Y(),rkLine.Origin().Z());
				v2=pPrevFrame->m_Camera.GetPos();
				v3.Set(rkLine.Direction().X(),rkLine.Direction().Y(),rkLine.Direction().Z());		
			}
			/*
			else
			{
				v1=(vPlanes[0][1]+vPlanes[1][1])*0.5;
				v2=pPrevFrame->m_Camera.GetPos();
				v3=(vPlanes[0][2]+vPlanes[1][2])*0.5;				
			}
			*/

			// correct baseline
			vector3f vDir=v3-v2;
			vDir.Normalize();
			pCam->SetPos(v2-(vDir*fCamDist));		

			// central ray dir...
			vector3f vRay1[2];		
			vRay1[0]=v2; // cam 1 center
			m_pVideo->UnProjectFromScreen(pMiddle->m_v2DPos[k2],vRay1[1],&m_lstFrames[k2]->m_Camera);
			
			vector3f vBestRot;		
			ftype fBestError=99999;

			vector3f v2DReference[4];
			double tx1,ty1;

			// this is the correct baseline projection in the k1 (next) view
			// taken from prev k2 view corresp. line - use it as reference
			//vector3f vLine=pVertMinMax[0]->m_vEpiLines[k1];
			vector3f vLine=pVertMinMax[0]->m_vEpiLines[1][k2];
			tx1=0;ty1=-(vLine.x*tx1+vLine.z)/vLine.y;
			v2DReference[0].Set(tx1,ty1,1.0);
			tx1=w;ty1=-(vLine.x*tx1+vLine.z)/vLine.y;
			v2DReference[1].Set(tx1,ty1,1.0);

			//vLine=pVertMinMax[1]->m_vEpiLines[k1];
			vLine=pVertMinMax[1]->m_vEpiLines[1][k2];
			tx1=0;ty1=-(vLine.x*tx1+vLine.z)/vLine.y;
			v2DReference[2].Set(tx1,ty1,1.0);
			tx1=w;ty1=-(vLine.x*tx1+vLine.z)/vLine.y;
			v2DReference[3].Set(tx1,ty1,1.0);		

			vector3f	vRotStart=pPrevCam->GetAngles();		
			ftype		 	fRange=45;
			ftype			fStep=5.0;

			while (fBestError>1 && fRange>0.01)
			{		
				for (ftype zrot=vRotStart.z-fRange;zrot<vRotStart.z+fRange;zrot+=fStep)
				{
					for (ftype yrot=vRotStart.y-fRange;yrot<vRotStart.y+fRange;yrot+=fStep)
					{
						for (ftype xrot=vRotStart.x-fRange;xrot<vRotStart.x+fRange;xrot+=fStep)
						{					
							CMatrixf matrix2;
							matrix2.Identity();		
							matrix2.RotateMatrixf(xrot,1,0,0);	//pitch	
							matrix2.RotateMatrixf(zrot,0,1,0);	//roll
							matrix2.RotateMatrixf(yrot,0,0,1);	//yaw
							
							pCam->SetAngles(vector3f(xrot,yrot,zrot));
							pCam->m_Matrix=matrix2;
							pCam->m_OrigMatrix=matrix2;

							m_pVideo->SetCamera(pCam,false,false);
						
							ftype fError=0;
							for (int k=0;k<2;k++)
							{					
								// project plane points for this camera config
								vector3f vp1,vp2,vp3;
								m_pVideo->ProjectToScreen(vExtremePoints[k*2+0],vp1,pCam,false);
								m_pVideo->ProjectToScreen(vExtremePoints[k*2+1],vp2,pCam,false);
								
								// find line equation, calc points and calc error
								ftype m=((vp2.y-vp1.y)/(vp2.x-vp1.x));
								ftype q=(-m*vp1.x)+vp1.y;
							
								tx1=0;ty1=m*tx1+q;
								fError+=v2DReference[k*2+0].Distance2(vector3f(tx1,ty1,1.0));
								tx1=w;ty1=m*tx1+q;
								fError+=v2DReference[k*2+1].Distance2(vector3f(tx1,ty1,1.0));
							} //k

							if (fError<fBestError)
							{
								fBestError=fError;
								vBestRot.Set(xrot,yrot,zrot);
							}

						}//x
					} //y
				} //z
				
				vRotStart=vBestRot;			
				fRange/=2.0;
				fStep/=2.0;
			} // fError

			MyOutputDebugString("Rotation error for camera %d: %f\n",k1,sqrt(fBestError));

			tLoadingBar.Tick(100.0/8.0);

			CMatrixf matrix2;
			matrix2.Identity();		
			matrix2.RotateMatrixf(vBestRot.x,1,0,0);	//pitch	
			matrix2.RotateMatrixf(vBestRot.z,0,1,0);	//roll
			matrix2.RotateMatrixf(vBestRot.y,0,0,1);	//yaw		
			pCam->SetAngles(vBestRot);
			pCam->m_Matrix=matrix2;
			pCam->m_OrigMatrix=matrix2;
		
			// find which camera is the correct one, the one on
			// the left or on the right
			if (k1==1)
			{		
				vector3f vBestPos=pCam->GetPos();		
				bool bPosFound=false;
				for (int k=-1;k<2;k+=2)
				{		
					pCam->SetPos(v2+((ftype)(k)*(vDir*fCamDist)));		
					m_pVideo->SetCamera(pCam,false,false);

					// project vertex closest to the center where radial 
					// distortion is lowest, calc error from epipolar line
					// reprojecting in the other view		
					vector3f vRay2[2];		
					vRay2[0]=pCam->GetPos(); // cam 2 center
					m_pVideo->UnProjectFromScreen(pMiddle->m_v2DPos[k1],vRay2[1],pCam,false);

					vector3f vRes1,vRes2;
					//if (IntersectLines(vRay1[0],vRay1[1],vRay2[0],vRay2[1],vRes1,vRes2,false)==-1)
					if (IntersectLines(vRay1[0],vRay1[1],vRay2[0],vRay2[1],vRes1,vRes2,true)==0)
						continue;

					CPlanef tPlane1,tPlane2;
					vector3f v1,v2,v3;
					v1=pPrevCam->GetPos();
					v2=pPrevCam->GetPos()+(pPrevCam->GetRightVector()*5.0);
					v3=pPrevCam->GetPos()+(pPrevCam->GetUpVector()*5.0);
					tPlane1.CalcPlane(v1,v2,v3);

					v1=pCam->GetPos();
					v2=pCam->GetPos()+(pCam->GetRightVector()*5.0);
					v3=pCam->GetPos()+(pCam->GetUpVector()*5.0);
					tPlane2.CalcPlane(v1,v2,v3);

					ftype fDist1=tPlane1.DistFromPlane(vRes1);
					ftype fDist2=tPlane1.DistFromPlane(vRes2);

					if (fDist1*fDist2<0)
						continue;

					fDist1=tPlane2.DistFromPlane(vRes1);
					fDist2=tPlane2.DistFromPlane(vRes2);

					if (fDist1*fDist2<0)
						continue;

					vBestPos=pCam->GetPos();

					bPosFound=true;
					break;
				}	//k 		

				ASSERT(bPosFound);
				pCam->SetPos(vBestPos);		
				m_pVideo->SetCamera(pCam,false,false);		
			}
			else		
			{
				// find translation along the baseline and rotation
				// that will best match the already existing 3d points 
				// - for the 2nd camera (frame 1) it is already arbitrarily set

				vector3f	vPosStart=pPrevCam->GetPos();
				
				ftype		 	fRange=fCamDist*1.5;
				ftype			fStep=(0.25*fGlobalScale);

				// it was set on the baseline for finding planes
				vector3f vDir=pCam->GetPos()-vPosStart; 
				vDir.Normalize();
				ftype fBestError=999999;
				vector3f vBestPos(0,0,0);

				vector3f vRotStart=pCam->GetAngles();
				vector3f vBestRot=vRotStart;

				lstPhotoVertices testVerts;
				GetSampleVerts(&testVerts,w,h,k1);

				lstPhotoVertices testVerts2;
				GetSampleVerts(&testVerts2,w,h,k1,true,1000);

				while (fBestError>1 && fRange>0.1)
				{		
					vector3f vPos=vPosStart-(vDir*fRange);
					vector3f vInc=vDir*fStep;
					
					for (ftype fT=-fRange;fT<+fRange;fT+=fStep)
					{										
						pCam->SetPos(vPos);	
						m_pVideo->SetCamera(pCam,false,false);

						ftype fError=0;
						for (lstPhotoVerticesIt i1=testVerts2.begin();i1!=testVerts2.end();i1++)
						{
							CPVertex *pVert=(*i1);

							vector3f vProj;
							m_pVideo->ProjectToScreen(pVert->m_vPos,vProj,pCam);
							vProj.z=1.0;						
							fError+=pVert->m_v2DPos[k1].Distance2(vProj);
							if (fError>fBestError)
								break;
						} //i1
						
						if (fError<fBestError)
						{
							vBestPos=vPos;
							fBestError=fError;
						} 
						
						vPos+=vInc;
					} //fT
					
					CalcBestRot(pCam,vBestRot,vBestPos,fRange*4.0,fRange/8.0,vRotStart,fBestError,k1,&testVerts);

					vPosStart=vBestPos;
					vRotStart=vBestRot;
					fRange/=2.0;
					fStep/=2.0;
				} // fError			

				pCam->SetPos(vBestPos);	
				CMatrixf matrix2;
				matrix2.Identity();		
				matrix2.RotateMatrixf(vBestRot.x,1,0,0);	//pitch	
				matrix2.RotateMatrixf(vBestRot.z,0,1,0);	//roll
				matrix2.RotateMatrixf(vBestRot.y,0,0,1);	//yaw		
				pCam->SetAngles(vBestRot);
				pCam->m_Matrix=matrix2;
				pCam->m_OrigMatrix=matrix2;

				m_pVideo->SetCamera(pCam,false,false);		

				MyOutputDebugString("Translation square error for camera %d: %f\n",k1,fBestError);
			}	//k1!=1
			
			// find position for 3d points 		
			for (lstPhotoVerticesIt i1=m_lstVertices.begin();i1!=m_lstVertices.end();i1++)
			{	
				CPVertex *pVert=(*i1);

				int x1=pVert->GetX(k1);
				int y1=pVert->GetY(k1);

				int x2=pVert->GetX(k1-1);
				int y2=pVert->GetY(k1-1);

				if (x1<0 || y1<0 || x2<0 || y2<0)
				{
					// feature not present in this view pair
					continue; 
				}

				vector3f vRay2[2];		
				// get ray
				vRay2[0]=pCam->GetPos(); // cam 2 center			
				m_pVideo->UnProjectFromScreen(pVert->m_v2DPos[k1],vRay2[1],pCam);
				
				//if (pVert->m_vPos.x==0 && pVert->m_vPos.y==0)	
				if (pVert->m_vPos.IsEquivalent(vector3f(0,0,0),0.000001))
				{			
					// first time, calc intersection and use z coords

					vector3f vRay1[2];		
					vRay1[0]=pPrevCam->GetPos(); // cam 1 center			
					// get ray
					m_pVideo->UnProjectFromScreen(pVert->m_v2DPos[k2],vRay1[1],pPrevCam);

					vector3f vRes1,vRes2;
					//if (IntersectLines(vRay1[0],vRay1[1],vRay2[0],vRay2[1],vRes1,vRes2,false)==-1)
					if (IntersectLines(vRay1[0],vRay1[1],vRay2[0],vRay2[1],vRes1,vRes2,true)==0)
					{				
						pVert->m_v2DPos[k1].Set(-1,-1,-1);
						pVert->m_vPos.Clear();
						continue;
					}
									
					vector3f vPos=(vRes1+vRes2)/2.0;
					vector3f vSource;
					// get screen z of this averaged pos
					m_pVideo->ProjectToScreen(vPos,vSource,pPrevCam);
					// get 3d vert as if it was in the ideal pixel pos
					// using computed z
					vSource.x=pVert->m_v2DPos[k2].x;
					vSource.y=pVert->m_v2DPos[k2].y;
					m_pVideo->UnProjectFromScreen(vSource,pVert->m_vPos,pPrevCam);

					// average again
					pVert->m_vPos=(pVert->m_vPos+vPos)/2.0;
				}
				// new cameras should rather match these points			
				else			
				{
					// find closest distance from existing point to line
					vector3f v1=pVert->m_vPos-vRay2[0];
					vector3f v2=vRay2[1]-vRay2[0];
					v2.Normalize();
					//ftype d=vRay2[0].Distance(vRay2[1]);
					ftype t=v2*v1;
					vector3f vRes1;
					if (t<=0)
						vRes1=vRay2[0];
					else
						vRes1=vRay2[0]+(v2*t);

					// then average with previous point
					pVert->m_vPos=(pVert->m_vPos+vRes1)/2.0;													
				}			

				// remove vertices projected behind the first cam
				// skip far clipping plane
				//if (!m_lstFrames[KEY_FRAME]->m_Camera.IsPointVisible(pVert->m_vPos,MASK_ALL_PLANES))				
				if (!m_lstFrames[KEY_FRAME]->m_Camera.IsPointVisible(pVert->m_vPos,1|4|8|16|32))
				{				
					pVert->m_vPos.Clear();		
					for (int k=0;k<nFrames;k++)				
						pVert->m_v2DPos[k].Set(-1,-1,-1);				
				}					

			} //i1
						
			//MyOutputDebugString("Self calibration for camera %d done \n",k1);
		//} //k1
					

			tLoadingBar.Tick(100.0/8.0);
		//////////////////////////////////////////////////////////////////////////
		// bundle adjustment
		//for (int k1=1;k1<nFrames;k1++)
		//{
			CCamera *pCam2=&m_lstFrames[k1]->m_Camera;

			// just to be sure proj matrix is set at least once
			m_pVideo->SetCamera(pCam2,false,false);  
			
			//ftype fBestError=99999;		
			fBestError=99999;		
			//ftype	fRange=0.5;
			fRange=0.5;
			//ftype fStep=fRange/16.0;
			fStep=fRange/16.0;
			//vector3f vRotStart=pCam2->GetAngles();		
			vRotStart=pCam2->GetAngles();		
			vector3f vPosStart=pCam2->GetPos();
			//vector3f vBestRot;
			vector3f vBestPos;
			
			MyOutputDebugString("Bundle adjustment step 1.0 for camera %d \n",k1);

			lstPhotoVertices vertsTest;
			GetSampleVerts(&vertsTest,w,h,k1);

			if (vertsTest.size()<8)
			{
				char szBuffer[1024];
				sprintf(szBuffer,"Couldn't get enough feature verts for key frame %s\n, total sample verts found: %d",m_lstFrames[KEY_FRAME]->m_pImage->GetName(),vertsTest.size());
				if (!theApp.m_bStandalone)
					AfxMessageBox(szBuffer);
				return (false);
			}		

			for (ftype zpos=vPosStart.z-fRange;zpos<vPosStart.z+fRange;zpos+=fStep)		
			{	
				vector3f vPos(vPosStart.x,vPosStart.y,zpos);
				ftype fError=CalcBestRot(pCam2,vBestRot,vPos,fRange,fStep,vRotStart,fBestError,k1,&vertsTest);
				if (fError<fBestError)
				{
					vBestPos=vPos;
					fBestError=fError;
				}
			}
			tLoadingBar.Tick(100.0/8.0);


			MyOutputDebugString("Bundle adjustment step 1.1 for camera %d \n",k1);

			vPosStart=vBestPos;
			vRotStart=vBestRot;
			pCam2->SetPos(vBestPos);
			pCam2->SetAngles(vBestRot);

			for (ftype ypos=vPosStart.y-fRange;ypos<vPosStart.y+fRange;ypos+=fStep)			
			{
				vector3f vPos(vPosStart.x,ypos,vPosStart.z);
				ftype fError=CalcBestRot(pCam2,vBestRot,vPos,fRange,fStep,vRotStart,fBestError,k1,&vertsTest);
				if (fError<fBestError)
				{
					vBestPos=vPos;
					fBestError=fError;
				}
			}
			tLoadingBar.Tick(100.0/8.0);

			vPosStart=vBestPos;
			vRotStart=vBestRot;
			pCam2->SetPos(vBestPos);
			pCam2->SetAngles(vBestRot);

			MyOutputDebugString("Bundle adjustment step 1.2 for camera %d \n",k1);

			for (ftype xpos=vPosStart.x-fRange;xpos<vPosStart.x+fRange;xpos+=fStep)
			{
				vector3f vPos(xpos,vPosStart.y,vPosStart.z);
				ftype fError=CalcBestRot(pCam2,vBestRot,vPos,fRange,fStep,vRotStart,fBestError,k1,&vertsTest);
				if (fError<fBestError)
				{
					vBestPos=vPos;
					fBestError=fError;
				}			
			} //xpos
			tLoadingBar.Tick(100.0/8.0);

			//CMatrixf matrix2;
			matrix2.Identity();		
			matrix2.RotateMatrixf(vBestRot.x,1,0,0);	//pitch	
			matrix2.RotateMatrixf(vBestRot.z,0,1,0);	//roll
			matrix2.RotateMatrixf(vBestRot.y,0,0,1);	//yaw		
			pCam2->SetAngles(vBestRot);
			pCam2->m_Matrix=matrix2;
			pCam2->m_OrigMatrix=matrix2;
			pCam2->SetPos(vBestPos);
			m_pVideo->SetCamera(pCam2,false,false);		

			MyOutputDebugString("Bundle adjustment step 1:\n Sum of square errors for 8 verts for camera %d: %f \n",k1,fBestError);
		} //k1

		//////////////////////////////////////////////////////////////////////////
		// bundle adjustment for verts	
		MyOutputDebugString("Bundle adjustment step 3: correcting vertices \n");

		m_fMinZ=9999;
		m_fMaxZ=0;

		for (int k=0;k<2;k++)
		{	
			if (k==1)
			{
				ftype fRange=m_fMaxZ-m_fMinZ;
				m_fMaxZ+=fRange/5000;
				m_fMinZ-=fRange/5000;
			}
			for (lstPhotoVerticesIt i=m_lstVertices.begin();i!=m_lstVertices.end();i++)
			{
				CPVertex *pVert=(*i);		

				//if (pVert->m_vPos.x==0 && pVert->m_vPos.y==0)
				if (pVert->m_vPos.IsEquivalent(vector3f(0,0,0),0.000001))
					continue;
						
				lstCameras cameras;				

				for (int k1=0;k1<nFrames;k1++)
				{		
					// find all cameras that see this vert
					if (pVert->m_v2DPos[k1].x<0 || pVert->m_v2DPos[k1].y<0)
						continue;		
					CCamera *pCam1=&m_lstFrames[k1]->m_Camera;
					cameras.push_back(pCam1);									
				}

				if (cameras.size()<2)
					continue;

				if (k==0)
				{
					for (int i=0;i<(int)(cameras.size()-1);i++)
					{
						vector3f vPos;
						m_pVideo->ProjectToScreen(pVert->m_vPos,vPos,cameras[i]);

						if (vPos.z>m_fMaxZ)
							m_fMaxZ=vPos.z;
						if (vPos.z<m_fMinZ)
							m_fMinZ=vPos.z;			
					}
					continue;
				}
				
				for (int i=0;i<(int)(cameras.size()-1);i++)
				{		
					CCamera *pCam1=cameras[i];
					CCamera *pCam2=cameras[i+1];

					vector3f vRes;
					FindBest3DPos(pVert,m_lstFrames[pCam1->m_nFrame],m_lstFrames[pCam2->m_nFrame],vRes);
					if (i==0)				
						pVert->m_vPos=vRes;									
					else
						pVert->m_vPos=(pVert->m_vPos+vRes)/2.0;
				}
			} //i			
		}

		//////////////////////////////////////////////////////////////////////////
		// adjust focal length, for all cameras
		for (int k1=0;k1<nFrames;k1++)
		{
			CCamera *pCam2=&m_lstFrames[k1]->m_Camera;		

			lstPhotoVertices testVerts2;
			GetSampleVerts(&testVerts2,w,h,k1,true);

			vector3f vBestRot=pCam2->GetAngles();
			vector3f vBestPos=pCam2->GetPos();

			CMatrixf matrix2;
			matrix2.Identity();		
			matrix2.RotateMatrixf(vBestRot.x,1,0,0);	//pitch	
			matrix2.RotateMatrixf(vBestRot.z,0,1,0);	//roll
			matrix2.RotateMatrixf(vBestRot.y,0,0,1);	//yaw		

			// try focal lenght now
			ftype fRange=DEG2RAD(8.0);	
			ftype fStep=fRange/8.0; // 0.01
			ftype	fFovStart=pCam2->m_fFov;		
			ftype	fBestFov=fFovStart;
			ftype fBestError=99999;		
			while (fBestError>1 && fRange>0.0001)
			{					
				for (ftype fFov=fFovStart-fRange;fFov<fFovStart+fRange;fFov+=fStep)
				{
					pCam2->Init(w,h,fFov,100,0.15);
					pCam2->SetAngles(vBestRot);
					pCam2->m_Matrix=matrix2;
					pCam2->m_OrigMatrix=matrix2;
					pCam2->SetPos(vBestPos);
					m_pVideo->SetCamera(pCam2,false,false);		

					ftype fError=0;
					for (lstPhotoVerticesIt i1=testVerts2.begin();i1!=testVerts2.end();i1++)
					{
						CPVertex *pVert=(*i1);

						vector3f vProj;
						m_pVideo->ProjectToScreen(pVert->m_vPos,vProj,pCam2);
						vProj.z=1.0;						
						fError+=pVert->m_v2DPos[k1].Distance2(vProj);
						if (fError>fBestError)
							break;
					} //i1

					if (fError<fBestError)
					{
						fBestError=fError;
						fBestFov=fFov;
					}
				} //fFov

				fRange/=2.0;
				fStep/=2.0;
				fFovStart=fBestFov;
			} // while

			pCam2->Init(w,h,fBestFov,100,0.15);
			pCam2->SetAngles(vBestRot);
			pCam2->m_Matrix=matrix2;
			pCam2->m_OrigMatrix=matrix2;
			pCam2->SetPos(vBestPos);
			m_pVideo->SetCamera(pCam2,false,false);		
		} //k1
	} // loaded from bojou 

	//////////////////////////////////////////////////////////////////////////
	// remove useless verts
	lstPhotoVertices tempVerts;
	for (lstPhotoVerticesIt i=m_lstVertices.begin();i!=m_lstVertices.end();i++)
	{
		CPVertex *pVert=(*i);		

		//if (pVert->m_vPos.x==0 && pVert->m_vPos.y==0)
		if (pVert->m_vPos.IsEquivalent(vector3f(0,0,0),0.000001))
		{
			delete pVert;
			continue;
		}

		// remove vertices projected behind the first cam
		//if (!m_lstFrames[KEY_FRAME]->m_Camera.IsPointVisible(pVert->m_vPos,MASK_ALL_PLANES))
		if (!m_lstFrames[KEY_FRAME]->m_Camera.IsPointVisible(pVert->m_vPos,1|4|8|16|32))
		{				
			pVert->m_vPos.Clear();		
			for (int k=0;k<nFrames;k++)				
				pVert->m_v2DPos[k].Set(-1,-1,-1);				
		}			

		tempVerts.push_back(pVert);
	} //i

	m_lstVertices.clear();
	for (lstPhotoVerticesIt i=tempVerts.begin();i!=tempVerts.end();i++)
	{
		CPVertex *pVert=(*i);		
		m_lstVertices.push_back(pVert);
	} //i
	tempVerts.clear();	
	
	MatchAdditionalFeatures();

	m_vMins.SetMax();
	m_vMaxs.SetMin();

	//////////////////////////////////////////////////////////////////////////
	for (int k1=0;k1<nFrames;k1++)
	{			
		vector3f vProj;
		CCamera *pCam=&m_lstFrames[k1]->m_Camera;

		ftype fErrorSum=0,fNorm=0;
		for (lstPhotoVerticesIt i=m_lstVertices.begin();i!=m_lstVertices.end();i++)
		{
			CPVertex *pVert=(*i);		

			if (pVert->m_v2DPos[k1].x<0 || pVert->m_v2DPos[k1].y<0)
				continue; // feature not present in this view
			
			m_vMaxs.CheckMax(pVert->m_vPos);
			m_vMins.CheckMin(pVert->m_vPos);

			m_pVideo->ProjectToScreen(pVert->m_vPos,vProj,pCam);
			vProj.z=1.0;
			vProj.IntValues();
			fErrorSum+=pVert->m_v2DPos[k1].Distance2(vProj);										
			fNorm+=1.0;
		} //i

		fErrorSum/=fNorm;

		// rock2 test:
		// Final average pixel reprojection error for camera 0: 0.777778 
		// Final average pixel reprojection error for camera 1: 0.840367 
		// Final average pixel reprojection error for camera 2: 0.714499 
		MyOutputDebugString("Final average pixel reprojection error for camera %d: %f \n",k1,fErrorSum);
	
	} //k1	

	// enlarge bbox a little
	vector3f vCenter=(m_vMins+m_vMaxs)/2.0;
	vector3f vDir=m_vMins-vCenter;
	m_vMins=vCenter+(vDir*1.15);
	vDir=m_vMaxs-vCenter;
	m_vMaxs=vCenter+(vDir*1.15);
	m_vMaxs=m_vMaxs;
	m_vMins=m_vMins;

	SaveCalibData();
	
	return (true);
}