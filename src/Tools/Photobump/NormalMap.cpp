
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
#include "opencv/highgui.h"

#include "WmlApprPlaneFit3.h"
using namespace Wml;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////////
uchar *CPhotoBump10Doc::AlignNormalMap(const uchar *pData,int nW,int nH)
{
		
	int nDiffs[3];
	uchar cS[3];

	/*
	uchar *dst=new uchar [nW*nH*3];
	// calc average using a gaussian filter		
	IplImage *pImgHeaderSrc=cvCreateImageHeader(cvSize(nW,nH),8,3);
	IplImage *pImgHeaderDst=cvCreateImageHeader(cvSize(nW,nH),8,3);
	pImgHeaderSrc->imageData=pImgHeaderSrc->imageDataOrigin=(char *)(pData);
	pImgHeaderDst->imageData=pImgHeaderSrc->imageDataOrigin=(char *)(dst);
	//cvResize(pImgHeaderSrc,pImgHeaderDst);			
	//cvSmooth(pImgHeaderSrc,pImgHeaderDst,CV_GAUSSIAN,31,31);
	cvSmooth(pImgHeaderSrc,pImgHeaderDst,CV_GAUSSIAN,55,55);
	cvReleaseImageHeader(&pImgHeaderSrc);
	cvReleaseImageHeader(&pImgHeaderDst);			
	*/
	
	/*
	uchar *dst2=new uchar [1*2*3];
	::Resize(dst,dst2,nW,nH,1,1,24);
	
	cS[0]=dst2[0];
	cS[1]=dst2[1];
	cS[2]=dst2[2];
	*/

	int64 nVals[3]={0,0,0};	
	for (int k=0;k<nW*nH;k++)
	{
		for (int j=0;j<3;j++)
			nVals[j]+=pData[k*3+j];
	} //k
	for (int j=0;j<3;j++)
		cS[j]=nVals[j]/(nW*nH);
	
	nDiffs[0]=128-(int)(cS[0]);
	nDiffs[1]=128-(int)(cS[1]);
	nDiffs[2]=255-(int)(cS[2]);

	//SAFE_DELETE_ARRAY(dst);
	//SAFE_DELETE_ARRAY(dst2);	
	
	uchar *szRes=new uchar [nW*nH*3];
	for (int k=0;k<nW*nH;k++)
	{
		for (int j=0;j<3;j++)
		{
			/*
			int cRes1=pData[k*3+j];
			int cRes2=dst[k*3+j];
			int nDiff=cRes2-cRes1;
			int cRes=cRes1+nDiff/2;
			*/
						
			int cRes=pData[k*3+j];
			cRes+=nDiffs[j];
			//szRes[k*3+j]=cS[j];			

			if (cRes<0)
				cRes=0;			
			if (cRes>255)
				cRes=255;
			szRes[k*3+j]=cRes;			

			//szRes[k*3+j]=dst[k*3+j];			

		} //j
	} //k	

	return (szRes);
}

//////////////////////////////////////////////////////////////////////////
uchar *CPhotoBump10Doc::AlignNormalMapFromFile(const char *szFilename,int &nW,int &nH)
{
	CPhotoImage *pImg=m_pVideo->LoadTexture(szFilename,CGTEXT_FLAG_DONOTUPLOAD|CGTEXT_FLAG_KEEP);
	if (!pImg)
		return (NULL);

	nW=pImg->m_nWidth;
	nH=pImg->m_nHeight;
	uchar *szRes=AlignNormalMap(pImg->m_pRawPixels,nW,nH);
	m_pVideo->RemoveTexture(pImg);

	return (szRes);
}

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10Doc::GenerateNormalMap(CPaintDC *dc)
{
	CLoadingBar tLoadingBar(m_pVideo,dc,"Step 3 of 3: Generating normal map and heightmap");		

	int w=m_lstFrames[KEY_FRAME]->m_pImage->m_nWidth;
	int h=m_lstFrames[KEY_FRAME]->m_pImage->m_nHeight;
	CPhotoFrame *pFrame=m_lstFrames[KEY_FRAME];
	CCamera *pCam=&pFrame->m_Camera;
	char szFilename[1024];
	vector3f *vPoints=m_lstFrames[KEY_FRAME]->m_pPoints;
	uchar *pGray=m_lstFrames[KEY_FRAME]->m_pImage->m_pRawPixels;

	//////////////////////////////////////////////////////////////////////////
	// create normal map and heightmap images				
	CPlanef tPlane;
			
	vector3f	vCenter=(m_vMins+m_vMaxs)/2.0;
	vector3f	vNormal=pCam->GetPos()-vCenter;
	vNormal.Normalize();
	ftype	fPlaneDist=vCenter.Distance(pCam->GetPos());
	tPlane.Set(vNormal,-fPlaneDist);		

	//ftype fScale=1.0;
	ftype	*pDepth=new ftype [w*h];		

	//ftype	*pDepth=pFrame->m_pBestDepth;
	ftype fMinDist=999999,fMaxDist=-999999;
	vector3f vPos;	

	for (int k=0;k<w*h;k++)
	{
		vPos=pFrame->m_pPoints[k];
		if ((vPos.x==0 && vPos.y==0) || pFrame->m_pBestDepth[k]<0) 
		{
			//pDepth[k]=-3333;
			continue;
		}
				
		if (!(vPos.InsideBBox(m_vMins,m_vMaxs)))
		{
			ftype fZ=pFrame->m_pBestDepth[k];
			if (fZ<m_fMinZ)
				fZ=m_fMinZ;
			if (fZ>m_fMaxZ)
				fZ=m_fMaxZ;

			int y=k/w;
			int x=k-(y*w);
			vector3f vSource(x,y,fZ);
			m_pVideo->UnProjectFromScreen(vSource,vPos,pCam);
		}		
				
		pDepth[k]=tPlane.DistFromPlane(vPos);		
		if (pDepth[k]<fMinDist)
		{
			fMinDist=pDepth[k];
			//pDepth[k]=fMinDist;		
		}
		if (pDepth[k]>fMaxDist)
		{
			fMaxDist=pDepth[k];
			//pDepth[k]=fMaxDist;		
		}	
	} //k		

	//////////////////////////////////////////////////////////////////////////
	ftype fScale=(255.0/(fMaxDist-fMinDist));
	
	ftype	fC,fCx,fCy;
	
	uchar	*pDest=new uchar [w*h*3];
	memset(pDest,0,w*h*3);

	float fNormalMapScale=m_fNormalMapScale;
	if (m_bStereoMode)
		fNormalMapScale*=3.0;
	ftype fMult=(m_fDetailScale-1.0)/10.0;
	if (m_bGenerateNormalMap)
	{	
		for (int y=1;y<h;y++)
		{
			for (int x=0;x<w-1;x++)
			{				
				if (pFrame->m_pBestDepth[y*w+x]<0)
					continue;
				fC=pDepth[y*w+x];

				if (pFrame->m_pBestDepth[y*w+x+1]<0)
					continue;
				fCx=pDepth[y*w+x+1];

				if (pFrame->m_pBestDepth[(y-1)*w+x]<0)
					continue;
				fCy=pDepth[(y-1)*w+x];

				fC+=(((ftype)0.5-(pGray[y*w+x]/255.0))*fMult); 
				fCx+=(((ftype)0.5-(pGray[y*w+x+1]/255.0))*fMult); 
				fCy+=(((ftype)0.5-(pGray[(y-1)*w+x]/255.0))*fMult); 

				ftype dcx=2.0*fScale*((fC-fMinDist)-(fCx-fMinDist))*fNormalMapScale;
				ftype dcy=2.0*fScale*((fC-fMinDist)-(fCy-fMinDist))*fNormalMapScale;

				//ftype dcx=32.0*(fC-fCx);
				//ftype dcy=32.0*(fC-fCy);

				// Normalize the vector 
				ftype sqlen = dcx*dcx + dcy*dcy + 1;
				ftype reciplen = 1.0/(ftype)sqrt(sqlen);
				ftype nx = dcy*reciplen;
				ftype ny = -dcx*reciplen;
				ftype nz = reciplen;

				// Repack the normalized vector into an RGB unsigned byte
				// vector in the normal map image 

				//////////////////////////////////////////////////////////////////////////
				// swap and invert channels

				//pDest[y*w*3+x*3+0]=(uchar) (128.0 + 127.0*nx);
				//pDest[y*w*3+x*3+1]=(uchar) (128.0 + 127.0*ny);
				//pDest[y*w*3+x*3+2]=(uchar) (128.0 + 127.0*nz);

				pDest[y*w*3+x*3+0]=255-(uchar)(128.0 + 127.0*ny);
				pDest[y*w*3+x*3+1]=255-(uchar)(128.0 + 127.0*nx);
				pDest[y*w*3+x*3+2]=(uchar)(128.0 + 127.0*nz);
				//////////////////////////////////////////////////////////////////////////
			} //x
		} //y	

		uchar *szRes=AlignNormalMap(pDest,w,h);
		memcpy(pDest,szRes,w*h*3);
		SAFE_DELETE_ARRAY(szRes);

		sprintf(szFilename,"%s-NormalMap.tga",pFrame->m_pImage->m_szName);
		SaveTga(pDest,3,w,h,szFilename,true);
	}

	tLoadingBar.Tick(50);

	//ftype fNormalizer=fMaxDist-fMinDist;
	if (m_bGenerateHeightMap)
	{	
		memset(pDest,0,w*h*3);
		for (int k=0;k<w*h;k++)
		{
			//if (pDepth[k]==-3333)
			if (pFrame->m_pBestDepth[k]<0)
				continue;
			ftype fVal=((pDepth[k]-fMinDist)*fScale);
			fVal+=(((ftype)0.5-(pGray[k]/255.0))*(1.0-m_fDetailScale)*8.0);
			//ftype fVal=(pDepth[k]-fMinDist);
			if (fVal<0)
				fVal=0;
			if (fVal>255)
				fVal=255;
			pDest[k*3+0]=(uchar)(fVal);
			pDest[k*3+1]=(uchar)(fVal);
			pDest[k*3+2]=(uchar)(fVal);
		} //k

		sprintf(szFilename,"%s-HeightMap.tga",pFrame->m_pImage->m_szName);
		SaveTga(pDest,3,w,h,szFilename,true);
	}

	delete []	pDepth;
	delete [] pDest;

	tLoadingBar.Tick(50);
}

