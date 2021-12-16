

#include "StdAfx.h"
//#include "imageprocessing.h"
#include "PhotoImage.h"
#include "segm.h"
#include "opencv/cv.h"
#include "opencv/highgui.h"

extern int option;

//////////////////////////////////////////////////////////////////////////
void CPhotoImage::SegmentImage(unsigned char *src,unsigned char *dst,int nOption,int *regions,int w,int h)
{		
 // int i; 
  SegmenterMS::sRectangle rects[Max_rects];

  long	selects = 0L;
  selects |= Lightness; selects |= Ustar; selects |= Vstar; 
  int p_dim=3;
  
  assert( p_dim <= p_max );	// must be since all array dimensions are such
  Boolean       block = false;
  unsigned int	seed  = 29254088; //random # generator

	option=nOption; // o = oversegmentation

	int		n_rect = 0;
	
	//////////////////////////////////////////////////////////////////////////
	// convert to silly structure
	
	Octet**	datain = new Octet*[p_max];
	for ( int i = 0; i < p_max; i++ ) 
	{
		datain[i] = new Octet[w * h];
	}

  Octet *temp0 = datain[0];
	Octet *temp1 = datain[1];
	Octet *temp2 = datain[2];

	uchar *temp_buf=(uchar *)(src);
	for (int j = 0, idx = 0; j < h*w; j++) 
	{
		temp0[j] = temp_buf[idx++];
	  temp1[j] = temp_buf[idx++];
	  temp2[j] = temp_buf[idx++];
	}
 
  XfRaster::Info	info;
  info.rows = h;
  info.columns = w;
  info.origin_x = 0;
  info.origin_y = 0;

  RasterIpChannels*	signal=new RasterIpChannels(info,p_max,eDATA_OCTET,datain,true);

	//////////////////////////////////////////////////////////////////////////
	
  SegmenterMS	segmenter;

	uchar *borders=new uchar [w*h];

  segmenter.ms_segment( signal, rects, n_rect, selects, seed, block,borders,m_pImageProcessing->m_Regions1,m_pImageProcessing);
  RasterIpChannels*	sig_result = segmenter.result_ras_;

	temp_buf=(uchar *)(dst);
  temp0 = sig_result->chdata_[0];
  temp1 = sig_result->chdata_[1];
  temp2 = sig_result->chdata_[2];   
  for (int j=0,idx=0;j<sig_result->rows_*sig_result->columns_;j++) 
	{
		//if (borders[j]==255)
		{
			temp_buf[idx++]=temp0[j];  //red
			temp_buf[idx++]=temp1[j];  //green
			temp_buf[idx++]=temp2[j];  //blue
		}			
		/*
		else
		{
			temp_buf[idx++]=255;  
			temp_buf[idx++]=255;  
			temp_buf[idx++]=255;  
		}	
		*/
  }

	SAFE_DELETE_ARRAY(borders);	
  delete signal;
  delete sig_result;  


	/*
	uint *nTemp=new uint [w*h];
	// calc neighbors
	for (segmlistit i=m_pImageProcessing->m_lstSegments.begin();i!=m_pImageProcessing->m_lstSegments.end();i++)
	{
		CSegment *pSeg=(*i);
		for (int k=0;k<pSeg->m_nNumPixels;k++)
		{
			int nPos=pSeg->m_nPos[k];
			int y=nPos/w;
			int x=nPos-(y*w);
			for (int j=1;j<9;j++)
			{
				int xpos=x+dirs[j][0];
				int ypos=y+dirs[j][1];
				if (xpos<0 || ypos<0 || xpos>=w || ypos>=h)
					continue;
				int nSeg=m_pImageProcessing->m_Regions1[ypos*w+xpos];
				if (nSeg!=pSeg->m_nSegmNumber)
				{
					// check if it isn't there already
					for (int k2=0;k2<pSeg->m_nNumNeighbors;k2++)
					{
						if (pSeg->m_nNeighbors[k2]==nSeg)
							break;
					} //k2
					if (k2==pSeg->m_nNumNeighbors)						
						nTemp[pSeg->m_nNumNeighbors++]=nSeg;					
				}
			} //j
		} //k

		if (pSeg->m_nNumNeighbors>0)
		{
			pSeg->m_nNeighbors=new uint[pSeg->m_nNumNeighbors];
			memcpy(pSeg->m_nNeighbors,nTemp,sizeof(uint)*pSeg->m_nNumNeighbors);
		}
	} //i

	SAFE_DELETE_ARRAY(nTemp);

	*/

}

//////////////////////////////////////////////////////////////////////////
void CPhotoImage::SegmentImage(bool bSaveTga)
{

	char szFilename[1024];
	sprintf(szFilename,"%s-Segmented.bin",m_szName);			
	if (LoadSegmentedImage(szFilename))
		return;
			
	IplImage *pTemp=cvCloneImage(m_pImgHeader);		
	IplImage *pTemp2=cvCloneImage(m_pImgHeader);
	//for (int k=0;k<4;k++)	
	{	
		// first "semplify" the image with a bilateral filtering						
		// to remove all small residual crap inside the image				
		cvSmooth(pTemp2,pTemp,CV_BILATERAL,50,3);		
		memcpy(pTemp2->imageData,pTemp->imageData,m_nWidth*m_nHeight*(m_nBpp>>3));
	}

	//////////////////////////////////////////////////////////////////////////
	// debug
	//sprintf(szFilename,"%s-Filtered.tga",m_szName);
	//SaveTga((uchar *)(pTemp->imageData),3,m_nWidth,m_nHeight,szFilename,true);
	//////////////////////////////////////////////////////////////////////////
	
	cvReleaseImage(&pTemp2);

	m_pImageProcessing->m_Regions1=new int [m_nWidth*m_nHeight];
	uchar *pSegmented=new unsigned char[m_nWidth*m_nHeight*3];
	//SegmentImage(m_pRawPixels,pSegmented,1,m_pImageProcessing->m_Regions1,m_nWidth,m_nHeight);
	SegmentImage((uchar *)(pTemp->imageData),pSegmented,1,m_pImageProcessing->m_Regions1,m_nWidth,m_nHeight);

	if (bSaveTga)
	{
		char szFilename[1024];
		sprintf(szFilename,"%s-Segmented.tga",m_szName);
		SaveTga(pSegmented,3,m_nWidth,m_nHeight,szFilename,true);
	}
	SaveSegmentedImage(szFilename);

	SAFE_DELETE_ARRAY(pSegmented);
	cvReleaseImage(&pTemp);
}

//////////////////////////////////////////////////////////////////////////
void CPhotoImage::SaveSegmentedImage(const char *szFilename)
{	
	FILE *fp=fopen(szFilename,"wb");
	fwrite(&m_nWidth,sizeof(int),1,fp);
	fwrite(&m_nHeight,sizeof(int),1,fp);
	fwrite(m_pImageProcessing->m_Regions1,m_nWidth*m_nHeight*sizeof(int),1,fp);
	int nVal=(int)(m_pImageProcessing->m_lstSegments.size());
	fwrite(&nVal,sizeof(int),1,fp);
	for (segmlistit i=m_pImageProcessing->m_lstSegments.begin();i!=m_pImageProcessing->m_lstSegments.end();i++)
	{
		CSegment *pSeg=(*i);
		fwrite(&pSeg->m_nSegmNumber,sizeof(pSeg->m_nSegmNumber),1,fp);
		fwrite(&pSeg->m_nNumPixels,sizeof(pSeg->m_nNumPixels),1,fp);
		fwrite(pSeg->m_nPos,sizeof(uint)*pSeg->m_nNumPixels,1,fp);
		/*
		fwrite(&pSeg->m_nNumNeighbors,sizeof(pSeg->m_nNumNeighbors),1,fp);
		if (pSeg->m_nNumNeighbors>0)
			fwrite(pSeg->m_nNeighbors,sizeof(uint)*pSeg->m_nNumNeighbors,1,fp);
		fwrite(&pSeg->m_fBestZ,sizeof(ftype),1,fp);
		fwrite(&pSeg->m_fBestError,sizeof(ftype),1,fp);
		*/
	} //i
	fclose(fp);
}

//////////////////////////////////////////////////////////////////////////
bool CPhotoImage::LoadSegmentedImage(const char *szFilename)
{	
	FILE *fp=fopen(szFilename,"rb");
	if (!fp)
		return (false);
	int w,h,nVal;
	fread(&w,sizeof(int),1,fp);
	fread(&h,sizeof(int),1,fp);	
	ASSERT(w==m_nWidth);
	ASSERT(h==m_nHeight);
	m_pImageProcessing->m_Regions1=new int [w*h];
	fread(m_pImageProcessing->m_Regions1,m_nWidth*m_nHeight*sizeof(int),1,fp);	
	fread(&nVal,sizeof(int),1,fp);
	m_pImageProcessing->m_lstSegments.reserve(nVal);
	for (int i=0;i<nVal;i++)
	{
		CSegment *pSeg=new CSegment();
		fread(&pSeg->m_nSegmNumber,sizeof(pSeg->m_nSegmNumber),1,fp);
		fread(&pSeg->m_nNumPixels,sizeof(pSeg->m_nNumPixels),1,fp);
		pSeg->m_nPos=new uint [pSeg->m_nNumPixels];
		fread(pSeg->m_nPos,sizeof(uint)*pSeg->m_nNumPixels,1,fp);
		/*
		fread(&pSeg->m_nNumNeighbors,sizeof(pSeg->m_nNumNeighbors),1,fp);
		if (pSeg->m_nNumNeighbors>0)
		{
			pSeg->m_nNeighbors=new uint [pSeg->m_nNumNeighbors];
			fread(pSeg->m_nNeighbors,sizeof(uint)*pSeg->m_nNumNeighbors,1,fp);
		}
		fread(&pSeg->m_fBestZ,sizeof(ftype),1,fp);
		fread(&pSeg->m_fBestError,sizeof(ftype),1,fp);
		*/
		m_pImageProcessing->m_lstSegments.push_back(pSeg);
	} //i
	fclose(fp);
	return (true);
}
