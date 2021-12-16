

#pragma once

boolean invert_matrix(const double * m, double * out);
void		MyPerspective3(double fovx, double aspect, double zNear, double zFar,double *m);
int MyUnproject(double winx, double winy, double winz,
								const double invmview[16], int nViewport[4],						
								double * objx, double * objy, double * objz);
int MyProject(double objx, double objy, double objz,
							const double mview[16], int nViewport[4],											
							double * winx, double * winy, double * winz);

//////////////////////////////////////////////////////////////////////////
inline float sqr(float fVal) { return(fVal*fVal);}

//////////////////////////////////////////////////////////////////////////
inline float __fastcall Ffabs(float f) 
{
	*((unsigned *) & f) &= ~0x80000000;
	return (f);
}

//////////////////////////////////////////////////////////////////////////
inline int iabs(int nVal) 
{ 
	if (nVal<0)
		return (-nVal);
	return(nVal);
}

//////////////////////////////////////////////////////////////////////////
inline float Clamp(float fVal,float fMin,float fMax)
{
	if (fVal<fMin)
		return (fMin);
	if (fVal>fMax)
		return (fMax);
	return (fVal);
}

//////////////////////////////////////////////////////////////////////////
inline double Clamp(double fVal,double fMin,double fMax)
{
	if (fVal<fMin)
		return (fMin);
	if (fVal>fMax)
		return (fMax);
	return (fVal);
}

//////////////////////////////////////////////////////////////////////////
inline bool InsideBBox2D(ftype x1,ftype y1,ftype x2,ftype y2,ftype cx,ftype cy)
{
	if (cx>x1 && cx<x2 && cy>y1 && cy<y2)
		return (true);

	return (false);
}
