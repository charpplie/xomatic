
#include "stdafx.h"
#include "FaceAnim.h"

#include "FaceAnimDoc.h"
#include "FaceAnimView.h"
#include "CalibrationAVI4.h"
//#include "klt/klt.h"

#include <direct.h> // getcwd

#ifdef _DEBUG
#define new DEBUG_NEW 
#endif 

//////////////////////////////////////////////////////////////////////////
void on_mouse( int event, int x, int y, int flags, void* param )
{

	CFaceAnimDoc *pDoc=(CFaceAnimDoc *)param;
	if ( !pDoc || !pDoc->m_bEditMode )
		return;

	pDoc->EditMarkers(x,y,event);

}

#ifdef _WIN32
void AppInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line,	uintptr_t pReserved ) {
	fprintf(
		stderr,
		"Invalid parameter detected in function %s. File: %s Line: %d\n",
		function,
		file,
		line
		);
	fprintf(
		stderr,
		"Expression: %s\n",
		expression
		);
	// Cause a Debug Breakpoint.
	DebugBreak();
}
#endif



//////////////////////////////////////////////////////////////////////////
bool CFaceAnimDoc::CalibrationAVI3_2(const char *_szFilename)
{	
	char szText[256];
	char szFilename[256];

#ifdef _WIN32
	// Every once and awhile something looks at a std::vector or some other
	// CRT/STL construct that throws an exception when an invalid parameter
	// is detected.  In this case we should dump whatever information we 
	// can and then bail.  When we bail we should dump as much data as 
	// possible.
	_set_invalid_parameter_handler(AppInvalidParameterHandler);
#endif


	// Create an object that decodes the input video stream. 
	CvCapture *input_video = cvCaptureFromAVI(_szFilename);
	if (!input_video)
	{
		// Either the video didn't exist OR it uses a codec OpenCV
		// doesn't support.					
		MyError("Cannot open file %s \n",_szFilename);
		return false;
	}

	sprintf(m_szWindowFilename,"FaceAnim [%s]",GetFilename(_szFilename));

	// This is a workaround. If we don't call this first then getting capture
	// properties (below) won't work right. This is an OpenCV bug. We
	// ignore the return value here. But it's actually a video frame.	
	cvQueryFrame( input_video );

	// Read the video's frame size out of the AVI. 
	CvSize frame_size;
	int w=frame_size.width =(int) cvGetCaptureProperty( input_video, CV_CAP_PROP_FRAME_WIDTH );
	int h=frame_size.height =(int) cvGetCaptureProperty( input_video, CV_CAP_PROP_FRAME_HEIGHT );	
	int fps=(int) cvGetCaptureProperty( input_video, CV_CAP_PROP_FPS );	

	// Determine the number of frames in the AVI. 
	long number_of_frames;
	// Go to the end of the AVI (ie: the fraction is "1") 
	cvSetCaptureProperty( input_video, CV_CAP_PROP_POS_AVI_RATIO, 1. );
	// Now that we're at the end, read the AVI position in frames 
	number_of_frames = (int) cvGetCaptureProperty( input_video, CV_CAP_PROP_POS_FRAMES );
	// Return to the beginning 
	cvSetCaptureProperty( input_video, CV_CAP_PROP_POS_FRAMES, 0. );

	cvNamedWindow(m_szWindowFilename, CV_WINDOW_AUTOSIZE);

	cvSetMouseCallback( m_szWindowFilename, on_mouse, this );

	tFaceDetect	tFace;
	m_pFace=&tFace;
	
	//////////////////////////////////////////////////////////////////////////

	strcpy(tFace.lst_ref_Markers[FA_LT_EYEBROW_LT].szName,"left_eyebrow_left");
	strcpy(tFace.lst_ref_Markers[FA_LT_EYEBROW_RT].szName,"left_eyebrow_right");
	strcpy(tFace.lst_ref_Markers[FA_RT_EYEBROW_LT].szName,"right_eyebrow_left");
	strcpy(tFace.lst_ref_Markers[FA_RT_EYEBROW_RT].szName,"right_eyebrow_right");	
	strcpy(tFace.lst_ref_Markers[FA_LT_CHEEK].szName,"left_cheek");
	strcpy(tFace.lst_ref_Markers[FA_RT_CHEEK].szName,"right_cheek");
	strcpy(tFace.lst_ref_Markers[FA_MOUTH_LT].szName,"left_mouthcorner");
	strcpy(tFace.lst_ref_Markers[FA_MOUTH_RT].szName,"right_mouthcorner");
	strcpy(tFace.lst_ref_Markers[FA_LT_LIP_TOP].szName,"left_lip_top");
	strcpy(tFace.lst_ref_Markers[FA_RT_LIP_TOP].szName,"right_lip_top");
	//strcpy(tFace.lst_ref_Markers[FA_LT_LIP_BOTTOM].szName,"left_lip_bottom");
	//strcpy(tFace.lst_ref_Markers[FA_RT_LIP_BOTTOM].szName,"right_lip_bottom");
	strcpy(tFace.lst_ref_Markers[FA_LT_NOSE].szName,"left_nose");
	strcpy(tFace.lst_ref_Markers[FA_RT_NOSE].szName,"right_nose");
	strcpy(tFace.lst_ref_Markers[FA_CHIN].szName,"chin");
	strcpy(tFace.lst_ref_Markers[FA_NOSE].szName,"nose");

	tFace.lst_ref_Markers[FA_LT_EYEBROW_LT].SetFollowFlowReplacement(FA_LT_EYEBROW_RT,FA_RT_EYEBROW_LT,FA_RT_EYEBROW_RT);
	tFace.lst_ref_Markers[FA_LT_EYEBROW_RT].SetFollowFlowReplacement(FA_LT_EYEBROW_LT,FA_RT_EYEBROW_LT,FA_RT_EYEBROW_RT);
	tFace.lst_ref_Markers[FA_RT_EYEBROW_LT].SetFollowFlowReplacement(FA_RT_EYEBROW_RT,FA_LT_EYEBROW_RT,FA_LT_EYEBROW_LT);
	tFace.lst_ref_Markers[FA_RT_EYEBROW_RT].SetFollowFlowReplacement(FA_RT_EYEBROW_LT,FA_LT_EYEBROW_RT,FA_LT_EYEBROW_LT);
	tFace.lst_ref_Markers[FA_LT_CHEEK].SetFollowFlowReplacement(FA_MOUTH_LT,FA_NOSE,FA_RT_CHEEK);
	tFace.lst_ref_Markers[FA_RT_CHEEK].SetFollowFlowReplacement(FA_MOUTH_RT,FA_NOSE,FA_LT_CHEEK);
	tFace.lst_ref_Markers[FA_MOUTH_LT].SetFollowFlowReplacement(FA_LT_CHEEK,FA_LT_LIP_TOP,FA_MOUTH_RT);
	tFace.lst_ref_Markers[FA_MOUTH_RT].SetFollowFlowReplacement(FA_RT_CHEEK,FA_RT_LIP_TOP,FA_MOUTH_LT);
	tFace.lst_ref_Markers[FA_LT_LIP_TOP].SetFollowFlowReplacement(FA_LT_CHEEK,FA_NOSE,FA_MOUTH_LT);
	tFace.lst_ref_Markers[FA_RT_LIP_TOP].SetFollowFlowReplacement(FA_RT_CHEEK,FA_NOSE,FA_MOUTH_RT);
	//tFace.lst_ref_Markers[FA_LT_LIP_BOTTOM].SetFollowFlowReplacement(FA_MOUTH_LT,FA_CHIN,FA_MOUTH_RT);
	//tFace.lst_ref_Markers[FA_RT_LIP_BOTTOM].SetFollowFlowReplacement(FA_MOUTH_RT,FA_CHIN,FA_MOUTH_LT);
	tFace.lst_ref_Markers[FA_LT_NOSE].SetFollowFlowReplacement(FA_NOSE,FA_LT_CHEEK,FA_RT_NOSE);
	tFace.lst_ref_Markers[FA_RT_NOSE].SetFollowFlowReplacement(FA_NOSE,FA_RT_CHEEK,FA_LT_NOSE);
	tFace.lst_ref_Markers[FA_CHIN].SetFollowFlowReplacement(FA_MOUTH_LT,FA_MOUTH_RT,-1,-1);
	tFace.lst_ref_Markers[FA_NOSE].SetFollowFlowReplacement(-1,-1,-1);

	for (int k=0;k<FA_NUM_MARKERS;k++)	
	{
		tFace.lst_ref_Markers[k].bExclude=false;		
		tFace.lst_ref_Markers[k].fScaleX=1.0f;
		tFace.lst_ref_Markers[k].fScaleY=1.0f;

		tFace.lst_ref_Markers[k].fMinRangeX=tFace.lst_ref_Markers[k].fMinRangeY=9999.0f;
		tFace.lst_ref_Markers[k].fMaxRangeX=tFace.lst_ref_Markers[k].fMaxRangeY=-9999.0f;

		tFace.lst_ref_Markers[k].bFlipDirs=false;
		tFace.lst_ref_Markers[k].nAverage=-1;
		tFace.lst_ref_Markers[k].bExport=true;
	} //k
	
	//tFace.lst_ref_Markers[FA_LT_LIP_TOP].bExclude=true;
	//tFace.lst_ref_Markers[FA_RT_LIP_TOP].bExclude=true;
	//tFace.lst_ref_Markers[FA_LT_LIP_BOTTOM].bExclude=true;
	//tFace.lst_ref_Markers[FA_RT_LIP_BOTTOM].bExclude=true;

	tFace.lst_ref_Markers[FA_LT_CHEEK].bFlipDirs=true;
	tFace.lst_ref_Markers[FA_RT_CHEEK].bFlipDirs=true;
	tFace.lst_ref_Markers[FA_MOUTH_LT].bFlipDirs=true;
	tFace.lst_ref_Markers[FA_MOUTH_RT].bFlipDirs=true;
	tFace.lst_ref_Markers[FA_LT_NOSE].bFlipDirs=true;
	tFace.lst_ref_Markers[FA_RT_NOSE].bFlipDirs=true;

	tFace.lst_ref_Markers[FA_NOSE].bExport=false;

	tFace.lst_ref_Markers[FA_LT_NOSE].nAverage=FA_RT_NOSE;
	tFace.lst_ref_Markers[FA_RT_NOSE].nAverage=FA_LT_NOSE;

	if (!m_bCreateCalibrationData)
	{	
		// when creating calibration data, leave all settings to 1, don't load anything.
		// else load appropriate calibration file
		if (!LoadCalibrationFile(theApp.m_szCalibrationFile))
		{
			sprintf(szText,"Cannot load calibration file %s, reverting to default calibration file (%s)",theApp.m_szCalibrationFile,DEFAULT_CALIBRATION_FILE);
			AfxMessageBox(szText,MB_OK );

			strcpy(theApp.m_szCalibrationFile,DEFAULT_CALIBRATION_FILE);
			if (!LoadCalibrationFile(theApp.m_szCalibrationFile))
			{
				sprintf(szText,"Cannot load default calibration file (%s). Using default values.",DEFAULT_CALIBRATION_FILE);
				AfxMessageBox(szText,MB_OK );
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////

	CvScalar colors[] = 
	{
		{{0,0,255}},
		{{0,128,255}},
		{{0,255,255}},
		{{0,255,0}},
		{{255,128,0}},
		{{255,255,0}},
		{{255,0,0}},
		{{255,0,255}}
	};

	//////////////////////////////////////////////////////////////////////////
	// get reference frame and features
	IplImage *frame = NULL, *frame_edit=NULL, *frame1 = NULL, *frame11 = NULL, *frame2 = NULL, *frame2_1C = NULL, *eig_image = NULL, *temp_image = NULL, *pyramid1 = NULL, *pyramid2 = NULL;		
	IplImage	*subframeROI=NULL;
	long start_frame=0;
	int stop_frame=number_of_frames;

	//////////////////////////////////////////////////////////////////////////
		
	strcpy(szFilename,_szFilename);
	ReplaceExtension(szFilename,"txt");
	FILE *fp=fopen(szFilename,"wt");

	sprintf(szText,"// File %s\n",szFilename);
	fputs(szText,fp);
	sprintf(szText,"// Program version %d (compiled on %s -%s-)\n",FA_PROGRAM_VERSION,__DATE__, __TIME__);
	fputs(szText,fp);
	sprintf(szText,"// Using calibration file %s \n",theApp.m_szCalibrationFile);
	fputs(szText,fp);

	sprintf(szText,"VideoFile=%s Frames=%d FPS=%d Width=%d Height=%d\n",GetFilename(_szFilename),stop_frame-start_frame-1,fps,w,h);
	//sprintf(szText,"Frames=%d, FPS=%d, Width=%d, Height=%d\n",stop_frame-start_frame-1,fps,w,h);
	fputs(szText,fp);

	sprintf(szText,"Markers=%d\n",FA_NUM_MARKERS);
	fputs(szText,fp);
	for (int k=0;k<FA_NUM_MARKERS;k++)
	{
		sprintf(szText,"Joystick=%s Scale=(%0.6f,%0.6f)\n",tFace.lst_ref_Markers[k].szName,tFace.lst_ref_Markers[k].fScaleX,tFace.lst_ref_Markers[k].fScaleY);
		fputs(szText,fp);
	} //k

	//////////////////////////////////////////////////////////////////////////

	m_bEditMode=true;
	m_bDrawLabels=false;
	if (m_bBatchMode)
		m_bEditMode=false;

	//const int nMaxFeatures=192;	
	//const int nMaxFeatures=256;	
	const int nMaxFeatures=256+64;	

	/*
	//const double pi = 3.14159265358979323846;
	KLT_TrackingContext m_tc;
	KLT_FeatureList m_fl;
	KLT_FeatureTable m_ft;
	m_tc = KLTCreateTrackingContext();		
	m_fl = KLTCreateFeatureList(nMaxFeatures);
	m_ft = KLTCreateFeatureTable(number_of_frames, nMaxFeatures);  	
	m_tc->sequentialMode = TRUE;
	*/

	CvFont font1;
	cvInitFont( &font1, CV_FONT_HERSHEY_SIMPLEX, 0.35, 0.35);  		
	CvFont font2;
	cvInitFont( &font2, CV_FONT_HERSHEY_SIMPLEX, 1, 1);  		

	tFace.font1=font1;

	long current_frame=start_frame;
	long prev_frame=start_frame;		

	//////////////////////////////////////////////////////////////////////////

	// features found in frame 1
	CvPoint2D32f frame1_features[nMaxFeatures];

	// This array will contain the locations of the points from frame 1 in frame 2. 
	CvPoint2D32f frame2_features[nMaxFeatures];
	// The i-th element of this array will be non-zero if and only if the i-th feature of
	// frame 1 was found in frame 2.
	char optical_flow_found_feature[nMaxFeatures];
	// The i-th element of this array is the error in the optical flow for the i-th feature
	// of frame1 as found in frame 2.  
	float optical_flow_feature_error[nMaxFeatures];
	// This array stores the positions of the markers found
	//CvPoint2D32f lst_markers[FA_NUM_MARKERS];	

	// all features
	tFeatureProcessing *lstFrames=new tFeatureProcessing[number_of_frames];

	// Preparation: Allocate the necessary storage. 
	//allocateOnDemand2( &eig_image, frame_size, IPL_DEPTH_32F, 1 );
	//allocateOnDemand2( &temp_image, frame_size, IPL_DEPTH_32F, 1 );
	
	tFace.frame1_1C=NULL;
	tFace.frame2_1C=NULL;

	allocateOnDemand2( &frame1, frame_size, 8, 3 );
	allocateOnDemand2( &frame_edit, frame_size, 8, 3 );
	for (int k=0;k<number_of_frames;k++)
	{	
		tFeatureProcessing *tFrame=&lstFrames[k];
		tFrame->nFrame=k;
		tFrame->bReferenceFrame=false;
		tFrame->lst_Frame1_features=frame1_features;
		tFrame->lst_Frame2_features=frame2_features;
		tFrame->lst_optical_flow_found_feature=optical_flow_found_feature;
		tFrame->lst_optical_flow_feature_error=optical_flow_feature_error;
		tFrame->nFeatures=FA_NUM_MARKERS;
		tFrame->pFace=&tFace;		
		tFrame->pDrawingFrame=frame1;
		tFrame->pDrawingFrameEdit=frame_edit;
	} //k

	tFeatureProcessing *tFrame0=&lstFrames[start_frame];		
	tFrame0->bReferenceFrame=true;
	tFrame0->vRotation.Clear();
	
	// load obj with position and names for reference

	strcpy(szFilename,"faceaa.obj");
	MyOutputDebugString("Attempting to load %s\n",szFilename);
	if (!LoadOBJ(szFilename,&tFace))
	{	
		GetModuleFileName(AfxGetInstanceHandle(), szText, 250);
		FixPath(szText);
		strcpy(szFilename,GetPath(szText));
		strcat(szFilename,"faceaa.obj");	

		MyOutputDebugString("Attempting to load %s\n",szFilename);
		if (!LoadOBJ(szFilename,&tFace))
		{
			strcpy(szFilename,GetPath(_szFilename));
			FixPath(szFilename);
			strcat(szFilename,"faceaa.obj");	
			MyOutputDebugString("Attempting to load %s\n",szFilename);
			if (!LoadOBJ(szFilename,&tFace))
			{			
				_getcwd(szFilename,250);
				FixPath(szFilename);
				strcat(szFilename,"/faceaa.obj");	
				MyOutputDebugString("Attempting to load %s\n",szFilename);
				if (!LoadOBJ(szFilename,&tFace))
					MyError("Cannot load OBJ");
			}
		}
	}
	//////////////////////////////////////////////////////////////////////////
	// load .bmp with same filename as video, add to tface. This is the reference frame
	strcpy(szFilename,_szFilename);
	ReplaceExtension(szFilename,"bmp");
	MyOutputDebugString("Attempting to load %s\n",szFilename);
	IplImage	*pFrame=cvLoadImage(szFilename);

	tFace.pRefFrame=NULL;
	tFace.pRefFrame2=NULL;

	bool	bSaveFrame=true;
	bool	bCompute=false;

	if (pFrame)
	{		
		tFace.pRefFrame=pFrame;
		tFace.pRefFrame2=cvCreateImage(frame_size,8,3);
		cvConvertImage(tFace.pRefFrame,tFace.pRefFrame2);
		tFrame0->pDrawingFrame=tFace.pRefFrame2;
		bCompute=Calc3DInfo2(tFrame0);

		for (int k=0;k<FA_NUM_MARKERS;k++)
		{		
			sprintf(szText,"%s",tFace.lst_ref_Markers[k].szName);
			cvPutText( tFrame0->pDrawingFrame, szText,cvPoint((int)(tFace.lst_ref_Markers[k].v2DPos.x),(int)(tFace.lst_ref_Markers[k].v2DPos.y)), &font1, cvScalar(255,0,0) );
		} //k

		SaveTGA((uchar *)tFrame0->pDrawingFrame->imageData,3,w,h,"FaceMatching.tga",true,true);

		memset(optical_flow_found_feature,0,nMaxFeatures);

		FindOrientation2(lstFrames,current_frame+1,1);	
		FindOrientation2(lstFrames,current_frame,0);	

		bSaveFrame=false;		
	}
	else
		bCompute=true;

	memset(optical_flow_found_feature,0,nMaxFeatures);

	//////////////////////////////////////////////////////////////////////////

	tFace.nIter=0;
	float fDirX,fDirY; 
	bool bExportFrames=false;	
	bool bExportFrames2=false;	

	int nFlip=1;

	float fDirXPrev[256];
	float fDirYPrev[256];
	for (int k=0;k<256;k++)
		fDirXPrev[k]=fDirYPrev[k]=0;

	while(bCompute)
	{		
		tFeatureProcessing *tFrame=&lstFrames[current_frame];
		tFace.pCurrFrame=tFrame;

		cvSetCaptureProperty( input_video, CV_CAP_PROP_POS_FRAMES, current_frame );

		// first frame
		frame = cvQueryFrame( input_video );		

		// We'll make a full color backup of this frame so that we can draw on it.
		// (It's not the best idea to draw on the static memory space of cvQueryFrame().)		
		allocateOnDemand2( &tFace.frame1_1C, frame_size, 8, 1 );
		cvConvertImage(frame, tFace.frame1_1C, nFlip);		
		cvConvertImage(frame, frame1, nFlip);				
		cvConvertImage(frame, frame_edit, 0);				

		if (bSaveFrame)
		{ 
			char szFilenameTemp[256];
			//sprintf(szFilenameTemp,"Frame%03d.tga",current_frame);
			//SaveTGA((uchar *)frame1->imageData,3,w,h,szFilenameTemp,true,true);

			//sprintf(szFilenameTemp,"Frame%03d.bmp",current_frame);
			strcpy(szFilenameTemp,szFilename);
			ReplaceExtension(szFilenameTemp,"bmp");			
			cvSaveImage(szFilenameTemp,frame1);

			//sprintf(szText,"Frame %d saved",current_frame);
			//cvPutText( frame1, szText,cvPoint(10,60), &font1, cvScalar(0,0,255) );	
			cvShowImage(m_szWindowFilename, frame1);
			//cvWaitKey(0);
			bSaveFrame=false;
			break;
		}
 
		if (bExportFrames)
		{		
			sprintf(szText,"%d/%d",current_frame,stop_frame);
			cvPutText( frame1, szText,cvPoint(20,40), &font2, cvScalar(255,0,0) );

			char szFilenameTemp[256];
			sprintf(szFilenameTemp,"%sFrame%03d.jpg",GetPath(_szFilename),current_frame-start_frame);
			//SaveTGA((uchar *)frame->imageData,3,w,h,szFilenameTemp,false,true);
			cvSaveImage(szFilenameTemp,frame1);
		}

		// Allocate the necessary storage
		CvSize		ROIsize=cvSize(tFace.face_region.width,tFace.face_region.height);

		//allocateOnDemand2(&subframeROI,ROIsize,8,1);
		//allocateOnDemand2( &eig_image, ROIsize, IPL_DEPTH_32F, 1 );
		//allocateOnDemand2( &temp_image, ROIsize, IPL_DEPTH_32F, 1 );
		subframeROI=cvCreateImage(ROIsize,8,1);
		eig_image=cvCreateImage(ROIsize, IPL_DEPTH_32F, 1 );
		temp_image=cvCreateImage(ROIsize, IPL_DEPTH_32F, 1 );

		int number_of_features = nMaxFeatures;
		
		//int cx=(int)(tFrame->lst_markers[FA_NOSE].x);
		//int cy=(int)(tFrame->lst_markers[FA_NOSE].y);
		//CvRect ROI=cvRect(cx-tFace.face_region.x,cy-tFace.face_region.y,
		//	tFace.face_region.width,tFace.face_region.height);
		//cvDrawRect(tFrame->pDrawingFrame,cvPoint(cx-tFace.face_region.x,cy-tFace.face_region.y),
		//	cvPoint(cx-tFace.face_region.x+tFace.face_region.width,cy-tFace.face_region.y+tFace.face_region.height),
		//	colors[0]);
		cvDrawRect(tFrame->pDrawingFrame,cvPoint(tFace.face_region.x,tFace.face_region.y),
			cvPoint(tFace.face_region.x+tFace.face_region.width,tFace.face_region.y+tFace.face_region.height),
			colors[0]);

		// center for getting the image rect is not the nose center
		float cx=((tFace.face_region.x)+(tFace.face_region.x+tFace.face_region.width))/2.0f;
		float cy=((tFace.face_region.y)+(tFace.face_region.y+tFace.face_region.height))/2.0f;
		// need to be careful with calculation here...
		float x1=cx-tFace.face_region.width/2.0f;
		float y1=cy-tFace.face_region.height/2.0f;
		cvGetRectSubPix(tFace.frame1_1C,subframeROI,cvPoint2D32f(cx,cy));
		cvGoodFeaturesToTrack(subframeROI, eig_image, temp_image, frame1_features, &number_of_features, .01, .01, NULL);				
		//cvGoodFeaturesToTrack(tFace.frame1_1C, eig_image, temp_image, frame1_features, &number_of_features, .01, .01, NULL);		
		// put back the offset
		for (int i = 0; i < number_of_features; i++)
		{			
			frame1_features[i].x+=x1;
			frame1_features[i].y+=y1;
		} //i

		cvReleaseImage(&subframeROI);
		cvReleaseImage(&eig_image);
		cvReleaseImage(&temp_image);
		//////////////////////////////////////////////////////////////////////////
		// process the features		
		tFrame->nFeatures=number_of_features; 
		tFrame->pDrawingFrame=frame1;

		// refine positions found in the previous frame		
		FindOrientation2(lstFrames,current_frame,0);	

		// second frame
		frame = cvQueryFrame( input_video );		
		allocateOnDemand2( &frame2, frame_size, 8, 3 );
		allocateOnDemand2( &tFace.frame2_1C, frame_size, 8, 1 );
		cvConvertImage(frame, tFace.frame2_1C, nFlip);		
		cvConvertImage(frame, frame2, nFlip);		

		//CvSize optical_flow_window = cvSize(3,3);
		//CvSize optical_flow_window = cvSize(5,5);
		CvSize optical_flow_window = cvSize(7,7);

		CvTermCriteria optical_flow_termination_criteria
			//= cvTermCriteria( CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, .3 );
			= cvTermCriteria( CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, 0.03 );

		allocateOnDemand2( &pyramid1, frame_size, IPL_DEPTH_8U, 1 );
		allocateOnDemand2( &pyramid2, frame_size, IPL_DEPTH_8U, 1 );

		cvCalcOpticalFlowPyrLK(tFace.frame1_1C, tFace.frame2_1C, pyramid1, pyramid2, frame1_features, frame2_features, number_of_features, optical_flow_window, 4, optical_flow_found_feature, optical_flow_feature_error, optical_flow_termination_criteria, 0 );
				
		float fDirXAverage=0;
		float fDirYAverage=0;
		int nDirsAverage=0;
		float fLenAverage=0;
		
		for (int i = 0; i < number_of_features; i++)
		{			
			if ( optical_flow_found_feature[i] == 0 )	
				continue;

			int line_thickness;				line_thickness = 1;			
			CvScalar line_color;			line_color = CV_RGB(255,0,0);

			CvPoint p,q;
			p.x = (int) frame1_features[i].x;
			p.y = (int) frame1_features[i].y;
			q.x = (int) frame2_features[i].x;
			q.y = (int) frame2_features[i].y;

			float fDirX=frame2_features[i].x-frame1_features[i].x;
			float fDirY=frame2_features[i].y-frame1_features[i].y;			

			fDirXAverage+=fDirX;
			fDirYAverage+=fDirY;			
			fLenAverage+=sqrt(sqr(fDirX)+sqr(fDirY));
			nDirsAverage++;
		} //i
		fDirXAverage/=(float)(nDirsAverage);
		fDirYAverage/=(float)(nDirsAverage);
		fLenAverage/=(float)(nDirsAverage);
		

		for (int i = 0; i < number_of_features; i++)
		{			
			CvPoint p,q;
			p.x = (int) frame1_features[i].x;
			p.y = (int) frame1_features[i].y;

			cvCircle(tFrame->pDrawingFrame,p,2,CV_RGB(0,0,255),1);

			if ( optical_flow_found_feature[i] == 0 )	
				continue;

			q.x = (int) frame2_features[i].x;
			q.y = (int) frame2_features[i].y;

			float fDirX=frame2_features[i].x-frame1_features[i].x;
			float fDirY=frame2_features[i].y-frame1_features[i].y;			
			float fLen2=(sqr(fDirX)+sqr(fDirY));
			
			if (fLen2>(fLenAverage*fLenAverage)*6)
			{
				optical_flow_found_feature[i]=0;
				continue; // was a false positive
			}				

			cvCircle(tFrame->pDrawingFrame,q,2,CV_RGB(0,255,0),1);

			cvDrawLine(tFrame->pDrawingFrame,p,q,colors[0]);
		} //i

		//////////////////////////////////////////////////////////////////////////
		if (m_bEditMode)
		{			
			cvPutText( frame1, "EDIT MODE",cvPoint(60,40), &font1, cvScalar(255,0,0) );			
		}
		
		FindOrientation2(lstFrames,current_frame+1,1);	

		//////////////////////////////////////////////////////////////////////////

		sprintf(szText,"%d/%d - %0.4f",current_frame,stop_frame,(float)(tFace.fGlobalError));
		cvPutText( frame1, szText,cvPoint(10,20), &font1, cvScalar(255,0,0) );

		if (m_bDrawLabels)
		{		
			for (int k=0;k<FA_NUM_MARKERS;k++)
			{		
				sprintf(szText,"%s",tFace.lst_ref_Markers[k].szName);
				cvPutText( tFrame->pDrawingFrame, szText,cvPoint((int)(tFrame->lst_markers[k].x),(int)(tFrame->lst_markers[k].y)), &font1, cvScalar(255,0,0) );
			} //k
		}

		cvShowImage(m_szWindowFilename, tFrame->pDrawingFrame);
		//cvShowImage("FaceAnim", tFace.frame1_1C);

		// debug
		//char szFilenameTemp[256];
		//sprintf(szFilenameTemp,"%sFrameDebug%03d.jpg",GetPath(_szFilename),current_frame-start_frame);		
		//cvSaveImage(szFilenameTemp,tFrame->pDrawingFrame);


		MyOutputDebugString("Frame %d/%d\n",current_frame,stop_frame);

		/*
		if (tFace.fGlobalError>1 && tFace.nIter<8)
		{
			tFace.nIter++;
			cvWaitKey(1);			
			continue; // doing this on the same frame will refine results
		}
		*/

		int key_pressed;

		if (m_bEditMode)
			key_pressed =cvWaitKey(0);
		else
			key_pressed =cvWaitKey(1);

		if (key_pressed=='c' || key_pressed=='C')
		{
			char szFilenameTemp[256];
			sprintf(szFilenameTemp,"Frame%03d.tga",current_frame);
			SaveTGA((uchar *)frame->imageData,3,w,h,szFilenameTemp,false,true);
			//sprintf(szText,"Frame %d saved",current_frame);
			//cvPutText( frame1, szText,cvPoint(10,60), &font1, cvScalar(0,0,255) );

			continue;
		}

		if (key_pressed=='e' || key_pressed=='E')
		{
			// edit mode			
			m_bEditMode=!m_bEditMode;
		}

		if (key_pressed=='l' || key_pressed=='L')
		{
			// display lables mode			
			m_bDrawLabels=!m_bDrawLabels;
		}

		if (m_bEditOperation)
		{
			m_bEditOperation=false;
			continue;
		}

		tFace.nIter=0;
		int nStep=1;

		//if (key_pressed == 'b' || key_pressed == 'B')	
		//	current_frame-=nStep;
		//else											
		
		current_frame+=nStep;

		//current_frame+=9;			 
		// Don't run past the front/end of the AVI. 
		if (current_frame < 0)						
			current_frame = 0;
		if (current_frame >= stop_frame - 1)	
		{
			break;
			//current_frame = number_of_frames - 2;
			current_frame = 0;
			prev_frame=0;
		}

		//////////////////////////////////////////////////////////////////////////
		prev_frame=current_frame;

		sprintf(szText,"FrameStart %d/%d\n",current_frame,stop_frame);
		fputs(szText,fp);

		// in the head coordinate system
		// x is up/down
		// z is left/right
		// y is roll
		
		ftype fHeadScale=1.0;
		// in the joystick head, x is left/right, y is up/down				
		sprintf(szText,"Feature Head\n");
		fputs(szText,fp);
		// in the joystick, x is left/right, y is up/down		
		fDirX=(float)((tFrame->vRotation.z/4.0)*fHeadScale);
		fDirY=(float)(((tFrame->vRotation.x)/10.0)*fHeadScale);

		fDirX=(fDirX+fDirXPrev[FA_NUM_MARKERS+1])/2.0f;
		fDirY=(fDirY+fDirYPrev[FA_NUM_MARKERS+1])/2.0f;
		fDirX=-Clamp(fDirX,-1,1);
		fDirY=Clamp(fDirY,-1,1);
		sprintf(szText,"PositionJoystick=(%f,%f)\n",fDirX,fDirY);
		fputs(szText,fp);
		sprintf(szText,"Position2D=(%f,%f)\n",0.0f,0.0f);
		fputs(szText,fp);

		fDirXPrev[FA_NUM_MARKERS+1]=fDirX;
		fDirYPrev[FA_NUM_MARKERS+1]=fDirY;


		sprintf(szText,"Feature Lean\n");
		fputs(szText,fp);
		fDirY=0;
		fDirX=(float)((tFrame->vRotation.y/20.0)*fHeadScale);				
		fDirX=(fDirX+fDirXPrev[FA_NUM_MARKERS+2])/2.0f;
		fDirY=(fDirY+fDirYPrev[FA_NUM_MARKERS+2])/2.0f;
		fDirX=-Clamp(fDirX,-1,1);		
		fDirY=Clamp(fDirY,-1,1);
		sprintf(szText,"PositionJoystick=(%f,%f)\n",fDirX,fDirY);
		fputs(szText,fp);
		sprintf(szText,"Position2D=(%f,%f)\n",0.0f,0.0f);
		fputs(szText,fp);
		fDirXPrev[FA_NUM_MARKERS+2]=fDirX;
		fDirYPrev[FA_NUM_MARKERS+2]=fDirY;

		//////////////////////////////////////////////////////////////////////////		
		
		CMatrixf matrix2;
		matrix2.Identity();		
		matrix2.RotateMatrixf(tFrame->vRotation.x,1,0,0);	//pitch	
		matrix2.RotateMatrixf(tFrame->vRotation.z,0,1,0);	//roll
		matrix2.RotateMatrixf(tFrame->vRotation.y,0,0,1);	//yaw		
		
		CMatrixf matrix22;				
		matrix22=matrix2;
		// only rotation, no need to invert
		matrix22.Transpose();		
		//ftype			fScale=w>>6;		

		for (int k=0;k<FA_NUM_MARKERS;k++)
		{
			if (tFrame->pFace->lst_ref_Markers[k].bExport)
			{			
				sprintf(szText,"Feature %s\n",tFrame->pFace->lst_ref_Markers[k].szName);
				fputs(szText,fp);			
			}

			//ftype fDistA1=tFrame->pFace->lst_ref_Markers[k].v3DPos.Distance2(tFrame->pFace->lst_ref_Markers[FA_NOSE].v3DPos);
			//ftype fDistA2=tFrame->pFace->lst_3D_markers[k].Distance2(tFrame->pFace->lst_3D_markers[FA_NOSE]);
			//fDirX=0;
			//fDirY=((fDistA2-fDistA1))*tFrame->pFace->lst_ref_Markers[k].fScale;
			
			// NOTE: DONT OVERRIDE Z!
			/*
			tFrame->pFace->lst_ref_Markers[k].v2DPos.z=1;
			tFrame->pFace->lst_ref_Markers[FA_NOSE].v2DPos.z=1;
			ftype fDistA1=tFrame->pFace->lst_ref_Markers[k].v2DPos.Distance2(tFrame->pFace->lst_ref_Markers[FA_NOSE].v2DPos);
			tFrame->lst_markers[k].z=1;
			tFrame->lst_markers[FA_NOSE].z=1;
			ftype fDistA2=tFrame->lst_markers[k].Distance2(tFrame->lst_markers[FA_NOSE]);			
			
			fDirY=(float)((fDistA2-fDistA1)/fDistA1); //*tFrame->pFace->lst_ref_Markers[k].fScale;
			
			vector3f	v2=tFrame->lst_markers[k];v2.z=0;
			vector3f	v1=tFrame->lst_markers[FA_NOSE];v1.z=0;
			vector3f	vDir=v2-v1;vDir.Normalize();
			ftype			fDot=vDir*tFrame->pFace->lst_ref_Markers[k].vDir;
			fDot=Clamp(fDot,-1,1);
			ftype			fAng=RAD2DEG(acos(fDot));
			fDirX=(float)(fAng/(90.0/4.0));
			*/
			
			/*
			vector3f v11=tFrame->pFace->lst_ref_Markers[k].v3DPos;
			// remove rotation			
			vector3f vPos1=matrix2.TransformPoint(v11);
			vector3f vC1;
			ProjectToScreen(vPos1,vC1,tFrame);
			vector3f vDest=tFrame->lst_markers[k];
						
			// calc movement
			CvPoint p,q;
			p.x = (int) vDest.x;
			p.y = (int) vDest.y;
			cvCircle(tFrame->pDrawingFrame,p,3,CV_RGB(0,255,0), -1);

			// original point
			q.x = (int) vC1.x;
			q.y = (int) vC1.y;
			cvCircle(tFrame->pDrawingFrame,q,3,CV_RGB(128,0,0), -1);
			
			fDirX=(float)((vDest.x-vC1.x)/fScale);
			fDirY=(float)((vC1.y-vDest.y)/fScale);
			*/


			// rotate in the head direction
			vector3f v11=tFrame->pFace->lst_ref_Markers[k].v3DPos;
			vector3f v1=matrix2.TransformPoint(v11);
			// get z
			vector3f vC1;
			ProjectToScreen(v1,vC1,tFrame);
			// get 3d point back
			v11.x=tFrame->lst_markers[k].x;v11.y=tFrame->lst_markers[k].y;v11.z=vC1.z;
			UnProjectFromScreen(v11,v1,tFrame);
			// get center
			v11=tFrame->pFace->lst_ref_Markers[FA_NOSE].v3DPos;
			vector3f v2=matrix2.TransformPoint(v11);
			ProjectToScreen(v2,vC1,tFrame);			
			// get 3d point back
			v11.x=tFrame->lst_markers[FA_NOSE].x;v11.y=tFrame->lst_markers[FA_NOSE].y;v11.z=vC1.z;
			UnProjectFromScreen(v11,v2,tFrame);

			// calc ratio
			ftype fDistA1=tFrame->pFace->lst_ref_Markers[k].v3DPos.Distance2(tFrame->pFace->lst_ref_Markers[FA_NOSE].v3DPos);
			ftype fDistA2=v1.Distance2(v2);

			fDirY=(float)((fDistA2-fDistA1)/fDistA1);

			// we have 3d points being rotate. 
			// remove the rotation
			vector3f v1N=matrix22.TransformPoint(v1);
			vector3f v2N=matrix22.TransformPoint(v2); // nose

			vector3f vDir=v1N-v2N;
			vDir.Normalize();
			vector3f vOut(0,0,1);
			vector3f vDir2=vDir^vOut;			

			vDir=tFrame->pFace->lst_ref_Markers[k].v3DPos-tFrame->pFace->lst_ref_Markers[FA_NOSE].v3DPos;
			vDir.Normalize();
			
			ftype			fDot=(vDir*vDir2)*2.0;
			fDot=Clamp(fDot,-1,1);
			//ftype			fAng=RAD2DEG(acos(fDot));
			//fDirX=(float)(fAng/(90.0/4.0));
			fDirX=(float)(fDot*2.0);

			/*
			ProjectToScreen(v1N,vC1,tFrame);
			CvPoint p,q;
			p.x = (int) vC1.x;
			p.y = (int) vC1.y;
			vector3f vPos=v1N+(vDir2*0.025);
			ProjectToScreen(vPos,vC1,tFrame);
			q.x = (int) vC1.x;
			q.y = (int) vC1.y;
			if (fDirX>0)
				cvDrawLine(tFrame->pDrawingFrame,p,q,CV_RGB(0,255,0));
			else
				cvDrawLine(tFrame->pDrawingFrame,p,q,CV_RGB(255,0,0));
			//cvCircle(tFrame->pDrawingFrame,p,3,CV_RGB(0,255,0), -1);			
			*/						

			vector3f v111=tFrame->pFace->lst_ref_Markers[k].v2DPos;v111.z=1;
			vector3f v222=tFrame->pFace->lst_ref_Markers[FA_NOSE].v2DPos;v222.z=1;
			ftype fDistA11=tFrame->pFace->lst_ref_Markers[k].v2DPos.Distance2(tFrame->pFace->lst_ref_Markers[FA_NOSE].v2DPos);
			v111=tFrame->lst_markers[k]; v111.z=1;
			v222=tFrame->lst_markers[FA_NOSE]; v222.z=1;
			ftype fDistA22=v111.Distance2(v222);			

			if (tFrame->pFace->lst_ref_Markers[k].nAverage>=0)
			{
				int nOther=tFrame->pFace->lst_ref_Markers[k].nAverage;
				vector3f v1111=tFrame->pFace->lst_ref_Markers[nOther].v2DPos;v1111.z=1;
				vector3f v2222=tFrame->pFace->lst_ref_Markers[FA_NOSE].v2DPos;v2222.z=1;
				ftype fDistA111=tFrame->pFace->lst_ref_Markers[nOther].v2DPos.Distance2(tFrame->pFace->lst_ref_Markers[FA_NOSE].v2DPos);
				v1111=tFrame->lst_markers[nOther]; v1111.z=1;
				v2222=tFrame->lst_markers[FA_NOSE]; v2222.z=1;
				ftype fDistA222=v1111.Distance2(v2222);			

				ftype fAverage=(fDistA22+fDistA222)/2.0;
				fDistA22=fAverage;
				//ftype fDiff=fAverage-fDistA22;
				//fDistA22+=fDiff*0.999995;
			}

			fDirY=(float)((fDistA22-fDistA11)/fDistA11); //*tFrame->pFace->lst_ref_Markers[k].fScale;

			if (k==FA_NOSE || tFrame->pFace->lst_ref_Markers[k].bExclude)
			{
				fDirX=0;
				fDirY=0;
			}
			else
			if (tFrame->pFace->lst_ref_Markers[k].bFlipDirs)
			{
				// works better when swapped
				float fTemp=fDirX;
				fDirX=fDirY;
				fDirY=fTemp;
			}

			fDirX=(fDirX+fDirXPrev[k])/2.0f;

			if (k!=FA_CHIN)
			{							
				fDirY=(fDirY+fDirYPrev[k])/2.0f;
			}
			else
			if (k==FA_CHIN)
			{
				if (fDirY>0)
					fDirY-=0.15f;

				if (fDirY<0)
					fDirY=(fDirY+fDirYPrev[k])/2.0f;
			}

			// store values for averaging before scaling otherwise they grow bigger and bigger
			fDirXPrev[k]=fDirX;
			fDirYPrev[k]=fDirY;
 
			//fDirX=Clamp((float)(fDirX*tFrame->pFace->lst_ref_Markers[k].fScaleX),-1,1);
			//fDirY=Clamp((float)(fDirY*tFrame->pFace->lst_ref_Markers[k].fScaleY),-1,1);

			fDirX=((float)(fDirX*tFrame->pFace->lst_ref_Markers[k].fScaleX));
			fDirY=((float)(fDirY*tFrame->pFace->lst_ref_Markers[k].fScaleY));

			if (m_bCreateCalibrationData)
			{			
				if (fDirX<tFrame->pFace->lst_ref_Markers[k].fMinRangeX)
					tFrame->pFace->lst_ref_Markers[k].fMinRangeX=fDirX;
				if (fDirX>tFrame->pFace->lst_ref_Markers[k].fMaxRangeX)
					tFrame->pFace->lst_ref_Markers[k].fMaxRangeX=fDirX;

				if (fDirY<tFrame->pFace->lst_ref_Markers[k].fMinRangeY)
					tFrame->pFace->lst_ref_Markers[k].fMinRangeY=fDirY;
				if (fDirY>tFrame->pFace->lst_ref_Markers[k].fMaxRangeY)
					tFrame->pFace->lst_ref_Markers[k].fMaxRangeY=fDirY;
			}

			if (tFrame->pFace->lst_ref_Markers[k].bExport)
			{			
				sprintf(szText,"PositionJoystick=(%f,%f)\n",fDirX,fDirY);
				fputs(szText,fp);

				float fX=(float)tFrame->lst_markers[k].x;
				float fY=(float)tFrame->lst_markers[k].y;
				sprintf(szText,"Position2D=(%f,%f)\n",fX,fY);
				fputs(szText,fp);
			}
		} //k

		//cvShowImage("FaceAnim", tFrame->pDrawingFrame);
		//////////////////////////////////////////////////////////////////////////

		sprintf(szText,"FrameEnd %d/%d\n",current_frame,stop_frame);
		fputs(szText,fp);

		//////////////////////////////////////////////////////////////////////////

		if (bExportFrames2)
		{		
			sprintf(szText,"%d/%d",current_frame,stop_frame);
			cvPutText( frame1, szText,cvPoint(20,40), &font2, cvScalar(255,0,0) );

			char szFilenameTemp[256];
			sprintf(szFilenameTemp,"%sFrame%03d.jpg",GetPath(_szFilename),current_frame-start_frame);
			//SaveTGA((uchar *)frame->imageData,3,w,h,szFilenameTemp,false,true);
			cvSaveImage(szFilenameTemp,tFrame->pDrawingFrame);
		}

		if (key_pressed=='x' || key_pressed=='X')
			break;

	}

	//KLTFreeFeatureTable(m_ft);	
	//KLTFreeFeatureList(m_fl);	
	//KLTFreeTrackingContext(m_tc);		

	sprintf(szText,"End of file\n");
	fputs(szText,fp);
	fclose(fp);

	//////////////////////////////////////////////////////////////////////////
	if (m_bCreateCalibrationData)
	{
		FILE *fp=fopen(theApp.m_szCalibrationFile,"wt");
		sprintf(szText,"Markers=%d\n",FA_NUM_MARKERS);
		fputs(szText,fp);
		for (int k=0;k<FA_NUM_MARKERS;k++)
		{
			ftype fRangeX=(tFace.lst_ref_Markers[k].fMaxRangeX-tFace.lst_ref_Markers[k].fMinRangeX);
			if (fRangeX==0)
				fRangeX=1;

			ftype fRangeY=(tFace.lst_ref_Markers[k].fMaxRangeY-tFace.lst_ref_Markers[k].fMinRangeY);
			if (fRangeY==0)
				fRangeY=1;

			float fScaleX=(float)(abs(1.0/(fRangeX)));
			float fScaleY=(float)(abs(1.0/(fRangeY)));
			sprintf(szText,"Joystick=%s Scale=(%0.6f,%0.6f)\n",tFace.lst_ref_Markers[k].szName,fScaleX,fScaleY);
			fputs(szText,fp);
		} //k
		fclose(fp);
	}

	// save the last calib file used
	// create config file
	fp=fopen("config.ini","wt");	
	if (fp)
	{	
		sprintf(szText,"CalibrationFile %s\n",theApp.m_szCalibrationFile);
		fputs(szText,fp);		
		fclose(fp);
	}
	//////////////////////////////////////////////////////////////////////////
	//cvReleaseMemStorage(&storage);
	
	// (It's not the best idea to draw on the static memory space of cvQueryFrame().)
	//cvReleaseImage(&frame);
	cvReleaseImage(&frame1);
	cvReleaseImage(&frame2);
	cvReleaseImage(&frame_edit);

	cvReleaseImage(&tFace.frame1_1C);
	cvReleaseImage(&tFace.frame2_1C);
	cvReleaseImage(&eig_image);
	cvReleaseImage(&temp_image);

	cvvDestroyWindow(m_szWindowFilename);
	cvReleaseCapture(&input_video);

	SAFE_DELETE_ARRAY(lstFrames);

	cvReleaseImage(&tFace.pRefFrame);
	cvReleaseImage(&tFace.pRefFrame2);
	cvReleaseImage(&subframeROI);

	SAFE_DELETE_ARRAY(tFace.lst_vModel_3D);
	SAFE_DELETE_ARRAY(tFace.lst_Tris);
	SAFE_DELETE_ARRAY(tFace.lst_vModel_2D);

	return (true);
}

