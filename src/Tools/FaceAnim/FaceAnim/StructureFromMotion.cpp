
#include "stdafx.h"
#include "FaceAnim.h"

#include "FaceAnimDoc.h"
#include "FaceAnimView.h"
#include "CalibrationAVI4.h"

//////////////////////////////////////////////////////////////////////////
// 3D points
//////////////////////////////////////////////////////////////////////////
bool CFaceAnimDoc::Calc3DInfo2(tFeatureProcessing *tFrame)
{
	tFaceDetect *pFace=tFrame->pFace;

	//////////////////////////////////////////////////////////////////////////
	// taken from a generic 3d model
	// x right
	// y inside screen
	// z up

	//////////////////////////////////////////////////////////////////////////

	// Setup a camera with arbitrary focal length and extract approximate 3d points.			
	pFace->mProjMatrix.Identity();			
	ftype fZMin=0.05;
	ftype fZMax=50;

	// note: the FOV is wrong, must be specified directly in degrees, however this 
	// gives better results, probably because the face is very close when recording the videos 
	// - see also objloader2.cpp
	MyPerspective2(RAD2DEG(60.0),(ftype)(tFrame->pDrawingFrame->width)/(ftype)(tFrame->pDrawingFrame->height),fZMin,fZMax,&pFace->mProjMatrix.m_Tvalues[0][0]);
	//MyPerspective2(60.0,(ftype)(tFrame->pDrawingFrame->width)/(ftype)(tFrame->pDrawingFrame->height),fZMin,fZMax,&pFace->mProjMatrix.m_Tvalues[0][0]);

	// X right
	// Y up
	// Z out of the screen			
	pFace->mViewMatrix.Identity();
	pFace->mViewMatrix.TranslateMatrix(vector3f(0,0,20));
	pFace->mViewMatrix.MultMatrixf(&pFace->mProjMatrix.m_Tvalues[0][0]);

	pFace->nViewport[0]=0;pFace->nViewport[2]=tFrame->pDrawingFrame->width;
	pFace->nViewport[1]=0;pFace->nViewport[3]=tFrame->pDrawingFrame->height;

	double m[16];
	invert_matrix(&pFace->mViewMatrix.m_Tvalues[0][0], m);
	memcpy(&pFace->mInvertedViewMatrix.m_Tvalues[0][0],m,sizeof(CMatrixf));		
	tFrame->vRotation.Clear();

	int w=pFace->pRefFrame->width;
	int h=pFace->pRefFrame->height;

	// find features in ref frame
	int nFeaturesFound=0;
	uchar *pSrc=(uchar *)pFace->pRefFrame->imageData;
	pFace->v2DMins.SetMax();
	pFace->v2DMaxs.SetMin();
	for (int k=0;k<w*h;k++)
	{
		int r,g,b;
		r=pSrc[k*3+0];
		g=pSrc[k*3+1];
		b=pSrc[k*3+2];

		if (r==0 && g==255 && b==0)
		{
			int y=k/w;
			int x=k-y*w;
			pFace->lst_ref_Markers[nFeaturesFound].v2DPos.x=x;
			pFace->lst_ref_Markers[nFeaturesFound].v2DPos.y=y;
			pFace->lst_ref_Markers[nFeaturesFound].v2DPos.z=1.0;

			pFace->lst_ref_Markers[nFeaturesFound].nAssignedMarker1=-1;
			pFace->lst_ref_Markers[nFeaturesFound].nAssignedMarker2=-1;

			pFace->v2DMins.CheckMin(pFace->lst_ref_Markers[nFeaturesFound].v2DPos);
			pFace->v2DMaxs.CheckMax(pFace->lst_ref_Markers[nFeaturesFound].v2DPos);

			nFeaturesFound++;
		}
	} //k

	if (nFeaturesFound!=FA_NUM_MARKERS)
	{
		AfxMessageBox("Number of features mismatch-the BMP file doesn't have enough markers",MB_OK | MB_ICONERROR);		
		return (false);		
	}

	//////////////////////////////////////////////////////////////////////////
	// estimate x/y bbox size...
	vector3f vMins,vMaxs;
	vector3f	vCenter(0,0,0);
	vMins.SetMax();
	vMaxs.SetMin();	
	ftype fUnknownZ=0.99;
	for (int k=0;k<FA_NUM_MARKERS;k++)
	{
		vector3f vSource,vDest;
		vSource.x=pFace->lst_ref_Markers[k].v2DPos.x;
		vSource.y=pFace->lst_ref_Markers[k].v2DPos.y;

		CvPoint p;
		p.x = (int) vSource.x;
		p.y = (int) vSource.y;
		cvCircle(tFrame->pDrawingFrame,p,3,CV_RGB(0,0,255),1);

		vSource.z=fUnknownZ;
		UnProjectFromScreen(vSource,vDest,tFrame);
		vMins.CheckMin(vDest);
		vMaxs.CheckMax(vDest);		

		vCenter+=vDest;
	} //k	
	vCenter/=(ftype)(FA_NUM_MARKERS);

	// initial estimate
	vector3f vScale;
	vScale.x=(vMaxs.x-vMins.x)/(pFace->vMaxs.x-pFace->vMins.x);
	vScale.y=(vMaxs.y-vMins.y)/(pFace->vMaxs.y-pFace->vMins.y);
	// z is an estimation	
	ftype fRatio=(vScale.x+vScale.y)/2.0;
	vScale.z=fRatio/sqr((pFace->vMaxs.z-pFace->vMins.z)*2.0);	


	//////////////////////////////////////////////////////////////////////////
	// scale and move to the center
	pFace->vRef_3DCenter.Magnitude(vScale);	

	vector3f vDiff=vCenter-pFace->vRef_3DCenter;

	for (int k=0;k<FA_NUM_MARKERS;k++)
	{
		ftype fZ=pFace->lst_ref_Markers[k].v3DPos.z-pFace->vMins.z;

		pFace->lst_ref_Markers[k].v3DPos.Magnitude(vScale);

		pFace->lst_ref_Markers[k].v3DPos.x+=vDiff.x;
		pFace->lst_ref_Markers[k].v3DPos.y+=vDiff.y;

		pFace->lst_ref_Markers[k].v3DPos.z=vCenter.z+fZ*vScale.z;

	} //k	

	//////////////////////////////////////////////////////////////////////////
	// assign markers

	vector3f	ref_lst_temp_markers[FA_NUM_MARKERS];
	int nMismatch=0;

	for (int k=0;k<FA_NUM_MARKERS;k++)
	{
		vector3f vSource,vDest;
		vSource=pFace->lst_ref_Markers[k].v3DPos;		

		ProjectToScreen(vSource,vDest,tFrame);		
		vDest.z=1.0;

		CvPoint p;
		p.x = (int) vDest.x;
		p.y = (int) vDest.y;
		// original pos
		if (k==FA_NOSE)
			cvCircle(tFrame->pDrawingFrame,p,2,CV_RGB(128,128,0),-1);
		else
			cvCircle(tFrame->pDrawingFrame,p,2,CV_RGB(128,0,0),-1);		


		// find closest
		int		nBestPos=0;
		ftype	fBestDist=9999;
		for (int k1=0;k1<FA_NUM_MARKERS;k1++)
		{
			ftype fDist2=vDest.Distance2(pFace->lst_ref_Markers[k1].v2DPos);
			if (fDist2<fBestDist)
			{
				nBestPos=k1;
				fBestDist=fDist2;
			}
		} //k1

		//ref_lst_temp_markers[k]=pFace->lst_ref_Markers[nBestPos].v2DPos;

		if (pFace->lst_ref_Markers[nBestPos].nAssignedMarker1==-1)
			pFace->lst_ref_Markers[nBestPos].nAssignedMarker1=k;
		else
		{
			if (pFace->lst_ref_Markers[nBestPos].nAssignedMarker2==-1)
				pFace->lst_ref_Markers[nBestPos].nAssignedMarker2=k;
			else
			{
				AfxMessageBox("Too many mismatched markers between the reference model and the picture",MB_OK | MB_ICONERROR);		
				return (false);
			}

			nMismatch++;
			if (nMismatch>1)
			{
				AfxMessageBox("Too many mismatched markers between the reference model and the picture",MB_OK | MB_ICONERROR);		
				return (false);
			}
		}		
	} //k	
	
	if (nMismatch>0)
	{		
		int	nMissing=-1;
		int nDoubleMarker=-1;
		// find the 2 markers assigned to the same spot and the missing one
		for (int k=0;k<FA_NUM_MARKERS;k++)
		{
			if (pFace->lst_ref_Markers[k].nAssignedMarker1==-1)
			{
				nMissing=k;
				continue;
			}

			if (pFace->lst_ref_Markers[k].nAssignedMarker2!=-1)
			{
				nDoubleMarker=k;				
			}
		} //k

		if (nDoubleMarker==-1 || nMissing==-1)
		{		
			AfxMessageBox("Cannot find the 2 mismatched markers",MB_OK | MB_ICONERROR);		
			return (false);
		}

		// see which configuration will cause the smallest error		
		vector3f vSource,v1,v2;

		ftype fError1=0,fError2=0;
		vSource=pFace->lst_ref_Markers[pFace->lst_ref_Markers[nDoubleMarker].nAssignedMarker1].v3DPos;
		ProjectToScreen(vSource,v1,tFrame);v1.z=1.0;		
		vSource=pFace->lst_ref_Markers[pFace->lst_ref_Markers[nDoubleMarker].nAssignedMarker2].v3DPos;
		ProjectToScreen(vSource,v2,tFrame);v2.z=1.0;	

		fError1+=v1.Distance2(pFace->lst_ref_Markers[nDoubleMarker].v2DPos);
		fError1+=v2.Distance2(pFace->lst_ref_Markers[nMissing].v2DPos);

		fError2+=v1.Distance2(pFace->lst_ref_Markers[nMissing].v2DPos);
		fError2+=v2.Distance2(pFace->lst_ref_Markers[nDoubleMarker].v2DPos);

		if (fError1<fError2)
		{
			pFace->lst_ref_Markers[nMissing].nAssignedMarker1=pFace->lst_ref_Markers[nDoubleMarker].nAssignedMarker2;
			pFace->lst_ref_Markers[nDoubleMarker].nAssignedMarker1=pFace->lst_ref_Markers[nDoubleMarker].nAssignedMarker1;			
		}
		else
		{
			pFace->lst_ref_Markers[nMissing].nAssignedMarker1=pFace->lst_ref_Markers[nDoubleMarker].nAssignedMarker1;
			pFace->lst_ref_Markers[nDoubleMarker].nAssignedMarker1=pFace->lst_ref_Markers[nDoubleMarker].nAssignedMarker2;			
		}
	}

	for (int k=0;k<FA_NUM_MARKERS;k++)
	{
		ref_lst_temp_markers[pFace->lst_ref_Markers[k].nAssignedMarker1]=pFace->lst_ref_Markers[k].v2DPos;
	}

	//////////////////////////////////////////////////////////////////////////
	// at this point markers must have been assigned correctly

	for (int k=0;k<FA_NUM_MARKERS;k++)
	{
		// 2d pos
		pFace->lst_ref_Markers[k].v2DPos=ref_lst_temp_markers[k];

		vector3f vSource=pFace->lst_ref_Markers[k].v3DPos;
		vector3f vDest;		

		ProjectToScreen(vSource,vDest,tFrame);

		// add 2d ref position + z from 3d point
		vSource.x=pFace->lst_ref_Markers[k].v2DPos.x;
		vSource.y=pFace->lst_ref_Markers[k].v2DPos.y;
		vSource.z=vDest.z;

		// get back 3d pos
		UnProjectFromScreen(vSource,vDest,tFrame);
		pFace->lst_ref_Markers[k].v3DPos=vDest;

		// set 2d tracked pos
		tFrame->lst_markers[k]=vSource;

		CvPoint p;
		p.x = (int) vSource.x;
		p.y = (int) vSource.y;

		if (k==FA_NOSE)
			cvCircle(tFrame->pDrawingFrame,p,2,CV_RGB(255,255,0),-1);
		else
			cvCircle(tFrame->pDrawingFrame,p,2,CV_RGB(255,0,0),-1);		
	} //k
	pFace->vRef_3DCenter=pFace->lst_ref_Markers[FA_NOSE].v3DPos;

	ftype	fSizeX=pFace->v2DMaxs.x-pFace->v2DMins.x;
	ftype	fSizeY=pFace->v2DMaxs.y-pFace->v2DMins.y;
	ftype	fRegionScale=0.12;
	pFace->face_region.x=(int)(pFace->v2DMins.x-fSizeX*fRegionScale);
	int nClipX=0;
	if (pFace->face_region.x<0)
	{
		nClipX=-pFace->face_region.x;
		pFace->face_region.x=0;
	}
	pFace->face_region.width=(int)(fSizeX+fSizeX*(fRegionScale*2))-nClipX; //0.1+0.1

	pFace->face_region.y=(int)(pFace->v2DMins.y-fSizeY*fRegionScale); //
	int nClipY=0;
	if (pFace->face_region.y<0)
	{
		nClipY=-pFace->face_region.y;
		pFace->face_region.y=0;
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
	// offset
	//pFace->face_region.x=pFace->face_region.width/2;
	//pFace->face_region.y=(int)(pFace->lst_ref_Markers[FA_NOSE].v2DPos.y-pFace->face_region.y);


	pFace->vMins.SetMax();
	pFace->vMaxs.SetMin();

	for (int k=0;k<FA_NUM_MARKERS;k++)
	{
		pFace->lst_ref_Markers[k].fDist2=pFace->vRef_3DCenter.Distance2(pFace->lst_ref_Markers[k].v3DPos);

		vector3f v2=pFace->lst_ref_Markers[k].v2DPos;v2.z=0;
		vector3f v1=pFace->lst_ref_Markers[FA_NOSE].v2DPos;v1.z=0;
		pFace->lst_ref_Markers[k].vDir=v2-v1;
		pFace->lst_ref_Markers[k].vDir.Normalize();

		pFace->vMins.CheckMin(pFace->lst_ref_Markers[k].v3DPos);
		pFace->vMaxs.CheckMax(pFace->lst_ref_Markers[k].v3DPos);

		pFace->lst_3D_dist2[k]=pFace->lst_ref_Markers[k].fDist2;
		pFace->lst_3D_markers[k]=pFace->lst_ref_Markers[k].v3DPos;		
		tFrame->bFound[k]=true;
	}
	
	/*
	for (int k=0;k<pFace->nNumVertices;k++)
	{
		vector3f vSource,vDest;
		vSource=pFace->lst_vModel_3D[k];

		ftype fZ=vSource.z-pFace->vMins.z;

		vSource.Magnitude(vScale);
		vSource.x+=vDiff.x;
		vSource.y+=vDiff.y;
		//vSource.z=vMins.z; 		
		vSource.z=vCenter.z+fZ*vScale.z;

		pFace->lst_vModel_3D[k]=vSource;

		ProjectToScreen(vSource,vDest,tFrame);
		
		DrawRectangle(tFrame->pDrawingFrame,(float)vDest.x,(float)vDest.y,(float)vDest.x,(float)vDest.y,CV_RGB(255,255,255));
	} //k		
	*/

	return (true);
}

//////////////////////////////////////////////////////////////////////////
void CFaceAnimDoc::CalcBestRot2(vector3f &vBestRot,ftype fRange,ftype fStep,const vector3f &vRotStart,ftype &fBestError,tFeatureProcessing *tFrame1)
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
					if (!tFrame1->bFound[k])
						continue; // dont use lost markers for estimation

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
void CFaceAnimDoc::FindRotation2(tFeatureProcessing *tFrame1,tFeatureProcessing *tFrame2)
{
	if (tFrame1->pFace->nIter==0)
	{	
		ftype	fRange=10.0; // very large range	

		// start from previous rotation
		vector3f	vRotStart=tFrame1->vRotation;	
		ftype			fBestError=99999;
		vector3f	vBestRot=vRotStart;

		// first calc one with the current rotation to skip lots of iterations later
		CalcBestRot2(vBestRot,0,10,vRotStart,fBestError,tFrame1);

		while (fBestError>0.5 && fRange>0.05) // iterate until the error is extremely small
		{		
			CalcBestRot2(vBestRot,fRange,fRange/8.0,vRotStart,fBestError,tFrame1);

			vRotStart=vBestRot;
			fRange/=2.0;			
		}
		tFrame1->vRotation=vBestRot;

		//tFrame1->vRotation=(vBestRot+vRotStart)/2.0f;
	}

	
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

	tFrame2->pFace->fGlobalError=0;
	for (int k=0;k<FA_NUM_MARKERS;k++)
	{
		if (tFrame1->pFace->lst_ref_Markers[k].bExclude)
			continue;

		vector3f vPos1=tFrame1->pFace->lst_3D_markers[k];
		vector3f vPos2=matrix2.TransformPoint(vPos1);
		vector3f vProj;
		ProjectToScreen(vPos2,vProj,tFrame1);

		tFrame2->pFace->fGlobalError+=sqr(tFrame1->lst_markers[k].x-vProj.x);
		tFrame2->pFace->fGlobalError+=sqr(tFrame1->lst_markers[k].y-vProj.y);

		/*
		// estimate the position thru the sequence
		vector3f	v1,v2; //v3,v4;
		// combined 2d tracked pos + z found from rotation 
		v1.Set(tFrame1->lst_markers[k].x,tFrame1->lst_markers[k].y,vProj.z);		
		// get another 3d pos 
		UnProjectFromScreen(v1,v2,tFrame1);
		// where would this point be in absence of rotation
		v1=matrix22.TransformPoint(v2);

		// combine
		tFrame1->pFace->lst_3D_markers[k]=(v1+vPos1)/2.0f;		
		*/

		CvPoint p;
		p.x = (int) vProj.x;
		p.y = (int) vProj.y;
		cvCircle(tFrame1->pDrawingFrame,p,3,CV_RGB(0,255,255), -1);
	}

	tFrame1->pFace->v3DCenter=tFrame1->pFace->lst_3D_markers[FA_NOSE];

	for (int k=0;k<FA_NUM_MARKERS;k++)
	{
		ftype fDist2=tFrame1->pFace->v3DCenter.Distance2(tFrame1->pFace->lst_3D_markers[k]);
		tFrame1->pFace->lst_3D_dist2[k]=fDist2;
	} //k

	//matrix2.Identity();		
	//matrix2.RotateMatrixf(tFrame1->nFrame,1,0,0);	//x	
	//matrix2.RotateMatrixf(0,0,1,0);	//z
	//matrix2.RotateMatrixf(0,0,0,1);	//y

	CvPoint p,q;
	vector3f v00=tFrame2->pFace->v3DCenter;
	vector3f v11=v00+vector3f(1,0,0);
	vector3f v22=v00+vector3f(0,1,0);
	vector3f v33=v00+vector3f(0,0,-1);
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

	/*
	for (int k=0;k<tFrame2->pFace->nNumVertices;k++)
	{
		vector3f vSource,vDest;
		vSource=tFrame2->pFace->lst_vModel_3D[k];
		vector3f v0=matrix2.TransformPoint(vSource);
		ProjectToScreen(v0,vDest,tFrame2);
		DrawRectangle(tFrame2->pDrawingFrame,(float)vDest.x,(float)vDest.y,(float)vDest.x,(float)vDest.y,CV_RGB(255,255,255));
	} //k		
	*/

	tFrame2->vRotation=tFrame1->vRotation;
	//tFrame2->pFace->fGlobalError=fBestError;

	MyOutputDebugString("Frame=%d,Error=%f,Rotation=%f,%f,%f\n",tFrame1->nFrame,tFrame2->pFace->fGlobalError,tFrame2->vRotation.x,tFrame2->vRotation.y,tFrame2->vRotation.z);
}
