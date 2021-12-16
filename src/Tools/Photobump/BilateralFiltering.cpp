

#include "StdAfx.h"
#include "photoimage.h"
#include "FreeImage.h" 
#include "Video.h"

#include "opencv/cv.h"
#include "opencv/highgui.h"

//////////////////////////////////////////////////////////////////////////
//IPCVAPI_IMPL( CvStatus,
     //icvBilateralFiltering_8u_CnR, ( uchar* src, int srcStep,
//                                     uchar* dst, int dstStep,
//                                     CvSize* roiSize, int* param, int /*stub*/ ))
void BilateralFiltering(double *src,double *dst,double fSigmaColor,double fSigmaSpace,int nChannels,int width,int height)
{ 
    //CvSize size = *roiSize;
		CvSize size = cvSize(width,height);

		int srcStep=width;
		int dstStep=width;
    
    int channels = nChannels; //param[0];
    double sigma_color = fSigmaColor; //param[1]; 
    double sigma_space = fSigmaSpace; //param[2];

    double i2sigma_color = 1./(sigma_color*sigma_color);
    double i2sigma_space = 1./(sigma_space*sigma_space); 

    double w;
    int deltas[8];
    double weight_tab[8];

		double mean1[3]={0,0,0};
    double mean0=1;
		int i=0, j=0;

		//dst[i] = (uchar)cvRound(mean1[0]/mean0);
        
#define INIT_C1\
            color = src[0]; \
            mean0 = 1; mean1[0] = color;

#define COLOR_DISTANCE_C1(c1, c2)\
            (c1 - c2)*(c1 - c2)
#define KERNEL_ELEMENT_C1(k)\
            temp_color = src[deltas[k]];\
            w = weight_tab[k] + COLOR_DISTANCE_C1(color, temp_color)*i2sigma_color;\
            w = 1./(w*w + 1); \
            mean0 += w;\
            mean1[0] += temp_color*w;

#define INIT_C3\
            mean0 = 1; mean1[0] = src[0];mean1[1] = src[1];mean1[2] = src[2];

#define UPDATE_OUTPUT_C1\
						dst[i] = mean1[0]/mean0;

#define COLOR_DISTANCE_C3(c1, c2)\
            ((c1[0] - c2[0])*(c1[0] - c2[0]) + \
             (c1[1] - c2[1])*(c1[1] - c2[1]) + \
             (c1[2] - c2[2])*(c1[2] - c2[2]))
#define KERNEL_ELEMENT_C3(k)\
            temp_color = src + deltas[k];\
            w = weight_tab[k] + COLOR_DISTANCE_C3(src, temp_color)*i2sigma_color;\
            w = 1./(w*w + 1); \
            mean0 += w;\
            mean1[0] += temp_color[0]*w; \
            mean1[1] += temp_color[1]*w; \
            mean1[2] += temp_color[2]*w;

#define UPDATE_OUTPUT_C3\
            mean0 = 1./mean0;\
            dst[i*3 + 0] = (uchar)cvRound(mean1[0]*mean0); \
            dst[i*3 + 1] = (uchar)cvRound(mean1[1]*mean0); \
            dst[i*3 + 2] = (uchar)cvRound(mean1[2]*mean0);

    CV_INIT_3X3_DELTAS( deltas, srcStep, channels );

    weight_tab[0] = weight_tab[2] = weight_tab[4] = weight_tab[6] = i2sigma_space;
    weight_tab[1] = weight_tab[3] = weight_tab[5] = weight_tab[7] = i2sigma_space*2;

    if( channels == 1 )
    {
        double color, temp_color;

        for( i = 0; i < size.width; i++, src++ )
        {
            INIT_C1;
            KERNEL_ELEMENT_C1(6);
            if( i > 0 )
            {
                KERNEL_ELEMENT_C1(5);
                KERNEL_ELEMENT_C1(4);
            }
            if( i < size.width - 1 )
            {
                KERNEL_ELEMENT_C1(7);
                KERNEL_ELEMENT_C1(0);
            }
            UPDATE_OUTPUT_C1;
        }

        src += srcStep - size.width;
        dst += dstStep;
    
        for( j = 1; j < size.height - 1; j++, dst += dstStep )
        {
            i = 0;
            INIT_C1;
            KERNEL_ELEMENT_C1(0);
            KERNEL_ELEMENT_C1(1);
            KERNEL_ELEMENT_C1(2);
            KERNEL_ELEMENT_C1(6);
            KERNEL_ELEMENT_C1(7);
            UPDATE_OUTPUT_C1;

            for( i = 1, src++; i < size.width - 1; i++, src++ )
            {
                INIT_C1;
                KERNEL_ELEMENT_C1(0);
                KERNEL_ELEMENT_C1(1);
                KERNEL_ELEMENT_C1(2);
                KERNEL_ELEMENT_C1(3);
                KERNEL_ELEMENT_C1(4);
                KERNEL_ELEMENT_C1(5);
                KERNEL_ELEMENT_C1(6);
                KERNEL_ELEMENT_C1(7);
                UPDATE_OUTPUT_C1;
            }

            INIT_C1;
            KERNEL_ELEMENT_C1(2);
            KERNEL_ELEMENT_C1(3);
            KERNEL_ELEMENT_C1(4);
            KERNEL_ELEMENT_C1(5);
            KERNEL_ELEMENT_C1(6);
            UPDATE_OUTPUT_C1;

            src += srcStep + 1 - size.width;
        }

        for( i = 0; i < size.width; i++, src++ )
        {
            INIT_C1;
            KERNEL_ELEMENT_C1(2);
            if( i > 0 )
            {
                KERNEL_ELEMENT_C1(3);
                KERNEL_ELEMENT_C1(4);
            }
            if( i < size.width - 1 )
            {
                KERNEL_ELEMENT_C1(1);
                KERNEL_ELEMENT_C1(0);
            }
            UPDATE_OUTPUT_C1;
        }
    }
    else
    {
        //uchar* temp_color;
				double* temp_color;

        if( channels != 3 )
            Break();
        
        for( i = 0; i < size.width; i++, src += 3 )
        {
            INIT_C3;
            KERNEL_ELEMENT_C3(6);
            if( i > 0 )
            {
                KERNEL_ELEMENT_C3(5);
                KERNEL_ELEMENT_C3(4);
            }
            if( i < size.width - 1 )
            {
                KERNEL_ELEMENT_C3(7);
                KERNEL_ELEMENT_C3(0);
            }
            UPDATE_OUTPUT_C3;
        }

        src += srcStep - size.width*3;
        dst += dstStep;
    
        for( j = 1; j < size.height - 1; j++, dst += dstStep )
        {
            i = 0;
            INIT_C3;
            KERNEL_ELEMENT_C3(0);
            KERNEL_ELEMENT_C3(1);
            KERNEL_ELEMENT_C3(2);
            KERNEL_ELEMENT_C3(6);
            KERNEL_ELEMENT_C3(7);
            UPDATE_OUTPUT_C3;

            for( i = 1, src += 3; i < size.width - 1; i++, src += 3 )
            {
                INIT_C3;
                KERNEL_ELEMENT_C3(0);
                KERNEL_ELEMENT_C3(1);
                KERNEL_ELEMENT_C3(2);
                KERNEL_ELEMENT_C3(3);
                KERNEL_ELEMENT_C3(4);
                KERNEL_ELEMENT_C3(5);
                KERNEL_ELEMENT_C3(6);
                KERNEL_ELEMENT_C3(7);
                UPDATE_OUTPUT_C3;
            }

            INIT_C3;
            KERNEL_ELEMENT_C3(2);
            KERNEL_ELEMENT_C3(3);
            KERNEL_ELEMENT_C3(4);
            KERNEL_ELEMENT_C3(5);
            KERNEL_ELEMENT_C3(6);
            UPDATE_OUTPUT_C3;

            src += srcStep + 3 - size.width*3;
        }

        for( i = 0; i < size.width; i++, src += 3 )
        {
            INIT_C3;
            KERNEL_ELEMENT_C3(2);
            if( i > 0 )
            {
                KERNEL_ELEMENT_C3(3);
                KERNEL_ELEMENT_C3(4);
            }
            if( i < size.width - 1 )
            {
                KERNEL_ELEMENT_C3(1);
                KERNEL_ELEMENT_C3(0);
            }
            UPDATE_OUTPUT_C3;
        }
    }
    
#undef INIT_C1
#undef KERNEL_ELEMENT_C1
#undef UPDATE_OUTPUT_C1
#undef INIT_C3
#undef KERNEL_ELEMENT_C3
#undef UPDATE_OUTPUT_C3
#undef COLOR_DISTANCE_C3
}
