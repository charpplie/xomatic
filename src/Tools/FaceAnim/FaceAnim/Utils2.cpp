
#include "stdafx.h"
#include "FaceAnim.h"

#include "FaceAnimDoc.h"

//////////////////////////////////////////////////////////////////////////
void CFaceAnimDoc::MyPerspective2(double fovx, double aspect, double zNear, double zFar,double *m)
{
	double xmin, xmax, ymin, ymax;
	// NOTE: This is not an identity! it should pass a matrix set to identity, and then
	// return results multiply by m. 
	//double m[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	xmax = zNear * tan(fovx * M_PI / 360.0);
	xmin = -xmax;

	ymin = (xmin / aspect);
	ymax = (xmax / aspect);

#define M(row,col)  m[col*4+row]

	// Set up the projection matrix
	M(0,0) = (2.0 * zNear) / (xmax - xmin);
	M(1,1) = (2.0 * zNear) / (ymax - ymin);
	M(2,2) = -(zFar + zNear) / (zFar - zNear);

	M(0,2) = (xmax + xmin) / (xmax - xmin);
	M(1,2) = (ymax + ymin) / (ymax - ymin);
	M(3,2) = -1.0;

	M(2,3) = -(2.0 * zFar * zNear) / (zFar - zNear);

#undef M	

	// Add to current matrix
	//glMultMatrixd(m);
}

//////////////////////////////////////////////////////////////////////////
void MyPerspective3(double fovx, double aspect, double zNear, double zFar,double *m)
{
	double xmin, xmax, ymin, ymax;
	// NOTE: This is not an identity! it should pass a matrix set to identity, and then
	// return results multiply by m. 
	//double m[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	xmax = zNear * tan(fovx * M_PI / 360.0);
	xmin = -xmax;

	ymin = (xmin / aspect);
	ymax = (xmax / aspect);

#define M(row,col)  m[col*4+row]

	// Set up the projection matrix
	M(0,0) = (2.0 * zNear) / (xmax - xmin);
	M(1,1) = (2.0 * zNear) / (ymax - ymin);
	M(2,2) = -(zFar + zNear) / (zFar - zNear);

	M(0,2) = (xmax + xmin) / (xmax - xmin);
	M(1,2) = (ymax + ymin) / (ymax - ymin);
	M(3,2) = -1.0;

	M(2,3) = -(2.0 * zFar * zNear) / (zFar - zNear);

#undef M	

	// Add to current matrix
	//glMultMatrixd(m);
	//mproj.MultMatrixf(m);
}

/*
* Transform a point (column vector) by a 4x4 matrix.  I.e.  out = m * in
* Input:  m - the 4x4 matrix
*         in - the 4x1 vector
* Output:  out - the resulting 4x1 vector.
*/
static void
transform_point(double out[4], const double m[16], const double in[4])
{
#define M(row,col)  m[col*4+row]
	out[0] =
		M(0, 0) * in[0] + M(0, 1) * in[1] + M(0, 2) * in[2] + M(0, 3) * in[3];
	out[1] =
		M(1, 0) * in[0] + M(1, 1) * in[1] + M(1, 2) * in[2] + M(1, 3) * in[3];
	out[2] =
		M(2, 0) * in[0] + M(2, 1) * in[1] + M(2, 2) * in[2] + M(2, 3) * in[3];
	out[3] =
		M(3, 0) * in[0] + M(3, 1) * in[1] + M(3, 2) * in[2] + M(3, 3) * in[3];
#undef M
}

/*
* Perform a 4x4 matrix multiplication  (product = a x b).
* Input:  a, b - matrices to multiply
* Output:  product - product of a and b
*/
static void
matmul(double * product, const double * a, const double * b)
{
	// This matmul was contributed by Thomas Malik 
	double temp[16];
	int i;

#define A(row,col)  a[(col<<2)+row]
#define B(row,col)  b[(col<<2)+row]
#define T(row,col)  temp[(col<<2)+row]

	// i-te Zeile 
	for (i = 0; i < 4; i++) {
		T(i, 0) =
			A(i, 0) * B(0, 0) + A(i, 1) * B(1, 0) + A(i, 2) * B(2, 0) + A(i,
			3) *
			B(3, 0);
		T(i, 1) =
			A(i, 0) * B(0, 1) + A(i, 1) * B(1, 1) + A(i, 2) * B(2, 1) + A(i,
			3) *
			B(3, 1);
		T(i, 2) =
			A(i, 0) * B(0, 2) + A(i, 1) * B(1, 2) + A(i, 2) * B(2, 2) + A(i,
			3) *
			B(3, 2);
		T(i, 3) =
			A(i, 0) * B(0, 3) + A(i, 1) * B(1, 3) + A(i, 2) * B(2, 3) + A(i,
			3) *
			B(3, 3);
	}

#undef A
#undef B
#undef T
	memcpy(product, temp, 16 * sizeof(double));
}

//////////////////////////////////////////////////////////////////////////
int MyProject(double objx, double objy, double objz,
											const double mview[16], int nViewport[4],											
											double * winx, double * winy, double * winz)
{
	// matrice de transformation 
	double in[4], out[4];
	//double	m[16];
	const double	*matptr;

	// initilise la matrice et le vecteur a transformer 
	in[0] = objx;
	in[1] = objy;
	in[2] = objz;
	in[3] = 1.0;
	//transform_point(out, model, in);
	//transform_point(in, proj, out);

	//transform_point(out, pCam->m_dGLViewMatrix, in);
	//transform_point(in, pCam->m_dGLProjMatrix, out);

	matptr=mview;

	transform_point(out, matptr, in);

	// d'ou le resultat normalise entre -1 et 1 
	if (out[3] == 0.0)
		return 0;

	out[0] /= out[3];
	out[1] /= out[3];
	out[2] /= out[3];

	// en coordonnees ecran 
	*winx = nViewport[0] + (1 + out[0]) * nViewport[2] / 2;
	*winy = nViewport[1] + (1 + out[1]) * nViewport[3] / 2;
	// entre 0 et 1 suivant z 
	*winz = (1 + out[2]) / 2;
	return 1;
}

/*
* Compute inverse of 4x4 transformation matrix.
* Code contributed by Jacques Leroy jle@star.be
* Return GL_TRUE for success, GL_FALSE for failure (singular matrix)
*/

boolean invert_matrix(const double * m, double * out)
{
	// NB. OpenGL Matrices are COLUMN major. 
#define SWAP_ROWS(a, b) { double *_tmp = a; (a)=(b); (b)=_tmp; }
#define MAT(m,r,c) (m)[(c)*4+(r)]

	double wtmp[4][8];
	double m0, m1, m2, m3, s;
	double *r0, *r1, *r2, *r3;

	r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];

	r0[0] = MAT(m, 0, 0), r0[1] = MAT(m, 0, 1),
		r0[2] = MAT(m, 0, 2), r0[3] = MAT(m, 0, 3),
		r0[4] = 1.0, r0[5] = r0[6] = r0[7] = 0.0,
		r1[0] = MAT(m, 1, 0), r1[1] = MAT(m, 1, 1),
		r1[2] = MAT(m, 1, 2), r1[3] = MAT(m, 1, 3),
		r1[5] = 1.0, r1[4] = r1[6] = r1[7] = 0.0,
		r2[0] = MAT(m, 2, 0), r2[1] = MAT(m, 2, 1),
		r2[2] = MAT(m, 2, 2), r2[3] = MAT(m, 2, 3),
		r2[6] = 1.0, r2[4] = r2[5] = r2[7] = 0.0,
		r3[0] = MAT(m, 3, 0), r3[1] = MAT(m, 3, 1),
		r3[2] = MAT(m, 3, 2), r3[3] = MAT(m, 3, 3),
		r3[7] = 1.0, r3[4] = r3[5] = r3[6] = 0.0;

	// choose pivot - or die 
	if (fabs(r3[0]) > fabs(r2[0]))
		SWAP_ROWS(r3, r2);
	if (fabs(r2[0]) > fabs(r1[0]))
		SWAP_ROWS(r2, r1);
	if (fabs(r1[0]) > fabs(r0[0]))
		SWAP_ROWS(r1, r0);
	if (0.0 == r0[0])
		return false;

	// eliminate first variable     
	m1 = r1[0] / r0[0];
	m2 = r2[0] / r0[0];
	m3 = r3[0] / r0[0];
	s = r0[1];
	r1[1] -= m1 * s;
	r2[1] -= m2 * s;
	r3[1] -= m3 * s;
	s = r0[2];
	r1[2] -= m1 * s;
	r2[2] -= m2 * s;
	r3[2] -= m3 * s;
	s = r0[3];
	r1[3] -= m1 * s;
	r2[3] -= m2 * s;
	r3[3] -= m3 * s;
	s = r0[4];
	if (s != 0.0) {
		r1[4] -= m1 * s;
		r2[4] -= m2 * s;
		r3[4] -= m3 * s;
	}
	s = r0[5];
	if (s != 0.0) {
		r1[5] -= m1 * s;
		r2[5] -= m2 * s;
		r3[5] -= m3 * s;
	}
	s = r0[6];
	if (s != 0.0) {
		r1[6] -= m1 * s;
		r2[6] -= m2 * s;
		r3[6] -= m3 * s;
	}
	s = r0[7];
	if (s != 0.0) {
		r1[7] -= m1 * s;
		r2[7] -= m2 * s;
		r3[7] -= m3 * s;
	}

	// choose pivot - or die 
	if (fabs(r3[1]) > fabs(r2[1]))
		SWAP_ROWS(r3, r2);
	if (fabs(r2[1]) > fabs(r1[1]))
		SWAP_ROWS(r2, r1);
	if (0.0 == r1[1])
		return false;

	// eliminate second variable 
	m2 = r2[1] / r1[1];
	m3 = r3[1] / r1[1];
	r2[2] -= m2 * r1[2];
	r3[2] -= m3 * r1[2];
	r2[3] -= m2 * r1[3];
	r3[3] -= m3 * r1[3];
	s = r1[4];
	if (0.0 != s) {
		r2[4] -= m2 * s;
		r3[4] -= m3 * s;
	}
	s = r1[5];
	if (0.0 != s) {
		r2[5] -= m2 * s;
		r3[5] -= m3 * s;
	}
	s = r1[6];
	if (0.0 != s) {
		r2[6] -= m2 * s;
		r3[6] -= m3 * s;
	}
	s = r1[7];
	if (0.0 != s) {
		r2[7] -= m2 * s;
		r3[7] -= m3 * s;
	}

	// choose pivot - or die 
	if (fabs(r3[2]) > fabs(r2[2]))
		SWAP_ROWS(r3, r2);
	if (0.0 == r2[2])
		return false;

	// eliminate third variable 
	m3 = r3[2] / r2[2];
	r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
		r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6], r3[7] -= m3 * r2[7];

	// last check 
	if (0.0 == r3[3])
		return false;

	s = 1.0 / r3[3];		// now back substitute row 3 
	r3[4] *= s;
	r3[5] *= s;
	r3[6] *= s;
	r3[7] *= s;

	m2 = r2[3];			// now back substitute row 2 
	s = 1.0 / r2[2];
	r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
		r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
	m1 = r1[3];
	r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
		r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
	m0 = r0[3];
	r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
		r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;

	m1 = r1[2];			// now back substitute row 1 
	s = 1.0 / r1[1];
	r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
		r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
	m0 = r0[2];
	r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
		r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;

	m0 = r0[1];			// now back substitute row 0 
	s = 1.0 / r0[0];
	r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
		r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);

	MAT(out, 0, 0) = r0[4];
	MAT(out, 0, 1) = r0[5], MAT(out, 0, 2) = r0[6];
	MAT(out, 0, 3) = r0[7], MAT(out, 1, 0) = r1[4];
	MAT(out, 1, 1) = r1[5], MAT(out, 1, 2) = r1[6];
	MAT(out, 1, 3) = r1[7], MAT(out, 2, 0) = r2[4];
	MAT(out, 2, 1) = r2[5], MAT(out, 2, 2) = r2[6];
	MAT(out, 2, 3) = r2[7], MAT(out, 3, 0) = r3[4];
	MAT(out, 3, 1) = r3[5], MAT(out, 3, 2) = r3[6];
	MAT(out, 3, 3) = r3[7];

	return true;

#undef MAT
#undef SWAP_ROWS
}


//////////////////////////////////////////////////////////////////////////
int MyUnproject(double winx, double winy, double winz,
												const double invmview[16], int nViewport[4],											
												double * objx, double * objy, double * objz)
{
	// matrice de transformation 		
	double in[4], out[4];
	double z = winz; //nearZ + winz * (farZ - nearZ);

	// transformation coordonnees normalisees entre -1 et 1 
	in[0] = (winx - nViewport[0]) * 2 / nViewport[2] - 1.0;
	in[1] = (winy - nViewport[1]) * 2 / nViewport[3] - 1.0;
	in[2] = 2.0 * z - 1.0;
	in[3] = 1.0f; //clipw;

	// calcul transformation inverse 
	const double *matptr=invmview;

	// d'ou les coordonnees objets 
	transform_point(out, matptr, in);
	if (out[3] == 0.0)
		return 0;
	*objx = out[0] / out[3];
	*objy = out[1] / out[3];
	*objz = out[2] / out[3];
	//*objw = out[3];
	return 1;
}


//////////////////////////////////////////////////////////////////////
void CFaceAnimDoc::ProjectToScreen(const vector3f &vSource,vector3f &vDest,tFeatureProcessing *tFrame)
{
	double dx,dy,dz;	
	
	MyProject(vSource.x,vSource.y,vSource.z,		
		&tFrame->pFace->mViewMatrix.m_Tvalues[0][0],tFrame->pFace->nViewport,
		&dx,&dy,&dz);

	vDest.x=(ftype)(dx);
	vDest.y=(ftype)((tFrame->pFace->nViewport[3])-dy);
	vDest.z=(ftype)(dz);
}

//////////////////////////////////////////////////////////////////////
void CFaceAnimDoc::UnProjectFromScreen(const vector3f &vSource,vector3f &vDest,tFeatureProcessing *tFrame)
{
	double dx,dy,dz;

	vector3f vTemp=vSource;
	vTemp.y=(tFrame->pFace->nViewport[3]-vSource.y);

	MyUnproject(vTemp.x,vTemp.y,vTemp.z,&tFrame->pFace->mInvertedViewMatrix.m_Tvalues[0][0],tFrame->pFace->nViewport,&dx,&dy,&dz);		

	vDest.x=(ftype)(dx);vDest.y=(ftype)(dy);vDest.z=(ftype)(dz);	
}

//////////////////////////////////////////////////////////////////////////
void CFaceAnimDoc::allocateOnDemand2( IplImage **img, CvSize size, int depth, int channels )
{
	if ( *img != NULL )	return;

	*img = cvCreateImage( size, depth, channels );
	if ( *img == NULL )	
		MyError("Error: Couldn't allocate image");			
}

//////////////////////////////////////////////////////////////////////////
bool CFaceAnimDoc::InsideRect(int x,int y,const CvRect &tRect,int &dist)
{
	int x1=tRect.x;
	int y1=tRect.y;

	int x2=tRect.x+tRect.width;
	int y2=tRect.y+tRect.height;

	if (x>x1 && x<x2 && y>y1 && y<y2)
		return (true);

	int distX=0,distY=0;
	if (x<x1 || x>x2)
	{	
		int distX1=abs(x-x1);
		int distX2=abs(x-x2);
		distX=min(distX1,distX2);
	}
	if (y<y1 || y>y2)
	{	
		int distY1=abs(y-y1);
		int distY2=abs(y-y2);
		distY=min(distY1,distY2);
	}
	dist=distX+distY;
	return (false);
}

//////////////////////////////////////////////////////////////////////////
void CFaceAnimDoc::CalcSimpleMoment(const CvRect &tRectFrom,const CvRect &tRectTo,float &fX,float &fY)
{
	int cx1=tRectFrom.x+tRectFrom.width/2;
	int cy1=tRectFrom.y+tRectFrom.height/2;
	int cx2=tRectTo.x+tRectTo.width/2;
	int cy2=tRectTo.y+tRectTo.height/2;
	fX=(float)(cx2-cx1);
	fY=(float)(cy2-cy1);
}

//////////////////////////////////////////////////////////////////////////
void CFaceAnimDoc::DrawRectangle(IplImage *pFrame,const CvRect &tRect,CvScalar color)
{
	cvDrawRect(pFrame,cvPoint(tRect.x,tRect.y),cvPoint(tRect.x+tRect.width,tRect.y+tRect.height),color);
}

//////////////////////////////////////////////////////////////////////////
void CFaceAnimDoc::DrawLine(IplImage *pFrame,const vector3f &v1,const vector3f &v2,CvScalar color)
{
	cvDrawLine(pFrame,cvPoint((int)(v1.x),(int)(v1.y)),cvPoint((int)(v2.x),(int)(v2.y)),color);
}

//////////////////////////////////////////////////////////////////////////
void CFaceAnimDoc::DrawLine3d(tFeatureProcessing *tFrame,const vector3f &v1,const vector3f &v2,CvScalar color)
{
	vector3f v11,v22;
	ProjectToScreen(v1,v11,tFrame);
	ProjectToScreen(v2,v22,tFrame);
	cvDrawLine(tFrame->pDrawingFrame,cvPoint((int)(v11.x),(int)(v11.y)),cvPoint((int)(v22.x),(int)(v22.y)),color);
}

//////////////////////////////////////////////////////////////////////////
void CFaceAnimDoc::DrawRectangle(IplImage *pFrame,float x1,float y1,float x2,float y2,CvScalar color)
{
	CvPoint p,q;
	p.x = (int) x1;
	p.y = (int) y1;
	q.x = (int) x2;
	q.y = (int) y2;
	CvRect	rect=cvRect(p.x,p.y,q.x-p.x,q.y-p.y);		

	DrawRectangle(pFrame,rect,color);	
}

//////////////////////////////////////////////////////////////////////////
void CFaceAnimDoc::DrawRectangle(IplImage *pFrame,const vector3f &v1,const vector3f &v2,CvScalar color)
{
	DrawRectangle(pFrame,(float)(v1.x),(float)(v1.y),(float)(v2.x),(float)(v2.y),color);
}

//////////////////////////////////////////////////////////////////////////
void CFaceAnimDoc::DrawRectangleCenter(IplImage *pFrame,const CvRect &tRect,CvScalar color)
{
	CvPoint p;
	p.x=tRect.x+tRect.width/2;
	p.y=tRect.y+tRect.height/2;
	cvRectangle(pFrame,cvPoint((int)(p.x)-1,(int)(p.y)-1),cvPoint((int)(p.x)+1,(int)(p.y)+1),color);	
}

//////////////////////////////////////////////////////////////////////////
//gets the filename without path
const char *CFaceAnimDoc::GetFilename(const char *szSrc)
{
	//find the filename
	int nLen=(int)strlen(szSrc);
	//go at the end of the filename 
	const char *szPtr=szSrc+nLen-1;

	//move back till the first '/' or '\\'
	while (*szPtr && szPtr>szSrc)
	{
		if ((*szPtr=='/') || (*szPtr=='\\'))
		{
			//if character found, increment the 
			//string pointer to point to the correct position 
			//(first character after the '/') and stop the loop
			szPtr++; 
			break;
		}

		szPtr--;
	}		

	return (szPtr);
}

//gets the path
/////////////////////////////////////////////////////////////////////////////
const char *CFaceAnimDoc::GetPath(const char *szFilename)
{	
	//extract the path		
	strcpy(m_szBuffer,szFilename);

	char *szSrc = m_szBuffer+strlen(m_szBuffer)-1;
	while (*szSrc && szSrc>m_szBuffer)
	{
		if (*szSrc=='\\' || *szSrc=='/')
		{ 
			szSrc++;
			*szSrc=0; //terminate the string here
			break;
		}
		szSrc--;
	}  

	return (m_szBuffer);
}

//fixs the path
/////////////////////////////////////////////////////////////////////////////
void CFaceAnimDoc::FixPath(char *szFilename)
{	
	char *szSrc = szFilename;
	while (*szSrc++)
	{
		if (*szSrc=='\\') 
			*szSrc='/';		
	}  	
}

//removes extension from filename
//////////////////////////////////////////////////////////////////////
void CFaceAnimDoc::RemoveExtension(char *szPath)
{
	char *szSrc=szPath+strlen(szPath)-1;
	while (*szSrc && szSrc>szPath && *szSrc!='/')
	{
		if (*szSrc=='.')
		{ 
			*szSrc=0; // remove extension 
			return;  
		}
		szSrc--;
	}
}

//replace filename extension
//////////////////////////////////////////////////////////////////////
void CFaceAnimDoc::ReplaceExtension(char *szPath, const char *szExt)
{
	RemoveExtension(szPath);
	strcat(szPath,".");
	strcat(szPath,szExt);
}

//////////////////////////////////////////////////////////////////////////
//TGA
//////////////////////////////////////////////////////////////////////////
#pragma pack (push)
#pragma pack (1)
typedef struct
{
	unsigned char  id_length, colormap_type, image_type;
	unsigned short colormap_index, colormap_length;
	unsigned char  colormap_size;
	unsigned short x_origin, y_origin, width, height;
	unsigned char  pixel_size, attributes;
} TargaHeader_t;
#pragma pack (pop)


//////////////////////////////////////////////////////////////////////////
bool CFaceAnimDoc::SaveTGA(unsigned char *sourcedata,int sourceformat,int w,int h,const char *filename,bool flipimage,bool flipcolors)
{
	if (flipimage)
	{
		int size=w*(sourceformat);
		unsigned char *tempw=new unsigned char [size];
		unsigned char *src1=sourcedata;		
		unsigned char *src2=sourcedata+(w*(sourceformat))*(h-1);
		for (int k=0;k<h/2;k++)
		{
			memcpy(tempw,src1,size);
			memcpy(src1,src2,size);
			memcpy(src2,tempw,size);
			src1+=size;
			src2-=size;
		}
		delete [] tempw;
	}

	TargaHeader_t header;

	memset(&header, 0, sizeof(header));
	header.image_type = 2;
	header.width = w;
	header.height = h;

	if (sourceformat==4)
		header.pixel_size = 32;
	else
		header.pixel_size = 24;

	unsigned char *data = new unsigned char[w*h*(header.pixel_size>>3)];
	unsigned char *dest = data;
	unsigned char *source = sourcedata;

	/*
	if (sourceformat==1)
	{
	memcpy(dest,source,w*h);
	header.pixel_size = 8;
	}
	else
	*/
	{			  	
		for (int a1 = 0; a1 < h; a1++)
		{
			for (int b1 = 0; b1 < w; b1++)
			{
				unsigned char r, g, b, a;

				if (sourceformat==1)
				{
					r=g=b=*source++;
					*dest = b; dest++;
					*dest = g; dest++;
					*dest = r; dest++;
				}
				else
				{				
					if (flipcolors)
					{
						b = *source; source++;
						g = *source; source++;
						r = *source; source++;
					}
					else
					{					
						r = *source; source++;
						g = *source; source++;
						b = *source; source++;
					}
					*dest = b; dest++;
					*dest = g; dest++;
					*dest = r; dest++;
					if (sourceformat==4) 
					{
						a = *source; source++;
						*dest = a; dest++;
					}				
				}				
			}
		}
	}	

	FILE *f = fopen(filename,"wb");
	if (!f)
	{		
		MyOutputDebugString("Cannot save %s\n",filename);
		delete [] data;
		return (false);
	}

	if (!fwrite(&header,1, sizeof(header),f))
	{		
		MyOutputDebugString("Cannot save %s\n",filename);
		delete [] data;
		fclose(f);
		return (false);
	}

	if (!fwrite(data,1, w*h*(header.pixel_size>>3),f))
	{		
		MyOutputDebugString("Cannot save %s\n",filename);		
		delete [] data;
		fclose(f);
		return (false);
	}

	fclose(f);
	delete [] data;

	return (true);
}
