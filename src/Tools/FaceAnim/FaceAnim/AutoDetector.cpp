
#include "stdafx.h"
#include "FaceAnim.h"

#include "FaceAnimDoc.h"
#include "FaceAnimView.h"
#include "CalibrationAVI4.h"


//////////////////////////////////////////////////////////////////////////
void CFaceAnimDoc::FindPosition(tFeatureProcessing *tFrame,ftype x1,ftype y1,ftype x2,ftype y2,ftype x11,ftype y11,ftype x22,ftype y22,ftype &fX1,ftype &fY1,ftype &fX2,ftype &fY2)
{
	fX1=0;
	fY1=0;
	int		nCount1=0;
	fX2=0;
	fY2=0;
	int		nCount2=0;

	for (int i = 0; i < tFrame->nFeatures; i++)
	{			
		if (tFrame->lst_optical_flow_found_feature[i]==0)	
			continue;

		ftype fX=tFrame->lst_Frame1_features[i].x;
		ftype fY=tFrame->lst_Frame1_features[i].y;

		if (InsideBBox2D(x1,y1,x2,y2,fX,fY))
		{
			fX1+=fX;
			fY1+=fY;
			nCount1++;
		}
		else
		if (InsideBBox2D(x11,y11,x22,y22,fX,fY))
		{
			fX2+=fX;
			fY2+=fY;
			nCount2++;
		}
	} //i

	fX1/=(ftype)(nCount1);
	fY1/=(ftype)(nCount1);

	fX2/=(ftype)(nCount2);
	fY2/=(ftype)(nCount2);	
}

//////////////////////////////////////////////////////////////////////////
void CFaceAnimDoc::FindOrientation(tFeatureProcessing * lstFrames, int nFrame,int nStep)
{

	tFeatureProcessing *tFrame=&lstFrames[nFrame];

	// it is dependent on video and face size
	ftype fDistMax=(ftype)max(tFrame->pFace->ref_face.width,tFrame->pFace->ref_face.height);
	fDistMax=sqr(fDistMax*0.05f);			

	if (nStep==0)
	{
		// refine
		for (int k=0;k<FA_NUM_MARKERS;k++)
		{
			if (!tFrame->bFound[k])
				Refine(tFrame,k,fDistMax*3);
			else
				Refine(tFrame,k,fDistMax);

			//CvPoint p;
			//p.x = (int) tFrame->lst_markers[k].x;
			//p.y = (int) tFrame->lst_markers[k].y;
			//cvCircle(tFrame->pDrawingFrame,p,3,CV_RGB(0,255,0), -1);

		} //k

		//////////////////////////////////////////////////////////////////////////
		tFeatureProcessing *tFrame2=&lstFrames[nFrame+1];
		//FindRotation(tFrame,tFrame2);
		//////////////////////////////////////////////////////////////////////////
		 
		if (tFrame->bReferenceFrame)
		{
			tFaceDetect *pFace=tFrame->pFace;
			pFace->vRef_AverageFaceCenter.x=0;
			pFace->vRef_AverageFaceCenter.y=0;
			for (int k=0;k<FA_NUM_MARKERS;k++)
			{
				ftype fX=(tFrame->pFace->ref_lst_markers[k].x);
				ftype fY=(tFrame->pFace->ref_lst_markers[k].y);

				pFace->vRef_AverageFaceCenter.x+=fX;
				pFace->vRef_AverageFaceCenter.y+=fY;	
			} //k		
			pFace->vRef_AverageFaceCenter.x/=(float)(FA_NUM_MARKERS);
			pFace->vRef_AverageFaceCenter.y/=(float)(FA_NUM_MARKERS);

			CvPoint p;
			p.x = (int) pFace->vRef_AverageFaceCenter.x;
			p.y = (int) pFace->vRef_AverageFaceCenter.y;
			cvCircle(tFrame->pDrawingFrame,p,3,CV_RGB(0,255,0), -1);
									
		}

		return;
	}


	if (tFrame->bReferenceFrame)		
	{	
		// find the ref markers for the first frame
		tFaceDetect *pFace=tFrame->pFace;

		float x1=(float)(pFace->ref_eye_left.x);
		float y1=(float)(pFace->ref_eye_left.y+pFace->ref_eye_left.height);
		float x2=(float)(pFace->ref_eye_left.x+pFace->ref_eye_left.width);
		float y2=(float)(pFace->ref_mouth.y);
		x1-=(x2-x1)*0.2f; // expand 20% not really needed but for clarity when debug drawing		
		y2-=(y2-y1)*0.35f; // reduce 40%, not really needed but for clarity when debug drawing
		//DrawRectangle(tFrame->pDrawingFrame,x1,y1,x2,y2,CV_RGB(255,0,0));
		CvRect	cheek_left=cvRect((int)(x1),(int)(y1),(int)(x2-x1),(int)(y2-y1));				

		float x11=(float)(pFace->ref_eye_right.x);
		float y11=(float)(pFace->ref_eye_right.y+pFace->ref_eye_right.height);
		float x22=(float)(pFace->ref_eye_right.x+pFace->ref_eye_right.width);
		float y22=(float)(pFace->ref_mouth.y);
		x22+=(x22-x11)*0.2f; // expand 20% not really needed but for clarity when debug drawing		
		y22-=(y22-y11)*0.35f; // reduce 40%, not really needed but for clarity when debug drawing
		//DrawRectangle(tFrame->pDrawingFrame,x11,y11,x22,y22,CV_RGB(255,0,0));
		CvRect	cheek_right=cvRect((int)(x11),(int)(y11),(int)(x22-x11),(int)(y22-y11));	

		ftype fX1,fY1,fX2,fY2;

		FindPosition(tFrame,x1,y1,x2,y2,x11,y11,x22,y22,fX1,fY1,fX2,fY2);

		pFace->ref_lst_markers[FA_LT_CHEEK].x=fX1;
		pFace->ref_lst_markers[FA_LT_CHEEK].y=fY1;

		pFace->ref_lst_markers[FA_RT_CHEEK].x=fX2;
		pFace->ref_lst_markers[FA_RT_CHEEK].y=fY2;

		CvPoint p,q;
		p.x = (int) fX1;
		p.y = (int) fY1;
		q.x = (int) fX2;
		q.y = (int) fY2;	

		cvCircle(tFrame->pDrawingFrame,p,3,CV_RGB(255,0,0), -1);
		cvCircle(tFrame->pDrawingFrame,q,3,CV_RGB(255,0,0), -1);

		//////////////////////////////////////////////////////////////////////////

		x1=(float)(pFace->ref_eye_left.x);
		y1=(float)(cheek_left.y+cheek_left.height);
		x2=(float)(pFace->ref_mouth.x);
		y2=(float)(pFace->ref_mouth.y+pFace->ref_mouth.height);
		//DrawRectangle(tFrame->pDrawingFrame,x1,y1,x2,y2,CV_RGB(255,0,0));

		x11=(float)(pFace->ref_mouth.x+pFace->ref_mouth.width);
		y11=(float)(cheek_right.y+cheek_right.height);
		x22=(float)(pFace->ref_eye_right.x+pFace->ref_eye_right.width);
		y22=(float)(pFace->ref_mouth.y+pFace->ref_mouth.height);
		//DrawRectangle(tFrame->pDrawingFrame,x11,y11,x22,y22,CV_RGB(255,0,0));

		FindPosition(tFrame,x1,y1,x2,y2,x11,y11,x22,y22,fX1,fY1,fX2,fY2);

		pFace->ref_lst_markers[FA_MOUTH_LT].x=fX1;
		pFace->ref_lst_markers[FA_MOUTH_LT].y=fY1;

		pFace->ref_lst_markers[FA_MOUTH_RT].x=fX2;
		pFace->ref_lst_markers[FA_MOUTH_RT].y=fY2;
		
		p.x = (int) fX1;
		p.y = (int) fY1;
		q.x = (int) fX2;
		q.y = (int) fY2;	

		cvCircle(tFrame->pDrawingFrame,p,3,CV_RGB(255,0,0), -1);
		cvCircle(tFrame->pDrawingFrame,q,3,CV_RGB(255,0,0), -1);

		//////////////////////////////////////////////////////////////////////////

		x1=(float)(cheek_left.x);
		y1=(float)(pFace->ref_eye_left.y-pFace->ref_eye_left.height);
		x2=(float)(cheek_left.x+cheek_left.x+cheek_left.width)*0.5f;
		y2=(float)(pFace->ref_eye_left.y);
		//DrawRectangle(tFrame->pDrawingFrame,x1,y1,x2,y2,CV_RGB(255,0,0));

		x11=(float)(cheek_left.x+cheek_left.x+cheek_left.width)*0.5f;
		y11=(float)(pFace->ref_eye_left.y-pFace->ref_eye_left.height);
		x22=(float)(pFace->ref_eye_left.x+pFace->ref_eye_left.width);
		y22=(float)(pFace->ref_eye_left.y);
		//DrawRectangle(tFrame->pDrawingFrame,x11,y11,x22,y22,CV_RGB(255,0,0));

		FindPosition(tFrame,x1,y1,x2,y2,x11,y11,x22,y22,fX1,fY1,fX2,fY2);

		pFace->ref_lst_markers[FA_LT_EYEBROW_LT].x=fX1;
		pFace->ref_lst_markers[FA_LT_EYEBROW_LT].y=fY1;

		pFace->ref_lst_markers[FA_LT_EYEBROW_RT].x=fX2;
		pFace->ref_lst_markers[FA_LT_EYEBROW_RT].y=fY2;
		
		p.x = (int) fX1;
		p.y = (int) fY1;
		q.x = (int) fX2;
		q.y = (int) fY2;	

		cvCircle(tFrame->pDrawingFrame,p,3,CV_RGB(255,0,0), -1);
		cvCircle(tFrame->pDrawingFrame,q,3,CV_RGB(255,0,0), -1);

		//////////////////////////////////////////////////////////////////////////

		x1=(float)(pFace->ref_eye_right.x);
		y1=(float)(pFace->ref_eye_right.y-pFace->ref_eye_right.height);
		x2=(float)(cheek_right.x+cheek_right.x+cheek_right.width)*0.5f;
		y2=(float)(pFace->ref_eye_right.y);
		//DrawRectangle(tFrame->pDrawingFrame,x1,y1,x2,y2,CV_RGB(255,0,0));

		x11=(float)(cheek_right.x+cheek_right.x+cheek_right.width)*0.5f;
		y11=(float)(pFace->ref_eye_right.y-pFace->ref_eye_right.height);
		x22=(float)(cheek_right.x+cheek_right.width);
		y22=(float)(pFace->ref_eye_right.y);
		//DrawRectangle(tFrame->pDrawingFrame,x11,y11,x22,y22,CV_RGB(255,0,0));

		FindPosition(tFrame,x1,y1,x2,y2,x11,y11,x22,y22,fX1,fY1,fX2,fY2);

		pFace->ref_lst_markers[FA_RT_EYEBROW_LT].x=fX1;
		pFace->ref_lst_markers[FA_RT_EYEBROW_LT].y=fY1;

		pFace->ref_lst_markers[FA_RT_EYEBROW_RT].x=fX2;
		pFace->ref_lst_markers[FA_RT_EYEBROW_RT].y=fY2;
		
		p.x = (int) fX1;
		p.y = (int) fY1;
		q.x = (int) fX2;
		q.y = (int) fY2;	

		cvCircle(tFrame->pDrawingFrame,p,3,CV_RGB(255,0,0), -1);
		cvCircle(tFrame->pDrawingFrame,q,3,CV_RGB(255,0,0), -1);

		pFace->vRef_AverageFaceCenter.x=0;
		pFace->vRef_AverageFaceCenter.y=0;
		for (int k=0;k<FA_NUM_MARKERS;k++)
		{
			pFace->vRef_AverageFaceCenter.x+=(tFrame->pFace->ref_lst_markers[k].x);
			pFace->vRef_AverageFaceCenter.y+=(tFrame->pFace->ref_lst_markers[k].y);						
		} //k		
		pFace->vRef_AverageFaceCenter.x/=(float)(FA_NUM_MARKERS);
		pFace->vRef_AverageFaceCenter.y/=(float)(FA_NUM_MARKERS);

		for (int k=0;k<FA_NUM_MARKERS;k++)
		{
			//float fX=(pFace->ref_lst_markers[k].x-pFace->vRef_AverageFaceCenter.x);
			//float fY=(pFace->ref_lst_markers[k].y-pFace->vRef_AverageFaceCenter.y);

			// copy to current frame			
			tFrame->lst_markers[k]=pFace->ref_lst_markers[k];
			tFrame->bFound[k]=true;
		} //k		

		Calc3DInfo(tFrame);
	}
	else
	{				
		// when current_frame is N, here is passed N+1
		// optical flow is from N to N+1		
		tFeatureProcessing *pFrame1=&lstFrames[nFrame-1];
		tFeatureProcessing *pFrame2=&lstFrames[nFrame];
		tFaceDetect *pFace=pFrame1->pFace;
		
		ftype fDirX[FA_NUM_MARKERS];
		ftype fDirY[FA_NUM_MARKERS];

		ftype fDirXSum=0;
		ftype fDirYSum=0;
		for (int k=0;k<FA_NUM_MARKERS;k++)
		{
			// position where it was found in the previous frame.
			ftype fX=(pFrame1->lst_markers[k].x);
			ftype fY=(pFrame1->lst_markers[k].y);

			CvPoint p;
			p.x = (int) fX;
			p.y = (int) fY;

			fDirX[k]=0;
			fDirY[k]=0;
			int nFeatures=0;

			// check if there was something there, and where did it go
			for (int i = 0; i < pFrame1->nFeatures; i++)
			{			
				if (pFrame2->lst_optical_flow_found_feature[i]==0)
					continue;

				// feature in the previous frame
				ftype fx1=pFrame2->lst_Frame1_features[i].x-fX;
				ftype fy1=pFrame2->lst_Frame1_features[i].y-fY;
				ftype fDist=sqr(fx1)+sqr(fy1);
				if (fDist>fDistMax)
					continue;

				fDirX[k]+=pFrame2->lst_Frame2_features[i].x-pFrame2->lst_Frame1_features[i].x;
				fDirY[k]+=pFrame2->lst_Frame2_features[i].y-pFrame2->lst_Frame1_features[i].y;
				nFeatures++;
			} //i
						
			if (nFeatures<1)
			{
				// nothing was found - predict from the others		
				pFrame2->lst_markers[k]=pFrame1->lst_markers[k];
				//cvCircle(pFrame2->pDrawingFrame,p,3,CV_RGB(255,0,0), -1);

				pFrame2->bFound[k]=false;
				continue;
			}

			pFrame2->bFound[k]=true;
			fDirX[k]=(fDirX[k]/(float)(nFeatures));
			fDirY[k]=(fDirY[k]/(float)(nFeatures));
			fDirXSum+=fDirX[k];
			fDirYSum+=fDirY[k];
			pFrame2->lst_markers[k].x=pFrame1->lst_markers[k].x+fDirX[k];
			pFrame2->lst_markers[k].y=pFrame1->lst_markers[k].y+fDirY[k];

			//Refine(pFrame2,k,fDistMax);
 
			cvCircle(pFrame2->pDrawingFrame,p,3,CV_RGB(0,255,0), -1);
			//int nRadius=(int)(sqrt(fDistMax));
			//cvCircle(tFrame->pDrawingFrame,p,nRadius,CV_RGB(0,255,0));

			CvPoint q;
			q.x = (int) pFrame2->lst_markers[k].x;
			q.y = (int) pFrame2->lst_markers[k].y;
			//cvCircle(pFrame2->pDrawingFrame,q,3,CV_RGB(0,255,255), -1);
		} //k		

		fDirXSum/=(float)(FA_NUM_MARKERS);
		fDirYSum/=(float)(FA_NUM_MARKERS);

		int nRefs[FA_NUM_MARKERS]=
		{
			FA_LT_EYEBROW_RT, //FA_LT_EYEBROW_LT 0 
			FA_LT_EYEBROW_LT, //FA_LT_EYEBROW_RT 1
			FA_RT_EYEBROW_RT, //FA_RT_EYEBROW_LT 2
			FA_RT_EYEBROW_RT, //FA_RT_EYEBROW_RT 3
			FA_RT_CHEEK, //FA_LT_CHEEK 4
			FA_LT_CHEEK, //FA_RT_CHEEK 5
			FA_MOUTH_RT, //FA_MOUTH_LT 6
			FA_MOUTH_LT//FA_MOUTH_RT 7			
		};

		int nRefs2[FA_NUM_MARKERS]=
		{
			FA_RT_EYEBROW_LT, //FA_LT_EYEBROW_LT 0 
			FA_RT_EYEBROW_RT, //FA_LT_EYEBROW_RT 1
			FA_LT_EYEBROW_LT, //FA_RT_EYEBROW_LT 2
			FA_LT_EYEBROW_RT, //FA_RT_EYEBROW_RT 3
			FA_RT_CHEEK, //FA_LT_CHEEK 4
			FA_LT_CHEEK, //FA_RT_CHEEK 5
			FA_LT_CHEEK, //FA_MOUTH_LT 6
			FA_RT_CHEEK//FA_MOUTH_RT 7			
		};
		
		//////////////////////////////////////////////////////////////////////////		
		for (int k=0;k<FA_NUM_MARKERS;k++)  
		{
			if (pFrame2->bFound[k])
				continue;
			
			int nRef=nRefs[k];
			if (!pFrame2->bFound[nRef])
			{				
				nRef=nRefs2[k];
				if (!pFrame2->bFound[nRef])
				{									
					pFrame2->lst_markers[k].x=pFrame1->lst_markers[k].x+fDirXSum;
					pFrame2->lst_markers[k].y=pFrame1->lst_markers[k].y+fDirYSum;
				}
				else
				{			
					pFrame2->lst_markers[k].x=pFrame1->lst_markers[k].x+fDirX[nRef];
					pFrame2->lst_markers[k].y=pFrame1->lst_markers[k].y+fDirY[nRef];
				}
			}
			else
			{			
				pFrame2->lst_markers[k].x=pFrame1->lst_markers[k].x+fDirX[nRef];
				pFrame2->lst_markers[k].y=pFrame1->lst_markers[k].y+fDirY[nRef];
			}

			CvPoint p;
			p.x = (int) pFrame2->lst_markers[k].x;
			p.y = (int) pFrame2->lst_markers[k].y;

			//int nRadius=(int)(sqrt(fDistMax));
			//cvCircle(tFrame->pDrawingFrame,p,nRadius,CV_RGB(255,0,255));
			cvCircle(pFrame2->pDrawingFrame,p,3,CV_RGB(255,0,255), -1);
			
		} //k			
	}
}

//////////////////////////////////////////////////////////////////////////
bool CFaceAnimDoc::DetectFace(IplImage *pFrame,tFaceDetect &tFace)
{

	tFace.bMouthDetected=false;
	tFace.bEyeLDetected=false;
	tFace.bEyeRDetected=false;

	IplImage *frame1_1C= tFace.frame1_1C;

	if (tFace.bFlip)
		cvConvertImage(pFrame, frame1_1C, 1);				
	else
		cvConvertImage(pFrame, frame1_1C, 0);				
	uchar *pGray1=(uchar *)frame1_1C->imageData;

	// equalize for face detection etc.
	cvEqualizeHist( frame1_1C, frame1_1C );
	cvClearMemStorage( tFace.storage );

	//cvShowImage("FaceAnim", frame1_1C);
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
	t = (double)cvGetTickCount() - t;
	//MyOutputDebugString( "detection time = %gms\n", t/((double)cvGetTickFrequency()*1000.) );

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

		//cvCircle( frame1, face_center, radius, colors[0], 3, 8, 0 );
		//cvDrawRect(frame1,cvPoint(r->x,r->y),cvPoint(r->x+r->width,r->y+r->height),colors[0]);
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
			//cvDrawRect(frame1,cvPoint(face_rect.x+eye_bestL.x,face_rect.y+eye_bestL.y),cvPoint(face_rect.x+eye_bestL.x+eye_bestL.width,face_rect.y+eye_bestL.y+eye_bestL.height),colors[1]);
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
			//cvDrawRect(frame1,cvPoint(face_rect.x+eye_bestR.x,face_rect.y+eye_bestR.y),cvPoint(face_rect.x+eye_bestR.x+eye_bestR.width,face_rect.y+eye_bestR.y+eye_bestR.height),colors[2]);
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
			//cvDrawRect(frame1,cvPoint(face_rect.x+mouth_best.x,face_rect.y+mouth_best.y),cvPoint(face_rect.x+mouth_best.x+mouth_best.width,face_rect.y+mouth_best.y+mouth_best.height),colors[4]);
			tFace.mouth=cvRect(face_rect.x+mouth_best.x,face_rect.y+mouth_best.y,mouth_best.width,mouth_best.height);
		}

		cvReleaseImage(&pTemp);
	}

	//cvReleaseImage(&frame1_1C);
	
	if (!bFaceFound)
	{		
		tFace.face=tFace.prev_face;
		tFace.eye_left=tFace.prev_eye_left;
		tFace.eye_right=tFace.prev_eye_right;
		tFace.mouth=tFace.prev_mouth;
	}

	if (!tFace.bEyeLDetected)
		tFace.eye_left=tFace.prev_eye_left;

	if (!tFace.bEyeRDetected)
		tFace.eye_right=tFace.prev_eye_right;

	if (!tFace.bMouthDetected)
		tFace.mouth=tFace.prev_mouth;

	return (bFaceFound);	
}
