///////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   BrushPrimitive.cpp
//  Version:     v1.00
//  Created:     5/5/2010 by Jaesik.
//  Compilers:   Visual Studio.NET
//  Description: 
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "BrushPrimitive.h"
#include "Brush.h"

static const float PI = 3.14159265f;

void CBrushPrimitive::CreateBox( SBrush* brush, const Vec3 &mins, const Vec3& maxs ) const
{
	for(int i=0 ; i<3 ; ++i)
	{
		if (maxs[i] < mins[i])
			CLogFile::WriteLine ("Error: SBrush::Create: backwards");
	}

	Vec3 vlist[] = {	mins, 
										Vec3(mins[0], maxs[1], mins[2]),
										Vec3(maxs[0], maxs[1], mins[2]),
										Vec3(maxs[0], mins[1], mins[2]),
										Vec3(mins[0], mins[1], maxs[2]),
										Vec3(mins[0], maxs[1], maxs[2]),
										maxs,
										Vec3(maxs[0], mins[1], maxs[2])	};

	int indexlist[][4] = {	{0,4,5,1},  
													{1,5,6,2},  
													{2,6,7,3},  
													{3,7,4,0},  
													{6,5,4,7},  
													{0,1,2,3} };

	int indexnum = sizeof(indexlist)/sizeof(*indexlist);
	for( int i=0; i<indexnum; ++i )
	{
		size_t basevtxidx = brush->GetNumberOfVertices();

		for( int k=0; k<4; ++k )
			brush->AddVertex(vlist[indexlist[i][k]]);			

		brush->AddTriangle(basevtxidx+0, basevtxidx+1, basevtxidx+2);
		brush->AddTriangle(basevtxidx+2, basevtxidx+3, basevtxidx+0);		
	}	
}


void CBrushPrimitive::CreateSphere( SBrush* brush, const Vec3& mins, const Vec3& maxs, int numSides ) const
{
	assert( numSides >= 3 );

	float radius = ( ( maxs - mins ) * 0.5f ).GetLength();

	float costheta(radius*cosf(0));
	float sintheta(radius*sinf(0));
	float cosnexttheta(costheta);
	float sinnexttheta(sintheta);

	for( int i = 1; i < numSides+1; ++i )
	{
		float nexttheta	= PI * (float)i/(float)numSides;	

		cosnexttheta = radius * cosf(nexttheta);
		sinnexttheta = radius * sinf(nexttheta);

		float cosphi(cosf(0));
		float sinphi(sinf(0));
		float cosnextphi(cosphi);
		float sinnextphi(sinphi);

		for( int j = 1; j < numSides+1; ++j )
		{			
			float nextphi = 2 * PI * (float)j/(float)numSides;

			cosnextphi = cosf(nextphi);
			sinnextphi = sinf(nextphi);

			SBrushCommon::VertexFaceIndexType baseidx = (SBrushCommon::VertexFaceIndexType)brush->GetNumberOfVertices();

			if( i == 1 || i == numSides )
			{
				brush->AddVertex( Vec3(sintheta*cosphi, sintheta*sinphi,	costheta) );				

				if( i == 1 )
				{
					brush->AddVertex( Vec3( sinnexttheta*cosnextphi,	sinnexttheta*sinnextphi, cosnexttheta ) );
					brush->AddVertex( Vec3( sinnexttheta*cosphi,			sinnexttheta*sinphi,		 cosnexttheta ) );
				}
				else if( i == numSides )
				{
					brush->AddVertex( Vec3( sintheta*cosnextphi,			sintheta*sinnextphi,			costheta ) );
					brush->AddVertex( Vec3( sinnexttheta*cosnextphi,	sinnexttheta*sinnextphi,	cosnexttheta ) );
				}

				brush->AddTriangle(baseidx, baseidx+2, baseidx+1);
			}
			else
			{
				brush->AddVertex( Vec3( sintheta*cosphi,					sintheta*sinphi,					costheta ) );
				brush->AddVertex( Vec3( sintheta*cosnextphi,			sintheta*sinnextphi,			costheta ) );
				brush->AddVertex( Vec3( sinnexttheta*cosnextphi,	sinnexttheta*sinnextphi,	cosnexttheta ) );
				brush->AddVertex( Vec3( sinnexttheta*cosphi,			sinnexttheta*sinphi,			cosnexttheta ) );

				brush->AddTriangle( baseidx+2, baseidx+1, baseidx );
				brush->AddTriangle( baseidx, baseidx+3, baseidx+2 );
			}

			cosphi = cosnextphi;
			sinphi = sinnextphi;
		}

		costheta	= cosnexttheta;
		sintheta	= sinnexttheta;
	}
}


void CBrushPrimitive::CreateCylinder( SBrush* brush, const Vec3& mins, const Vec3& maxs, int numSides ) const
{
	assert( numSides >= 3 );

	Vec3 BottomMaxs(maxs.x,maxs.y,0);
	Vec3 BottomMinx(mins.x,mins.y,0);
	float radius = ( ( BottomMaxs - BottomMinx ) * 0.5f ).GetLength();

	float costheta(cosf(0));
	float sintheta(sinf(0));

	for( int i = 0; i < numSides; ++i )
	{
		float theta = 2 * PI * (float)i/(float)numSides;

		costheta = cosf(theta);
		sintheta = sinf(theta);

		float x = radius * costheta;
		float y = radius * sintheta;

		brush->AddVertex( Vec3(x,y,mins.z) );
	}

	for( int i = 0; i < numSides; ++i )
	{
		const Vec3& v = brush->GetVertexPos(i);
		brush->AddVertex( Vec3( v.x, v.y, maxs.z ) );
	}

	for( int i = 0; i < numSides-2; ++i )
	{
		brush->AddTriangle( i+2, i+1, 0 );
	}

	for( int i = numSides; i < 2*numSides-2; ++i )
	{
		brush->AddTriangle( numSides, i+1, i+2 );
	}

	for( int i = 0; i < numSides; ++i )
	{
		int nexti = (i+1)%numSides;

		size_t basevidx = brush->GetNumberOfVertices();

		brush->AddVertex( brush->GetVertexPos(i) );
		brush->AddVertex( brush->GetVertexPos(nexti) );
		brush->AddVertex( brush->GetVertexPos(numSides+nexti) );
		brush->AddVertex( brush->GetVertexPos(numSides+i) );

		brush->AddTriangle( basevidx, basevidx+1, basevidx+2 );
		brush->AddTriangle( basevidx+2, basevidx+3, basevidx );
	}
}

void CBrushPrimitive::CreateSimpleMesh( SBrush* brush, const std::vector<Vec3>& vtxs, const float height) const
{
	bool reverseWinding = false;

	//find winding order
	Vec3 cross = (vtxs[2] - vtxs[0]).Cross(vtxs[1] - vtxs[0]);
	if ( cross.z < 0.0f )
	{
		//counter-clockwise creation order needs revers winding
		reverseWinding = true;
	}

	//add first verts
	brush->AddVertex(vtxs[0]);

	Vec3 newvtx(vtxs[0]);
	newvtx.z += height;
	brush->AddVertex(newvtx);

	//verts
	if ( !reverseWinding )
	{
		int numsideverts = vtxs.size();
		for (int vid = 1; vid < numsideverts; ++vid )
		{
			brush->AddVertex(vtxs[vid]);

			Vec3 newvtx(vtxs[vid]);
			newvtx.z += height;
			brush->AddVertex(newvtx);
		}
	}
	else
	{
		for (int vid = vtxs.size()-1; vid > 0; vid-- )
		{
			brush->AddVertex(vtxs[vid]);

			Vec3 newvtx(vtxs[vid]);
			newvtx.z += height;
			brush->AddVertex(newvtx);
		}
	}

	//side faces
	int numsidefaces = vtxs.size() * 2;
	for ( int fid = 0; fid < numsidefaces; ++fid )
	{	
		int v1id = fid;
		int v2id = fid+1 < numsidefaces ? fid + 1 : (fid+1)-numsidefaces;
		int v3id = fid+2 < numsidefaces ? fid + 2 : (fid+2)-numsidefaces;
		if ( (fid % 2) ) //fix winding order
		{			
			brush->AddTriangle(v1id,v3id,v2id);
		}
		else
		{
			brush->AddTriangle(v1id,v2id,v3id);			
		}
	}

	int numbottomfaces = vtxs.size() - 2;
	for ( int fid = 0; fid < numbottomfaces; ++fid )
	{
		int v1id = 0;
		int v2id = 2 + ( fid*2 );
		int v3id = 4 + ( fid*2 );
		brush->AddTriangle(v1id,v2id,v3id);
	}

	int numtopfaces = vtxs.size() - 2;
	for ( int fid = 0; fid < numtopfaces; ++fid )
	{
		int v1id = 1;
		int v2id = 3 + ( fid*2 );
		int v3id = 5 + ( fid*2 );
		brush->AddTriangle(v1id,v3id,v2id);
	}
}

void CBrushPrimitive::CreateCone( SBrush* brush, const Vec3& mins, const Vec3& maxs, int numSides ) const
{
	assert( numSides >= 3 );

	Vec3 BottomMaxs(maxs.x,maxs.y,0);
	Vec3 BottomMinx(mins.x,mins.y,0);
	float radius = ( ( BottomMaxs - BottomMinx ) * 0.5f ).GetLength();

	float costheta(cosf(0));
	float sintheta(sinf(0));

	for( int i = 0; i < numSides; ++i )
	{
		float theta = 2 * PI * (float)i/(float)numSides;

		costheta = cosf(theta);
		sintheta = sinf(theta);

		float x = radius * costheta;
		float y = radius * sintheta;

		brush->AddVertex(Vec3(x,y,mins.z));		
	}

	for( int i = 0; i < numSides-2; ++i )
	{
		brush->AddTriangle(i+2, i+1, 0);
	}

	Vec3 center = ((maxs+mins)*0.5f);
	Vec3 peak(center.x,center.y,maxs.z);

	for( int i = 0; i < numSides; ++i )
	{
		size_t basevidx = brush->GetNumberOfVertices();
		brush->AddVertex(peak);
		brush->AddVertex(brush->GetVertexPos(i));
		brush->AddVertex(brush->GetVertexPos((i+1)%numSides));
		brush->AddTriangle(basevidx, basevidx+1, basevidx+2);
	}
}

void CBrushPrimitive::CreatePlane( SBrush* brush, const Vec3& mins, const Vec3& maxs ) const
{
	if (maxs[0] < mins[0] || maxs[1] < mins[1])
		CLogFile::WriteLine ("Error: SBrush::Create: backwards");

	Vec3 vlist[] = {
		Vec3(mins[0], mins[1],0),
		Vec3(maxs[0],mins[1],0),
		Vec3(maxs[0],maxs[1],0),
		Vec3(mins[0],maxs[1],0)
	};

	int indexlist[4] = {	 0,1, 2, 3 };

	for( int k=0; k<4; ++k )
		brush->AddVertex(vlist[indexlist[k]]);

	brush->AddTriangle(0, 1, 3);
	brush->AddTriangle(1, 2, 3);
}