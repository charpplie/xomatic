/*M///////////////////////////////////////////////////////////////////////////////////////
  //
  //  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
  //
  //  By downloading, copying, installing or using the software you agree to this license.
  //  If you do not agree to this license, do not download, install,
  //  copy or use the software.
  //
  //
  //                        Intel License Agreement
  //                For Open Source Computer Vision Library
  //
  // Copyright (C) 2000, Intel Corporation, all rights reserved.
  // Third party copyrights are property of their respective owners.
  //
  // Redistribution and use in source and binary forms, with or without modification,
  // are permitted provided that the following conditions are met:
  //
  //   * Redistribution's of source code must retain the above copyright notice,
  //     this list of conditions and the following disclaimer.
  //
  //   * Redistribution's in binary form must reproduce the above copyright notice,
  //     this list of conditions and the following disclaimer in the documentation
  //     and/or other materials provided with the distribution.
  //
  //   * The name of Intel Corporation may not be used to endorse or promote products
  //     derived from this software without specific prior written permission.
  //
  // This software is provided by the copyright holders and contributors "as is" and
  // any express or implied warranties, including, but not limited to, the implied
  // warranties of merchantability and fitness for a particular purpose are disclaimed.
  // In no event shall the Intel Corporation or contributors be liable for any direct,
  // indirect, incidental, special, exemplary, or consequential damages
  // (including, but not limited to, procurement of substitute goods or services;
  // loss of use, data, or profits; or business interruption) however caused
  // and on any theory of liability, whether in contract, strict liability,
  // or tort (including negligence or otherwise) arising in any way out of
  // the use of this software, even if advised of the possibility of such damage.
  //
  //M*/

#include <math.h>
#include <iostream.h>

#include "cvaux.h"
#include "_cverror.h"

#if _MSC_VER >= 1000
#pragma warning( disable: 4244 )
#endif

const cvSDTYPE SDTYPEMAX = 0xffffU;
const float CvStereoCorrelation::NODISP = -1234567.0;

typedef struct {
    uchar R, G, B;
} myRGB;

CvStereoCorrelation::CvStereoCorrelation()
    : w(0), h(0), winX(0), winY(0), dMin(0), dMax(0),
      leftImage(0), rightImage(0),
      mapLine(0), mapSize(0), mapSizeOld(0),
      _dispMap(0), dispMap(0), rdisp(0),
      sdLine(0), sdSlice(0), sdSize(0),
      sdSizeOld(0), wOld(0), dMinOld(0), dMaxOld(0),
      SD(0), LRlut(0), RLlut(0),
      dispImg(0), dispImgRecalc(false)
{
}

CvStereoCorrelation::~CvStereoCorrelation()
{
    delete [] _dispMap;
    delete [] rdisp;
    delete [] SD;
    delete [] LRlut;
    delete [] RLlut;
    cvReleaseImage( &dispImg );
}

void CvStereoCorrelation::setSourceImages( IplImage* img_left,
					   IplImage* img_right )
{
    CV_FUNCNAME("CvStereoCorrelation::setSourceImages");
    __BEGIN__;

    if( !img_left || !img_right )
	//CV_ERROR_FROM_STATUS( CV_NULLPTR_ERR );
	// [marco]
	CV_ERROR( CV_StsNullPtr, "null images" );

    if ( !CV_ARE_SIZES_EQ( img_left, img_right ) ||
	 img_left->nChannels != img_right->nChannels ||
	 img_left->depth != IPL_DEPTH_8U || 
	 img_right->depth != IPL_DEPTH_8U )
	 // [marco]
	//CV_ERROR_FROM_STATUS( CV_UNMATCHED_FORMATS_ERR );
	CV_ERROR( CV_BadDepth, "source images must be 8u" );

    leftImage = img_left;
    rightImage = img_right;
    w = img_left->width;
    h = img_left->height;

    cvReleaseImage( &dispImg );
    CvSize sz;
    sz.width = w;
    sz.height = h;
    dispImg = cvCreateImage( sz, IPL_DEPTH_8U, 3 );
    dispImgRecalc = true;

    __CLEANUP__;
    __END__;
}

void CvStereoCorrelation::setDisparityRange( int min,
					     int max )
{
    CV_FUNCNAME("CvStereoCorrelation::setDisparityRange");
    __BEGIN__;
    if ( min < max ) {
	dMin = min;
	dMax = max;
    } else
	// [marco]
	//CV_ERROR_FROM_STATUS( CV_BADARG_ERR );
	CV_ERROR(CV_StsBadArg,"max>=min");
    __CLEANUP__;
    __END__;
}

void CvStereoCorrelation::setWindowSize( unsigned int wx,
					 unsigned int wy )
{
    CV_FUNCNAME("CvStereoCorrelation::setWindowSize");
    __BEGIN__;
    // window sizes must not be even numbers
    if ( ( wx & 1 ) && ( wy & 1 ) ) { 
	winX = wx;
	winY = wy;
    } else
	// [marco]
	//CV_ERROR_FROM_STATUS( CV_BADARG_ERR );
	CV_ERROR(CV_StsBadArg,"window sizes must not be even numbers");
    __CLEANUP__;
    __END__;
}

void CvStereoCorrelation::setMinUniqueFactor( float lambda )
{
    CV_FUNCNAME("CvStereoCorrelation::setMinUniqueFactor");
    __BEGIN__;
    if ( lambda >= 0.0 )
	minUniqueFactor = 1.0f + lambda;
    else
	// [marco] 	
	//CV_ERROR_FROM_STATUS( CV_BADARG_ERR );
	CV_ERROR(CV_StsBadArg,"lambda<0");
    __CLEANUP__;
    __END__;
}

CV_INLINE
int CvStereoCorrelation::getMinDisparity() const
{
    return dMin;
}

CV_INLINE
int CvStereoCorrelation::getMaxDisparity() const
{
    return dMax;
}

CV_INLINE
unsigned int CvStereoCorrelation::getWindowSizeX() const
{
    return winX;
}

CV_INLINE
unsigned int CvStereoCorrelation::getWindowSizeY() const
{
    return winY;
}

CV_INLINE
float CvStereoCorrelation::getMinUniqueFactor() const
{
    return ( minUniqueFactor - 1.0f );
}

void CvStereoCorrelation::doCorrelation()
{
    int wx2 = (winX-1)/2, wy2 = (winY-1)/2;
    int jump = w-winX-1, pad, y;
    float *disp, *dEnd;
	
    CV_FUNCNAME("CvStereoCorrelation::doCorrelation");
    __BEGIN__;

    resizeBuffers();
    pad = mapLine-w;
    disp = dispMap;
    dEnd = disp + wy2*mapLine + wx2;
    switch ( leftImage->nChannels ) {
    case 1:
	correlate8uC1();
	break;
    case 3:
	correlate8uC3();
	break;
    case 4:
	correlate8uC4();
	break;
    default:
	CV_ERROR(CV_StsBadArg,"");
    }
	
    // reset unreliably matched border
    for( ; disp != dEnd; disp++ )
	*disp = NODISP;
    for( y = 0; y < h-winY; y++ ) {
	disp += jump;
	dEnd = disp + winX+1 + pad;
	for( ; disp != dEnd; disp++ )
	    *disp = NODISP;
    }
    dEnd = dispMap + mapLine*h - 1;
    for( ; disp != dEnd; disp++ )
	*disp = NODISP;
	
    dispImgRecalc = true;

    __CLEANUP__;
    __END__;
}

float* CvStereoCorrelation::disparityLine( int y ) const
{
    return ( dispMap + y * mapLine );
}

float CvStereoCorrelation::disparity( int x, int y ) const
{
    return dispMap[ x + y * mapLine ];
}

IplImage* CvStereoCorrelation::disparityImage()
{
    if ( dispImgRecalc ) {
	int           x, y;
	float*        disp;
	unsigned char gray, *imgptr;
	float         lowest = 1e6, highest = -1e6;

	for( y = 0; y < h; y++ ) {
	    disp = disparityLine( y );
	    for( x = 0; x < w; x++, disp++ )
		if ( *disp != NODISP ) {
		    if ( *disp < lowest ) lowest = *disp;
		    if ( *disp > highest ) highest = *disp;
		}
	}
	for( y = 0; y < h; y++ ) {
	    disp = disparityLine( y );
	    imgptr = (unsigned char*)(dispImg->imageData + y * dispImg->widthStep);
	    for( x = 0; x < w; x++, disp++ ) 
		if ( *disp == NODISP ) {
		    *imgptr++ = 0;
		    *imgptr++ = 0;
		    *imgptr++ = 255;
		} else {
		    gray = (unsigned char)(255.0*(*disp-lowest)/(highest-lowest));
		    *imgptr++ = gray;
		    *imgptr++ = gray;
		    *imgptr++ = gray;
		}
	}
	dispImgRecalc = false;
    }
    return dispImg;
}

void CvStereoCorrelation::resizeBuffers()
{
    int x;

    // disparity map (receives results)
    mapLine = (w + 8) & (~7L);
    mapSize = mapLine * h + ((winX+8+(winY-1)/2*mapLine)&~7L);
    if ( !_dispMap || 
	 ( mapSize > mapSizeOld ) ||
	 ( mapSize < ( mapSizeOld / 2 ) ) ) {
	delete [] _dispMap;
	_dispMap = new float [ mapSize ];
	dispMap = _dispMap + winX + 1 + (winY-1)/2*mapLine;
	mapSizeOld = mapSize;
    }

    // disparity-space volume
    sdLine = (dMax-dMin+1+7)&(~3L);
    sdSlice = sdLine*(w+winX+1);
    sdSize = sdSlice*(winY+2);
    if ( !SD || 
	 ( sdSize > sdSizeOld ) ||
	 ( sdSize < ( sdSizeOld / 2 ) ) ) {
	delete [] SD;
	SD = new cvSDTYPE[ sdSize ];
	sdSizeOld = sdSize;
    }
    if ( wOld != w ||
	 dMin != dMinOld || dMax != dMaxOld ) {
	int dispMin = dMinOld = dMin;
	int dispMax = dMaxOld = dMax;
	int d0, d1;
	
	delete [] rdisp;
	rdisp = new float[w];

	delete [] LRlut;
	delete [] RLlut;
	LRlut = new cvIdxDispRange[w];
	RLlut = new cvIdxDispRange[w];
	
	for( x = 0; x < w; x++ ) {
	    // left-to-right
	    // assume we're in part II
	    d0 = dispMin;
	    d1 = dispMax;
	    if ( x < -dispMin ) { // part I
		d0 = -x;
		if ( d0 > dispMax ) // nothing to do for this x
		    d1 = d0-1;
	    }
	    if ( x > w-1-dispMax ) { // part III
		d1 = w-1-x;
		if ( d1 < dispMin ) // nothing to do
		    d0 = d1+1;
	    }
	    LRlut[x].idx = x*sdLine + d0 - dispMin; // index in slice
	    LRlut[x].d0 = d0;                       // first disparity
	    LRlut[x].d1 = d1;                       // last disparity
	    // right-to-left
	    // again, start with assuming part II
	    d0 = dispMin;
	    d1 = dispMax;
	    if ( x-d1 < 0 ) { // part I
		d1 = x;
		if ( x-d0 < 0 ) // nothing to do
		    d0 = d1+1;
	    }
	    if ( x-d0 > w-1 ) { //part III
		d0 = 1+x-w;
		if ( x-d1 > w-1 ) // nothing to do
		    d1 = d0-1;
	    }
	    RLlut[x].idx = (x-d0)*sdLine + d0 - dispMin;
	    RLlut[x].d0 = d0;
	    RLlut[x].d1 = d1;
	}
	wOld = w;
    }
}

void CvStereoCorrelation::correlate8uC1()
{
    cvSDTYPE      *sdSliceBase, *sd;
    cvSDTYPE      *sdSliceMwyBase, *sdMwy;
    cvSDTYPE      *sdSliceMwyM1Base, *sdMwyM1;
    float         *disp, rdpos;
    int           x, y, d, dpos = 0, rdposInt;
    int           pad = mapLine - w;
    cvSDTYPE      sdVal, sdVal2, sdVal3, *sdPos;
    cvSDTYPE      ssdsum;
    unsigned int  maxclip = MIN( 768, (int)(ceil(65535.0/(winX*winY))) ); 
    float         fm1, fp1, rdposFrac;
    double        rdpi;
    unsigned char *leftPixel, *rightPixel, *rpStart, *rpStop;
 
    memset( SD, 0, sdSize * sizeof( cvSDTYPE ) );
  
    sdSliceBase = SD + sdLine*(winX+1);
    sdSliceMwyM1Base = sdSliceBase + sdSlice - sdLine*winX;
    sdSliceMwyBase = sdSliceBase + 2 * sdSlice - sdLine*winX;

    disp = dispMap-(winX-1)/2 - (winY-1)*mapLine/2;

    for( y = 0; y < h; y++,
	     disp += pad,
	     sdSliceBase += sdSlice,
	     sdSliceMwyBase += sdSlice,
	     sdSliceMwyM1Base += sdSlice ) {

	// wrap pointers back into ringbuffer if necessary
	if ( sdSliceBase >= ( SD + sdSize ) )
	    sdSliceBase -= sdSize;
	if ( sdSliceMwyBase >= ( SD + sdSize ) )
	    sdSliceMwyBase -= sdSize;
	if ( sdSliceMwyM1Base >= ( SD + sdSize ) )
	    sdSliceMwyM1Base -= sdSize;

	memset( sdSliceBase-sdLine*(winX+1), 0, sdSlice*sizeof(cvSDTYPE) );
	memset( rdisp, 0, w * sizeof(float) );

	leftPixel = (unsigned char*)leftImage->imageData + y * leftImage->widthStep;

	for( x = 0; x < w; x++, disp++, leftPixel++ ) {
	    if ( LRlut[x].d1 <= LRlut[x].d0 ) {
		*disp = CvStereoCorrelation::NODISP;
		continue;
	    }
	    rightPixel = rpStart = (unsigned char*)(rightImage->imageData + 
						    y * rightImage->widthStep) +
		x + LRlut[x].d0;
	    rpStop = rightPixel + LRlut[x].d1 - LRlut[x].d0;

	    sd = sdSliceBase + LRlut[x].idx;
	    sdMwy = sdSliceMwyBase + LRlut[x].idx;
	    sdMwyM1 = sdSliceMwyM1Base + LRlut[x].idx;

	    sdVal = SDTYPEMAX;
	    sdVal2 = sdVal3 = 0;
	    for( ; rightPixel <= rpStop; rightPixel++,
		     sd++,
		     sdMwy++,
		     sdMwyM1++ ) {
		ssdsum  = abs( *leftPixel - *rightPixel );
		if ( ssdsum > maxclip )
		    ssdsum = maxclip;
		*sd = (cvSDTYPE)ssdsum;
		ssdsum += *( sd - sdLine*(winX+1) ) - *( sd - sdLine*winX ); 
		*( sd - sdLine*winX ) = (cvSDTYPE)ssdsum;
		ssdsum += *sdMwyM1 - *sdMwy;
		*sdMwy = (cvSDTYPE)ssdsum;
		if ( ssdsum <= sdVal ) {
		    sdVal3 = sdVal2;
		    dpos = (int)(rightPixel);
		    sdVal2 = sdVal;
		    sdVal = ssdsum;
		}
	    }
	    dpos = (unsigned char*)dpos - rpStart;
	    sd = sdSliceMwyBase + LRlut[x].idx + dpos;
	    dpos += LRlut[x].d0;
	    if ( (dpos <= LRlut[x].d0) || (dpos >= LRlut[x].d1) || 
		 (float)sdVal3 <= (minUniqueFactor * (float)sdVal)  )
		*disp = CvStereoCorrelation::NODISP;
	    else {
		fm1 = *(sd-1);
		fp1 = *(sd+1);
		*disp = dpos + (fm1-fp1)/(2.0f*(fm1-2.0f*(float)sdVal+fp1));
	    }
	} // for x, L->R

	// R -> L
	for( x = 0; x < w; x++ ) {
	    if ( RLlut[x].d1 <= RLlut[x].d0 ) {
		rdisp[x] = CvStereoCorrelation::NODISP;
		continue;
	    }
	    dpos = -1;
	    sdVal = SDTYPEMAX;
	    sdVal2 = sdVal3 = 0;
	    sd = sdPos = sdSliceMwyBase + RLlut[x].idx;
	    for( d = RLlut[x].d0; d <= RLlut[x].d1; d++, sd -= sdLine-1 )
		if ( *sd <= sdVal ) {
		    sdVal3 = sdVal2;
		    dpos = d;
		    sdVal2 = sdVal;
		    sdPos = sd;
		    sdVal = *sd;
		}
	    if ( (dpos <= RLlut[x].d0) ||
		 (dpos >= RLlut[x].d1) ||
		 (float)sdVal3 <= (minUniqueFactor * (float)sdVal) )
		rdisp[x] = CvStereoCorrelation::NODISP;
	    else {
		fm1 = *(sdPos+sdLine-1);
		fp1 = *(sdPos-sdLine+1);
		rdisp[x] = dpos + (fm1-fp1)/(2.0f*(fm1-2.0f*(float)sdVal+fp1));
	    }
	} // end R->L

	disp -= w;

	// combine
	for( x = 0; x < w; x++, disp++ )
	    if ( *disp != CvStereoCorrelation::NODISP ) {
		rdposFrac = (float)modf( x + *disp, &rdpi );
		rdposInt = (int)rdpi;
		rdpos = (1.0f-rdposFrac)*rdisp[rdposInt] + rdposFrac*rdisp[rdposInt+1];
		if ( rdisp[rdposInt] == CvStereoCorrelation::NODISP || 
		     rdisp[rdposInt+1] == CvStereoCorrelation::NODISP || 
		     fabs( rdpos - *disp ) > 1.0 )
		    *disp = CvStereoCorrelation::NODISP;
		else
		    *disp = 0.5f * ( *disp + rdpos );
	    } // end combine
    } // for y
} // correlate8uC1()

void CvStereoCorrelation::correlate8uC3()
{
    cvSDTYPE     *sdSliceBase, *sd;
    cvSDTYPE     *sdSliceMwyBase, *sdMwy;
    cvSDTYPE     *sdSliceMwyM1Base, *sdMwyM1;
    float        *disp, rdpos;
    int          x, y, d, dpos = 0, rdposInt;
    int          pad = mapLine - w;
    cvSDTYPE     sdVal, sdVal2, sdVal3, *sdPos;
    cvSDTYPE     ssdsum;
    unsigned int maxclip = MIN( 768, (int)(ceil(65535.0/(winX*winY))) ); 
    float        fm1, fp1, rdposFrac;
    double       rdpi;
    myRGB        *leftPixel, *rightPixel, *rpStart, *rpStop;
 
    memset( SD, 0, sdSize * sizeof( cvSDTYPE ) );
  
    sdSliceBase = SD + sdLine*(winX+1);
    sdSliceMwyM1Base = sdSliceBase + sdSlice - sdLine*winX;
    sdSliceMwyBase = sdSliceBase + 2 * sdSlice - sdLine*winX;

    disp = dispMap-(winX-1)/2 - (winY-1)*mapLine/2;

    for( y = 0; y < h; y++,
	     disp += pad,
	     sdSliceBase += sdSlice,
	     sdSliceMwyBase += sdSlice,
	     sdSliceMwyM1Base += sdSlice ) {

	// wrap pointers back into ringbuffer if necessary
	if ( sdSliceBase >= ( SD + sdSize ) )
	    sdSliceBase -= sdSize;
	if ( sdSliceMwyBase >= ( SD + sdSize ) )
	    sdSliceMwyBase -= sdSize;
	if ( sdSliceMwyM1Base >= ( SD + sdSize ) )
	    sdSliceMwyM1Base -= sdSize;

	memset( sdSliceBase-sdLine*(winX+1), 0, sdSlice*sizeof(cvSDTYPE) );
	memset( rdisp, 0, w * sizeof(float) );

	leftPixel = (myRGB*)(leftImage->imageData + y * leftImage->widthStep);

	for( x = 0; x < w; x++, disp++, leftPixel++ ) {
	    if ( LRlut[x].d1 <= LRlut[x].d0 ) {
		*disp = CvStereoCorrelation::NODISP;
		continue;
	    }
	    rightPixel = rpStart = (myRGB*)(rightImage->imageData + 
					    y * rightImage->widthStep) +
		x + LRlut[x].d0;
	    rpStop = rightPixel + LRlut[x].d1 - LRlut[x].d0;

	    sd = sdSliceBase + LRlut[x].idx;
	    sdMwy = sdSliceMwyBase + LRlut[x].idx;
	    sdMwyM1 = sdSliceMwyM1Base + LRlut[x].idx;

	    sdVal = SDTYPEMAX;
	    sdVal2 = sdVal3 = 0;
	    for( ; rightPixel <= rpStop; rightPixel++,
		     sd++,
		     sdMwy++,
		     sdMwyM1++ ) {
		ssdsum  = abs( leftPixel->R - rightPixel->R );
		ssdsum += abs( leftPixel->G - rightPixel->G );
		ssdsum += abs( leftPixel->B - rightPixel->B );
		if ( ssdsum > maxclip )
		    ssdsum = maxclip;
		*sd = (cvSDTYPE)ssdsum;
		ssdsum += *( sd - sdLine*(winX+1) ) - *( sd - sdLine*winX ); 
		*( sd - sdLine*winX ) = (cvSDTYPE)ssdsum;
		ssdsum += *sdMwyM1 - *sdMwy;
		*sdMwy = (cvSDTYPE)ssdsum;
		if ( ssdsum <= sdVal ) {
		    sdVal3 = sdVal2;
		    dpos = (int)(rightPixel);
		    sdVal2 = sdVal;
		    sdVal = ssdsum;
		}
	    }
	    dpos = (myRGB*)dpos - rpStart;
	    sd = sdSliceMwyBase + LRlut[x].idx + dpos;
	    dpos += LRlut[x].d0;
	    if ( (dpos <= LRlut[x].d0) || (dpos >= LRlut[x].d1) || 
		 (float)sdVal3 <= (minUniqueFactor * (float)sdVal)  )
		*disp = CvStereoCorrelation::NODISP;
	    else {
		fm1 = *(sd-1);
		fp1 = *(sd+1);
		*disp = dpos + (fm1-fp1)/(2.0f*(fm1-2.0f*(float)sdVal+fp1));
	    }
	} // for x, L->R

	// R -> L
	for( x = 0; x < w; x++ ) {
	    if ( RLlut[x].d1 <= RLlut[x].d0 ) {
		rdisp[x] = CvStereoCorrelation::NODISP;
		continue;
	    }
	    dpos = -1;
	    sdVal = SDTYPEMAX;
	    sdVal2 = sdVal3 = 0;
	    sd = sdPos = sdSliceMwyBase + RLlut[x].idx;
	    for( d = RLlut[x].d0; d <= RLlut[x].d1; d++, sd -= sdLine-1 )
		if ( *sd <= sdVal ) {
		    sdVal3 = sdVal2;
		    dpos = d;
		    sdVal2 = sdVal;
		    sdPos = sd;
		    sdVal = *sd;
		}
	    if ( (dpos <= RLlut[x].d0) ||
		 (dpos >= RLlut[x].d1) ||
		 (float)sdVal3 <= (minUniqueFactor * (float)sdVal) )
		rdisp[x] = CvStereoCorrelation::NODISP;
	    else {
		fm1 = *(sdPos+sdLine-1);
		fp1 = *(sdPos-sdLine+1);
		rdisp[x] = dpos + (fm1-fp1)/(2.0f*(fm1-2.0f*(float)sdVal+fp1));
	    }
	} // end R->L

	disp -= w;

	// combine
	for( x = 0; x < w; x++, disp++ )
	    if ( *disp != CvStereoCorrelation::NODISP ) {
		rdposFrac = (float)modf( x + *disp, &rdpi );
		rdposInt = (int)rdpi;
		rdpos = (1.0f-rdposFrac)*rdisp[rdposInt] + rdposFrac*rdisp[rdposInt+1];
		if ( rdisp[rdposInt] == CvStereoCorrelation::NODISP || 
		     rdisp[rdposInt+1] == CvStereoCorrelation::NODISP || 
		     fabs( rdpos - *disp ) > 1.0 )
		    *disp = CvStereoCorrelation::NODISP;
		else
		    *disp = 0.5f * ( *disp + rdpos );
	    } // end combine
    } // for y
} // correlate8uC3()

void CvStereoCorrelation::correlate8uC4()
{
    cvSDTYPE     *sdSliceBase, *sd;
    cvSDTYPE     *sdSliceMwyBase, *sdMwy;
    cvSDTYPE     *sdSliceMwyM1Base, *sdMwyM1;
    float        *disp, rdpos;
    int          x, y, d, dpos = 0, rdposInt;
    int          pad = mapLine - w;
    cvSDTYPE     sdVal, sdVal2, sdVal3, *sdPos;
    unsigned int *leftPixel, *rightPixel, *rpStart, *rpStop;
    cvSDTYPE     ssdsum;
    int          lpix, rpix, diff;
    unsigned int maxclip = MIN( 768, (int)(ceil(65535.0/(winX*winY))) ); 
    float        fm1, fp1, rdposFrac;
    double       rdpi;
 
    memset( SD, 0, sdSize * sizeof( cvSDTYPE ) );
  
    sdSliceBase = SD + sdLine*(winX+1);
    sdSliceMwyM1Base = sdSliceBase + sdSlice - sdLine*winX;
    sdSliceMwyBase = sdSliceBase + 2 * sdSlice - sdLine*winX;

    disp = dispMap-(winX-1)/2 - (winY-1)*mapLine/2;

    for( y = 0; y < h; y++,
	     disp += pad,
	     sdSliceBase += sdSlice,
	     sdSliceMwyBase += sdSlice,
	     sdSliceMwyM1Base += sdSlice ) {

	// wrap pointers back into ringbuffer if necessary
	if ( sdSliceBase >= ( SD + sdSize ) )
	    sdSliceBase -= sdSize;
	if ( sdSliceMwyBase >= ( SD + sdSize ) )
	    sdSliceMwyBase -= sdSize;
	if ( sdSliceMwyM1Base >= ( SD + sdSize ) )
	    sdSliceMwyM1Base -= sdSize;

	memset( sdSliceBase-sdLine*(winX+1), 0, sdSlice*sizeof(cvSDTYPE) );
	memset( rdisp, 0, w * sizeof(float) );

	leftPixel = (unsigned int *)(leftImage->imageData + y * leftImage->widthStep);

	for( x = 0; x < w; x++, disp++, leftPixel++ ) {
	    if ( LRlut[x].d1 <= LRlut[x].d0 ) {
		*disp = CvStereoCorrelation::NODISP;
		continue;
	    }
	    lpix = (int)*leftPixel;
	    rightPixel = rpStart = (unsigned int *)(rightImage->imageData + 
						    y * rightImage->widthStep) +
		x + LRlut[x].d0;
	    rpStop = rightPixel + LRlut[x].d1 - LRlut[x].d0;

	    sd = sdSliceBase + LRlut[x].idx;
	    sdMwy = sdSliceMwyBase + LRlut[x].idx;
	    sdMwyM1 = sdSliceMwyM1Base + LRlut[x].idx;

	    sdVal = SDTYPEMAX;
	    sdVal2 = sdVal3 = 0;
	    for( ; rightPixel <= rpStop; rightPixel++,
		     sd++,
		     sdMwy++,
		     sdMwyM1++ ) {
		rpix = (int)*rightPixel;
		diff = ( lpix & 0xff ) - ( rpix & 0xff );
		ssdsum = abs(diff);
		diff = ( ( lpix & 0xff00 ) - ( rpix & 0xff00 ) ) >> 8;
		ssdsum += abs(diff);
		diff = ( ( lpix & 0xff0000 ) - ( rpix & 0xff0000 ) ) >> 16;
		ssdsum += abs(diff);
		if ( ssdsum > maxclip )
		    ssdsum = maxclip;
		*sd = (cvSDTYPE)ssdsum;
		ssdsum += *( sd - sdLine*(winX+1) ) - *( sd - sdLine*winX ); 
		*( sd - sdLine*winX ) = (cvSDTYPE)ssdsum;
		ssdsum += *sdMwyM1 - *sdMwy;
		*sdMwy = (cvSDTYPE)ssdsum;
		if ( ssdsum <= sdVal ) {
		    sdVal3 = sdVal2;
		    dpos = (int)(rightPixel);
		    sdVal2 = sdVal;
		    sdVal = ssdsum;
		}
	    }
	    dpos = (unsigned int *)dpos - rpStart;
	    sd = sdSliceMwyBase + LRlut[x].idx + dpos;
	    dpos += LRlut[x].d0;
	    if ( (dpos <= LRlut[x].d0) || (dpos >= LRlut[x].d1) || 
		 (float)sdVal3 <= (minUniqueFactor * (float)sdVal)  )
		*disp = CvStereoCorrelation::NODISP;
	    else {
		fm1 = *(sd-1);
		fp1 = *(sd+1);
		*disp = dpos + (fm1-fp1)/(2.0f*(fm1-2.0f*(float)sdVal+fp1));
	    }
	} // for x, L->R

	// R -> L
	for( x = 0; x < w; x++ ) {
	    if ( RLlut[x].d1 <= RLlut[x].d0 ) {
		rdisp[x] = CvStereoCorrelation::NODISP;
		continue;
	    }
	    dpos = -1;
	    sdVal = SDTYPEMAX;
	    sdVal2 = sdVal3 = 0;
	    sd = sdPos = sdSliceMwyBase + RLlut[x].idx;
	    for( d = RLlut[x].d0; d <= RLlut[x].d1; d++, sd -= sdLine-1 )
		if ( *sd <= sdVal ) {
		    sdVal3 = sdVal2;
		    dpos = d;
		    sdVal2 = sdVal;
		    sdPos = sd;
		    sdVal = *sd;
		}
	    if ( (dpos <= RLlut[x].d0) ||
		 (dpos >= RLlut[x].d1) ||
		 (float)sdVal3 <= (minUniqueFactor * (float)sdVal) )
		rdisp[x] = CvStereoCorrelation::NODISP;
	    else {
		fm1 = *(sdPos+sdLine-1);
		fp1 = *(sdPos-sdLine+1);
		rdisp[x] = dpos + (fm1-fp1)/(2.0f*(fm1-2.0f*(float)sdVal+fp1));
	    }
	} // end R->L

	disp -= w;

	// combine
	for( x = 0; x < w; x++, disp++ )
	    if ( *disp != CvStereoCorrelation::NODISP ) {
		rdposFrac = (float)modf( x + *disp, &rdpi );
		rdposInt = (int)rdpi;
		rdpos = (1.0f-rdposFrac)*rdisp[rdposInt] + rdposFrac*rdisp[rdposInt+1];
		if ( rdisp[rdposInt] == CvStereoCorrelation::NODISP || 
		     rdisp[rdposInt+1] == CvStereoCorrelation::NODISP || 
		     fabs( rdpos - *disp ) > 1.0 )
		    *disp = CvStereoCorrelation::NODISP;
		else
		    *disp = 0.5f * ( *disp + rdpos );
	    } // end combine
    } // for y
} // correlate8UC4()
