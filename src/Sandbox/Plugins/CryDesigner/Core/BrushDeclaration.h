#pragma once

#define ENABLE_BRUSH_F64

#ifdef ENABLE_BRUSH_F64
typedef f64 BrushFloat;
#else
typedef float BrushFloat;
#endif

typedef Vec2_tpl<BrushFloat> BrushVec2;
typedef Vec3_tpl<BrushFloat> BrushVec3;
typedef Vec4_tpl<BrushFloat> BrushVec4;
typedef Matrix33_tpl<BrushFloat> BrushMatrix33;
typedef Matrix34_tpl<BrushFloat> BrushMatrix34;
typedef Matrix44_tpl<BrushFloat> BrushMatrix44;

static bool operator < ( const BrushVec3& v0, const BrushVec3& v1 )
{
	return v0.x < v1.x || v0.x == v1.x && v0.y < v1.y || v0.x == v1.x && v0.y == v1.y && v0.z < v1.z;
}

static BrushMatrix33 ToBrushMatrix33( const Matrix33& matrix )
{
	if( sizeof(BrushMatrix33) == sizeof(Matrix33) )
		return matrix;

	static BrushMatrix33 brushTM;

	for( int i = 0; i < 3; ++i )
		for(int k = 0; k < 3; ++k )
			brushTM(i,k) = BrushFloat(matrix(i,k));

	return brushTM;
}

static BrushMatrix34 ToBrushMatrix34( const Matrix34& matrix )
{
	if( sizeof(BrushMatrix34) == sizeof(Matrix34) )
		return matrix;

	static BrushMatrix34 brushTM;

	for( int i = 0; i < 3; ++i )
		for(int k = 0; k < 4; ++k )
			brushTM(i,k) = BrushFloat(matrix(i,k));

	return brushTM;
}

static Vec3 ToVec3( const BrushVec3& v )
{
	if( sizeof(BrushVec3) == sizeof(Vec3) )
		return v;
	return Vec3( (float)v.x, (float)v.y, (float)v.z );
}

static BrushVec3 ToBrushVec3( const Vec3& v )
{
	if( sizeof(BrushVec3) == sizeof(Vec3) )
		return v;
	return BrushVec3( (BrushFloat)v.x, (BrushFloat)v.y, (BrushFloat)v.z );
}

static float ToFloat( const BrushFloat f )
{
	if( sizeof(BrushFloat) == sizeof(float) )
		return f;
	return (float)f;
}