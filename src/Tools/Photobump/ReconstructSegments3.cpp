


#include "stdafx.h"
#include "PhotoBump10.h"
#include "PhotoBump10Doc.h"
#include "PhotoBump10View.h"
#include "Video.h"
#include "PhotoFrame.h"
#include "PhotoImage.h"
#include "PVertex.h"
#include "ImageProcessing.h"

#include "opencv/cv.h"
//#include "opencv/cvaux.h"
#include "opencv/highgui.h"

#include "WmlApprPlaneFit3.h"
#include "WmlBSplineRectangle.h"

#include "WmlApprParaboloidFit3.h"

#include "LoadingBar.h"

using namespace Wml;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10Doc::SmoothSegments3()
{
	int nFrames=(int)(m_lstFrames.size());

	for (int k1=0;k1<nFrames-1;k1++)
	{	
		int w=m_lstFrames[k1]->m_pImage->m_nWidth;
		int h=m_lstFrames[k1]->m_pImage->m_nHeight;

		CImageProcessing *pImageProc=m_lstFrames[k1]->m_pImage->m_pImageProcessing;
		vector3f *vPoints=m_lstFrames[k1]->m_pPoints;
		CCamera	*pCam=&m_lstFrames[k1]->m_Camera;
		
		ftype *pDepth=m_lstFrames[k1]->m_pBestDepth;

		CPhotoFrame *pFrame=m_lstFrames[k1];
		int *nRegions=pImageProc->m_Regions1;	
		//ftype fMinZ=999999,fMaxZ=-999999;
						
		//////////////////////////////////////////////////////////////////////////
		// step 2:fit the planes using segments	
		
		Vector3d *akPoint=new Vector3d [w*h];

		for (segmlistit i=pImageProc->m_lstSegments.begin();i!=pImageProc->m_lstSegments.end();i++)
		{
			CSegment *pSeg=(*i);
			int nPoints=0;
			// fit plane
			for (int k=0;k<pSeg->m_nNumPixels;k++)
			{
				int nPos=pSeg->m_nPos[k];
				int y=nPos/w;
				int x=nPos-(y*w);

				if (!pFrame->m_pState[y*w+x])
					continue;

				ftype fZ=pFrame->m_pBestDepth[nPos];

				if ((fZ*1.25)<(m_fMinZ))
					fZ=m_fMinZ;
				else
				if ((fZ)>(m_fMaxZ*1.25))
					fZ=m_fMaxZ;

				akPoint[nPoints]=Vector3d((double)(x),(double)(y),fZ);
				nPoints++;
			}

			if (nPoints<3)
			{
				pSeg->Clear(pFrame);
				continue;
			}

			double fA,fB,fC;
			if (!HeightPlaneFit(nPoints,akPoint,fA,fB,fC))
			{
				pSeg->Clear(pFrame);
				continue;
			}
			pSeg->m_vPlane.Set(fA,fB,fC);
			pSeg->m_bFit=true;
		} //i
	 	
		delete [] akPoint;
							
		//////////////////////////////////////////////////////////////////////////
		// step 3:fit all the segments				
		for (segmlistit i=pImageProc->m_lstSegments.begin();i!=pImageProc->m_lstSegments.end();i++)
		{
			CSegment *pSeg=(*i);
			if (!pSeg->m_bFit)
				continue;
			for (int k=0;k<pSeg->m_nNumPixels;k++)
			{
				int nPos=pSeg->m_nPos[k];
				int y=nPos/w;
				int x=nPos-(y*w);
								
				ftype fZ=pSeg->m_vPlane.x*(ftype)(x)+pSeg->m_vPlane.y*(ftype)(y)+pSeg->m_vPlane.z;
				if ((fZ*1.25)<(m_fMinZ))
					fZ=m_fMinZ;
				else
				if ((fZ)>(m_fMaxZ*1.25))
					fZ=m_fMaxZ;
				pDepth[nPos]=fZ;				
			} //k
		}	//i
	} //k1

	//SmoothSegments3Part2(dc);
}

//////////////////////////////////////////////////////////////////////////
void CPhotoBump10Doc::SmoothSegments3Part2(CPaintDC *dc)
{

	CLoadingBar tLoadingBar(m_pVideo,dc,"Step 3 of 3:Generating geometry");

	int w=m_lstFrames[KEY_FRAME]->m_pImage->m_nWidth;
	int h=m_lstFrames[KEY_FRAME]->m_pImage->m_nHeight;
	
	int nFrames=(int)(m_lstFrames.size());

	// work in 3d
	for (int k1=KEY_FRAME;k1<KEY_FRAME+1;k1++)
	//for (int k1=1;k1<2;k1++)
	//for (int k1=0;k1<nFrames-1;k1++)
	{
		CPhotoFrame *pFrame=m_lstFrames[k1];
		CCamera *pCam=&pFrame->m_Camera;
		pFrame->m_pPoints=new vector3f [w*h];
		vector3f *vPoints=m_lstFrames[k1]->m_pPoints;
		if (!pFrame->m_pState)
			pFrame->m_pState=new char [w*h];

		ftype *pDepth=m_lstFrames[k1]->m_pBestDepth;
		memset(pFrame->m_pState,0,w*h);

		char *pState=pFrame->m_pState;
		uchar *pEdges=pFrame->m_pImage->m_pEdges;
		CImageProcessing *pImageProc=m_lstFrames[k1]->m_pImage->m_pImageProcessing;		
		int *nRegions=pImageProc->m_Regions1;	

		//////////////////////////////////////////////////////////////////////////
		// set states remove invalid points
		int nReplaced=0;
		for (int k=0;k<w*h;k++)
		{		
			ftype fZ=pFrame->m_pBestDepth[k];

			if ((fZ*1.25<m_fMinZ) || (fZ>m_fMaxZ*1.25))			
			{								
				pFrame->m_pPoints->Clear();				
				continue;
			}			

			pFrame->m_pState[k]=1;
		}
		
		ftype fMaxDiff=(m_fMaxZ-m_fMinZ)/3000.0;
		typedef std::pair <ftype,int> ZPair;
		typedef std::multimap<ftype,int> ZMultiMatMap;
		typedef ZMultiMatMap::iterator ZMultiMatMapIt;

		ZMultiMatMap lstPeaks;

		for (segmlistit i=pImageProc->m_lstSegments.begin();i!=pImageProc->m_lstSegments.end();i++)
		{
			CSegment *pSeg=(*i);
			lstPeaks.clear();
			for (int k=0;k<pSeg->m_nNumPixels;k++)
			{
				int nPos=pSeg->m_nPos[k];
				int y=nPos/w;
				int x=nPos-(y*w);

				if (!pFrame->m_pState[y*w+x])
					continue;

				ftype fZ=pFrame->m_pBestDepth[nPos];
				
				lstPeaks.insert(ZPair(fZ,nPos));
			}
			if (lstPeaks.empty())
				continue;
			for (ZMultiMatMapIt i2=lstPeaks.begin();i2!=lstPeaks.end();i2++)
			{
				int nPos=i2->second;
				int y=nPos/w;
				int x=nPos-(y*w);

				ftype fZ=pFrame->m_pBestDepth[nPos];
				ftype fSum=0;
				int nArea=0;
				bool bPeak=false;
				for (int k3=1;k3<9;k3++)
				{
					int x1=x+dirs[k3][0];
					int y1=y+dirs[k3][1];
					if (x1<0 || y1<0 || x1>=w || y1>=h)
						continue;

					if (!pFrame->m_pState[y1*w+x1])
						continue;

					if ((fZ-pFrame->m_pBestDepth[y1*w+x1])>fMaxDiff)
						bPeak=true;					
					fSum+=pFrame->m_pBestDepth[y1*w+x1];
					nArea++;
				} //k3
				if (bPeak)
				{	
					fSum/=(ftype)(nArea);
					pFrame->m_pBestDepth[nPos]=fSum;
				}
			} //i2
		} //i

		tLoadingBar.Tick(100.0/5.0);
		
		fMaxDiff=(m_fMaxZ-m_fMinZ)/500.0;
		//////////////////////////////////////////////////////////////////////////
		// Step2: smooth positions based on segments area
		for (int j=0;j<4;j++)
		for (int k=0;k<w*h;k++)
		{		
			if (!pState[k])
				continue;

			if (pEdges[k])
				continue;

			ftype fZ1=pFrame->m_pBestDepth[k];
			CSegment *pSeg=pImageProc->m_lstSegments[nRegions[k]];			
			ftype fArea1=pSeg->m_nNumPixels;			

			int y=k/w;
			int x=k-(y*w);

			for (int k3=0;k3<9;k3++)
			{
				int x1=x+dirs[k3][0];
				int y1=y+dirs[k3][1];
				if (x1<0 || y1<0 || x1>=w || y1>=h)
					continue;					
				if (!pState[y1*w+x1])
					continue;

				ftype fZ2=pFrame->m_pBestDepth[y1*w+x1];				

				if (fabs(fZ1-fZ2)>fMaxDiff)
				{
					CSegment *pSeg2=pImageProc->m_lstSegments[nRegions[y1*w+x1]];
					ftype fArea2=pSeg2->m_nNumPixels;
					ftype fSum=fArea1+fArea2;
					ftype fZ=fZ1*(fArea1/fSum)+fZ2*(fArea2/fSum);

					pFrame->m_pBestDepth[k]=fZ;
					pFrame->m_pBestDepth[y1*w+x1]=fZ;
				}
			} //k3
		} //k		

		tLoadingBar.Tick(100.0/5.0);

		//////////////////////////////////////////////////////////////////////////
		// step 2:fit the planes using segments	
		
		Vector3d *akPoint=new Vector3d [w*h];

		for (segmlistit i=pImageProc->m_lstSegments.begin();i!=pImageProc->m_lstSegments.end();i++)
		{
			CSegment *pSeg=(*i);
			int nPoints=0;
			// fit plane
			for (int k=0;k<pSeg->m_nNumPixels;k++)
			{
				int nPos=pSeg->m_nPos[k];

				if (!pFrame->m_pState[nPos])
					continue;

				int y=nPos/w;
				int x=nPos-(y*w);

				ftype fZ=pFrame->m_pBestDepth[nPos];

				akPoint[nPoints]=Vector3d((double)(x),(double)(y),fZ);
				nPoints++;
			} //k

			//if (nPoints<3)
			if (nPoints<32)
			{
				pSeg->Clear(pFrame);
				continue;
			}
			
			double fA,fB,fC;
			if (!HeightPlaneFit(nPoints,akPoint,fA,fB,fC))
			{
				pSeg->Clear(pFrame);
				continue;
			}			
			pSeg->m_vPlane.Set(fA,fB,fC);			

			if (!ParaboloidFit(nPoints,akPoint,pSeg->m_vCoeffs))
			{
				pSeg->Clear(pFrame);
				continue;
			}			

			pSeg->m_bFit=true;
		} //i
		
		delete [] akPoint;
							
		//////////////////////////////////////////////////////////////////////////
		// step 3:fit all the segments				
		for (segmlistit i=pImageProc->m_lstSegments.begin();i!=pImageProc->m_lstSegments.end();i++)
		{
			CSegment *pSeg=(*i);
			if (!pSeg->m_bFit)
				continue;

			// calc error from paraboloid and plane and choose
			// the best one			
			ftype fPlaneError=0;
			ftype fParabError=0;
			for (int k=0;k<pSeg->m_nNumPixels;k++)
			{
				int nPos=pSeg->m_nPos[k];
				if (!pFrame->m_pState[nPos])
					continue;
				int y=nPos/w;
				int x=nPos-(y*w);

				ftype fZ=pFrame->m_pBestDepth[nPos];

				ftype fZ1=pSeg->m_vPlane.x*(ftype)(x)+pSeg->m_vPlane.y*(ftype)(y)+pSeg->m_vPlane.z;
				fPlaneError+=sqr(fZ-fZ1);

				ftype fX=x,fY=y;
				fZ1=pSeg->m_vCoeffs[0]*sqr(fX)+pSeg->m_vCoeffs[1]*fX*fY+
					pSeg->m_vCoeffs[2]*sqr(fY)+pSeg->m_vCoeffs[3]*fX+
					pSeg->m_vCoeffs[4]*fY+pSeg->m_vCoeffs[5];
				fParabError+=sqr(fZ-fZ1);
			} //k

			bool bUsePlane=false;
			if (fPlaneError<fParabError)
				bUsePlane=true;

			for (int k=0;k<pSeg->m_nNumPixels;k++)
			{
				int nPos=pSeg->m_nPos[k];

				int y=nPos/w;
				int x=nPos-(y*w);
							
				ftype fZ;
				if (bUsePlane)
					fZ=pSeg->m_vPlane.x*(ftype)(x)+pSeg->m_vPlane.y*(ftype)(y)+pSeg->m_vPlane.z;
				else
				{				
					//z = c0*x^2+c1*x*y+c2*y^2+c3*x+c4*y+c5
					ftype fX=x,fY=y;
					fZ=pSeg->m_vCoeffs[0]*sqr(fX)+pSeg->m_vCoeffs[1]*fX*fY+
						pSeg->m_vCoeffs[2]*sqr(fY)+pSeg->m_vCoeffs[3]*fX+
						pSeg->m_vCoeffs[4]*fY+pSeg->m_vCoeffs[5];
				}

				if ((fZ*1.25)<(m_fMinZ))
					fZ=m_fMinZ;
				else
				if ((fZ)>(m_fMaxZ*1.25))
					fZ=m_fMaxZ;

				pDepth[nPos]=fZ;				
				pFrame->m_pState[nPos]=1;
			} //k
		}	//i

		tLoadingBar.Tick(100.0/5.0);

		//////////////////////////////////////////////////////////////////////////
		// remove errors created by small segments
		fMaxDiff=(m_fMaxZ-m_fMinZ)/100.0;
		for (int j=0;j<2;j++)
		for (int k=0;k<w*h;k++)
		{		
			if (!pState[k])
				continue;

			if (pEdges[k])
				continue;

			ftype fZ1=pFrame->m_pBestDepth[k];
			CSegment *pSeg=pImageProc->m_lstSegments[nRegions[k]];			
			ftype fArea1=pSeg->m_nNumPixels;			

			int y=k/w;
			int x=k-(y*w);

			for (int k3=0;k3<9;k3++)
			{
				int x1=x+dirs[k3][0];
				int y1=y+dirs[k3][1];
				if (x1<0 || y1<0 || x1>=w || y1>=h)
					continue;					
				if (!pState[y1*w+x1])
					continue;

				ftype fZ2=pFrame->m_pBestDepth[y1*w+x1];				

				if (fabs(fZ1-fZ2)>fMaxDiff)
				{
					CSegment *pSeg2=pImageProc->m_lstSegments[nRegions[y1*w+x1]];
					ftype fArea2=pSeg2->m_nNumPixels;
					ftype fSum=fArea1+fArea2;
					ftype fZ=fZ1*(fArea1/fSum)+fZ2*(fArea2/fSum);

					pFrame->m_pBestDepth[k]=fZ;
					pFrame->m_pBestDepth[y1*w+x1]=fZ;
				}
			} //k3
		} //k		

		//////////////////////////////////////////////////////////////////////////
		// create 3d points
		for (int k=0;k<w*h;k++)
		{		
			if (!pFrame->m_pState[k])
				continue;
			ftype fZ=pFrame->m_pBestDepth[k];
			
			int y=k/w;
			int x=k-(y*w);						
			vector3f vPos(x,y,fZ);

			m_pVideo->UnProjectFromScreen(vPos,pFrame->m_pPoints[k],pCam);
		} //k	

		//////////////////////////////////////////////////////////////////////////
		// fix segments connections
		for (int j=0;j<2;j++)
		for (int k=0;k<w*h;k++)
		{		
			if (!pState[k])
				continue;

			if (pEdges[k])
				continue;

			int y=k/w;
			int x=k-(y*w);

			if ((x>=w-1) || (y>=h-1))
				continue;

			CSegment *pSeg1=pImageProc->m_lstSegments[nRegions[k]];			
			CSegment *pSeg2=pImageProc->m_lstSegments[nRegions[k+1]];			
			if (pSeg1==pSeg2)
			{
				// search y dir then
				pSeg2=pImageProc->m_lstSegments[nRegions[k+w]];
				if (pSeg1==pSeg2)
					continue;
			}
			
			ftype fArea1=pSeg1->m_nNumPixels;			

			for (int k3=1;k3<9;k3++)
			{
				int x1=x+dirs[k3][0];
				int y1=y+dirs[k3][1];
				if (x1<0 || y1<0 || x1>=w || y1>=h)
					continue;					
				if (!pState[y1*w+x1])
					continue;

				vector3f v1=pFrame->m_pPoints[k];
				vector3f v2=pFrame->m_pPoints[y1*w+x1];				

				CSegment *pSeg2=pImageProc->m_lstSegments[nRegions[y1*w+x1]];
				ftype fArea2=pSeg2->m_nNumPixels;
				ftype fSum=fArea1+fArea2;
				vector3f vZ=(v1*(fArea1/fSum))+(v2*(fArea2/fSum));

				pFrame->m_pPoints[k]=vZ;
				pFrame->m_pPoints[y1*w+x1]=vZ;
			} //k3
		} //k		
		
		//continue;

		tLoadingBar.Tick(100.0/5.0);
		
		//////////////////////////////////////////////////////////////////////////
		// step3: fit a b-spline over the sharp/noisy plane fitted mesh and fill holes	
		// calc spline		
		int nSplineScale=8;
		//int nSplineScale=16;
		//if (m_bHumans)
		//	nSplineScale=16;
		//int nPoints=w/nSplineScale*h/nSplineScale;
		Vector3d **aakCtrlPoint=new Vector3d*[w/nSplineScale+1];
		for (int k=0;k<w/nSplineScale+1;k++)
			aakCtrlPoint[k]=new Vector3d [h/nSplineScale+1];
		pCam=&m_lstFrames[KEY_FRAME]->m_Camera;	

		int nV=0;
		for (int y=0;y<h;y+=nSplineScale,nV++)
		{
			int nU=0;
			for (int x=0;x<w;x+=nSplineScale,nU++)
			{			
				vector3f vPos=vPoints[y*w+x];
				//if (vPos.x==0 && vPos.y==0)
				if (!pState[y*w+x])
				{
					// search for the closest valid point, project z
					bool bFound=false;
					int nRadius=1;
					vector3f vRes;
					while (!bFound)
					{
						for (int y1=y-nRadius;y1<=y+nRadius;y1++)
						{	
							if (y1<0 || y1>=h)
								continue;	
							// check only the borders every time radius 
							// increases
							int nStep;
							if ((y1==(y-nRadius)) || (y1==(y+nRadius)))
								nStep=1;
							else
								nStep=nRadius*2;
							for (int x1=x-nRadius;x1<=x+nRadius;x1+=nStep)
							{	
								if (x1<0 || x1>=w)
									continue;							
								vRes=vPoints[y1*w+x1];
								//if (vRes.x!=0 && vRes.y!=0)
								if (pState[y1*w+x1])
								{
									bFound=true;
									break;
								}
							} //x1
							if (bFound)
								break;
						} //y1
						nRadius++;
						if (nRadius>(max(w,h)/8))
							break;
					} // bFound

					if (!bFound)					
						m_pVideo->UnProjectFromScreen(vector3f(x,y,((m_fMinZ+m_fMaxZ)/2.0)),vRes,pCam);					

					vPos=vRes;
					//vPos.x=-5.0+(ftype)(x)/(ftype)(w)*10.0;
					//vPos.y=(-5.0+(ftype)((h-1)-y)/(ftype)(h)*10.0)*fYScale;
					m_pVideo->ProjectToScreen(vPos,vRes,pCam);
					// reproject from where it should be on the 2d photo
					m_pVideo->UnProjectFromScreen(vector3f(x,y,vRes.z),vPos,pCam);
				} // x==0 && y==0			
				ASSERT(nU<(w/nSplineScale+1));
				ASSERT(nV<(h/nSplineScale+1));
				aakCtrlPoint[nU][nV]=Vector3d(vPos.x,vPos.y,vPos.z);
			} //x
		} //y

		BSplineRectangled *tBSpline=new BSplineRectangled(w/nSplineScale+1,h/nSplineScale+1,aakCtrlPoint,4,4,false,false,true,true);
		for (int k=0;k<w/nSplineScale+1;k++)
			delete [] aakCtrlPoint[k];
		delete [] aakCtrlPoint;	

		//CtTriangle **pTris=m_lstFrames[KEY_FRAME]->m_pTriangles;

		int nTimes=1;
		//if (m_bHumans)
		//	nTimes++;
		for (int j=0;j<nTimes;j++)
		for (int y=0;y<h;y++)
		{		
			ftype fV=(ftype)(y)/(ftype)(h);
			for (int x=0;x<w;x++)
			{	
				//CSegment *pSeg=pImageProc->m_lstSegments[nRegions[y*w+x]];
				//if (!pSeg->m_bFit)
				//	continue;

				vector3f vPos1=vPoints[y*w+x];

				ftype fWeight1=0.65,fWeight2=0.35;

				//ftype fWeight1=0.75,fWeight2=0.15;

				//if (vPos1.x==0 && vPos1.y==0)
				if (!pState[y*w+x])
				{
					// if this is a valid position inside the mesh, fit on the spline
					//CtTriangle *pTri=pTris[y*w+x];
					//if (pTri)
						continue; // skip borders				
				}
				
				//ftype fArea1=pSeg->m_nNumPixels;			
				//if (fArea1<256.0)
	//				fWeight2=0;
		//		else
			//		fWeight2=1.0/(fArea1/256.0);			
				//fWeight1=1.0-fWeight2;						
				
				ftype fU=(ftype)(x)/(ftype)(w);
				Vector3d vSplinePos=tBSpline->GetPosition(fU,fV);			
				vector3f vPos2=vector3f(vSplinePos.X(),vSplinePos.Y(),vSplinePos.Z());

				vector3f vSum(0,0,0);
				if (fWeight1>0)
					vSum+=(vPos1*fWeight1);
				vSum+=(vPos2*fWeight2);

				vPoints[y*w+x]=vSum;
			} //x
		} //y

		delete tBSpline;	

		//////////////////////////////////////////////////////////////////////////	
		// fill holes
		for (segmlistit i=pImageProc->m_lstSegments.begin();i!=pImageProc->m_lstSegments.end();i++)
		{
			CSegment *pSeg=(*i);
			if (pSeg->m_bFit)
				continue;

			bool bFilling=true;
			while (bFilling)
			{			
				int nFilled=0;
				for (int k=0;k<pSeg->m_nNumPixels;k++)
				{
					int nPos=pSeg->m_nPos[k];

					if (pFrame->m_pState[nPos])
						continue;

					int y=nPos/w;
					int x=nPos-(y*w);

					vector3f vSum(0,0,0);
					int nArea=0;
					for (int k3=1;k3<9;k3++)
					{
						int x1=x+dirs[k3][0];
						int y1=y+dirs[k3][1];
						if (x1<0 || y1<0 || x1>=w || y1>=h)
							continue;					
						if (!pState[y1*w+x1])
							continue;

						vSum+=pFrame->m_pPoints[y1*w+x1];
						nArea++;
					}
					bFilling=true;
					if (nArea>0)
					{
						nFilled++;
						pFrame->m_pPoints[nPos]=(vSum/(ftype)(nArea));
						pFrame->m_pState[nPos]=1;						
					}
				} //k
				if (!nFilled)
					break;	// avoid infinite loops in empty segments
									// surrounded by empty segments
			} //bFilling
		}//i

		//continue;

		// fill depth map from 3d points
		double *pDepthMap=m_lstFrames[KEY_FRAME]->m_pBestDepth;
		vPoints=m_lstFrames[KEY_FRAME]->m_pPoints;
		for (int k=0;k<w*h;k++)
		{
			vector3f vSource=vPoints[k];
			//if (vSource.w<0)
			if (!pState[k])
			{
				pDepthMap[k]=-1;
				continue;
			}
			vector3f vPos;
			/*
			int y=k/w;
			int x=k-(y*w);
			vSource.x=x;
			vSource.y=y;
			vSource.z=pDepthMap[k];									
			m_pVideo->UnProjectFromScreen(vSource,vPos,&m_lstFrames[KEY_FRAME]->m_Camera);
			*/
			m_pVideo->ProjectToScreen(vSource,vPos,pCam);
			pDepth[k]=vPos.z;			
			//vPoints[k]=vPos;		
			vPoints[k].w=pDepth[k];
		} //k
		
		// denoise the depth
		double *pDest=new double [w*h];
		for (int k=0;k<2;k++)
		{		
			BilateralFiltering(pDepthMap,pDest,50.0/8192.0,3,1,w,h);		
			memcpy(pDepthMap,pDest,w*h*sizeof(double));
		}
		delete [] pDest;
		
		//////////////////////////////////////////////////////////////////////////	
		// Final step: add micro details and rectify the positions after being moved
		// by the smoothing process		
		uchar *pGray=m_lstFrames[KEY_FRAME]->m_pImage->m_pRawPixels;
		pCam=&m_lstFrames[KEY_FRAME]->m_Camera;
		ftype fDetailScale=35000.0;

		vector3f vPos;

		// convert from depth to 3d, add details
		for (int k=0;k<w*h;k++)
		{
			if (!pState[k])
			{
				vPoints[k].Clear();		
				vPoints[k].w=-1;
				pDepth[k]=-1;
				continue;
			}
			
			//vector3f vSource=vPoints[k];
			// scale the points, it has been reduced by the smoothing
			// process
			//vSource*=2.0f;
			//m_pVideo->ProjectToScreen(vSource,vPos,pCam);
			int y=k/w;
			int x=k-(y*w);
			vector3f vSource(x,y,pDepth[k]);
			vSource.z+=(((ftype)0.5-(pGray[k]/255.0))/fDetailScale); 

			//vSource.x=x;
			//vSource.y=y;
			//vSource.z=vPos.z;			
			m_pVideo->UnProjectFromScreen(vSource,vPos,pCam);
			pDepth[k]=vSource.z;			
			vPoints[k]=vPos;		
			vPoints[k].w=pDepth[k];
		}//k		

		/*
		uchar *pTemp=new uchar [w*h*3];
		if (pFrame->m_pBestDepth)
		{	
			char szFilename[512];
			sprintf(szFilename,"%s-BestDepth.tga",pFrame->m_pImage->m_szName);

			//ftype fScale=255.0/(fMaxz-fMinz);
			ftype fScale=8192.0;

			for (int k=0;k<w*h;k++)
			{
				int nRes=(int)((pFrame->m_pBestDepth[k]-m_fMinZ)*fScale);		
				if (nRes<0)
					nRes=0;
				if (nRes>255)
					nRes=255;

				for (int j=0;j<3;j++)
				{
					pTemp[k*3+j]=nRes;
				} //j
			}//k
			SaveTga(pTemp,3,w,h,szFilename,true);
		}
		delete [] pTemp;
		*/

		tLoadingBar.Tick(100.0/5.0);

	} //k1

	Save3DPoints(m_lstFrames[KEY_FRAME]);
		
}

//////////////////////////////////////////////////////////////////////////
void CSegment::Clear(CPhotoFrame *pFrame)
{
	int w=pFrame->m_pImage->m_nWidth;
	int h=pFrame->m_pImage->m_nHeight;

	for (int k=0;k<m_nNumPixels;k++)
	{
		int nPos=m_nPos[k];
		//int y=nPos/w;
		//int x=nPos-(y*w);
		if (pFrame->m_pPoints)
			pFrame->m_pPoints[nPos].Clear();
		else
			if (pFrame->m_pBestDepth)
			{
				pFrame->m_pBestDepth[nPos]=0;
				pFrame->m_pState[nPos]=0;
			}
	} //k
	m_bFit=false;
}

