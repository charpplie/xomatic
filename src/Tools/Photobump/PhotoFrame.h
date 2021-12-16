#pragma once

//////////////////////////////////////////////////////////////////////////
class CPhotoImage;
#include "OpenCV/cvtypes.h"

//////////////////////////////////////////////////////////////////////////
class CPhotoFrame
{
public:
	CPhotoFrame(void);
	virtual ~CPhotoFrame(void);
	CVideo	*GetVideo();

	// scaling and upsampling	
	void	Scale(int nScale);
	void	UpSample();

	void	RemoveVertices();
	
	CCamera	m_Camera;
	CPhotoImage	*m_pImage;	
	int				m_nFrameNum;

	lstPhotoVertices	m_lstFeatures;
	int				m_nDiffused;
	int				*m_nLastDiffuse;
	int				*m_nCurrDiffuse;
	int				m_nDiffusePixel;

	// 2
	CPhotoImage	*m_pScaledImage;
	ftype			*m_pBestDepth,*m_pError;
	ftype			*m_pConfidence,*m_pCurrZ;	

	// 3
	vector3f	*m_pPoints;	 
	CtTriangle	**m_pTriangles;
	CvMat			*m_FundMatrix;

	char			*m_pState;
	uint			*m_pPos;
	ushort		*m_pFrame;

	ftype			m_fSlopeA,m_fSlopeB;
};
