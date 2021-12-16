
#include "stdafx.h"

/*
//   Calculate the line segment PaPb that is the shortest route between
//   two lines P1P2 and P3P4. Calculate also the values of mua and mub where
//      Pa = P1 + mua (P2 - P1)
//      Pb = P3 + mub (P4 - P3)
//   Return FALSE if no solution exists.
#define EPS (1.0E-7F)
bool LineLineIntersect(
   const vector3f &p1,const vector3f &p2,const vector3f &p3,const vector3f &p4,
	 vector3f *pa,vector3f *pb,double *mua, double *mub,bool bAbsoluteValues)
{
   vector3f p13,p43,p21;
   double d1343,d4321,d1321,d4343,d2121;
   double numer,denom;

   p13.x = p1.x - p3.x;
   p13.y = p1.y - p3.y;
   p13.z = p1.z - p3.z;
   p43.x = p4.x - p3.x;
   p43.y = p4.y - p3.y;
   p43.z = p4.z - p3.z;
   if (fabs(p43.x)  < EPS && fabs(p43.y)  < EPS && fabs(p43.z)  < EPS)
      return(FALSE);
   p21.x = p2.x - p1.x;
   p21.y = p2.y - p1.y;
   p21.z = p2.z - p1.z;
   if (fabs(p21.x)  < EPS && fabs(p21.y)  < EPS && fabs(p21.z)  < EPS)
      return(FALSE);

   d1343 = p13.x * p43.x + p13.y * p43.y + p13.z * p43.z;
   d4321 = p43.x * p21.x + p43.y * p21.y + p43.z * p21.z;
   d1321 = p13.x * p21.x + p13.y * p21.y + p13.z * p21.z;
   d4343 = p43.x * p43.x + p43.y * p43.y + p43.z * p43.z;
   d2121 = p21.x * p21.x + p21.y * p21.y + p21.z * p21.z;

   denom = d2121 * d4343 - d4321 * d4321;
   if (fabs(denom) < EPS)
      return(FALSE);
   numer = d1343 * d4321 - d1321 * d4343;

   *mua = numer / denom;
   *mub = (d1343 + d4321 * (*mua)) / d4343;

	 if (bAbsoluteValues)
	 {	
		if (*mua<0)
			*mua=-*mua;

		if (*mub<0)
			*mub=-*mub;
	 }

   pa->x =(ftype)(p1.x + *mua * p21.x);
   pa->y =(ftype)( p1.y + *mua * p21.y);
   pa->z =(ftype)( p1.z + *mua * p21.z);
   pb->x =(ftype)( p3.x + *mub * p43.x);
   pb->y =(ftype)( p3.y + *mub * p43.y);
   pb->z =(ftype)( p3.z + *mub * p43.z);

   return(TRUE);
}

int		IntersectLines(const vector3f &vA1,const vector3f &vA2,
										 const vector3f &vB1,const vector3f &vB2,
										 vector3f	&vRes1,vector3f	&vRes2,bool bAbsoluteValues)
{
	
	double m1,m2;
	bool bRes=LineLineIntersect(vA1,vA2,vB1,vB2,&vRes1,&vRes2,&m1,&m2,bAbsoluteValues);
	if (!bRes)
		return (-1);	
	return (2);	
}
*/

// demonstrate c function line_line_closest_points3d 
//#define EPS (1.0E-7F)
#define EPS 0.00000001
#define MINLEN (EPS)
#define MAXRECIPLEN (1.0 / MINLEN)

// non-homogeneous 3D
typedef struct pointstruct {
    ftype px;
    ftype py;
    ftype pz;
    } xPOINT;

// plane ax + by + cz + d == 0
typedef struct planestruct {
    ftype a;
    ftype b;
    ftype c;
    ftype d;
    } PLANE;

// 
typedef struct vecstruct {
    ftype dx;
    ftype dy;
    ftype dz;
    } VECTOR;


ftype 
edist (
    xPOINT *a,
    xPOINT *b)
{
ftype rval;

    rval = sqrt ((b->px - a->px) * (b->px - a->px) 
				  + (b->py - a->py) * (b->py - a->py) 
				  + (b->pz - a->pz) * (b->pz - a->pz));
    return (rval);
}

//  a = p + v  
xPOINT *
pplusv (
    xPOINT *a,
    xPOINT *p,
    VECTOR *v)
{
    a->px = p->px + v->dx;
    a->py = p->py + v->dy;
    a->pz = p->pz + v->dz;
    return (a);
}

//  c = a x b   
// CAREFUL: C cannot == a or b 
VECTOR *
vcross (
    VECTOR *c,
    VECTOR *a,
    VECTOR *b)
{
    c->dx = a->dy * b->dz - a->dz * b->dy;
    c->dy = a->dz * b->dx - a->dx * b->dz;
    c->dz = a->dx * b->dy - a->dy * b->dx;
    return (c);
}

ftype
vdot(
    VECTOR *a,
    VECTOR *b)
{
    return (a->dx * b->dx + a->dy * b->dy + a->dz * b->dz);
}

ftype
vmag(
    VECTOR *a)
{
    return (sqrt(vdot(a, a)));
}

ftype
recip_vmag(
    VECTOR *a)
{
    return (1.0 / sqrt(vdot(a, a)));
}

//  a /= ||a||  
VECTOR *
vnorm  (
    VECTOR *a)
{
ftype d;

    if ((d = recip_vmag(a)) > MAXRECIPLEN)     
		{
        //fprintf (stderr, "\nvector at 0x%x: %g %g %g?", 
				//	 (int)a, a->dx, a->dy, a->dz);
				Break();
        a->dx = a->dy = a->dz = 0.0;
        return (NULL);
        }
    else    {
        a->dx *= d;
        a->dy *= d;
        a->dz *= d;
        return (a);
        }
}

ftype 
point_plane_dist (
    xPOINT *a,
    PLANE *P)
{
ftype rval;

    rval = (a->px * P->a + a->py * P->b + a->pz * P->c + P->d);
    return (rval);
}

// p = (1-t)a + tb 
xPOINT *
plerp (
    xPOINT *p,
    xPOINT *a, 
    xPOINT *b,
    ftype t)
{
    p->px = t * b->px + (1-t) * a->px;
    p->py = t * b->py + (1-t) * a->py;
    p->pz = t * b->pz + (1-t) * a->pz;
    return (p);
}

xPOINT *
intersect_line_plane (
    xPOINT *p,               // returned as intersection point 
    xPOINT *a, xPOINT *b,     // the line to intersect 
    PLANE *M)               // the plane to intersect 
{
ftype Mdota, Mdotb, denom, t;
//int ip,iq;

    Mdota = point_plane_dist (a, M);
    Mdotb = point_plane_dist (b, M);

    denom = Mdota - Mdotb;
    if (fabs(denom / (fabs(Mdota) + fabs(Mdotb))) < EPS) 
		{
        //fprintf (stderr, "int_line_plane(): no intersection?\n");
				Break();
        p->px = p->py = p->pz = 0.0;
        return (NULL);
        }
    else    {
        t = Mdota / denom;
        plerp (p, a, b, t);
        return (p);
        }
}

// <point,direction> line form
xPOINT *
intersect_dline_plane (
    xPOINT *p,               // returned as intersection point
    xPOINT *a, VECTOR *adir, // the line to intersect
    PLANE *M)               // the plane in question
{
static xPOINT B, *b = &B;

    pplusv (b, a, adir);
    return (intersect_line_plane (p, a, b, M));
}

PLANE *
plane_from_two_vectors_and_point (
    PLANE *M,
    VECTOR *u, VECTOR *v, xPOINT *p)
{
    vnorm (vcross ((VECTOR *)M, u, v));

    // plane contains p 
    M->d = -(M->a * p->px + M->b * p->py + M->c * p->pz);
    return (M);
}

// computes pB ON line B closest TO line A
// computes pA ON line A closest TO line B
// return: 0 if parallel; 1 if coincident; 2 if generic
int
line_line_closest_points3d (
    xPOINT *pA, xPOINT *pB,
    xPOINT *a, VECTOR *adir,
    xPOINT *b, VECTOR *bdir)
{
static VECTOR Cdir, *cdir = &Cdir;
static PLANE Ac, *ac = &Ac, Bc, *bc = &Bc;

    // connecting line is perpendicular to both
    vcross (cdir, adir, bdir);

    // lines are near-parallel -- all points are closest
    if (!vnorm(cdir))   {
        *pA = *a;
        *pB = *b;
        return (0);
        }

    // form plane containing line A, parallel to cdir
    plane_from_two_vectors_and_point (ac, cdir, adir, a);

    // form plane containing line B, parallel to cdir
    plane_from_two_vectors_and_point (bc, cdir, bdir, b);

    // closest point on A is line A ^ bc
    intersect_dline_plane (pA, a, adir, bc);

    // closest point on B is line B ^ ac
    intersect_dline_plane (pB, b, bdir, ac);

    if (edist (pA, pB) < EPS)       
        return (1);     // coincident
    else
        return (2);     // distinct
}

// return: 0 if parallel; 1 if coincident; 2 if generic
int		IntersectLines(const vector3f &vA1,const vector3f &vA2,
										 const vector3f &vB1,const vector3f &vB2,
										 vector3f	&vRes1,vector3f	&vRes2,bool bAbsoluteValues)
{
  xPOINT A = { vA1.x, vA1.y, vA1.z };
	vector3f vDir=vA2-vA1;
	vDir.Normalize();
  VECTOR dA = { vDir.x, vDir.y, vDir.z};
  xPOINT B = { vB1.x, vB1.y, vB1.z };
	vDir=vB2-vB1;
	vDir.Normalize();
  VECTOR dB = { vDir.x, vDir.y, vDir.z };
  xPOINT pA, pB;

	int rval = line_line_closest_points3d(&pA,&pB,&A,&dA,&B,&dB);

	vRes1.Set(pA.px,pA.py,pA.pz);
	vRes2.Set(pB.px,pB.py,pB.pz);

	return (rval);
}

/*
main ()
{
  static xPOINT A = { 0.0, 0.0, 0.0 };
  static VECTOR dA = { 1.0, 1.0, 1.0 };
  static xPOINT B = { 1.0, 0.0, 0.0 };
  static VECTOR dB = { 0.0, 0.0, 1.0 };
  static xPOINT pA, pB;

  int rval;

  rval = line_line_closest_points3d ( &pA, &pB, &A, &dA, &B, &dB );
  printf ( "l_l_c_p returns %d; pA = %g %g %g ; pB = %g %g %g\n",
		   rval, pA.px, pA.py, pA.pz, pB.px, pB.py, pB.pz ); 
}
*/
