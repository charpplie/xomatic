#ifndef _2_PASS_SCALE_H_
#define _2_PASS_SCALE_H_

#include <math.h> 

#include "Filters.h" 

typedef struct 
{ 
   double *Weights;  // Normalized weights of neighboring pixels
   int Left,Right;   // Bounds of source pixels window
} ContributionType;  // Contirbution information for a single pixel

typedef struct 
{ 
   ContributionType *ContribRow; // Row (or column) of contribution weights 
   UINT WindowSize,              // Filter window size (of affecting source pixels) 
        LineLength;              // Length of line (no. or rows / cols) 
} LineContribType;               // Contribution information for an entire line (row or column)

typedef BOOL (*ProgressAnbAbortCallBack)(BYTE bPercentComplete);

template <class FilterClass>
class C2PassScale 
{
public:

    C2PassScale (ProgressAnbAbortCallBack callback = NULL) : 
        m_Callback (callback) {}

    virtual ~C2PassScale() {}

    UCHAR * AllocAndScale (  
        UCHAR   *pOrigImage, 
        UINT        uOrigWidth, 
        UINT        uOrigHeight, 
        UINT        uNewWidth, 
        UINT        uNewHeight,
				UINT				nBpp);

    UCHAR * Scale (  
        UCHAR   *pOrigImage, 
        UINT        uOrigWidth, 
        UINT        uOrigHeight, 
        UCHAR   *pDstImage,
        UINT        uNewWidth, 
        UINT        uNewHeight,
				UINT				nBpp);

private:

    ProgressAnbAbortCallBack    m_Callback;
    BOOL                        m_bCanceled;

    LineContribType *AllocContributions (   UINT uLineLength, 
                                            UINT uWindowSize);

    void FreeContributions (LineContribType * p);

    LineContribType *CalcContributions (    UINT    uLineSize, 
                                            UINT    uSrcSize, 
                                            double  dScale);

    void ScaleRow ( UCHAR           *pSrc, 
                    UINT                uSrcWidth,
                    UCHAR           *pRes, 
                    UINT                uResWidth,
                    UINT                uRow, 
                    LineContribType    *Contrib,
										UINT								nBpp);

    void HorizScale (   UCHAR           *pSrc, 
                        UINT                uSrcWidth,
                        UINT                uSrcHeight,
                        UCHAR           *pDst,
                        UINT                uResWidth,
                        UINT                uResHeight,
												UINT								nBpp);

    void ScaleCol ( UCHAR           *pSrc, 
                    UINT                uSrcWidth,
                    UCHAR           *pRes, 
                    UINT                uResWidth,
                    UINT                uResHeight,
                    UINT                uCol, 
                    LineContribType    *Contrib,
										UINT								nBpp);

    void VertScale (    UCHAR           *pSrc, 
                        UINT                uSrcWidth,
                        UINT                uSrcHeight,
                        UCHAR           *pDst,
                        UINT                uResWidth,
                        UINT                uResHeight,
												UINT								nBpp);
};

template <class FilterClass>
LineContribType *
C2PassScale<FilterClass>::
AllocContributions (UINT uLineLength, UINT uWindowSize)
{
    LineContribType *res = new LineContribType; 
        // Init structure header 
    res->WindowSize = uWindowSize; 
    res->LineLength = uLineLength; 
        // Allocate list of contributions 
    res->ContribRow = new ContributionType[uLineLength];
    for (UINT u = 0 ; u < uLineLength ; u++) 
    {
        // Allocate contributions for every pixel
        res->ContribRow[u].Weights = new double[uWindowSize];
    }
    return res; 
} 
 
template <class FilterClass>
void
C2PassScale<FilterClass>::
FreeContributions (LineContribType * p)
{ 
    for (UINT u = 0; u < p->LineLength; u++) 
    {
        // Free contribs for every pixel
        delete [] p->ContribRow[u].Weights;
    }
    delete [] p->ContribRow;    // Free list of pixels contribs
    delete p;                   // Free contribs header
} 
 
template <class FilterClass>
LineContribType *
C2PassScale<FilterClass>::
CalcContributions (UINT uLineSize, UINT uSrcSize, double dScale)
{ 
    FilterClass CurFilter;

    double dWidth;
    double dFScale = 1.0;
    double dFilterWidth = CurFilter.GetWidth();

    if (dScale < 1.0) 
    {    // Minification
        dWidth = dFilterWidth / dScale; 
        dFScale = dScale; 
    } 
    else
    {    // Magnification
        dWidth= dFilterWidth; 
    }
 
    // Window size is the number of sampled pixels
    int iWindowSize = 2 * (int)ceil(dWidth) + 1; 
 
    // Allocate a new line contributions strucutre
    LineContribType *res = AllocContributions (uLineSize, iWindowSize); 
 
    for (UINT u = 0; u < uLineSize; u++) 
    {   // Scan through line of contributions
        double dCenter = (double)u / dScale;   // Reverse mapping
        // Find the significant edge points that affect the pixel
        int iLeft = max (0, (int)floor (dCenter - dWidth)); 
        res->ContribRow[u].Left = iLeft; 
        int iRight = min ((int)ceil (dCenter + dWidth), int(uSrcSize) - 1); 
        res->ContribRow[u].Right = iRight;
 
        // Cut edge points to fit in filter window in case of spill-off
        if (iRight - iLeft + 1 > iWindowSize) 
        {
            if (iLeft < (int(uSrcSize) - 1 / 2)) 
            {
                iLeft++; 
            }
            else 
            {
                iRight--; 
            }
        }
        double dTotalWeight = 0.0;  // Zero sum of weights
        for (int iSrc = iLeft; iSrc <= iRight; iSrc++)
        {   // Calculate weights
            dTotalWeight += (res->ContribRow[u].Weights[iSrc-iLeft] =  
                                dFScale * CurFilter.Filter (dFScale * (dCenter - (double)iSrc))); 
        }
        ASSERT (dTotalWeight >= 0.0);   // An error in the filter function can cause this 
        if (dTotalWeight > 0.0)
        {   // Normalize weight of neighbouring points
            for (iSrc = iLeft; iSrc <= iRight; iSrc++)
            {   // Normalize point
                res->ContribRow[u].Weights[iSrc-iLeft] /= dTotalWeight; 
            }
        }
   } 
   return res; 
} 
 
 
template <class FilterClass>
void 
C2PassScale<FilterClass>::
ScaleRow (  UCHAR           *pSrc, 
            UINT                uSrcWidth,
            UCHAR           *pRes, 
            UINT                uResWidth,
            UINT                uRow, 
            LineContribType    *Contrib,
						UINT								nBpp)
{
    UCHAR *pSrcRow = &(pSrc[uRow * uSrcWidth * (nBpp>>3)]);
    UCHAR *pDstRow = &(pRes[uRow * uResWidth * (nBpp>>3)]);
		if (nBpp==24)
		{
			for (UINT x = 0; x < uResWidth; x++) 
			{   // Loop through row
					BYTE r = 0;
					BYTE g = 0;
					BYTE b = 0;
					int iLeft = Contrib->ContribRow[x].Left;    // Retrieve left boundries
					int iRight = Contrib->ContribRow[x].Right;  // Retrieve right boundries
					for (int i = iLeft; i <= iRight; i++)
					{   // Scan between boundries
							// Accumulate weighted effect of each neighboring pixel
							r += (BYTE)(Contrib->ContribRow[x].Weights[i-iLeft] * (double)((pSrcRow[i*3+0]))); 
							g += (BYTE)(Contrib->ContribRow[x].Weights[i-iLeft] * (double)((pSrcRow[i*3+1]))); 
							b += (BYTE)(Contrib->ContribRow[x].Weights[i-iLeft] * (double)((pSrcRow[i*3+2]))); 
					} 
					//MyOutputDebugString("dstrowx=%d\n",x);
					pDstRow[x*3+0] = r; // Place result in destination pixel
					pDstRow[x*3+1] = g; // Place result in destination pixel
					pDstRow[x*3+2] = b; // Place result in destination pixel
			} 
		}
		else
		if (nBpp==32)
		{
			for (UINT x = 0; x < uResWidth; x++) 
			{   // Loop through row
					BYTE r = 0;
					BYTE g = 0;
					BYTE b = 0;
					BYTE a = 0;
					int iLeft = Contrib->ContribRow[x].Left;    // Retrieve left boundries
					int iRight = Contrib->ContribRow[x].Right;  // Retrieve right boundries
					for (int i = iLeft; i <= iRight; i++)
					{   // Scan between boundries
							// Accumulate weighted effect of each neighboring pixel
							r += (BYTE)(Contrib->ContribRow[x].Weights[i-iLeft] * (double)((pSrcRow[i*3+0]))); 
							g += (BYTE)(Contrib->ContribRow[x].Weights[i-iLeft] * (double)((pSrcRow[i*3+1]))); 
							b += (BYTE)(Contrib->ContribRow[x].Weights[i-iLeft] * (double)((pSrcRow[i*3+2]))); 
							a += (BYTE)(Contrib->ContribRow[x].Weights[i-iLeft] * (double)((pSrcRow[i*3+3]))); 
					} 
					pDstRow[x*3+0] = r; // Place result in destination pixel					
					pDstRow[x*3+1] = g; // Place result in destination pixel					
					pDstRow[x*3+2] = b; // Place result in destination pixel					
					pDstRow[x*3+3] = a; // Place result in destination pixel					
			} 
		}
} 

template <class FilterClass>
void
C2PassScale<FilterClass>::
HorizScale (    UCHAR           *pSrc, 
                UINT                uSrcWidth,
                UINT                uSrcHeight,
                UCHAR           *pDst, 
                UINT                uResWidth,
                UINT                uResHeight,
								UINT								nBpp)
{ 
    //TRACE ("Performing horizontal scaling...\n"); 
    if (uResWidth == uSrcWidth)
    {   // No scaling required, just copy
        memcpy (pDst, pSrc, (nBpp>>3) * uSrcHeight * uSrcWidth);				
    }
    // Allocate and calculate the contributions
    LineContribType * Contrib = CalcContributions (uResWidth, uSrcWidth, double(uResWidth) / double(uSrcWidth)); 
    for (UINT u = 0; u < uResHeight; u++)
    {   // Step through rows
        if (NULL != m_Callback)
        {
            //
            // Progress and report callback supplied
            //
            if (!m_Callback (BYTE(double(u) / double (uResHeight) * 50.0)))
            {
                //
                // User wished to abort now
                //
                m_bCanceled = TRUE;
                FreeContributions (Contrib);  // Free contributions structure
                return;
            }
        }
                
				//MyOutputDebugString("row=%d\n",u);
        ScaleRow (  pSrc, 
                    uSrcWidth,
                    pDst,
                    uResWidth,
                    u,
                    Contrib,nBpp);    // Scale each row 
    }
    FreeContributions (Contrib);  // Free contributions structure
} 
 
template <class FilterClass>
void
C2PassScale<FilterClass>::
ScaleCol (  UCHAR           *pSrc, 
            UINT                uSrcWidth,
            UCHAR           *pRes, 
            UINT                uResWidth,
            UINT                uResHeight,
            UINT                uCol, 
            LineContribType    *Contrib,
						UINT								nBpp)
{ 
		if (nBpp==24)
		{
			for (UINT y = 0; y < uResHeight; y++) 
			{    // Loop through column
					BYTE r = 0;
					BYTE g = 0;
					BYTE b = 0;
					int iLeft = Contrib->ContribRow[y].Left;    // Retrieve left boundries
					int iRight = Contrib->ContribRow[y].Right;  // Retrieve right boundries
					for (int i = iLeft; i <= iRight; i++)
					{   // Scan between boundries
							// Accumulate weighted effect of each neighboring pixel
							UCHAR *pCurSrc = &pSrc[i * uSrcWidth * 3 + uCol*3];
							r += BYTE(Contrib->ContribRow[y].Weights[i-iLeft] * (double)((pCurSrc[0])));
							g += BYTE(Contrib->ContribRow[y].Weights[i-iLeft] * (double)((pCurSrc[1])));
							b += BYTE(Contrib->ContribRow[y].Weights[i-iLeft] * (double)((pCurSrc[2])));
					}
					pRes[y * uResWidth *3 + uCol*3 + 0] = r;   // Place result in destination pixel
					pRes[y * uResWidth *3 + uCol*3 + 1] = g;   // Place result in destination pixel
					pRes[y * uResWidth *3 + uCol*3 + 2] = b;   // Place result in destination pixel
			}
		}
		else
		if (nBpp==32)
		{
			for (UINT y = 0; y < uResHeight; y++) 
			{    // Loop through column
					BYTE r = 0;
					BYTE g = 0;
					BYTE b = 0;
					BYTE a = 0;
					int iLeft = Contrib->ContribRow[y].Left;    // Retrieve left boundries
					int iRight = Contrib->ContribRow[y].Right;  // Retrieve right boundries
					for (int i = iLeft; i <= iRight; i++)
					{   // Scan between boundries
							// Accumulate weighted effect of each neighboring pixel
							UCHAR *pCurSrc = &pSrc[i * uSrcWidth *4 + uCol*4];
							r += BYTE(Contrib->ContribRow[y].Weights[i-iLeft] * (double)(pCurSrc[0]));
							g += BYTE(Contrib->ContribRow[y].Weights[i-iLeft] * (double)(pCurSrc[1]));
							b += BYTE(Contrib->ContribRow[y].Weights[i-iLeft] * (double)(pCurSrc[2]));
							a += BYTE(Contrib->ContribRow[y].Weights[i-iLeft] * (double)(pCurSrc[3]));
					}
					pRes[y * uResWidth *4 + uCol*4 +0] = r;   // Place result in destination pixel
					pRes[y * uResWidth *4 + uCol*4 +1] = g;   // Place result in destination pixel
					pRes[y * uResWidth *4 + uCol*4 +2] = b;   // Place result in destination pixel
					pRes[y * uResWidth *4 + uCol*4 +3] = a;   // Place result in destination pixel
			}
		}
} 
 

template <class FilterClass>
void
C2PassScale<FilterClass>::
VertScale ( UCHAR           *pSrc, 
            UINT                uSrcWidth,
            UINT                uSrcHeight,
            UCHAR           *pDst, 
            UINT                uResWidth,
            UINT                uResHeight,
						UINT								nBpp)
{ 
    //TRACE ("Performing vertical scaling..."); 

    if (uSrcHeight == uResHeight)
    {   // No scaling required, just copy
        memcpy (pDst, pSrc, sizeof (UCHAR) * uSrcHeight * uSrcWidth);
    }
    // Allocate and calculate the contributions
    LineContribType * Contrib = CalcContributions (uResHeight, uSrcHeight, double(uResHeight) / double(uSrcHeight)); 
    for (UINT u = 0; u < uResWidth; u++)
    {   // Step through columns
        if (NULL != m_Callback)
        {
            //
            // Progress and report callback supplied
            //
            if (!m_Callback (BYTE(double(u) / double (uResWidth) * 50.0) + 50))
            {
                //
                // User wished to abort now
                //
                m_bCanceled = TRUE;
                FreeContributions (Contrib);  // Free contributions structure
                return;
            }
        }
        ScaleCol (  pSrc,
                    uSrcWidth,
                    pDst,
                    uResWidth,
                    uResHeight,
                    u,
                    Contrib,nBpp);   // Scale each column
    }
    FreeContributions (Contrib);     // Free contributions structure
} 

template <class FilterClass>
UCHAR *
C2PassScale<FilterClass>::
AllocAndScale ( 
    UCHAR   *pOrigImage, 
    UINT        uOrigWidth, 
    UINT        uOrigHeight, 
    UINT        uNewWidth, 
    UINT        uNewHeight,
		UINT				nBpp)
{ 
    // Scale source image horizontally into temporary image
    m_bCanceled = FALSE;
    UCHAR *pTemp = new UCHAR [uNewWidth * uOrigHeight];
    HorizScale (pOrigImage, 
                uOrigWidth,
                uOrigHeight,
                pTemp,
                uNewWidth,
                uOrigHeight,
								nBpp);
    if (m_bCanceled)
    {
        delete [] pTemp;
        return NULL;
    }
    // Scale temporary image vertically into result image    
    UCHAR *pRes = new UCHAR [uNewWidth * uNewHeight];
    VertScale ( pTemp,
                uNewWidth,
                uOrigHeight,
                pRes,
                uNewWidth,
                uNewHeight,
								nBpp);
    if (m_bCanceled)
    {
        delete [] pTemp;
        delete [] pRes;
        return NULL;
    }
    delete [] pTemp;
    return pRes;
} 

template <class FilterClass>
UCHAR *
C2PassScale<FilterClass>::
Scale ( 
    UCHAR   *pOrigImage, 
    UINT        uOrigWidth, 
    UINT        uOrigHeight, 
    UCHAR   *pDstImage, 
    UINT        uNewWidth, 
    UINT        uNewHeight,
		UINT				nBpp)
{ 
    // Scale source image horizontally into temporary image
    m_bCanceled = FALSE;
    UCHAR *pTemp = new UCHAR [uNewWidth * uOrigHeight * (nBpp>>3)];
    HorizScale (pOrigImage, 
                uOrigWidth,
                uOrigHeight,
                pTemp,
                uNewWidth,
                uOrigHeight,nBpp);
    if (m_bCanceled)
    {
        delete [] pTemp;
        return NULL;
    }

    // Scale temporary image vertically into result image    
    VertScale ( pTemp,
                uNewWidth,
                uOrigHeight,
                pDstImage,
                uNewWidth,
                uNewHeight,nBpp);
    delete [] pTemp;
    if (m_bCanceled)
    {
        return NULL;
    }
    return pDstImage;
} 


#endif //   _2_PASS_SCALE_H_
