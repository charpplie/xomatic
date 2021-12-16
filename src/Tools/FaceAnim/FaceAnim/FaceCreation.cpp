
#include "stdafx.h"
#include "FaceAnim.h"
#include "FaceAnimDoc.h"
#include "FaceAnimView.h"
#include "CalibrationAVI4.h"

//////////////////////////////////////////////////////////////////////////
void on_mouse2( int event, int x, int y, int flags, void* param )
{
	
	CFaceAnimDoc *pDoc=(CFaceAnimDoc *)param;
	if ( !pDoc || !pDoc->m_bEditMode )
		return;

	pDoc->EditMarkers2(x,y,event);
}

//////////////////////////////////////////////////////////////////////////
void CFaceAnimDoc::EditMarkers2(int x,int y,int event)
{
	if (event==CV_EVENT_LBUTTONDOWN)
	{
		if ((x>(m_pFace->face.x+m_pFace->face.width)) || (x<(m_pFace->face.x)))
		{
			m_bScaleOperationX=true;
		}
		else
		if ((y>(m_pFace->face.y+m_pFace->face.height)) || (y<(m_pFace->face.y)))
		{
			m_bScaleOperationY=true;
		}
		else
			m_bMoveOperation=true;
	}
	else
	if (event==CV_EVENT_LBUTTONUP)
	{
		m_bScaleOperationX=m_bScaleOperationY=false;
		m_bMoveOperation=false;
		for (int k=0;k<m_pFace->nNumVertices;k++)
		{
			vector3f vPos=m_pFace->lst_vModel_3D[k];			
			vPos.Magnitude(m_pFace->vScale);
			vPos+=m_pFace->vOffset;
			m_pFace->lst_vModel_3D[k]=vPos;
		}
		for (int k=0;k<FA_NUM_MARKERS;k++)
		{
			vector3f vPos=m_pFace->lst_ref_Markers[k].v3DPos;			
			vPos.Magnitude(m_pFace->vScale);
			vPos+=m_pFace->vOffset;
			m_pFace->lst_ref_Markers[k].v3DPos=vPos;
		}
		m_pFace->vScale.Set(1,1,1);				
		m_pFace->vOffset.Set(0,0,0);
	}
	else
	if (event==CV_EVENT_MOUSEMOVE)
	{
		if (m_bScaleOperationX)
		{
			ftype fScale;
			if (x>m_pFace->face_center.x)
				fScale=(ftype)(x-m_pFace->face_center.x)/(ftype)(m_pFace->face.width/2);
			else
				fScale=(ftype)(m_pFace->face_center.x-x)/(ftype)(m_pFace->face.width/2);
			m_pFace->vScale.x=fScale;
		}
		if (m_bScaleOperationY)
		{
			ftype fScale;
			if (y>m_pFace->face_center.y)
				fScale=(ftype)(y-m_pFace->face_center.y)/(ftype)(m_pFace->face.height/2);
			else
				fScale=(ftype)(m_pFace->face_center.y-y)/(ftype)(m_pFace->face.height/2);
			m_pFace->vScale.y=fScale;
		}
		if (m_bMoveOperation)
		{
			m_pFace->vOffset.x=(x-m_pFace->face_center.x)*0.01;
			m_pFace->vOffset.y=-(y-m_pFace->face_center.y)*0.01;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
bool CFaceAnimDoc::CreateFace(const char *szFilename)
{
	m_bEditMode=false;
	m_bScaleOperationX=m_bScaleOperationY=false;
	m_bMoveOperation=false;

	IplImage	*pFrame=cvLoadImage(szFilename);
	if (!pFrame)
		return (false);

	IplImage *pFrameDraw=cvCloneImage(pFrame);

	CvSize frame_size;
	int w=frame_size.width =pFrame->width;
	int h=frame_size.height =pFrame->height;		

	tFaceDetect	tFace;	
	m_pFace=&tFace;

	//////////////////////////////////////////////////////////////////////////
	tFace.storage = cvCreateMemStorage(0);

	const char *cascadeface_name = "Data/haarcascades/haarcascade_frontalface_alt2.xml";
	//sprintf(szFilename,"%s/%s",szFolder,cascadeface_name);
	tFace.cascadeface = (CvHaarClassifierCascade*)cvLoad( cascadeface_name, 0, 0, 0 );	

	//const char *cascadeeyes_name = "Data/haarcascades/parojos.xml";
	//sprintf(szFilename,"%s/%s",szFolder,cascadeeyes_name);
	//CvHaarClassifierCascade*cascadeeyes = (CvHaarClassifierCascade*)cvLoad( szFilename, 0, 0, 0 );	

	const char *cascade_righteye_name = "Data/haarcascades/ojoD.xml";
	//sprintf(szFilename,"%s/%s",szFolder,cascade_righteye_name);
	tFace.cascade_righteye = (CvHaarClassifierCascade*)cvLoad( cascade_righteye_name , 0, 0, 0 );	

	const char *cascade_lefteye_name = "Data/haarcascades/ojoI.xml";
	//sprintf(szFilename,"%s/%s",szFolder,cascade_lefteye_name);
	tFace.cascade_lefteye = (CvHaarClassifierCascade*)cvLoad( cascade_lefteye_name , 0, 0, 0 );	

	const char *cascade_mouth_name = "Data/haarcascades/boca.xml";
	//const char *cascade_mouth_name = "Data/haarcascades/haarcascade_profileface.xml";
	//sprintf(szFilename,"%s/%s",szFolder,cascade_mouth_name);
	tFace.cascade_mouth = (CvHaarClassifierCascade*)cvLoad( cascade_mouth_name, 0, 0, 0 );	
		
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

	CvFont font1;
	cvInitFont( &font1, CV_FONT_HERSHEY_SIMPLEX, 0.35, 0.35);  		
	cvNamedWindow("FaceCreation", CV_WINDOW_AUTOSIZE);
	cvSetMouseCallback( "FaceCreation", on_mouse2, this );
	
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

	char szText[256];
	sprintf(szText,"%s","Test");
	cvPutText( pFrameDraw, szText,cvPoint(10,20), &font1, cvScalar(255,0,0) );
	
	MyOutputDebugString("Attempting to load faceaa.obj\n");
	if (!LoadOBJ("faceaa.obj",&tFace))
	{
		MyError("Cannot load obj") ;
	}

	IplImage *frame1_1C= cvCreateImage(frame_size,8,1);	
	cvConvertImage(pFrame, frame1_1C, 0);				
	// equalize for face detection etc.
	cvEqualizeHist( frame1_1C, frame1_1C );
	cvClearMemStorage( tFace.storage );
	//cvShowImage("FaceCreation", frame1_1C);
	//cvWaitKey(0);

	//////////////////////////////////////////////////////////////////////////
	// face
	double t = (double)cvGetTickCount();
	CvSeq* faces = cvHaarDetectObjects( frame1_1C, tFace.cascadeface, tFace.storage,
		//1.1, 2, 0/*CV_HAAR_DO_CANNY_PRUNING*/,
		1.1, 3, 0/*CV_HAAR_DO_CANNY_PRUNING*/
		//,cvSize(30, 30) );
		//,cvSize(20, 20) );
		);

	bool bFaceFound=false;
	if (faces && faces->total==1)
	{		
		bFaceFound=true;

		float scale=1.0f;
		//for(int i = 0; i < (faces ? faces->total : 0); i++ )

		CvRect* r = (CvRect*)cvGetSeqElem( faces, 0 );
		// copy cos its sharing data pointers
		CvRect	face_rect=cvRect(r->x,r->y,r->width,r->height);		

		tFace.face_center.x = cvRound((r->x + r->width*0.5));
		tFace.face_center.y = cvRound((r->y + r->height*0.5));
		tFace.radius = cvRound((r->width + r->height)*0.25);

		//cvCircle( pFrameDraw, tFace.face_center, tFace.radius, colors[0], 3, 8, 0 );
		//cvDrawRect(pFrameDraw,cvPoint(r->x,r->y),cvPoint(r->x+r->width,r->y+r->height),colors[0]);
		tFace.face=cvRect(r->x,r->y,r->width,r->height);
		
		CvSize face_size;
		face_size.width=r->width;
		face_size.height=r->height;
		IplImage *pTemp=cvCreateImage( face_size, 8, 1 );			
		cvGetRectSubPix(frame1_1C,pTemp,cvPoint2D32f(tFace.face_center.x,tFace.face_center.y));

		//////////////////////////////////////////////////////////////////////////
		// left eye

		cvClearMemStorage( tFace.storage );
		t = (double)cvGetTickCount();
		CvSeq* eyeL = cvHaarDetectObjects( pTemp, tFace.cascade_lefteye, tFace.storage,
			//1.1, 2, 0//CV_HAAR_DO_CANNY_PRUNING,
			1.1, 3, 0//CV_HAAR_DO_CANNY_PRUNING
			//,cvSize(30, 30) );
			//,cvSize(20, 20) );
			);

		CvRect	eye_bestL;
		int		bestyposL=-1000;		
		for(int i = 0; i < (eyeL ? eyeL->total : 0); i++ )
		{			
			CvRect* r2 = (CvRect*)cvGetSeqElem( eyeL, i );

			CvPoint eye_center;				
			eye_center.x = face_rect.x+cvRound((r2->x + r2->width*0.5));
			eye_center.y = face_rect.y+cvRound((r2->y + r2->height*0.5));
			//if (eye_center.y>face_center.y)
			//	continue; // mouth or nose

			if (face_rect.y+r2->y+r2->height>(tFace.face_center.y+4))
				continue; // mouth or nose

			if (eye_center.x>tFace.face_center.x)
				continue; // right eye

			// copy cos its sharing data pointers
			CvRect	eyeRect=cvRect(r2->x,r2->y,r2->width,r2->height);
			if (eye_center.y>bestyposL)
			{
				eye_bestL=eyeRect;
				bestyposL=eye_center.y;
			}			
			tFace.bEyeLDetected=true;
		} //i

		if (tFace.bEyeLDetected)
		{	
			//cvDrawRect(pFrameDraw,cvPoint(face_rect.x+eye_bestL.x,face_rect.y+eye_bestL.y),cvPoint(face_rect.x+eye_bestL.x+eye_bestL.width,face_rect.y+eye_bestL.y+eye_bestL.height),colors[1]);
			tFace.eye_left=cvRect(face_rect.x+eye_bestL.x,face_rect.y+eye_bestL.y,eye_bestL.width,eye_bestL.height);
		}

		//////////////////////////////////////////////////////////////////////////
		// right eye

		cvClearMemStorage( tFace.storage );
		t = (double)cvGetTickCount();
		CvSeq* eyeR = cvHaarDetectObjects( pTemp, tFace.cascade_righteye, tFace.storage,
			//1.1, 2, 0//CV_HAAR_DO_CANNY_PRUNING,
			1.1, 3, 0//CV_HAAR_DO_CANNY_PRUNING
			//,cvSize(30, 30) );
			//,cvSize(20, 20) );
			);

		CvRect	eye_bestR;
		int		bestyposR=-1000;
		//int		nBestSize=0;		

		for(int i = 0; i < (eyeR ? eyeR->total : 0); i++ )
		{			
			CvRect* r2 = (CvRect*)cvGetSeqElem( eyeR, i );

			//cvDrawRect(frame1,cvPoint(face_rect.x+r2->x,face_rect.y+r2->y),cvPoint(face_rect.x+r2->x+r2->width,face_rect.y+r2->y+r2->height),colors[2]);

			CvPoint eye_center;				
			eye_center.x = face_rect.x+cvRound((r2->x + r2->width*0.5));
			eye_center.y = face_rect.y+cvRound((r2->y + r2->height*0.5));
			//if (eye_center.y>face_center.y)
			if (face_rect.y+r2->y+r2->height>(tFace.face_center.y+4))
				continue; // mouth or nose

			if (eye_center.x<tFace.face_center.x)
				continue; // left eye

			// copy cos its sharing data pointers
			CvRect	eyeRect=cvRect(r2->x,r2->y,r2->width,r2->height);				
			if (eye_center.y>bestyposR)
			{
				eye_bestR=eyeRect;
				bestyposR=eye_center.y;
				//nBestSize=nSize;
			}			
			tFace.bEyeRDetected=true;				
		} //i

		if (tFace.bEyeRDetected)
		{	
			//cvDrawRect(pFrameDraw,cvPoint(face_rect.x+eye_bestR.x,face_rect.y+eye_bestR.y),cvPoint(face_rect.x+eye_bestR.x+eye_bestR.width,face_rect.y+eye_bestR.y+eye_bestR.height),colors[2]);
			tFace.eye_right=cvRect(face_rect.x+eye_bestR.x,face_rect.y+eye_bestR.y,eye_bestR.width,eye_bestR.height);
		}

		//////////////////////////////////////////////////////////////////////////
		// mouth

		cvClearMemStorage( tFace.storage );
		t = (double)cvGetTickCount();
		CvSeq* mouth = cvHaarDetectObjects( pTemp, tFace.cascade_mouth, tFace.storage,
			//1.1, 2, 0//CV_HAAR_DO_CANNY_PRUNING,
			1.1, 3, 0//CV_HAAR_DO_CANNY_PRUNING
			//,cvSize(30, 30) );
			//,cvSize(20, 20) );
			);

		CvRect	mouth_best;
		int		bestypos_mouth=-1000;		

		for(int i = 0; i < (mouth ? mouth->total : 0); i++ )
		{			
			CvRect* r2 = (CvRect*)cvGetSeqElem( mouth, i );

			CvPoint mouth_center;				
			mouth_center.x = face_rect.x+cvRound((r2->x + r2->width*0.5));
			mouth_center.y = face_rect.y+cvRound((r2->y + r2->height*0.5));				

			//cvDrawRect(frame1,cvPoint(face_rect.x+r2->x,face_rect.y+r2->y),cvPoint(face_rect.x+r2->x+r2->width,face_rect.y+r2->y+r2->height),colors[5]);

			//if (mouth_center.y<face_center.y)
			if (face_rect.y+r2->y<tFace.face_center.y)
				continue; // nose

			// copy cos its sharing data pointers
			CvRect	mouthRect=cvRect(r2->x,r2->y,r2->width,r2->height);
			if (mouth_center.y>bestypos_mouth)
			{
				mouth_best=mouthRect;
				bestypos_mouth=mouth_center.y;
			}			
			tFace.bMouthDetected=true;				
		} //i

		if (tFace.bMouthDetected)
		{	
			//cvDrawRect(pFrameDraw,cvPoint(face_rect.x+mouth_best.x,face_rect.y+mouth_best.y),cvPoint(face_rect.x+mouth_best.x+mouth_best.width,face_rect.y+mouth_best.y+mouth_best.height),colors[4]);
			tFace.mouth=cvRect(face_rect.x+mouth_best.x,face_rect.y+mouth_best.y,mouth_best.width,mouth_best.height);
		}

		cvReleaseImage(&pTemp);
	
		//////////////////////////////////////////////////////////////////////////
		// camera		
		ftype fZMin=0.15f;
		ftype fZMax=20.0f;

		tFace.nViewport[0]=0;tFace.nViewport[2]=w;
		tFace.nViewport[1]=0;tFace.nViewport[3]=h;

		tFace.mProjMatrix.Identity();
		MyPerspective3(60.0,(ftype)(w)/(ftype)(h),fZMin,fZMax,&tFace.mProjMatrix.m_Tvalues[0][0]);

		// view matrix		
		tFace.mViewMatrix.Identity();		
		//mat.RotateMatrixf(180.0,1.0,0,0);//rot+=1.0;		
		//mat.MultMatrixf(&m_Orig.m_Tvalues[0][0]);
		//mat.TranslateMatrix(-m_vPos);

		// cached MyProject matrix
		double m[16];
		matmul2(m,&tFace.mProjMatrix.m_Tvalues[0][0],&tFace.mViewMatrix.m_Tvalues[0][0]);				
		memcpy(&tFace.mMatrix.m_Tvalues[0][0],m,sizeof(double)*16);				

		// cached MyUnProject matrix
		invert_matrix(&tFace.mMatrix.m_Tvalues[0][0], m);
		memcpy(&tFace.mInvertedViewMatrix.m_Tvalues[0][0],m,sizeof(CMatrixf));

		vector3f vMins,vMaxs;
		vMins.SetMax();vMaxs.SetMin();		
		vector3f vCorners2D[4],vCorners3D[4];
		vCorners2D[0].Set(tFace.face.x,tFace.face.y,0.99);
		vCorners2D[1].Set(tFace.face.x+tFace.face.width,tFace.face.y,0.99);
		vCorners2D[2].Set(tFace.face.x+tFace.face.width,tFace.face.y+tFace.face.height,0.99);
		vCorners2D[3].Set(tFace.face.x,tFace.face.y+tFace.face.height,0.99);
		vector3f vCenter(0,0,0);
		for (int j=0;j<4;j++)		
		{
			tFace.UnProjectFromScreen(vCorners2D[j],vCorners3D[j]);
			vMins.CheckMin(vCorners3D[j]);
			vMaxs.CheckMax(vCorners3D[j]);
			tFace.ProjectToScreen(vCorners3D[j],vCorners2D[j]);
			vCenter+=vCorners3D[j];
		}
		vCenter/=4.0;		


		// flip all
		for (int k=0;k<FA_NUM_MARKERS;k++)
		{
			vector3f vPos=tFace.lst_ref_Markers[k].v3DPos;
			vPos.y=-vPos.y;
			tFace.lst_ref_Markers[k].v3DPos=vPos;			
		} //k

		tFace.v3DMins.SetMax();
		tFace.v3DMaxs.SetMin();
		tFace.vRef_3DCenter.Clear();
		for (int k=0;k<tFace.nNumVertices;k++)
		{
			vector3f vPos=tFace.lst_vModel_3D[k];
			vPos.y=-vPos.y;
			tFace.lst_vModel_3D[k]=vPos;
			tFace.v3DMins.CheckMin(tFace.lst_vModel_3D[k]);
			tFace.v3DMaxs.CheckMax(tFace.lst_vModel_3D[k]);
			tFace.vRef_3DCenter+=tFace.lst_vModel_3D[k];
		} //k
		tFace.vRef_3DCenter/=(ftype)(tFace.nNumVertices);
				
		//ftype	fScaleX=(vMaxs.x-vMins.x)/(tFace.v3DMaxs.x-tFace.v3DMins.x);
		//ftype	fScaleY=(vMaxs.y-vMins.y)/(tFace.v3DMaxs.y-tFace.v3DMins.y);

		vector3f vDest;		
		vector3f v1,v2,v3;
		vDest.Set(tFace.eye_left.x,tFace.eye_left.y,0.99);
		tFace.UnProjectFromScreen(vDest,v1);
		vDest.Set(tFace.eye_right.x,tFace.eye_right.y,0.99);
		tFace.UnProjectFromScreen(vDest,v2);
		v3=(v1+v2)*0.5f; // middle eye brows, nose
		ftype	fDist1=v1.Distance(v2);
		v1=(tFace.lst_ref_Markers[FA_LT_EYEBROW_LT].v3DPos+tFace.lst_ref_Markers[FA_LT_EYEBROW_RT].v3DPos)*0.5f;
		v2=(tFace.lst_ref_Markers[FA_RT_EYEBROW_LT].v3DPos+tFace.lst_ref_Markers[FA_RT_EYEBROW_RT].v3DPos)*0.5f;
		ftype	fDist2=v1.Distance(v2);
		ftype fScaleX=fDist1/fDist2;

		vDest.Set(tFace.mouth.x+tFace.mouth.width/2,tFace.mouth.y+tFace.mouth.height/2,0.99);
		tFace.UnProjectFromScreen(vDest,v1);
		fDist1=v3.Distance(v1);		

		v3=(tFace.lst_ref_Markers[FA_LT_EYEBROW_RT].v3DPos+tFace.lst_ref_Markers[FA_RT_EYEBROW_LT].v3DPos)*0.5f;
		v1=(tFace.lst_ref_Markers[FA_MOUTH_LT].v3DPos+tFace.lst_ref_Markers[FA_MOUTH_RT].v3DPos)*0.5f;
		fDist2=v3.Distance(v1);
		ftype fScaleY=fDist1/fDist2;
		
		ftype	fScaleZ=max(fScaleX,fScaleY);
		vector3f vScale(fScaleX,fScaleY,fScaleZ);
		vector3f vModelCenter=tFace.vRef_3DCenter;
		vModelCenter.Magnitude(vScale);
		vector3f vOff=vCenter-vModelCenter;		

		tFace.vScale=vScale;
		tFace.vOffset=vOff;
		for (int k=0;k<tFace.nNumVertices;k++)
		{
			vector3f vPos=tFace.lst_vModel_3D[k];			
			vPos.Magnitude(tFace.vScale);
			vPos+=tFace.vOffset;
			tFace.lst_vModel_3D[k]=vPos;
		}
		for (int k=0;k<FA_NUM_MARKERS;k++)
		{
			vector3f vPos=tFace.lst_ref_Markers[k].v3DPos;			
			vPos.Magnitude(tFace.vScale);
			vPos+=tFace.vOffset;
			tFace.lst_ref_Markers[k].v3DPos=vPos;
		}
		vScale.Set(1,1,1);		
		vOff.Set(0,0,0);

		tFace.vScale=vScale;
		tFace.vOffset=vOff;
		tFace.pRefFrame=pFrame;
		tFace.pRefFrame2=pFrameDraw;
		m_bEditMode=true;

		while (1)
		{		
			cvConvertImage(pFrame,pFrameDraw);

			for (int k=0;k<tFace.nNumVertices;k++)
			{
				vector3f vPos=tFace.lst_vModel_3D[k];			
				vPos.Magnitude(tFace.vScale);
				vPos+=tFace.vOffset;
				tFace.ProjectToScreen(vPos,vDest);
				DrawRectangle(pFrameDraw,vDest-vector3f(0,0,0),vDest+vector3f(0,0,0),colors[3]);
			} //k
					
			for (int k=0;k<FA_NUM_MARKERS;k++)
			{
				vector3f vPos=tFace.lst_ref_Markers[k].v3DPos;			
				vPos.Magnitude(tFace.vScale);
				vPos+=tFace.vOffset;
				tFace.ProjectToScreen(vPos,vDest);

				if (strstr(tFace.lst_ref_Markers[k].szName,"eye"))
					DrawRectangle(pFrameDraw,vDest-vector3f(1,1,1),vDest+vector3f(1,1,1),colors[0]);
				else
					DrawRectangle(pFrameDraw,vDest-vector3f(1,1,1),vDest+vector3f(1,1,1),colors[1]);
			} //k		

			cvDrawRect(pFrameDraw,cvPoint((int)(vCorners2D[0].x),(int)(vCorners2D[0].y)),cvPoint((int)(vCorners2D[2].x),(int)(vCorners2D[2].y)),colors[4]);
			cvShowImage("FaceCreation", pFrameDraw);
			int key_pressed=cvWaitKey(1);

			if (key_pressed=='E' || key_pressed=='e')
			{
				FILE *fp=fopen("faceexport.mtl","wt");
				fprintf(fp,"newmtl test1\n");
				fprintf(fp,"	Ns 0\n");
				fprintf(fp,"	d 1\n");
				fprintf(fp,"	illum 2\n");
				fprintf(fp,"	Kd 0.8 0.8 0.8\n");
				fprintf(fp,"	Ks 0 0 0\n");
				fprintf(fp,"	Ks 0.225 0.225 0.225\n");
				fprintf(fp,"	map_Kd faceexport.jpg\n");
				
				fclose(fp);

				fp=fopen("faceexport.obj","wt");
				fprintf(fp,"mtllib faceexport.mtl\n");
				// export obj
				for (int k=0;k<tFace.nNumVertices;k++)
				{
					vector3f vPos=tFace.lst_vModel_3D[k];
					fprintf(fp,"v %0.6f %0.6f %0.6f\n",vPos.x,vPos.y,vPos.z);
				} //k
				fprintf(fp,"\n");
				for (int k=0;k<tFace.nNumVertices;k++)
				{
					vector3f vPos=tFace.lst_vModel_3D[k];
					tFace.ProjectToScreen(vPos,vDest);
					vDest.x/=(ftype)(w);
					vDest.y=(h-vDest.y-1)/(ftype)(h);
					fprintf(fp,"vt %0.6f %0.6f\n",vDest.x,vDest.y);
				} //k
				fprintf(fp,"\n");
				fprintf(fp,"usemtl Test1\n");
				fprintf(fp,"g Group1\n");
				for (int k=0;k<tFace.nNumTris;k++)
				{
					int i1,i2,i3;
					i1=tFace.lst_Tris[k*3+2];
					i2=tFace.lst_Tris[k*3+1];
					i3=tFace.lst_Tris[k*3+0];
					int i4=1;
					fprintf(fp,"f %d/%d %d/%d %d/%d\n",i1,i1,i2,i2,i3,i3);
				} //k
				fprintf(fp,"\n");
				fclose(fp);
				MyOutputDebugString("Obj exported\n");
			}
		}

	}	

	//cvShowImage("FaceCreation", pFrameDraw);
	//cvWaitKey(0);

	cvReleaseImage(&pFrame);
	cvReleaseImage(&pFrameDraw);	
	
	cvvDestroyWindow("FaceCreation");	

	cvReleaseHaarClassifierCascade(&tFace.cascade_lefteye);
	cvReleaseHaarClassifierCascade(&tFace.cascade_righteye);
	cvReleaseHaarClassifierCascade(&tFace.cascade_mouth);
	cvReleaseHaarClassifierCascade(&tFace.cascadeface);
	cvReleaseMemStorage(&tFace.storage);

	return (true);
}

//////////////////////////////////////////////////////////////////////////
void on_mouse3( int event, int x, int y, int flags, void* param )
{

	CFaceAnimDoc *pDoc=(CFaceAnimDoc *)param;
	if ( !pDoc)
		return;

	pDoc->EditMarkers3(x,y,event);
}

//////////////////////////////////////////////////////////////////////////
void CFaceAnimDoc::EditMarkers3(int x,int y,int event)
{
	static int nPrevX,nPrevY;

	if (event==CV_EVENT_MOUSEMOVE)
	{
		if (m_bMouse3)
		{				
			ftype fDir=(y-nPrevY);
			fDir=Clamp(fDir,-1.0,1.0)*0.001;
			nPrevY=y;

			m_pFace->v2DCenter.z+=fDir;

			m_pFace->v2DCenter.z=Clamp(m_pFace->v2DCenter.z,0,1.0);
			vector3f v1;
			// get the center at some distance
			m_pFace->UnProjectFromScreen(m_pFace->v2DCenter,v1);
			m_pFace->vOffset=v1-m_pFace->v3DCenter;		
		}
		else
		if (m_bMouse1)
		{
			ftype fDirX=(x-nPrevX);
			fDirX=Clamp(fDirX,-1.0,1.0)*0.1;
			m_pFace->vAngles.y-=fDirX;				

			ftype fDirY=(y-nPrevY);
			fDirY=Clamp(fDirY,-1.0,1.0)*0.1;
			m_pFace->vAngles.x+=fDirY;				
		}
		else
		if (m_bMouse2)
		{
			ftype fDirX=(x-nPrevX);
			fDirX=Clamp(fDirX,-1.0,1.0)*0.1;
			m_pFace->vAngles.z+=fDirX;	
		}


		nPrevX=x;
		nPrevY=y;
	}

	if (event==CV_EVENT_LBUTTONDOWN)
	{
		m_bMouse1=true;
	}
	else
	if (event==CV_EVENT_LBUTTONUP)
	{
		m_bMouse1=false;
	}

	if (event==CV_EVENT_RBUTTONDOWN)
	{
		m_bMouse2=true;
	}
	else
	if (event==CV_EVENT_RBUTTONUP)
	{
		m_bMouse2=false;
	}

	if (event==CV_EVENT_MBUTTONUP)
	{
		m_bMouse3=false;
	}
	else
	if (event==CV_EVENT_MBUTTONDOWN)
	{
		m_bMouse3=true;
	}

}

#include "wlm/WmlDelaunay2a.h"
using namespace Wml;

//////////////////////////////////////////////////////////////////////////
bool CFaceAnimDoc::CreateFace2(const char *_szFilename)
{
	//////////////////////////////////////////////////////////////////////////
	tFaceDetect	tFace;	
	m_pFace=&tFace;
	
	FILE *fp=fopen(_szFilename,"rt");

	int nVerts;
	fscanf(fp,"%d \n",&nVerts);
	std::vector<vector3f> lstVerts;
	lstVerts.reserve(nVerts);
	for (int k=0;k<nVerts;k++)
	{
		float vx,vy,vz;		
		fscanf(fp,"%f %f %f \n",&vx,&vy,&vz);
		vector3f vPos(vx,-vy,vz);
		lstVerts.push_back(vPos);
	} //k
	// next one is center again
	float vx,vy,vz;		
	fscanf(fp,"%f %f %f \n",&vx,&vy,&vz);
	tFace.v3DCenter.Set(vx,-vy,vz);

	/*
	int nTris;
	fscanf(fp,"%d \n",&nTris);
	std::vector<int> lstTris;
	lstTris.reserve(nTris*3);
	for (int k=0;k<nTris;k++)
	{
		int idx0,idx1,idx2;
		fscanf(fp,"%d %d %d \n",&idx0,&idx1,&idx2);
		lstTris.push_back(idx0);
		lstTris.push_back(idx1);
		lstTris.push_back(idx2);
	} //k
	*/

	fclose(fp);

	//////////////////////////////////////////////////////////////////////////
	char szText[256];
	char szFilename[256];

	strcpy(szFilename,_szFilename);
	ReplaceExtension(szFilename,"avi");

	// Create an object that decodes the input video stream. 
	CvCapture *input_video = cvCaptureFromAVI(szFilename);
	if (!input_video)
	{
		// Either the video didn't exist OR it uses a codec OpenCV
		// doesn't support.					
		MyError("Cannot open file %s \n",szFilename);
		return false;
	}

	sprintf(m_szWindowFilename,"FaceAnim [%s]",GetFilename(szFilename));

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

	IplImage *pFrame = NULL;		
	long start_frame=0;
	int stop_frame=number_of_frames;

	CvFont font1;
	cvInitFont( &font1, CV_FONT_HERSHEY_SIMPLEX, 0.35, 0.35);  			

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

	//sprintf(szText,"%s","Test");
	//cvPutText( pFrameDraw, szText,cvPoint(10,20), &font1, cvScalar(255,0,0) );

	cvNamedWindow(m_szWindowFilename, CV_WINDOW_AUTOSIZE);

	cvSetMouseCallback( m_szWindowFilename, on_mouse3, this );


	//////////////////////////////////////////////////////////////////////////
	// camera

	tFace.mProjMatrix.Identity();
	ftype fZMin=0.15f;
	ftype fZMax=20.0f;
	tFace.vAngles.Set(0,0,0);
	tFace.vScale.Set(1,1,1);

	tFace.nViewport[0]=0;tFace.nViewport[2]=w;
	tFace.nViewport[1]=0;tFace.nViewport[3]=h;

	tFace.fFov=20.0;

	tFace.mProjMatrix.Identity();
	MyPerspective3(tFace.fFov,(ftype)(w)/(ftype)(h),fZMin,fZMax,&tFace.mProjMatrix.m_Tvalues[0][0]);

	// view matrix		
	tFace.mViewMatrix.Identity();		
	//CMatrixf mat;
	//mat.RotateMatrixf(180.0,1.0,0,0);//rot+=1.0;		
	//mat.MultMatrixf(&m_Orig.m_Tvalues[0][0]);
	//mat.TranslateMatrix(-m_vPos);

	// cached MyProject matrix
	double m[16];
	matmul2(m,&tFace.mProjMatrix.m_Tvalues[0][0],&tFace.mViewMatrix.m_Tvalues[0][0]);				
	memcpy(&tFace.mMatrix.m_Tvalues[0][0],m,sizeof(double)*16);				

	// cached MyUnProject matrix
	invert_matrix(&tFace.mMatrix.m_Tvalues[0][0], m);
	memcpy(&tFace.mInvertedViewMatrix.m_Tvalues[0][0],m,sizeof(CMatrixf));

	IplImage *frame = cvQueryFrame( input_video );		
	// We'll make a full color backup of this frame so that we can draw on it.
	// (It's not the best idea to draw on the static memory space of cvQueryFrame().)		
	pFrame=cvCloneImage(frame);
	IplImage *pFrameDraw=cvCloneImage(frame);

	// set nosbri as center and remove rotation

	//////////////////////////////////////////////////////////////////////////
	// get scale and offset
	// center is marked as 255,0,255
	// use find rotation for best match

	// later: load obj and match with chr vertices
	// parametrize the texture video using uv offsets from model
	
	vector3f vMins2D,vMaxs2D;
	vMins2D.SetMax();vMaxs2D.SetMin();

	std::vector<vector3f> lstVerts2D;
	strcpy(szFilename,_szFilename);
	ReplaceExtension(szFilename,"bmp");
	IplImage *pFrame0=cvLoadImage(szFilename);
	if (pFrame0)
	{
		uchar *pSrc=(uchar *)pFrame0->imageData;
		for (int k=0;k<w*h;k++)
		{
			int y=k/w;
			int x=k-y*w;
			// NOTE: FLIPPED!
			vector3f vPos(x,h-y-1,0);

			if (pSrc[k*3+0]==0 && pSrc[k*3+1]==255 && pSrc[k*3+2]==0)
			{
				lstVerts2D.push_back(vPos);
				vMins2D.CheckMin(vPos);vMaxs2D.CheckMax(vPos);
			}
			else
			if (pSrc[k*3+0]==255 && pSrc[k*3+1]==0 && pSrc[k*3+2]==255)
			{
				m_pFace->v2DCenter=vPos;
				lstVerts2D.push_back(vPos);
				vMins2D.CheckMin(vPos);vMaxs2D.CheckMax(vPos);
			}
		} //k

		// first center
		m_pFace->v2DCenter.z=0.476;
		vector3f v1;
		// get the center at some distance
		tFace.UnProjectFromScreen(m_pFace->v2DCenter,v1);
		tFace.vOffset=v1-tFace.v3DCenter;		

		assert(nVerts==lstVerts2D.size());		
		// scrivere su schermo errore		
	}

	//////////////////////////////////////////////////////////////////////////
	int nMatches[256];
	CMatrixf mObj;
	mObj.Identity();				
	mObj.TranslateMatrix(tFace.vOffset);
	mObj.RotateMatrix(m_pFace->vAngles);
	//mObj.ScaleMatrix(m_pFace->vScale.x,m_pFace->vScale.y,m_pFace->vScale.z);

	for (int k=0;k<nVerts;k++)
	{
		vector3f vPos=mObj.TransformPoint(lstVerts[k]-m_pFace->v3DCenter);
		vPos+=m_pFace->v3DCenter;
		vector3f vDest;
		tFace.ProjectToScreen(vPos,vDest);

		ftype fBest=w*w;
		for (int k1=0;k1<nVerts;k1++)
		{
			ftype fDist=lstVerts2D[k1].Distance2(vDest);
			if (fDist<fBest)
			{
				fBest=fDist;
				nMatches[k]=k1;
			}
		} //k1
	} //k

	while (pFrame0)
	{		
		cvConvertImage(pFrame,pFrameDraw);

		ftype fTotalError=0;
		
		mObj.Identity();				
		mObj.TranslateMatrix(tFace.vOffset);
		mObj.RotateMatrix(m_pFace->vAngles);
		//mObj.ScaleMatrix(m_pFace->vScale.x,m_pFace->vScale.y,m_pFace->vScale.z);

		for (int k=0;k<nVerts;k++)
		{
			vector3f vPos=mObj.TransformPoint(lstVerts[k]-m_pFace->v3DCenter);
			vPos+=m_pFace->v3DCenter;
			vector3f vDest;
			tFace.ProjectToScreen(vPos,vDest);
			vDest.z=0;
			CvPoint p,q;
			p=cvPoint((int)(lstVerts2D[nMatches[k]].x),(int)(lstVerts2D[nMatches[k]].y));
			q=cvPoint((int)(vDest.x),(int)(vDest.y));			

			ftype fDist=lstVerts2D[nMatches[k]].Distance2(vDest);
			if (fDist<=2)
			{
				cvCircle( pFrameDraw, q, 4, colors[3], 1, 8, 1 );				
			}
			else
				cvDrawLine(pFrameDraw,p,q,colors[1]);

			DrawRectangle(pFrameDraw,vDest-vector3f(0,0,0),vDest+vector3f(0,0,0),colors[6]);
			DrawRectangle(pFrameDraw,lstVerts2D[k]-vector3f(0,0,0),lstVerts2D[k]+vector3f(0,0,0),colors[3]);

			fTotalError+=fDist;
		} //k

		sprintf(szText,"Total square error=%f, average=%f",(float)(fTotalError),(float)(fTotalError/(ftype)(nVerts)));
		cvPutText( pFrameDraw, szText,cvPoint(10,20), &font1, cvScalar(255,0,0) );

		sprintf(szText,"Depth=%f, fov=%f",(float)(m_pFace->v2DCenter.z),(float)(tFace.fFov));
		cvPutText( pFrameDraw, szText,cvPoint(10,40), &font1, cvScalar(0,255,0) );

		vector3f vPos;
		tFace.UnProjectFromScreen(m_pFace->v2DCenter,vPos);
		sprintf(szText,"Center3D=%f %f %f",(float)(vPos.x),(float)(vPos.y),(float)(vPos.z));
		cvPutText( pFrameDraw, szText,cvPoint(10,60), &font1, cvScalar(0,255,0) );

		sprintf(szText,"Angles=%f %f %f",(float)(m_pFace->vAngles.x),(float)(m_pFace->vAngles.y),(float)(m_pFace->vAngles.z));
		cvPutText( pFrameDraw, szText,cvPoint(10,80), &font1, cvScalar(0,255,0) );

		//////////////////////////////////////////////////////////////////////////
		// debug axis
		vector3f v0=mObj.TransformPoint(m_pFace->v3DCenter-m_pFace->v3DCenter);
		v0+=m_pFace->v3DCenter;
		vector3f vDest;tFace.ProjectToScreen(v0,vDest);v0=vDest;
		vector3f v1=mObj.TransformPoint(m_pFace->v3DCenter+vector3f(0.5,0,0)-m_pFace->v3DCenter);
		v1+=m_pFace->v3DCenter;
		tFace.ProjectToScreen(v1,vDest);v1=vDest;
		CvPoint p,q;
		p=cvPoint((int)(v0.x),(int)(v0.y));
		q=cvPoint((int)(v1.x),(int)(v1.y));
		cvDrawLine(pFrameDraw,p,q,colors[0]);

		v0=mObj.TransformPoint(m_pFace->v3DCenter-m_pFace->v3DCenter);
		v0+=m_pFace->v3DCenter;
		tFace.ProjectToScreen(v0,vDest);v0=vDest;
		v1=mObj.TransformPoint(m_pFace->v3DCenter+vector3f(0,0.5,0)-m_pFace->v3DCenter);
		v1+=m_pFace->v3DCenter;
		tFace.ProjectToScreen(v1,vDest);v1=vDest;		
		p=cvPoint((int)(v0.x),(int)(v0.y));
		q=cvPoint((int)(v1.x),(int)(v1.y));
		cvDrawLine(pFrameDraw,p,q,colors[3]);

		v0=mObj.TransformPoint(m_pFace->v3DCenter-m_pFace->v3DCenter);
		v0+=m_pFace->v3DCenter;
		vDest;tFace.ProjectToScreen(v0,vDest);v0=vDest;
		v1=mObj.TransformPoint(m_pFace->v3DCenter+vector3f(0,0,0.5)-m_pFace->v3DCenter);
		v1+=m_pFace->v3DCenter;
		tFace.ProjectToScreen(v1,vDest);v1=vDest;		
		p=cvPoint((int)(v0.x),(int)(v0.y));
		q=cvPoint((int)(v1.x),(int)(v1.y));
		cvDrawLine(pFrameDraw,p,q,colors[6]);

		//////////////////////////////////////////////////////////////////////////

		cvShowImage(m_szWindowFilename, pFrameDraw);
		int key_pressed=cvWaitKey(1);

		if (key_pressed=='q' || key_pressed=='Q')
			break;
		
		if (key_pressed==45 || key_pressed==43)
		{			
			if (key_pressed==43)
				tFace.fFov+=0.25;
			else
				tFace.fFov-=0.25;

			tFace.mProjMatrix.Identity();
			MyPerspective3(tFace.fFov,(ftype)(w)/(ftype)(h),fZMin,fZMax,&tFace.mProjMatrix.m_Tvalues[0][0]);						
			// view matrix		
			tFace.mViewMatrix.Identity();		
			// cached MyProject matrix
			double m[16];
			matmul2(m,&tFace.mProjMatrix.m_Tvalues[0][0],&tFace.mViewMatrix.m_Tvalues[0][0]);				
			memcpy(&tFace.mMatrix.m_Tvalues[0][0],m,sizeof(double)*16);				
			// cached MyUnProject matrix
			invert_matrix(&tFace.mMatrix.m_Tvalues[0][0], m);
			memcpy(&tFace.mInvertedViewMatrix.m_Tvalues[0][0],m,sizeof(CMatrixf));

			vector3f v1;
			// get the center at some distance
			tFace.UnProjectFromScreen(m_pFace->v2DCenter,v1);
			tFace.vOffset=v1-tFace.v3DCenter;		
		}

		//////////////////////////////////////////////////////////////////////////
		if (key_pressed=='r' || key_pressed=='R')
		{
			vector3f	vProj;
			vector3f	vPos1,vPos2;				

			ftype	fRange=20; // very large range
			ftype fBestError=fTotalError;
			vector3f v2DStart=m_pFace->v2DCenter;			
			ftype fFovStart=tFace.fFov;

			ftype fBestFov=fFovStart;
			vector3f vBest2DCenter=v2DStart;

			while (fRange>0.005)
			{				
				ftype fStep=fRange/64.0;

				for (ftype fFov=fFovStart-fRange;fFov<=fFovStart+fRange;fFov+=fStep)
				{					
					if (fFov<15 || fFov>120)
						continue;

					tFace.mProjMatrix.Identity();
					MyPerspective3(fFov,(ftype)(w)/(ftype)(h),fZMin,fZMax,&tFace.mProjMatrix.m_Tvalues[0][0]);						
					// view matrix		
					tFace.mViewMatrix.Identity();		
					// cached MyProject matrix
					double m[16];
					matmul2(m,&tFace.mProjMatrix.m_Tvalues[0][0],&tFace.mViewMatrix.m_Tvalues[0][0]);				
					memcpy(&tFace.mMatrix.m_Tvalues[0][0],m,sizeof(double)*16);				
					// cached MyUnProject matrix
					invert_matrix(&tFace.mMatrix.m_Tvalues[0][0], m);
					memcpy(&tFace.mInvertedViewMatrix.m_Tvalues[0][0],m,sizeof(CMatrixf));

					ftype fZRange=fRange/30.0;
					ftype fZStep=fZRange/64.0;
					
					for (ftype fZ=v2DStart.z-fZRange;fZ<=v2DStart.z+fZRange;fZ+=fZStep)
					{
						if (fZ<=0 || fZ>=1)
							continue;

						vector3f v1;
						// get the center at some distance
						tFace.UnProjectFromScreen(vector3f(v2DStart.x,v2DStart.y,fZ),v1);
						tFace.vOffset=v1-tFace.v3DCenter;	

						mObj.Identity();				
						mObj.TranslateMatrix(tFace.vOffset);
						//mObj.RotateMatrix(m_pFace->vAngles);

						ftype fErrorSum=0;

						for (int k=0;k<nVerts;k++)
						{
							vector3f vPos=mObj.TransformPoint(lstVerts[k]-m_pFace->v3DCenter);
							vPos+=m_pFace->v3DCenter;
							vector3f vDest;
							tFace.ProjectToScreen(vPos,vDest);

							ftype fDist=lstVerts2D[nMatches[k]].Distance2(vDest);

							fErrorSum+=fDist;
							if (fErrorSum>fBestError)
								break;
						} //k

						if (fErrorSum<fBestError)
						{
							fBestError=fErrorSum;
							vBest2DCenter=vector3f(v2DStart.x,v2DStart.y,fZ);
							fBestFov=fFov;
						}
					} //fZ
				} //fFov

				MyOutputDebugString("Step1 - Range=%f,BestError=%f\n",(float)(fRange),(float)(fBestError));

				fFovStart=fBestFov;
				v2DStart=vBest2DCenter;
				fRange/=2.0;				
			}

			tFace.fFov=fBestFov;
			m_pFace->v2DCenter=vBest2DCenter;

			tFace.mProjMatrix.Identity();
			MyPerspective3(tFace.fFov,(ftype)(w)/(ftype)(h),fZMin,fZMax,&tFace.mProjMatrix.m_Tvalues[0][0]);						
			// view matrix		
			tFace.mViewMatrix.Identity();		
			// cached MyProject matrix
			double m[16];
			matmul2(m,&tFace.mProjMatrix.m_Tvalues[0][0],&tFace.mViewMatrix.m_Tvalues[0][0]);				
			memcpy(&tFace.mMatrix.m_Tvalues[0][0],m,sizeof(double)*16);				
			// cached MyUnProject matrix
			invert_matrix(&tFace.mMatrix.m_Tvalues[0][0], m);
			memcpy(&tFace.mInvertedViewMatrix.m_Tvalues[0][0],m,sizeof(CMatrixf));

			vector3f v1;
			// get the center at some distance
			tFace.UnProjectFromScreen(m_pFace->v2DCenter,v1);
			tFace.vOffset=v1-tFace.v3DCenter;	

			mObj.Identity();				
			mObj.TranslateMatrix(tFace.vOffset);
			mObj.RotateMatrix(m_pFace->vAngles);

			//////////////////////////////////////////////////////////////////////////

			vector3f vRotStart=m_pFace->vAngles;
			vector3f vBestRot=vRotStart;

			fRange=5.0; // very large range
			fBestError=fTotalError;

			while (fRange>0.05)
			{		
				ftype fStep=fRange/16.0;				

				for (ftype xrot=vRotStart.x-fRange;xrot<=vRotStart.x+fRange;xrot+=fStep)
				{	
					for (ftype zrot=vRotStart.z-fRange;zrot<=vRotStart.z+fRange;zrot+=fStep)
					{			
						for (ftype yrot=vRotStart.y-fRange;yrot<=vRotStart.y+fRange;yrot+=fStep)
						{	
							ftype fErrorSum=0;

							CMatrixf mObjTemp;
							mObjTemp.Identity();				
							mObjTemp.TranslateMatrix(tFace.vOffset);
							mObjTemp.RotateMatrix(vector3f(xrot,yrot,zrot));							

							for (int k=0;k<nVerts;k++)
							{
								vector3f vPos=mObjTemp.TransformPoint(lstVerts[k]-m_pFace->v3DCenter);
								vPos+=m_pFace->v3DCenter;
								vector3f vDest;
								tFace.ProjectToScreen(vPos,vDest);

								ftype fDist=lstVerts2D[nMatches[k]].Distance2(vDest);

								fErrorSum+=fDist;
								if (fErrorSum>fBestError)
									break;
							} //k

							if (fErrorSum<fBestError)
							{
								fBestError=fErrorSum;
								vBestRot.Set(xrot,yrot,zrot);
							}
						} //y
					} //z
				} //x

				MyOutputDebugString("Step 2 - Range=%f,BestError=%f\n",(float)(fRange),(float)(fBestError));
				vRotStart=vBestRot;
				fRange/=2.0;				
			} //fRange

			m_pFace->vAngles=vBestRot;
			MyOutputDebugString("Done\n");
		} //r
		//////////////////////////////////////////////////////////////////////////

		if (key_pressed=='e' || key_pressed=='E')
		{

			FILE *fp=fopen("faceexport.mtl","wt");
			fprintf(fp,"newmtl test1\n");
			fprintf(fp,"	Ns 0\n");
			fprintf(fp,"	d 1\n");
			fprintf(fp,"	illum 2\n");
			fprintf(fp,"	Kd 0.8 0.8 0.8\n");
			fprintf(fp,"	Ks 0 0 0\n");
			fprintf(fp,"	Ks 0.225 0.225 0.225\n");
			strcpy(szFilename,_szFilename);
			ReplaceExtension(szFilename,"bmp");
			fprintf(fp,"	map_Kd %s\n",GetFilename(szFilename));

			fclose(fp);

			fp=fopen("faceexport.obj","wt");
			fprintf(fp,"mtllib faceexport.mtl\n");
			// export obj
			for (int k=0;k<nVerts;k++)
			{
				vector3f vPos=lstVerts[k];				
				fprintf(fp,"v %0.6f %0.6f %0.6f\n",vPos.x,vPos.y,vPos.z);
			} //k
			fprintf(fp,"\n");

			// 2d triangulation	
			Vector2f *akVertex=new Vector2f [nVerts];	
				
			MyOutputDebugString("Vertices to triangulate: %d \n",nVerts);

			for (int k=0;k<nVerts;k++)
			{
				vector3f vDest;
				
				vector3f vPos=mObj.TransformPoint(lstVerts[k]-m_pFace->v3DCenter);
				vPos+=m_pFace->v3DCenter;
				tFace.ProjectToScreen(vPos,vDest);

				vDest.x/=(ftype)(w);
				vDest.y=(vDest.y)/(ftype)(h);
				fprintf(fp,"vt %0.6f %0.6f\n",vDest.x,vDest.y);

				akVertex[k]=Vector2f((float)(vDest.x),(float)(vDest.y));			
			} //k
			fprintf(fp,"\n");
			fprintf(fp,"usemtl Test1\n");
			fprintf(fp,"g Group1\n");

			int *OutVerts,*OutAdj;
			int nOutTriang;
			Delaunay2af *delaunay=new Delaunay2af(nVerts,akVertex,nOutTriang,OutVerts,OutAdj);

			MyOutputDebugString("Generated %d triangles \n",nOutTriang);

			//The i-th triangle has edge index pairs
			//   edge0 = <raiTVertex[3*i],raiTVertex[3*i+1]>
			//   edge1 = <raiTVertex[3*i+1],raiTVertex[3*i+2]>
			//   edge2 = <raiTVertex[3*i+2],raiTVertex[3*i]>			
			for (int k=0;k<nOutTriang;k++)	
			{
				// get triangle's edges
				int i1=OutVerts[k*3+2]+1;	
				int i2=OutVerts[k*3+1]+1;	
				int i3=OutVerts[k*3+0]+1;	

				fprintf(fp,"f %d/%d %d/%d %d/%d\n",i1,i1,i2,i2,i3,i3);				
			}

			delete delaunay;
			SAFE_DELETE_ARRAY(OutVerts);
			SAFE_DELETE_ARRAY(OutAdj);
			SAFE_DELETE_ARRAY(akVertex);

			/*
			for (int k=0;k<nTris;k++)
			{
				int i1,i2,i3;
				i1=lstTris[k*3+2]+1;
				i2=lstTris[k*3+1]+1;
				i3=lstTris[k*3+0]+1;				
				fprintf(fp,"f %d/%d %d/%d %d/%d\n",i1,i1,i2,i2,i3,i3);
			} //k
			*/
			fprintf(fp,"\n");
			fclose(fp);
			MyOutputDebugString("Obj exported\n");
			
		}
	}

	//////////////////////////////////////////////////////////////////////////

	//cvReleaseImage(&frame1_1C);
	cvReleaseImage(&pFrame0);
	cvReleaseImage(&pFrame);
	cvReleaseImage(&pFrameDraw);
	cvvDestroyWindow(m_szWindowFilename);
	cvReleaseCapture(&input_video);
	//cvReleaseMemStorage(&tFace.storage);

	return (true);
}