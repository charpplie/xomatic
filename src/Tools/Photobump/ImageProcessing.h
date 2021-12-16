#pragma once

//////////////////////////////////////////////////////////////////////////
class CPVertex;

//////////////////////////////////////////////////////////////////////////
class CSegment
{
public:
	CSegment() 
	{ 
		m_nPos=NULL; 
		//m_fBestError=99999999;
		//m_fBestZ=0;
		//m_nNeighbors=NULL;
		//m_nNeighbors=0;
		m_bFit=false;
	}
	~CSegment() 
	{ 
		SAFE_DELETE_ARRAY(m_nPos); 
		//SAFE_DELETE_ARRAY(m_nNeighbors);
	}

	bool	PlaneFitSegment(CPhotoFrame *pFrame,bool bMarkFit);
	void	Clear(CPhotoFrame *pFrame);
	void	CalcCenter(int &cx,int &cy,CPhotoFrame *pFrame);
	void	Draw(CPhotoFrame *pFrame,const vector3f &vCol,int nx=0,int ny=0);
	bool	FindBestMatch(const vector3f &v1,const vector3f &v2,int &rX,int &rY,CPhotoFrame *pKeyFrame,CPhotoFrame *pOther,IplImage *pImg);

	int		m_nSegmNumber;
	int		m_nNumPixels;	
	uint	*m_nPos;
	//ftype	m_fBestError,m_fBestZ;
	//int		m_nNumNeighbors; 
	//uint	*m_nNeighbors;
	//std::vector<vector3f>	m_lstVertices;
	bool	m_bFit;
	//vector3f	m_vCenter;
	vector3f m_vPlane;
	double m_vCoeffs[6]; // 6 coeffs for paraboloid
};

typedef std::vector<CSegment *> segmlist;
typedef segmlist::iterator segmlistit;

//////////////////////////////////////////////////////////////////////////
class CImageProcessing
{
public:
	CImageProcessing(void)
	{
		m_Regions1=NULL;
	}
	virtual ~CImageProcessing(void)
	{
		SAFE_DELETE_ARRAY(m_Regions1);
		for (segmlistit i=m_lstSegments.begin();i!=m_lstSegments.end();i++)
		{
			CSegment *pSeg=(*i);
			delete pSeg;
		} //i
	}

	int		*m_Regions1;
	segmlist	m_lstSegments;
};
