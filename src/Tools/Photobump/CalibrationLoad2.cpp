

#include "stdafx.h"
#include "PhotoBump10.h"
#include "PhotoBump10Doc.h"
#include "Video.h"
#include "PhotoFrame.h"
#include "PhotoImage.h"
#include "PVertex.h"
#include "ReconstructionOptionsDialog.h"

#include "opencv/cv.h"
//#include "opencv/cvaux.h"
#include "opencv/highgui.h"

#include "LoadingBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//#define MAX_IMAGES 32
#define MAX_IMAGES 2

//////////////////////////////////////////////////////////////////////////
bool CPhotoBump10Doc::LoadCalibrationData2(const char *_szFilename,CPaintDC *dc)
{	
 
	if (!theApp.m_bStandalone)
	{
		CReconstructionOptionsDialog RecDialog;	
		if (RecDialog.DoModal()!=IDOK)
			return (false);	
		m_fNormalMapScale=RecDialog.m_fNormalMapScale;
		m_fDetailScale=RecDialog.m_fDetailScale;
		m_bShowFeaturesOnly=(RecDialog.m_bShowFeaturesOnly?true:false);
		m_bUseSingleStereoPhoto=(RecDialog.m_bUseSingleStereoPhoto?true:false);
		m_bUseDoubleStereoPhoto=(RecDialog.m_bUseDoubleStereoPhoto?true:false);
		if (m_bUseDoubleStereoPhoto)
			m_bUseSingleStereoPhoto=false;		
	}
	else
	{
		m_fNormalMapScale=1.0;
		m_fDetailScale=1.0;
		//m_bShowFeaturesOnly=true;
		//m_bUseSinglePhoto=false;
		m_bShowFeaturesOnly=false;
		m_bUseSingleStereoPhoto=false;
		m_bUseDoubleStereoPhoto=true;
	}

	m_bStereoMode=m_bUseSingleStereoPhoto||m_bUseDoubleStereoPhoto;

	//return (RectifyDistortion(_szFilename,dc));

	//if (strstr(_szFilename,".avi"))
	//{		
	//	return (CalibrationAVI2(_szFilename,dc));
	//}

	if (strstr(_szFilename,".txt"))
	{
		return (LoadBojouCalibrationData(_szFilename,dc));
	}

	CLoadingBar tLoadingBar(m_pVideo,dc,"Loading Images");	

	char	szPath[512],szFilename[512];
	strcpy(szFilename,_szFilename);
	strcpy(szPath,m_pUtils->GetPath(_szFilename));

	//////////////////////////////////////////////////////////////////////////
	// load pictures and frames
	int k=0;
	while (1)
	{		
		char szNewFilename[512];
		strcpy(szNewFilename,szFilename);
		// replace frame num in filename, increasing by 1
		// for each frame
		char szExt[32];
		strcpy(szExt,m_pUtils->GetExtension(szNewFilename));
		m_pUtils->RemoveExtension(szNewFilename);
		char szNumber[32];
		memset(szNumber,0,32);
		char *szPtr=szNewFilename+strlen(szNewFilename)-1;
		char *szNumPtr=szNumber; 
		int nDigits=0;
		while (*szPtr>='0' && *szPtr<='9')
		{
			*szPtr--;
			nDigits++;
		}				
		szPtr++;
		// remove all zeros in front of the number
		while (*szPtr=='0' && nDigits>1)
		{
			szPtr++;
			nDigits--;
		}
		char *szPtrStart=szPtr;
		int j;
		for (j=0;j<nDigits;j++)
		{
			*szNumPtr++=*szPtr++;
		} //j						
		int nNum=atoi(szNumber)+(k); // increase frame num
		// replace into filename
		sprintf(szNumber,"%d",nNum);
		szNumPtr=szNumber;
		for (j=0;j<nDigits;j++)
		{
			*szPtrStart++=*szNumPtr++;
		} //j						

		strcat(szNewFilename,".");
		strcat(szNewFilename,szExt);

		char szImageName[512];
		strcpy(szImageName,m_pUtils->GetFilename(szNewFilename));
		sprintf(szNewFilename,"%s%s",szPath,szImageName);

		CPhotoImage *pImg=NULL;
		CPhotoFrame *pFrame=NULL;

		if (m_bUseSingleStereoPhoto)
		{
			pImg=m_pVideo->LoadTexture(szNewFilename,CGTEXT_FLAG_KEEP|CGTEXT_FLAG_DONOTUPLOAD);
			if (!pImg)
				break;			

			// split in two
			CPhotoImage *pImg1=new CPhotoImage(m_pVideo);
			CPhotoImage *pImg2=new CPhotoImage(m_pVideo);

			pImg1->m_nBpp=pImg->m_nBpp;
			pImg1->m_nWidth=pImg->m_nWidth>>1;
			pImg1->m_nHeight=pImg->m_nHeight;			
			strcpy(pImg1->m_szName,pImg->m_szName);
			pImg1->m_pRawPixels=new uchar [pImg1->m_nWidth*pImg1->m_nHeight*(pImg1->m_nBpp>>3)];
			pImg1->m_pImgHeader=cvCreateImageHeader(cvSize(pImg1->m_nWidth,pImg1->m_nHeight),8,(pImg1->m_nBpp>>3));
			pImg1->m_pImgHeader->imageData=(char *)(pImg1->m_pRawPixels);

			pFrame=new CPhotoFrame;
			pFrame->m_nFrameNum=k;
			pFrame->m_pImage=pImg;

			pImg2->m_nBpp=pImg->m_nBpp;
			pImg2->m_nWidth=pImg->m_nWidth>>1;
			pImg2->m_nHeight=pImg->m_nHeight;			
			strcpy(pImg2->m_szName,pImg->m_szName);
			pImg2->m_pRawPixels=new uchar [pImg2->m_nWidth*pImg2->m_nHeight*(pImg->m_nBpp>>3)];
			pImg2->m_pImgHeader=cvCreateImageHeader(cvSize(pImg2->m_nWidth,pImg2->m_nHeight),8,(pImg2->m_nBpp>>3));
			pImg2->m_pImgHeader->imageData=(char *)(pImg2->m_pRawPixels);

			int nBpp=pImg->m_nBpp>>3;

			for (int y=0;y<pImg->m_nHeight;y++)
			{
				memcpy(&pImg1->m_pRawPixels[y*pImg1->m_nWidth*nBpp],&pImg->m_pRawPixels[y*pImg->m_nWidth*nBpp],pImg1->m_nWidth*nBpp);
				memcpy(&pImg2->m_pRawPixels[y*pImg2->m_nWidth*nBpp],&pImg->m_pRawPixels[(y*pImg->m_nWidth*nBpp)+(pImg1->m_nWidth*nBpp)],pImg2->m_nWidth*nBpp);
			} //y

			pImg1->LoadInVideoMemory(CGTEXT_FLAG_KEEP|CGTEXT_FLAG_NOMIPMAP,NULL);	

			// TODO: add dialog
			ftype fFilmSizew=35.0; // mm
			ftype fFocalLength=28.0; //mm

			pFrame=new CPhotoFrame;
			pFrame->m_nFrameNum=0;
			pFrame->m_pImage=pImg1;
			m_lstFrames.push_back(pFrame);			

			CCamera *pCam=&pFrame->m_Camera;
			pCam->m_fFocalLenght=fFocalLength;
			pCam->m_vPrincipalPoint.Set(pImg1->m_nWidth/2.0,pImg1->m_nHeight/2.0,0);
			ftype fFov=(2.0*(ftype)(atan((fFilmSizew)/(2.0*fFocalLength))));
			//pCam->Init(pFrame->m_pImage->m_nWidth,pFrame->m_pImage->m_nHeight,fFov,200.0,0.15);
			pCam->Init(pFrame->m_pImage->m_nWidth,pFrame->m_pImage->m_nHeight,fFov,100.0,0.15);
			pCam->m_nFrame=pFrame->m_nFrameNum;
			
			m_pVideo->RemoveTexture(pImg);
			
			pFrame=new CPhotoFrame;
			pFrame->m_nFrameNum=1;
			pFrame->m_pImage=pImg2;
			m_lstFrames.push_back(pFrame);

			pCam=&pFrame->m_Camera;
			pCam->m_fFocalLenght=fFocalLength;
			pCam->m_vPrincipalPoint.Set(pImg2->m_nWidth/2.0,pImg2->m_nHeight/2.0,0);
			fFov=(2.0*(ftype)(atan((fFilmSizew)/(2.0*fFocalLength))));
			//pCam->Init(pFrame->m_pImage->m_nWidth,pFrame->m_pImage->m_nHeight,fFov,200.0,0.15);
			pCam->Init(pFrame->m_pImage->m_nWidth,pFrame->m_pImage->m_nHeight,fFov,100.0,0.15);
			pCam->m_nFrame=pFrame->m_nFrameNum;

			break;
		}
		else
		{
#ifdef CREATE_RENDERING_DATA
			if (k==KEY_FRAME)					
				//if (k==1)					
			{	
				//char szTestName[512];
				//sprintf(szTestName,"%s-segmented.tga",szNewFilename);
				//pImg=m_pVideo->LoadTexture(szTestName,CGTEXT_FLAG_KEEP|CGTEXT_FLAG_NOMIPMAP);						
				//strcpy(pImg->m_szName,szNewFilename);
				pImg=m_pVideo->LoadTexture(szNewFilename,CGTEXT_FLAG_KEEP|CGTEXT_FLAG_NOMIPMAP);						
			}
			else
				pImg=m_pVideo->LoadTexture(szNewFilename,CGTEXT_FLAG_DONOTUPLOAD|CGTEXT_FLAG_KEEP);
#else

			pImg=m_pVideo->LoadTexture(szNewFilename,CGTEXT_FLAG_DONOTUPLOAD|CGTEXT_FLAG_KEEP);
#endif

			if (!pImg)
				break;

			pFrame=new CPhotoFrame;
			pFrame->m_nFrameNum=k;
			pFrame->m_pImage=pImg;
		}

		// TODO: add dialog
		ftype fFilmSizew=35.0; // mm
		ftype fFocalLength=28.0; //mm

		CCamera *pCam=&pFrame->m_Camera;
		pCam->m_fFocalLenght=fFocalLength;
		pCam->m_vPrincipalPoint.Set(pImg->m_nWidth/2.0,pImg->m_nHeight/2.0,0);
		ftype fFov=(2.0*(ftype)(atan((fFilmSizew)/(2.0*fFocalLength))));
		//pCam->Init(pFrame->m_pImage->m_nWidth,pFrame->m_pImage->m_nHeight,fFov,200.0,0.15);
		pCam->Init(pFrame->m_pImage->m_nWidth,pFrame->m_pImage->m_nHeight,fFov,100.0,0.15);
		pCam->m_nFrame=pFrame->m_nFrameNum;

		m_lstFrames.push_back(pFrame);
		k++;
		
		if (k>=MAX_IMAGES || m_bUseSingleStereoPhoto)
			break;

	} //1	

	tLoadingBar.Tick(100.0);

	return (m_lstFrames.size()>=2);
}

#define CALIB_DATA_VERSION	18

//////////////////////////////////////////////////////////////////////////
// loading data
bool CPhotoBump10Doc::LoadCalibData(CPaintDC *dc)
{
	if (m_bLoadedFromBojou)
		return (false);

	char szCalibFile[512];
	sprintf(szCalibFile,"%s-CalibData.bin",m_lstFrames[0]->m_pImage->GetName());
	FILE *fp=fopen(szCalibFile,"rb");
	if (!fp)
		return (false);

	CLoadingBar tLoadingBar(m_pVideo,dc,"Loading Calibration Data");

	int nDummy;
	fread(&nDummy,sizeof(nDummy),1,fp);
	if (nDummy!=CALIB_DATA_VERSION)
	{
		fclose(fp);
		return (false);
	}

	fread(&m_fMinZ,sizeof(m_fMinZ),1,fp);
	fread(&m_fMaxZ,sizeof(m_fMaxZ),1,fp);

	MyOutputDebugString("Minz=%f,Maxz=%f \n",m_fMinZ,m_fMaxZ);

	fread(&nDummy,sizeof(nDummy),1,fp);
	if (nDummy!=m_lstFrames.size())
	{
		fclose(fp);
		return (false); // another file 
	}
	
	int nFrames=nDummy;
	MyOutputDebugString("Loading %d frames \n",nFrames);

	for (int k=0;k<nFrames;k++)
	{
		CPhotoFrame *pFrame=m_lstFrames[k];
		if (k==KEY_FRAME)
			//pFrame->m_pImage->SegmentImage(true);
			pFrame->m_pImage->SegmentImage(false);

		fread(&pFrame->m_Camera,sizeof(pFrame->m_Camera),1,fp);
		m_pVideo->SetCamera(&pFrame->m_Camera,false,false);
		if (k<nFrames-1)
		{		
			m_lstFrames[k]->m_FundMatrix=cvCreateMat(3,3,CV_MAT64D);		
			for (int j=0;j<9;j++)
			{
				ftype fVal;
				fread(&fVal,sizeof(ftype),1,fp);
				cvmSet(m_lstFrames[k]->m_FundMatrix,j/3,j%3,fVal);
			} //j
		}

		tLoadingBar.Tick(100.0/(ftype)(nFrames+1));
	} //k

	fread(&nDummy,sizeof(nDummy),1,fp);
	MyOutputDebugString("Loading %d vertices \n",nDummy);
	m_lstVertices.reserve(nDummy);
	m_vMins.SetMax();
	m_vMaxs.SetMin();
	for (int i=0;i<nDummy;i++)
	{
		CPVertex *pVert=new CPVertex(nFrames);
		fread(pVert->m_v2DPos,sizeof(vector3f)*nFrames,1,fp);
		fread(pVert->m_vEpiLines[0],sizeof(vector3f)*nFrames,1,fp);
		fread(pVert->m_vEpiLines[1],sizeof(vector3f)*nFrames,1,fp);
		fread(&pVert->m_vPos,sizeof(vector3f),1,fp);
		m_vMaxs.CheckMax(pVert->m_vPos);
		m_vMins.CheckMin(pVert->m_vPos);
		fread(&pVert->m_vColor,sizeof(vector3f),1,fp);
		m_lstVertices.push_back(pVert);
	}
	fclose(fp);

	// if data was loaded then convert images to grayscale
	// for dense matching
	for (int k1=0;k1<nFrames;k1++)
		m_lstFrames[k1]->m_pImage->GrayScale();

	tLoadingBar.Tick(100.0/(ftype)(nFrames+1));

	return (true);
}

//////////////////////////////////////////////////////////////////////////
// saving data
void CPhotoBump10Doc::SaveCalibData()
{
	int nFrames=(int)(m_lstFrames.size());

	char szCalibFile[512];
	sprintf(szCalibFile,"%s-CalibData.bin",m_lstFrames[0]->m_pImage->GetName());
	FILE *fp=fopen(szCalibFile,"wb");

	int nDummy=CALIB_DATA_VERSION;
	fwrite(&nDummy,sizeof(nDummy),1,fp);

	fwrite(&m_fMinZ,sizeof(m_fMinZ),1,fp);
	fwrite(&m_fMaxZ,sizeof(m_fMaxZ),1,fp);

	nDummy=nFrames;
	fwrite(&nDummy,sizeof(nDummy),1,fp);
	for (int k=0;k<nFrames;k++)
	{
		CPhotoFrame *pFrame=m_lstFrames[k];
		fwrite(&pFrame->m_Camera,sizeof(pFrame->m_Camera),1,fp);
		if (k<nFrames-1)
		{		
			for (int j=0;j<9;j++)
			{			
				ftype fVal=cvmGet(m_lstFrames[k]->m_FundMatrix,j/3,j%3);
				fwrite(&fVal,sizeof(ftype),1,fp);			
			} //j
		}
	}//k
	nDummy=(int)(m_lstVertices.size());
	fwrite(&nDummy,sizeof(nDummy),1,fp);
	for (lstPhotoVerticesIt i=m_lstVertices.begin();i!=m_lstVertices.end();i++)
	{
		CPVertex *pVert=(*i);
		fwrite(pVert->m_v2DPos,sizeof(vector3f)*nFrames,1,fp);
		fwrite(pVert->m_vEpiLines[0],sizeof(vector3f)*nFrames,1,fp);
		fwrite(pVert->m_vEpiLines[1],sizeof(vector3f)*nFrames,1,fp);
		fwrite(&pVert->m_vPos,sizeof(vector3f),1,fp);
		fwrite(&pVert->m_vColor,sizeof(vector3f),1,fp);
	}
	fclose(fp);
}

//////////////////////////////////////////////////////////////////////////
bool CPhotoBump10Doc::Load3DPoints(CPhotoFrame *pFrame)
{
	char szFilename[1024];
	sprintf(szFilename,"%s-3DPoints.bin",pFrame->m_pImage->m_szName);

	int w=pFrame->m_pImage->m_nWidth;
	int h=pFrame->m_pImage->m_nHeight;

	CCamera *pCam=&pFrame->m_Camera;

	FILE *fp=fopen(szFilename,"rb");
	if (fp)
	{		
		int nDummy;
		fread(&nDummy,sizeof(nDummy),1,fp);
		if (nDummy!=CALIB_DATA_VERSION)
		{
			fclose(fp);
			return (false);
		}

		int w1,h1;
		fread(&w1,sizeof(int),1,fp);
		fread(&h1,sizeof(int),1,fp);
		if (w1==w && h1==h)
		{
			if (!pFrame->m_pBestDepth)
				pFrame->m_pBestDepth=new ftype [w*h];		
			if (!pFrame->m_pPoints)
				pFrame->m_pPoints=new vector3f [w*h];
			//fread(pFrame->m_pPoints,w*h*sizeof(vector3f),1,fp);
			fread(pFrame->m_pBestDepth,w*h*sizeof(ftype),1,fp);
			fclose(fp);
			for (int k=0;k<w*h;k++)			
			{
				int y=k/w;
				int x=k-(y*w);
				m_pVideo->UnProjectFromScreen(vector3f(x,y,pFrame->m_pBestDepth[k]),pFrame->m_pPoints[k],pCam);
				pFrame->m_pPoints[k].w=pFrame->m_pBestDepth[k];			
			} //k
			return(true);			
		}
		fclose(fp);
	}
	return (false);
}

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10Doc::Save3DPoints(CPhotoFrame *pFrame)
{
	char szFilename[1024];
	sprintf(szFilename,"%s-3DPoints.bin",pFrame->m_pImage->m_szName);

	int w=pFrame->m_pImage->m_nWidth;
	int h=pFrame->m_pImage->m_nHeight;

	FILE *fp=fopen(szFilename,"wb");
	if (fp)
	{		
		int nDummy=CALIB_DATA_VERSION;
		fwrite(&nDummy,sizeof(nDummy),1,fp);

		int w1=w,h1=h;
		fwrite(&w1,sizeof(int),1,fp);
		fwrite(&h1,sizeof(int),1,fp);
		//fwrite(pFrame->m_pPoints,w*h*sizeof(vector3f),1,fp);
		fwrite(pFrame->m_pBestDepth,w*h*sizeof(ftype),1,fp);
		fclose(fp);
	}	
}
