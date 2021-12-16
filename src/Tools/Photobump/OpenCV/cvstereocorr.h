
#ifndef CV_STEREO_CORR_H
#define CV_STEREO_CORR_H

/****************************************************************************************\
*                        Stereo Correlation with fixed window size                       *
\****************************************************************************************/

typedef short unsigned int cvSDTYPE; // 16bit unsigned integer...
typedef struct cvIdxDispRange {
    int idx, d0, d1;
};

class CvStereoCorrelation
{
public:
    CVAUX_DLL_ENTRY CvStereoCorrelation();
    CVAUX_DLL_ENTRY ~CvStereoCorrelation();
    
    // configuration
    CVAUX_DLL_ENTRY void setSourceImages( IplImage* img_left,
					  IplImage* img_right );
    CVAUX_DLL_ENTRY void setDisparityRange( int min,
					    int max );
    CVAUX_DLL_ENTRY void setWindowSize( unsigned int wx,
					unsigned int wy );
    CVAUX_DLL_ENTRY void setMinUniqueFactor( float lambda );
    
    // queries
    CVAUX_DLL_ENTRY int getMinDisparity() const;
    CVAUX_DLL_ENTRY int getMaxDisparity() const;
    CVAUX_DLL_ENTRY unsigned int getWindowSizeX() const;
    CVAUX_DLL_ENTRY unsigned int getWindowSizeY() const;
    CVAUX_DLL_ENTRY float getMinUniqueFactor() const;
    
    // actual calculation
    CVAUX_DLL_ENTRY void doCorrelation();
    
    // result access
    CVAUX_DLL_ENTRY float* disparityLine( int y ) const;
    CVAUX_DLL_ENTRY float disparity( int x, int y ) const;
    CVAUX_DLL_ENTRY IplImage* disparityImage();
    
    // value of pixels without disparity estimate
    static const float NODISP;
    
protected:
    int            w, h, winX, winY, dMin, dMax;
    float          minUniqueFactor;
    IplImage       *leftImage, *rightImage;
    int            mapLine, mapSize, mapSizeOld;
    float          *_dispMap, *dispMap, *rdisp;
    int            sdLine, sdSlice, sdSize,
	           sdSizeOld, wOld, dMinOld, dMaxOld;
    cvSDTYPE       *SD;
    cvIdxDispRange *LRlut, *RLlut;
    IplImage*      dispImg;
    bool           dispImgRecalc;

    void resizeBuffers();
    void correlate8uC1();
    void correlate8uC3();
    void correlate8uC4();
};

#endif