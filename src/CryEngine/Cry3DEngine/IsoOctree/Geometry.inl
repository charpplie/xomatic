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
/////////////
// Point3D //
/////////////

float RandomX(void){return float(rand())/RAND_MAX;}

Vec3 RandomBallPoint(void){
	Vec3 p;
	while(1){
		p[0]=float(1.0f-2.0f*RandomX());
		p[1]=float(1.0f-2.0f*RandomX());
		p[2]=float(1.0f-2.0f*RandomX());
		float l=SquareLength(p);
		if(l<=1){return p;}
	}
}

Vec3 RandomSpherePoint(void){
	Vec3 p=RandomBallPoint();
	return p/Length(p);
}


float SquareLength(const Vec3& p)
{
	return DotProduct(p,p);
}


float DotProduct(const Vec3& p,const Vec3& q)
{
	return p[0]*q[0]+p[1]*q[1]+p[2]*q[2];
}


float Length(const Vec3& p)
{
	return float(sqrtf(SquareLength(p)));
}


float SquareDistance(const Vec3& p1,const Vec3& p2)
{
	return SquareLength(p1-p2);
}


float CalcDistance(const Vec3& p1,const Vec3& p2){return float(sqrtf(SquareDistance(p1,p2)));}


void CrossProduct(const Vec3& p1,const Vec3& p2,Vec3& p){
	p[0]= p1[1]*p2[2]-p1[2]*p2[1];
	p[1]=-p1[0]*p2[2]+p1[2]*p2[0];
	p[2]= p1[0]*p2[1]-p1[1]*p2[0];
}

template<class T>
void CrossProductD(const Vec3& p1,const Vec3& p2,Vec3_tpl<T>& p){
  p[0]= p1[1]*p2[2]-p1[2]*p2[1];
  p[1]=-p1[0]*p2[2]+p1[2]*p2[0];
  p[2]= p1[0]*p2[1]-p1[1]*p2[0];
}

Vec3 Normal(const Vec3& p1,const Vec3& p2,const Vec3& p3)
{
	Vec3 q1,q2,n;
	q1=p2-p1;
	q2=p3-p1;
	CrossProduct(q1,q2,n);
	return n;
}


float DistanceToEdge(const Vec3& p,const Vec3 e[2])
{
	Vec3 q,v;
	float dot;
	q=p-e[0];
	v=e[1]-e[0];
	dot=DotProduct(q,v);
	if(dot<=0)
		return Length(q);
	else if (dot>SquareLength(v))
		return CalcDistance(p,e[1]);
	else
	{
		float t=dot/SquareLength(v);
		v=e[0]*(float(1.0)-t)+e[1]*t;
		return CalcDistance(p,v);
	}
}

float DistanceToTriangle(const Vec3& p,const Vec3 t[3])
{
	Vec3 e[2];
	Vec3 q,v,n,nn;
	
	n=Normal(t[0],t[1],t[2]);
	for(int i=0;i<3;i++)
	{
		v=t[(i+1)%3]-t[i];
		q=p-t[i];
		CrossProduct(n,v,nn);
		if(DotProduct(q,nn)<=0)
		{
			e[0]=t[i];
			e[1]=t[(i+1)%3];
			return DistanceToEdge(p,e);
		}
	}
	return fabs(DotProduct(q,n))/Length(n);
}

Vec3 NearestPointOnEdge(const Vec3& p,const Vec3 e[2],int& vFlag)
{
	Vec3 q,v;
	float dot;

	q=p-e[0];
	v=e[1]-e[0];

	dot=DotProduct(q,v);
	if(dot<=0)
	{
		vFlag=1;
		return e[0];
	}
	else if (dot>=SquareLength(v))
	{
		vFlag=2;
		return e[1];
	}
	else
	{
		float t=dot/float(SquareLength(v));
		v=e[0]*(float(1.0)-t)+e[1]*t;
		vFlag=3;
		return v;
	}
}

Vec3 NearestPointOnTriangle(const Vec3& p,const Vec3 t[3],int& vFlag)
{
	Vec3 e[2];
	Vec3 q,v,n,nn,nearest;
	vFlag=0;
	
	n=Normal(t[0],t[1],t[2]);
	for(int i=0;i<3;i++)
	{
		v=t[(i+1)%3]-t[i];
		q=p-t[i];

		CrossProduct(n,v,nn);
		if(DotProduct(q,nn)<=0)
		{
			int tempFlag;
			e[0]=t[i];
			e[1]=t[(i+1)%3];
			nearest=NearestPointOnEdge(p,e,tempFlag);
			if(tempFlag&1) vFlag|=1<<i;
			if(tempFlag&2) vFlag|=1<<((i+1)%3);
			return nearest;
		}
	}

	n/=Length(n);
	v=p-n*DotProduct(q,n);
	vFlag=7;
	return v;
}


int OutCode(const Vec3& ctr,const float& w,const Vec3& p)
{
	int oc=0;
	if(p[0]<ctr[0]-w/2)
		oc|=1;
	if(p[0]>ctr[0]+w/2)
		oc|=2;
	if(p[1]<ctr[1]-w/2)
		oc|=4;
	if(p[1]>ctr[1]+w/2)
		oc|=8;
	if(p[2]<ctr[2]-w/2)
		oc|=16;
	if(p[2]>ctr[2]+w/2)
		oc|=32;
	return oc;
}

int PointInCube(const Vec3& ctr,const float& w,const Vec3& p)
{
	return !OutCode(ctr,w,p);
}

int EdgeInCube(const Vec3& ctr,const float& w,const Vec3 e[2])
{
	int oc[2],dir,off;
	float t,x;
	oc[0]=OutCode(ctr,w,e[0]);
	oc[1]=OutCode(ctr,w,e[1]);
	if(!oc[0] || !oc[1])	return 1;
	if(oc[0] & oc[1])		return 0;
#if 1
	for(dir=0;dir<3;dir++)
		if((oc[0]>>(dir<<1))&3)
		{
			off=( (oc[0]>>(dir<<1))&2) >> 1;
			t=( e[0][dir]-(ctr[dir] - w/2 + w*off) ) / (e[0][dir]-e[1][dir]);
			int inside=0;
			for(int i=1;i<3;i++)
			{
				int j=(dir+i)%3;
				x=e[0][j]*(float(1.0)-t)+e[1][j]*t;
				if(x>=(ctr[j]-w/2) && x<=(ctr[j]+w/2))
					inside++;
			}
			if(inside==2)
				return 1;
		}
	return 0;
#else
	for(dir=0;dir<3;dir++)
		if((oc[0]>>(dir<<1))&3)
			break;
	off=( (oc[0]>>(dir<<1))&2) >> 1;
	t=( e[0][dir]-(ctr[dir] -w/2 + w*off) ) / (e[0][dir]-e[1][dir]);
	for(int i=1;i<3;i++)
	{
		int j=(dir+i)%3;
		x=e[0][j]*(float(1.0)-t)+e[1][j]*t;
		if(x<(ctr[j]-w/2) || x>(ctr[j]+w/2))
			return 0;
	}
	return 1;
#endif
}

int TriangleInCube(const Vec3& ctr,const float& w,const Vec3 t[3])
{
	Vec3 e[2],n,nn[3];
	int oc[3];
	oc[0]=OutCode(ctr,w,t[0]);
	oc[1]=OutCode(ctr,w,t[1]);
	oc[2]=OutCode(ctr,w,t[2]);
	if(!oc[0] || !oc[1] || !oc[2])
		return 1;
	if(oc[0] & oc[1] & oc[2])
		return 0;

	for(int i=0;i<3;i++)
	{
		e[0]=t[i];
		e[1]=t[(i+1)%3];
		if(EdgeInCube(ctr,w,e))
			return 1;
	}

	n=Normal(t[0],t[1],t[2]);
	for(int i=0;i<3;i++)
		CrossProduct(n,t[(i+1)%3]-t[i],nn[i]);

	for(int i=0;i<Cube::EDGES;i++)
	{
		int c1,c2,x[3];
		float tt,dot[2];
		Vec3 p;
		Cube::EdgeCorners(i,c1,c2);
		Cube::FactorCornerIndex(c1,x[0],x[1],x[2]);
		for(int j=0;j<3;j++)	e[0][j]=ctr[j]-w/2+w*x[j];

		Cube::FactorCornerIndex(c2,x[0],x[1],x[2]);
		for(int j=0;j<3;j++)	e[1][j]=ctr[j]-w/2+w*x[j];

		dot[0]=DotProduct(n,e[0]-t[0]);
		dot[1]=DotProduct(n,e[1]-t[0]);
		if(dot[0]*dot[1] >=0 )	continue;
		tt=dot[0]/(dot[0]-dot[1]);
		p=e[0]*(float(1.0)-tt)+e[1]*tt;
		if(DotProduct(p-t[0],nn[0])>0 && DotProduct(p-t[1],nn[1])>0 && DotProduct(p-t[2],nn[2])>0 )
			return 1;
	}
	return 0;
}

int IsBrushAffectsNode(const Vec3& ctr, const float& w, EVoxelEditOperation eOperation, float fMapSize, CTerrainNode * pTerrainNode, const SBrushAABB * pBrushAABB, bool bForcingDepth = false)
{
  Vec3 hw(w*.5f,w*.5f,w*.5f);
  AABB nodeBox(ctr-hw,ctr+hw);

  if(bForcingDepth)
    if(eOperation == eveoCopyTerrainPos || eOperation == eveoCopyTerrainNeg)
  {
    if(pTerrainNode)
    {
      AABB hmBox = pTerrainNode->GetBBox();
      hmBox.max /= fMapSize;
      hmBox.min /= fMapSize;
      return Overlap::AABB_AABB(hmBox,nodeBox) && Overlap::Sphere_AABB2D(pBrushAABB->spEps,nodeBox);
    }
    else
    {
      AABB nodeBoxWS = nodeBox;
      nodeBoxWS.max *= fMapSize;
      nodeBoxWS.min *= fMapSize;
      
      float fHMMin=10000, fHMMax=0;

      for(float x=nodeBoxWS.min.x; x<=nodeBoxWS.max.x; x+=2)
      {
        for(float y=nodeBoxWS.min.y; y<=nodeBoxWS.max.y; y+=2)
        {
          float fH = Cry3DEngineBase::GetTerrain()->GetZ((int)x,(int)y,0);
          fHMMax = max(fHMMax, fH);
          fHMMin = min(fHMMin, fH);
        }
      }

      if(fHMMax<nodeBoxWS.min.z)
        return false;
      else if(fHMMin>nodeBoxWS.max.z)
        return false;

      return Overlap::Sphere_AABB2D(pBrushAABB->spEps,nodeBox);
    }
  }

  return Overlap::Sphere_AABB(pBrushAABB->spEps,nodeBox);
}

