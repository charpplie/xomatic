///////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   BrushPrimitive.h
//  Version:     v1.00
//  Created:     5/5/2010 by Jaesik.
//  Compilers:   Visual Studio.NET
//  Description: 
////////////////////////////////////////////////////////////////////////////

struct SBrush;

class CBrushPrimitive
{

public:
	
	void CreateBox( SBrush* brush, const Vec3 &mins, const Vec3& maxs ) const;
	void CreateSphere( SBrush* brush, const Vec3& mins, const Vec3& maxs, int numSides ) const;
	void CreateCylinder( SBrush* brush, const Vec3& mins, const Vec3& maxs, int numSides ) const;
	void CreateSimpleMesh(SBrush* brush, const std::vector<Vec3>& vtxs, const float height) const;
	void CreateCone( SBrush* brush, const Vec3& mins, const Vec3& maxs, int numSides ) const;
	void CreatePlane( SBrush* brush, const Vec3& mins, const Vec3& maxs ) const;

};