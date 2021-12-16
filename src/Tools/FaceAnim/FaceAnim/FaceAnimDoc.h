// FaceAnimDoc.h : interface of the CFaceAnimDoc class
//


#pragma once 

#include "CalibrationAVI4.h"

//////////////////////////////////////////////////////////////////////////
class CFaceAnimDoc : public CDocument
{
protected: // create from serialization only
	CFaceAnimDoc();
	DECLARE_DYNCREATE(CFaceAnimDoc)

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
	virtual ~CFaceAnimDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	bool	SaveTGA(unsigned char *sourcedata,int sourceformat,int w,int h,const char *filename,bool flipimage,bool bflipcolors);
	void	EditMarkers(int x,int y,int event);
	void	EditMarkers2(int x,int y,int event);	
	void	EditMarkers3(int x,int y,int event);	
	bool	LoadCalibrationFile(const char *szFilename);

	tFaceDetect	*m_pFace;

	bool				m_bBatchMode;

	// editing
	bool				m_bEditMode;	
	int					m_nMarkerSelected;
	bool				m_bEditOperation;
	bool				m_bDrawLabels;	

	bool				m_bCreateCalibrationData;

	char				m_szWindowFilename[256];

protected:

	bool CalibrationAVI3(const char *_szFilename);
	bool CalibrationAVI3_2(const char *_szFilename);
	void allocateOnDemand2( IplImage **img, CvSize size, int depth, int channels );
	bool DetectFace(IplImage *pFrame,tFaceDetect &tFace);

	const char *GetFilename(const char *szSrc);
	const char *GetPath(const char *szFilename);
	void	FixPath(char *szFilename);
	void	RemoveExtension(char *szPath);
	void	ReplaceExtension(char *szPath, const char *szExt);
	void	DrawRectangle(IplImage *pFrame,const CvRect &tRect,CvScalar color);
	void	DrawRectangle(IplImage *pFrame,float x1,float y1,float x2,float y2,CvScalar color);
	void	DrawRectangle(IplImage *pFrame,const vector3f &v1,const vector3f &v2,CvScalar color);
	void	DrawLine(IplImage *pFrame,const vector3f &v1,const vector3f &v2,CvScalar color);
	void	DrawLine3d(tFeatureProcessing *tFrame,const vector3f &v1,const vector3f &v2,CvScalar color);
	void	DrawRectangleCenter(IplImage *pFrame,const CvRect &tRect,CvScalar color);
	bool	InsideRect(int x,int y,const CvRect &tRect,int &dist);
	void	CalcSimpleMoment(const CvRect &tRectFrom,const CvRect &tRectTo,float &fX,float &fY);
	void	ProcessFeatures(tFeatureProcessing * lstFrames, long iFrameNum);
	
	void	FindOrientation(tFeatureProcessing * lstFrames, int nFrame,int nStep);
	void	FindPosition(tFeatureProcessing *tFrame,ftype x1,ftype y1,ftype x2,ftype y2,ftype x11,ftype y11,ftype x22,ftype y22,ftype &fX1,ftype &fY1,ftype &fX2,ftype &fY2);
	int		Refine(tFeatureProcessing *tFrame,int nFeatureID,ftype fDistMax,bool bCheckFlow=true);

	void	MyPerspective2(double fovx, double aspect, double zNear, double zFar,double *m);
	void	ProjectToScreen(const vector3f &vSource,vector3f &vDest,tFeatureProcessing *tFrame);
	void	UnProjectFromScreen(const vector3f &vSource,vector3f &vDest,tFeatureProcessing *tFrame);

	void	FindRotation(tFeatureProcessing *tFrame1,tFeatureProcessing *tFrame2);
	void	CalcBestRot(vector3f &vBestRot,ftype fRange,ftype fStep,const vector3f &vRotStart,ftype &fBestError,tFeatureProcessing *tFrame1);
	void	Calc3DInfo(tFeatureProcessing *tFrame);

	void	FindOrientation2(tFeatureProcessing * lstFrames, int nFrame,int nStep);
	void	FindRotation2(tFeatureProcessing *tFrame1,tFeatureProcessing *tFrame2);
	void	CalcBestRot2(vector3f &vBestRot,ftype fRange,ftype fStep,const vector3f &vRotStart,ftype &fBestError,tFeatureProcessing *tFrame1);
	bool	Calc3DInfo2(tFeatureProcessing *tFrame);
	bool	LoadOBJ(const char *szFilename,tFaceDetect *pFace);
	void	ProcessFolder(const char *szFilename);

	bool			CreateFace(const char *szFilename);	
	bool			CreateFace2(const char *_szFilename);	
	bool			m_bScaleOperationX,m_bScaleOperationY,m_bMoveOperation;
	ftype			m_fScaleOperationX,m_fScaleOperationY,m_fMoveX,m_fMoveY;	
	bool			m_bMouse1,m_bMouse2,m_bMouse3;

	char	m_szBuffer[256];

	// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnFileOpen();
	afx_msg void OnFileCreatecalibrationfile();
	afx_msg void OnFileLoadcalibrationfile();
	afx_msg void OnFileCreateFace();
	afx_msg void OnFileBatchProcessFolder();
};


