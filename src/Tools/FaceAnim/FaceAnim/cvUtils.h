//  Utility functions for openCV

// This could be faster if specialised
template <class T> void cvuSetImageElement( IplImage* image, int x, int y, T value)
{
	int elementSize = sizeof(T);
	int offset = (y * image->widthStep + x * elementSize);
	T* element = (T*) (void*) (image->imageData + offset);
	*element = value;
}

// This could be faster if specialised
template <class T> void cvuGetImageElement( IplImage* image, int x, int y, T &value)
{
	int elementSize = sizeof(T);
	int offset = (y * image->widthStep + x * elementSize);
	T* element = (T*) (void*) (image->imageData + offset);
	value = *element;
}

// Intended for use with CvPoints
// Would be nice if it returned doubles if points are doubles
template <class T> float cvuGetPointsDistanceSq( T p1, T p2 )
{
	return sqr(p1.x - p2.x) + sqr(p1.y - p2.y);
}


template <class T> float cvuGetPointsDistance( T p1, T p2 )
{
	return sqrt(cvuGetPointsDistanceSq(p1,p2));
}

template <class T> T cvuPointAdd( T p1, T p2 )
{
	T result;
	result.x = p1.x + p2.x;
	result.y = p1.y + p2.y;
	return result;
}

template <class T> T cvuPointSub( T p1, T p2 )
{
	T result;
	result.x = p1.x - p2.x;
	result.y = p1.y - p2.y;
	return result;
}

template <class T> T cvuPointMul( T p1, float m )
{
	T result;
	result.x = p1.x * m;
	result.y = p1.y * m;
	return result;
}

template <class T> T cvuPointDiv( T p1, float d )
{
	T result;
	result.x = p1.x / d;
	result.y = p1.y / d;
	return result;
}



// Takes a vector and returns an angle in radians measuring clockwise from +x, +
template <class T> float cvuGetVectorAngleRad(T vCentreToPoint)
{
	float radAngle = atan(vCentreToPoint.y/vCentreToPoint.x);
	// Deal with quadrants
	if (vCentreToPoint.x < 0.0f)
	{
		radAngle = (float)CV_PI + radAngle;
	}
	else if (vCentreToPoint.y < 0.0f)
	{
		radAngle = 2.0f * (float)CV_PI + radAngle;
	}
	return radAngle;
}

template <class T> CvPoint cvuToPoint(T p)
{
	CvPoint result = cvPoint((int)p.x, (int)p.y);
	return result;
}

template <class T> CvPoint2D32f cvuToPoint2D32f(T p)
{
	CvPoint2D32f result = cvPoint2D32f((float)p.x, (float)p.y));
	return result;
}


template <class T> T cvuRadToDeg(T rad)
{
	T deg = rad * (T)(180/CV_PI);
	return deg;
}

template <class T> T cvuDegToRad(T deg)
{
	T rad = deg * (T)(CV_PI/180);
	return rad;
}




