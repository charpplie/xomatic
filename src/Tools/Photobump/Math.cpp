

//include headers
/////////////////////////////////////////////////////////////////
#include	"stdafx.h"

#define SIN_TABLE_LOGBASE2	12
#define SIN_TABLE_SIZE			(1L << SIN_TABLE_LOGBASE2)
#define SIN_TABLE_AND				((SIN_TABLE_SIZE)-1)			

/////////////////////////////////////////////////////////////////
ftype fSinTable[SIN_TABLE_SIZE]; 

int dirs[9][2]={{0,0},{0,-1},{1,-1},{1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1}};
int dirswrap[9][2]={{-1,-1},{0,-1},{1,-1},{1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1}};

int dirs55[25][2]={
{-2,-2},{-1,-2},{0,-2},{1,-2},{2,-2},
{-2,-1},{-1,-1},{0,-1},{1,-1},{2,-1},
{-2,0},{-1,0},{0,0},{1,0},{2,0},
{-2,1},{-1,1},{0,1},{1,1},{2,1},
{-2,2},{-1,2},{0,2},{1,2},{2,2}
};

int posdirs[3][2]={{1,-1},{1,0},{1,1}};

int dirs77[49][2];
int dirs1515[15*15][2];

//////////////////////////////////////////////////////////////////////////
/*
int g_nEdgeInvTable[64][64];
int g_nEdgeModTable[64][64];

//////////////////////////////////////////////////////////////////////////
void InitEdgeTables()
{
	memset(g_nEdgeInvTable,-1,64*64);
	memset(g_nEdgeModTable,-1,64*64);

	int nDest1,nDest2;
	for (int k=0;k<64;k++)
	{
		g_nEdgeInvTable[k][0]=k+2;
		g_nEdgeModTable[k][0]=1;
		nDest1=k+2;if (nDest1>64) nDest1=64;
		for (int j=0;j<nDest1;j++)
		{
			nDest2=j+1;if (nDest2>63) nDest2=63;
			g_nEdgeInvTable[k][nDest2]=j;
			g_nEdgeModTable[k][nDest2]=j+2;
		} //j
		g_nEdgeModTable[k][j]=0;
	} //k
}
*/

unsigned char rndtable[256] = {
    0,   8, 109, 220, 222, 241, 149, 107,  75, 248, 254, 140,  16,  66 ,
    74,  21, 211,  47,  80, 242, 154,  27, 205, 128, 161,  89,  77,  36 ,
    95, 110,  85,  48, 212, 140, 211, 249,  22,  79, 200,  50,  28, 188 ,
    52, 140, 202, 120,  68, 145,  62,  70, 184, 190,  91, 197, 152, 224 ,
    149, 104,  25, 178, 252, 182, 202, 182, 141, 197,   4,  81, 181, 242 ,
    145,  42,  39, 227, 156, 198, 225, 193, 219,  93, 122, 175, 249,   0 ,
    175, 143,  70, 239,  46, 246, 163,  53, 163, 109, 168, 135,   2, 235 ,
    25,  92,  20, 145, 138,  77,  69, 166,  78, 176, 173, 212, 166, 113 ,
    94, 161,  41,  50, 239,  49, 111, 164,  70,  60,   2,  37, 171,  75 ,
    136, 156,  11,  56,  42, 146, 138, 229,  73, 146,  77,  61,  98, 196 ,
    135, 106,  63, 197, 195,  86,  96, 203, 113, 101, 170, 247, 181, 113 ,
    80, 250, 108,   7, 255, 237, 129, 226,  79, 107, 112, 166, 103, 241 ,
    24, 223, 239, 120, 198,  58,  60,  82, 128,   3, 184,  66, 143, 224 ,
    145, 224,  81, 206, 163,  45,  63,  90, 168, 114,  59,  33, 159,  95 ,
    28, 139, 123,  98, 125, 196,  15,  70, 194, 253,  54,  14, 109, 226 ,
    71,  17, 161,  93, 186,  87, 244, 138,  20,  52, 123, 251,  26,  36 ,
    17,  46,  52, 231, 232,  76,  31, 221,  84,  37, 216, 165, 212, 106 ,
    197, 242,  98,  43,  39, 175, 254, 145, 190,  84, 118, 222, 187, 136 ,
    120, 163, 236, 249
};

/////////////////////////////////////////////////////////////////
void InitSinTable()
{
	for (int k=0;k<SIN_TABLE_SIZE;k++)
	{
		fSinTable[k]=(ftype)sin(((ftype)k*M_PI/((ftype)(SIN_TABLE_SIZE)*2.0)));			
	} //k
	//InitEdgeTables();

	int nCount=0;
	for (int y=-3;y<=3;y++)
	{
		for (int x=-3;x<=3;x++)
		{
			dirs77[nCount][0]=x;
			dirs77[nCount][1]=y;
			nCount++;
		}
	}

	nCount=0;
	for (int y=-2;y<=2;y++)
	{
		for (int x=-2;x<=2;x++)
		{
			dirs55[nCount][0]=x;
			dirs55[nCount][1]=y;
			nCount++;
		}
	}

	nCount=0;
	for (int y=-15/2;y<=15/2;y++)
	{
		for (int x=-15/2;x<=15/2;x++)
		{
			dirs1515[nCount][0]=x;
			dirs1515[nCount][1]=y;
			nCount++;
		}
	}

}

/////////////////////////////////////////////////////////////////
ftype Fsin(ftype x) 
{

	int index=(int)((ftype)(SIN_TABLE_SIZE) * x / (M_PI * 0.5f));
	int quad=(index>>SIN_TABLE_LOGBASE2) & 3;
	index&=SIN_TABLE_AND;

	switch (quad) 
	{
		case 0:return (fSinTable[index]);
		case 1:return (fSinTable[SIN_TABLE_AND-index]);
		case 2:return (-fSinTable[index]);
		case 3:return (-fSinTable[SIN_TABLE_AND-index]);
	}
	return 0;
}

/////////////////////////////////////////////////////////////////
ftype Fcos(ftype x) 
{
	int index=(int)((ftype)(SIN_TABLE_SIZE) * x / (M_PI * 0.5f));
	int quad=(index>>SIN_TABLE_LOGBASE2) & 3;
	index&=SIN_TABLE_AND;

	switch (quad) 
	{
		case 3:return (fSinTable[index]);
		case 0:return (fSinTable[SIN_TABLE_AND-index]);
		case 1:return (-fSinTable[index]);
		case 2:return (-fSinTable[SIN_TABLE_AND-index]);
	}
	return 0;
}

/////////////////////////////////////////////////////////////////
int Findpow2Dim(int dim)
{
	if (dim<1) 
		return (0);

	int i=1;

	while (i<dim) 
		i<<=1;
	return (i);
}

/////////////////////////////////////////////////////////////////
int Flog2(int value) 
{
	int res=0;	

	while ( ( value>>=1 ) != 0 ) 
		res++;	
	return res;
}

/////////////////////////////////////////////////////////////////
ftype Snap(ftype value,int snap)
{	
	int res=(int)(value/snap);
	return ((ftype)(res*snap));
}

////////////////////////////////////////////////////////////////		
//misc functions
//! check if a ray hit an AABB
bool	RayHitBBox(const vector3f &start,const vector3f &end,const vector3f &mins,const vector3f &maxs)
{
	vector3f vMid=(maxs+mins)*0.5f;	
	vector3f vExt=maxs-vMid;

  vector3f l=(end-start)*0.5f;
	vector3f m=(start-vMid)+l;
  vector3f a=vExt; //maxs;

  //vector3f l=(end-start)*0.5f;
  //vector3f m=start+l;
  //vector3f a=maxs;
  
  vector3f al(l.x<0?-l.x:l.x, l.y<0?-l.y:l.y, l.z<0?-l.z:l.z);
  
  if (m.x*m.x-sqr(a.x+al.x) > 1E-8)
    return (false);
  if (m.y*m.y-sqr(a.y+al.y) > 1E-8)
    return (false);
  if (m.z*m.z-sqr(a.z+al.z) > 1E-8)
    return (false);
  
  if (sqr(m.z*l.y-m.y*l.z)-sqr(a.y*al.z+a.z*al.y) > 1E-8)
    return (false);
  if (sqr(m.x*l.z-m.z*l.x)-sqr(a.x*al.z+a.z*al.x) > 1E-8)
    return (false);
  if (sqr(m.x*l.y-m.y*l.x)-sqr(a.x*al.y+a.y*al.x) > 1E-8)
    return (false);
  
  return (true);
}

////////////////////////////////////////////////////////////////		
//! check if a Ray hit a sphere
bool	RayHitSphere(const vector3f &start,const vector3f &end,const vector3f &center, ftype radius2)
{
  vector3f diff;
  vector3f AC, AB;
  vector3f point;

	//check if one of the two edpoints of the line is inside the sphere  
  diff.x = end.x-center.x;
  diff.y = end.y-center.y;
  diff.z = end.z-center.z;
  if (diff.x*diff.x+diff.y*diff.y+diff.z*diff.z <= radius2) 
    return (true);
  
  AC.x = center.x-start.x;
  AC.y = center.y-start.y;
  AC.z = center.z-start.z;
  if (AC.x*AC.x+AC.y*AC.y+AC.z*AC.z <= radius2) 
    return (true);

	//check distance from the sphere to the line
  AB.x = end.x-start.x;
  AB.y = end.y-start.y;
  AB.z = end.z-start.z;
  
  ftype r = (AC.x*AB.x+AC.y*AB.y+AC.z*AB.z) / (AB.x*AB.x+AB.y*AB.y+AB.z*AB.z);

	//projection falls outside the line
  if (r<0 || r>1) 
    return (false);

	//check if the distance from the line to the center of the sphere is less than radius
  point.x = start.x + r*AB.x;
  point.y = start.y + r*AB.y;
  point.z = start.z + r*AB.z;
  
  if ((point.x-center.x)*(point.x-center.x) + (point.y-center.y)*(point.y-center.y) + (point.z-center.z)*(point.z-center.z) > radius2)
    return (false);
  
  return (true);
}

//Given a normalized forward vector, create two
//other perpendicular vectors
/////////////////////////////////////////////////////////////////
void MakeNormalVectors(const vector3f &vForward,vector3f &vRight,vector3f &vUp)
{
	// this rotate and negate guarantees a vector
	// not colinear with the original
	vRight[1]= -vForward[0];
	vRight[2]= vForward[1];
	vRight[0]= vForward[2];

	ftype d = vRight*vForward;	
	vRight=vRight+((-d)*vForward); //VectorMA (right, -d, forward, right);
	vRight.Normalize();
	vUp=vRight^vForward;	
}

////////////////////////////////////////////////////////////////		
//! calc the area of a polygon giving a list of vertices and normal
ftype	CalcArea(const vector3f *vertices,int numvertices,const vector3f &normal)
{	
  vector3f csum(0,0,0);
  
  int n=numvertices;
  for (int i = 0, j = 1; i <= n-2; i++, j++)
  {
    csum.x += vertices[i].y*vertices[j].z-
				  vertices[i].z*vertices[j].y;
		csum.y += vertices[i].z*vertices[j].x-
				  vertices[i].x*vertices[j].z;
    csum.z += vertices[i].x*vertices[j].y-
				  vertices[i].y*vertices[j].x;
  }
  
  csum.x += vertices[n-1].y*vertices[0].z-
    vertices[n-1].z*vertices[0].y;
  csum.y += vertices[n-1].z*vertices[0].x-
    vertices[n-1].x*vertices[0].z;
  csum.z += vertices[n-1].x*vertices[0].y-
    vertices[n-1].y*vertices[0].x;
  
  ftype area=0.5f*(ftype)Ffabs(normal*csum);
  return (area);
}

//////////////////////////////////////////////////////////////////////////
bool SphereIntersectBBox(const vector3f &vSphereOrigin,ftype fRadius,const CAABBd &AABB)
{
	ftype fRadius2=fRadius*fRadius;
	
	ftype fDmin=0;	
  for (int k=0;k<3;k++) 
	{
    if (vSphereOrigin[k]<AABB.m_vMins[k]) 
			fDmin+=(vSphereOrigin[k]-AABB.m_vMins[k])*(vSphereOrigin[k]-AABB.m_vMins[k]); 
		else
    if (vSphereOrigin[k]>AABB.m_vMaxs[k]) 
			fDmin+=(vSphereOrigin[k]-AABB.m_vMaxs[k])*(vSphereOrigin[k]-AABB.m_vMaxs[k]); 
  } //k

  if (fDmin<=fRadius2) 
		return(true);

	return (false);
}

//////////////////////////////////////////////////////////////////////////
bool Overlap(const CAABBd &AABB1,const CAABBd &AABB2)
{

	vector3f vWorldMins1=AABB1.m_vMins+AABB1.m_vWorldSpacePos;
	vector3f vWorldMaxs1=AABB1.m_vMaxs+AABB1.m_vWorldSpacePos;

	vector3f vWorldMins2=AABB2.m_vMins+AABB2.m_vWorldSpacePos;
	vector3f vWorldMaxs2=AABB2.m_vMaxs+AABB2.m_vWorldSpacePos;

	if (vWorldMins1.x>vWorldMaxs2.x || vWorldMins1.y>vWorldMaxs2.y || vWorldMins1.z > vWorldMaxs2.z 
	 || vWorldMaxs1.x<vWorldMins2.x || vWorldMaxs1.y<vWorldMins2.y || vWorldMaxs1.z < vWorldMins2.z ) 
	{
		return (false);
	}	

	return (true);
}

//////////////////////////////////////////////////////////////////////////
vector3f	ClosestPointOnLine(const vector3f &a, const vector3f &b, const vector3f &p)
{
	vector3f c = p - a;
	vector3f v = b - a;
	v.Normalize();

	ftype	t = v * c;
	//ftype	d = b.Distance(a);
	ftype	d = Distance(a,b);
	return (a + (v * t));
}

//////////////////////////////////////////////////////////////////////////
bool	IsPointInsideEdge(const vector3f &a, const vector3f &b, const vector3f &p)
{
	vector3f v1=a-p;
	vector3f v2=b-p;
	return ((v1 * v2) <= 0);
}

//////////////////////////////////////////////////////////////////////////
void vector3f::VecFromAngles()
{	
	vector3f vDir(0,0,-1);	
	CMatrixf tm;
	tm.Identity();
	tm.RotateMatrix(*this);
	*this = tm.TransformPoint(vDir);	
}

//////////////////////////////////////////////////////////////////////////
void vector3f::VecFromCameraAngles()
{	
	vector3f vDir(0,0,-1);	
	CMatrixf tm;
	tm.Identity();
	vector3f vTemp;
	vTemp.x=x;
	vTemp.y=z;
	vTemp.z=-(y+180.0);	
	tm.RotateMatrix(vTemp);
	*this = tm.TransformPoint(vDir);	
}

//////////////////////////////////////////////////////////////////////////
void vector3f::VecFromCameraAngles2()
{	
	vector3f vDir(0,0,-1);	
	CMatrixf tm;
	tm.Identity();
	vector3f vTemp;
	vTemp.x=x;
	vTemp.y=z;
	vTemp.z=y+180.0;	
	tm.RotateMatrix(vTemp);
	*this = tm.TransformPoint(vDir);	
}

//////////////////////////////////////////////////////////////////////////
bool LineIntersect2D(const vector3f &vA,const vector3f &vB, const vector3f &vC,const vector3f &vD, vector3f &vRes)
{
	ftype fDen=(vB.x-vA.x)*(vD.y-vC.y)-(vB.y-vA.y)*(vD.x-vC.x);
	if (Ffabs(fDen)<0.00001f)
		return (false); // parallel lines
	ftype fNum=(vA.y-vC.y)*(vD.x-vC.x)-(vA.x-vC.x)*(vD.y-vC.y);
	if (Ffabs(fNum)<0.00001f)
		return (false); // collinear lines
	ftype r=fNum/fDen;

	if (r<0 || r>1)
		return (false); // intersection is on the line extent

	fNum=(vA.y-vC.y)*(vB.x-vA.x)-(vA.x-vC.x)*(vB.y-vA.y);
	ftype s=fNum/fDen;

	if (s<0 || s>1)
		return (false); // intersection is on the line extent

	vRes.Clear();
	vRes.x=vA.x+r*(vB.x-vA.x);
  vRes.y=vA.y+r*(vB.y-vA.y);

	return (true);
}
