
#include "StdAfx.h"
#include "photoframe.h"
#include "PhotoImage.h"
#include "PVertex.h"

#include "opencv/cv.h"
//#include "opencv/cvaux.h"
#include "opencv/highgui.h"

//////////////////////////////////////////////////////////////////////////
CPhotoFrame::CPhotoFrame(void)
{
	m_pImage=NULL;
	m_Camera.m_bCustomPerspective=true;
	m_pScaledImage=NULL;
	m_pConfidence=NULL;
	m_pBestDepth=NULL;	
	m_pError=NULL;
	m_pCurrZ=NULL;

	// 3
	m_pState=NULL;
	m_pPoints=NULL;
	m_pPos=NULL;
	m_pFrame=NULL;
	m_pTriangles=NULL;
	m_FundMatrix=NULL;

	m_nLastDiffuse=NULL;
	m_nCurrDiffuse=NULL;
	m_nDiffused=0;
	m_nDiffusePixel=0;
}

//////////////////////////////////////////////////////////////////////////
CPhotoFrame::~CPhotoFrame(void)
{
	SAFE_DELETE(m_pImage);
	SAFE_DELETE_ARRAY(m_pScaledImage);
	SAFE_DELETE_ARRAY(m_pConfidence);
	SAFE_DELETE_ARRAY(m_pBestDepth);
	SAFE_DELETE_ARRAY(m_pError);
	SAFE_DELETE_ARRAY(m_pCurrZ);

	SAFE_DELETE_ARRAY(m_pState);
	SAFE_DELETE_ARRAY(m_pPoints);
	SAFE_DELETE_ARRAY(m_pPos);	
	SAFE_DELETE_ARRAY(m_pFrame);
	SAFE_DELETE_ARRAY(m_pTriangles);
		
	SAFE_DELETE_ARRAY(m_nLastDiffuse);
	SAFE_DELETE_ARRAY(m_nCurrDiffuse);

	RemoveVertices();

	cvReleaseMat(&m_FundMatrix);
}

//////////////////////////////////////////////////////////////////////////
void CPhotoFrame::RemoveVertices()
{
	for (lstPhotoVerticesIt i=m_lstFeatures.begin();i!=m_lstFeatures.end();i++)
	{
		CPVertex *pVert=(*i);
		delete pVert;
	}
	m_lstFeatures.clear();
}

//////////////////////////////////////////////////////////////////////////
CVideo	*CPhotoFrame::GetVideo() 
{ 
	return(m_pImage->m_pVideo); 
}

//////////////////////////////////////////////////////////////////////////
void CPhotoFrame::Scale(int nScale)
{
	SAFE_DELETE(m_pScaledImage);
	m_pScaledImage=m_pImage->GaussianScale(nScale);		
}

