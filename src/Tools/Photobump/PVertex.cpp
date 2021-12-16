
#include "StdAfx.h"
#include "pvertex.h"
#include "Video.h"
#include "PhotoFrame.h"
#include "Photoimage.h"

//////////////////////////////////////////////////////////////////////////
CPVertex::CPVertex(int nFrames)
{
	m_vPos.Clear();	
	m_v2DPos=new vector3f [nFrames];
	m_vEpiLines[0]=new vector3f [nFrames];
	m_vEpiLines[1]=new vector3f [nFrames];
	for (int j=0;j<nFrames;j++)
	{
		// invalid in this view (2d match not found)
		// unless otherwise stated (-1=not valid
		m_v2DPos[j].Set(-1,-1,-1); 

		m_vEpiLines[0][j].Clear();
		m_vEpiLines[1][j].Clear();
	}
	m_vColor.Clear();
	//m_descriptor=NULL;

	/*
	for (int j=0;j<3;j++)
	{
		m_X[j]=m_Y[j]=0;
	}	
	m_fWinZ=0;
	//m_pQuad=NULL;
	*/
}

//////////////////////////////////////////////////////////////////////////
CPVertex::~CPVertex(void)
{
	SAFE_DELETE_ARRAY(m_v2DPos);
	SAFE_DELETE_ARRAY(m_vEpiLines[0]);
	SAFE_DELETE_ARRAY(m_vEpiLines[1]);
	//SAFE_DELETE_ARRAY(m_descriptor);
}

//////////////////////////////////////////////////////////////////////////
int CPVertex::GetX(int nFrame)
{
	return (int)(m_v2DPos[nFrame].x+ROUNDING_VAL);
}

//////////////////////////////////////////////////////////////////////////
int CPVertex::GetY(int nFrame)
{
	return (int)(m_v2DPos[nFrame].y+ROUNDING_VAL);
}

//////////////////////////////////////////////////////////////////////////
ftype CPVertex::GetConfidence(int w,int h,CPhotoFrame *pFrame)
{
	ASSERT(0);
	/*
	int x=GetX();
	int y=GetY();
	if (x<0 || y<0 || x>=w || y>=h)
		return (-1);
	return (pFrame->m_pConfidence[y*w+x]);
	*/
	return (0);
}

//////////////////////////////////////////////////////////////////////////
void CPVertex::SetColor(int w,int h,CPhotoFrame *pFrame)
{
	int x=GetX(pFrame->m_nFrameNum);
	int y=GetY(pFrame->m_nFrameNum);
	if (x<0 || y<0 || x>=w || y>=h)
		return;
	for (int j=0;j<3;j++)
		m_vColor[j]=(ftype)(pFrame->m_pImage->m_pRawPixels[y*w*3+x*3+j])/255.0;
}

// calculates m_Plane,m_fArea3d and m_fArea
//////////////////////////////////////////////////////////////////////////
void CtTriangle::RefreshInternals(void)
{
	//m_Plane.CalcPlane(m_Verts[2],m_Verts[1],m_Verts[0]);					// triangle plane
	// area size in world space
	m_fArea3d=Calc3DArea(m_pVerts[0]->m_vPos,m_pVerts[1]->m_vPos,m_pVerts[2]->m_vPos);				
	for (int k=0;k<3;k++)
	{	
		ftype pfX[3],pfY[3];
		for (int j=0;j<3;j++)
		{
			pfX[j]=m_pVerts[j]->m_v2DPos[k].x;
			pfY[j]=m_pVerts[j]->m_v2DPos[k].y;
		}
		// area size in texture
		m_fArea[k]=Calc2DArea(pfX,pfY);																	
		if (m_fArea[k]<1.0)
			m_fArea[k]=1.0;
	}
	//if(m_fArea==0.0f)m_fArea=0.001f;															// to prevent crash
}

//calc the area of a triangle using texture coords
//////////////////////////////////////////////////////////////////////
ftype CtTriangle::Calc2DArea(const ftype *pfX,const ftype *pfY)
{
//m_fArea=(bx*cy - by*cx) + (ax*by - ay*bx) + (ay*cx - ax*cy) 

  ftype prod1=pfX[1]*pfY[2]-pfY[1]*pfX[2];
  ftype prod2=pfX[0]*pfY[1]-pfY[0]*pfX[1];
  ftype prod3=pfY[0]*pfX[2]-pfX[0]*pfY[2];

  return (prod1+prod2+prod3)/2.0f; 
}

//calc the area of a triangle in 3d
//////////////////////////////////////////////////////////////////////
ftype CtTriangle::Calc3DArea(const vector3f &p0,const vector3f &p1,const vector3f &p2)
{
	// ABC = (1/2)*||(B–A)×(C–A)||

	return(((p1-p0)^(p2-p0)).Length()*0.5);
}

//////////////////////////////////////////////////////////////////////
ftype CtTriangle::Calc3DArea()
{		
	return(Calc3DArea(m_pVerts[0]->m_vPos,m_pVerts[1]->m_vPos,m_pVerts[2]->m_vPos));
}

//////////////////////////////////////////////////////////////////////
ftype CtTriangle::Calc2DArea(int nFrame)
{
	ftype pfX[3],pfY[3];
	for (int j=0;j<3;j++)
	{
		pfX[j]=m_pVerts[j]->m_v2DPos[nFrame].x;
		pfY[j]=m_pVerts[j]->m_v2DPos[nFrame].y;
	}

	return (Calc2DArea(pfX,pfY));
}

//////////////////////////////////////////////////////////////////////////
vector3f CtTriangle::Calc2DBarycentrics(ftype inS,ftype inT,int nFrame,bool inbDoClipping)
{
	ftype ftemptriX[3],ftemptriY[3];  

  //calc the area of first sub triangle
  //memcpy(ftemptriX,m_fS,sizeof(ftype)*3);
  //memcpy(ftemptriY,m_fT,sizeof(ftype)*3);

	for (int j=0;j<3;j++)
	{
		ftemptriX[j]=m_pVerts[j]->m_v2DPos[nFrame].x;
		ftemptriY[j]=m_pVerts[j]->m_v2DPos[nFrame].y;
	}
  ftemptriX[0]=inS;
	ftemptriY[0]=inT;
	ftype a1=Calc2DArea(ftemptriX,ftemptriY);

  //calc the area of second sub triangle
  //memcpy(ftemptriX,m_fS,sizeof(ftype)*3);
  //memcpy(ftemptriY,m_fT,sizeof(ftype)*3);

	for (int j=0;j<3;j++)
	{
		ftemptriX[j]=m_pVerts[j]->m_v2DPos[nFrame].x;
		ftemptriY[j]=m_pVerts[j]->m_v2DPos[nFrame].y;
	}
  ftemptriX[1]=inS;
	ftemptriY[1]=inT;
  ftype a2=Calc2DArea(ftemptriX,ftemptriY);  

	//calc alpha beta and gamma components as area ratio
  ftype alpha=a1/m_fArea[nFrame];
  ftype beta=a2/m_fArea[nFrame];
	
	if (inbDoClipping)
	{
		ftype sub=alpha+beta-1.0;
		if(sub>0.0)
		{
			sub*=0.5;
			alpha-=sub;
			beta-=sub;
		}
		if(alpha<0.0)
			alpha=0.0;
		if(beta<0.0)
			beta=0.0;
	}

	vector3f outCoor;
	outCoor[0]=alpha;
	outCoor[1]=beta;
  outCoor[2]=1.0-alpha-beta;
	return (outCoor);
}

//////////////////////////////////////////////////////////////////////////
vector3f CtTriangle::Get3DCoordsFrom2DBarycentric(ftype inS,ftype inT,int nFrame)
{
	vector3f vBary=Calc2DBarycentrics(inS,inT,nFrame);
  //interpolate position and normal
  vector3f vRes=vBary[0]*m_pVerts[0]->m_vPos+vBary[1]*m_pVerts[1]->m_vPos+vBary[2]*m_pVerts[2]->m_vPos;

  //normal= fBar[0]*m_VertsNormal[0]	+ fBar[1]*m_VertsNormal[1]	+ fBar[2]*m_VertsNormal[2];
	return (vRes);
}

//////////////////////////////////////////////////////////////////////////
void CPVertex::GetNearbyVertices(lstPhotoVertices *m_lstVertices,CPVertex *pVerts[3])
{
	ASSERT(0);
	/*
	pVerts[0]=pVerts[1]=pVerts[2]=NULL;
	ftype	fBestDists[3]={0,0,0};

	for (int j=0;j<3;j++)
	{
		for (lstPhotoVerticesIt i=m_lstVertices->begin();i!=m_lstVertices->end();i++)
		{
			CPVertex *pVert=(*i);
			ftype fDist=Distance2D(*pVert);
			if (!pVerts[j] || fDist<fBestDists[j])
			{
				if (pVert!=pVerts[0] && pVert!=pVerts[1] && pVert!=pVerts[2])
				{				
					pVerts[j]=pVert;
					fBestDists[j]=fDist;				
					pVert->m_fVal=fDist;
				}
			}
		} //i	
	} //j
	*/
}

//////////////////////////////////////////////////////////////////////////
ftype CPVertex::GetClosestVertices(const vector3f &vPos,lstPhotoVertices *m_lstVertices,CPVertex *pVerts[3],int nVerts)
{
	ASSERT(0);
	/*
	pVerts[0]=pVerts[1]=pVerts[2]=NULL;
	ftype	fBestDists[3]={0,0,0},fSum=0;

	for (int j=0;j<nVerts;j++)
	{
		for (lstPhotoVerticesIt i=m_lstVertices->begin();i!=m_lstVertices->end();i++)
		{
			CPVertex *pVert=(*i);
			ftype fDist=pVert->Distance2D(vPos);
			if (!pVerts[j] || fDist<fBestDists[j])
			{
				if (pVert!=pVerts[0] && pVert!=pVerts[1] && pVert!=pVerts[2])
				{				
					pVerts[j]=pVert;
					fBestDists[j]=fDist;				
					pVert->m_fVal=fDist;
				}
			}
		} //i	
		fSum+=fBestDists[j];
	} //j	
	return (fSum);
	*/
	return (0);
}

/*
//////////////////////////////////////////////////////////////////////////
void CPVertex::Init(lstFrames *pListFrames,int xpos,int ypos,ftype WinZ,CVideo *pVideo)
{
	ASSERT(0);
	
	CPhotoFrame *pFrame=(*pListFrames)[0];

	int w=(*pListFrames)[0]->m_pImage->m_nWidth;
	int h=(*pListFrames)[0]->m_pImage->m_nHeight;

	m_X[0]=xpos;
	m_Y[0]=ypos;
	m_fWinZ=WinZ;
	vector3f vSource(m_X[0],m_Y[0],m_fWinZ);
	CCamera *pCam=&((*pListFrames)[0]->m_Camera);
	(*pListFrames)[0]->GetVideo()->UnProjectFromScreen(vSource,m_vPos,pCam);

	// get 2d point for other views
	for (int k=1;k<3;k++)
	{
		CCamera *pCam2=&((*pListFrames)[k]->m_Camera);
		vector3f vRes;
		(*pListFrames)[0]->GetVideo()->ProjectToScreen(m_vPos,vRes,pCam2);
		m_X[k]=(int)(vRes.x+0.5);
		m_Y[k]=(int)(vRes.y+0.5);
		//if (m_X[k]<0) m_X[k]=0;
		//if (m_X[k]>(w-1)) m_X[k]=w-1;
		//if (m_Y[k]<0) m_Y[k]=0;
		//if (m_Y[k]>(h-1)) m_Y[k]=h-1;		
	} // k	
}
*/
