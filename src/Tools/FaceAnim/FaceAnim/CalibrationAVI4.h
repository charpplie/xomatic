

#pragma once

#define FA_PROGRAM_VERSION				14L
#define DEFAULT_CALIBRATION_FILE	"Default_ROMCalibration.ini"

#define FA_LT_EYEBROW_LT	0
#define FA_LT_EYEBROW_RT	1
#define FA_RT_EYEBROW_LT	2
#define FA_RT_EYEBROW_RT	3
#define FA_LT_CHEEK				4
#define FA_RT_CHEEK				5
#define FA_MOUTH_LT				6
#define FA_MOUTH_RT				7
#define FA_LT_LIP_TOP			8
#define FA_RT_LIP_TOP			9
//#define FA_LT_LIP_BOTTOM	10
//#define FA_RT_LIP_BOTTOM	11
#define FA_LT_NOSE				10
#define FA_RT_NOSE				11
#define FA_CHIN						12
// this is used as reference center.
// The middle part of the Nose (directly on the cartilage) 
// is the steadiest point in the middle face.
#define FA_NOSE						13	
#define FA_NUM_MARKERS		14

#define OF_REPLACE_FEATURES	4

typedef struct  
{
	// name assigned to markers
	char	szName[32];

	// features to follow the optical flow from, in case this one gets lost 
	void	SetFollowFlowReplacement(int n1,int n2,int n3,int n4=-1)
	{
		nFollowFlow[0]=n1;nFollowFlow[1]=n2;nFollowFlow[2]=n3;nFollowFlow[3]=n4;		
	}

	// features to follow the optical flow from, in case this one gets lost 
	int		nFollowFlow[OF_REPLACE_FEATURES];
	// to preserve the structure, a ratio of how much the extent from the reference point (see above) can vary
	ftype fMaxExtentRatio;
	// to preserve the structure, a ratio of how much the direction vector from the reference point (see above) can vary
	// expressed as dot product
	ftype fMaxAngleDiff;
	// The 2D ref position of the markers found in the first frame
	vector3f	v2DPos;
	// The 3D ref position of the markers found in the first frame
	vector3f	v3DPos;
	// The reference angle between center (nose) and marker
	ftype			fDot;

	// original vector from the center in the original frame
	vector3f	vDir;
	// squared distance from the ref center in the original frame
	ftype			fDist2; 

	// exclude this feature from processing.
	bool			bExclude;

	// flip	
	bool			bFlipDirs;
	bool			bExport;
	int				nAverage;

	ftype			fScaleX;
	ftype			fScaleY;

	// for calibration
	ftype			fMinRangeX,fMaxRangeX;
	ftype			fMinRangeY,fMaxRangeY;

	//	during markers detection
	int				nAssignedMarker1,nAssignedMarker2;

}tMarkerInfo;

struct tFeatureProcessing;

//////////////////////////////////////////////////////////////////////////
struct tFaceDetect
{
	CvSize frame_size;
	CvMemStorage	*storage;
	bool	bFlip;
	IplImage	*frame1_1C;
	IplImage	*frame2_1C;

	CvHaarClassifierCascade* cascadeface;
	CvHaarClassifierCascade* cascade_lefteye;
	CvHaarClassifierCascade* cascade_righteye;
	CvHaarClassifierCascade* cascade_mouth;

	CvPoint face_center;
	int			radius;
	bool		bEyeLDetected;
	bool		bEyeRDetected;
	bool		bMouthDetected;

	CvRect	eye_left;
	CvRect	eye_right;
	CvRect	mouth;
	CvRect	face;		

	CvRect	prev_eye_left;
	CvRect	prev_eye_right;
	CvRect	prev_mouth;
	CvRect	prev_face;	

	CvRect	ref_eye_left;
	CvRect	ref_eye_right;
	CvRect	ref_mouth;
	CvRect	ref_face;	

	float ref_rads_center_to_markers[FA_NUM_MARKERS];
	float	ref_dist2_center_to_markers[FA_NUM_MARKERS];
	CvPoint2D32f	ref_dir_center_to_markers[FA_NUM_MARKERS];
	CvPoint2D32f ref_center;

	vector3f vRef_AverageFaceCenter;
	// This array stores the 2D positions of the markers found in the first frame
	//vector3f	ref_lst_markers[FA_NUM_MARKERS];

	// This array stores the estimate 3D positions of the markers found 
	//vector3f			ref_lst_3D_markers[FA_NUM_MARKERS];			

	// This array stores the estimate 3D positions of the markers found in the last frame
	vector3f			lst_3D_markers[FA_NUM_MARKERS];			
	CMatrixf			mProjMatrix;
	CMatrixf			mViewMatrix; // combined with projmatrix
	CMatrixf			mMatrix; // combined with projmatrix
	CMatrixf			mInvertedViewMatrix; // combined with projmatrix
	int						nViewport[4];	

	vector3f			vRef_3DCenter;			
	vector3f			v2DCenter,v3DCenter; // last center found			
	// distances	from the center in the original frame
	//ftype					ref_lst_3D_dist2[FA_NUM_MARKERS]; 

	// distances	from the center found in the last frame
	ftype					lst_3D_dist2[FA_NUM_MARKERS]; 

	tMarkerInfo		lst_ref_Markers[FA_NUM_MARKERS];

	// only created in the refframe
	int						nNumVertices;
	int						nNumTris;
	int						*lst_Tris;
	vector3f			*lst_vModel_3D;
	vector3f			*lst_vModel_2D;
	IplImage			*pRefFrame;
	IplImage			*pRefFrame2;
	vector3f			vMins,vMaxs;		
	vector3f			v2DMins,v2DMaxs;

	vector3f			v3DMins,v3DMaxs;	// based on face vertices
	CvRect				face_region;

	ftype					fGlobalError;
	ftype					fFov;
	int						nIter;

	tFeatureProcessing *pCurrFrame;

	vector3f			vScale;
	vector3f			vOffset;
	vector3f			vAngles;
	
	void ProjectToScreen(const vector3f &vSource,vector3f &vDest)
	{
		double dx,dy,dz;	

		MyProject(vSource.x,vSource.y,vSource.z,		
			&mMatrix.m_Tvalues[0][0],nViewport,&dx,&dy,&dz);

		vDest.x=(ftype)(dx);
		vDest.y=(ftype)((nViewport[3])-dy);
		vDest.z=(ftype)(dz);
	}

	//////////////////////////////////////////////////////////////////////
	void UnProjectFromScreen(const vector3f &vSource,vector3f &vDest)
	{
		double dx,dy,dz;

		vector3f vTemp=vSource;
		vTemp.y=(nViewport[3]-vSource.y);

		MyUnproject(vTemp.x,vTemp.y,vTemp.z,
			&mInvertedViewMatrix.m_Tvalues[0][0],nViewport,&dx,&dy,&dz);		

		vDest.x=(ftype)(dx);vDest.y=(ftype)(dy);vDest.z=(ftype)(dz);	
	}
	
	CvFont	font1;
};

//////////////////////////////////////////////////////////////////////////
typedef struct tFeatureProcessing 
{
	tFaceDetect	*pFace;

	int nFeatures;	
	// features found in frame 1
	CvPoint2D32f	*lst_Frame1_features;
	// This array will contain the locations of the points from frame 1 in frame 2
	CvPoint2D32f	*lst_Frame2_features;	
	// The i-th element of this array will be non-zero if and only if the i-th feature of
	// frame 1 was found in frame 2
	char *lst_optical_flow_found_feature;
	// The i-th element of this array is the error in the optical flow for the i-th feature
	// of frame1 as found in frame 2  
	float *lst_optical_flow_feature_error;

	//CvPoint2D32f *lst_markers;	
	CvPoint2D32f centre;
	float rads_face_rotation;
	
	bool	bReferenceFrame;
	IplImage *pDrawingFrame;
	IplImage *pDrawingFrameEdit;

	// current frame
	int	nFrame;

	// This array stores the positions of the markers found 
	vector3f			lst_markers[FA_NUM_MARKERS];	
	bool					bFound[FA_NUM_MARKERS];

	// 3D positions
	vector3f	vRotation;

} tFeatureProcessing;
