
#include "stdafx.h"
#include "PhotoBump10.h"
#include "PhotoBump10Doc.h"
#include "PhotoBump10View.h"
#include "Video.h"
#include "PhotoFrame.h"
#include "PhotoImage.h"
#include "PVertex.h"
#include "ImageProcessing.h"
#include "LoadingBar.h"

#include "opencv/cv.h"
//#include "opencv/cvaux.h"
#include "opencv/highgui.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif 

//////////////////////////////////////////////////////////////////////////
ftype CPhotoBump10Doc::CalcNormalizedCrossCorrelation(const vector3f &v1,const vector3f &v2,uchar *ptr1,uchar *ptr2,int w,int h,int nWindow)
{
	int x1=(int)(v1.x);
	int y1=(int)(v1.y);
	int x2=(int)(v2.x);
	int y2=(int)(v2.y);
	return (CalcNormalizedCrossCorrelation(x1,y1,x2,y2,ptr1,ptr2,w,h,nWindow));
}

//////////////////////////////////////////////////////////////////////////
ftype	CPhotoBump10Doc::FindBest3DPos(vector3f v1,vector3f v2,CPhotoFrame *pFrame1,CPhotoFrame *pFrame2,vector3f &vRes)
{
	vector3f	vRes1,vRes2,vBestPos(0,0,0);
	ftype		 	fRange=m_fMaxZ-m_fMinZ;
	ftype			fMinz=m_fMinZ;
	ftype			fMaxz=m_fMaxZ;	
	ftype			fBestError=99999;	

	CCamera *pCam1=&pFrame1->m_Camera;
	CCamera *pCam2=&pFrame2->m_Camera;
	
	vector3f	vMin,vMax;

	// find min/max 3d verts in cam 1
	m_pVideo->UnProjectFromScreen(vector3f(v1.x,v1.y,fMinz),vRes1,pCam1);
	m_pVideo->UnProjectFromScreen(vector3f(v1.x,v1.y,fMaxz),vRes2,pCam1);

	// project in cam 2
	m_pVideo->ProjectToScreen(vRes1,vMin,pCam2);
	m_pVideo->ProjectToScreen(vRes2,vMax,pCam2);

	int nLastX=0;
	int nLastY=0;
	
	while (fBestError>1)
	{						
		v1.z=(fMinz+fMaxz)/2.0;		
		m_pVideo->UnProjectFromScreen(v1,vRes1,pCam1);
		m_pVideo->ProjectToScreen(vRes1,vRes2,pCam2);
		ftype fError=v2.Distance2(vRes2);
		if (fError<fBestError)
		{
			fBestError=fError;
			vBestPos=vRes1;			
		}

		ftype fDistMin=v2.Distance2(vMin);
		ftype fDistMax=v2.Distance2(vMax);

		if (fDistMin<fDistMax)
		{
			fMaxz=v1.z;
			vMax=vRes2;
		}
		else
		{
			fMinz=v1.z;
			vMin=vRes2;
		}		

		if (((int)(vRes2.x)==nLastX) && ((int)(vRes2.y)==nLastY))
			break;

		nLastX=(int)(vRes2.x);
		nLastY=(int)(vRes2.y);
	}

	vRes=vBestPos;
	return (fBestError);
}

//////////////////////////////////////////////////////////////////////////
ftype CPhotoBump10Doc::FindBest3DPos(CPVertex *p1,CPhotoFrame *pFrame1,CPhotoFrame *pFrame2,vector3f &vRes)
{
	vector3f v1=p1->m_v2DPos[pFrame1->m_nFrameNum];v1.z=1.0;		
	vector3f v2=p1->m_v2DPos[pFrame2->m_nFrameNum];v2.z=1.0;		
	return (FindBest3DPos(v1,v2,pFrame1,pFrame2,vRes));
}

#define CROSS_CORR
//////////////////////////////////////////////////////////////////////////
void CPhotoBump10Doc::DenseMatching(CPaintDC *dc)
{

	int w=m_lstFrames[KEY_FRAME]->m_pImage->m_nWidth;
	int h=m_lstFrames[KEY_FRAME]->m_pImage->m_nHeight;

	int nFrames=(int)(m_lstFrames.size());

	if (Load3DPoints(m_lstFrames[KEY_FRAME]))
	{
		GenerateNormalMap(dc);
		return;
	}
		
	bool bLoaded=false;
	for (int k1=0;k1<nFrames;k1++)
	{
		CPhotoFrame *pFrame1=m_lstFrames[k1];

		m_lstFrames[k1]->m_pImage->ExtractEdges(false,false);		
	} //k1

	// TODO: combine info from all views
	//////////////////////////////////////////////////////////////////////////	

	// allocate memory for diffusion process
	for (int k1=0;k1<nFrames-1;k1++)
	{
		CPhotoFrame *pFrame=m_lstFrames[k1];

		pFrame->m_pBestDepth=new ftype [w*h];		
		pFrame->m_pState=new char [w*h];				
		pFrame->m_pPos=new uint [w*h*2]; // x+y - match pos in the 2nd view
		pFrame->m_nLastDiffuse=new int [w*h];		
		// need 2 of them, to add matches while going thru previous matches
		// and to swap at the end
		pFrame->m_nCurrDiffuse=new int [w*h]; 
		//memset(pFrame->m_pBestDepth,0,sizeof(ftype)*w*h);
		for (int k=0;k<w*h;k++)
			pFrame->m_pBestDepth[k]=-1;
		memset(pFrame->m_pState,0,sizeof(char)*w*h);				
		for (int k=0;k<w*h*2;k++)
			pFrame->m_pPos[k]=-1;
		//memset(pFrame->m_pPos,0,sizeof(uint)*w*h*2*nFrames);	
		memset(pFrame->m_nLastDiffuse,0,sizeof(uint)*w*h);	
		memset(pFrame->m_nCurrDiffuse,0,sizeof(uint)*w*h);	

		pFrame->m_nDiffused=0;
		pFrame->m_nDiffusePixel=0;
	} //k1
	
	// init seed points	
	int kvert=0;
	for (lstPhotoVerticesIt it=m_lstVertices.begin();it!=m_lstVertices.end();it++)
	{
		CPVertex *pVert=(*it);
				
		for (int k1=0;k1<nFrames-1;k1++)
		{		
			CCamera *pCam1=&m_lstFrames[k1]->m_Camera;
			CPhotoFrame *pFrame1=m_lstFrames[pCam1->m_nFrame];

			int x1=pVert->GetX(k1);
			int y1=pVert->GetY(k1);

			int x2=pVert->GetX(k1+1);
			int y2=pVert->GetY(k1+1);

			if (x1<0 || y1<0 || x2<0 || y2<0)
				continue;		

			vector3f vPos;
			m_pVideo->ProjectToScreen(pVert->m_vPos,vPos,pCam1);
			pVert->m_v2DPos[pFrame1->m_nFrameNum].z=vPos.z;
			pFrame1->m_pBestDepth[y1*w+x1]=vPos.z;

			//CCamera *pCam2=&m_lstFrames[k1+1]->m_Camera;			
			//CPhotoFrame *pFrame2=m_lstFrames[pCam2->m_nFrame];

			pFrame1->m_pState[y1*w+x1]=1; // sure match
			pFrame1->m_nLastDiffuse[pFrame1->m_nDiffused++]=y1*w+x1;

			pFrame1->m_pPos[y1*w*2+x1*2+0]=x2;
			pFrame1->m_pPos[y1*w*2+x1*2+1]=y2;		
						
		}//k1
	} //it
	
	CvMat* wpoints1 = cvCreateMat(3,1,CV_64F);
	CvMat* corrLines2 = cvCreateMat(3,1,CV_64F);
	CvMat* wpoints2 = cvCreateMat(3,1,CV_64F);
	CvMat* corrLines1 = cvCreateMat(3,1,CV_64F);

	int nBufW=5;
	uchar *buffer1=new uchar [nBufW*3];
	uchar *buffer2=new uchar [nBufW*3];	

	//////////////////////////////////////////////////////////////////////////
	for (int k1=0;k1<nFrames-1;k1++)
	{

		char szBuf[256];
		sprintf(szBuf,"Step 3 of 4: Dense Matching for camera %d",k1);
		CLoadingBar tLoadingBar(m_pVideo,dc,szBuf);		

		CPhotoFrame *pFrame1=m_lstFrames[k1];
		CPhotoFrame *pFrame2=m_lstFrames[k1+1];

		CCamera *pCam1=&pFrame1->m_Camera;
		CCamera *pCam2=&pFrame2->m_Camera;
		
		uchar *ptr1=pFrame1->m_pImage->m_pRawPixels;				
		uchar *ptr2=pFrame2->m_pImage->m_pRawPixels;				
				
		ftype fMaxDiff=(m_fMaxZ-m_fMinZ)/100.0;
		m_fThreshold=0.9;
		//m_fThreshold=0.7;
		while (m_fThreshold>(0.6)) // diffuse
		//while (m_fThreshold>(0.8)) // diffuse
		{			
			int nCurrDiffuse=0;

			for (int k4=0;k4<pFrame1->m_nDiffused;k4++)
			{	
				int y=pFrame1->m_nLastDiffuse[k4]/w;
				int x=pFrame1->m_nLastDiffuse[k4]-(y*w);
											
				for (int k3=1;k3<9;k3++)
				{									
					// diffuse neighborhood pixels
					int x1=x+dirs[k3][0];
					int y1=y+dirs[k3][1];

					if (x1<0 || y1<0 || x1>=w || y1>=h)
						continue;
					
					if (pFrame1->m_pState[y1*w+x1]!=0)	
						continue; // error or already found

					//if (pFrame1->m_pImage->m_pEdges[y1*w+x1])
					//	continue; // do not cross depth discontinuities

					int nRangeL=0;
					int nRangeR=0;

					if (dirs[k3][0]>0)
					{
						nRangeL=1;
						nRangeR=2;
					}
					else
					if (dirs[k3][0]<0)
					{
						nRangeR=2;
						nRangeL=1;
					}
					else
						nRangeL=nRangeR=1;

					// position in the other frame
					int x2=pFrame1->m_pPos[(y*w+x)*2+0]+dirs[k3][0];
					int y2=pFrame1->m_pPos[(y*w+x)*2+1]+dirs[k3][1];

					//int nBestError=8*nBufW*nBufW;
					int nBestError=((10-(int)(m_fThreshold*4))*nBufW*3)/2;
					ftype fBestError=0;
					int nBestPosX=x2;
					int nBestPosY=y2;				

					if ((x1-(nBufW>>1))<0 || (x1+(nBufW>>1))>=w || y1<(nBufW*2) || y1>=(h-nBufW*2))														
						continue;																								

					double a1[3],a2[3],b1[3],b2[3],c1[3],c2[3];

					for (int j=-1;j<=1;j++)
					{					
						cvmSet(wpoints2,0,0,x2);
						cvmSet(wpoints2,1,0,y2+(ftype)(j));
						cvmSet(wpoints2,2,0,1.0);
						cvComputeCorrespondEpilines(wpoints2,2,m_lstFrames[k1]->m_FundMatrix,corrLines1);
						a1[j+1]=cvmGet(corrLines1,0,0);
						b1[j+1]=cvmGet(corrLines1,1,0);
						c1[j+1]=cvmGet(corrLines1,2,0);

						cvmSet(wpoints1,0,0,x1);
						cvmSet(wpoints1,1,0,y1+(ftype)(j));
						cvmSet(wpoints1,2,0,1.0);
						cvComputeCorrespondEpilines(wpoints1,1,m_lstFrames[k1]->m_FundMatrix,corrLines2);
						// Each epipolar line is present by coefficients a,b,c of line equation: 
						// a*x + b*y + c = 0				
						a2[j+1]=cvmGet(corrLines2,0,0);
						b2[j+1]=cvmGet(corrLines2,1,0);
						c2[j+1]=cvmGet(corrLines2,2,0);
					}

					int xpos;
					for (xpos=-(nBufW>>1);xpos<=(nBufW>>1);xpos++)
					{
						int x3=x1;

						ftype ypos1=-(a1[0]*(x3+xpos)+c1[0])/b1[0];
						// NOTE: must add 0.5 otherwise results suck
						int y3=(int)(ypos1+0.5);			 		
						if (y3<0 || y3>=h)
							break;
						buffer1[xpos+(nBufW>>1)]=ptr1[y3*w+x3+xpos];

						ypos1=-(a1[1]*(x3+xpos)+c1[1])/b1[1];						
						y3=(int)(ypos1+0.5);					
						if (y3<0 || y3>=h)
							break;
						buffer1[xpos+(nBufW>>1)+nBufW]=ptr1[y3*w+x3+xpos];

						ypos1=-(a1[2]*(x3+xpos)+c1[2])/b1[2];						
						y3=(int)(ypos1+0.5);					
						if (y3<0 || y3>=h)
							break;
						buffer1[xpos+(nBufW>>1)+(nBufW*2)]=ptr1[y3*w+x3+xpos];
					} //xpos

					if (xpos<=(nBufW>>1))
						continue;
					
					bool bFound=false;
					for (int x4=x2-nRangeL;x4<x2+nRangeR;x4++)
					{
						ftype ypos2=-(a2[1]*(x4)+c2[1])/b2[1];
						// NOTE: must add 0.5 otherwise results suck
						int y5=(int)(ypos2+0.5);					

						ftype fError=0;
						int nError=0;

						if ((x4-(nBufW>>1))<0 || (x4+(nBufW>>1))>=w || y2<(nBufW*2) || y2>=(h-nBufW*2))						
							continue;						

						for (int xpos=-(nBufW>>1);xpos<=(nBufW>>1);xpos++)
						{						
							ftype ypos2=-(a2[0]*(x4+xpos)+c2[0])/b2[0];
							// NOTE: must add 0.5 otherwise results suck
							int y4=(int)(ypos2+0.5);					
							if (y4<0 || y4>=h)
								break;
							
#ifdef CROSS_CORR
							buffer2[xpos+(nBufW>>1)]=ptr2[y4*w+x4+xpos];

							ypos2=-(a2[1]*(x4+xpos)+c2[1])/b2[1];							
							y4=(int)(ypos2+0.5);					
							if (y4<0 || y4>=h)
								break;
							buffer2[xpos+(nBufW>>1)+nBufW]=ptr2[y4*w+x4+xpos];

							ypos2=-(a2[2]*(x4+xpos)+c2[2])/b2[2];							
							y4=(int)(ypos2+0.5);					
							if (y4<0 || y4>=h)
								break;
							buffer2[xpos+(nBufW>>1)+nBufW*2]=ptr2[y4*w+x4+xpos];
#else							
							nError+=abs((int)(buffer1[xpos+(nBufW>>1)])-(int)(ptr2[y4*w+x4+xpos]));	

							ypos2=-(a2[1]*(x4+xpos)+c2[1])/b2[1];							
							y4=(int)(ypos2+0.5);					
							nError+=abs((int)(buffer1[xpos+(nBufW>>1)+nBufW])-(int)(ptr2[y4*w+x4+xpos]));	

							ypos2=-(a2[2]*(x4+xpos)+c2[2])/b2[2];							
							y4=(int)(ypos2+0.5);					
							nError+=abs((int)(buffer1[xpos+(nBufW>>1)+nBufW*2])-(int)(ptr2[y4*w+x4+xpos]));
							if (nError>nBestError)
								break;
#endif
						} //xpos

						if (xpos<=(nBufW>>1))
							continue;

#ifdef CROSS_CORR
						fError+=CalcNormalizedCrossCorrelation((nBufW>>1),1,(nBufW>>1),1,buffer1,buffer2,nBufW,3,nBufW);
						if (fError>fBestError)
#else
						if (nError<nBestError)
#endif
						{
							fBestError=fError;
							nBestError=nError;
							nBestPosX=x4;
							nBestPosY=y5;
							bFound=true;
						}
					} //x4

#ifdef CROSS_CORR
					if (fBestError>m_fThreshold)
#else
					if (bFound)
#endif
					{	
						vector3f v1(x1,y1,1.0);
						vector3f v2(nBestPosX,nBestPosY,1.0);
						vector3f vRes,vRes1;
						FindBest3DPos(v1,v2,pFrame1,pFrame2,vRes);
						
						m_pVideo->ProjectToScreen(vRes,v1,pCam1);

						//ftype prevZ=pFrame1->m_pBestDepth[y*w+x];
						//if ((fabs(prevZ-v1.z))>fMaxDiff)
						//{
						//	if (v1.z<prevZ)
						//		v1.z=prevZ-fMaxDiff;
						//	else
						//		v1.z=prevZ+fMaxDiff;
						//}

						pFrame1->m_pState[y1*w+x1]=1;
						//pFrame1->m_pBestDepth[y1*w+x1]=v1.z;
						pFrame1->m_pBestDepth[y1*w+x1]=(v1.z+pFrame1->m_pBestDepth[y*w+x])/2.0;						

						/*
						ftype fSum=0;
						int nArea=0;
						for (int k3=0;k3<9;k3++)
						{				
							int x1=x+dirs[k3][0];
							int y1=y+dirs[k3][1];

							if (x1<0 || y1<0 || x1>=w || y1>=h)
								continue;

							if (!pFrame1->m_pState[y1*w+x1])
								continue;
							fSum+=pFrame1->m_pBestDepth[y1*w+x1];
							nArea++;
						} //k3
						pFrame1->m_pBestDepth[y1*w+x1]=fSum/(ftype)(nArea);
						*/

						pFrame1->m_nDiffusePixel++;
						pFrame1->m_nCurrDiffuse[nCurrDiffuse++]=y1*w+x1;
						pFrame1->m_pPos[(y1*w+x1)*2+0]=nBestPosX;
						pFrame1->m_pPos[(y1*w+x1)*2+1]=nBestPosY;				 							
					}
				} //k3
			} //k4

			tLoadingBar.Set(((ftype)(pFrame1->m_nDiffusePixel)/(ftype)(w*h))*100.0);
			MyOutputDebugString("Frame %d, completion %d, threshold=%f, diffused=%d \n",pFrame1->m_nFrameNum,(int)(((ftype)(pFrame1->m_nDiffusePixel)/(ftype)(w*h))*100.0),m_fThreshold,pFrame1->m_nDiffused);
			
			if (nCurrDiffuse==0 && m_fThreshold>(0.6))
			{		
				// if it didn't diffuse any more pixels, search
				// for pixel to diffuse and decrease threshold
//#ifndef CROSS_CORR
//				break;
//#endif

				pFrame1->m_nDiffused=0;
				// add back last assigned points and decrease threshold		
				for (int k=0;k<w*h;k++)
				{
					if (pFrame1->m_pState[k]==0)
						continue;
					int y=k/w;
					int x=k-(y*w);

					int k3;
					for (k3=1;k3<9;k3++)
					{				
						int x1=x+dirs[k3][0];
						int y1=y+dirs[k3][1];

						if (x1<0 || y1<0 || x1>=w || y1>=h)
							continue;

						if (pFrame1->m_pState[y1*w+x1]==0)
							break; // there is a hole nearby, diffuse it then
					} //k3
					
					if (k3<9)
						pFrame1->m_nLastDiffuse[pFrame1->m_nDiffused++]=k;
				} //k
				m_fThreshold-=0.1;				
			}
			else
			{			
				memcpy(pFrame1->m_nLastDiffuse,pFrame1->m_nCurrDiffuse,nCurrDiffuse*sizeof(int));
				pFrame1->m_nDiffused=nCurrDiffuse;
				// continue diffusion from last diffused pixels
			}			
		} //while

		//break; // TODO: all frames
	} //k1	

	SAFE_DELETE_ARRAY(buffer1);
	SAFE_DELETE_ARRAY(buffer2);

	cvReleaseMat(&wpoints1);
	cvReleaseMat(&corrLines2);	
	cvReleaseMat(&wpoints2);
	cvReleaseMat(&corrLines1);	

	for (int k1=0;k1<nFrames;k1++)	
	{

		CPhotoFrame *pFrame=m_lstFrames[k1];

		/*
		if (pFrame->m_pBestDepth)
		{		
			char szFilename[1024];
			sprintf(szFilename,"%s-Depth.bin",pFrame->m_pImage->m_szName);
			FILE *fp=fopen(szFilename,"wb");
			fwrite(&w,sizeof(int),1,fp);
			fwrite(&h,sizeof(int),1,fp);
			fwrite(pFrame->m_pBestDepth,w*h*sizeof(ftype),1,fp);
			fclose(fp);
		}
		*/

		/*
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
		*/
	} //k1

	SmoothSegments3Part2(dc);

	GenerateNormalMap(dc);

	/*
	for (int k1=0;k1<nFrames-1;k1++)
	{
		CPhotoFrame *pFrame1=m_lstFrames[k1];

		pFrame1->m_nDiffused=0;
		pFrame1->m_nDiffusePixel=0;
		memset(pFrame1->m_pState,0,w*h);

		for (lstPhotoVerticesIt it=m_lstVertices.begin();it!=m_lstVertices.end();it++)
		{
			CPVertex *pVert=(*it);

			int x1=pVert->GetX(pFrame1->m_nFrameNum);
			int y1=pVert->GetY(pFrame1->m_nFrameNum);

			if (x1<0 || y1<0)
				continue;

			pFrame1->m_pState[y1*w+x1]=1; // sure match
			pFrame1->m_nLastDiffuse[pFrame1->m_nDiffused++]=y1*w+x1;
		} //it
	} //k1		
	*/
}

