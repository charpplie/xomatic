
//////////////////////////////////////////////////////////////////////////
// A single keypoint, the final result of keypoint creation. Contains the
// keypoint descriptor and position.
class Keypoint
{
public:

	CPhotoImage *image;

	Keypoint ()
	{
		hasFV = false;
		image=NULL;
		featureVector=NULL;
	}

	~Keypoint()
	{
		SAFE_DELETE_ARRAY(featureVector);
	}

	double x, y;
	double imgScale;	// The scale of the image the keypoint was found in
	// The absolute keypoint scale, where 1.0 is the original input image
	double scale;
	double orientation;

	// The actual keypoint descriptor.
	bool hasFV;

	double *featureVector;

	double FVGet (int xI, int yI, int oI)
	{
		return (featureVector[(xI * yDim * oDim) + (yI * oDim) + oI]);
	}
	void FVSet (int xI, int yI, int oI, double value)
	{
		featureVector[(xI * yDim * oDim) + (yI * oDim) + oI] = value;
	}
	
	int FVLinearDim()
	{
		return (DimensionCount());
	}

	double FVLinearGet (int idx)
	{
		return (featureVector[idx]);
	}

	void FVLinearSet (int idx, double value)
	{
		featureVector[idx] = value;
	}

	void CreateLinearVector (int dim)
	{
		featureVector = new double[dim];
	}

	int xDim, yDim, oDim;
	void CreateVector (int xDim, int yDim, int oDim)
	{
		hasFV = true;
		this->xDim = xDim;
		this->yDim = yDim;
		this->oDim = oDim;
		featureVector = new double[yDim * xDim * oDim];
		for (int k=0;k<yDim * xDim * oDim;k++)
			featureVector[k]=0;
	}

	// Keypoint constructor.
	//
	// image: The smoothed gaussian image the keypoint was located in.
	// x, y: The subpixel level coordinates of the keypoint.
	// imgScale: The scale of the gaussian image, with 1.0 being the original
	//    detail scale (source image), and doubling at each octave.
	// kpScale: The scale of the keypoint.
	// orientation: Orientation degree in the range of [-PI ; PI] of the
	//    keypoint.
	//
	// First add a keypoint, then use 'MakeDescriptor' to generate the local
	// image descriptor for this keypoint.
	Keypoint (CPhotoImage *image, double x, double y, double imgScale,
		double kpScale, double orientation)
	{
		this->image = image;
		this->x = x;
		this->y = y;
		this->imgScale = imgScale;
		this->scale = kpScale;
		this->orientation = orientation;
	}

	int DimensionCount() 
	{
		return(yDim * xDim * oDim);
		/*
		get {
			return (FVLinearDim);
		}
		*/
	}

	double GetDimensionElement (int dim)
	{
		return (FVLinearGet (dim));
	}
};

typedef std::vector<Keypoint *> lstKeypoint;
typedef lstKeypoint::iterator		lstKeypointIt;
