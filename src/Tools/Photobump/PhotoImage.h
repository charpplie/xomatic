
#pragma once

//////////////////////////////////////////////////////////////////////////
#include "ImageProcessing.h"

//////////////////////////////////////////////////////////////////////////
bool	SaveTga(unsigned char *sourcedata,int sourceformat,int w,int h,const char *filename,bool flip);
void	BilateralFiltering(double *src,double *dst,double fSigmaColor,double fSigmaSpace,int nChannels,int width,int height);
void	Resize(uchar *src,uchar *dst,int w,int h,int nw,int nh,int nBpp);

//////////////////////////////////////////////////////////////////////////
class CVideo;
typedef struct _IplImage IplImage;

//////////////////////////////////////////////////////////////////////////
class CPhotoImage
{
public:
	CPhotoImage(CVideo *pVideo);
	virtual ~CPhotoImage(void);
	virtual bool	Load(const char *szFilename,unsigned int dwFlags=0,const char *szExportName=NULL);		

	const char *GetName() { return(m_szName); }	
	void	DrawPoint(int x,int y,int nSize,uchar r,uchar g,uchar b,uchar *pDest=NULL,int nBpp=0);
	void	DrawPoint(const vector3f &vPos,int nSize,uchar r,uchar g,uchar b)
	{
		DrawPoint((int)(vPos.x+ROUNDING_VAL),(int)(vPos.y+ROUNDING_VAL),nSize,r,g,b);
	}
	void	DrawLine(vector3f v1,vector3f v2,vector3f c1,vector3f c2);
	CPhotoImage *Scale(int nScale);
	CPhotoImage *GaussianScale(int nScale);
	CPhotoImage *Resize(int w,int h,int nBpp,bool bConvertToFloat);
	void	SegmentImage(bool bSaveTga);
	void	ExtractEdges(bool bSaveTga,bool bAllocateNormals=true,bool bSmoothImage=true);
	void	SaveSegmentedImage(const char *szFilename);
	bool	LoadSegmentedImage(const char *szFilename);
	void	GrayScale();
	uchar	*ConvertToGrayScale();
	void	SaveAsTga(const char *szFilename,const char *szExtra,...);
	static	void	Smooth(ftype	*pImage,int w,int h,int nIteration);
	IplImage *GetIplHeader() { return(m_pImgHeader); }
	bool			LoadInVideoMemory(unsigned int dwFlags,const char *szExportName);

	unsigned int	m_nID;
	char	m_szName[256];
	int		m_nWidth,m_nHeight;	
	int		m_nGLWidth,m_nGLHeight;		
	int		m_nBpp;
	unsigned char *m_pRawPixels;
	unsigned char *m_pEdges;
	float *m_pFloatPixels;
	CVideo	*m_pVideo;
	CImageProcessing	*m_pImageProcessing;
	vector3float *m_vNormals;	
	IplImage	*m_pImgHeader;

protected:	
	void			FlipAndSwap(unsigned char *pMem);	

	// option:	0= quantization
	//					1= oversegmentation
	//					2= undersegmentation
	void	SegmentImage(unsigned char *src,unsigned char *dst,int nOption,int *regions,int w,int h);	
};

