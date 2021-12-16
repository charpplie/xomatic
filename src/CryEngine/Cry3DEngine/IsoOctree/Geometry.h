/*
Copyright (c) 2006, Michael Kazhdan and Matthew Bolitho
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of
conditions and the following disclaimer. Redistributions in binary form must reproduce
the above copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the distribution. 

Neither the name of the Johns Hopkins University nor the names of its contributors
may be used to endorse or promote products derived from this software without specific
prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO THE IMPLIED WARRANTIES 
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.
*/
#ifndef GEOMETRY_INCLUDED
#define GEOMETRY_INCLUDED
#include <math.h>
#include <vector>


//float Random(void);


struct Point3D{
	float coords[3];

	float& operator[] (const int& idx);
	const float& operator[] (const int& idx) const;
	Point3D operator + (const Point3D& p) const;
	Point3D operator - (const Point3D& p) const;
	Point3D operator * (const float& s) const;
	Point3D operator / (const float& s) const;
	Point3D& operator += (const Point3D& p);
	Point3D& operator -= (const Point3D& p);
	Point3D& operator *= (const float& s);
  Point3D& operator /= (const float& s);
  Point3D(const Vec3 & p) { coords[0] = p[0]; coords[1] = p[1]; coords[2] = p[2]; }
  Point3D() { coords[0] = coords[1] = coords[2] = 0; }
};


Vec3 RandomBallPoint(void);


Vec3 RandomSpherePoint(void);


float Length(const Vec3& p);


float SquareLength(const Vec3& p);


float DotProduct(const Vec3& p,const Vec3& q);


float CalcDistance(const Vec3& p1,const Vec3& p2);


float SquareDistance(const Vec3& p1,const Vec3& p2);

void CrossProduct(const Vec3& p1,const Vec3& p2,Vec3& p);

Vec3 Normal(const Vec3& p1,const Vec3& p2,const Vec3& p3);


float DistanceToEdge(const Vec3& p,const Vec3 e[2]);

float DistanceToTriangle(const Vec3& p,const Vec3 t[3]);


Vec3 NearestPointOnEdge(const Vec3& p,const Vec3 e[2],int& vFlag);

Vec3 NearestPointOnTriangle(const Vec3& p,const Vec3 t[3],int& vFlag);



int OutCode(const Vec3& ctr,const float& w,const Vec3& p);


int PointInCube(const Vec3& ctr,const float& w,const Vec3& p);

int EdgeInCube(const Vec3& ctr,const float& w,const Vec3 e[2]);

int TriangleInCube(const Vec3& ctr,const float& w,const Vec3 t[3]);

struct SBrushAABB
{
  AABB boxEps;
  Sphere spEps;
  float fEps;
};

#include "Geometry.inl"

#endif // GEOMETRY_INCLUDED
