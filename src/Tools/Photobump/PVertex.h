
#pragma once

//////////////////////////////////////////////////////////////////////////
#define VERTEX_FLAG_EXPANDED	1

//////////////////////////////////////////////////////////////////////////
class CPhotoFrame;
class CPQuadtree;
class CPVertex;

//////////////////////////////////////////////////////////////////////////
class CtTriangle
{
public:

	CtTriangle()
	{
		m_pVerts[0]=m_pVerts[1]=m_pVerts[2]=NULL;
		m_fArea[0]=m_fArea[1]=m_fArea[2]=0;
		m_fArea3d=0;
	}

	void	RefreshInternals(void);

	vector3f Calc2DBarycentrics(ftype inS,ftype inT,int nFrame,bool inbDoClipping=false);
	vector3f Get3DCoordsFrom2DBarycentric(ftype inS,ftype inT,int nFrame);

	ftype	Calc2DArea(int nFrame);
	ftype	Calc3DArea();

	static ftype Calc2DArea(const ftype *pfX,const ftype *pfY);
	static ftype Calc3DArea(const vector3f &p0,const vector3f &p1,const vector3f &p2);

	CPVertex *m_pVerts[3];
	ftype			m_fArea[3],m_fArea3d;
};

//////////////////////////////////////////////////////////////////////////
class CPVertex
{
public:
	CPVertex(int nFrames=3);	
	~CPVertex(void);

	//ftype	Distance2D(const CPVertex &pVert) { return (m_v2DPos.Distance(pVert.m_v2DPos)); }
	//ftype	Distance2D(const vector3f &vPos) { return (m_v2DPos.Distance(vPos)); }

	//ftype	Distance2D(const CPVertex &pVert) { return (sqrt(sqr(m_v2DPos.x-pVert.m_v2DPos.x)+sqr(m_v2DPos.y-pVert.m_v2DPos.y))); }
	//ftype	Distance2D(const vector3f &vPos) { return (sqrt(sqr(m_v2DPos.x-vPos.x)+sqr(m_v2DPos.y-vPos.y))); }

	int			GetX(int nFrame);
	int			GetY(int nFrame);	
	ftype		GetConfidence(int w,int h,CPhotoFrame *pFrame);
	void		SetColor(int w,int h,CPhotoFrame *pFrame);
	void		GetNearbyVertices(lstPhotoVertices *m_lstVertices,CPVertex *pVerts[3]);
	static	ftype	GetClosestVertices(const vector3f &vPos,lstPhotoVertices *m_lstVertices,CPVertex *pVerts[3],int nVerts);

	vector3f	m_vPos;	
	vector3f	*m_v2DPos;
	// coeff a,b,c of line eq - corr epilines for two views
	vector3f	*m_vEpiLines[2]; 
	vector3f	m_vColor;
	//double		*m_descriptor;
	//ftype			m_fVal;

	//ftype			m_fWinZ;	
	//ftype			m_X[3],m_Y[3];	
	//int				m_dwFlags;
	//int				m_nDistance;
	//CPQuadtree	*m_pQuad;

};
