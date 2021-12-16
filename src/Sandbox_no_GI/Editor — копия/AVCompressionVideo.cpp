////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2010.
// -------------------------------------------------------------------------
//  File name:   AV_CompressionVideo.cpp
//  Version:     v1.00
//  Created:     09/04/2010 by John Stewart.
//  Compilers:   
//  Description: Video Encoding Routines.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AVCompression.h"
#include "AVCompressionVideo.h"
#include "AVCompressionVideoTable.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAVVideoCompression Class routines
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Constructor
CAVVideoCompression::CAVVideoCompression()

{

	// Detect the type of SIMD available if any
	m_simdTypeAvail = NO_ACCEL_AVAIL;
	int flags = gEnv->pSystem->GetCPUFlags();
	if(flags & CPUF_SSE) m_simdTypeAvail = SSE_AVAIL;

	// Set the prev mQuant to an invalid value to force initial filling of quantization table
	m_prevmQuant = -1;  

}


// Destructor
CAVVideoCompression::~CAVVideoCompression()
{

}


////////////////////////////////////////////////////////////////////////////////////////
// Initializes the video encoder
//
//  Input parameters : 
//		startRow		:	starting row to work on
//		sectionHeight	:	number of pixel lines to work on
//		pSerialBuff		:	pointer to a buffer to use for the encoded data
//		serialBuffSize	:	size of the encoded data buffer
//		pRGBbuffer		:	address of the input frame buffer
//		pYUVbuffer		:	pointer to a frame buffer for use in internal calculations
//		pCurRefBuffer	:	pointer to a ref frame buffer if this is a P or B frame
//		pNxtRefBuffer	:	pointer to a ref frame buffer to use for the reconstructed frame
//		enocdedWidth	:	width of the encoded frame in pixels
//		encodedHeight	:	height of the encoded frame in pixels
//		mQuant			:	quantization parameter
//		videoCodecType	:   MPEG1 or MPEG2
//		inputFormat		:   YUV or RGB
//		frameType		:	I,P or B
//
//	Output : none
//
//
void CAVVideoCompression::InitVideoCompression(unsigned int startRow, unsigned int sectionHeight,uint8 *pSerialBuff, unsigned int serialBuffSize,uint8 *pRGBbuffer,
											   uint8 *pYUVbuffer, uint8 *pCurRefBuffer, uint8 *pNxtRefBuffer,unsigned int encodedWidth, unsigned int encodedHeight,int mQuant,
											   int videoPesID,unsigned int containerStyle,int videoCodecType,int inputFormat,int frameType)
{
	m_startRow = startRow;
	m_sectionHeight = sectionHeight;
	m_pInputFrame = pRGBbuffer;
	m_pOutputFrame = pYUVbuffer;
	m_encodedHeight = encodedHeight;
	m_encodedWidth = encodedWidth;
	m_MBWidth = encodedWidth >> MBHEIGHTSHIFT;
	m_mQuant = mQuant;
	m_videoPesID = videoPesID;
	m_containerStyle = containerStyle;
	m_videoCodecType = videoCodecType;
	m_inputFormat = inputFormat;
	m_frameType = frameType;
	m_pCurRefBuffer = pCurRefBuffer;
	m_pNxtRefBuffer = pNxtRefBuffer;

	m_fcodeHoriz = DEFAULTFCODE;
	m_fcodeVert = DEFAULTFCODE;

	if(m_videoCodecType == MPEG1_STYLE)
	{
		m_qScaleType = 0;
		m_intraVLCformat = 0;
		m_intraDcPred = INTRADCPRED8BIT;
		m_coeffLimit = MPEG1COEFFLIMIT;
	}
	else
	{
		m_qScaleType = 1;
		m_intraVLCformat = 1;
		m_intraDcPred = INTRADCPRED10BIT; 
		m_coeffLimit = MPEG2COEFFLIMIT;
	}



	m_dcPredShift = (DEFAULTDCMASK >> m_intraDcPred);

	// Initialize the serial write buffer that will be used to hold the encoded video
	m_SerialStore.Init(pSerialBuff,serialBuffSize,SERIALWRITE);

	m_functionType = COMPRESSVIDEO;



}

////////////////////////////////////////////////////////////////////////////////////////
// Initializes the video decoder
//
//  Input parameters : 
//		pOutputFrame	:	pointer to a frame buffer where decoded section is placed
//		pSerialBuff		:	pointer to a buffer that holds the encoded data
//		serialBuffSize	:	size of the encoded data buffer
//		sectionHeight	:	number of lines in the section
//		enocdedWidth	:	width of the encoded frame in pixels
//		encodedHeight	:	height of the encoded frame in pixels
//		mQuant			:	quantization parameter
//		videoCodecType	:	MPEG1 or MPEG2
//
//	Output : none
//
//
void CAVVideoCompression::InitVideoDeCompression(uint8 *pOutputFrame, uint8 *pSerialBuff, unsigned int serialBuffSize,unsigned int sectionHeight,unsigned int encodedWidth,
												 unsigned int encodedHeight,int mQuant,int videoCodecType)

{

	m_pOutputFrame = pOutputFrame;
	m_sectionHeight = sectionHeight;
	m_encodedHeight = encodedHeight;
	m_encodedWidth = encodedWidth;
	m_mQuant = mQuant;
	m_videoCodecType = videoCodecType;


	if(m_videoCodecType == MPEG1_STYLE)
	{
		m_qScaleType = 0;
		m_intraVLCformat = 0;
		m_intraDcPred = INTRADCPRED8BIT;
		m_coeffLimit = MPEG1COEFFLIMIT;
	}
	else
	{
		m_qScaleType = 1;
		m_intraVLCformat = 1;
		m_intraDcPred = INTRADCPRED10BIT; 
		m_coeffLimit = MPEG2COEFFLIMIT;
	}

	// Initialize the serial read buffer that will be used to hold the encoded video
	m_SerialStore.Init(pSerialBuff,serialBuffSize,SERIALREAD);

	m_functionType = DECOMPRESSVIDEO;

}

////////////////////////////////////////////////////////////////////////////////////////
// Cancel does nothing in this thread
//
//  Input parameters : none
//
//	Output : none
//
void CAVVideoCompression::Cancel(void)
{
	 // Does nothing 
}



////////////////////////////////////////////////////////////////////////////////////////
// Thread that encodes or decodes a section of the input frame
//
//  Input parameters : none
//
//	Output : none
//
void CAVVideoCompression::Run(void)
{


	// Determine if we are compressing or decompressing
	if(m_functionType == COMPRESSVIDEO)
	{
		// Generate the quantization table, if needed
		SetQuantTable();


		// Convert the frame from RGB to YCrCb 4:2:0 if needed
		if(m_inputFormat == AVCRGB) ConvertFrameRGB2YCrCb();

		// Compress the frame into a serial buffer
		CompressFrame();

		// Save the number of encoded bytes
		// So the main thread will know how many bytes were encoded
		m_encodedBytes = m_SerialStore.GetBytes();
	}


	if(m_functionType == DECOMPRESSVIDEO) DecompressFrame();

}


////////////////////////////////////////////////////////////////////////////////////////
// Convert a portion of a frame that is in RGBA format to planer Y,Cr,Cb format
// using SSE2 if it is available
// Each input pixel is represented by 32 bits (4 bytes of RGBA)
// Each output pixel is represented by a short Y, short Cr and short Cb sample
// The Y, Cr,Cb values are in separate planer arrays.  The sampling is 4:2:0 so the 
// Cr and Cb arrays are 1/4 the size of the Y array
//
//  Input parameters : none
//
//	Output : None
//
//
void CAVVideoCompression::ConvertFrameRGB2YCrCb(void)
{

	// Locate the starting and ending frame rows for this thread
	unsigned int startRow = m_startRow;
	unsigned int endRow = startRow + m_sectionHeight;


	// The input and output addresses must be at least 16 byte aligned
	uint8 *pAddrIn = m_pInputFrame + startRow*m_encodedWidth*BYTESPERPIXEL;

	// Calculate the address of the Y, Cr, and Cb planes
	unsigned int planeOffset = m_encodedWidth*m_encodedHeight;
	short *outAddr = (short *) m_pOutputFrame;

	// Calculate the starting address of the section this thread is working on
	int chromaRow = m_startRow >> 1;
	int chromaStride = m_encodedWidth >> 1;
	short *plumaout = outAddr + m_startRow*m_encodedWidth;
	short *pCrout = outAddr + planeOffset  + chromaRow*chromaStride;
	short *pCbout = outAddr + (5*planeOffset/4) +  chromaRow*chromaStride;


	if(m_simdTypeAvail != SSE_AVAIL)
	{
		// No SSE available so convert the data one pixel at a time
		for(unsigned int i=startRow;i<endRow;++i)
		{
			for(unsigned int j=0;j<m_encodedWidth;++j)
			{

				// Grab the RGB value
				int B1 = (int) *pAddrIn;
				++pAddrIn;
				int G1 = (int) *pAddrIn;
				++pAddrIn;
				int R1 = (int) *pAddrIn;
				++pAddrIn;
				++pAddrIn; // skip over alpha Byte


				// Calculate the Luma value per ITU BT.709-5 or BT.601 depending upon the K values
				int luma = ( (  KRY * R1 + KGY * G1 +  KBY * B1 + KSCALEHALF) >> KSCALESHIFT) +  LUMAFLOOR;
				luma = CLAMP(luma,0,255);
				*plumaout = (short) (luma - KSCALEHALF);
				++plumaout;


				// Calculate the Chroma values per ITU BT.709-5 or BT.601 depending upon the K values
				// Only 1 out of 4 possible chroma values is calculated giving the required (4:2:0) sampling
				// Note : the Chroma is directly sub-sampled without filtering or shifting of position
				if( ((i|j)&1) == 0) 
				{
					int C1 = ( ( KRU * R1 +  KGU * G1 + KBU * B1 + KSCALEHALF) >> KSCALESHIFT) + CHROMAFLOOR;
					int C2 = ( ( KRV * R1 +  KGV * G1 +  KBV * B1 + KSCALEHALF) >> KSCALESHIFT) + CHROMAFLOOR;
					C1 = CLAMP(C1,0,255);
					C2 = CLAMP(C2,0,255);
					*pCrout =  (short) (C1-KSCALEHALF);
					++pCrout;
					*pCbout =  (short) (C2-KSCALEHALF);
					++pCbout;
				}

			}
		}

	}
	else
	{

     // Have SSE2 available so use it
#if defined(WIN32) || defined(WIN64)

		__m128i m0,m1,m2,m3,m4,m5,m7,m8,m9,m10,m11,m12,m13,m14;
		__m128i mY0,mY1,Y,mCb0,mCr0,mCb1,mCr1,mCb2,mCr2,mCb3,mCr3,mCb,mCr;


		// Load the filter coefficients and offsets
		__m128i c0 = _mm_set_epi16(0x00FF,0x00FF,0x00FF,0x00FF,0x00FF,0x00FF,0x00FF,0x00FF);  // Mask
		__m128i c1 = _mm_set_epi16(SSESCALE*KRY,SSESCALE*KBY,SSESCALE*KRY,SSESCALE*KBY,SSESCALE*KRY,SSESCALE*KBY,SSESCALE*KRY,SSESCALE*KBY);
		__m128i c2 = _mm_set_epi16(0,SSESCALE*KGY,0,SSESCALE*KGY,0,SSESCALE*KGY,0,SSESCALE*KGY);
		__m128i c3 = _mm_set_epi32(LUMAFLOORROUNDSSE,LUMAFLOORROUNDSSE,LUMAFLOORROUNDSSE,LUMAFLOORROUNDSSE);
		__m128i c4 = _mm_set_epi16(SSESCALE*KRV,SSESCALE*KBV,SSESCALE*KRV,SSESCALE*KBV,SSESCALE*KRV,SSESCALE*KBV,SSESCALE*KRV,SSESCALE*KBV);
		__m128i c5 = _mm_set_epi16(0,SSESCALE*KGV,0,SSESCALE*KGV,0,SSESCALE*KGV,0,SSESCALE*KGV);
		__m128i c6 = _mm_set_epi32(CHROMAFLOORROUNDSSE,CHROMAFLOORROUNDSSE,CHROMAFLOORROUNDSSE,CHROMAFLOORROUNDSSE);
		__m128i c7 = _mm_set_epi16(SSESCALE*KRU,SSESCALE*KBU,SSESCALE*KRU,SSESCALE*KBU,SSESCALE*KRU,SSESCALE*KBU,SSESCALE*KRU,SSESCALE*KBU);
		__m128i c8 = _mm_set_epi16(0,SSESCALE*KGU,0,SSESCALE*KGU,0,SSESCALE*KGU,0,SSESCALE*KGU);
		__m128i c9 = _mm_set_epi16(0,0,0,0,0,0,0,0);
		__m128i c10 = _mm_set_epi16(-KSCALEHALF,-KSCALEHALF,-KSCALEHALF,-KSCALEHALF,-KSCALEHALF,-KSCALEHALF,-KSCALEHALF,-KSCALEHALF);


		// This loop handles 2 rows per iteration  The first row requires Chroma calculations,
		// the second does not due to the 4:2:0 sampling
		for(unsigned int i=0;i<m_sectionHeight/2;++i) 
		{ 

			// First of two rows.  For this one we do the Chroma calculations
			// Each iteration of the loop does 16 points resulting in 16 luma, 8 Cb, and 8 Cr points calculated
			for(unsigned int j=0;j<m_encodedWidth/LUMAMBWIDTH;++j) 
			{ 

				// First 4
				//Calculate Y for 4 points
				m0 = _mm_load_si128((__m128i *) pAddrIn);  // m0 = ABGRABGRABGRABGR
				m1 = _mm_and_si128(m0,c0);               // m1 = 0B0R0B0R0B0R0B0R
				m2 = _mm_srli_epi16(m0,8);               // m2 = 0A0G0A0G0A0G0A0G
				m3 = _mm_madd_epi16(c1,m1);              // m3 = KBY*B + KRY*R
				m4 = _mm_madd_epi16(c2,m2);              // m4 = 0*A + KGY*G
				m5 = _mm_add_epi32(m3,m4);               // m5 = KRY*R + KGY*G + KBY*B
				mY0 = _mm_add_epi32(c3,m5);              // mY0 = KRY*R + KGY*G + KBY*B + 540672
				mY0 = _mm_srai_epi32(mY0,KSCALESHIFTSSE);            // mY0 = mY0/32768

				//Calculate Cb for 4 points
				m7 = _mm_madd_epi16(c4,m1);              // m7 = KBV*B + KRV*R
				m8 = _mm_madd_epi16(c5,m2);              // m8 = 0*A + KGV*G
				m9 = _mm_add_epi32(m7,m8);               // m9 = KRV*R + KGV*G + KBV*B
				mCb0 = _mm_add_epi32(c6,m9);             // mCb0 = KRV*R + KGV*G + KBV*B + 4210688
				mCb0 = _mm_srai_epi32(mCb0,KSCALESHIFTSSE);          // mCb0 = mCb0/32768

				//Calculate Cr for 4 points
				m10 = _mm_madd_epi16(c7,m1);             // m10 = KBU*B + KRU*R
				m11 = _mm_madd_epi16(c8,m2);             // m11 = 0*A + KGU*G
				m12 = _mm_add_epi32(m10,m11);            // m12 = KRU*R + KGU*G + KBU*B
				mCr0 = _mm_add_epi32(c6,m12);            // mCr0 = KRU*R + KGU*G + KBU*B + 4210688
				mCr0 = _mm_srai_epi32(mCr0,KSCALESHIFTSSE);          // mCr0 = mCr0/32768

				pAddrIn += LUMAMBWIDTH; // 4 pixels, 4 bytes each


				// Second 4
				//Calculate Y for 4 points
				m0 = _mm_load_si128((__m128i *) pAddrIn);  // m0 = ABGRABGRABGRABGR
				m1 = _mm_and_si128(m0,c0);               // m1 = 0B0R0B0R0B0R0B0R
				m2 = _mm_srli_epi16(m0,8);               // m2 = 0A0G0A0G0A0G0A0G
				m3 = _mm_madd_epi16(c1,m1);              // m3 = KBY*B + KRY*R
				m4 = _mm_madd_epi16(c2,m2);              // m4 = 0*A + KGY*G
				m5 = _mm_add_epi32(m3,m4);               // m5 = KRY*R + KGY*G + KBY*B
				mY1 = _mm_add_epi32(c3,m5);              // mY1 = KRY*R + KGY*G + KBY*B + 540672
				mY1 = _mm_srai_epi32(mY1,KSCALESHIFTSSE);            // mY1 = mY1/32768

				//Calculate Cb for 4 points
				m7 = _mm_madd_epi16(c4,m1);              // m7 = KBV*B + KRV*R
				m8 = _mm_madd_epi16(c5,m2);              // m8 = 0*A + KGV*G
				m9 = _mm_add_epi32(m7,m8);               // m9 = KRV*R + KGV*G + KBV*B
				mCb1 = _mm_add_epi32(c6,m9);             // mCb1 = KRV*R + KGV*G + KBV*B + 4210688
				mCb1 = _mm_srai_epi32(mCb1,KSCALESHIFTSSE);          // mCb1 = mCb1/32768

				//Calculate Cr for 4 points
				m10 = _mm_madd_epi16(c7,m1);             // m10 = KBU*B + KRU*R
				m11 = _mm_madd_epi16(c8,m2);             // m11 = 0*A + KGU*G
				m12 = _mm_add_epi32(m10,m11);            // m12 = KRU*R + KGU*G + KBU*B
				mCr1 = _mm_add_epi32(c6,m12);            // mCr1 = KRU*R + KGU*G + KBU*B + 4210688
				mCr1 = _mm_srai_epi32(mCr1,KSCALESHIFTSSE);          // mCr1 = mCr1/32768

				pAddrIn += LUMAMBWIDTH;  // 4 pixels, 4 bytes each

				// Pack the Y data into shorts and store to memory
				Y = _mm_packs_epi32(mY0,mY1);
				Y = _mm_packus_epi16(Y,Y);  // Saturate to an 8 bit value
				Y = _mm_unpacklo_epi8(Y,c9); // Turn back into a 16 bit value
				Y = _mm_add_epi16(Y,c10);    // subtract 128 
				_mm_storeu_si128((__m128i *) plumaout,Y);


				plumaout += BLOCKDIM;  // stored 8 luma as shorts

				// Third 4
				//Calculate Y for 4 points
				m0 = _mm_load_si128((__m128i *) pAddrIn);  // m0 = ABGRABGRABGRABGR
				m1 = _mm_and_si128(m0,c0);               // m1 = 0B0R0B0R0B0R0B0R
				m2 = _mm_srli_epi16(m0,8);               // m2 = 0A0G0A0G0A0G0A0G
				m3 = _mm_madd_epi16(c1,m1);              // m3 = KBY*B + KRY*R
				m4 = _mm_madd_epi16(c2,m2);              // m4 = 0*A + KGY*G
				m5 = _mm_add_epi32(m3,m4);               // m5 = KRY*R + KGY*G + KBY*B
				mY0 = _mm_add_epi32(c3,m5);              // mY0 = KRY*R + KGY*G + KBY*B + 540672
				mY0 = _mm_srai_epi32(mY0,KSCALESHIFTSSE);           // mY0 = mY0/32768

				//Calculate Cb for 4 points
				m7 = _mm_madd_epi16(c4,m1);              // m7 = KBV*B + KRV*R
				m8 = _mm_madd_epi16(c5,m2);              // m8 = 0*A + KGV*G
				m9 = _mm_add_epi32(m7,m8);               // m9 = KRV*R + KGV*G + KBV*B
				mCb2 = _mm_add_epi32(c6,m9);             // mCb2 = KRV*R + KGV*G + KBV*B + 4210688
				mCb2 = _mm_srai_epi32(mCb2,KSCALESHIFTSSE);          // mCb2 = mCb2/32768


				//Calculate Cr for 4 points
				m10 = _mm_madd_epi16(c7,m1);             // m10 = KBU*B + KRU*R
				m11 = _mm_madd_epi16(c8,m2);             // m11 = 0*A + KGU*G
				m12 = _mm_add_epi32(m10,m11);            // m12 = m12 = KRU*R + KGU*G + KBU*B
				mCr2 = _mm_add_epi32(c6,m12);            // mCr2 = KRU*R + KGU*G + KBU*B + 4210688
				mCr2 = _mm_srai_epi32(mCr2,KSCALESHIFTSSE);          // mCr2 = mCr2/32768

				pAddrIn += LUMAMBWIDTH;  // 4 pixels, 4 bytes each

				// Fourth 4
				//Calculate Y for 4 points
				m0 = _mm_load_si128((__m128i *) pAddrIn);  // m0 = ABGRABGRABGRABGR
				m1 = _mm_and_si128(m0,c0);               // m1 = 0B0R0B0R0B0R0B0R
				m2 = _mm_srli_epi16(m0,8);               // m2 = 0A0G0A0G0A0G0A0G
				m3 = _mm_madd_epi16(c1,m1);              // m3 = KBY*B + KRY*R
				m4 = _mm_madd_epi16(c2,m2);              // m4 = 0*A + KGY*G
				m5 = _mm_add_epi32(m3,m4);               // m5 = KRY*R + KGY*G + KBY*B
				mY1 = _mm_add_epi32(c3,m5);              // mY1 = KRY*R + KGY*G + KBY*B + 540672
				mY1 = _mm_srai_epi32(mY1,KSCALESHIFTSSE);            // mY1 = mY1/32768

				//Calculate Cb for 4 points
				m7 = _mm_madd_epi16(c4,m1);              // m7 = KBV*B + KRV*R
				m8 = _mm_madd_epi16(c5,m2);              // m8 = 0*A + KGV*G
				m9 = _mm_add_epi32(m7,m8);               // m9 = KRV*R + KGV*G + KBV*B
				mCb3 = _mm_add_epi32(c6,m9);             // mCb3 = KRV*R + KGV*G + KBV*B + 4210688
				mCb3 = _mm_srai_epi32(mCb3,KSCALESHIFTSSE);          // mCb3 = mCb3/32768


				//Calculate Cr for 4 points
				m10 = _mm_madd_epi16(c7,m1);             // m10 = KBU*B + KRU*R
				m11 = _mm_madd_epi16(c8,m2);             // m11 = 0*A + KGU*G
				m12 = _mm_add_epi32(m10,m11);            // m12 = m12 = KRU*R + KGU*G + KBU*B
				mCr3 = _mm_add_epi32(c6,m12);            // mCr3 = KRU*R + KGU*G + KBU*B + 4210688
				mCr3 = _mm_srai_epi32(mCr3,KSCALESHIFTSSE);          // mCr3 = mCr3/32768

				pAddrIn += LUMAMBWIDTH;  // 4 pixels, 4 bytes each

				// Pack the Y data into shorts and store to memory
				Y = _mm_packs_epi32(mY0,mY1);
				Y = _mm_packus_epi16(Y,Y);  // Saturate to an 8 bit value
				Y = _mm_unpacklo_epi8(Y,c9); // Turn back into a 16 bit value
				Y = _mm_add_epi16(Y,c10);    // subtract 128 
				_mm_storeu_si128 ((__m128i *) plumaout,Y);

				plumaout += BLOCKDIM;  // store second set of 8 luma as shorts




				// Pack all of the chroma stuff, discarding every other sample
				// due to 4:2:0 sampling and save to memory

				// Pack the Cb data into shorts and store to memory
				m13 = _mm_packs_epi32(mCb0,mCb1);
				m13 = _mm_srli_epi32(m13,16);
				m13 = _mm_packus_epi16(m13,m13);  // Saturate to an 8 bit value
				m13 = _mm_unpacklo_epi8(m13,c9); // Turn back into a 16 bit value

				m14 = _mm_packs_epi32(mCb2,mCb3);
				m14 = _mm_srli_epi32(m14,16);      
				m14 = _mm_packus_epi16(m14,m14);  // Saturate to an 8 bit value
				m14 = _mm_unpacklo_epi8(m14,c9); // Turn back into a 16 bit value

				mCb = _mm_packs_epi32(m13,m14);
				mCb = _mm_add_epi16(mCb,c10);    // subtract 128 
				_mm_storeu_si128((__m128i *) pCbout,mCb);


				// Pack the Cr data into shorts and store to memory
				m13 = _mm_packs_epi32(mCr0,mCr1);
				m13 = _mm_srli_epi32(m13,16);      
				m13 = _mm_packus_epi16(m13,m13);  // Saturate to an 8 bit value
				m13 = _mm_unpacklo_epi8(m13,c9); // Turn back into a 16 bit value


				m14 = _mm_packs_epi32(mCr2,mCr3);
				m14 = _mm_srli_epi32(m14,16);      
				m14 = _mm_packus_epi16(m14,m14);  // Saturate to an 8 bit value
				m14 = _mm_unpacklo_epi8(m14,c9); // Turn back into a 16 bit value

				mCr = _mm_packs_epi32(m13,m14);
				mCr = _mm_add_epi16(mCr,c10);    // subtract 128 
				_mm_storeu_si128((__m128i *) pCrout,mCr);


				pCbout += CHROMAMBWIDTH;  // store set of 8 Cb values as shorts
				pCrout += CHROMAMBWIDTH;  // store set of 8 Cr values as shorts


			}

			// Second of two rows.  For this one we do not need to do Chroma due to 4:2:0 sampling
			// Each iteration does 8 points at a time.  This results in 8 luma, 0 Cb, and 0 Cr points calculated
			for(unsigned int j=0;j<m_encodedWidth/8;++j) 
			{  

				// First 4
				//Calculate Y for 4 points
				m0 = _mm_load_si128((__m128i *) pAddrIn);  // m0 = ABGRABGRABGRABGR
				m1 = _mm_and_si128(m0,c0);               // m1 = 0B0R0B0R0B0R0B0R
				m2 = _mm_srli_epi16(m0,8);               // m2 = 0A0G0A0G0A0G0A0G
				m3 = _mm_madd_epi16(c1,m1);              // m3 = KBY*B + KRY*R
				m4 = _mm_madd_epi16(c2,m2);              // m4 = 0*A + KGY*G
				m5 = _mm_add_epi32(m3,m4);               // m5 = KRY*R + KGY*G + KBY*B
				mY0 = _mm_add_epi32(c3,m5);              // mY0 = KRY*R + KGY*G + KBY*B + 540672
				mY0 = _mm_srai_epi32(mY0,KSCALESHIFTSSE);           // mY0 = mY0/32768

				pAddrIn += LUMAMBWIDTH;  // 4 pixels, 4 bytes each

				// Second 4
				//Calculate Y for 4 points
				m0 = _mm_load_si128((__m128i *) pAddrIn);  // m0 = ABGRABGRABGRABGR
				m1 = _mm_and_si128(m0,c0);               // m1 = 0B0R0B0R0B0R0B0R
				m2 = _mm_srli_epi16(m0,8);               // m2 = 0A0G0A0G0A0G0A0G
				m3 = _mm_madd_epi16(c1,m1);              // m3 = KBY*B + KRY*R
				m4 = _mm_madd_epi16(c2,m2);              // m4 = 0*A + KGY*G
				m5 = _mm_add_epi32(m3,m4);               // m5 = KRY*R + KGY*G + KBY*B
				mY1 = _mm_add_epi32(c3,m5);              // mY1 = KRY*R + KGY*G + KBY*B + 540672
				mY1 = _mm_srai_epi32(mY1,KSCALESHIFTSSE);            // mY1 = mY1/32768

				pAddrIn += LUMAMBWIDTH;  // 4 pixels, 4 bytes each

				// Pack the Y data into shorts and store to memory
				Y = _mm_packs_epi32(mY0,mY1);
				Y = _mm_packus_epi16(Y,Y);  // Saturate to an 8 bit value
				Y = _mm_unpacklo_epi8(Y,c9); // Turn back into a 16 bit value
				Y = _mm_add_epi16(Y,c10);    // subtract 128 
				_mm_storeu_si128((__m128i *) plumaout,Y);


				plumaout += BLOCKDIM;  // store second set of 8 luma as shorts

			}

		}
#endif  // if defined(WIN32) || defined(WIN64)
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Compress a portion of a frame consisting of a number of consecutive slices (i.e., macroblock rows)
// Each input  pixel is represented by a short Y, short Cr and short Cb sample
// The Y, Cr,Cb values are in separate planer arrays.  The sampling is 4:2:0 so the 
// Cr and Cb arrays are 1/4 the size of the Y array
//
//  Input parameters : none
//
//	Output : None
//
//
void CAVVideoCompression::CompressFrame(void)
{

	// Need some temporary buffers for holding macroblock data
	// Each macroblock consists of 6 64 sample blocks each sample is a short + 16 extra needed for alignment
	// 6*64*sizeof(short) + 16 = 784
   uint8 tbuff1[784]; 
   uint8 tbuff2[784];
   uint8 tbuff3[784];
   uint8 tbuff4[784];
   uint8 tbuff5[1552];

   // FOR SSE operation, these buffers need to be 16 byte aligned
   short *pCurMB    = (short *)  (((UINT_PTR)tbuff1 + 15) & ( (UINT_PTR) ~15));
   short *pCurEnc   = (short *)  (((UINT_PTR)tbuff2 + 15) & ( (UINT_PTR) ~15));
   short *pMBref    = (short *)  (((UINT_PTR)tbuff3 + 15) & ( (UINT_PTR) ~15));
   short *pMBdelta  = (short *)  (((UINT_PTR)tbuff4 + 15) & ( (UINT_PTR) ~15));
   int *pReGen  = (int *)  (((UINT_PTR)tbuff5 + 15) & ( (UINT_PTR) ~15));


	// Get the starting and ending macroblock rows to process for this thread
	unsigned int startMBrow = m_startRow >> MBHEIGHTSHIFT;
	unsigned int endMBrow = startMBrow + (m_sectionHeight >> MBHEIGHTSHIFT);


	// Encode several rows of macroblocks.  Each row is a separate slice with its own pes and slice header
	for(unsigned int i=startMBrow;i < endMBrow;++i) 
	{

		unsigned int start_loc = m_SerialStore.GetBytes();
		ResetPredictors(RESETALL);

		// Generate the slice header
		GenSliceHdr(i);

		// process the MBs in the slice row
		int	addrInc = 1;
		for(unsigned int j=0;j<m_MBWidth;++j) 
		{

			// Get a single macroblock from the working frame buffer
			getMB(j,i,pCurMB,m_pOutputFrame);

			// Find out the type of macroblock encoding that is needed
			int codedPattern = 0;
			int bestX,bestY;
			int MBType = FindMBType(j,i,pCurMB,&bestX,&bestY);

			switch(MBType)
			{


				case MB_SKIP :				if(m_pNxtRefBuffer)
											{
												getMB(j,i,pMBref,m_pCurRefBuffer);
												PlaceMB(j,i,(uint8 *) pMBref,m_pNxtRefBuffer,0);
											}
											addrInc++;
											ResetPredictors(RESETALL);
											break;


				case MB_INTRA :				EncodeMBAddr(addrInc);
											addrInc = 1;
											EncodeMBType(MB_INTRA);
											ForwardDctQuantize(pCurMB,pCurEnc,MBType);
											EncodeDataVLC(pCurEnc,MB_INTRA,codedPattern);
											if(m_pNxtRefBuffer)
											{
												InverseQuantDct(pCurEnc,pReGen,MB_INTRA);
												PlaceMB(j,i,(uint8 *) pReGen,m_pNxtRefBuffer,1);
											}
											ResetPredictors(RESETMV);
											break;


				case MB_NOMOTION_CODED :	getMB(j,i,pMBref,m_pCurRefBuffer);
											for(unsigned int k=0;k<MBSIZE;++k) pMBdelta[k] = pCurMB[k] - pMBref[k];
											ForwardDctQuantize(pMBdelta,pCurEnc,MBType);
											codedPattern = FindCBP(pCurEnc);

											// Check if we need to change the MB type
											if(codedPattern == 0)
											{
												if(j != 0 && j != m_MBWidth-1) 
												{
													// This should be a skip MB
													if(m_pNxtRefBuffer) PlaceMB(j,i,(uint8 *) pMBref,m_pNxtRefBuffer,0);
													addrInc++;
												}
												else
												{
													// Can't use skip, so use MB_MOTION_NOTCODED type with 0,0 motion vectors
													EncodeMBAddr(addrInc);
													addrInc = 1;
													EncodeMBType(MB_MOTION_NOTCODED);
													EncodeMotion(0,0);
													if(m_pNxtRefBuffer) PlaceMB(j,i,(uint8 *) pMBref,m_pNxtRefBuffer,0);
												}
											}
											else
											{
												EncodeMBAddr(addrInc);
												addrInc = 1;
												EncodeMBType(MB_NOMOTION_CODED);
												EncodeDataVLC(pCurEnc,MB_NOMOTION_CODED,codedPattern);
												if(m_pNxtRefBuffer)
												{
													InverseQuantDct(pCurEnc,pReGen,MB_NOMOTION_CODED);
													for(unsigned int k=0;k<MBSIZE;++k)  
													{
														pReGen[k] += pMBref[k];
														pReGen[k] = CLAMP(pReGen[k],-MPEG1COEFFLIMIT,MPEG1COEFFLIMIT);
													}
													PlaceMB(j,i,(uint8 *) pReGen,m_pNxtRefBuffer,1);
												}
												
											}
											ResetPredictors(RESETALL);
											break;


				case MB_MOTION_NOTCODED :	EncodeMBAddr(addrInc);
											addrInc = 1;
											EncodeMBType(MB_MOTION_NOTCODED);
											EncodeMotion(bestX,bestY);
											if(m_pNxtRefBuffer)
											{
												GetBlock((bestX/2) + j*LUMAMBWIDTH,(bestY/2) + i*LUMAMBWIDTH,pMBref,m_pCurRefBuffer);
												PlaceMB(j,i,(uint8 *) pMBref,m_pNxtRefBuffer,0);
											}
											ResetPredictors(RESETDC);
											break;


				case MB_MOTION_CODED :		GetBlock((bestX/2) + j*LUMAMBWIDTH,(bestY/2) + i*LUMAMBWIDTH,pMBref,m_pCurRefBuffer);
											for(unsigned int k=0;k<MBSIZE;++k) pMBdelta[k] = pCurMB[k] - pMBref[k];
											ForwardDctQuantize(pMBdelta,pCurEnc,MBType);
											codedPattern = FindCBP(pCurEnc);

											// Check if we need to change the MB type
											if(codedPattern == 0)
											{
												// Type should be MB_MOTION_NOTCODED
												EncodeMBAddr(addrInc);
												addrInc = 1;
												EncodeMBType(MB_MOTION_NOTCODED);
												EncodeMotion(bestX,bestY);
												if(m_pNxtRefBuffer) PlaceMB(j,i,(uint8 *) pMBref,m_pNxtRefBuffer,0);
												ResetPredictors(RESETDC);

											}
											else
											{
												EncodeMBAddr(addrInc);
												addrInc = 1;
												EncodeMBType(MB_MOTION_CODED);
												EncodeMotion(bestX,bestY);
												EncodeDataVLC(pCurEnc,MB_MOTION_CODED,codedPattern);
												if(m_pNxtRefBuffer)
												{
													InverseQuantDct(pCurEnc,pReGen,MB_MOTION_CODED);
													for(unsigned int k=0;k<MBSIZE;++k)  
													{
														pReGen[k] += pMBref[k];
														pReGen[k] = CLAMP(pReGen[k],-MPEG1COEFFLIMIT,MPEG1COEFFLIMIT);
													}
													PlaceMB(j,i,(uint8 *) pReGen,m_pNxtRefBuffer,1);
												}
											}
											ResetPredictors(RESETDC);
											break;												
			}

		}

		// Make sure that the serial buffer ends on a SERIALLINE size boundary
		m_SerialStore.AlignToWord(0);  


		// If this needed a pes header, then update the pes size
		if(m_containerStyle == MPEG1_STYLE || m_containerStyle == MPEG2_STYLE)
		{
			unsigned int audioPesSize = m_SerialStore.GetBytes() - start_loc - VIDEOPESHDROFFSET; 
			uint8 *pTmp = m_SerialStore.GetBufferAddress();
			pTmp[start_loc + 4] = (uint8) ((audioPesSize >> 8) & 0xFF);
			pTmp[start_loc + 5] = (uint8) ((audioPesSize) & 0xFF);
		}
	}

}



////////////////////////////////////////////////////////////////////////////////////////
// Reset the DC and motion vector predictors
//
//  Input parameters : 
//		resetType  --  RESETDC,RESETMV or RESETALL
//
//	Output : none
//
//
void CAVVideoCompression::ResetPredictors(int resetType)
{

	if(resetType&1)
	{
		m_dcDctPred[0] = 0;
		m_dcDctPred[1] = 0;
		m_dcDctPred[2] = 0;
	}
	if(resetType&2)
	{
		m_pmv[0] = 0;
		m_pmv[1] = 0;
	}
}


////////////////////////////////////////////////////////////////////////////////////////
// Encode the macroblock type
//
//  Input parameters : 
//		MBType  --  Type of the macroblock
//
//	Output : none
//
//
// See ISO/IEC 13818-2, tables B2 - B8
//
void CAVVideoCompression::EncodeMBType(int MBType)
{
	switch(MBType)
	{
		case MB_INTRA :				if(m_frameType == IFRAMETYPE || m_frameType == IREFFRAMETYPE) m_SerialStore.PutBit(1,1);
									else m_SerialStore.PutBit(3,5);
									break;

		case MB_NOMOTION_CODED :	m_SerialStore.PutBit(1,2);
									break;

		case MB_MOTION_NOTCODED :	m_SerialStore.PutBit(1,3);
									break;

		case MB_MOTION_CODED :		m_SerialStore.PutBit(1,1);
									break;
	}

}



////////////////////////////////////////////////////////////////////////////////////////
// Calculate the coded block pattern for a macroblock
//
//  Input parameters : 
//		pbuff  --  pointer to a macroblock
//
//	Output : Returns the coded block pattern for the macroblock
//
//
int CAVVideoCompression::FindCBP(short *pBuff)
{

   int codedPattern = 0;
   short *pIn = pBuff;
   for(unsigned int i=0;i<NUMSUBBLOCKS;++i)
   {
	    int notZeroFlag = 0;
		for(unsigned int j=0;j<BLOCK8x8SIZE;++j) 
		{
			if(pIn[j] != 0)
			{
				notZeroFlag = 1;
				break;
			}
		}

		codedPattern = (codedPattern << 1) + notZeroFlag;
		pIn += BLOCK8x8SIZE;
   }
   return(codedPattern);
}


////////////////////////////////////////////////////////////////////////////////////////
// VLC encode a macroblock
//
//  Input parameters : 
//		pbuff  --  pointer to a macroblock
//		MBType -- type of macroblock to be encoded
//		codedPattern  --  coded block pattern for the macroblock
//
//	Output : none
//
//
void CAVVideoCompression::EncodeDataVLC(short *pBuff,int MBType,int codedPattern)
{

	short pb1[MBSIZE];
	short *pIn = pBuff;
	short *pOut = pb1;

	if(MBType == MB_INTRA)
	{
		// Encode the 6 blocks
		for(unsigned int k=0;k<NUMSUBBLOCKS;++k)
		{
			int cc = (k < 4) ? 0 : k-3;  // 0 for luma (blocks 0-3) , 1 for Cr sub block, 2 for Cb subblock
			int lastNonZero=ConvertScanOrder(pIn,pOut,MBType);
			PutIntraBlk(pOut,cc,lastNonZero);
			pIn  += BLOCK8x8SIZE;
			pOut += BLOCK8x8SIZE;
		}
	}
	else
	{

		// write the coded pattern
		m_SerialStore.PutBit(CodedPatternCode[codedPattern],CodedPatternLen[codedPattern]);

		// Encode the 6 blocks
		for(unsigned int k=0;k<NUMSUBBLOCKS;++k)
		{
			int lastNzLoc = ConvertScanOrder(pIn,pOut,MBType);
			if(lastNzLoc != -1) PutInterBlk(pOut,lastNzLoc);
			pIn  += BLOCK8x8SIZE;
			pOut += BLOCK8x8SIZE;
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////
// Check if a macroblock can be skipped
//
//  Input parameters : 
//		x  --  macroblock x location
//		y  --  macroblock y location
//		pMBbuff  -- pointer to the macroblock data
//
//	Output : 1 = skip macroblock, 0 = don't skip
//
//
int CAVVideoCompression::CheckForSkip(int x,int y,short *pMBbuff)
{

	short pMBref[MBSIZE];

	// Get a single macroblock from the reference buffer
	getMB(x,y,pMBref,m_pCurRefBuffer);

	int sum = 0;
	for(unsigned int i=0;i<MBSIZE;++i)
	{
		int diff = pMBref[i] - pMBbuff[i];
		if(diff < 0) diff = -diff;
		sum += diff;
	}

	if(sum < SKIPTHRESHOLD) return(1);
	else return(0);
}




////////////////////////////////////////////////////////////////////////////////////////
// Perform a search to find the best match for the current MB in the reference frame
//
//  Input parameters : 
//		x  --  macroblock x location
//		y  --  macroblock y location
//		pMBbuff  -- Pointer to the current MB data
//		pMBref	--	Location to put the best found match
//		pBestX  -- Pointer to the location for the best X location
//		pBestY  -- Pointer to the location for the best Y location
//
//	Output : None
//
void CAVVideoCompression::MBsearch(int x,int y,short *pMBbuff,short *pMBref,int *pBestX,int *pBestY)
{

	// FOR SSE operation, these buffers need to be 16 byte aligned
	uint8 tbuff1[784]; 
	short *pRef    = (short *)  (((UINT_PTR)tbuff1 + 15) & ( (UINT_PTR) ~15));

    // Use a diamond search pattern
	int *offsettabX = diamondOffsetTableX;
	int *offsettabY = diamondOffsetTableY;
	int numSearchPts = DIAMONDSEARCHPOINTS;

	// Arrays to hold the previous and new search results
	int xloc[2][DIAMONDSEARCHPOINTS];
	int yloc[2][DIAMONDSEARCHPOINTS];
	int xpoint;
	int ypoint;
	int outsideBoundary;
	int newscore;



	// Initialize the search point memory
	int oldPointsindex = 0;
	int newPointsindex = 1;
    for(unsigned int i=0;i<numSearchPts;++i)
	{
		xloc[oldPointsindex][i] = MVOUTOFRANGE;
		yloc[oldPointsindex][i] = MVOUTOFRANGE;
	}


	// First check the 0,0 location and set as the best score
	int xorigin = x*LUMAMBWIDTH;
	int yorigin = y*LUMAMBWIDTH;
	GetBlock(xorigin,yorigin,pRef,m_pCurRefBuffer);
	float bestScore = CalcSSE(pMBbuff,pRef);
	int bestXloc = xorigin;
	int bestYloc = yorigin;


	int itters = DIAMONDSEARCHITERS;
	// Iterate using the diamond search pattern
	while(itters > 0)
	{

		int newBestFlag = 0;
		for(unsigned int i=0;i<numSearchPts;++i)
		{
			xpoint = xorigin + offsettabX[i];
			ypoint = yorigin + offsettabY[i];

			// Save the current point location
			xloc[newPointsindex][i] = xpoint;
			yloc[newPointsindex][i] = ypoint; 

			// check if we have already done this point
			int doneflag = 0;
			for(unsigned int j=0;j<numSearchPts;++j)
			{
				if(xloc[oldPointsindex][j] == xpoint && yloc[oldPointsindex][j] == ypoint) 
				{
					doneflag = 1;
					break;
				}
			}

			if(!doneflag)
			{
				// Need to calculate a new point

				// check the point boundaries
				outsideBoundary = (xpoint < 0 || xpoint+LUMAMBWIDTH > m_encodedWidth || ypoint < 0 || ypoint+LUMAMBWIDTH > m_encodedHeight) ? 1 : 0;
				if(!outsideBoundary)
				{
					// New point so calculate score
					GetBlock(xpoint,ypoint,pRef,m_pCurRefBuffer);
					newscore = CalcSSE(pMBbuff,pRef);

					if(newscore < bestScore)
					{
						bestScore = newscore;
						bestXloc = xpoint;
						bestYloc = ypoint;
						newBestFlag = 1;
					}
				}
			}
		}

		// Reset the origin to the best location so far
		xorigin = bestXloc;
		yorigin = bestYloc;

		--itters;

		// Swap the new and old point arrays
		oldPointsindex = (oldPointsindex + 1)%2;
		newPointsindex = (newPointsindex + 1)%2;

		// If we failed to find a new best point on this iteration, then we are done
		if(!newBestFlag) break; 

	}



    // Finish the search by searching the immediate points around the origin point
	// That were not searched already
	offsettabX = diamondOffsetTable2X;
	offsettabY = diamondOffsetTable2Y;
	numSearchPts = DIAMONDSEARCHPOINTS2;

	for(unsigned int i=0;i<numSearchPts;++i)
	{
		xpoint = xorigin + offsettabX[i];
		ypoint = yorigin + offsettabY[i];

		// check the point boundaries
		outsideBoundary = (xpoint < 0 || xpoint+LUMAMBWIDTH > m_encodedWidth || ypoint < 0 || ypoint+LUMAMBWIDTH > m_encodedHeight) ? 1 : 0;

		if(!outsideBoundary)
		{
			// calculate the score
			GetBlock(xpoint,ypoint,pRef,m_pCurRefBuffer);
			newscore = CalcSSE(pMBbuff,pRef);

			if(newscore < bestScore)
			{
				bestScore = newscore;
				bestXloc = xpoint;
				bestYloc = ypoint;
			}
		}
	}

	// Put the best match into the ref buff
	GetBlock(bestXloc,bestYloc,pMBref,m_pCurRefBuffer);

	// Calculate the motion vector accounting for half pel vector	
	*pBestX = (bestXloc - x*LUMAMBWIDTH)*2;
	*pBestY = (bestYloc - y*LUMAMBWIDTH)*2;

}



////////////////////////////////////////////////////////////////////////////////////////
// Calculate the sum of squared error between two MB
//
//  Input parameters : 
//		pMB1  --  pointer to first macroblock, must be 16 byte aligned
//		pMB2  --  pointer to second macroblock, must be 16 byte aligned
//
//	Output : sum of squared error between the 2 macroblocks
//
//
float CAVVideoCompression::CalcSSE(short *pMB1,short *pMB2)
{

	if(m_simdTypeAvail != SSE_AVAIL)
	{

		// No SSE available
		float score = 0.0f;
		for (unsigned int i=0;i<MBSIZE;++i)
		{
			float delta = pMB1[i] - pMB2[i];
			score += delta*delta;
		}
		return(score);
	}
	else
	{

		// Have SSE2 available so use it
#if defined(WIN32) || defined(WIN64)

		__m128i *setA = (__m128i *) pMB1;
		__m128i *setB = (__m128i *) pMB2;


		// Set the sum to 0
		__m128i sum = _mm_setzero_si128();

		for(unsigned int i=0;i<MBROWS;++i)
		{
			__m128i v0 = _mm_sub_epi16(setA[i],setB[i]);
			__m128i v1 = _mm_madd_epi16(v0,v0);
			sum = _mm_add_epi32 (sum,v1);
		}


		int *pSum = (int *) &sum;

		float total = pSum[0] + pSum[1] + pSum[2] + pSum[3];
		return(total);
	}
#endif

}

////////////////////////////////////////////////////////////////////////////////////////
// Determine the proper macroblock type for the current macroblock
//
//  Input parameters : 
//		x  --  macroblock x location
//		y  --  macroblock y location
//		pMBbuff  --  pointer to the current macroblock data
//		pBestX  -- pointer to the best found X location
//		pBestY	--	pointer to the best found Y location
//
//	Output : returns the proper MB type for the macroblock
//
int CAVVideoCompression::FindMBType(int x,int y,short *pMBbuff,int *pBestX,int *pBestY)
{

	// FOR SSE operation, these buffers need to be 16 byte aligned
	uint8 tbuff1[784]; 
	short *pMBref    = (short *)  (((UINT_PTR)tbuff1 + 15) & ( (UINT_PTR) ~15));

	*pBestX = 0;
	*pBestY = 0;
	int MBType = MB_UNKNOWN;

	// If I frame, the type is always INTRA
	if(m_frameType == IFRAMETYPE || m_frameType == IREFFRAMETYPE)  MBType = MB_INTRA;

	// Check to see if this macroblock can be skipped
	if(MBType == MB_UNKNOWN && CheckForSkip(x,y,pMBbuff)) MBType = MB_SKIP;
	
	// Special case :  can't skip first or last MB in slice
	// so change from skip if this is the case
	if(MBType == MB_SKIP && (x == 0 || x == m_MBWidth-1)) MBType = MB_MOTION_NOTCODED;


	// If still unknown, check motion possibilities
	if(MBType == MB_UNKNOWN)
	{

		MBsearch(x,y,pMBbuff,pMBref,pBestX,pBestY);

		// check whether the MB should be intra, by comparing the variance of the 
		// macroblock with the variance of the difference with the best found reference block
		int iSum = 0;
		int sSum = 0;
		int dVar = 0;
		for(unsigned int i=0;i<MBSIZE;++i)
		{
			iSum += pMBbuff[i];
			sSum += (pMBbuff[i] * pMBbuff[i]);
			int diff = (pMBbuff[i] - pMBref[i]);
			dVar += (diff * diff);
		}
		iSum = iSum/(float) MBSIZE;
		sSum = sSum/(float) MBSIZE;
		dVar = dVar/(float) MBSIZE;

		int iVar = sSum - (iSum*iSum);

		// If the variance of the intra is less than the variance of the difference image
		// then pick intra, except in the case where the variance is small then use the motion 
		// image anyway
		if((dVar > PINTRATHRESHOLD) && (dVar > iVar))
		{
			MBType = MB_INTRA;
		}
		else 
		{
			if(*pBestX == 0 && *pBestY == 0) MBType = MB_NOMOTION_CODED;  // motion is 0, so no need to send motion vector
			else
			{
				if(dVar < NOTCODEDTHRESHOLD) MBType = MB_MOTION_NOTCODED;  // needs motion, but don't have to encode any data
				else MBType = MB_MOTION_CODED;  // need to send both motion vectors and encoded data
			}
		}
	}


	return(MBType);

}
////////////////////////////////////////////////////////////////////////////////////////
// Encode the macroblock address
//
//  Input parameters : 
//
//		skipCount  -  number of macroblocks that were skipped + 1
//
//	Output : none
//
//
void CAVVideoCompression::EncodeMBAddr(int skipCount)
{

	int skipLen = skipCount;
	while(skipLen > VLCADDRESCLIMIT)
	{
		// Use address esc sequence
		// Generated from ISO 13818-2 table B-1
			m_SerialStore.PutBit(8,11);
			skipLen -= VLCADDRESCLIMIT;
	}
	m_SerialStore.PutBit(MBAddrCode[skipLen],MBAddrLen[skipLen]);
}



////////////////////////////////////////////////////////////////////////////////////////
// Generate a pes elementary stream header
//
//  Input parameters : None
//
//	Output : Starting buffer location
//
//
unsigned int CAVVideoCompression::GenPesElemHdr(void)
{

	unsigned int start_loc = m_SerialStore.GetBytes();

	// Add a pes packet header for this slice
	// See ISO 13818-1 sec. 2.4.3.6 for field definitions
	m_SerialStore.PutBit(1,24);
	m_SerialStore.PutBit(m_videoPesID,8);
	m_SerialStore.PutBit(0,16);   // PES packet length, filled in later once length is known

	if(m_containerStyle == MPEG1_STYLE) m_SerialStore.PutBit(0x0f,8);    // padding bits
	else
	{
		m_SerialStore.PutBit(2,2);    // padding bits
		m_SerialStore.PutBit(0,2);    // PES_scrambling_control
		m_SerialStore.PutBit(0,1);    // PES_priority
		m_SerialStore.PutBit(1,1);    // data_alignment_indicator
		m_SerialStore.PutBit(0,1);    // copyright
		m_SerialStore.PutBit(1,1);    // original_or_copy
		m_SerialStore.PutBit(0,2);    // PTS_DTS_flags
		m_SerialStore.PutBit(0,1);    // ESCR_flag
		m_SerialStore.PutBit(0,1);    // ES_rate_flag
		m_SerialStore.PutBit(0,1);    // DSM_trick_mode_flag
		m_SerialStore.PutBit(0,1);    // additional_copy_info_flag
		m_SerialStore.PutBit(0,1);    // PES_CRC_flag
		m_SerialStore.PutBit(0,1);    // PES_extension_flag
		m_SerialStore.PutBit(0,8);    // PES_header_data_length
	}


	return(start_loc);

}


////////////////////////////////////////////////////////////////////////////////////////
// Generate a slice header
//
//  Input parameters :
//			sliceRow :  number of the slice row, 0=first row
//
//	Output : None
//
//
void CAVVideoCompression::GenSliceHdr(unsigned int sliceRow)
{

	// Generate a pes packet header for this slice if it is needed
	if(m_containerStyle == MPEG1_STYLE || m_containerStyle == MPEG2_STYLE) GenPesElemHdr();

	// Add a slice header
	// See ISO 13818-2 sec. 6.2.4 for field definitions
	m_SerialStore.PutBit(0,16); // slice start code
	m_SerialStore.PutBit(SLICESTARTCODE + sliceRow,16); // slice start code
	m_SerialStore.PutBit(m_mQuant,5);   // quantization index
	m_SerialStore.PutBit(0,1);   // extra bit slice

}

////////////////////////////////////////////////////////////////////////////////////////
// VLC encode an intra macroblock into a serial buffer
//
//  Input parameters :
//		pblk :  Pointer to the macroblock which consists of 64 shorts
//		cc :  0 for the 4 luma blocks of a macroblock, 1 and 2 for the 2 chroma blocks of a macroblock
//		lastNonZero  :  gives the index to the last nonzero value in a macroblock
//
//	Output : None
//
//
void CAVVideoCompression::PutIntraBlk(short *pblk,unsigned int cc,unsigned int lastNonZero)
{
	assert(pblk);


	// Calculate the DC differential from the last DC and encode the dc value
	int dct_diff = pblk[0] - m_dcDctPred[cc]; 
	m_dcDctPred[cc] = pblk[0];
	PutDC(cc,dct_diff);

	// Encode the remaining AC coeffs
	int run = 0;
	if(m_intraVLCformat == 0)
	{
		if(lastNonZero != -1)
		{
			for(unsigned int i=1;i<=lastNonZero;++i) 
			{
				if (pblk[i] != 0) 
				{
					PutACb14(run,pblk[i]);
					run = 0;
				}
				else
				{
					++run;
				}
			}
		}

		// End of block ISO 13818-2 table B-14
		m_SerialStore.PutBit(VLCEOBCODEMPEG1,VLCEOBCODEMPEG1LEN); 
	}
	else 
	{
		if(lastNonZero != -1)
		{
			for(unsigned int i=1;i<=lastNonZero;++i) 
			{
				if (pblk[i] != 0) 
				{
					PutACb15(run,pblk[i]);
					run = 0;
				}
				else
				{
					++run;
				}
			}
		}

		// End of block ISO 13818-2 table B-15
		m_SerialStore.PutBit(VLCEOBCODEMPEG2,VLCEOBCODEMPEG2LEN); 
	}
}


////////////////////////////////////////////////////////////////////////////////////////
// VLC encode an inter  macroblock into a serial buffer
//
//  Input parameters :
//		pblk :  Pointer to the macroblock which consists of 64 shorts
//		cc :  0 for the 4 luma blocks of a macroblock, 1 and 2 for the 2 chroma blocks of a macroblock
//		lastNonZero  :  gives the index to the last nonzero value in a macroblock
//
//	Output : None
//
//
void CAVVideoCompression::PutInterBlk(short *pblk,unsigned int lastNonZero)
{
	assert(pblk);

	int startLoc = 0;
	// Special case for the first coeff
	// This is different than in the intra blocks
	if(pblk[0] == 1)
	{
		m_SerialStore.PutBit(2,2); 
		startLoc = 1;
	}

	if(pblk[0] == -1)
	{
		m_SerialStore.PutBit(3,2); 
		startLoc = 1;
	}


	// Encode the rest of the coeffs
	int run = 0;
	for(unsigned int i=startLoc;i<=lastNonZero;++i) 
	{
		if (pblk[i] != 0) 
		{
			PutACb14(run,pblk[i]);
			run = 0;
		}
		else
		{
			++run;
		}
	}

	// End of block ISO 13818-2 table B-14
	m_SerialStore.PutBit(VLCEOBCODEMPEG1,VLCEOBCODEMPEG1LEN); 

}


////////////////////////////////////////////////////////////////////////////////////////
// VLC encode motion vectors into a serial buffer
//
//  Input parameters :
//		MVX :  motion vector in the X direction
//		MVY :  motion vector in the Y direction
//
//	Output : None
//
void CAVVideoCompression::EncodeMotion(int MVX,int MVY)
{

	int delta0 = MVX - m_pmv[0];
	int delta1 = MVY - m_pmv[1];
	m_pmv[0] = MVX;
	m_pmv[1] = MVY;

	PutMV(delta0,m_fcodeHoriz);
	PutMV(delta1,m_fcodeVert);

}


////////////////////////////////////////////////////////////////////////////////////////
// VLC encode a differential motion vector into a serial buffer
//
//  Input parameters :
//		MV :  differential motion vector
//		fCode :  Motion vector range as encoded in the picture header
//
//	Output : None
//
void CAVVideoCompression::PutMV(int MV, int fCode)
{

	// See 13818-2 section 7.6.3.1 for algorithm description
	int fCodeIdx = fCode - 1;
	int f = 1 << fCodeIdx;
	int vmin = -16*f; 
	int vmax = 16*f - 1; 
	int dv = 32*f;

	if (MV > vmax) MV-= dv;
	else if (MV < vmin) MV+= dv;

	int motion_residual;
	int motion_code;
	if (MV == 0)
	{
		m_SerialStore.PutBit(MotionVectorCode[0],MotionVectorLen[0]);

	}
	else
	{
		motion_residual = (abs(MV) - 1) % f;
		motion_code = (abs(MV) - motion_residual) / f;

		if (f != 1) motion_code++;

		m_SerialStore.PutBit(MotionVectorCode[motion_code],MotionVectorLen[motion_code]);
		if (MV < 0) m_SerialStore.PutBit(1,1);
		else m_SerialStore.PutBit(0,1);

		if (fCode != 1 && motion_code != 0) m_SerialStore.PutBit(motion_residual,fCodeIdx);
	}
}


////////////////////////////////////////////////////////////////////////////////////////
// VLC encode a DC value of a macroblock into a serial buffer
//
//  Input parameters :
//		cc :  0 for the 4 luma blocks of a macroblock, 1 and 2 for the 2 chroma blocks of a macroblock
//		val  :  DC value to be encoded
//
//	Output : None
//
//
void CAVVideoCompression::PutDC(unsigned int cc,int DCval)
{


	// Get the absolute value of the DC component
	unsigned int absval;
	if(DCval < 0) absval = -DCval;
	else absval = DCval;

	// Find out how many bits are in the value
	unsigned int len = FindMinBits(absval);
	assert(len < 12);

	// Send the code describing the length of the value
	if(cc == 0) m_SerialStore.PutBit(LumaDcCode[len],LumaDcLen[len]);
	else m_SerialStore.PutBit(ChromaDcCode[len],ChromaDcLen[len]);

	// Send the actual Value which is converted to unsigned before sending
	if (len != 0) 
	{
		if(DCval < 0) absval = OffsetTable[len] - absval;
		m_SerialStore.PutBit(absval,len);
	}
}


////////////////////////////////////////////////////////////////////////////////////////
// VLC encode an AC value of a macroblock into a serial buffer using MPEG1 VLC codes
//
//  Input parameters :
//		run :  number of consecutive 0s before the value to encode
//		ACval  :  the value to encode
//
//	Output : None
//
//
void CAVVideoCompression::PutACb14(unsigned int run,int ACval)
{

	// Index into the vlc tables
	int absVal = ACval < 0 ? -ACval : ACval;
	int polarity = ACval < 0 ? 1 : 0;


	// See if it is in the table lookup, this branch should happen nearly every time
	if(run < MPEGRUNMAX && absVal < MPEGVALMAX)
	{
		int tblIndex = run*MPEGVALMAX + absVal;

		// Check for the two escape codes 
		if (VLCB14Len[tblIndex] == MPEGESCALEN || VLCB14Len[tblIndex] == MPEGESCBLEN)
		{

			m_SerialStore.PutBit(VLCESCAPECODE,VLCESCSIZEBITS); // Escape code
			m_SerialStore.PutBit(run,VLCRUNSIZEBITS); // 6 bit code for run length
			if(m_videoCodecType == MPEG2_STYLE) m_SerialStore.PutBit(ACval,VLCVALSIZEBITS); // 12 bits for the value
			else
			{
				// 20 bit case
				if(absVal < MPEG1ESCALIMIT) m_SerialStore.PutBit(ACval,8);
				else 
				{
					// 28 bit case
					if(ACval < 0) m_SerialStore.PutBit(ACval + MPEG1NEGOFFSET,16);
					else m_SerialStore.PutBit(ACval,16);
				}
			}
		}
		else
		{
			int valCode = VLCB14Code[tblIndex] + polarity;
			m_SerialStore.PutBit(valCode,VLCB14Len[tblIndex]);
		}

	}
	else
	{
		// Not in the table so it needs an escape code
		m_SerialStore.PutBit(VLCESCAPECODE,VLCESCSIZEBITS); // Escape code
		m_SerialStore.PutBit(run,VLCRUNSIZEBITS); // 6 bit code for run length
		if(m_videoCodecType == MPEG2_STYLE) m_SerialStore.PutBit(ACval,VLCVALSIZEBITS); // 12 bits for the value
		else
		{
			// 20 bit case
			if(absVal < MPEG1ESCALIMIT) m_SerialStore.PutBit(ACval,8);
			else 
			{
				// 28 bit case
				if(ACval < 0) m_SerialStore.PutBit(ACval + MPEG1NEGOFFSET,16);
				else m_SerialStore.PutBit(ACval,16);
			}
		}


	}

}

////////////////////////////////////////////////////////////////////////////////////////
// VLC encode an AC value of a macroblock into a serial buffer using MPEG2 VLC codes
//
//  Input parameters :
//		run :  number of consecutive 0s before the value to encode
//		ACval  :  the value to encode
//
//	Output : None
//
//
void CAVVideoCompression::PutACb15(unsigned int run,int ACval)
{

	// Index into the vlc tables
	int absVal = ACval < 0 ? -ACval : ACval;
	int polarity = ACval < 0 ? 1 : 0;


	// See if it is in the table lookup, this branch should happen nearly every time
	if(run < MPEGRUNMAX && absVal < MPEGVALMAX)
	{
		int tblIndex = run*MPEGVALMAX + absVal;

		// Check for the two escape codes 
		if (VLCB15Len[tblIndex] == MPEGESCALEN)
		{
			m_SerialStore.PutBit(VLCESCAPECODE,VLCESCSIZEBITS); // Escape code
			m_SerialStore.PutBit(run,VLCRUNSIZEBITS); // 6 bit code for run length
			m_SerialStore.PutBit(ACval,VLCVALSIZEBITS); // 12 bits for the value
		}
		else
		{
			int valCode = VLCB15Code[tblIndex] + polarity;
			m_SerialStore.PutBit(valCode,VLCB15Len[tblIndex]);
		}

	}
	else
	{
		// Not in the table so it needs an escape code
		m_SerialStore.PutBit(VLCESCAPECODE,VLCESCSIZEBITS); // Escape code
		m_SerialStore.PutBit(run,VLCRUNSIZEBITS); // 6 bit code for run length
		m_SerialStore.PutBit(ACval,VLCVALSIZEBITS); // 12 bits for the value
	}

}

////////////////////////////////////////////////////////////////////////////////////////
// Finds the number of bits needed to represent a positive value
//
//  Input parameters :
//		val :  val to check, should always be > 0
//
//	Output : returns the number of bits needed to represent the value (i.e., the location of
//			 the highest non-zero bit
//
//
unsigned int  CAVVideoCompression::FindMinBits(unsigned int val)
{


	// Find out how many bits are in the value
	unsigned len = 0;
	unsigned int newval = val;
	while(newval > 0) 
	{
		if(newval < MPEGLOGTBLSIZE) return(len + LogSizeTable[newval]);

		len += MPEGLOGTBLBITS;
		newval = newval >> MPEGLOGTBLBITS;
	}

	return(len);  // to remove compiler warning
}


////////////////////////////////////////////////////////////////////////////////////////
// Extract a macroblock from a planer frame
//
//  Input parameters :
//		x : x index of the macroblock
//		y : y index of the macroblcok
//		pMBbuff : Pointer to a buffer that will hold the extracted macroblock (384 shorts = 64 Luma0 + 64 Luma1 +64 Luma2 +64 Luma3 + 64 CR + 64 Cb values)
//		pFrameBuffer : Pointer to a buffer that holds the planer frame
//
//	Output : None
//
//
void CAVVideoCompression::getMB(unsigned int x, unsigned int y,short *pMBbuff,uint8 *pFrameBuffer)
{
	assert(pMBbuff);
	assert(pFrameBuffer);


	short *Ybase = (short *) pFrameBuffer;
	short *CbBase = Ybase + m_encodedWidth*m_encodedHeight;
	short *CrBase = CbBase + m_encodedWidth*m_encodedHeight/CHROMAPLANERRATIO;

	short *sourceBuff0 = Ybase + y*LUMAMBWIDTH*m_encodedWidth + x*LUMAMBWIDTH;
	short *pBuff0 = pMBbuff;
	short *pBuff1 = pBuff0 + BLOCK8x8SIZE;
	short *pBuff2 = pBuff1 + BLOCK8x8SIZE;
	short *pBuff3 = pBuff2 + BLOCK8x8SIZE;

    unsigned int offset1 = BLOCKDIM;
	unsigned int offset2 = BLOCKDIM*m_encodedWidth;
	unsigned int offset3 = BLOCKDIM*m_encodedWidth + BLOCKDIM;

	// Get the 4 8x8 luma blocks
	for(unsigned int i=0;i<BLOCKDIM;++i) 
	{
		for(unsigned int j=0;j<BLOCKDIM;++j) 
		{
			*pBuff0 = sourceBuff0[0];
			*pBuff1 = sourceBuff0[offset1];
			*pBuff2 = sourceBuff0[offset2];
			*pBuff3 = sourceBuff0[offset3];

			++sourceBuff0;
			++pBuff0;
			++pBuff1;
			++pBuff2;
			++pBuff3;
		}
		sourceBuff0 = sourceBuff0 - BLOCKDIM + m_encodedWidth;
	}

	// Get the Cr and Cb blocks
	sourceBuff0 = CbBase + y*4*m_encodedWidth  + x*CHROMAMBWIDTH;
	short *sourceBuff1 = CrBase + y*4*m_encodedWidth  + x*CHROMAMBWIDTH;
	pBuff0 = pMBbuff + 4*BLOCK8x8SIZE;
	pBuff1 = pMBbuff + 5*BLOCK8x8SIZE;
	unsigned int chromaStride = m_encodedWidth >> 1;

	for(unsigned int i=0;i<BLOCKDIM;++i) 
	{
		for(unsigned int j=0;j<BLOCKDIM;++j)
		{
			*pBuff0 = *sourceBuff0;
			*pBuff1 = *sourceBuff1;
			++sourceBuff0;
			++sourceBuff1;
			++pBuff0;
			++pBuff1;
		}
		sourceBuff0 = sourceBuff0 - BLOCKDIM + chromaStride;
		sourceBuff1 = sourceBuff1 - BLOCKDIM + chromaStride;

	}
}


////////////////////////////////////////////////////////////////////////////////////////
// Extract a macroblock from a planer frame
//
//  Input parameters :
//		x : x pixel location of the macroblock
//		y : y pixel location of the macroblock
//		pMBbuff : Pointer to a buffer that will hold the extracted macroblock (384 shorts = 64 Luma0 + 64 Luma1 +64 Luma2 +64 Luma3 + 64 CR + 64 Cb values)
//		pFrameBuffer : Pointer to a buffer that holds the planer frame
//
//	Output : None
//
//
void CAVVideoCompression::GetBlock(unsigned int x, unsigned int y,short *pMBbuff,uint8 *pFrameBuffer)
{
	assert(pMBbuff);
	assert(pFrameBuffer);


	short *Ybase = (short *) pFrameBuffer;
	short *CbBase = Ybase + m_encodedWidth*m_encodedHeight;
	short *CrBase = Ybase + 5*m_encodedWidth*m_encodedHeight/4;

	short *sourceBuff0 = Ybase + y*m_encodedWidth + x;
	short *pBuff0 = pMBbuff;
	short *pBuff1 = pBuff0 + BLOCK8x8SIZE;
	short *pBuff2 = pBuff1 + BLOCK8x8SIZE;
	short *pBuff3 = pBuff2 + BLOCK8x8SIZE;

	unsigned int offset1 = BLOCKDIM;
	unsigned int offset2 = BLOCKDIM*m_encodedWidth;
	unsigned int offset3 = BLOCKDIM*m_encodedWidth + BLOCKDIM;

	// Get the 4 8x8 luma blocks
	for(unsigned int i=0;i<BLOCKDIM;++i) 
	{
		for(unsigned int j=0;j<BLOCKDIM;++j) 
		{
			*pBuff0 = sourceBuff0[0];
			*pBuff1 = sourceBuff0[offset1];
			*pBuff2 = sourceBuff0[offset2];
			*pBuff3 = sourceBuff0[offset3];

			++sourceBuff0;
			++pBuff0;
			++pBuff1;
			++pBuff2;
			++pBuff3;
		}
		sourceBuff0 = sourceBuff0 - BLOCKDIM + m_encodedWidth;
	}

	// Get the Cr and Cb blocks
	int chromaStride = m_encodedWidth >> 1;
	int chromaY = y >> 1;
	int chromaX = x >> 1;
	sourceBuff0 = CbBase + chromaY*chromaStride  + chromaX;
	short *sourceBuff1 = CrBase + chromaY*chromaStride  + chromaX;
	pBuff0 = pMBbuff + 4*BLOCK8x8SIZE;
	pBuff1 = pMBbuff + 5*BLOCK8x8SIZE;

	for(unsigned int i=0;i<BLOCKDIM;++i) 
	{
		for(unsigned int j=0;j<BLOCKDIM;++j)
		{
			*pBuff0 = *sourceBuff0;
			*pBuff1 = *sourceBuff1;
			++sourceBuff0;
			++sourceBuff1;
			++pBuff0;
			++pBuff1;
		}
		sourceBuff0 = sourceBuff0 - BLOCKDIM + chromaStride;
		sourceBuff1 = sourceBuff1 - BLOCKDIM + chromaStride;
	}
}



////////////////////////////////////////////////////////////////////////////////////////
// Place a decoded macroblock into the decoded frame buffer
//
//  Input parameters : 
//				x  -  horizontal pixel location of the macroblock
//				y  -  vertical pixel location of the macroblock
//				pMBData  -  Pointer to the decoded macroblock data
//				pFrameBuffer  -  Pointer to the framebuffer
//				dataType  -  0 = short, 1 = int
//
//	Output : none
//
//
void CAVVideoCompression::PlaceMB(unsigned int x, unsigned int y,uint8 *pMBData,unsigned char *pFrameBuffer,int dataType)
{

	assert(pMBData);
	assert(pFrameBuffer);


	short *Ybase = (short *) pFrameBuffer;
	short *CbBase = Ybase + m_encodedWidth*m_encodedHeight;
	short *CrBase = CbBase + (m_encodedWidth*m_encodedHeight >> 2);
	short *Yaddr;


	if(dataType == 1)
	{
		// put in the first Y sub-block
		Yaddr = Ybase + y*LUMAMBWIDTH*m_encodedWidth + x*LUMAMBWIDTH;
		int *MBaddr = (int *) pMBData;
		for(unsigned int i=0;i<BLOCKDIM;++i)
		{
			for(unsigned int j=0;j<BLOCKDIM;++j) Yaddr[j] = (short) (MBaddr[j] & 0xFFFF);
			Yaddr += m_encodedWidth;
			MBaddr += BLOCKDIM;
		}

		// put in the second Y sub-block
		Yaddr = Ybase + y*LUMAMBWIDTH*m_encodedWidth + x*LUMAMBWIDTH + BLOCKDIM;
		MBaddr = ((int *) pMBData) + BLOCK8x8SIZE;
		for(unsigned int i=0;i<BLOCKDIM;++i)
		{
			for(unsigned int j=0;j<BLOCKDIM;++j) Yaddr[j] = (short) (MBaddr[j] & 0xFFFF);
			Yaddr += m_encodedWidth;
			MBaddr += BLOCKDIM;
		}


		// put in the third Y sub-block
		Yaddr = Ybase + (y*LUMAMBWIDTH + BLOCKDIM)*m_encodedWidth + x*LUMAMBWIDTH;
		MBaddr = ((int *) pMBData) + 2*BLOCK8x8SIZE;
		for(unsigned int i=0;i<BLOCKDIM;++i)
		{
			for(unsigned int j=0;j<BLOCKDIM;++j) Yaddr[j] = (short) (MBaddr[j] & 0xFFFF);
			Yaddr += m_encodedWidth;
			MBaddr += BLOCKDIM;
		}

		// put in the fourth Y sub-block
		Yaddr = Ybase + (y*LUMAMBWIDTH + BLOCKDIM)*m_encodedWidth + x*LUMAMBWIDTH + BLOCKDIM;
		MBaddr = ((int *) pMBData) + 3*BLOCK8x8SIZE;
		for(unsigned int i=0;i<BLOCKDIM;++i)
		{
			for(unsigned int j=0;j<BLOCKDIM;++j) Yaddr[j] = (short) (MBaddr[j] & 0xFFFF);
			Yaddr += m_encodedWidth;
			MBaddr += BLOCKDIM;
		}



		// put in the Cb sub-block
		short *Cbaddr = CbBase + y*4*m_encodedWidth  + x*CHROMAMBWIDTH;
		MBaddr = ((int *) pMBData) + 4*BLOCK8x8SIZE;
		for(unsigned int i=0;i<BLOCKDIM;++i)
		{
			for(unsigned int j=0;j<BLOCKDIM;++j) Cbaddr[j] = (short) (MBaddr[j] & 0xFFFF);
			Cbaddr += m_encodedWidth/2;
			MBaddr += BLOCKDIM;
		}


		// put in the Cr sub-block
		short *Craddr = CrBase + y*4*m_encodedWidth  + x*CHROMAMBWIDTH;
		MBaddr = ((int *) pMBData) + 5*BLOCK8x8SIZE;
		for(unsigned int i=0;i<BLOCKDIM;++i)
		{
			for(unsigned int j=0;j<BLOCKDIM;++j) Craddr[j] = (short) (MBaddr[j] & 0xFFFF);
			Craddr += m_encodedWidth/2;
			MBaddr += BLOCKDIM;
		}
	}
	else
	{
		// put in the first Y sub-block
		Yaddr = Ybase + y*LUMAMBWIDTH*m_encodedWidth + x*LUMAMBWIDTH;
		short *MBaddrs = (short *) pMBData;
		for(unsigned int i=0;i<BLOCKDIM;++i)
		{
			for(unsigned int j=0;j<BLOCKDIM;++j) Yaddr[j] = (short) (MBaddrs[j] & 0xFFFF);
			Yaddr += m_encodedWidth;
			MBaddrs += BLOCKDIM;
		}

		// put in the second Y sub-block
		Yaddr = Ybase + y*LUMAMBWIDTH*m_encodedWidth + x*LUMAMBWIDTH + BLOCKDIM;
		MBaddrs = ((short *) pMBData) + BLOCK8x8SIZE;
		for(unsigned int i=0;i<BLOCKDIM;++i)
		{
			for(unsigned int j=0;j<BLOCKDIM;++j) Yaddr[j] = (short) (MBaddrs[j] & 0xFFFF);
			Yaddr += m_encodedWidth;
			MBaddrs += BLOCKDIM;
		}


		// put in the third Y sub-block
		Yaddr = Ybase + (y*LUMAMBWIDTH + BLOCKDIM)*m_encodedWidth + x*LUMAMBWIDTH;
		MBaddrs = ((short *) pMBData) + 2*BLOCK8x8SIZE;
		for(unsigned int i=0;i<BLOCKDIM;++i)
		{
			for(unsigned int j=0;j<BLOCKDIM;++j) Yaddr[j] = (short) (MBaddrs[j] & 0xFFFF);
			Yaddr += m_encodedWidth;
			MBaddrs += BLOCKDIM;
		}

		// put in the fourth Y sub-block
		Yaddr = Ybase + (y*LUMAMBWIDTH + BLOCKDIM)*m_encodedWidth + x*LUMAMBWIDTH + BLOCKDIM;
		MBaddrs = ((short *) pMBData) + 3*BLOCK8x8SIZE;
		for(unsigned int i=0;i<BLOCKDIM;++i)
		{
			for(unsigned int j=0;j<BLOCKDIM;++j) Yaddr[j] = (short) (MBaddrs[j] & 0xFFFF);
			Yaddr += m_encodedWidth;
			MBaddrs += BLOCKDIM;
		}



		// put in the Cb sub-block
		short *Cbaddr = CbBase + y*4*m_encodedWidth  + x*CHROMAMBWIDTH;
		MBaddrs = ((short *) pMBData) + 4*BLOCK8x8SIZE;
		for(unsigned int i=0;i<BLOCKDIM;++i)
		{
			for(unsigned int j=0;j<BLOCKDIM;++j) Cbaddr[j] = (short) (MBaddrs[j] & 0xFFFF);
			Cbaddr += m_encodedWidth/2;
			MBaddrs += BLOCKDIM;
		}


		// put in the Cr sub-block
		short *Craddr = CrBase + y*4*m_encodedWidth  + x*CHROMAMBWIDTH;
		MBaddrs = ((short *) pMBData) + 5*BLOCK8x8SIZE;
		for(unsigned int i=0;i<BLOCKDIM;++i)
		{
			for(unsigned int j=0;j<BLOCKDIM;++j) Craddr[j] = (short) (MBaddrs[j] & 0xFFFF);
			Craddr += m_encodedWidth/2;
			MBaddrs += BLOCKDIM;
		}
	}


}


////////////////////////////////////////////////////////////////////////////////////////
// Convert a macroblock from x,y ordering to frequency scan order
//
//  Input parameters :
//		din : Pointer to the input block (64 shorts)
//		dout : Pointer to the output block which will hold the reordered macroblock
//		MBType : type of macroblock
//
//	Output : returns the index to the last non-zero value in the macroblock
//
//
unsigned int CAVVideoCompression::ConvertScanOrder(short *pDin,short *pDout,int MBType)
{

	assert(pDin);
	assert(pDout);

	// zig-zag scan pattern defined in ISO 13828-2 sec. 7.3
	//  00 01 05 06 14 15 27 28
	//  02 04 07 13 16 26 29 42
	//  03 08 12 17 25 30 41 43
	//  09 11 18 24 31 40 44 53
	//  10 19 23 32 39 45 52 54
	//  20 22 33 38 46 51 55 60
	//  21 34 37 47 50 56 59 61
	//  35 36 48 49 57 58 62 63


	int minval = -m_coeffLimit;
	int maxval = m_coeffLimit;

	int startLoc = 0;
	if(MBType == MB_INTRA) 
	{
		startLoc = 1;
		pDout[0] = pDin[0];  // Get the DC value, no clipping
	}


	// Get the AC values
	unsigned int lastnonzero = -1;
	for(unsigned int i = startLoc;i<BLOCK8x8SIZE;++i) 
	{
		short temp = pDin[ZigZagScan[i]]; 
		temp = CLAMP(temp,minval,maxval);
		if(temp) lastnonzero = i;
		pDout[i] = temp;
	}

	return(lastnonzero);

}

////////////////////////////////////////////////////////////////////////////////////////
// Routine that performs a forward 8x8 DCT followed by quantization on a single macroblock
// Uses SSE2 if it is available
//
//  Input parameters :
//		pDataIn				:  pointer to the input macroblock		(64 luma0 + 64 luma1 + 64 luma2 + 64 luma3 + 64 Cr + 64 Cb)
//		pQuantizedOutput	:  pointer to hold the processed data	(64 luma0 + 64 luma1 + 64 luma2 + 64 luma3 + 64 Cr + 64 Cb)
//		MBType				: type of macroblock
//
//	Output : none
//
//
void CAVVideoCompression::ForwardDctQuantize(short *pDatain, short *pQuantizedOutput,int MBType)
{


	assert(pDatain);
	assert(pQuantizedOutput);


	// Based on the hardware available, use SSE2 or not
	short *pIn = pDatain;
	short *pOut = pQuantizedOutput;


	if(MBType == MB_INTRA)
	{
		for(unsigned int i=0;i<NUMSUBBLOCKS;++i)
		{

			if(m_simdTypeAvail == SSE_AVAIL)
			{
				ForwardDctSSE2(pIn);
				QuantizeIntraSSE2(pIn,pOut);
			}
			else
			{
				ForwardDct(pIn);
				QuantizeIntra(pIn,pOut);
			}

			pIn += BLOCK8x8SIZE;
			pOut += BLOCK8x8SIZE;
		}
	}
	else
	{
		for(unsigned int i=0;i<NUMSUBBLOCKS;++i)
		{

			ForwardDct(pIn);
			QuantizeInter(pIn,pOut);

			pIn += BLOCK8x8SIZE;
			pOut += BLOCK8x8SIZE;
		}
	}

}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Perform an in-place forward DCT on a 64 sample block of data
//  Based on the following paper  :  C. Loeffler, A. Ligtenberg and G. Moschytz, 
//  "Practical Fast 1-D DCT Algorithms with 11 Multiplications", Proc. Int'l. Conf. on Acoustics,
//  Speech, and Signal Processing 1989 (ICASSP '89), pp. 988-991.
//
//  Input parameters :
//		pData : pointer to the input data, output data will be placed here as well
//
//	Output : none
//
void CAVVideoCompression::ForwardDct(short *pData)
{

	assert(pData);

	// Transform the rows 
	short *dataptr = pData;
	for(unsigned int i=0;i<BLOCKDIM;++i) 
	{

		int tmp0 = dataptr[0] + dataptr[7];
		int tmp7 = dataptr[0] - dataptr[7];
		int tmp1 = dataptr[1] + dataptr[6];
		int tmp6 = dataptr[1] - dataptr[6];
		int tmp2 = dataptr[2] + dataptr[5];
		int tmp5 = dataptr[2] - dataptr[5];
		int tmp3 = dataptr[3] + dataptr[4];
		int tmp4 = dataptr[3] - dataptr[4];

		int tmp10 = tmp0 + tmp3;        
		int tmp13 = tmp0 - tmp3;
		int tmp11 = tmp1 + tmp2;
		int tmp12 = tmp1 - tmp2;

		// Get the even values (0,2,4,6)
		dataptr[0] = (short) ((tmp10 + tmp11) << DCT_SCALE1);
		dataptr[4] = (short) ((tmp10 - tmp11) << DCT_SCALE1);

		int tmp14 = ((tmp12 + tmp13)*DCT_C1);

		dataptr[2] = (short) ((tmp14 + tmp13*DCT_C2) >> DCT_SCALE2);
		dataptr[6] = (short) ((tmp14 - tmp12*DCT_C3) >> DCT_SCALE2);

		// Get the odd values (1,3,5,7)
		tmp11 = tmp4 + tmp7;      
		tmp12 = tmp5 + tmp6;
		tmp13 = tmp4 + tmp6;
		tmp14 = tmp5 + tmp7;
		int tmp15 = (tmp13 + tmp14) *  DCT_C4;


		tmp4 = tmp4 * DCT_C5;
		tmp5 = tmp5 * DCT_C6;
		tmp6 = tmp6 * DCT_C7;
		tmp7 = tmp7 * DCT_C8;

		tmp11 = tmp11 * DCT_C9;
		tmp12 = tmp12 * DCT_C10;
		tmp13 = tmp13 * DCT_C11;
		tmp14 = tmp14 * DCT_C12;

		tmp13 = tmp13 + tmp15;
		tmp14 = tmp14 + tmp15;
        
		dataptr[7] = (short) ((tmp4 + tmp11 + tmp13) >> DCT_SCALE2);
		dataptr[5] = (short) ((tmp5 + tmp12 + tmp14) >> DCT_SCALE2);
		dataptr[3] = (short) ((tmp6 + tmp12 + tmp13) >> DCT_SCALE2);
		dataptr[1] = (short) ((tmp7 + tmp11 + tmp14) >> DCT_SCALE2);

		dataptr += BLOCKDIM;
	}


	// Transform the columns
	dataptr = pData;
	for (unsigned int i=0;i<BLOCKDIM;++i) 
	{

		int tmp0 = dataptr[BLOCKDIM*0] + dataptr[BLOCKDIM*7];
		int tmp7 = dataptr[BLOCKDIM*0] - dataptr[BLOCKDIM*7];
		int tmp1 = dataptr[BLOCKDIM*1] + dataptr[BLOCKDIM*6];
		int tmp6 = dataptr[BLOCKDIM*1] - dataptr[BLOCKDIM*6];
		int tmp2 = dataptr[BLOCKDIM*2] + dataptr[BLOCKDIM*5];
		int tmp5 = dataptr[BLOCKDIM*2] - dataptr[BLOCKDIM*5];
		int tmp3 = dataptr[BLOCKDIM*3] + dataptr[BLOCKDIM*4];
		int tmp4 = dataptr[BLOCKDIM*3] - dataptr[BLOCKDIM*4];

		int tmp10 = tmp0 + tmp3;        
		int tmp13 = tmp0 - tmp3;
		int tmp11 = tmp1 + tmp2;
		int tmp12 = tmp1 - tmp2;

		// Get the even values (0,2,4,6)
		dataptr[BLOCKDIM*0] = (short) ((tmp10 + tmp11 + DCT_ROUND3) >> DCT_SCALE3);
		dataptr[BLOCKDIM*4] = (short) ((tmp10 - tmp11 + DCT_ROUND3) >> DCT_SCALE3);

		int tmp14 = ((tmp12 + tmp13)*DCT_C1);

		dataptr[BLOCKDIM*2] = (short) ((tmp14 + tmp13*DCT_C2 + DCT_ROUND4) >> DCT_SCALE4);   
		dataptr[BLOCKDIM*6] = (short) ((tmp14 - tmp12*DCT_C3 + DCT_ROUND4) >> DCT_SCALE4);

		// Get the odd values (1,3,5,7)
		tmp11 = tmp4 + tmp7;      
		tmp12 = tmp5 + tmp6;
		tmp13 = tmp4 + tmp6;
		tmp14 = tmp5 + tmp7;
		int tmp15 = (tmp13 + tmp14) *  DCT_C4;

		tmp4 = tmp4 * DCT_C5;
		tmp5 = tmp5 * DCT_C6;
		tmp6 = tmp6 * DCT_C7;
		tmp7 = tmp7 * DCT_C8;

		tmp11 = tmp11 * DCT_C9;
		tmp12 = tmp12 * DCT_C10;
		tmp13 = tmp13 * DCT_C11;
		tmp14 = tmp14 * DCT_C12;

		tmp13 = tmp13 + tmp15;
		tmp14 = tmp14 + tmp15;

		dataptr[BLOCKDIM*7] = (short) ((tmp4 + tmp11 + tmp13 + DCT_ROUND4) >> DCT_SCALE4);
		dataptr[BLOCKDIM*5] = (short) ((tmp5 + tmp12 + tmp14 + DCT_ROUND4) >> DCT_SCALE4);
		dataptr[BLOCKDIM*3] = (short) ((tmp6 + tmp12 + tmp13 + DCT_ROUND4) >> DCT_SCALE4);
		dataptr[BLOCKDIM*1] = (short) ((tmp7 + tmp11 + tmp14 + DCT_ROUND4) >> DCT_SCALE4);


		++dataptr;                 
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Perform an in-place forward DCT on a 64 sample block of data using SSE
//  Based off of the algorithm shown in Y. Arai, T.Agui and M. Nakajima, "A Fast DCT-SQ Scheme for Images",
//  Transactions of the IEICE, vol. E71, no. 11, p. 1095, Nov. 1988.
//  and 
//  C. Loeffler, A. Ligtenberg and G. Moschytz, 
//  "Practical Fast 1-D DCT Algorithms with 11 Multiplications", Proc. Int'l. Conf. on Acoustics,
//  Speech, and Signal Processing 1989 (ICASSP '89), pp. 988-991.
//
//  Input parameters :
//		pData : pointer to the input data, output data will also be placed here
//
//	Output : none
//
//  Note : this routine is speed critical and so contains no looping or branches
//
//
void CAVVideoCompression::ForwardDctSSE2(short *pData)
{

	assert(pData);



#if defined(WIN32) || defined(WIN64)



	// Load some scaling and DCT multiplier constants
	__m128 c_cos6 = _mm_load1_ps(&c_cos6_c);
	__m128 c_sqrt2inv = _mm_load1_ps(&c_sqrt2inv_c);
	__m128 c_msqrt2inv = _mm_load1_ps(&c_msqrt2inv_c);
	__m128 c_cos6xsqrt2 =  _mm_load1_ps(&c_cos6xsqrt2_c);
	__m128 c_cos2xsqrt2 = _mm_load1_ps(&c_cos2xsqrt2_c);
	__m128 c_roundvec = _mm_load1_ps(&c_roundvec_c);

	__m128 dftscale0a = _mm_load_ps(dftscale0a_c);
	__m128 dftscale0b = _mm_load_ps(dftscale0b_c);
	__m128 dftscale1a = _mm_load_ps(dftscale1a_c);
	__m128 dftscale1b = _mm_load_ps(dftscale1b_c);
	__m128 dftscale2a = _mm_load_ps(dftscale2a_c);
	__m128 dftscale2b = _mm_load_ps(dftscale2b_c);
	__m128 dftscale3a = _mm_load_ps(dftscale3a_c);
	__m128 dftscale3b = _mm_load_ps(dftscale3b_c);
	__m128 dftscale4a = _mm_load_ps(dftscale4a_c);
	__m128 dftscale4b = _mm_load_ps(dftscale4b_c);
	__m128 dftscale5a = _mm_load_ps(dftscale5a_c);
	__m128 dftscale5b = _mm_load_ps(dftscale5b_c);
	__m128 dftscale6a = _mm_load_ps(dftscale6a_c);
	__m128 dftscale6b = _mm_load_ps(dftscale6b_c);
	__m128 dftscale7a = _mm_load_ps(dftscale7a_c);
	__m128 dftscale7b = _mm_load_ps(dftscale7b_c);


	// Load the 8x8 block from memory
	__m128i m0 = _mm_load_si128((__m128i *) (pData+BLOCKDIM*0));  // m0 = 00 01 02 03 04 05 06 07
	__m128i m1 = _mm_load_si128((__m128i *) (pData+BLOCKDIM*1));  // m1 = 10 11 12 13 14 15 16 17
	__m128i m2 = _mm_load_si128((__m128i *) (pData+BLOCKDIM*2));  // m1 = 20 21 22 23 24 25 26 27
	__m128i m3 = _mm_load_si128((__m128i *) (pData+BLOCKDIM*3));  // m1 = 30 31 32 33 34 35 36 37
	__m128i m4 = _mm_load_si128((__m128i *) (pData+BLOCKDIM*4));  // m1 = 40 41 42 43 44 45 46 47
	__m128i m5 = _mm_load_si128((__m128i *) (pData+BLOCKDIM*5));  // m1 = 50 51 52 53 54 55 56 57
	__m128i m6 = _mm_load_si128((__m128i *) (pData+BLOCKDIM*6));  // m1 = 60 61 62 63 64 65 66 67
	__m128i m7 = _mm_load_si128((__m128i *) (pData+BLOCKDIM*7));  // m1 = 70 71 72 73 74 75 76 77


	// First process is to transform the rows.  Since the SSE works on columns, this requires a row/column transpose
	// so first do a row column transpose
	__m128i temp0 = _mm_unpacklo_epi16 (m0,m1);  // temp0 = 00 10 01 11 02 12 03 13
	__m128i temp1 = _mm_unpackhi_epi16 (m0,m1);  // temp1 = 04 14 05 15 06 16 07 17
	__m128i temp2 = _mm_unpacklo_epi16 (m2,m3);  // temp2 = 20 30 21 31 22 32 23 33
	__m128i temp3 = _mm_unpackhi_epi16 (m2,m3);  // temp3 = 24 34 25 35 26 36 27 37
	__m128i temp4 = _mm_unpacklo_epi16 (m4,m5);  // temp4 = 40 50 41 51 42 52 43 53
	__m128i temp5 = _mm_unpackhi_epi16 (m4,m5);  // temp5 = 44 54 45 55 46 56 47 57
	__m128i temp6 = _mm_unpacklo_epi16 (m6,m7);  // temp6 = 60 70 61 71 62 72 63 73
	__m128i temp7 = _mm_unpackhi_epi16 (m6,m7);  // temp7 = 64 74 65 75 66 76 67 77


	__m128i temp8 = _mm_unpacklo_epi32 (temp0,temp2);  // temp8 = 00 10 20 30 01 11 21 31
	__m128i temp9 = _mm_unpackhi_epi32 (temp0,temp2);  // temp9 = 02 12 22 32 03 13 23 33

	__m128i temp10 = _mm_unpacklo_epi32 (temp1,temp3);  // temp10 = 04 14 24 34 05 15 25 35
	__m128i temp11 = _mm_unpackhi_epi32 (temp1,temp3);  // temp11 = 06 16 26 36 07 17 27 37

	__m128i temp12 = _mm_unpacklo_epi32 (temp4,temp6);  // temp12 = 40 50 60 70 41 51 61 71
	__m128i temp13 = _mm_unpackhi_epi32 (temp4,temp6);  // temp13 = 42 52 62 72 43 53 63 73

	__m128i temp14 = _mm_unpacklo_epi32 (temp5,temp7);  // temp14 = 44 54 64 74 45 55 65 75
	__m128i temp15 = _mm_unpackhi_epi32 (temp5,temp7);  // temp15 = 46 56 66 76 47 57 67 77



	__m128i temp0B = _mm_unpacklo_epi64 (temp8,temp12);   // temp16 = 00 10 20 30 40 50 60 70
	__m128i temp1B = _mm_unpackhi_epi64 (temp8,temp12);   // temp17 = 01 11 21 31 41 51 61 71
	__m128i temp2B = _mm_unpacklo_epi64 (temp9,temp13);   // temp18 = 02 12 22 32 42 52 62 72
	__m128i temp3B = _mm_unpackhi_epi64 (temp9,temp13);   // temp19 = 03 13 23 33 43 53 63 73
	__m128i temp4B = _mm_unpacklo_epi64 (temp10,temp14);  // temp20 = 04 14 24 34 44 54 64 74
	__m128i temp5B = _mm_unpackhi_epi64 (temp10,temp14);  // temp21 = 05 15 25 35 45 55 65 75
	__m128i temp6B = _mm_unpacklo_epi64 (temp11,temp15);  // temp22 = 06 16 26 36 46 56 66 76
	__m128i temp7B = _mm_unpackhi_epi64 (temp11,temp15);  // temp23 = 07 17 27 37 47 57 67 77


	// Now do the row transform
	// Transform is done in single precision floating point
	temp0 = _mm_adds_epi16(temp0B,temp7B);
	temp7 = _mm_sub_epi16(temp0B,temp7B);
	temp1 = _mm_adds_epi16(temp1B,temp6B);
	temp6 = _mm_sub_epi16(temp1B,temp6B);
	temp2 = _mm_adds_epi16(temp2B,temp5B);
	temp5 = _mm_sub_epi16(temp2B,temp5B); 
	temp3 = _mm_adds_epi16(temp3B,temp4B);
	temp4 = _mm_sub_epi16(temp3B,temp4B);

	temp10 = _mm_adds_epi16(temp0,temp3);
	temp13 = _mm_sub_epi16(temp0,temp3);
	temp11 = _mm_adds_epi16(temp1,temp2);
	temp12 = _mm_sub_epi16(temp1,temp2);

	// Do the even part (0,2,4,6)
	__m128i d0 = _mm_adds_epi16(temp10,temp11);
	__m128i tmpa12 = _mm_srai_epi32(d0,16);
	__m128  xfa0 = _mm_cvtepi32_ps(tmpa12);


	        d0 = _mm_slli_si128(d0,2);
	__m128i tmpb12 = _mm_srai_epi32(d0,16);
	__m128  xfb0 = _mm_cvtepi32_ps(tmpb12);


	__m128i d4 = _mm_sub_epi16(temp10,temp11);
	        tmpa12 = _mm_srai_epi32(d4,16);
	__m128  xfa4 = _mm_cvtepi32_ps(tmpa12);

	        d4 = _mm_slli_si128 (d4,2);
	        tmpb12 = _mm_srai_epi32(d4,16);
	__m128  xfb4 = _mm_cvtepi32_ps(tmpb12);


	__m128i z1 = _mm_adds_epi16(temp12,temp13);
	        tmpa12 = _mm_srai_epi32(z1,16);
	__m128  zaf1 = _mm_cvtepi32_ps(tmpa12);

	       z1 = _mm_slli_si128 (z1,2);
	       tmpb12 = _mm_srai_epi32(z1,16);
	__m128 zbf1 = _mm_cvtepi32_ps(tmpb12);

	zaf1 = _mm_mul_ps(zaf1,c_sqrt2inv);
	zbf1 = _mm_mul_ps(zbf1,c_sqrt2inv);

	tmpa12 = _mm_srai_epi32(temp13,16);
	__m128 tempa13 = _mm_cvtepi32_ps(tmpa12);

	temp13 = _mm_slli_si128 (temp13,2);
	tmpb12 = _mm_srai_epi32(temp13,16);
	__m128 tempb13 = _mm_cvtepi32_ps(tmpb12);

	__m128 xfa2 = _mm_add_ps(tempa13,zaf1);
	__m128 xfb2 = _mm_add_ps(tempb13,zbf1);
	__m128 xfa6 = _mm_sub_ps(tempa13,zaf1);
	__m128 xfb6 = _mm_sub_ps(tempb13,zbf1);


	// Do the odd part (1,3,5,7)
	temp10 = _mm_adds_epi16(temp4,temp5);
	temp11 = _mm_adds_epi16(temp5,temp6);
	temp12 = _mm_adds_epi16(temp6,temp7);


	__m128i z5 = _mm_sub_epi16(temp10,temp12);
	tmpa12 = _mm_srai_epi32(z5,16);
	__m128 zaf5 = _mm_cvtepi32_ps(tmpa12);


	z5 = _mm_slli_si128 (z5,2);
	tmpb12 = _mm_srai_epi32(z5,16);
	__m128 zbf5 = _mm_cvtepi32_ps(tmpb12);


	zaf5 = _mm_mul_ps(zaf5,c_cos6);
	zbf5 = _mm_mul_ps(zbf5,c_cos6);


	tmpa12 = _mm_srai_epi32(temp10,16);
	__m128 tempa10 = _mm_cvtepi32_ps(tmpa12);

	temp10 = _mm_slli_si128 (temp10,2);
	tmpb12 = _mm_srai_epi32(temp10,16);
	__m128 tempb10 = _mm_cvtepi32_ps(tmpb12);

	tempa10 = _mm_mul_ps(tempa10,c_cos6xsqrt2);
	__m128 zaf2 = _mm_add_ps(tempa10,zaf5);

	tempb10 = _mm_mul_ps(tempb10,c_cos6xsqrt2);
	__m128 zbf2 = _mm_add_ps(tempb10,zbf5);

	tmpa12 = _mm_srai_epi32(temp12,16);
	__m128 tempa12 = _mm_cvtepi32_ps(tmpa12);

	temp12 = _mm_slli_si128(temp12,2);
	tmpb12 = _mm_srai_epi32(temp12,16);
	__m128 tempb12 = _mm_cvtepi32_ps(tmpb12);


	tempa12 = _mm_mul_ps(tempa12,c_cos2xsqrt2);
	__m128 zaf4 = _mm_add_ps(tempa12,zaf5);

	tempb12 = _mm_mul_ps(tempb12,c_cos2xsqrt2);
	__m128 zbf4 = _mm_add_ps(tempb12,zbf5);


	tmpa12 = _mm_srai_epi32(temp11,16);
	__m128 tempa11 = _mm_cvtepi32_ps(tmpa12);

	temp11 = _mm_slli_si128 (temp11,2);
	tmpb12 = _mm_srai_epi32(temp11,16);
	__m128 tempb11 = _mm_cvtepi32_ps(tmpb12);

	__m128 zaf3 = _mm_mul_ps(tempa11,c_sqrt2inv);
	__m128 zbf3 = _mm_mul_ps(tempb11,c_sqrt2inv);

	tmpa12 = _mm_srai_epi32(temp7,16);
	__m128 tempa07 = _mm_cvtepi32_ps(tmpa12);

	temp7 = _mm_slli_si128 (temp7,2);
	tmpb12 = _mm_srai_epi32(temp7,16);
	__m128 tempb07 = _mm_cvtepi32_ps(tmpb12);


	__m128 zaf11 = _mm_add_ps(tempa07,zaf3);
	__m128 zbf11 = _mm_add_ps(tempb07,zbf3);

	__m128 zaf13 = _mm_sub_ps(tempa07,zaf3);
	__m128 zbf13 = _mm_sub_ps(tempb07,zbf3);
	__m128 xfa5 = _mm_add_ps(zaf13,zaf2);
	__m128 xfb5 = _mm_add_ps(zbf13,zbf2);

	__m128 xfa3 = _mm_sub_ps(zaf13,zaf2);
	__m128 xfb3 = _mm_sub_ps(zbf13,zbf2);

	__m128 xfa1 = _mm_add_ps(zaf11,zaf4);
	__m128 xfb1 = _mm_add_ps(zbf11,zbf4);

	__m128 xfa7 = _mm_sub_ps(zaf11,zaf4);
	__m128 xfb7 = _mm_sub_ps(zbf11,zbf4);


	// Do another transpose to go back to the original row/column orientation before doing the column transform
	_MM_TRANSPOSE4_PS(xfa0, xfa1, xfa2, xfa3)
	_MM_TRANSPOSE4_PS(xfb0, xfb1, xfb2, xfb3)
	_MM_TRANSPOSE4_PS(xfa4, xfa5, xfa6, xfa7)
	_MM_TRANSPOSE4_PS(xfb4, xfb5, xfb6, xfb7)

	// Correct the arrangement due to interleaved columns and transpose macro
	// This does no operations, it just does a renaming so that the following code makes sense
	__m128 dfa0 = xfb0;
	__m128 dfa1 = xfa0;
	__m128 dfa2 = xfb1;
	__m128 dfa3 = xfa1;

	__m128 dfa4 = xfb2;
	__m128 dfa5 = xfa2;
	__m128 dfa6 = xfb3;
	__m128 dfa7 = xfa3;

	__m128 dfb0 = xfb4;
	__m128 dfb1 = xfa4;
	__m128 dfb2 = xfb5;
	__m128 dfb3 = xfa5;

	__m128 dfb4 = xfb6;
	__m128 dfb5 = xfa6;
	__m128 dfb6 = xfb7;
	__m128 dfb7 = xfa7;


	//  Data is now in the following format (each vector holds 4 single precision values)
	//  dfa0   dfb0
	//  dfa1   dfb1
	//  dfa2   dfb2
	//  dfa3   dfb3
	//  dfa4   dfb4
	//  dfa5   dfb5
	//  dfa6   dfb6
	//  dfa7   dfb7



	// Now do the column transform
	__m128 tempa00 = _mm_add_ps(dfa0,dfa7);
	__m128 tempb00 = _mm_add_ps(dfb0,dfb7);
	tempa07 = _mm_sub_ps(dfa0,dfa7);
	tempb07 = _mm_sub_ps(dfb0,dfb7);
	__m128 tempa01 = _mm_add_ps(dfa1,dfa6);
	__m128 tempb01 = _mm_add_ps(dfb1,dfb6);
	__m128 tempa06 = _mm_sub_ps(dfa1,dfa6);
	__m128 tempb06 = _mm_sub_ps(dfb1,dfb6);
	__m128 tempa02 = _mm_add_ps(dfa2,dfa5);
	__m128 tempb02 = _mm_add_ps(dfb2,dfb5);
	__m128 tempa05 = _mm_sub_ps(dfa2,dfa5); 
	__m128 tempb05 = _mm_sub_ps(dfb2,dfb5); 
	__m128 tempa03 = _mm_add_ps(dfa3,dfa4);
	__m128 tempb03 = _mm_add_ps(dfb3,dfb4);
	__m128 tempa04 = _mm_sub_ps(dfa3,dfa4);
	__m128 tempb04 = _mm_sub_ps(dfb3,dfb4);

	tempa10 = _mm_add_ps(tempa00,tempa03);
	tempb10 = _mm_add_ps(tempb00,tempb03);
	tempa13 = _mm_sub_ps(tempa00,tempa03);
	tempb13 = _mm_sub_ps(tempb00,tempb03);
	tempa11 = _mm_add_ps(tempa01,tempa02);
	tempb11 = _mm_add_ps(tempb01,tempb02);
	tempa12 = _mm_sub_ps(tempa01,tempa02);
	tempb12 = _mm_sub_ps(tempb01,tempb02);

	// Do the even part (0,2,4,6)

	dfa0 = _mm_add_ps(tempa10,tempa11);
	dfb0 = _mm_add_ps(tempb10,tempb11);
	dfa4 = _mm_sub_ps(tempa10,tempa11);
	dfb4 = _mm_sub_ps(tempb10,tempb11);


	zaf1 = _mm_add_ps(tempa12,tempa13); 
	zbf1 = _mm_add_ps(tempb12,tempb13); 

	dfa2 = _mm_mul_ps(zaf1,c_sqrt2inv);
	dfa2 = _mm_add_ps(dfa2,tempa13);
	dfb2 = _mm_mul_ps(zbf1,c_sqrt2inv);
	dfb2 = _mm_add_ps(dfb2,tempb13);
	dfa6 = _mm_mul_ps(zaf1,c_msqrt2inv);
	dfa6 = _mm_add_ps(dfa6,tempa13);
	dfb6 = _mm_mul_ps(zbf1,c_msqrt2inv);
	dfb6 = _mm_add_ps(dfb6,tempb13);


	// Do the odd part (1,3,5,7)
	tempa10 = _mm_add_ps(tempa04,tempa05);
	tempb10 = _mm_add_ps(tempb04,tempb05);
	tempa11 = _mm_add_ps(tempa05,tempa06);
	tempb11 = _mm_add_ps(tempb05,tempb06);
	tempa12 = _mm_add_ps(tempa06,tempa07);
	tempb12 = _mm_add_ps(tempb06,tempb07);

	zaf5 = _mm_sub_ps(tempa10,tempa12);
	zbf5 = _mm_sub_ps(tempb10,tempb12);
	zaf5 = _mm_mul_ps(zaf5,c_cos6);
	zbf5 = _mm_mul_ps(zbf5,c_cos6);


	zaf2 = _mm_mul_ps(tempa10,c_cos6xsqrt2);
	zaf2 = _mm_add_ps(zaf2,zaf5);
	zbf2 = _mm_mul_ps(tempb10,c_cos6xsqrt2);
	zbf2 = _mm_add_ps(zbf2,zbf5);

	zaf4 = _mm_mul_ps(tempa12,c_cos2xsqrt2);
	zaf4 = _mm_add_ps(zaf4,zaf5);
	zbf4 = _mm_mul_ps(tempb12,c_cos2xsqrt2);
	zbf4 = _mm_add_ps(zbf4,zbf5);

	zaf3 = _mm_mul_ps(tempa11,c_sqrt2inv);
	zbf3 = _mm_mul_ps(tempb11,c_sqrt2inv);


	zaf11 = _mm_add_ps(tempa07,zaf3);
	zbf11 = _mm_add_ps(tempb07,zbf3);
	zaf13 = _mm_sub_ps(tempa07,zaf3);
	zbf13 = _mm_sub_ps(tempb07,zbf3);

	dfa5 = _mm_add_ps(zaf13,zaf2);
	dfb5 = _mm_add_ps(zbf13,zbf2);
	dfa3 = _mm_sub_ps(zaf13,zaf2);
	dfb3 = _mm_sub_ps(zbf13,zbf2);
	dfa1 = _mm_add_ps(zaf11,zaf4);
	dfb1 = _mm_add_ps(zbf11,zbf4);
	dfa7 = _mm_sub_ps(zaf11,zaf4);
	dfb7 = _mm_sub_ps(zbf11,zbf4);


	// Do some post transform scaling
	xfa0 = _mm_mul_ps(dfa0,dftscale0a);
	xfb0 = _mm_mul_ps(dfb0,dftscale0b);
	xfa1 = _mm_mul_ps(dfa1,dftscale1a);
	xfb1 = _mm_mul_ps(dfb1,dftscale1b);
	xfa2 = _mm_mul_ps(dfa2,dftscale2a);
	xfb2 = _mm_mul_ps(dfb2,dftscale2b);
	xfa3 = _mm_mul_ps(dfa3,dftscale3a);
	xfb3 = _mm_mul_ps(dfb3,dftscale3b);
	xfa4 = _mm_mul_ps(dfa4,dftscale4a);
	xfb4 = _mm_mul_ps(dfb4,dftscale4b);
	xfa5 = _mm_mul_ps(dfa5,dftscale5a);
	xfb5 = _mm_mul_ps(dfb5,dftscale5b);
	xfa6 = _mm_mul_ps(dfa6,dftscale6a);
	xfb6 = _mm_mul_ps(dfb6,dftscale6b);
	xfa7 = _mm_mul_ps(dfa7,dftscale7a);
	xfb7 = _mm_mul_ps(dfb7,dftscale7b);


	// Convert back to integer and save back to memory
	m0 = _mm_cvtps_epi32(xfa0);
	m1 = _mm_cvtps_epi32(xfb0);
	m4 = _mm_packs_epi32(m0,m1);
	_mm_store_si128 ((__m128i *) pData,m4);

	m0 = _mm_cvtps_epi32(xfa1);
	m1 = _mm_cvtps_epi32(xfb1);
	m4 = _mm_packs_epi32(m0,m1);
	_mm_store_si128 ((__m128i *) (pData+BLOCKDIM*1),m4);

	m0 = _mm_cvtps_epi32(xfa2);
	m1 = _mm_cvtps_epi32(xfb2);
	m4 = _mm_packs_epi32(m0,m1);
	_mm_store_si128 ((__m128i *) (pData+BLOCKDIM*2),m4);

	m0 = _mm_cvtps_epi32(xfa3);
	m1 = _mm_cvtps_epi32(xfb3);
	m4 = _mm_packs_epi32(m0,m1);
	_mm_store_si128 ((__m128i *) (pData+BLOCKDIM*3),m4);

	m0 = _mm_cvtps_epi32(xfa4);
	m1 = _mm_cvtps_epi32(xfb4);
	m4 = _mm_packs_epi32(m0,m1);
	_mm_store_si128 ((__m128i *) (pData+BLOCKDIM*4),m4);

	m0 = _mm_cvtps_epi32(xfa5);
	m1 = _mm_cvtps_epi32(xfb5);
	m4 = _mm_packs_epi32(m0,m1);
	_mm_store_si128 ((__m128i *) (pData+BLOCKDIM*5),m4);

	m0 = _mm_cvtps_epi32(xfa6);
	m1 = _mm_cvtps_epi32(xfb6);
	m4 = _mm_packs_epi32(m0,m1);
	_mm_store_si128 ((__m128i *) (pData+BLOCKDIM*6),m4);

	m0 = _mm_cvtps_epi32(xfa7);
	m1 = _mm_cvtps_epi32(xfb7);
	m4 = _mm_packs_epi32(m0,m1);
	_mm_store_si128 ((__m128i *) (pData+BLOCKDIM*7),m4);

#endif // if defined(WIN32) || defined(WIN64)

}


////////////////////////////////////////////////////////////////////////////////////////
// Perform an inverse DCT in-place on an 8x8 block of data
//
//  Input parameters : 
//				pData  -  Pointer to the block of data to transform
//
//	Output : none
//
//  Based off of the algorithm shown in Y. Arai, T.Agui and M. Nakajima, "A Fast DCT-SQ Scheme for Images",
//  Transactions of the IEICE, vol. E71, no. 11, p. 1095, Nov. 1988.
//  and 
//  C. Loeffler, A. Ligtenberg and G. Moschytz, 
//  "Practical Fast 1-D DCT Algorithms with 11 Multiplications", Proc. Int'l. Conf. on Acoustics,
//  Speech, and Signal Processing 1989 (ICASSP '89), pp. 988-991.
//
//
void CAVVideoCompression::InverseDCT(int *pData)
{
	float temp[BLOCK8x8SIZE];
	float ta17,tb17,ta53,tb53,ta04,tb04,ta26,tb26,t0,t1,t2,t3,t7,t8,t9,t10;

	for(unsigned int i=0; i<BLOCK8x8SIZE; i++) temp[i] = pData[i] * idctprescale[i];

	// transform the rows
	for(unsigned int i=0; i<BLOCK8x8SIZE; i+= BLOCKDIM){
		ta17= temp[1 + i] + temp[7 + i];
		tb17= temp[1 + i] - temp[7 + i];
		ta53= temp[5 + i] + temp[3 + i];
		tb53= temp[5 + i] - temp[3 + i];

		t0=  ta17 + ta53;
		t1= (ta17 - ta53)*c_cos4_c;

		t2=  tb17*c_b6a2_c - tb53*c_cos2_c;
		t3=  tb53*c_b6a2_c + tb17*c_cos2_c;

		t3 -= t0;
		t1 -= t3;
		t2 += t1;

		ta26 = temp[2 + i] + temp[6 + i];
		tb26 = temp[2 + i] - temp[6 + i];
		tb26*= c_cos4_c;
		tb26-= ta26;

		ta04= temp[0 + i] + temp[4 + i];
		tb04= temp[0 + i] - temp[4 + i];

		t7= ta04 + ta26;
		t8= ta04 - ta26;
		t9= tb04 + tb26;
		t10= tb04 - tb26;

		temp[0 + i]= t7 + t0;
		temp[7 + i]= t7 - t0;
		temp[1 + i]= t9 + t3;
		temp[6 + i]= t9 - t3;
		temp[2 + i]= t10 + t1;
		temp[5 + i]= t10 - t1;
		temp[3 + i]= t8 - t2;
		temp[4 + i]= t8 + t2;

	}


	// transform the columns
	for(unsigned int i=0; i<BLOCKDIM; ++i){
		ta17= temp[1*BLOCKDIM + i] + temp[7*BLOCKDIM + i];
		tb17= temp[1*BLOCKDIM + i] - temp[7*BLOCKDIM + i];
		ta53= temp[5*BLOCKDIM + i] + temp[3*BLOCKDIM + i];
		tb53= temp[5*BLOCKDIM + i] - temp[3*BLOCKDIM + i];

		t0=  ta17 + ta53;
		t1= (ta17 - ta53)*c_cos4_c;

		t2=  tb17*c_b6a2_c - tb53*c_cos2_c;
		t3=  tb53*c_b6a2_c + tb17*c_cos2_c;

		t3 -= t0;
		t1 -= t3;
		t2 += t1;

		ta26 = temp[2*BLOCKDIM + i] + temp[6*BLOCKDIM + i];
		tb26 = temp[2*BLOCKDIM + i] - temp[6*BLOCKDIM + i];
		tb26*= c_cos4_c;
		tb26-= ta26;

		ta04= temp[0*BLOCKDIM + i] + temp[4*BLOCKDIM + i];
		tb04= temp[0*BLOCKDIM + i] - temp[4*BLOCKDIM + i];

		t7= ta04 + ta26;
		t8= ta04 - ta26;
		t9= tb04 + tb26;
		t10= tb04 - tb26;

		pData[0*BLOCKDIM + i]=  (t7 + t0);
		pData[7*BLOCKDIM + i]=  (t7 - t0);
		pData[1*BLOCKDIM + i]=  (t9 + t3);
		pData[6*BLOCKDIM + i]=  (t9 - t3);
		pData[2*BLOCKDIM + i]=  (t10 + t1);
		pData[5*BLOCKDIM + i]=  (t10 - t1);
		pData[3*BLOCKDIM + i]=  (t8 - t2);
		pData[4*BLOCKDIM + i]=  (t8 + t2);
	}
}


////////////////////////////////////////////////////////////////////////////////////////
// Perform an inverse DCT in-place on an 8x8 block of data using the SSE2 simd
//
//  Input parameters : 
//				pData  -  Pointer to the block of data to transform
//
//	Output : none
//
//  Based off of the algorithm shown in Y. Arai, T.Agui and M. Nakajima, "A Fast DCT-SQ Scheme for Images",
//  Transactions of the IEICE, vol. E71, no. 11, p. 1095, Nov. 1988.
//  and 
//  C. Loeffler, A. Ligtenberg and G. Moschytz, 
//  "Practical Fast 1-D DCT Algorithms with 11 Multiplications", Proc. Int'l. Conf. on Acoustics,
//  Speech, and Signal Processing 1989 (ICASSP '89), pp. 988-991.
//
//
void CAVVideoCompression::InverseDctSSE2(int *pData)
{

	assert(pData);
#if defined(WIN32) || defined(WIN64)



	// Load some scaling and DCT multiplier constants
	__m128 c_cos4 = _mm_load1_ps(&c_cos4_c);
	__m128 c_cos2 = _mm_load1_ps(&c_cos2_c);
	__m128 c_b6a2 = _mm_load1_ps(&c_b6a2_c);

	__m128 dftscale0a = _mm_load_ps(idftscale0a_c);
	__m128 dftscale0b = _mm_load_ps(idftscale0b_c);
	__m128 dftscale1a = _mm_load_ps(idftscale1a_c);
	__m128 dftscale1b = _mm_load_ps(idftscale1b_c);
	__m128 dftscale2a = _mm_load_ps(idftscale2a_c);
	__m128 dftscale2b = _mm_load_ps(idftscale2b_c);
	__m128 dftscale3a = _mm_load_ps(idftscale3a_c);
	__m128 dftscale3b = _mm_load_ps(idftscale3b_c);
	__m128 dftscale4a = _mm_load_ps(idftscale4a_c);
	__m128 dftscale4b = _mm_load_ps(idftscale4b_c);
	__m128 dftscale5a = _mm_load_ps(idftscale5a_c);
	__m128 dftscale5b = _mm_load_ps(idftscale5b_c);
	__m128 dftscale6a = _mm_load_ps(idftscale6a_c);
	__m128 dftscale6b = _mm_load_ps(idftscale6b_c);
	__m128 dftscale7a = _mm_load_ps(idftscale7a_c);
	__m128 dftscale7b = _mm_load_ps(idftscale7b_c);


	// Load the 8x8 block from memory and convert to floating point
	__m128i tmp128 = _mm_load_si128((__m128i *) (pData)); 
	__m128  xfa0 = _mm_cvtepi32_ps(tmp128);
	tmp128 = _mm_load_si128((__m128i *) (pData+4));
	__m128  xfb0 = _mm_cvtepi32_ps(tmp128);
	tmp128 = _mm_load_si128((__m128i *) (pData+8));
	__m128  xfa1 = _mm_cvtepi32_ps(tmp128);
	tmp128 = _mm_load_si128((__m128i *) (pData+12));
	__m128  xfb1 = _mm_cvtepi32_ps(tmp128);
	tmp128 = _mm_load_si128((__m128i *) (pData+16));
	__m128  xfa2 = _mm_cvtepi32_ps(tmp128);
	tmp128 = _mm_load_si128((__m128i *) (pData+20));
	__m128  xfb2 = _mm_cvtepi32_ps(tmp128);
	tmp128 = _mm_load_si128((__m128i *) (pData+24));
	__m128  xfa3 = _mm_cvtepi32_ps(tmp128);
	tmp128 = _mm_load_si128((__m128i *) (pData+28));
	__m128  xfb3 = _mm_cvtepi32_ps(tmp128);
	tmp128 = _mm_load_si128((__m128i *) (pData+32));
	__m128  xfa4 = _mm_cvtepi32_ps(tmp128);
	tmp128 = _mm_load_si128((__m128i *) (pData+36));
	__m128  xfb4 = _mm_cvtepi32_ps(tmp128);
	tmp128 = _mm_load_si128((__m128i *) (pData+40));
	__m128  xfa5 = _mm_cvtepi32_ps(tmp128);
	tmp128 = _mm_load_si128((__m128i *) (pData+44));
	__m128  xfb5 = _mm_cvtepi32_ps(tmp128);
	tmp128 = _mm_load_si128((__m128i *) (pData+48));
	__m128  xfa6 = _mm_cvtepi32_ps(tmp128);
	tmp128 = _mm_load_si128((__m128i *) (pData+52));
	__m128  xfb6 = _mm_cvtepi32_ps(tmp128);
	tmp128 = _mm_load_si128((__m128i *) (pData+56));
	__m128  xfa7 = _mm_cvtepi32_ps(tmp128);
	tmp128 = _mm_load_si128((__m128i *) (pData+60));
	__m128  xfb7 = _mm_cvtepi32_ps(tmp128);


	// Prescale the values
	xfa0 = _mm_mul_ps(xfa0,dftscale0a);
	xfb0 = _mm_mul_ps(xfb0,dftscale0b);
	xfa1 = _mm_mul_ps(xfa1,dftscale1a);
	xfb1 = _mm_mul_ps(xfb1,dftscale1b);
	xfa2 = _mm_mul_ps(xfa2,dftscale2a);
	xfb2 = _mm_mul_ps(xfb2,dftscale2b);
	xfa3 = _mm_mul_ps(xfa3,dftscale3a);
	xfb3 = _mm_mul_ps(xfb3,dftscale3b);
	xfa4 = _mm_mul_ps(xfa4,dftscale4a);
	xfb4 = _mm_mul_ps(xfb4,dftscale4b);
	xfa5 = _mm_mul_ps(xfa5,dftscale5a);
	xfb5 = _mm_mul_ps(xfb5,dftscale5b);
	xfa6 = _mm_mul_ps(xfa6,dftscale6a);
	xfb6 = _mm_mul_ps(xfb6,dftscale6b);
	xfa7 = _mm_mul_ps(xfa7,dftscale7a);
	xfb7 = _mm_mul_ps(xfb7,dftscale7b);


	// First process is to transform the rows.  Since the SSE works on columns, this requires a row/column transpose
	// so first do a row column transpose

	_MM_TRANSPOSE4_PS(xfa0, xfa1, xfa2, xfa3)
		_MM_TRANSPOSE4_PS(xfb0, xfb1, xfb2, xfb3)
		_MM_TRANSPOSE4_PS(xfa4, xfa5, xfa6, xfa7)
		_MM_TRANSPOSE4_PS(xfb4, xfb5, xfb6, xfb7)


		// Correct the arrangement due to interleaved columns and transpose macro
		// This does no operations, it just does a renaming so that the following code makes sense
		__m128 dfa0 = xfa0;
	__m128 dfa1 = xfa1;
	__m128 dfa2 = xfa2;
	__m128 dfa3 = xfa3;

	__m128 dfa4 = xfb0;
	__m128 dfa5 = xfb1;
	__m128 dfa6 = xfb2;
	__m128 dfa7 = xfb3;

	__m128 dfb0 = xfa4;
	__m128 dfb1 = xfa5;
	__m128 dfb2 = xfa6;
	__m128 dfb3 = xfa7;

	__m128 dfb4 = xfb4;
	__m128 dfb5 = xfb5;
	__m128 dfb6 = xfb6;
	__m128 dfb7 = xfb7;



	// Do the Row transform
	__m128 ta17_a = _mm_add_ps(dfa1,dfa7);
	__m128 ta17_b = _mm_add_ps(dfb1,dfb7);
	__m128 tb17_a = _mm_sub_ps(dfa1,dfa7);
	__m128 tb17_b = _mm_sub_ps(dfb1,dfb7);

	__m128 ta53_a = _mm_add_ps(dfa5,dfa3);
	__m128 ta53_b = _mm_add_ps(dfb5,dfb3);
	__m128 tb53_a = _mm_sub_ps(dfa5,dfa3);
	__m128 tb53_b = _mm_sub_ps(dfb5,dfb3);

	__m128 t0_a = _mm_add_ps(ta17_a,ta53_a);
	__m128 t0_b = _mm_add_ps(ta17_b,ta53_b);
	__m128 t1_a = _mm_sub_ps(ta17_a,ta53_a);
	__m128 t1_b = _mm_sub_ps(ta17_b,ta53_b);
	t1_a = _mm_mul_ps(t1_a,c_cos4);
	t1_b = _mm_mul_ps(t1_b,c_cos4);

	__m128 tx_a = _mm_mul_ps(tb17_a,c_b6a2);
	__m128 tx_b = _mm_mul_ps(tb17_b,c_b6a2);
	__m128 ty_a = _mm_mul_ps(tb53_a,c_cos2);
	__m128 ty_b = _mm_mul_ps(tb53_b,c_cos2);
	__m128 t2_a = _mm_sub_ps(tx_a,ty_a);
	__m128 t2_b = _mm_sub_ps(tx_b,ty_b);
	tx_a = _mm_mul_ps(tb17_a,c_cos2);
	tx_b = _mm_mul_ps(tb17_b,c_cos2);
	ty_a = _mm_mul_ps(tb53_a,c_b6a2);
	ty_b = _mm_mul_ps(tb53_b,c_b6a2);
	__m128 t3_a = _mm_add_ps(tx_a,ty_a);
	__m128 t3_b = _mm_add_ps(tx_b,ty_b);

	t3_a = _mm_sub_ps(t3_a,t0_a);
	t3_b = _mm_sub_ps(t3_b,t0_b);
	t1_a = _mm_sub_ps(t1_a,t3_a);
	t1_b = _mm_sub_ps(t1_b,t3_b);
	t2_a = _mm_add_ps(t2_a,t1_a);
	t2_b = _mm_add_ps(t2_b,t1_b);

	__m128 ta26_a = _mm_add_ps(dfa2,dfa6);
	__m128 ta26_b = _mm_add_ps(dfb2,dfb6);
	__m128 tb26_a = _mm_sub_ps(dfa2,dfa6);
	__m128 tb26_b = _mm_sub_ps(dfb2,dfb6);
	tb26_a = _mm_mul_ps(tb26_a,c_cos4);
	tb26_b = _mm_mul_ps(tb26_b,c_cos4);
	tb26_a = _mm_sub_ps(tb26_a,ta26_a);
	tb26_b = _mm_sub_ps(tb26_b,ta26_b);

	__m128 ta04_a = _mm_add_ps(dfa0,dfa4);
	__m128 ta04_b = _mm_add_ps(dfb0,dfb4);
	__m128 tb04_a = _mm_sub_ps(dfa0,dfa4);
	__m128 tb04_b = _mm_sub_ps(dfb0,dfb4);

	__m128 t7_a = _mm_add_ps(ta04_a,ta26_a);
	__m128 t7_b = _mm_add_ps(ta04_b,ta26_b);
	__m128 t8_a = _mm_sub_ps(ta04_a,ta26_a);
	__m128 t8_b = _mm_sub_ps(ta04_b,ta26_b);
	__m128 t9_a = _mm_add_ps(tb04_a,tb26_a);
	__m128 t9_b = _mm_add_ps(tb04_b,tb26_b);
	__m128 t10_a = _mm_sub_ps(tb04_a,tb26_a);
	__m128 t10_b = _mm_sub_ps(tb04_b,tb26_b);

	xfa0 = _mm_add_ps(t7_a,t0_a);
	xfb0 = _mm_add_ps(t7_b,t0_b);
	xfa7 = _mm_sub_ps(t7_a,t0_a);
	xfb7 = _mm_sub_ps(t7_b,t0_b);
	xfa1 = _mm_add_ps(t9_a,t3_a);
	xfb1 = _mm_add_ps(t9_b,t3_b);
	xfa6 = _mm_sub_ps(t9_a,t3_a);
	xfb6 = _mm_sub_ps(t9_b,t3_b);
	xfa2 = _mm_add_ps(t10_a,t1_a);
	xfb2 = _mm_add_ps(t10_b,t1_b);
	xfa5 = _mm_sub_ps(t10_a,t1_a);
	xfb5 = _mm_sub_ps(t10_b,t1_b);
	xfa3 = _mm_sub_ps(t8_a,t2_a);
	xfb3 = _mm_sub_ps(t8_b,t2_b);
	xfa4 = _mm_add_ps(t8_a,t2_a);
	xfb4 = _mm_add_ps(t8_b,t2_b);


	// transpose back to original order
	_MM_TRANSPOSE4_PS(xfa0, xfa1, xfa2, xfa3)
		_MM_TRANSPOSE4_PS(xfb0, xfb1, xfb2, xfb3)
		_MM_TRANSPOSE4_PS(xfa4, xfa5, xfa6, xfa7)
		_MM_TRANSPOSE4_PS(xfb4, xfb5, xfb6, xfb7)

		// Correct the arrangement due to interleaved columns and transpose macro
		// This does no operations, it just does a renaming so that the following code makes sense
		dfa0 = xfa0;
	dfa1 = xfa1;
	dfa2 = xfa2;
	dfa3 = xfa3;

	dfa4 = xfb0;
	dfa5 = xfb1;
	dfa6 = xfb2;
	dfa7 = xfb3;

	dfb0 = xfa4;
	dfb1 = xfa5;
	dfb2 = xfa6;
	dfb3 = xfa7;

	dfb4 = xfb4;
	dfb5 = xfb5;
	dfb6 = xfb6;
	dfb7 = xfb7;

	ta17_a = _mm_add_ps(dfa1,dfa7);
	ta17_b = _mm_add_ps(dfb1,dfb7);
	tb17_a = _mm_sub_ps(dfa1,dfa7);
	tb17_b = _mm_sub_ps(dfb1,dfb7);

	ta53_a = _mm_add_ps(dfa5,dfa3);
	ta53_b = _mm_add_ps(dfb5,dfb3);
	tb53_a = _mm_sub_ps(dfa5,dfa3);
	tb53_b = _mm_sub_ps(dfb5,dfb3);

	t0_a = _mm_add_ps(ta17_a,ta53_a);
	t0_b = _mm_add_ps(ta17_b,ta53_b);
	t1_a = _mm_sub_ps(ta17_a,ta53_a);
	t1_b = _mm_sub_ps(ta17_b,ta53_b);
	t1_a = _mm_mul_ps(t1_a,c_cos4);
	t1_b = _mm_mul_ps(t1_b,c_cos4);

	tx_a = _mm_mul_ps(tb17_a,c_b6a2);
	tx_b = _mm_mul_ps(tb17_b,c_b6a2);
	ty_a = _mm_mul_ps(tb53_a,c_cos2);
	ty_b = _mm_mul_ps(tb53_b,c_cos2);
	t2_a = _mm_sub_ps(tx_a,ty_a);
	t2_b = _mm_sub_ps(tx_b,ty_b);
	tx_a = _mm_mul_ps(tb17_a,c_cos2);
	tx_b = _mm_mul_ps(tb17_b,c_cos2);
	ty_a = _mm_mul_ps(tb53_a,c_b6a2);
	ty_b = _mm_mul_ps(tb53_b,c_b6a2);
	t3_a = _mm_add_ps(tx_a,ty_a);
	t3_b = _mm_add_ps(tx_b,ty_b);

	t3_a = _mm_sub_ps(t3_a,t0_a);
	t3_b = _mm_sub_ps(t3_b,t0_b);
	t1_a = _mm_sub_ps(t1_a,t3_a);
	t1_b = _mm_sub_ps(t1_b,t3_b);
	t2_a = _mm_add_ps(t2_a,t1_a);
	t2_b = _mm_add_ps(t2_b,t1_b);


	ta26_a = _mm_add_ps(dfa2,dfa6);
	ta26_b = _mm_add_ps(dfb2,dfb6);
	tb26_a = _mm_sub_ps(dfa2,dfa6);
	tb26_b = _mm_sub_ps(dfb2,dfb6);
	tb26_a = _mm_mul_ps(tb26_a,c_cos4);
	tb26_b = _mm_mul_ps(tb26_b,c_cos4);
	tb26_a = _mm_sub_ps(tb26_a,ta26_a);
	tb26_b = _mm_sub_ps(tb26_b,ta26_b);

	ta04_a = _mm_add_ps(dfa0,dfa4);
	ta04_b = _mm_add_ps(dfb0,dfb4);
	tb04_a = _mm_sub_ps(dfa0,dfa4);
	tb04_b = _mm_sub_ps(dfb0,dfb4);

	t7_a = _mm_add_ps(ta04_a,ta26_a);
	t7_b = _mm_add_ps(ta04_b,ta26_b);
	t8_a = _mm_sub_ps(ta04_a,ta26_a);
	t8_b = _mm_sub_ps(ta04_b,ta26_b);
	t9_a = _mm_add_ps(tb04_a,tb26_a);
	t9_b = _mm_add_ps(tb04_b,tb26_b);
	t10_a = _mm_sub_ps(tb04_a,tb26_a);
	t10_b = _mm_sub_ps(tb04_b,tb26_b);

	xfa0 = _mm_add_ps(t7_a,t0_a);
	xfb0 = _mm_add_ps(t7_b,t0_b);
	xfa7 = _mm_sub_ps(t7_a,t0_a);
	xfb7 = _mm_sub_ps(t7_b,t0_b);
	xfa1 = _mm_add_ps(t9_a,t3_a);
	xfb1 = _mm_add_ps(t9_b,t3_b);
	xfa6 = _mm_sub_ps(t9_a,t3_a);
	xfb6 = _mm_sub_ps(t9_b,t3_b);
	xfa2 = _mm_add_ps(t10_a,t1_a);
	xfb2 = _mm_add_ps(t10_b,t1_b);
	xfa5 = _mm_sub_ps(t10_a,t1_a);
	xfb5 = _mm_sub_ps(t10_b,t1_b);
	xfa3 = _mm_sub_ps(t8_a,t2_a);
	xfb3 = _mm_sub_ps(t8_b,t2_b);
	xfa4 = _mm_add_ps(t8_a,t2_a);
	xfb4 = _mm_add_ps(t8_b,t2_b);


	// Convert back to integer and save back to memory
	__m128i m0 = _mm_cvtps_epi32(xfa0);
	_mm_store_si128 ((__m128i *) pData,m0);
	m0 = _mm_cvtps_epi32(xfb0);
	_mm_store_si128 ((__m128i *) (pData + 4),m0);
	m0 = _mm_cvtps_epi32(xfa1);
	_mm_store_si128 ((__m128i *) (pData + 8),m0);
	m0 = _mm_cvtps_epi32(xfb1);
	_mm_store_si128 ((__m128i *) (pData + 12),m0);
	m0 = _mm_cvtps_epi32(xfa2);
	_mm_store_si128 ((__m128i *) (pData + 16),m0);
	m0 = _mm_cvtps_epi32(xfb2);
	_mm_store_si128 ((__m128i *) (pData + 20),m0);
	m0 = _mm_cvtps_epi32(xfa3);
	_mm_store_si128 ((__m128i *) (pData + 24),m0);
	m0 = _mm_cvtps_epi32(xfb3);
	_mm_store_si128 ((__m128i *) (pData + 28),m0);
	m0 = _mm_cvtps_epi32(xfa4);
	_mm_store_si128 ((__m128i *) (pData + 32),m0);
	m0 = _mm_cvtps_epi32(xfb4);
	_mm_store_si128 ((__m128i *) (pData + 36),m0);
	m0 = _mm_cvtps_epi32(xfa5);
	_mm_store_si128 ((__m128i *) (pData + 40),m0);
	m0 = _mm_cvtps_epi32(xfb5);
	_mm_store_si128 ((__m128i *) (pData + 44),m0);
	m0 = _mm_cvtps_epi32(xfa6);
	_mm_store_si128 ((__m128i *) (pData + 48),m0);
	m0 = _mm_cvtps_epi32(xfb6);
	_mm_store_si128 ((__m128i *) (pData + 52),m0);
	m0 = _mm_cvtps_epi32(xfa7);
	_mm_store_si128 ((__m128i *) (pData + 56),m0);
	m0 = _mm_cvtps_epi32(xfb7);
	_mm_store_si128 ((__m128i *) (pData + 60),m0);

#endif // if defined(WIN32) || defined(WIN64)


}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantize an 8x8 subblock of video samples, uses a lookup table for the first 256 quantized values
// for speed purposes
//
//  Input parameters :
//		pDin	:  pointer to the input macroblock (64 luma0 + 64 luma1 + 64 luma2 + 64 luma3 + 64 Cr + 64 Cb)
//		pDout	:  pointer to hold the processed data (64 luma0 + 64 luma1 + 64 luma2 + 64 luma3 + 64 Cr + 64 Cb)
//
//	Output : none
//
void CAVVideoCompression::QuantizeIntra(short *pDin,short *pDout)

{

	assert(pDin);
	assert(pDout);
	assert(m_dcPredShift);


	// Quantize the DC value
	// per ISO 13818-2section 7.4.1
	int x = (int) pDin[0];
	if(x < 0) 
	{
		x = -x;
		int dcquant = (x + (m_dcPredShift >> 1))/m_dcPredShift; 
		pDout[0] = (short) -dcquant;
	} 
	else 
	{
		int dcquant = (x + (m_dcPredShift >> 1))/m_dcPredShift; 
		pDout[0] = (short) dcquant;
	} 



	// Quantize the 63 AC values
	// per ISO 13818-2 section 7.4.2
	int quantVal = (m_qScaleType == 1) ? MquantMapNonlinear[m_mQuant] :  MquantMapLinear[m_mQuant];
	for (unsigned int i=1; i<BLOCK8x8SIZE; ++i)
	{

		x = pDin[i];

		// positive and negative values split for speed
		if(x < 0) 
		{
			x = -x;
			if(x < m_zeroRowCount) pDout[i] = (short) 0;
			else 
			{
				if(x < QUANTTABLESIZE) pDout[i] = (short) -m_quantTable[i][x];
				else 
				{
					// Not in table so quantize manually per ISO 13818-2 section 7.4.2
					int d = (short) QuantMatrixIntra[i];
					int y = ((QUANTUPSCALE*x) + (d>>1))/d;   // d cannot be zero since quant mat values of 0 are not allowed
					d = (QUANTSCALE3*quantVal + QUANTRND) >> QUANTRND;
					y = (y+d)/(QUANTRND*quantVal);
					pDout[i] = (short) -y;
				}
			}
		} 
		else 
		{
			if(x < m_zeroRowCount) pDout[i] = (short) 0;
			else 
			{
				if(x < QUANTTABLESIZE) pDout[i] = (short) m_quantTable[i][x];
				else 
				{
					// Not in table so quantize manually per ISO 13818-2 section 7.4.2
					int d = (short) QuantMatrixIntra[i];
					int y = ((QUANTUPSCALE*x) + (d>>1))/d;  // d cannot be zero since quant mat values of 0 are not allowed
					d = (QUANTSCALE3*quantVal + QUANTRND) >> QUANTRND;
					y = (y+d)/(QUANTRND*quantVal);
					pDout[i] = (short) y;
				}
			}
		}

	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantize an 8x8 subblock of inter video samples
//
//  Input parameters :
//		pDin	:  pointer to the input macroblock (64 luma0 + 64 luma1 + 64 luma2 + 64 luma3 + 64 Cr + 64 Cb)
//		pDout	:  pointer to hold the processed data (64 luma0 + 64 luma1 + 64 luma2 + 64 luma3 + 64 Cr + 64 Cb)
//
//	Output : none
//
void CAVVideoCompression::QuantizeInter(short *pDin,short *pDout)
{

	assert(pDin);
	assert(pDout);


	// Quantize the 64 AC values
	// per ISO 13818-2 section 7.4.2
	int quantVal = (m_qScaleType == 1) ? MquantMapNonlinear[m_mQuant] :  MquantMapLinear[m_mQuant];
	for (unsigned int i=0; i<BLOCK8x8SIZE; ++i)
	{
	
		int x = pDin[i];
		int d = (short) QuantMatrixInter[i];

		// positive and negative values split for speed
		if(x < 0) 
		{
			x = -x;
			int y = ((QUANTUPSCALE*x) + (d>>1))/d;  // d cannot be zero since quant mat values of 0 are not allowed
			y = y/(QUANTRND*quantVal);
			pDout[i] = (short) -y;
		} 
		else 
		{
			int y = ((QUANTUPSCALE*x) + (d>>1))/d;  // d cannot be zero since quant mat values of 0 are not allowed
			y = y/(QUANTRND*quantVal);
			pDout[i] = (short) y;
		}
	}
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Quantize an 8x8 subblock of video samples using SSE2 instructions
// per ISO 13818-2 section 7.4
//
//  Input parameters :
//		pDin	:  pointer to the input macroblock (64 luma0 + 64 luma1 + 64 luma2 + 64 luma3 + 64 Cr + 64 Cb)
//		pDout  :  pointer to hold the processed data (64 luma0 + 64 luma1 + 64 luma2 + 64 luma3 + 64 Cr + 64 Cb)
//
//	Output : none
//
void CAVVideoCompression::QuantizeIntraSSE2(short *pDin,short *pDout)
{

	assert(pDin);
	assert(pDout);
	assert(m_dcPredShift);


	const uint32 fmask1 = 0x7FFFFFFF;
	const float foffset1 = SSEQUANTRND;

#if defined(WIN32) || defined(WIN64)


	// Extract the DC value to work on last
	short DCval = pDin[0];


	// Get the input and output data locations (must be 16 byte aligned buffers)
	__m128i *datain = (__m128i *) pDin;
	__m128i *dataout = (__m128i *) pDout;

	__m128 mask1 = _mm_load1_ps((float *) &fmask1);
	__m128 foffset = _mm_load1_ps(&foffset1);

	// Get pointers to the pre calculated quantization scales factors 
	float *qa = m_quantScaleA;
	float *qb = m_quantScaleB;

	// Quantize the 8 rows of the 8x8 block
	// per ISO 13818-2 section 7.4.2
	for(unsigned int i=0;i<BLOCKDIM;++i) 
	{

		// Load the precalculated scale factors for this row which were based on the mquant value
		__m128 qscalea = _mm_loadu_ps(qa);
		__m128 qscaleb = _mm_loadu_ps(qb);
		qa += 4;  // SSE2 size = 4 floats per load
		qb += 4;  // SSE2 size = 4 floats per load

		// Load the input data
		__m128i temp1 = _mm_load_si128(&datain[i]);

		// Convert the input row to floating point  (a registers have odd values, b registers have even values)
		__m128i temp2 = _mm_srai_epi32(temp1,16);
		__m128 xfa0 = _mm_cvtepi32_ps(temp2);
		temp2 = _mm_slli_si128(temp1,2);
		temp2 = _mm_srai_epi32(temp2,16);
		__m128 xfb0 = _mm_cvtepi32_ps(temp2);


		// Save a sign vector and convert to abs
		__m128 signveca = _mm_andnot_ps(mask1,xfa0);
		__m128 abs_xfa0 = _mm_and_ps(mask1,xfa0);
		__m128 signvecb = _mm_andnot_ps(mask1,xfb0);
		__m128 abs_xfb0 = _mm_and_ps(mask1,xfb0);


		//Multiply by the quant matrix and add the offset
		abs_xfa0 = _mm_mul_ps(abs_xfa0,qscalea);
		abs_xfa0 = _mm_add_ps(abs_xfa0,foffset);
		abs_xfb0 = _mm_mul_ps(abs_xfb0,qscaleb);
		abs_xfb0 = _mm_add_ps(abs_xfb0,foffset);

		// Do the rounding by converting the floats to an int then back to a float
		// then add the sign back in, then convert back to an int
		temp2 = _mm_cvttps_epi32(abs_xfa0);
		abs_xfa0 = _mm_cvtepi32_ps(temp2);
		xfa0 = _mm_or_ps(signveca,abs_xfa0);
		temp2 = _mm_cvtps_epi32(xfa0);

		__m128i temp3 = _mm_cvttps_epi32(abs_xfb0);
		abs_xfb0 = _mm_cvtepi32_ps(temp3);
		xfb0 = _mm_or_ps(signvecb,abs_xfb0);
		temp3 = _mm_cvtps_epi32(xfb0);

		// Convert the results to 16 bit values and store to memory
		temp1 = _mm_unpacklo_epi32(temp3,temp2);
		temp2 = _mm_unpackhi_epi32(temp3,temp2);
		temp1 = _mm_packs_epi32(temp1,temp2);
		_mm_store_si128 (&dataout[i],temp1);

	}

	// Quantize the DC value that was saved
	// per ISO 13818-2section 7.4.1
	if(DCval < 0) 
	{
		DCval = -DCval;
		int dcquant = (DCval + (m_dcPredShift >> 1))/m_dcPredShift; 
		pDout[0] = (short) -dcquant;
	} 
	else 
	{
		int dcquant = (DCval + (m_dcPredShift >> 1))/m_dcPredShift; 
		pDout[0] = (short) dcquant;
	} 


#endif  // if defined(WIN32) || defined(WIN64)
}

////////////////////////////////////////////////////////////////////////////////////////
// Calculate a quantization table based on the input quantization parameters
// Based on ISO 13818-2 sec 7.4
//
//  Input parameters : none
//
//	Output : None
//
//
void CAVVideoCompression::SetQuantTable(void)
{

	assert(m_dcPredShift);

	// Check if we need to change the quant table
	if(m_mQuant != m_prevmQuant)
	{


		// Based on the current quantization setting, calculate 
		// some pre-quantized tables that are used to speed up the 
		// quantization process.  Instead of doing the actual quantization
		// the results can be looked up in a table.  For SSE, the calculations
		// are still done and some constant multipliers are pre-calculated

		unsigned int quantScale = (m_qScaleType == 1) ? MquantMapNonlinear[m_mQuant] : MquantMapLinear[m_mQuant];

		if(m_simdTypeAvail != SSE_AVAIL)
		{
			// Do the non SSE version
			m_zeroRowCount = 0;
			unsigned int incflag = 1;
			for(unsigned int j=0;j<QUANTTABLESIZE;++j) 
			{

				// calculate the dc coef quant value
				// per ISO 13818 sec 7.4.1
				m_quantTable[0][j] = (short) (j + (m_dcPredShift >> 1))/m_dcPredShift;

				// Calculate the AC coefs quant values
				// per ISO 13818 sec 7.4.2
				for (unsigned int i=1; i<BLOCK8x8SIZE; ++i)
				{
					int d = QuantMatrixIntra[i];  // d cannot be zero since this is not allowed in the quant matrix
					int y = ((QUANTUPSCALE*j) + (d>>1))/d; 
					d = (QUANTSCALE3*quantScale + QUANTRND) >> QUANTRND;
					y = (y+d)/(QUANTRND*quantScale); 

					if (y > m_coeffLimit) y = m_coeffLimit;
					m_quantTable[i][j] = (short) y;
					if(m_quantTable[i][j] != 0) incflag = 0;
				}
				m_zeroRowCount += incflag;
			}

		}
		else
		{
			// Pre calculate the SSE version
			// Calculate the AC coefs quant values
			// per ISO 13818 sec 7.4.2  (DC value is calculated separately)
			for(unsigned int i=0;i<BLOCKDIM;++i) 
			{
				for(unsigned int j=0;j<BLOCKDIM/2;++j) 
				{
					if(quantScale != 0) 
					{
						m_quantScaleA[i*BLOCKDIM/2+j] = 16.0f/((float) quantScale*QuantMatrixIntra[i*BLOCKDIM + j*2 + 1]); // quant_mat not allowed to be 0
						m_quantScaleB[i*BLOCKDIM/2+j] = 16.0f/((float) quantScale*QuantMatrixIntra[i*BLOCKDIM + j*2]);
					}
				}
			}
		}

	}

	m_prevmQuant = m_mQuant;


}


////////////////////////////////////////////////////////////////////////////////////////
// Decompress a series of MPEG-1 video frames
//
//  Input parameters : none
//
//	Output : returns true if successful, otherwise returns false
//
//
bool CAVVideoCompression::DecompressFrame(void)
{
	uint8 tbuff1[1552]; 
	bool status;

	// FOR SSE operation, this buffer needs to be 16 byte aligned
	int *MBdata  = (int *)  (((UINT_PTR)tbuff1 + 15) & ( (UINT_PTR) ~15));

	unsigned int numSlices = m_sectionHeight >> 4;
	unsigned int numMBs = m_encodedWidth >> 4;

	// decode slices
	for(unsigned int i=0;i<numSlices;++i)
	{

		// Reset the dc predictors for each slice
		ResetPredictors(RESETALL);


		// Align the data to a byte boundary
		m_SerialStore.AlignToByte();

		// Find the next slice start code
		// Assume it is byte aligned
		status = FindStartCode();
		if(!status) return(false);


		// Read the slice header
		// See ISO 13818-2 sec. 6.2.4 for field definitions
		unsigned int sliceStart = m_SerialStore.SerialRead(32); // slice start code
		m_mQuant = m_SerialStore.SerialRead(5);   // quantization index
		m_SerialStore.SerialRead(1);   // extra bit slice


		// Determine the macro block row that this slice describes
		sliceStart = (sliceStart & 0xFF) - 1;

		for(unsigned int j=0;j<numMBs;++j)
		{
			status = DecodeMB(MBdata);
			if(!status) return(false);
			PlaceMB(j,sliceStart,(uint8 *) MBdata,m_pOutputFrame,1);
		}
	}
	return(true);
}

////////////////////////////////////////////////////////////////////////////////////////
// Align the serial reader with a slice start code
//
//  Input parameters : none
//
//	Output : returns true if successful, otherwise returns false
//
//
bool CAVVideoCompression::FindStartCode(void)
{

	while(!m_SerialStore.ReadBufferEmpty())
	{
		unsigned int peekVal = m_SerialStore.SerialPeek(32);
		if(peekVal > MINSLICEROW && peekVal < MAXSLICEROW) return(true);
		m_SerialStore.SerialRead(8);  // Advance a byte
	}
	return(false);

}

////////////////////////////////////////////////////////////////////////////////////////
// Decode an MPEG-1 macroblock
//
//  Input parameters : 
//		pMBdata  -  Pointer to the subblock data
//
//	Output : returns true if successful, otherwise returns false
//
//
bool CAVVideoCompression::DecodeMB(int *pMBdata)
{
	m_SerialStore.SerialRead(2);  // Read the MBAinc and MB_INTRA flag

	// decode the sub-blocks
	bool status=true;
	int cc;
	for(unsigned int i=0;i<NUMSUBBLOCKS;++i)
	{
		cc = (i < 4) ? 0 : i-3;

		status = DecodeSubblock(pMBdata,cc);
		if(status == false || m_SerialStore.ReadBufferEmpty()) return(false);

		if(m_simdTypeAvail == SSE_AVAIL) InverseDctSSE2(pMBdata);
		else InverseDCT(pMBdata);

		pMBdata += BLOCK8x8SIZE;
	}

	return(true);
}

////////////////////////////////////////////////////////////////////////////////////////
// Decode an MPEG-1 8x8 sub block
//
//  Input parameters : 
//		MBdata  -  Pointer to the subblock data
//		type	-	0 = luma block, 1 = cr block, 2 = cb block
//
//	Output : returns true if successful, otherwise returns false
//
//
bool CAVVideoCompression::DecodeSubblock(int *pdata,int type)
{

    int dcVal;

	if(type == 0) dcVal = GetLumaDC();
	else dcVal = GetChromaDC();

	// Get the DC Value
	m_dcDctPred[type] += dcVal;
	pdata[0] = m_dcDctPred[type] << (3 - m_intraDcPred);


	// Zero out the output subblock location
	for(unsigned int j=1;j<BLOCK8x8SIZE;j++) pdata[j] = 0;

	// Get the AC coeffs
	unsigned int coeffloc = 0;
	int val,run,sign;
	while(1)
	{
		++coeffloc;
		bool status = DecodeCoeff(&run,&val,&sign);
		if(!status)  return(false);  // error, we are lost
		if(run == EOB) return(true);  // done
		coeffloc += run;
		if(coeffloc > BLOCK8x8SIZE-1) return(false);  // error, we are lost
		unsigned int matIndex = ZigZagScan[coeffloc];
		val = (4 * val * m_mQuant * QuantMatrixIntra[matIndex]) >> 5;
		if(sign) val = -val;
		pdata[matIndex] = CLAMP(val,-MPEG2COEFFLIMIT,MPEG2COEFFLIMIT);
	}

}


////////////////////////////////////////////////////////////////////////////////////////
// Perform inverse quantization and then an inverse DCT on an 8x8 block of data
//
//  Input parameters : 
//			pdin  -  pointer to input data
//			pdout - pointer to output data
//			MBType - macroblock type
//
//	Output :  none
//
//
void CAVVideoCompression::InverseQuantDct(short *pdin,int *pdout,int MBType)
{

	short *pin = pdin;
	int *pout = pdout;

	for(unsigned int i=0;i<NUMSUBBLOCKS;++i)
	{

		// Do the inverse quantization
		InverseQuant(pin,pout,MBType);

		// perform the inverse dct
		if(m_simdTypeAvail == SSE_AVAIL) InverseDctSSE2(pout);
		else InverseDCT(pout);

		pin += BLOCK8x8SIZE;
		pout += BLOCK8x8SIZE;
	}


}

////////////////////////////////////////////////////////////////////////////////////////
// Perform inverse quantization and then an inverse DCT on an 8x8 block of data
//
//  Input parameters : 
//			pdin  -  pointer to input data
//			pdout - pointer to output data
//			MBType - macroblock type
//
//	Output :  none
//
//  See 13818-2 Section 7.4 for details of the algorithm
//
void CAVVideoCompression::InverseQuant(short *pdin,int *pdout,int MBType)
{

	int quantVal = (m_qScaleType == 1) ? MquantMapNonlinear[m_mQuant] :  MquantMapLinear[m_mQuant];

	if(MBType == MB_INTRA)
	{
		// do the DC coeff
		pdout[0] = pdin[0] << (3 - m_intraDcPred);

		// do the remaining coeffs
		for(unsigned int j=1;j<BLOCK8x8SIZE;++j) 
		{
			int val = pdin[j];
			if(val == 0) pdout[j] = 0;
			else
			{
				val = (val * quantVal * QuantMatrixIntra[j]) >> 4;

				// Mpeg 1 mismatch control
				if((val & 1) == 0)
				{
					if(val > 0) val -= 1;
					else val += 1;
				}
				pdout[j] = val;
			}
		}
	}
	else
	{
		for(unsigned int j=0;j<BLOCK8x8SIZE;++j) 
		{
			int x = pdin[j];
			if(x == 0) pdout[j] = 0;
			else 
			{
				if(x < 0) 
				{
					x = -x;
					x = (int)((2*x+1)*QuantMatrixInter[j]*quantVal)/32;
					x = (x-1) | 1;  // mpeg 1 mismatch control
					x = -x;
				}
				else
				{
					x = (int)((2*x+1)*QuantMatrixInter[j]*quantVal)/32;
					x = (x-1) | 1;  // mpeg 1 mismatch control
				}
				pdout[j] = CLAMP(x,-MPEG2COEFFLIMIT,MPEG2COEFFLIMIT);
			}
		}
	}

}

////////////////////////////////////////////////////////////////////////////////////////
// Decode a luma DC coefficient from the serial read buffer
//
//  Input parameters : none
//
//	Output : returns the value of the decoded coefficient
//
//  See 13818-2 Section B-12 for details about the table
// that is being decoded in this routine
//
int CAVVideoCompression::GetLumaDC(void)
{

	unsigned int peekVal = m_SerialStore.SerialPeek(9);
	int val;

	if(peekVal < 128)
	{
		m_SerialStore.SerialRead(2);
		val = m_SerialStore.SerialRead(1);
		if(val > 0) return(1);
		else return(-1);
	}

	if(peekVal < 256)
	{
		m_SerialStore.SerialRead(2);
		val = m_SerialStore.SerialRead(2);
		if(val > 1) return(val);
		else return(val - 3);
	}

	peekVal = peekVal & 0xFF;

	if(peekVal < 64)
	{
		m_SerialStore.SerialRead(3);
		return(0);
	}

	if(peekVal < 128)
	{
		m_SerialStore.SerialRead(3);
		val = m_SerialStore.SerialRead(3);
		if(val > 3) return(val);
		else return(val - 7);
	}

	peekVal = peekVal & 0x7F;

	if(peekVal < 64)
	{
		m_SerialStore.SerialRead(3);
		val = m_SerialStore.SerialRead(4);
		if(val > 7) return(val);
		else return(val - 15);
	}

	peekVal = peekVal & 0x3F;

	if(peekVal < 32)
	{
		m_SerialStore.SerialRead(4);
		val = m_SerialStore.SerialRead(5);
		if(val > 15) return(val);
		else return(val - 31);
	}

	peekVal = peekVal & 0x1F;

	if(peekVal < 16)
	{
		m_SerialStore.SerialRead(5);
		val = m_SerialStore.SerialRead(6);
		if(val > 31) return(val);
		else return(val - 63);
	}

	peekVal = peekVal & 0x0F;

	if(peekVal < 8)
	{
		m_SerialStore.SerialRead(6);
		val = m_SerialStore.SerialRead(7);
		if(val > 63) return(val);
		else return(val - 127);
	}

	peekVal = peekVal & 0x07;

	if(peekVal < 4)
	{
		m_SerialStore.SerialRead(7);
		val = m_SerialStore.SerialRead(8);
		if(val > 127) return(val);
		else return(val - 255);
	}

	peekVal = peekVal & 0x03;

	if(peekVal < 2)
	{
		m_SerialStore.SerialRead(8);
		val = m_SerialStore.SerialRead(9);
		if(val > 255) return(val);
		else return(val - 511);
	}

	peekVal = peekVal & 0x01;

	if(peekVal < 1)
	{
		m_SerialStore.SerialRead(9);
		val = m_SerialStore.SerialRead(10);
		if(val > 511) return(val);
		else return(val - 1023);
	}
	else
	{
		m_SerialStore.SerialRead(9);
		val = m_SerialStore.SerialRead(11);
		if(val > 1023) return(val);
		else return(val - 2047);
	}

}

////////////////////////////////////////////////////////////////////////////////////////
// Decode a Chroma DC coefficient from the serial read buffer
//
//  Input parameters : none
//
//	Output : returns the value of the decoded coefficient
//
//  See 13818-2 Section B-13 for details about the table
//  that is being decoded in this routine
//
int CAVVideoCompression::GetChromaDC(void)
{

	unsigned int peekVal = m_SerialStore.SerialPeek(10);
	unsigned int val;

	if(peekVal < 256)
	{
		m_SerialStore.SerialRead(2);
		return(0);
	}

	if(peekVal < 512)
	{
		m_SerialStore.SerialRead(2);
		val = m_SerialStore.SerialRead(1);
		if(val > 0) return(1);
		else return(-1);
	}

	peekVal = peekVal & 0x1FF;

	if(peekVal < 256)
	{
		m_SerialStore.SerialRead(2);
		val = m_SerialStore.SerialRead(2);
		if(val > 1) return(val);
		else return(val-3);
	}

	peekVal = peekVal & 0xFF;

	if(peekVal < 128)
	{
		m_SerialStore.SerialRead(3);
		val = m_SerialStore.SerialRead(3);
		if(val > 3) return(val);
		else return(val - 7);
	}

	peekVal = peekVal & 0x7F;

	if(peekVal < 64)
	{
		m_SerialStore.SerialRead(4);
		val = m_SerialStore.SerialRead(4);
		if(val > 7) return(val);
		else return(val - 15);
	}

	peekVal = peekVal & 0x3F;

	if(peekVal < 32)
	{
		m_SerialStore.SerialRead(5);
		val = m_SerialStore.SerialRead(5);
		if(val > 15) return(val);
		else return(val - 31);
	}

	peekVal = peekVal & 0x1F;

	if(peekVal < 16)
	{
		m_SerialStore.SerialRead(6);
		val = m_SerialStore.SerialRead(6);
		if(val > 31) return(val);
		else return(val - 63);
	}

	peekVal = peekVal & 0x0F;

	if(peekVal < 8)
	{
		m_SerialStore.SerialRead(7);
		val = m_SerialStore.SerialRead(7);
		if(val > 63) return(val);
		else return(val - 127);
	}

	peekVal = peekVal & 0x07;

	if(peekVal < 4)
	{
		m_SerialStore.SerialRead(8);
		val = m_SerialStore.SerialRead(8);
		if(val > 127) return(val);
		else return(val - 255);
	}

	peekVal = peekVal & 0x03;

	if(peekVal < 2)
	{
		m_SerialStore.SerialRead(9);
		val = m_SerialStore.SerialRead(9);
		if(val > 255) return(val);
		else return(val - 511);
	}

	peekVal = peekVal & 0x01;

	if(peekVal < 1)
	{
		m_SerialStore.SerialRead(10);
		val = m_SerialStore.SerialRead(10);
		if(val > 511) return(val);
		else return(val - 1023);
	}
	else
	{
		m_SerialStore.SerialRead(10);
		val = m_SerialStore.SerialRead(11);
		if(val > 1023) return(val);
		else return(val - 2047);
	}

}

////////////////////////////////////////////////////////////////////////////////////////
// Decode an AC coefficient from the serial read buffer
//
//  Input parameters : 
//				run  -  Pointer to a variable that will hold the decoded run length
//				val  -  Pointer to a variable that will hold the absolute value of the decoded coefficient
//				sign  -  Pointer to a variable that will hold the sign of the decoded coefficient
//
//	Output : returns true if the coefficient was decoded correctly, otherwise returns false
//
//  See 13818-2 Section B-14 for details about the table
//  that is being decoded in this routine
//
bool CAVVideoCompression::DecodeCoeff(int *run, int *val,int *sign)
{

	unsigned int peekVal = m_SerialStore.SerialPeek(16);
	int readVal;

	if(peekVal > 0xBFFF)
	{
		*run = 0;
		readVal = m_SerialStore.SerialRead(3);
		*sign = readVal & 0x01;
		*val = 1;
		return(true);
	}


	if(peekVal > 0x7FFF)
	{
		*run = EOB;
		m_SerialStore.SerialRead(2);
		return(true);

	}

	if(peekVal > 0x5FFF)
	{
		*run = 1;
		readVal = m_SerialStore.SerialRead(4);
		*sign = readVal & 0x01;
		*val = 1;
		return(true);
	}

	if(peekVal > 0x4FFF)
	{
		*run = 2;
		readVal = m_SerialStore.SerialRead(5);
		*sign = readVal & 0x01;
		*val = 1;
		return(true);
	}

	if(peekVal > 0x3FFF)
	{
		*run = 0;
		readVal = m_SerialStore.SerialRead(5);
		*sign = readVal & 0x01;
		*val = 2;
		return(true);
	}

	if(peekVal > 0x37FF)
	{
		*run = 3;
		readVal = m_SerialStore.SerialRead(6);
		*sign = readVal & 0x01;
		*val = 1;
		return(true);
	}

	if(peekVal > 0x2FFF)
	{
		*run = 4;
		readVal = m_SerialStore.SerialRead(6);
		*sign = readVal & 0x01;
		*val = 1;
		return(true);
	}

	if(peekVal > 0x27FF)
	{
		*run = 0;
		readVal = m_SerialStore.SerialRead(6);
		*sign = readVal & 0x01;
		*val = 3;
		return(true);
	}


	if(peekVal > 0x1FFF)
	{
		unsigned char run1[8] = {13, 0,12,11, 3, 1, 0,10};
		unsigned char val1[8] = { 1, 6, 1, 1, 2, 3, 5, 1};

		unsigned int tabIndex = (peekVal >> 8) & 0x07;
		*run = run1[tabIndex];
		readVal = m_SerialStore.SerialRead(9);
		*sign = readVal & 0x01;
		*val = val1[tabIndex];
		return(true);
	}

	if(peekVal > 0x0FFF)
	{
		unsigned char run1[4] = {7,6,1,5};
		unsigned char val1[4] = {1,1,2,1};

		unsigned int tabIndex = (peekVal >> 10) & 0x03;
		*run = run1[tabIndex];
		readVal = m_SerialStore.SerialRead(7);
		*sign = readVal & 0x01;
		*val = val1[tabIndex];
		return(true);
	}

	if(peekVal > 0x07FF)
	{
		unsigned char run1[4] = {2,9,0,8};
		unsigned char val1[4] = {2,1,4,1};

		unsigned int tabIndex = (peekVal >> 9) & 0x03;
		*run = run1[tabIndex];
		readVal = m_SerialStore.SerialRead(8);
		*sign = readVal & 0x01;
		*val = val1[tabIndex];
		return(true);
	}



	// Escape sequence
	if(peekVal > 0x03FF)
	{
		m_SerialStore.SerialRead(6);
		*run = m_SerialStore.SerialRead(6);


		readVal = m_SerialStore.SerialRead(8);
		if(readVal == 0) 
		{
			readVal = m_SerialStore.SerialRead(8);

		}
		else
		{
			if(readVal == 0x80) readVal = m_SerialStore.SerialRead(8) - 256;
			else readVal = (readVal < 128) ? readVal : readVal - 256;
		}

		if(readVal < 0) 
		{
			*sign = 1;
			*val = -readVal;
		}
		else
		{
			*sign = 0;
			*val = readVal;
		}

		return(true);
	}


	if(peekVal > 0x01FF)
	{
		unsigned char run1[8] = {16, 5, 0, 2, 1,15,14, 4};
		unsigned char val1[8] = { 1, 2, 7, 3, 4, 1, 1, 2};

		unsigned int tabIndex = (peekVal >> 6) & 0x07;
		*run = run1[tabIndex];
		readVal = m_SerialStore.SerialRead(11);
		*sign = readVal & 0x01;
		*val = val1[tabIndex];
		return(true);
	}


	if(peekVal > 0x00FF)
	{
		unsigned char run1[16] = { 0, 8, 4, 0, 2, 7,21,20, 0,19,18, 1, 3, 0, 6,17};
		unsigned char val1[16] = {11, 2, 3,10, 4, 2, 1, 1, 9, 1, 1, 5, 3, 8, 2 ,1};

		unsigned int tabIndex = (peekVal >> 4) & 0x0F;
		*run = run1[tabIndex];
		readVal = m_SerialStore.SerialRead(13);
		*sign = readVal & 0x01;
		*val = val1[tabIndex];
		return(true);
	}

	if(peekVal > 0x007F)
	{
		unsigned char run1[16] = {10, 9, 5, 3, 2, 1, 1, 0, 0, 0, 0,26,25,24,23,22};
		unsigned char val1[16] = { 2, 2, 3, 4, 5, 7, 6,15,14,13,12, 1, 1, 1, 1, 1};

		unsigned int tabIndex = (peekVal >> 3) & 0x0F;
		*run = run1[tabIndex];
		readVal = m_SerialStore.SerialRead(14);
		*sign = readVal & 0x01;
		*val = val1[tabIndex];
		return(true);
	}

	if(peekVal > 0x003F)
	{
		*run = 0;
		readVal = m_SerialStore.SerialRead(15);
		*sign = readVal & 0x01;
		*val = 47 - (readVal >> 1);
		return(true);
	}

	if(peekVal > 0x001F)
	{
		unsigned char run1[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1 ,1, 1};
		unsigned char val1[16] = {40,39,38,37,36,35,34,33,32,14,13,12,11,10, 9, 8};

		unsigned int tabIndex = (peekVal >> 1) & 0x0F;
		*run = run1[tabIndex];
		readVal = m_SerialStore.SerialRead(16);
		*sign = readVal & 0x01;
		*val = val1[tabIndex];
		return(true);
	}

	if(peekVal > 0x000F)
	{
		unsigned char run1[16] = { 1, 1 ,1, 1, 6,16,15,14,13,12,11,31,30,29,28,27};
		unsigned char val1[16] = {18,17,16,15, 3, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1};

		unsigned int tabIndex = peekVal & 0x0F;
		*run = run1[tabIndex];
		readVal = m_SerialStore.SerialRead(17);
		*sign = readVal & 0x01;
		*val = val1[tabIndex];
		return(true);
	}


	// Bad code

	return(false);

}




