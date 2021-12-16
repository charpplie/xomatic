
#include "stdafx.h"
#include "FaceAnim.h"

#include "FaceAnimDoc.h"
#include "FaceAnimView.h"
#include "CalibrationAVI4.h"

//////////////////////////////////////////////////////////////////////////
int CFaceAnimDoc::Refine(tFeatureProcessing *tFrame,int nFeatureID,ftype fDistMax,bool bCheckFlow)
{
	// position where it is found in this frame
	ftype fX=(ftype)(tFrame->lst_markers[nFeatureID].x);
	ftype fY=(ftype)(tFrame->lst_markers[nFeatureID].y);

	ftype fXRes=fX;
	ftype fYRes=fY;
	int nFeatures=1;

	// check if there was something there
	for (int i = 0; i < tFrame->nFeatures; i++)
	{			
		if (bCheckFlow && tFrame->lst_optical_flow_found_feature[i]==0)
			continue;

		// feature in this frame
		ftype fx1=tFrame->lst_Frame1_features[i].x;
		ftype fy1=tFrame->lst_Frame1_features[i].y;
		ftype fDist=sqr(fx1-fX)+sqr(fy1-fY);
		if (fDist>fDistMax)
			continue;

		// weight for this point, inverse of squared distance
		//ftype fW=1.0f-(fDist/fDistMax);
		//fXRes+=(fx1*fW)+(fX*(1.0f-fW));
		//fYRes+=(fy1*fW)+(fY*(1.0f-fW));

		fXRes+=fx1;
		fYRes+=fy1;

		nFeatures++;
	} //i

	ftype fX2=fXRes/(ftype)(nFeatures);
	ftype fY2=fYRes/(ftype)(nFeatures);
	
	tFrame->lst_markers[nFeatureID].x=fX2;
	tFrame->lst_markers[nFeatureID].y=fY2;	

	return (nFeatures-1);
}

//////////////////////////////////////////////////////////////////////////
void CFaceAnimDoc::FindOrientation2(tFeatureProcessing * lstFrames, int nFrame,int nStep)
{

	tFeatureProcessing *tFrame=&lstFrames[nFrame];

	// it is dependent on video size
	ftype fDistMax=(ftype)max(tFrame->pFace->v2DMaxs.x-tFrame->pFace->v2DMins.x,tFrame->pFace->v2DMaxs.y-tFrame->pFace->v2DMins.y);
	fDistMax=sqr(fDistMax*0.05f);	
	ftype	fLostMultiplier=3.0;
	int w=tFrame->pDrawingFrame->width;
	int h=tFrame->pDrawingFrame->height;
	
	if (nStep==0)
	{
		// refine
		for (int k=0;k<FA_NUM_MARKERS;k++)
		{
			if (tFrame->pFace->lst_ref_Markers[k].bExclude)
				continue;

			if (!tFrame->bFound[k])			
			{
				int nRes=Refine(tFrame,k,fDistMax*fLostMultiplier);			
				if (!nRes)
					Refine(tFrame,k,fDistMax*fLostMultiplier*2);			

				//CvPoint q;
				//q.x = (int) tFrame->lst_markers[k].x;
				//q.y = (int) tFrame->lst_markers[k].y;
				//cvCircle(tFrame->pDrawingFrame,q,(int)(sqrt((fDistMax*3)+0.5)),CV_RGB(128,0,0), 1);
			}
			else
			{
				int nRes=Refine(tFrame,k,fDistMax);
			}
		} //k

		//////////////////////////////////////////////////////////////////////////
		// check for consistency

		bool	bNeedsConsistencyCheck=false;
		int		nAmbiguosMarkers[FA_NUM_MARKERS];
		for (int k=0;k<FA_NUM_MARKERS;k++)
			nAmbiguosMarkers[k]=-1;

		vector3f	vMins,vMaxs;
		vMins.SetMax();vMaxs.SetMin();
		for (int k=0;k<FA_NUM_MARKERS;k++)
		{
			if (tFrame->pFace->lst_ref_Markers[k].bExclude)
				continue;

			ftype fX1=tFrame->lst_markers[k].x;
			ftype fY1=tFrame->lst_markers[k].y;	

			vMins.CheckMin(tFrame->pFace->lst_3D_markers[k]);
			vMaxs.CheckMax(tFrame->pFace->lst_3D_markers[k]);

			for (int k1=k+1;k1<FA_NUM_MARKERS;k1++)
			{
				if (tFrame->pFace->lst_ref_Markers[k1].bExclude)
					continue;

				ftype fX2=tFrame->lst_markers[k1].x;
				ftype fY2=tFrame->lst_markers[k1].y;	

				ftype fDist=sqr(fX1-fX2)+sqr(fY1-fY2);
				if (fDist<fDistMax*2)
				{
					bNeedsConsistencyCheck=true;
					nAmbiguosMarkers[k1]=k;
					nAmbiguosMarkers[k]=k1;

					CvPoint p;
					p.x=(int)((fX1+fX2)*0.5);
					p.y=(int)((fY1+fY2)*0.5);
					cvCircle(tFrame->pDrawingFrame,p,(int)(sqrt(fDistMax*2)+0.5),CV_RGB(255,255,0), 1);
				}
			} //k1		
		} //k

		if (bNeedsConsistencyCheck)
		{
			if (!m_bBatchMode)
				m_bEditMode=true; // stop it so people can fix it

			tFaceDetect *pFace=tFrame->pFace;
			vector3f vScale;
			vScale.x=(vMaxs.x-vMins.x)/(pFace->vMaxs.x-pFace->vMins.x);
			vScale.y=(vMaxs.y-vMins.y)/(pFace->vMaxs.y-pFace->vMins.y);
			//vScale.z=(pFace->vMaxs.z-pFace->vMins.z)/(vMaxs.z-vMins.z);

			CMatrixf matrix2;
			matrix2.Identity();		
			matrix2.RotateMatrixf(tFrame->vRotation.x,1,0,0);	//pitch	
			matrix2.RotateMatrixf(tFrame->vRotation.z,0,1,0);	//roll
			matrix2.RotateMatrixf(tFrame->vRotation.y,0,0,1);	//yaw		

			for (int k=0;k<FA_NUM_MARKERS;k++)
			{					
				if (tFrame->pFace->lst_ref_Markers[k].bExclude)
					continue;

				if (nAmbiguosMarkers[k]<0)
					continue; 
				
				vector3f vPos1=tFrame->pFace->lst_ref_Markers[k].v3DPos;				
				vPos1.x*=vScale.x;
				vPos1.y*=vScale.y;
				vector3f vPos2=matrix2.TransformPoint(vPos1);
				vector3f vProj1;
				ProjectToScreen(vPos2,vProj1,tFrame);

				ftype			fDist1=sqr(vProj1.x-tFrame->lst_markers[k].x)+sqr(vProj1.y-tFrame->lst_markers[k].y);
				CvPoint p;
				p.x=(int)(vProj1.x);
				p.y=(int)(vProj1.y);
				cvCircle(tFrame->pDrawingFrame,p,3,CV_RGB(0,0,0), -1);

				int k1=nAmbiguosMarkers[k];
				vPos1=tFrame->pFace->lst_ref_Markers[k1].v3DPos;				
				vPos1.x*=vScale.x;
				vPos1.y*=vScale.y;
				vPos2=matrix2.TransformPoint(vPos1);
				vector3f vProj2;
				ProjectToScreen(vPos2,vProj2,tFrame);

				ftype		fDist2=sqr(vProj2.x-tFrame->lst_markers[k1].x)+sqr(vProj2.y-tFrame->lst_markers[k1].y);
				p.x=(int)(vProj2.x);
				p.y=(int)(vProj2.y);
				cvCircle(tFrame->pDrawingFrame,p,3,CV_RGB(0,0,0), -1);

				int nTransfer=-1;
				vector3f vDest;
				if (fDist1<fDist2)
				{
					nTransfer=k1;
					vDest=vProj2;
				}
				else
				{
					nTransfer=k;
					vDest=vProj1;
				}

				nAmbiguosMarkers[k]=-1;
				nAmbiguosMarkers[k1]=-1;
				
				tFrame->bFound[nTransfer]=false;
				tFrame->lst_markers[nTransfer].x=vDest.x;
				tFrame->lst_markers[nTransfer].y=vDest.y;	
				
			} //k
		}

		//////////////////////////////////////////////////////////////////////////

		tFeatureProcessing *tFrame2=&lstFrames[nFrame+1];
		FindRotation2(tFrame,tFrame2);		
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
		int		nCountFeatures[FA_NUM_MARKERS];

		ftype fDirXSum=0;
		ftype fDirYSum=0;
		for (int k=0;k<FA_NUM_MARKERS;k++)
		{
			
			if (pFace->lst_ref_Markers[k].bExclude)
			{
				pFrame2->bFound[k]=false;
				continue;
			}

			// position where it was found in the previous frame.
			ftype fX=(pFrame1->lst_markers[k].x);
			ftype fY=(pFrame1->lst_markers[k].y);

			CvPoint p;
			p.x = (int) fX;
			p.y = (int) fY;

			fDirX[k]=0;
			fDirY[k]=0;
			int nFeatures=0;

			ftype fDistSearch=fDistMax;
			if (!pFrame1->bFound[k])
				fDistSearch*=fLostMultiplier;

			// check if there was something there, and where did it go
			for (int i = 0; i < pFrame1->nFeatures; i++)
			{			
				if (pFrame2->lst_optical_flow_found_feature[i]==0)
					continue;

				// feature in the previous frame
				ftype fx1=pFrame2->lst_Frame1_features[i].x-fX;
				ftype fy1=pFrame2->lst_Frame1_features[i].y-fY;
				ftype fDist=sqr(fx1)+sqr(fy1);
				if (fDist>fDistSearch)
					continue;

				fDirX[k]+=pFrame2->lst_Frame2_features[i].x-pFrame2->lst_Frame1_features[i].x;
				fDirY[k]+=pFrame2->lst_Frame2_features[i].y-pFrame2->lst_Frame1_features[i].y;
				nFeatures++;
			} //i

			nCountFeatures[k]=nFeatures;
 
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

			if (nFeatures<2)
			{
				// use the others to help prediction from this point				
				pFrame2->bFound[k]=false;
				continue;
			}
			
			if (k==FA_NOSE)				
			{
				cvCircle(tFrame->pDrawingFrame,p,2,CV_RGB(255,255,0), -1);
				cvCircle(tFrame->pDrawingFrame,p,(int)(sqrt(fDistSearch)+0.5),CV_RGB(255,255,0), 1);
			}
			else
			{
				cvCircle(tFrame->pDrawingFrame,p,2,CV_RGB(255,0,0), -1);
				cvCircle(tFrame->pDrawingFrame,p,(int)(sqrt(fDistSearch)+0.5),CV_RGB(255,0,0), 1);
			}

			//cvCircle(pFrame2->pDrawingFrame,q,3,CV_RGB(0,255,255), -1);
		} //k		

		fDirXSum/=(float)(FA_NUM_MARKERS);
		fDirYSum/=(float)(FA_NUM_MARKERS);	

		for (int k=0;k<FA_NUM_MARKERS;k++)
		{
			if (pFrame2->bFound[k])
				continue;

			if (pFace->lst_ref_Markers[k].bExclude)
				continue;

			fDirX[k]=0;
			fDirY[k]=0;			
			int nFeaturesTotal=0;			

			for (int i=0;i<OF_REPLACE_FEATURES;i++)
			{
				int nRef=pFace->lst_ref_Markers[k].nFollowFlow[i];
				if (nRef<0)
					continue;
				if (pFrame2->bFound[nRef])				
					nFeaturesTotal+=nCountFeatures[nRef];				
			} //i
			if (nFeaturesTotal>0)
			{
				for (int i=0;i<OF_REPLACE_FEATURES;i++)
				{
					int nRef=pFace->lst_ref_Markers[k].nFollowFlow[i];
					if (nRef<0)
						continue;
					if (pFrame2->bFound[nRef])				
					{
						ftype fRatio=(ftype)(nCountFeatures[nRef])/(ftype)(nFeaturesTotal);
						ftype fX=fDirX[nRef]*fRatio;
						ftype fY=fDirY[nRef]*fRatio;
						fDirX[k]+=fX;
						fDirY[k]+=fY;					
					}
				} //i
			}
			else
			{
				fDirX[k]=fDirXSum;
				fDirY[k]=fDirYSum;				
			}
			
			pFrame2->lst_markers[k].x=pFrame1->lst_markers[k].x+fDirX[k];
			pFrame2->lst_markers[k].y=pFrame1->lst_markers[k].y+fDirY[k];

			CvPoint q;
			q.x = (int) pFrame2->lst_markers[k].x;
			q.y = (int) pFrame2->lst_markers[k].y;
			cvCircle(tFrame->pDrawingFrame,q,3,CV_RGB(255,255,255), -1);
		} //k

		//////////////////////////////////////////////////////////////////////////
		// automatic mask

		if (!pFrame1->bReferenceFrame)
		{		
			//tFaceDetect *pFace=tFrame->pFace;
			pFace->v2DMins.SetMax();
			pFace->v2DMaxs.SetMin();
			for (int k=0;k<FA_NUM_MARKERS;k++)
			{					
				if (tFrame->pFace->lst_ref_Markers[k].bExclude)
					continue;

				pFace->v2DMins.CheckMin(pFrame2->lst_markers[k]);
				pFace->v2DMaxs.CheckMax(pFrame2->lst_markers[k]);
			} //k

			ftype	fSizeX=pFace->v2DMaxs.x-pFace->v2DMins.x;
			ftype	fSizeY=pFace->v2DMaxs.y-pFace->v2DMins.y;
			ftype	fRegionScale=0.12;
			pFace->face_region.x=(int)(pFace->v2DMins.x-fSizeX*fRegionScale);
			int nClipX=0;
			if (pFace->face_region.x<1)
			{
				nClipX=1-pFace->face_region.x;
				pFace->face_region.x=1;
			}
			pFace->face_region.width=(int)(fSizeX+fSizeX*(fRegionScale*2))-nClipX; //0.1+0.1

			pFace->face_region.y=(int)(pFace->v2DMins.y-fSizeY*fRegionScale); //
			int nClipY=0;
			if (pFace->face_region.y<1)
			{
				nClipY=1-pFace->face_region.y;
				pFace->face_region.y=1;
			}
			pFace->face_region.height=(int)(fSizeY+fSizeY*(fRegionScale*2+0.05))-nClipY; // y extent bigger

			if ((pFace->face_region.x+pFace->face_region.width)>=w-5)
			{
				nClipX=pFace->face_region.x+pFace->face_region.width-(w-5);
				pFace->face_region.width-=nClipX;
			}

			if ((pFace->face_region.y+pFace->face_region.height)>=h-5)
			{
				nClipY=pFace->face_region.y+pFace->face_region.height-(h-5);
				pFace->face_region.height-=nClipY;
			}

			// align to 4
			pFace->face_region.width+=pFace->face_region.width-(4*(pFace->face_region.width/4));
			pFace->face_region.height+=pFace->face_region.height-(4*(pFace->face_region.height/4));

		}
	}

}


