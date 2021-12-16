
#include "StdAfx.h"
#include "photoimage.h"
#include "FreeImage.h" 
#include "Video.h"
#include "2passscale.h"

#include "opencv/cv.h"
#include "opencv/highgui.h"
#include <direct.h>

//////////////////////////////////////////////////////////////////////////
CPhotoImage::CPhotoImage(CVideo *pVideo)
{
	m_pVideo=pVideo;
	m_nID=0;
	m_nWidth=m_nHeight=0;
	m_nGLWidth=m_nGLHeight=0;
	memset(m_szName,0,sizeof(m_szName));		
	m_pRawPixels=NULL;
	m_pImgHeader=NULL;
	m_pImageProcessing=new CImageProcessing;	
	m_vNormals=NULL;
	m_pFloatPixels=NULL;
	m_pEdges=NULL;
}

//////////////////////////////////////////////////////////////////////////
CPhotoImage::~CPhotoImage(void)
{
	m_pVideo->RemoveTexture(this);
	
	if (m_nID>0)	
	{
		glDeleteTextures(1,&m_nID);		
		m_nID=0; // texture id=0 for the sake of clarity
	}	

	SAFE_DELETE_ARRAY(m_pRawPixels);
	if (m_pImgHeader)
		cvReleaseImageHeader(&m_pImgHeader);
	SAFE_DELETE(m_pImageProcessing);

	SAFE_DELETE_ARRAY(m_vNormals);
	SAFE_DELETE_ARRAY(m_pFloatPixels);
	SAFE_DELETE_ARRAY(m_pEdges);
}

/////////////////////////////////////////////////////////////////
bool CPhotoImage::Load(const char *_szFilename,unsigned int dwFlags,const char *szExportName)
{
	char szFilename[512];
	strcpy(szFilename,_szFilename);

	if (dwFlags & CGTEXT_USE_WORKING_DIR)
		sprintf(szFilename,"%s\\%s",m_pVideo->GetWorkingDir(),_szFilename);

	FREE_IMAGE_FORMAT fif = FreeImage_GetFIFFromFilename(szFilename);
  if (fif==FIF_UNKNOWN )
		return (false);

	FIBITMAP *handleFI=NULL;

	if (fif==FIF_JPEG)
		handleFI = FreeImage_Load(fif, szFilename,JPEG_ACCURATE);
	else
		handleFI = FreeImage_Load(fif, szFilename);

	if (!handleFI)
		return (false);

	// get the size of our image
	m_nWidth = FreeImage_GetWidth(handleFI);
	m_nHeight = FreeImage_GetHeight(handleFI);
	m_nBpp = FreeImage_GetBPP(handleFI);

	int nSize=m_nWidth *m_nHeight*(m_nBpp>>3);
	m_pRawPixels=new unsigned char[nSize];
	//FreeImage_ConvertToRawBits(m_pRawPixels, m_handleFI, m_nWidth, m_nBpp, 0,0,0);
	unsigned char *pBits=FreeImage_GetBits(handleFI);
	memcpy(m_pRawPixels,pBits,nSize);
	FlipAndSwap(m_pRawPixels);
	FreeImage_Unload(handleFI);	

	m_pImgHeader=cvCreateImageHeader(cvSize(m_nWidth,m_nHeight),8,(m_nBpp>>3));
	m_pImgHeader->imageData=(char *)(m_pRawPixels);

	// save the szFilename
	strcpy(m_szName,_szFilename);

	//m_pVideo->CheckError("Before Before Loading in video memory");
		
	return (LoadInVideoMemory(dwFlags,szExportName));	

	return (true);
}

//////////////////////////////////////////////////////////////////////////
void CPhotoImage::FlipAndSwap(unsigned char *pMem)
{
	//Microsoft Bitmap sux...swap rgba and flip image
	unsigned char r,g,b,a;
	unsigned char *source=pMem;

	for (int k=0;k<m_nWidth;k++)
	{		
		for (int j=0;j<m_nHeight;j++)
		{
			if (m_nBpp==32) 
			{	 			
				r=source[0];
				b=source[1];
				g=source[2];
				a=source[3];
				source[0]=r;
				source[1]=g;
				source[2]=b;
				source[3]=a;
				source+=4;
			}
			else
			{								
				b=source[0];
				g=source[1];
				r=source[2];
				source[0]=r;
				source[1]=g;
				source[2]=b;				
				source+=3;
			}			
		} //j
	} //k

	int linesize=m_nWidth*(m_nBpp>>3);
	unsigned char *tempbuf=new unsigned char [linesize];	
	unsigned char *linesource=pMem;
	unsigned char *linedest=pMem+((m_nHeight-1)*m_nWidth)*(m_nBpp>>3);

	for (int k=0;k<m_nHeight/2;k++)	
	{
		memcpy(tempbuf,linesource,linesize); //save source in tempbuf
		memcpy(linesource,linedest,linesize); //copy dest in source
		memcpy(linedest,tempbuf,linesize); //copy tempbuf in dest
		linesource+=linesize;
		linedest-=linesize;
	}

	SAFE_DELETE_ARRAY(tempbuf);	
}

//////////////////////////////////////////////////////////////////////////
bool CPhotoImage::LoadInVideoMemory(unsigned int dwFlags,const char *szExportName)
{	
	if (!(dwFlags & CGTEXT_FLAG_DONOTUPLOAD))
	{
		//Convert the image into an OpenGL texture 
		m_pVideo->CheckError("Before Loading in video memory");

		//int nWidth=FreeImage_GetPitch(m_handleFI)/(m_nBpp>>3);
		// Use the surface width and height expanded to powers of 2 
		m_nGLWidth = Findpow2Dim(m_nWidth);
		m_nGLHeight = Findpow2Dim(m_nHeight);

		if (m_nGLWidth>2048)
			m_nGLWidth=2048;

		if (m_nGLHeight>2048)
			m_nGLHeight=2048;

		unsigned char *pCurrMem = new unsigned char[m_nWidth*m_nHeight*(m_nBpp>>3)];
		//int nPitch=FreeImage_GetPitch(m_handleFI);
		//ASSERT(nPitch==(nWidth*(m_nBpp>>3)));
		//FreeImage_ConvertToRawBits(pCurrMem, m_handleFI, nWidth*(m_nBpp>>3), m_nBpp, 0,0,0);

		memcpy(pCurrMem,m_pRawPixels,m_nWidth*m_nHeight*(m_nBpp>>3));

		/*
		unsigned char cTemp;
		// swap colors since the freeimage mask doesn't work		
		if (m_nBpp==24)
		{		
			for (int k=0;k<nWidth*m_nHeight*3;k+=3)
			{
				cTemp=pCurrMem[k+0];
				pCurrMem[k+0]=pCurrMem[k+2];
				pCurrMem[k+2]=cTemp;
			} //k
		}
		*/

		//m_fGLWScale=((ftype)(m_nGLWidth)/(ftype)(m_nWidth))/(ftype)(m_nGLWidth);
		//m_fGLHScale=((ftype)(m_nGLHeight)/(ftype)(m_nHeight))/(ftype)(m_nGLHeight);

		if (m_nGLWidth!=m_nWidth || m_nGLHeight!=m_nHeight)
		{		
			unsigned char *pDestMem=new unsigned char [m_nGLWidth*m_nGLHeight*m_nBpp>>3];

			switch (m_nBpp) 
			{
				case 24:
					{					
					//gluScaleImage(GL_RGB, m_nWidth, m_nHeight,GL_UNSIGNED_BYTE, pCurrMem,
					//m_nGLWidth, m_nGLHeight,GL_UNSIGNED_BYTE, pDestMem);
					//::Resize(pCurrMem,pDestMem,m_nWidth,m_nHeight,m_nGLWidth, m_nGLHeight,m_nBpp);

					//C2PassScale <CBilinearFilter> ScaleEngine;			
					//ScaleEngine.Scale(pCurrMem,m_nWidth,m_nHeight,pDestMem,m_nGLWidth,m_nGLHeight,m_nBpp);					
					IplImage *pTemp=cvCreateImageHeader(cvSize(m_nWidth,m_nHeight),8,m_nBpp>>3);
					pTemp->imageData=(char *)(pCurrMem);
					IplImage *pDest=cvCreateImageHeader(cvSize(m_nGLWidth,m_nGLHeight),8,m_nBpp>>3);
					pDest->imageData=(char *)(pDestMem);
					cvResize(pTemp,pDest);
					cvReleaseImageHeader(&pTemp);
					cvReleaseImageHeader(&pDest);

					if (szExportName)
					{
						// Cannot USE GLGETIMAGE IT CORRUPTS MEMORY!						
						uchar *pixels=new uchar [m_nGLWidth*m_nGLHeight*(m_nBpp>>3)];
						memcpy(pixels,pDestMem,m_nGLWidth*m_nGLHeight*(m_nBpp>>3));
						SaveTga(pixels,3,m_nGLWidth,m_nGLHeight,szExportName,true);						
						delete [] pixels;
					}
					}
					break;

				case 32:
					gluScaleImage(GL_RGBA, m_nWidth, m_nHeight,GL_UNSIGNED_BYTE, pCurrMem,
					m_nGLWidth, m_nGLHeight,GL_UNSIGNED_BYTE, pDestMem);
					break;

				case 8:
					gluScaleImage(GL_ALPHA, m_nWidth, m_nHeight,GL_UNSIGNED_BYTE, pCurrMem,
					m_nGLWidth, m_nGLHeight,GL_UNSIGNED_BYTE, pDestMem);
					break;

				default: //wrong bpp
					MyOutputDebugString("<CTEXTURE> Image %s, wrong bpp (%d) \n",m_szName,m_nBpp);
					
					SAFE_DELETE_ARRAY(pCurrMem);	
					SAFE_DELETE_ARRAY(pDestMem);	
																		
					return (false);
			}

			SAFE_DELETE_ARRAY(pCurrMem);
			pCurrMem=pDestMem;
		}
			
		// Create an OpenGL texture for the image 		
		glGenTextures(1, &m_nID);
		glBindTexture(GL_TEXTURE_2D, m_nID);

		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		if (!(dwFlags & CGTEXT_FLAG_NOMIPMAP))
		{
			//Trilinear filter by default
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
								
			switch (m_nBpp) 
			{
				case 24:
					glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
					gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGB,m_nGLWidth,m_nGLHeight,GL_RGB,GL_UNSIGNED_BYTE,pCurrMem);
					break;

				case 32:
					glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
					gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA,m_nGLWidth,m_nGLHeight,GL_RGBA,GL_UNSIGNED_BYTE,pCurrMem);
					break;

				case 8:
					glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE, GL_MODULATE);
					gluBuild2DMipmaps(GL_TEXTURE_2D,GL_ALPHA,m_nGLWidth,m_nGLHeight,GL_ALPHA,GL_UNSIGNED_BYTE,pCurrMem);
					break;

				default: //wrong bpp
					MyOutputDebugString("<CTEXTURE> Image %s, wrong bpp (%d) \n",m_szName,m_nBpp);					
					SAFE_DELETE_ARRAY(pCurrMem);																			
					return (false);
			}
		}
		else
		{

			switch (m_nBpp) 
			{
				case 24:
					glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
					glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,m_nGLWidth,m_nGLHeight,0,GL_RGB,GL_UNSIGNED_BYTE,pCurrMem);
					break;

				case 32:
					glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
					glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,m_nGLWidth,m_nGLHeight,0,GL_RGBA,GL_UNSIGNED_BYTE,pCurrMem);
					break;

				case 8:
					glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE, GL_MODULATE);
					glTexImage2D(GL_TEXTURE_2D,0,GL_ALPHA,m_nGLWidth,m_nGLHeight,0,GL_ALPHA,GL_UNSIGNED_BYTE,pCurrMem);
					break;

				default: //wrong bpp
					MyOutputDebugString("<CTEXTURE> Image %s, wrong bpp (%d) \n",m_szName,m_nBpp);					
					SAFE_DELETE_ARRAY(pCurrMem);																			
					return(false);								
			}

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
		
		SAFE_DELETE_ARRAY(pCurrMem);	
																	
		m_pVideo->CheckError("After Loading in video memory");
	}

	if (!(dwFlags & CGTEXT_FLAG_KEEP))
	{		
		// If we don't need the original image anymore...
		SAFE_DELETE_ARRAY(m_pRawPixels);
	}
	
	return (true);
}

//////////////////////////////////////////////////////////////////////////
CPhotoImage *CPhotoImage::Scale(int nScale)
{
	CPhotoImage *pNew=new CPhotoImage(m_pVideo);	
	int wlen=m_nWidth/nScale;
	int hlen=m_nHeight/nScale;
	pNew->m_nBpp=m_nBpp;
	pNew->m_nWidth=wlen;
	pNew->m_nHeight=hlen;
	strcpy(pNew->m_szName,m_szName);
	pNew->m_pRawPixels=new uchar [wlen*hlen*(m_nBpp>>3)];

	IplImage *pImgHeader=cvCreateImageHeader(cvSize(wlen,hlen),8,(m_nBpp>>3));
	pImgHeader->imageData=(char *)(pNew->m_pRawPixels);

	cvResize(m_pImgHeader,pImgHeader);		

	memcpy(pNew->m_pRawPixels,pImgHeader->imageData,wlen*hlen*(m_nBpp>>3));

	cvReleaseImageHeader(&pImgHeader);

	return (pNew);
}

//////////////////////////////////////////////////////////////////////////
void	Resize(uchar *src,uchar *dst,int w,int h,int nw,int nh,int nBpp)
{
	IplImage *pImgHeaderSrc=cvCreateImageHeader(cvSize(w,h),8,(nBpp>>3));
	IplImage *pImgHeaderDst=cvCreateImageHeader(cvSize(nw,nh),8,(nBpp>>3));
	pImgHeaderSrc->imageData=pImgHeaderSrc->imageDataOrigin=(char *)(src);
	pImgHeaderDst->imageData=pImgHeaderSrc->imageDataOrigin=(char *)(dst);

	cvResize(pImgHeaderSrc,pImgHeaderDst);		

	//memcpy(pNew->m_pRawPixels,pImgHeader->imageData,wlen*hlen*(m_nBpp>>3));

	cvReleaseImageHeader(&pImgHeaderSrc);
	cvReleaseImageHeader(&pImgHeaderDst);
}

//////////////////////////////////////////////////////////////////////////
CPhotoImage *CPhotoImage::Resize(int wlen,int hlen,int nBpp,bool bConvertToFloat)
{
	CPhotoImage *pNew=new CPhotoImage(m_pVideo);		
	pNew->m_nWidth=wlen;
	pNew->m_nHeight=hlen;
	strcpy(pNew->m_szName,m_szName);

	IplImage *pImgHeader=NULL;

	if (bConvertToFloat)
	{
		pNew->m_nBpp=32;
		pNew->m_pFloatPixels=new float [wlen*hlen];
		pImgHeader=cvCreateImageHeader(cvSize(wlen,hlen),32,1);
		pImgHeader->imageData=(char *)(pNew->m_pFloatPixels);
		if (wlen==m_nWidth && hlen==m_nHeight)
		{
			// just copy
			if (!m_pFloatPixels)
			{
				for (int k=0;k<wlen*hlen;k++)
					pNew->m_pFloatPixels[k]=m_pRawPixels[k];
			}
			else
				memcpy(pNew->m_pFloatPixels,m_pFloatPixels,wlen*hlen*sizeof(float));
		}
		else		
		{
			ftype xRatio=(ftype)(m_nWidth)/(ftype)(wlen);
			ftype yRatio=(ftype)(m_nHeight)/(ftype)(hlen);
			if (m_pFloatPixels)
			{			
				// very simple resizing no filtering
				for (int y = 0 ; y < hlen ; ++y) 				
					for (int x = 0 ; x < wlen ; ++x) 
					{
						pNew->m_pFloatPixels[y*wlen+x]=m_pFloatPixels[(int)(y*yRatio)*m_nWidth+(int)(x*xRatio)];
					} //x				
			}
			else
			{			
				// very simple resizing no filtering
				for (int y = 0 ; y < hlen ; ++y) 				
					for (int x = 0 ; x < wlen ; ++x) 
					{
						pNew->m_pFloatPixels[y*wlen+x]=m_pRawPixels[(int)(y*yRatio)*m_nWidth+(int)(x*xRatio)];
					} //x				
			}
		}
	}
	else
	{
		pNew->m_nBpp=nBpp;
		pNew->m_pRawPixels=new uchar [wlen*hlen*(nBpp>>3)];
		pImgHeader=cvCreateImageHeader(cvSize(wlen,hlen),8,(nBpp>>3));
		pImgHeader->imageData=(char *)(pNew->m_pRawPixels);
		if (m_nBpp==nBpp)
			cvResize(m_pImgHeader,pImgHeader);		
		else
			memset(pNew->m_pRawPixels,0,wlen*hlen*(nBpp>>3));
	}
		
	pNew->m_pImgHeader=pImgHeader;
	
	return (pNew);
}

//////////////////////////////////////////////////////////////////////////
void CPhotoImage::ExtractEdges(bool bSaveTga,bool bAllocateNormals,bool bSmoothImage)
{

	if (m_vNormals && bAllocateNormals)
		return;

	// first "semplify" the image with a bilateral filtering						
	// to remove all small residual crap inside the image

	uchar *pSrc=m_pRawPixels;

	IplImage *pTemp=NULL;
	if (bSmoothImage)
	{	
		pTemp=cvCloneImage(m_pImgHeader);		
		cvSmooth(m_pImgHeader,pTemp,CV_BILATERAL,50,3);				
		cvCanny(pTemp,pTemp,120,60,3);				
		pSrc=(uchar *)pTemp->imageData;
	}

	if (!m_pEdges)
		m_pEdges=new unsigned char[m_nWidth*m_nHeight];
	memset(m_pEdges,0,m_nWidth*m_nHeight);
	uchar *pEdges=NULL;
	if (bSaveTga)
	{	
		pEdges=new unsigned char[m_nWidth*m_nHeight*3];
		for (int k=0;k<m_nWidth*m_nHeight;k++)
		{
			pEdges[k*3+0]=pEdges[k*3+1]=pEdges[k*3+2]=pSrc[k];		
		}
	}	

	int w=m_nWidth;
	int h=m_nHeight;

	//SAFE_DELETE_ARRAY(m_vNormals);

	if (bAllocateNormals)	
	{
		if (!m_vNormals)
			m_vNormals=new vector3float [w*h];
	}
	else
		SAFE_DELETE_ARRAY(m_vNormals);
	
	//memset(m_vNormals,0,sizeof(float)*w*h);

	int tempdirs[3][2]={{0,0},{1,0},{0,1}};
	for (int y=1;y<m_nHeight-2;y++)
	{
		for (int x=1;x<m_nWidth-2;x++)
		{
			//if (pEdges[y*w*3+x*3+0]==0)
			if (pSrc[y*w+x]==0)
			{				
				if (bAllocateNormals)
					m_vNormals[y*w+x].Clear();
				continue;
			}

			m_pEdges[y*w+x]=1;
			if (!bAllocateNormals)
				continue;

			ftype fC[3];

			//ftype fC=m_pRawPixels[y*w+x];
			//ftype fCx=m_pRawPixels[y*w+x+1];
			//ftype fCy=m_pRawPixels[(y+1)*w+x];

			for (int k=0;k<3;k++)
			{
				int x1=x+tempdirs[k][0];
				int y1=y+tempdirs[k][1];
				
				ftype fSum=0;
				for (int k1=0;k1<9;k1++)
				{
					int x2=x1+dirs[k1][0];
					int y2=y1+dirs[k1][1];
					fSum+=m_pRawPixels[y2*w+x2];
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

			if (bAllocateNormals)
				m_vNormals[y*w+x].Set((float)(nx),(float)(ny),(float)(nz));

			if (bSaveTga)
			{			
				// Repack the normalized vector into an RGB unsigned byte
				// vector in the normal map image 
				pEdges[y*w*3+x*3+0]=(uchar) (128.0 + 127.0*nx);
				pEdges[y*w*3+x*3+1]=(uchar) (128.0 + 127.0*ny);
				pEdges[y*w*3+x*3+2]=(uchar) (128.0 + 127.0*nz);
			}
		}//x
	} //y

	//SAFE_DELETE_ARRAY(m_vNormals);

	if (bSaveTga)
	{
		char szFilename[1024];
		sprintf(szFilename,"%s-Edges.tga",m_szName);
		//SaveTga((uchar *)(pTemp->imageData),(m_nBpp>>3),m_nWidth,m_nHeight,szFilename,true);
		SaveTga(pEdges,3,m_nWidth,m_nHeight,szFilename,true);		
		SAFE_DELETE_ARRAY(pEdges);
	}
	if (pTemp)
		cvReleaseImage(&pTemp);
}

//////////////////////////////////////////////////////////////////////////
CPhotoImage *CPhotoImage::GaussianScale(int nScale)
{
	CPhotoImage *pNew=new CPhotoImage(m_pVideo);	

	pNew->m_nBpp=m_nBpp;
	strcpy(pNew->m_szName,m_szName);

	int wlen=m_nWidth;
	int hlen=m_nHeight;	

	IplImage *pImgSrc=cvCreateImage(cvSize(wlen,hlen),8,(m_nBpp>>3));
	memcpy(pImgSrc->imageData,m_pRawPixels,m_nWidth*m_nHeight*(m_nBpp>>3));
	
	int oldScale=nScale;
	int wlendest=m_nWidth;
	nScale=0;
	while (wlendest>(m_nWidth/oldScale))
	{
		nScale++;
		wlendest>>=1;
	}

	for (int k=0;k<nScale;k++)
	{
		wlen>>=1;
		hlen>>=1;
		ASSERT(wlen>=0);
		ASSERT(hlen>=0);

		IplImage *pImgDst=cvCreateImage(cvSize(wlen,hlen),8,(m_nBpp>>3));

		cvPyrDown(pImgSrc,pImgDst);
		
		cvReleaseImage(&pImgSrc);
		pImgSrc=pImgDst;
	} //k

	ASSERT(wlen==m_nWidth/oldScale);
	ASSERT(hlen==m_nHeight/oldScale);
	
	pNew->m_nWidth=wlen;
	pNew->m_nHeight=hlen;	
	pNew->m_pRawPixels=new uchar [wlen*hlen*(m_nBpp>>3)];
	memcpy(pNew->m_pRawPixels,pImgSrc->imageData,wlen*hlen*(m_nBpp>>3));

	cvReleaseImage(&pImgSrc);

	return (pNew);
}

//////////////////////////////////////////////////////////////////////////
void CPhotoImage::DrawPoint(int x,int y,int nSize,uchar r,uchar g,uchar b,uchar *pDest,int _nBpp)
{
	uchar *pSrc=m_pRawPixels;
	int nBpp=m_nBpp;
	if (pDest)
	{
		pSrc=pDest;
		nBpp=_nBpp;
	}
	for (int j=-nSize>>1;j<=nSize>>1;j++)
	for (int i=-nSize>>1;i<=nSize>>1;i++)
	{						
		int nx=x+i;
		int ny=y+j;		
		if (nx<0 || nx>(m_nWidth-1))
			continue;
		if (ny<0 || ny>(m_nHeight-1))
			continue;
		if (nBpp==24)
		{		
			pSrc[ny*m_nWidth*3+nx*3+0]=r;
			pSrc[ny*m_nWidth*3+nx*3+1]=g;
			pSrc[ny*m_nWidth*3+nx*3+2]=b;
		}
		else
			m_pRawPixels[ny*m_nWidth+nx]=(r+g+b)/3;
	}
}

//////////////////////////////////////////////////////////////////////////
void CPhotoImage::DrawLine(vector3f v1,vector3f v2,vector3f c1,vector3f c2)
{
	if (v1.y<0 && v2.y<0)
		return;
	if (v1.y>=m_nHeight && v2.y>=m_nHeight)
		return;
	if (m_nBpp==8)
	cvLine(m_pImgHeader,cvPoint((int)(v1.x+ROUNDING_VAL),(int)(v1.y+ROUNDING_VAL)),
					cvPoint((int)(v2.x+ROUNDING_VAL),(int)(v2.y+ROUNDING_VAL)),c1.Length());
	else
	cvLine(m_pImgHeader,cvPoint((int)(v1.x+ROUNDING_VAL),(int)(v1.y+ROUNDING_VAL)),
					cvPoint((int)(v2.x+ROUNDING_VAL),(int)(v2.y+ROUNDING_VAL)),CV_RGB((int)(c1.x),(int)(c1.y),(int)(c1.z)));
}

//////////////////////////////////////////////////////////////////////////
void CPhotoImage::Smooth(ftype *pImage,int w,int h,int nIteration)
{
	for (int j=0;j<nIteration;j++)
	for (int y=0;y<h;y++)
	{
		for (int x=0;x<w;x++)
		{
			ftype fSum=0;
			ftype fArea=0;
			for (int k=0;k<9;k++)
			{
				int nX1=x+dirs[k][0];
				int nY1=y+dirs[k][1];
				if (nX1<0 || nX1>=w || nY1<0 || nY1>=h)				
					continue;

				fSum+=pImage[nY1*w+nX1];
				fArea+=1.0;								
			} //k
			pImage[y*w+x]=fSum/fArea;
		} //x
	} //y
}

//////////////////////////////////////////////////////////////////////////
#pragma pack (push)
#pragma pack (1)
typedef struct
{
  unsigned char  id_length, colormap_type, image_type;
  unsigned short colormap_index, colormap_length;
  unsigned char  colormap_size;
  unsigned short x_origin, y_origin, width, height;
  unsigned char  pixel_size, attributes;
} TargaHeader_t;
#pragma pack (pop)

//////////////////////////////////////////////////////////////////////////
bool SaveTga(unsigned char *sourcedata,int sourceformat,int w,int h,const char *filename,bool flip)
{
	if (flip)
	{
		int size=w*(sourceformat);
		unsigned char *tempw=new unsigned char [size];
		unsigned char *src1=sourcedata;		
		unsigned char *src2=sourcedata+(w*(sourceformat))*(h-1);
		for (int k=0;k<h/2;k++)
		{
			memcpy(tempw,src1,size);
			memcpy(src1,src2,size);
			memcpy(src2,tempw,size);
			src1+=size;
			src2-=size;
		}
		delete [] tempw;
	}

	TargaHeader_t header;

	memset(&header, 0, sizeof(header));
	header.image_type = 2;
	header.width = w;
	header.height = h;
	if (sourceformat==4)
		header.pixel_size = 32;
	else
		header.pixel_size = 24;
  	
	unsigned char *data = new unsigned char[w*h*(header.pixel_size>>3)];
	unsigned char *dest = data;
	unsigned char *source = sourcedata;

	for (int k = 0; k < h; k++)
	{
		for (int j = 0; j < w; j++)
		{
			unsigned char r, g, b, a;
			if (sourceformat==1) // 8 bits
			{
				/*
				switch(*source) 
				{
					case 0: r=g=b=0; break;
					case 1: r=255;g=b=0; break;
					case 2: r=0;g=255;b=0; break;
					case 3: r=0;g=0;b=255; break;
					case 4: r=255;g=255;b=0; break;
					case 5: r=0;g=255;b=255; break;
					default: r=255;g=255;b=255; break;
				}	
				*/
				r=g=b=*source++;
			}
			else
			{
				r = *source; source++;
				g = *source; source++;
				b = *source; source++;
			}
			*dest = b; dest++;
			*dest = g; dest++;
			*dest = r; dest++;			
			if (sourceformat==4) 
			{
				a = *source; source++;
				*dest = a; dest++;
			}				
		}
	}
	
	FILE *f = fopen(filename,"wb");
	if (!f)
	{		
		MyOutputDebugString("Cannot save %s\n",filename);
		delete [] data;
		return (false);
	}

	if (!fwrite(&header,1, sizeof(header),f))
	{		
		MyOutputDebugString("Cannot save %s\n",filename);
		delete [] data;
		fclose(f);
		return (false);
	}

	if (!fwrite(data,1, w*h*sourceformat,f))
	{		
		MyOutputDebugString("Cannot save %s\n",filename);		
		delete [] data;
		fclose(f);
		return (false);
	}

	fclose(f);
	delete [] data;

	return (true);
}

//////////////////////////////////////////////////////////////////////////
uchar *CPhotoImage::ConvertToGrayScale()
{	
	int r,g,b;
	int sum;
	uchar *pImg=new uchar [m_nWidth*m_nHeight];
	for (int k=0;k<m_nWidth*m_nHeight;k++)
	{
		r=m_pRawPixels[k*3+0];
		g=m_pRawPixels[k*3+1]; 
		b=m_pRawPixels[k*3+2];
		// NEVER DO IT!
		//sum=(int)(((ftype)(r+g+b)/3.0)+0.5);
		sum=(int)(((ftype)(r+g+b)/3.0));
		pImg[k]=sum;
	} //k
	return (pImg);
}

//////////////////////////////////////////////////////////////////////////
void CPhotoImage::GrayScale()
{
	if (m_nBpp==8)
		return;  // already grayscale

	uchar *pGray=ConvertToGrayScale();
	m_nBpp=8;
	delete [] m_pRawPixels;
	m_pRawPixels=pGray;
	if (m_pImgHeader)
		cvReleaseImageHeader(&m_pImgHeader);
	m_pImgHeader=cvCreateImageHeader(cvSize(m_nWidth,m_nHeight),8,(m_nBpp>>3));
	m_pImgHeader->imageData=(char *)(m_pRawPixels);	
}

//////////////////////////////////////////////////////////////////////////
void	CPhotoImage::SaveAsTga(const char *_szFilename,const char *szExtra,...)
{

	char szFilename[512];
	if (_szFilename)
		strcpy(szFilename,_szFilename);
	else	
	{
		va_list	argptr;
		char		msg[512];

		va_start (argptr,szExtra);
		vsprintf (msg,szExtra,argptr);
		va_end (argptr);	
		sprintf(szFilename,"%s-%s.tga",m_szName,msg);		
	}
	
	if (!m_pRawPixels)
	{
		if (m_pFloatPixels)
		{		
			uchar *temp=new uchar [m_nWidth*m_nHeight];
			for (int k=0;k<m_nWidth*m_nHeight;k++)	
				temp[k]=(uchar )(m_pFloatPixels[k]);
			SaveTga(temp,1,m_nWidth,m_nHeight,szFilename,true);
			delete [] temp;
		}
		else 
			ASSERT(0);
	}
	else
		SaveTga(m_pRawPixels,m_nBpp>>3,m_nWidth,m_nHeight,szFilename,true);
}