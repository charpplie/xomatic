
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

#define MAX_BOJOU_IMAGES 2

//////////////////////////////////////////////////////////////////////////
bool CPhotoBump10Doc::LoadBojouCalibrationData(const char *_szFilename,CPaintDC *dc)
{
	uint nSize;
	char *_pData=m_pUtils->LoadInMemory(_szFilename,nSize,true);
	if (!_pData)
		return (false);

	const char *pData=_pData;
	char szFilename[1024];
	szFilename[0]=0;
	int nFrameStart=-1,nFrameEnd=-1;
	bool bFramesLoaded=false,bCamerasLoaded=false;

	lstFrames lstDummyFrames;

	char	szPath[512];
	strcpy(szPath,m_pUtils->GetPath(_szFilename));

	//////////////////////////////////////////////////////////////////////////
	// load pictures and frames
	while (1)
	{		
		const char *pRes=m_pUtils->GetToken(pData);
		if (!pRes)
			break;
		if (stricmp(pRes,"sequence")==0)
		{
			if (szFilename[0]!=0)
				continue;

			for (int k=0;k<3;k++)
				m_pUtils->GetToken(pData); // skip "file name was"

			pRes=m_pUtils->GetToken(pData); // gets filename
			strcpy(szFilename,pRes);
		}

		if (stricmp(pRes,"frame")==0)
		{								
			if (nFrameStart>=0 && nFrameEnd>=0)
				continue;

			pRes=m_pUtils->GetToken(pData); // gets frame num
			if (nFrameStart==-1)
				nFrameStart=atoi(pRes);
			else
			{
				nFrameEnd=atoi(pRes);
				//m_lstFrames.reserve(nFrameEnd-nFrameStart+1);				
				m_lstFrames.reserve(MAX_BOJOU_IMAGES);				
				for (int k=nFrameStart;k<=nFrameEnd;k++)
				{
					CPhotoFrame *pFrame=new CPhotoFrame;
					pFrame->m_nFrameNum=k-nFrameStart;
					char szNewFilename[1024];
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
					int nNum=atoi(szNumber)+(k-nFrameStart); // increase frame num
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
					// necessary to keep photometric invariance? try it
					//pFrame->m_pImage->ConvertToGrayScale();

					pFrame->m_pImage=pImg;

					if (!pFrame->m_pImage)
					{
						char szMsg[1024];
						sprintf(szMsg,"Image %s not found\nApplication will now quit",szNewFilename);
						AfxMessageBox(szMsg);
						exit(1);
					}

					if (pFrame->m_nFrameNum<MAX_BOJOU_IMAGES)
						m_lstFrames.push_back(pFrame);
					else
						lstDummyFrames.push_back(pFrame);
				} //k

				// skip till camera's datas
				while (1)
				{		
					const char *pRes=m_pUtils->GetToken(pData);
					if (!pRes || (stricmp(pRes,"#The")==0))
						break;
				}
				bFramesLoaded=true;
				break;
			}
		}
	} // 1

	//////////////////////////////////////////////////////////////////////////
	// load cameras data
	int		k,nWidth=0,nHeight=0;
	ftype	fFilmSizew,fFilmSizeh,fFocalLength;
	while (1)
	{		
		const char *pRes=m_pUtils->GetToken(pData);
		if (!pRes)
			break;
		if (stricmp(pRes,"#Image")==0) // image size
		{
			m_pUtils->GetToken(pData); // skip 'size'
			nWidth=atoi(m_pUtils->GetToken(pData));
			nHeight=atoi(m_pUtils->GetToken(pData));
		}

		if (stricmp(pRes,"#Filmback")==0) // filmback size
		{
			m_pUtils->GetToken(pData); // skip 'size'
			fFilmSizew=(ftype)(atof(m_pUtils->GetToken(pData)));			
			fFilmSizeh=(ftype)(atof(m_pUtils->GetToken(pData)));
			// skip the next 3 lines
			for (k=0;k<3;k++)
			{
				pRes=m_pUtils->GetToken(pData); 
				while (!strstr(pRes,"#"))				
					pRes=m_pUtils->GetToken(pData); 
			} //k
			// skip last line, till (mm)
			pRes=m_pUtils->GetToken(pData); 
			while (!strstr(pRes,"(mm)"))				
				pRes=m_pUtils->GetToken(pData); 

			// load the camera frames
			for (k=0;k<(nFrameEnd-nFrameStart)+1;k++)
			{			
				int i,j;

				CCamera *pCam=NULL;
				if (k<MAX_BOJOU_IMAGES)
					pCam=&m_lstFrames[k]->m_Camera;
				else
					pCam=&lstDummyFrames[k-MAX_BOJOU_IMAGES]->m_Camera;

				pCam->m_bLoadedFromBojou=true;

				// rotation matrix
				CMatrixf matrix;
				matrix.Identity();
				ftype mat[3][3];
				for (j=0;j<3;j++)
					for (i=0;i<3;i++)
					{
						//matrix.m_Tvalues[j][i]=(ftype)(atof(m_pUtils->GetToken(pData)));
						matrix.m_Tvalues[i][j]=(ftype)(atof(m_pUtils->GetToken(pData)));
						//mat[j][i]=matrix.m_Tvalues[j][i];						
						mat[i][j]=matrix.m_Tvalues[i][j];						
					}

					// position
					vector3f vPos;
					for (j=0;j<3;j++)
					{
						vPos[j]=(ftype)(atof(m_pUtils->GetToken(pData))); 
					}

					// focal length
					fFocalLength=(ftype)(atof(m_pUtils->GetToken(pData))); 

					// calc fov
					//ftype fFov=(2.0*(ftype)(atan((43.0)/(2.0*fFocalLength))));
					ftype fFov=(2.0*(ftype)(atan((fFilmSizew)/(2.0*fFocalLength))));

					// init camera
					pCam->Init(nWidth,nHeight,fFov,100.0,0.15);
					//pCam->Init(nWidth,nHeight,fFov,200.0,0.05f);
					pCam->SetPos(vPos);	
					pCam->m_nFrame=k;

					pCam->m_nPivot=0;

					pCam->m_Matrix=matrix;
					pCam->m_OrigMatrix=matrix;
			} //k

			bCamerasLoaded=true;

			// skip till 3d points data
			pRes=m_pUtils->GetToken(pData); 
			while (!strstr(pRes,"z"))				
				pRes=m_pUtils->GetToken(pData); 
			break;
		}
	} // 1

	//////////////////////////////////////////////////////////////////////////
	// load 3d points data	

	// to set for cameras perspective
	m_bLoadedFromBojou=true;

	int nFrames=(int)(m_lstFrames.size());

	m_vMins.SetMax();
	m_vMaxs.SetMin();

	bool bEOF=false;
	while (1)
	{
		vector3f vPos;

		for (k=0;k<3;k++)
		{		
			const char *pRes=m_pUtils->GetToken(pData);
			if (!pRes || pRes[0]=='#')
			{
				bEOF=true;
				break;
			}
			vPos[k]=(atof(pRes));
		} //k

		if (bEOF)
			break;

		// each feature must be present X times, once for each view
		//CPhotoVertex *pVert=new CPhotoVertex((nFrameEnd-nFrameStart)+1);		
		CPVertex *pVert=new CPVertex(nFrames);		
		pVert->m_vPos=vPos;		

		// project to get 2d pos
		for (int k1=0;k1<nFrames;k1++)
		{
			CCamera *pCam=&m_lstFrames[k1]->m_Camera;
			m_pVideo->SetCamera(pCam,false,false);
			m_pVideo->ProjectToScreen(pVert->m_vPos,pVert->m_v2DPos[k1],pCam);					
			pVert->m_v2DPos[k1].z=1.0f;
		}

		m_vMaxs.CheckMax(pVert->m_vPos);
		m_vMins.CheckMin(pVert->m_vPos);

		m_lstVertices.push_back(pVert);
	} //1

	SAFE_DELETE_ARRAY(_pData);

	m_nNumFeatures=(int)(m_lstVertices.size());

	// delete excessive frames
	for (lstFramesIt i=lstDummyFrames.begin();i!=lstDummyFrames.end();i++)
	{
		CPhotoFrame *pFrame=(*i);
		delete pFrame;
	} //i

	if (bFramesLoaded && bCamerasLoaded && m_lstFrames.size()>=2)
		m_bLoadedFromBojou=true;
	else
		m_bLoadedFromBojou=false;

	return (m_bLoadedFromBojou);
}