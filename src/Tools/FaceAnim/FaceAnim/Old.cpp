


//////////////////////////////////////////////////////////////////////////
bool CFaceAnimDoc::CalibrationAVI3(const char *_szFilename)
{	
	// Create an object that decodes the input video stream. 
	CvCapture *input_video = cvCaptureFromAVI(_szFilename);
	if (!input_video)
	{
		// Either the video didn't exist OR it uses a codec OpenCV
		// doesn't support.					
		MyError("Cannot open file %s \n",_szFilename);
		return false;
	}

	// This is a workaround. If we don't call this first then getting capture
	// properties (below) won't work right. This is an OpenCV bug. We
	// ignore the return value here. But it's actually a video frame.	
	cvQueryFrame( input_video );

	// Read the video's frame size out of the AVI. 
	CvSize frame_size;
	int w=frame_size.width =(int) cvGetCaptureProperty( input_video, CV_CAP_PROP_FRAME_WIDTH );
	int h=frame_size.height =(int) cvGetCaptureProperty( input_video, CV_CAP_PROP_FRAME_HEIGHT );	
	int fps=(int) cvGetCaptureProperty( input_video, CV_CAP_PROP_FPS );	
	//CvSize frame_size_small;
	//frame_size_small.width=320;
	//frame_size_small.height=240;

	// Determine the number of frames in the AVI. 
	long number_of_frames;
	// Go to the end of the AVI (ie: the fraction is "1") 
	cvSetCaptureProperty( input_video, CV_CAP_PROP_POS_AVI_RATIO, 1. );
	// Now that we're at the end, read the AVI position in frames 
	number_of_frames = (int) cvGetCaptureProperty( input_video, CV_CAP_PROP_POS_FRAMES );
	// Return to the beginning 
	cvSetCaptureProperty( input_video, CV_CAP_PROP_POS_FRAMES, 0. );

	cvNamedWindow("FaceAnim", CV_WINDOW_AUTOSIZE);


	//////////////////////////////////////////////////////////////////////////
	CvMemStorage* storage = 0;
	CvHaarClassifierCascade* cascadeface = 0;
	//const char *cascade_name = "data/haarcascades/haarcascade_frontalface_alt2.xml";

	// with MFC is returning the path where the file is loaded from
	char	szFilename[256];
	char	szFolder[128];
	_getcwd( szFolder, sizeof(szFolder) - 1 );
	szFolder[128-1] = 0;	
	char *szFix=strstr(szFolder,"Bin");
	if (szFix)
		szFix[3]=0;
	szFix=szFolder;
	while (*szFix++) // fix the path
	{
		if (*szFix=='\\')
			*szFix='/';		
	}

	storage = cvCreateMemStorage(0);

	const char *cascadeface_name = "Data/haarcascades/haarcascade_frontalface_alt2.xml";
	sprintf(szFilename,"%s/%s",szFolder,cascadeface_name);
	cascadeface = (CvHaarClassifierCascade*)cvLoad( szFilename, 0, 0, 0 );	

	//const char *cascadeeyes_name = "Data/haarcascades/parojos.xml";
	//sprintf(szFilename,"%s/%s",szFolder,cascadeeyes_name);
	//CvHaarClassifierCascade*cascadeeyes = (CvHaarClassifierCascade*)cvLoad( szFilename, 0, 0, 0 );	

	const char *cascade_righteye_name = "Data/haarcascades/ojoD.xml";
	sprintf(szFilename,"%s/%s",szFolder,cascade_righteye_name);
	CvHaarClassifierCascade *cascade_righteye = (CvHaarClassifierCascade*)cvLoad( szFilename, 0, 0, 0 );	

	const char *cascade_lefteye_name = "Data/haarcascades/ojoI.xml";
	sprintf(szFilename,"%s/%s",szFolder,cascade_lefteye_name);
	CvHaarClassifierCascade *cascade_lefteye = (CvHaarClassifierCascade*)cvLoad( szFilename, 0, 0, 0 );	

	const char *cascade_mouth_name = "Data/haarcascades/boca.xml";
	//const char *cascade_mouth_name = "Data/haarcascades/haarcascade_profileface.xml";
	sprintf(szFilename,"%s/%s",szFolder,cascade_mouth_name);
	CvHaarClassifierCascade *cascade_mouth = (CvHaarClassifierCascade*)cvLoad( szFilename, 0, 0, 0 );	

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
	IplImage *frame = NULL, *frame1 = NULL, *frame11 = NULL, *frame2 = NULL, *frame2_1C = NULL, *eig_image = NULL, *temp_image = NULL, *pyramid1 = NULL, *pyramid2 = NULL;


	bool bFeatureDebugDraw = true;   // Debug draw for feature processing

	tFaceDetect	tFace;
	tFace.cascade_lefteye=cascade_lefteye;
	tFace.cascade_righteye=cascade_righteye;
	tFace.cascade_mouth=cascade_mouth;
	tFace.cascadeface=cascadeface;
	tFace.frame_size=frame_size;
	tFace.storage=storage;

	//////////////////////////////////////////////////////////////////////////

	tFace.frame1_1C=cvCreateImage( tFace.frame_size, 8, 1 );	
	tFace.frame2_1C=cvCreateImage( tFace.frame_size, 8, 1 );	
	frame11=cvCreateImage( tFace.frame_size, 8, 1 );	

	long start_frame=0;
	int stop_frame=number_of_frames;

	start_frame=1; //bender
	stop_frame=150; //bender	

	//start_frame=129; //ronin
	//stop_frame=325; //ronin

	//////////////////////////////////////////////////////////////////////////

	char szText[256];
	strcpy(szFilename,_szFilename);
	ReplaceExtension(szFilename,"txt");
	FILE *fp=fopen(szFilename,"wt");
	sprintf(szText,"Frames=%d, FPS=%d\n",number_of_frames,fps);
	fputs(szText,fp);

	//////////////////////////////////////////////////////////////////////////

	const int nMaxFeatures=128;	

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

	float fDirX=0;
	float fDirY=0;
	float fDirXHeadPrev=0;
	float fDirYHeadPrev=0;		
	float fDirXMouthPrev=0;
	float fDirYMouthPrev=0;		
	float fDirXLeanPrev=0;
	float fDirYLeanPrev=0;		
	float fDirXMouthFrownPrev=0;
	float fDirYMouthFrownPrev=0;		
	float fDirXLipsPrev=0;
	float fDirYLipsPrev=0;		
	//long current_frame = 0;	
	//int prev_frame=0;

	long current_frame=start_frame;
	long prev_frame=start_frame;		

	//////////////////////////////////////////////////////////////////////////
	// initial face
	cvSetCaptureProperty( input_video, CV_CAP_PROP_POS_FRAMES, start_frame );
	// first frame
	frame = cvQueryFrame( input_video );		
	allocateOnDemand2( &frame1, frame_size, 8, 3 );
	cvConvertImage(frame, frame1, 1);			
	tFace.bFlip=false;
	if (!DetectFace(frame1,tFace))
		return (false);	
	tFace.ref_face=tFace.prev_face=tFace.face;
	tFace.ref_eye_left=tFace.prev_eye_left=tFace.eye_left;
	tFace.ref_eye_right=tFace.prev_eye_right=tFace.eye_right;
	tFace.ref_mouth=tFace.prev_mouth=tFace.mouth;

	// features found in frame 1 - copy to exclude tracking those outside the face
	CvPoint2D32f frame11_features[nMaxFeatures];

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
	allocateOnDemand2( &eig_image, frame_size, IPL_DEPTH_32F, 1 );
	allocateOnDemand2( &temp_image, frame_size, IPL_DEPTH_32F, 1 );
	int number_of_features;

	number_of_features = nMaxFeatures;

	cvConvertImage(frame, tFace.frame1_1C, 1);
	cvGoodFeaturesToTrack(tFace.frame1_1C, eig_image, temp_image, frame1_features, &number_of_features, .01, .01, NULL);

	for (int i = 0; i < number_of_features; i++)
	{			
		int line_thickness;				line_thickness = 1;			
		CvScalar line_color;			line_color = CV_RGB(255,0,0);

		CvPoint p;
		p.x = (int) frame1_features[i].x;
		p.y = (int) frame1_features[i].y;

		float fDist2=sqr((float)(p.x-tFace.face_center.x))+sqr((float)(p.y-tFace.face_center.y));
		if (fDist2>tFace.radius*tFace.radius)					
		{
			optical_flow_found_feature[i]=0;
			continue;		
		}

		cvRectangle(frame1,cvPoint((int)(p.x)-1,(int)(p.y)-1),cvPoint((int)(p.x)+1,(int)(p.y)+1),CV_RGB(0,0,255));

		optical_flow_found_feature[i]=1;				
	} //i

	for (int k=0;k<number_of_frames;k++)
	{	
		tFeatureProcessing *tFrame=&lstFrames[k];
		tFrame->nFrame=k;
		tFrame->bReferenceFrame=false;
		tFrame->lst_Frame1_features=frame1_features;
		tFrame->lst_Frame2_features=frame2_features;
		tFrame->lst_optical_flow_found_feature=optical_flow_found_feature;
		tFrame->lst_optical_flow_feature_error=optical_flow_feature_error;
		tFrame->nFeatures=number_of_features;
		tFrame->pFace=&tFace;
		//tFrame0->lst_markers=lst_markers;
		tFrame->pDrawingFrame=frame1;
	} //k

	tFeatureProcessing *tFrame0=&lstFrames[start_frame];		
	tFrame0->bReferenceFrame=true;
	// find initial markers precisely, we have automatically detected mouth and eyes in this frame,
	// so use this info
	FindOrientation(lstFrames,start_frame,1);	
	// refine positions
	FindOrientation(lstFrames,start_frame,0);	

	DrawRectangle(frame1,tFace.face,colors[0]);
	DrawRectangle(frame1,tFace.eye_left,colors[1]);
	DrawRectangleCenter(frame1,tFace.eye_left,colors[4]);
	DrawRectangle(frame1,tFace.eye_right,colors[2]);
	DrawRectangleCenter(frame1,tFace.eye_right,colors[4]);
	DrawRectangle(frame1,tFace.mouth,colors[3]);
	SaveTGA((uchar *)frame1->imageData,3,w,h,"FaceDetected.tga",true,true);	


	//////////////////////////////////////////////////////////////////////////

	while(true)
	{		

		cvSetCaptureProperty( input_video, CV_CAP_PROP_POS_FRAMES, current_frame );

		// first frame
		frame = cvQueryFrame( input_video );		

		// We'll make a full color backup of this frame so that we can draw on it.
		// (It's not the best idea to draw on the static memory space of cvQueryFrame().)
		allocateOnDemand2( &frame1, frame_size, 8, 3 );
		//cvConvertImage(frame, frame1, CV_CVTIMG_FLIP);		
		cvConvertImage(frame, frame1, 1);		
		cvConvertImage(frame, tFace.frame1_1C, 1);
		// NOTE: detect face is modyfing the greyscale image - make a copy of the first frame
		//cvConvertImage(frame, frame11, 1);
		cvCopyImage(tFace.frame1_1C, frame11);

		// second frame
		frame = cvQueryFrame( input_video );		
		cvConvertImage(frame, tFace.frame2_1C, 1);
		// the other gray image is used 
		//uchar *pGray1=(uchar *)tFace.frame2_1C->imageData;		

		// Allocate the necessary storage
		allocateOnDemand2( &eig_image, frame_size, IPL_DEPTH_32F, 1 );
		allocateOnDemand2( &temp_image, frame_size, IPL_DEPTH_32F, 1 );

		tFace.bFlip=false;
		DetectFace(frame1,tFace);			
		tFace.prev_face=tFace.face;
		tFace.prev_eye_left=tFace.eye_left;
		tFace.prev_eye_right=tFace.eye_right;
		tFace.prev_mouth=tFace.mouth;


		// before the function call this variable is the array size
		// (or the maximum number of features to find).  
		// after the function call
		// this variable is the number of features actually found.		
		number_of_features = nMaxFeatures;

		// "frame1_1C" is the input image (grayscale)
		// "eig_image" and "temp_image" are just workspace for the algorithm.
		// The first ".01" specifies the minimum quality of the features (based on the eigenvalues).
		// The second ".01" specifies the minimum Euclidean distance between features.
		// "NULL" means use the entire input image.  
		// returns:
		// "frame1_features" will contain the feature points.
		// "number_of_features" will be set to a value <= nMaxFeatures indicating the number of feature points found.
		cvGoodFeaturesToTrack(frame11, eig_image, temp_image, frame11_features, &number_of_features, .01, .01, NULL);

		// copy back to the list only the interesting features
		int nID=0;
		for (int i = 0; i < number_of_features; i++)
		{						
			float x =frame11_features[i].x;
			float y =frame11_features[i].y;

			float fDist2=sqr((float)(x-tFace.face_center.x))+sqr((float)(y-tFace.face_center.y));
			if (fDist2>tFace.radius*tFace.radius)							
				continue;

			frame1_features[nID]=frame11_features[i];
			nID++;
		} //i

		number_of_features=nID;


		//////////////////////////////////////////////////////////////////////////
		// process the features
		tFeatureProcessing *tFrame=&lstFrames[current_frame];
		tFrame->nFeatures=number_of_features;
		//tFrame->lst_markers=lst_markers;
		tFrame->pDrawingFrame=(bFeatureDebugDraw ? frame1 : NULL);

		tFrame->pDrawingFrame=frame1;
		// refine positions found in the previous frame
		FindOrientation(lstFrames,current_frame,0);	

		//ProcessFeatures(lstFrames, current_frame);

		//////////////////////////////////////////////////////////////////////////


		// This is the window size to use to avoid the aperture problem 
		CvSize optical_flow_window = cvSize(3,3);

		// This termination criteria tells the algorithm to stop when it has either done 20 iterations or when
		// epsilon is better than .3.  You can play with these parameters for speed vs. accuracy but these values
		// work pretty well in many situations.

		CvTermCriteria optical_flow_termination_criteria
			= cvTermCriteria( CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, .3 );

		// This is some workspace for the algorithm.
		// (The algorithm actually carves the image into pyramids of different resolutions.)		
		allocateOnDemand2( &pyramid1, frame_size, IPL_DEPTH_8U, 1 );
		allocateOnDemand2( &pyramid2, frame_size, IPL_DEPTH_8U, 1 );

		// "frame1_1C" is the first frame with the known features.
		// "frame2_1C" is the second frame where we want to find the first frame's features.
		// "pyramid1" and "pyramid2" are workspace for the algorithm.
		// "frame1_features" are the features from the first frame.
		// "frame2_features" is the (outputted) locations of those features in the second frame.
		// "number_of_features" is the number of features in the frame1_features array.
		// "optical_flow_window" is the size of the window to use to avoid the aperture problem.
		// "5" is the maximum number of pyramids to use.  0 would be just one level.
		// "optical_flow_found_feature" is as described above (non-zero iff feature found by the flow).
		// "optical_flow_feature_error" is as described above (error in the flow for this feature).
		// "optical_flow_termination_criteria" is as described above (how long the algorithm should look).
		// "0" means disable enhancements.  (For example, the second aray isn't pre-initialized with guesses.)
		cvCalcOpticalFlowPyrLK(frame11, tFace.frame2_1C, pyramid1, pyramid2, frame1_features, frame2_features, number_of_features, optical_flow_window, 5, optical_flow_found_feature, optical_flow_feature_error, optical_flow_termination_criteria, 0 );

		//////////////////////////////////////////////////////////////////////////
		DrawRectangle(frame1,tFace.face,colors[0]);
		//DrawRectangle(frame1,tFace.eye_left,colors[1]);
		//DrawRectangle(frame1,tFace.eye_right,colors[2]);
		//DrawRectangle(frame1,tFace.mouth,colors[3]);

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
			float fLen2=(sqr(fDirX)+sqr(fDirY));

			if (fLen2>(fLenAverage*fLenAverage)*4)
			{
				optical_flow_found_feature[i]=0;
				continue; // was a false positive
			}			

			cvRectangle(frame1,cvPoint((int)(p.x)-1,(int)(p.y)-1),cvPoint((int)(p.x)+1,(int)(p.y)+1),CV_RGB(0,0,255));
			//cvRectangle(frame1,cvPoint((int)(q.x)-1,(int)(q.y)-1),cvPoint((int)(q.x)+1,(int)(q.y)+1),colors[1]);

			//cvDrawLine(frame1,p,q,colors[1]);
		} //i

		tFrame->nFeatures=number_of_features;
		tFrame->pDrawingFrame=frame1;
		tFrame->bReferenceFrame=false;						
		FindOrientation(lstFrames,current_frame+1,1);

		float fDirX,fDirY;

		prev_frame=current_frame;
		sprintf(szText,"%d/%d",current_frame,stop_frame);
		cvPutText( frame1, szText,cvPoint(10,20), &font1, cvScalar(255,0,0) );

		sprintf(szText,"FrameStart %d/%d\n",current_frame,stop_frame);
		fputs(szText,fp);


		// in the head coordinate system
		// x is up/down
		// z is left/right
		// y is roll

		// now the joysticks are not really setup with 
		// the same scale in mind. so do some adjustement
		// and scaling which should be done in the engine
		float fScale=0.15f;
		sprintf(szText,"Feature Head\n");
		fputs(szText,fp);
		// in the joystick, x is left/right, y is up/down		
		fDirX=(float)(tFrame->vRotation.z/15.0);
		fDirY=(float)((tFrame->vRotation.x-2.5)/15.0);
		fDirX=-Clamp(fDirX,-1,1);
		fDirY=Clamp(fDirY,-1,1);
		sprintf(szText,"Position=(%f,%f)\n",fDirX,fDirY);
		fputs(szText,fp);

		sprintf(szText,"Feature Lean\n");
		fputs(szText,fp);
		fDirY=0;
		fDirX=(float)(tFrame->vRotation.y/20.0);		
		fDirX=-Clamp(fDirX,-1,1);
		fDirY=Clamp(fDirY,-1,1);
		sprintf(szText,"Position=(%f,%f)\n",fDirX,fDirY);
		fputs(szText,fp);

		sprintf(szText,"Feature Mouth\n");
		fputs(szText,fp);
		ftype fDistA1=tFrame->pFace->ref_lst_3D_dist2[FA_MOUTH_LT];
		ftype fDistA2=tFrame->pFace->ref_lst_3D_dist2[FA_MOUTH_RT];
		ftype fDist11=((fDistA1+fDistA2)*0.5);
		ftype fDistB1=tFrame->pFace->lst_3D_dist2[FA_MOUTH_LT];
		ftype fDistB2=tFrame->pFace->lst_3D_dist2[FA_MOUTH_RT];
		ftype fDist22=((fDistB1+fDistB2)*0.5);
		fDirY=(float)(fDist22-fDist11)*50;
		fDirX=(float)((fDistB2-fDistA2)-((fDistB1-fDistA1)))*10;
		fDirX=Clamp(fDirX,-1,1);
		fDirY=-Clamp(fDirY,-1,1);
		sprintf(szText,"Position=(%f,%f)\n",fDirX,fDirY);
		fputs(szText,fp);

		sprintf(szText,"Feature Mouth_pucker_suck v Mouth_blow\n");
		fputs(szText,fp);
		fDistA1=tFrame->pFace->ref_lst_3D_markers[FA_MOUTH_LT].Distance2(tFrame->pFace->ref_lst_3D_markers[FA_MOUTH_RT]);
		fDistA2=tFrame->pFace->lst_3D_markers[FA_MOUTH_LT].Distance2(tFrame->pFace->lst_3D_markers[FA_MOUTH_RT]);
		fDirY=(float)(fDistA1-fDistA2)*10;
		fDirX=0;
		fDirX=Clamp(fDirX,-1,1);
		fDirY=-Clamp(fDirY,-1,1);
		sprintf(szText,"Position=(%f,%f)\n",fDirX,fDirY);
		fputs(szText,fp);


		sprintf(szText,"Feature Eyebrows\n");
		fputs(szText,fp);
		fDistA1=tFrame->pFace->ref_lst_3D_dist2[FA_LT_EYEBROW_RT];
		fDistA2=tFrame->pFace->lst_3D_dist2[FA_LT_EYEBROW_RT];		
		fDist11=(float)(fDistA1-fDistA2);
		fDistB1=tFrame->pFace->ref_lst_3D_dist2[FA_RT_EYEBROW_LT];
		fDistB2=tFrame->pFace->lst_3D_dist2[FA_RT_EYEBROW_LT];		
		fDist22=(float)(fDistB1-fDistB2);		
		fDirY=(float)((fDist11+fDist22)*0.5)*80;

		fDistA1=tFrame->pFace->ref_lst_3D_dist2[FA_LT_EYEBROW_LT];
		fDistA2=tFrame->pFace->lst_3D_dist2[FA_LT_EYEBROW_LT];		
		fDist11=(float)(fDistA1-fDistA2);
		fDistB1=tFrame->pFace->ref_lst_3D_dist2[FA_RT_EYEBROW_RT];
		fDistB2=tFrame->pFace->lst_3D_dist2[FA_RT_EYEBROW_RT];		
		fDist22=(float)(fDistB1-fDistB2);		
		fDirX=(float)((fDist11+fDist22)*0.5)*80;


		fDirX=-Clamp(fDirX,-1,1);
		fDirY=-Clamp(fDirY,-1,1);
		sprintf(szText,"Position=(%f,%f)\n",fDirX,fDirY);
		fputs(szText,fp);

		sprintf(szText,"FrameEnd %d/%d\n",current_frame,stop_frame);
		fputs(szText,fp);

		cvShowImage("FaceAnim", frame1);
		//cvShowImage("FaceAnim", tFace.frame1_1C);

		int key_pressed;
		key_pressed =cvWaitKey(0);

		if (key_pressed=='c' || key_pressed=='C')
		{
			SaveTGA((uchar *)frame->imageData,3,w,h,"FrameRef.tga",false,true);
			//sprintf(szText,"Frame %d saved",current_frame);
			//cvPutText( frame1, szText,cvPoint(10,60), &font1, cvScalar(0,0,255) );

			continue;
		}

		if (current_frame==stop_frame-2)
		{
			SaveTGA((uchar *)frame->imageData,3,w,h,"EndFrame.tga",false,true);
			//sprintf(szText,"Frame %d saved",current_frame);
			//cvPutText( frame1, szText,cvPoint(10,60), &font1, cvScalar(0,0,255) );			
		}


		if (key_pressed=='d' || key_pressed=='D')
		{
			// Toggle the feature processing debug drawing
			bFeatureDebugDraw ^= true;
			continue;
		}


		MyOutputDebugString("Frame %d/%d\n",current_frame,stop_frame);
		int nStep=1;
		if (key_pressed == 'b' || key_pressed == 'B')	
			current_frame-=nStep;
		else											
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

		if (key_pressed=='x' || key_pressed=='X')
			break;
	}

	//KLTFreeFeatureTable(m_ft);	
	//KLTFreeFeatureList(m_fl);	
	//KLTFreeTrackingContext(m_tc);		

	sprintf(szText,"End of file\n");
	fputs(szText,fp);
	fclose(fp);

	cvReleaseMemStorage(&storage);

	// (It's not the best idea to draw on the static memory space of cvQueryFrame().)
	//cvReleaseImage(&frame);
	cvReleaseImage(&frame1);
	cvReleaseImage(&tFace.frame1_1C);
	cvReleaseImage(&tFace.frame2_1C);
	//cvReleaseImage(&frame2_1C);
	cvReleaseImage(&eig_image);
	cvReleaseImage(&temp_image);

	cvvDestroyWindow("FaceAnim");
	cvReleaseCapture(&input_video);

	cvReleaseHaarClassifierCascade(&cascade_lefteye);
	cvReleaseHaarClassifierCascade(&cascade_righteye);
	cvReleaseHaarClassifierCascade(&cascade_mouth);
	cvReleaseHaarClassifierCascade(&cascadeface);

	SAFE_DELETE_ARRAY(lstFrames);

	return (true);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// 3D points
//////////////////////////////////////////////////////////////////////////
void CFaceAnimDoc::Calc3DInfo(tFeatureProcessing *tFrame)
{
	tFaceDetect *pFace=tFrame->pFace;

	//////////////////////////////////////////////////////////////////////////
	// taken from a generic 3d model
	// x right
	// y inside screen
	// z up

	pFace->lst_3d_points[0].Set(0,-15.483,158.124); // chin 
	pFace->lst_3d_points[1].Set(+1.428,-15.512,161.246); // lips bottom right
	pFace->lst_3d_points[2].Set(-1.428,-15.512,161.246); // lips bottom left
	pFace->lst_3d_points[3].Set(+3.309,-14.272,161.692); // mouth right
	pFace->lst_3d_points[4].Set(-3.309,-14.272,161.692); // mouth left
	pFace->lst_3d_points[5].Set(+1.493,-15.681,162.68); // lips top right
	pFace->lst_3d_points[6].Set(-1.493,-15.681,162.68); // lips top left
	pFace->lst_3d_points[7].Set(+4.257,-14.436,165.244); // cheek right
	pFace->lst_3d_points[8].Set(-4.257,-14.436,165.244); // cheek left
	pFace->lst_3d_points[9].Set(0,-17.662,166.82); // noise, center
	pFace->lst_3d_points[10].Set(+4.814,-13.259,167.954); // under eye right
	pFace->lst_3d_points[11].Set(-4.814,-13.259,167.954); // under eye left
	pFace->lst_3d_points[12].Set(-5.918,-12.771,171.778); // left eyebrows left
	pFace->lst_3d_points[13].Set(-2.521,-14.723,171.471); // left eyebrows right
	pFace->lst_3d_points[14].Set(+2.521,-14.723,171.471); // right eyebrows left
	pFace->lst_3d_points[15].Set(+5.918,-12.771,171.778); // right eyebrows right

	//////////////////////////////////////////////////////////////////////////

	// Setup a camera with arbitrary focal length and extract approximate 3d points.			
	pFace->mProjMatrix.Identity();			
	ftype fZMin=0.05;
	ftype fZMax=5;
	MyPerspective2(RAD2DEG(60.0),(ftype)(tFrame->pDrawingFrame->width)/(ftype)(tFrame->pDrawingFrame->height),fZMin,fZMax,&pFace->mProjMatrix.m_Tvalues[0][0]);

	// X right
	// Y up
	// Z out of the screen			
	pFace->mViewMatrix.Identity();
	pFace->mViewMatrix.TranslateMatrix(vector3f(0,0,9));
	pFace->mViewMatrix.MultMatrixf(&pFace->mProjMatrix.m_Tvalues[0][0]);

	pFace->nViewport[0]=0;pFace->nViewport[2]=tFrame->pDrawingFrame->width;
	pFace->nViewport[1]=0;pFace->nViewport[3]=tFrame->pDrawingFrame->height;

	double m[16];
	invert_matrix(&pFace->mViewMatrix.m_Tvalues[0][0], m);
	memcpy(&pFace->mInvertedViewMatrix.m_Tvalues[0][0],m,sizeof(CMatrixf));		
	tFrame->vRotation.Clear();

	tFrame->pFace->vRef_3DCenter.Clear();	

	for (int k=0;k<FA_NUM_MARKERS;k++)
	{

		vector3f vSource,vDest;
		vSource.x=pFace->ref_lst_markers[k].x;
		vSource.y=pFace->ref_lst_markers[k].y;
		vSource.z=1.0f;
		UnProjectFromScreen(vSource,vDest,tFrame);

		pFace->ref_lst_3D_markers[k].x=vDest.x;
		pFace->ref_lst_3D_markers[k].y=vDest.y;
		pFace->ref_lst_3D_markers[k].z=vDest.z;

		pFace->lst_3D_markers[k]=pFace->ref_lst_3D_markers[k];

		pFace->vRef_3DCenter+=vDest;			

		/*
		ProjectToScreen(pFace->ref_lst_3D_markers[k],vDest,tFrame);
		CvPoint p;
		p.x = (int) vDest.x;
		p.y = (int) vDest.y;
		cvCircle(tFrame->pDrawingFrame,p,3,CV_RGB(255,0,0), -1);
		*/
	} //k

	pFace->vRef_3DCenter/=(ftype)(FA_NUM_MARKERS);
	pFace->v3DCenter=pFace->vRef_3DCenter;

	for (int k=0;k<FA_NUM_MARKERS;k++)
	{
		pFace->ref_lst_3D_dist2[k]=pFace->vRef_3DCenter.Distance2(pFace->lst_3D_markers[k]);
		pFace->lst_3D_dist2[k]=pFace->ref_lst_3D_dist2[k];
	} //k
}

//////////////////////////////////////////////////////////////////////////
void CFaceAnimDoc::CalcBestRot(vector3f &vBestRot,ftype fRange,ftype fStep,const vector3f &vRotStart,ftype &fBestError,tFeatureProcessing *tFrame1)
{	
	vector3f	vProj;
	vector3f	vPos1,vPos2;	
	CMatrixf matrix1,matrix2,matrix3;

	for (ftype xrot=vRotStart.x-fRange;xrot<=vRotStart.x+fRange;xrot+=fStep)
	{	
		matrix1.Identity();		
		matrix1.RotateMatrixf(xrot,1,0,0);	//pitch	
		for (ftype zrot=vRotStart.z-fRange;zrot<=vRotStart.z+fRange;zrot+=fStep)
		{			
			memcpy(&matrix3,&matrix1,sizeof(CMatrixf));
			matrix3.RotateMatrixf(zrot,0,1,0);	//roll
			for (ftype yrot=vRotStart.y-fRange;yrot<=vRotStart.y+fRange;yrot+=fStep)
			{	
				ftype fErrorSum=0;

				memcpy(&matrix2,&matrix3,sizeof(CMatrixf));
				matrix2.RotateMatrixf(yrot,0,0,1);	//yaw				

				for (int k=0;k<FA_NUM_MARKERS;k++)
				{
					vPos1=tFrame1->pFace->lst_3D_markers[k];
					vPos2=matrix2.TransformPoint(vPos1);
					ProjectToScreen(vPos2,vProj,tFrame1);					
					fErrorSum+=sqr(tFrame1->lst_markers[k].x-vProj.x);
					fErrorSum+=sqr(tFrame1->lst_markers[k].y-vProj.y);
					if (fErrorSum>fBestError)
						break;
				}

				if (fErrorSum<fBestError)
				{
					fBestError=fErrorSum;
					vBestRot.Set(xrot,yrot,zrot);
				}
			} //y
		} //z
	} //x
}

//////////////////////////////////////////////////////////////////////////
void CFaceAnimDoc::FindRotation(tFeatureProcessing *tFrame1,tFeatureProcessing *tFrame2)
{
	ftype	fRange=10.0; // very large range	

	// start from previous rotation
	vector3f	vRotStart=tFrame1->vRotation;	
	ftype			fBestError=9999;
	vector3f	vBestRot=vRotStart;

	// first calc one with the current rotation to skip lots of iterations later
	CalcBestRot(vBestRot,0,10,vRotStart,fBestError,tFrame1);

	while (fBestError>0.5 && fRange>0.05) // iterate until the error is extremely small
	{		
		CalcBestRot(vBestRot,fRange,fRange/8.0,vRotStart,fBestError,tFrame1);

		vRotStart=vBestRot;
		fRange/=2.0;		
	}
	tFrame1->vRotation=vBestRot;

	CMatrixf matrix2;
	matrix2.Identity();		
	matrix2.RotateMatrixf(tFrame1->vRotation.x,1,0,0);	//pitch	
	matrix2.RotateMatrixf(tFrame1->vRotation.z,0,1,0);	//roll
	matrix2.RotateMatrixf(tFrame1->vRotation.y,0,0,1);	//yaw		

	CMatrixf matrix22;	
	//invert_matrix(&matrix2.m_Tvalues[0][0], &matrix22.m_Tvalues[0][0]);		
	matrix22=matrix2;
	// only rotation, no need to invert
	matrix22.Transpose();

	tFrame1->pFace->v3DCenter.Clear();	

	for (int k=0;k<FA_NUM_MARKERS;k++)
	{
		vector3f vPos1=tFrame1->pFace->lst_3D_markers[k];
		vector3f vPos2=matrix2.TransformPoint(vPos1);
		vector3f vProj;
		ProjectToScreen(vPos2,vProj,tFrame1);

		// estimate the position thru the sequence
		vector3f	v1,v2;
		// combined 2d tracked pos + z found from rotation 
		v1.Set(tFrame1->lst_markers[k].x,tFrame1->lst_markers[k].y,vProj.z);		
		// get another 3d pos 
		UnProjectFromScreen(v1,v2,tFrame1);
		// where would this point be in absence of rotation
		v1=matrix22.TransformPoint(v2);
		// combine
		tFrame1->pFace->lst_3D_markers[k]=(v1+vPos1)/2.0f;

		tFrame1->pFace->v3DCenter+=tFrame1->pFace->lst_3D_markers[k];

		CvPoint p;
		p.x = (int) vProj.x;
		p.y = (int) vProj.y;
		cvCircle(tFrame1->pDrawingFrame,p,3,CV_RGB(255,255,0), -1);
	}

	tFrame1->pFace->v3DCenter/=(ftype)(FA_NUM_MARKERS);

	for (int k=0;k<FA_NUM_MARKERS;k++)
	{
		tFrame1->pFace->lst_3D_dist2[k]=tFrame1->pFace->v3DCenter.Distance2(tFrame1->pFace->lst_3D_markers[k]);		
	} //k

	//matrix2.Identity();		
	//matrix2.RotateMatrixf(tFrame1->nFrame,1,0,0);	//x	
	//matrix2.RotateMatrixf(0,0,1,0);	//z
	//matrix2.RotateMatrixf(0,0,0,1);	//y

	CvPoint p,q;
	vector3f v00=tFrame2->pFace->v3DCenter;
	vector3f v11=v00+vector3f(1,0,0);
	vector3f v22=v00+vector3f(0,1,0);
	vector3f v33=v00+vector3f(0,0,1);
	vector3f v0=matrix2.TransformPoint(v00);
	vector3f v1=matrix2.TransformPoint(v11);
	vector3f v2=matrix2.TransformPoint(v22);
	vector3f v3=matrix2.TransformPoint(v33);
	vector3f vProj0,vProj1,vProj2,vProj3;
	ProjectToScreen(v0,vProj0,tFrame2);p.x=(int)vProj0.x;p.y=(int)vProj0.y;
	ProjectToScreen(v1,vProj1,tFrame2);q.x=(int)vProj1.x;q.y=(int)vProj1.y;
	cvDrawLine(tFrame2->pDrawingFrame,p,q,CV_RGB(255,0,0));
	ProjectToScreen(v2,vProj2,tFrame2);q.x=(int)vProj2.x;q.y=(int)vProj2.y;
	cvDrawLine(tFrame2->pDrawingFrame,p,q,CV_RGB(0,255,0));
	ProjectToScreen(v3,vProj3,tFrame2);q.x=(int)vProj3.x;q.y=(int)vProj3.y;		
	cvDrawLine(tFrame2->pDrawingFrame,p,q,CV_RGB(0,0,255));


	tFrame2->vRotation=tFrame1->vRotation;

	MyOutputDebugString("Frame=%d,Error=%f,Rotation=%f,%f,%f\n",tFrame1->nFrame,fBestError,tFrame2->vRotation.x,tFrame2->vRotation.y,tFrame2->vRotation.z);
}
