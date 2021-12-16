// This code is in the public domain -- Ignacio Castaño <castanyo@yahoo.es>

#include <nvmath/Triangle.h>

using namespace nv;


/// Tomas Möller, barycentric ray-triangle test.
bool rayTest_Moller(const Triangle & t, Vector3::Arg orig, Vector3::Arg dir, float * out_t, float * out_u, float * out_v)
{
	// find vectors for two edges sharing vert0 
	Vector3 e1 = t.v[1] - t.v[0];
	Vector3 e2 = t.v[2] - t.v[0];

	// begin calculating determinant - also used to calculate U parameter
	Vector3 pvec = cross(dir, e2);
	
	// if determinant is near zero, ray lies in plane of triangle
	float det = dot(e1, pvec);
	if (det < -NV_EPSILON) {
		return false;
	}

	// calculate distance from vert0 to ray origin
	Vector3 tvec = orig - t.v[0];

	// calculate U parameter and test bounds
	float u = dot(tvec, pvec);
	if( u < 0.0f || u > det ) {
		return false;
	}

	// prepare to test V parameter
	Vector3 qvec = cross(tvec, e1);

	// calculate V parameter and test bounds
	float v = dot(dir, qvec);
	if (v < 0.0f || u + v > det) {
		return false;
	}

	// calculate t, scale parameters, ray intersects triangle
	float inv_det = 1.0f / det;
	*out_t = dot(e2, qvec) * inv_det;
	*out_u = u * inv_det;	// v
	*out_v = v * inv_det;	// 1-(u+v)

	return true;
}























































































































