
#include "stdafx.h"
#include "PhotoBump10.h"
#include "PhotoBump10Doc.h"
#include "PhotoBump10View.h"
#include "Video.h"
#include "PhotoFrame.h"
#include "PhotoImage.h"
#include "PVertex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////////
ftype CPhotoBump10Doc::CalcError(CPhotoFrame *pFrame,CCamera *pCam)
{
	ftype fError=0;
	// just to get the gl matrix back
	int w=pFrame->m_pImage->m_nWidth;
	int h=pFrame->m_pImage->m_nHeight;

	pCam->m_nViewport[0]=0;
	pCam->m_nViewport[1]=0;
	pCam->m_nViewport[2]=w;
	pCam->m_nViewport[3]=h;		
	m_pVideo->SetCamera(pCam,false,false);				
	/*
	for (lstPhotoVerticesIt i2=m_lstVertices.begin();i2!=m_lstVertices.end();i2++)
	{
		CPVertex *pVert=(*i2);
		vector3f vRes;
		m_pVideo->ProjectToScreen(pVert->m_vPos,vRes,pCam);		
		ftype fDist=sqrt(dsqr(vRes.x-pVert->m_X[pFrame->m_nFrameNum])+dsqr(vRes.y-pVert->m_Y[pFrame->m_nFrameNum]));
		fError+=fDist;			
	} //i
	*/
	return (fError);
}

//#define DUMP_BIN

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10Doc::DumpError(CPhotoFrame *pFrame)
{
	int w=pFrame->m_pImage->m_nWidth;
	int h=pFrame->m_pImage->m_nHeight;

	CCamera *pCam=&pFrame->m_Camera;
	pCam->m_nViewport[0]=0;
	pCam->m_nViewport[1]=0;
	pCam->m_nViewport[2]=w;
	pCam->m_nViewport[3]=h;		
	m_pVideo->SetCamera(&pFrame->m_Camera,false,false);				

	if (m_bInitialUpdate)
	{
		//char szFilename[1024];
		//sprintf(szFilename,"%s-Original.tga",pFrame->m_pImage->m_szName);
		//SaveTga(pFrame->m_pImage->m_pRawPixels,3,w,h,szFilename,true);
		return;
	}

	int nNum=0;
	int k=pFrame->m_nFrameNum;
	for (lstPhotoVerticesIt i2=m_lstVertices.begin();i2!=m_lstVertices.end();i2++,nNum++)
	{
		if (nNum>=m_nNumFeatures)
			break;
		CPVertex *pVert=(*i2);
		vector3f vRes=pVert->m_v2DPos[k];

		//m_pVideo->ProjectToScreen(pVert->m_vPos,vRes,pCam);
		int x=(int)(vRes.x+ROUNDING_VAL);
		int y=(int)(vRes.y+ROUNDING_VAL);

		if (x<0 || x>=w || y<0 || y>=h)
			continue;

		pFrame->m_pImage->DrawPoint(vRes,1,0,255,0);						

		//m_pVideo->ProjectToScreen(pVert->m_vPos,vRes,&pFrame->m_Camera);				
		//pFrame->m_pImage->DrawPoint((int)(pVert->m_X[k]),(int)(pVert->m_Y[k]),3,0,255,0);						
		/*
		ftype fDist=sqrt(dsqr(vRes.x-pVert->m_X[k])+dsqr(vRes.y-pVert->m_Y[k]));				
		if (fDist>255)
			fDist=255;
		pFrame->m_pImage->DrawPoint((int)(vRes.x),(int)(vRes.y),3,(int)(fDist),0,255);			
		pFrame->m_pImage->DrawLine(vector3f(pVert->m_X[k],pVert->m_Y[k],0),vRes,
			vector3f(0,255,0),vector3f((fDist*2),0,255));
		*/
	} //i
	char szFilename[1024];
	sprintf(szFilename,"%s-Features.tga",pFrame->m_pImage->m_szName);
	SaveTga(pFrame->m_pImage->m_pRawPixels,3,w,h,szFilename,true);

	/*
	if (pFrame->m_pScaledImage)
	{	
		sprintf(szFilename,"%s-Scaled.tga",pFrame->m_pImage->m_szName);
		SaveTga(pFrame->m_pScaledImage->m_pRawPixels,3,pFrame->m_pScaledImage->m_nWidth,pFrame->m_pScaledImage->m_nHeight,szFilename,true);
	}
	*/

	uchar *pTemp=new uchar [w*h*3];
	if (pFrame->m_pBestDepth)
	{	
		sprintf(szFilename,"%s-BestDepth.tga",pFrame->m_pImage->m_szName);
		for (int k=0;k<w*h;k++)
		{
			/*
			if (pFrame->m_pConfidence[k]<0)
			{				
				pTemp[k*3+0]=255;
				pTemp[k*3+1]=0;
				pTemp[k*3+2]=0;
			}
			else
			*/
			{			
				int nRes=(int)((pFrame->m_pBestDepth[k]-m_fMinZ)*8192.0);				
				if (nRes<0)
					nRes=0;
				if (nRes>255)
					nRes=255;

				for (int j=0;j<3;j++)
				{
					pTemp[k*3+j]=nRes;
				} //j
			} 
		}//k
		SaveTga(pTemp,3,w,h,szFilename,true);
	}

	if (pFrame->m_pCurrZ)
	{		
		sprintf(szFilename,"%s-CurrentDepth.tga",pFrame->m_pImage->m_szName);		
		for (int k=0;k<w*h;k++)
		{			
			{			
				int nRes=(int)((pFrame->m_pCurrZ[k]-m_fMinZ)*8192.0);				
				if (nRes<0)
					nRes=0;
				if (nRes>255)
					nRes=255;

				for (int j=0;j<3;j++)
				{
					pTemp[k*3+j]=nRes;
				} //j
			} 
		}//k
		SaveTga(pTemp,3,w,h,szFilename,true);
	}

#ifdef DUMP_BIN
	if (pFrame->m_pBestDepth)
	{		
		sprintf(szFilename,"%s-Depth.bin",pFrame->m_pImage->m_szName);
		FILE *fp=fopen(szFilename,"wb");
		fwrite(&w,sizeof(int),1,fp);
		fwrite(&h,sizeof(int),1,fp);
		fwrite(pFrame->m_pBestDepth,w*h*sizeof(ftype),1,fp);
		fclose(fp);
	}

	if (pFrame->m_pState)
	{		
		sprintf(szFilename,"%s-State.bin",pFrame->m_pImage->m_szName);
		FILE *fp=fopen(szFilename,"wb");
		fwrite(&w,sizeof(int),1,fp);
		fwrite(&h,sizeof(int),1,fp);
		fwrite(pFrame->m_pState,w*h*sizeof(char),1,fp);
		fclose(fp);
	}

	/*
	for (int j=0;j<2;j++)
	if (pFrame->m_pPos[j])
	{		
		sprintf(szFilename,"%s-Pos%d.bin",pFrame->m_pImage->m_szName,j);
		FILE *fp=fopen(szFilename,"wb");
		fwrite(&w,sizeof(int),1,fp);
		fwrite(&h,sizeof(int),1,fp);
		fwrite(pFrame->m_pPos[j],w*h*sizeof(uint),1,fp);
		fclose(fp);
	}
	*/

	if (pFrame->m_pPoints)
	{
		sprintf(szFilename,"%s-Points.bin",pFrame->m_pImage->m_szName,j);
		FILE *fp=fopen(szFilename,"wb");
		fwrite(&w,sizeof(int),1,fp);
		fwrite(&h,sizeof(int),1,fp);
		fwrite(pFrame->m_pPoints,w*h*sizeof(vector3f),1,fp);
		fclose(fp);
	}
#endif

	if (pFrame->m_pConfidence)
	{			
		sprintf(szFilename,"%s-Confidence.tga",pFrame->m_pImage->m_szName);
		for (int k=0;k<w*h;k++)
		{
			if (pFrame->m_pConfidence[k]<0)
			{				
				pTemp[k*3+0]=255;
				pTemp[k*3+1]=0;
				pTemp[k*3+2]=0;
			}
			else
			{				
				int nRes=(int)(pFrame->m_pConfidence[k]*255.0);
				if (nRes<0)
					nRes=0;
				if (nRes>255)
					nRes=255;

				for (int j=0;j<3;j++)
				{
					pTemp[k*3+j]=nRes;
				}
			}
		} //k
		SaveTga(pTemp,3,w,h,szFilename,true);			
/*
#ifdef DUMP_BIN
		FILE *fp;

		sprintf(szFilename,"%s-Confidence.bin",pFrame->m_pImage->m_szName);
		fp=fopen(szFilename,"wb");
		fwrite(&w,sizeof(int),1,fp);
		fwrite(&h,sizeof(int),1,fp);
		fwrite(pFrame->m_pConfidence,w*h,1,fp);
		fclose(fp);

		sprintf(szFilename,"%s-Error.bin",pFrame->m_pImage->m_szName);
		fp=fopen(szFilename,"wb");
		fwrite(&w,sizeof(int),1,fp);
		fwrite(&h,sizeof(int),1,fp);
		fwrite(pFrame->m_pError,w*h,1,fp);
		fclose(fp);

#endif
*/	
	}

	SAFE_DELETE_ARRAY(pTemp);
}
