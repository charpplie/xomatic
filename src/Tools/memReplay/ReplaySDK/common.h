#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
typedef unsigned long long	u64;

typedef char				s8;
typedef signed short		s16;
typedef signed int			s32;
typedef signed long long	s64;

typedef float				f32;
typedef double				f64;

#define TRUE	1
#define FALSE	0

#define MAX_FLOAT FLT_MAX

#define SQR(x)			((x)*(x))
#define MAX(a,b)		(((a)>(b))?(a):(b))
#define MIN(a,b)		(((a)<(b))?(a):(b))
#define CLAMP(a,b,c)	MIN(MAX(a,b),c)
#define ARRAY_SIZEOF(a) (sizeof(a)/sizeof((a)[0]))

typedef u32 TAddress;
typedef u32 TThreadId;

#endif //__COMMON_H__
