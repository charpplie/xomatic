
#ifndef _MATH_DECL_H_
#define _MATH_DECL_H_

#pragma once

#include <math.h>

#define YAW		0  
#define PITCH	1    
#define ROLL	2   
 
#define M_PI	3.14159265358979323846
#define PI2		3.14159265358979323846*2.0
#define TO_RAD	(M_PI/180.0)	
#define	VEC_EPSILON	0.01

#define DEG2RAD( a ) ( ( (a) * M_PI ) / 180.0 )
#define RAD2DEG( a ) ( ( (a) * 180.0 ) / M_PI )

#define	ANGLE2SHORT(x)	((int)((x)*65536/360) & 65535)
#define	SHORT2ANGLE(x)	((x)*(360.0/65536))

#define PX 0
#define PY 1
#define PZ 2

#define SIDE_FRONT  0
#define SIDE_ON	    2
#define SIDE_BACK   1
#define SIDE_CROSS  -2

__forceinline float __fastcall Ffabs(float f) 
{
	*((unsigned int *) & f) &= ~0x80000000;

	return (f);
}

__forceinline int __fastcall iabs(int i) 
{
	if (i<0)
		return (-i);
	return (i);
}

inline int		Sign(ftype fVal)
{
	if (fVal>=0)
		return (0);
	return (1);
}

extern int dirs[9][2];
extern int dirswrap[9][2];
extern int dirs55[25][2];
extern int dirs77[49][2];
extern int dirs1515[15*15][2];
extern int posdirs[3][2];

extern unsigned char rndtable[256];
inline int	GetRandomNumber(int nVal)
{
	return (rndtable[nVal&255]);
}

int Findpow2Dim(int dim);
int Flog2(int value); 

void InitSinTable();
ftype Fsin(ftype x);
ftype Fcos(ftype x); 
ftype Snap(ftype value,int snap);

// debug utils
void  DtMatrixGetRotation(ftype *matrix, ftype *xRotation, ftype *yRotation, ftype *zRotation);
void	utlMtx2Euler(int ord, ftype m[3][3], ftype rot[3]);
//void	utlMtx2Euler(int ord, ftype *m, ftype *rot);

#endif //_MATH_DECL_H_
