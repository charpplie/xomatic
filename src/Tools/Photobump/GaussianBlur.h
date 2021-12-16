
//////////////////////////////////////////////////////////////////////////
class ConvLinearMask
{
public:
	
	int dim;
	int Count() {
			return (dim);		
	}
	int middle;
	int Middle() {
			return (middle);		
	}
	double* mask;
	double GetVal(int idx)
	{
		return (mask[idx]);
	}
	/*
	double this[int idx] {
		get {
			return (mask[idx]);
		}
		set {
			mask[idx] = value;
		}
	}
	*/
	double maskSum;
	/*
	double MaskSum {
		get {
			return (maskSum);
		}
		set {
			maskSum = value;
		}
	}
	*/

	~ConvLinearMask()
	{
		SAFE_DELETE(mask);
	}

	ConvLinearMask (int dim)
	{
		mask = new double[dim];
		this->dim = dim;
		this->middle = dim / 2;
		maskSum=0;
	}
};

// Static utility functions for convolution
//////////////////////////////////////////////////////////////////////////
class ConvolutionFilter
{
public:

	~ConvolutionFilter()
	{
		SAFE_DELETE(res);
		SAFE_DELETE(res2);
	}
	
	CPhotoImage *res,*res2;

	enum Direction {
		Vertical,
		Horizontal,
	};

	CPhotoImage *Convolve (CPhotoImage *img, ConvLinearMask *mask)
	{
		res = img->Resize(img->m_nWidth, img->m_nHeight);
		res2 = img->Resize(img->m_nWidth, img->m_nHeight);

		Convolve1D (res, mask, img, Vertical);
		Convolve1D (res2, mask, res, Horizontal);

		return (res2);
	}

	void Convolve1D (CPhotoImage *dest, ConvLinearMask *mask,
		CPhotoImage *src, Direction dir)
	{
		int maxN;	// outer loop maximum index
		int maxP;	// inner loop maximum index

		if (dir == Vertical) 
		{
			maxN = src->m_nWidth;
			maxP = src->m_nHeight;
		} 
		else 
		if (dir == Horizontal) 
		{
			maxN = src->m_nHeight;
			maxP = src->m_nWidth;
		} 
		else
			ASSERT(0);

		for (int n = 0 ; n < maxN ; ++n) 
		{
			for (int p = 0 ; p < maxP ; ++p) 
			{
				double val = CalculateConvolutionValue1D (src, mask,
					n, p, maxN, maxP, dir);

				ASSERT(val>=0 && val<=255);

				if (dir == Vertical)
					dest->m_pRawPixels[p*dest->m_nWidth+n] = (int)(val);
					//dest->m_pRawPixels[n*dest->m_nWidth+p] = (int)(val);
				else
					//dest->m_pRawPixels[p*dest->m_nWidth+n] = (int)(val);
					dest->m_pRawPixels[n*dest->m_nWidth+p] = (int)(val);
			}
		}
	}

	double CalculateConvolutionValue1D (CPhotoImage *src,
		ConvLinearMask *mask, int n, int p, int maxN, int maxP, Direction dir)
	{
		double sum = 0.0;

		bool isOut = false;
		double outBound = 0.0;	// values that are out of bound

		for (int xw = 0 ; xw < mask->Count() ; ++xw) {
			int curAbsP = xw - mask->Middle() + p;

			if (curAbsP < 0 || curAbsP >= maxP) {
				isOut = true;
				outBound += mask->GetVal(xw);

				continue;
			}

			if (dir == Vertical)
				sum += mask->GetVal(xw) * src->m_pRawPixels[curAbsP*src->m_nWidth+n];
				//sum += mask->GetVal(xw) * src->m_pRawPixels[n*src->m_nWidth+curAbsP];
			else
				sum += mask->GetVal(xw) * src->m_pRawPixels[n*src->m_nWidth+curAbsP];
				//sum += mask->GetVal(xw) * src->m_pRawPixels[curAbsP*src->m_nWidth+n];
		}

		// if part of the mask was outside, correct the resulting value by the
		// in/out ratio.
		if (isOut)
			sum *= 1.0 / (1.0 - outBound);

		return (sum);
	}
};

//////////////////////////////////////////////////////////////////////////
class GaussianConvolution
{
public:

	ConvLinearMask *mask;

	GaussianConvolution()
	{
		mask=NULL;
	}

	~GaussianConvolution()
	{
		SAFE_DELETE(mask);
	}

	GaussianConvolution (double sigma)
	{
		// From "Image Processing, Analysis and Machine Vision", pp. 84:
		// 'Pixels more distant from the center of the operator have smaller
		// influence, and pixels farther than 3 \sigma from the center have
		// neglible influence.'
		//
		// So, find the kernel size by rounding twice 3 \sigma up.
		GaussianConv(sigma, 1 + 2 * ((int) (3.0 * sigma)));	
	}

	// Like Generate, but manually specifying the kernel size.
	void GaussianConv(double sigma, int dim)
	{
		// Assert the kernel size is odd, so we have a clear center pixel.
		dim |= 1;
		mask=new ConvLinearMask(dim);

		double sigma2sq = 2 * sigma * sigma;
		double normalizeFactor = 1.0 / (sqrt (2.0 * M_PI) * sigma);

		for (int n = 0 ; n < dim ; ++n) 
		{
			int relPos = n - mask->Middle();

			double G = (relPos * relPos) / sigma2sq;
			G = exp(-G);
			G *= normalizeFactor;
			mask->mask[n] = G;
			mask->maskSum += G;
		}
	}

	// Apply the gaussian filter.
	void Convolve (CPhotoImage *img)
	{
		ConvolutionFilter filter;
		CPhotoImage *pRes=filter.Convolve(img, mask);
		memcpy(img->m_pRawPixels,pRes->m_pRawPixels,img->m_nWidth*img->m_nHeight);
	}
};
