// PhotoBump10Doc.h : interface of the CPhotoBump10Doc class
//

#pragma once

#define USE_GL_LIGHT
#define CREATE_RENDERING_DATA

#include "WmlConvexHull2.h"

//////////////////////////////////////////////////////////////////////////
class CPhotoBump10Doc : public CDocument
{
protected: // create from serialization only
	CPhotoBump10Doc();
	DECLARE_DYNCREATE(CPhotoBump10Doc)

// Attributes
public:

// Operations
public:

// Overrides
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);

// Implementation
public:
	virtual ~CPhotoBump10Doc();

	void	Reconstruct();
	void	Reconstruct2();	
	void	Reconstruct3();
	void	Reconstruct4();
	void	Reconstruct5(CPaintDC *dc);

	void	DumpError(CPhotoFrame *pFrame);
	int		GetCurrScale() { return(m_nScale); }
	int		GetInfo();	
	bool	LoadCalibrationData(const char *szFilename);	
	bool	LoadCalibrationData2(const char *szFilename,CPaintDC *dc);	
	bool	CalibrationAVI(const char *szFilename,CPaintDC *dc);	
	//bool	CalibrationAVI2(const char *szFilename,CPaintDC *dc);	
	bool	RectifyDistortion(const char *szFilename,CPaintDC *dc);	

	CPhotoBump10View *m_pGLView;
	CVideo	*m_pVideo;
	CUtils	*m_pUtils;
	lstFrames	m_lstFrames;
	lstPhotoVertices m_lstVertices;		

	bool			m_bLoadData;	
	bool			m_bDone;	
	char			m_szDataLoad[1024];
	
	uint			*m_Indices;
	float			*m_Vertices;
	int				m_nNumIndexes;
	int				m_nNumVertices;
	vector3f	m_vMins,m_vMaxs;
	bool	m_bShowFeaturesOnly;
	bool	m_bLoadedFromBojou;
	bool	m_bUseSingleStereoPhoto;
	bool	m_bUseDoubleStereoPhoto;
	bool	m_bStereoMode;

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

	void	InitialTriangulation();
	void	TriangulateAllFrames();
	void	DiffuseConfidence(int w,int h,ftype *pConfMap);
	void	RefineDepthMap(int w,int h,ftype *pDepthMap,ftype *pConfMap);

	void	RemoveCalibrationData();
	ftype	CalcError(CPhotoFrame *pFrame,CCamera *pCam);
	void	ReconstructInitialUpdate(CPaintDC *dc);
	void	ReconstructInitialUpdateOld();

	bool	SelfCalibration(CPaintDC *dc);
	bool	LoadBojouCalibrationData(const char *_szFilename,CPaintDC *dc);

		void	ReconstructInitialUpdate3();
		void	ReconstructInitialUpdate4();
		bool	ReconstructInitialUpdate5(CPaintDC *dc);
	
protected:	

	void			BuildQuadtree();
	bool			Iterate();
	bool			Iterate2();
	bool			Iterate3();
	bool			Iterate4();
	bool			Iterate5();
	void			FillinOccludedPixels();
	void			ReconstructSegments();
	ftype			CalcError(const vector3f &vp1,uchar *ptr1,int w,int h);
	void			RefineGrid(int nCellSize,bool bAddOffset=true,bool bAverageOnly=false);
	void			SmoothSegments();
	void			SmoothSegments2();
	uchar			*AlignNormalMap(const uchar *pData,int nW,int nH);
	//uchar			*GenerateHeightMap(const uchar *pData,int nW,int nH,int nIters);
	uchar			*AlignNormalMapFromFile(const char *szFilename,int &nW,int &nH);
	void			GenerateNormalMap(CPaintDC *dc);
	bool			CalcFundamentalMatrix();
	bool			ExportObj(const char *szFilename);
	uchar			*InvertChannels(const char *szFilename,int &nW,int &nH);
	uchar			*SwapRGChannelsInvertRGValues(const char *szFilename,int &nW,int &nH);
	uchar			*ConvertToHeightmap(const char *szFilename,int &nW,int &nH,int nIters);
	//uchar			*AutomaticTiling(const char *szFilename,int &nW,int &nH);
	void			CalcDepth(int x1,int y1,int x2,int w,int h);
	ftype			CalcNormalizedCrossCorrelation(const vector3f &v1,const vector3f &v2,uchar *ptr1,uchar *ptr2,int w,int h,int nWindow);
	ftype			CalcNormalizedCrossCorrelation(int x1,int y1,int x2,int y2,uchar *ptr1,uchar *ptr2,int w,int h,int nWindow);
	ftype			CalcBestRot(CCamera *pCam,vector3f &vBestRot,const vector3f &vPos,ftype fRange,ftype fStep,const vector3f &vRotStart,ftype fBestError,int nFrame,lstPhotoVertices *vertsTest);
	bool			DetectFeatures(CPhotoFrame *pFrame,int nFrames);
	int				MatchFeatures(CPhotoFrame *pFrame1,CPhotoFrame *pFrame2,lstPhotoVertices *tempVerts,int nFrames);
	void			DenseMatching(CPaintDC *dc);
	bool			LoadCalibData(CPaintDC *dc);
	void			SaveCalibData();
	void			GetSampleVerts(lstPhotoVertices *vertsTest,int w,int h,int k1,bool bAll=false,int nLimit=9999);
	void			FollowEdge(int x11,int y11,int x22,int y22,CPhotoImage *pImg1,CPhotoImage *pImg2,lstPhotoVertices *tempVerts,int nFrame1,int nFrame2,int w,int h,int nFrames);
	void			FollowEdge(const vector3f &v1,const vector3f &v2,CPhotoImage *pImg1,CPhotoImage *pImg2,lstPhotoVertices *tempVerts,int nFrame1,int nFrame2,int w,int h,int nFrames);
	ftype			FindBest3DPos(CPVertex *p1,CPhotoFrame *pFrame1,CPhotoFrame *pFrame2,vector3f &vRes);
	ftype			FindBest3DPos(vector3f v1,vector3f v2,CPhotoFrame *pFrame1,CPhotoFrame *pFrame2,vector3f &vRes);
	void			MatchAdditionalFeatures();
	void			MatchAdditionalFeatures2();
	void			SmoothSegments3();
	void			SmoothSegments3Part2(CPaintDC *dc);
	void			Save3DPoints(CPhotoFrame *pFrame);
	bool			Load3DPoints(CPhotoFrame *pFrame);
	void			PolygonReduction();
	
	bool			m_bInitialUpdate;
	bool			m_bSegErrorsSet;	
	CPQuadtree	*m_QTree;
	int				m_nNumFeatures,m_nUnreliableIterations;	
	int				m_nScale,m_nLastScale,m_nIteration;
	ftype			m_fMinZ,m_fMaxZ,m_fZRange,m_fZStep,m_fCurrZ,m_fMedZ;
	//	cached values
	int				m_x,m_y,m_nPos;	
	int				m_Pos[3][2];	
	int				m_nFrame;
	CtTriangle	*m_lstTriangles;
	Wml::ConvexHull2f	*m_ConvexHull2D;
	
	int				m_nSign;
	int				*m_IndexMap;
	ftype			m_fThreshold;
	
	bool	m_bHumans,m_bGenerateNormalMap,m_bGenerateHeightMap,m_bGenerateLowRes,m_bExportHiRes;
	bool	m_nUseSinglePhoto;
	ftype	m_fNormalMapScale;
	ftype	m_fDetailScale;
		
// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnFileSaveAs();
public:
	afx_msg void OnFileInvertchannels();
	//afx_msg void OnFileConverttoheightmap();
	afx_msg void OnSwapRGChannelsInvertRGValues();
	afx_msg void OnFileAlignto();
	//afx_msg void OnFileConverttoheightmap32775();
	//afx_msg void OnFileAutomatictiling();
};


