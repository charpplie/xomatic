////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2010.
// -------------------------------------------------------------------------
//  File name:   AV_CompressionVideo.h
//  Version:     v1.00
//  Created:     09/04/2010 by John Stewart.
//  Compilers:   
//  Description: Header file for Video Encoding Routines.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __AVCompressionVideo_h__
#define __AVCompressionVideo_h__
#pragma once

#include "AVCompressionUtil.h"


//  ITU 601 RGB to YUV conversion coeffs (generated from BT.601)
//#define KRY  66
//#define KGY  129
//#define KBY  25
//#define KRU -38
//#define KGU -74
//#define KBU 112
//#define KRV 112
//#define KGV -94
//#define KBV -18


//  ITU 709 RGB to YUV conversion coeffs (generated from BT.709-5)
#define KRY  47
#define KGY  157
#define KBY  16
#define KRU -26
#define KGU -86
#define KBU 112
#define KRV 112
#define KGV -102
#define KBV -10

// Constants used in the RGB to YUV conversion
#define KSCALESHIFT 8
#define KSCALEHALF 128
#define LUMAFLOOR 16
#define CHROMAFLOOR 128
#define SSESCALE 128  // Addition scaling to take advantage of SSE bit widths
#define KSCALESHIFTSSE 15 // removes the addition 128 scaling
#define LUMAFLOORROUNDSSE 540672
#define CHROMAFLOORROUNDSSE 4210688
#define DEFAULTFCODE 5
#define DEFAULTDCMASK 8

#define MPEG1COEFFLIMIT 255
#define MPEG2COEFFLIMIT 2047

#define MVOUTOFRANGE 99999

// Input video types
#define AVCRGB 0
#define AVCYUV 1

// VLC definitions
#define SLICESTARTCODE 257
#define VLCEOBCODEMPEG2 6
#define VLCEOBCODEMPEG2LEN 4
#define VLCEOBCODEMPEG1 2
#define VLCEOBCODEMPEG1LEN 2
#define VLCESCAPECODE 1

// Define the size of a macroblock
#define BLOCK8x8SIZE 64
#define NUMSUBBLOCKS 6
#define MBSIZE 384
#define BLOCKDIM 8
#define LUMAMBWIDTH 16
#define CHROMAMBWIDTH 8
#define LUMABLOCKSCALE 16
#define CHROMABLOCKSCALE 4
#define BYTESPERPIXEL 4
#define MBHEIGHTSHIFT 4
#define MBROWS 48
#define CHROMAPLANERRATIO 4

#define QUANTTABLESIZE 256
#define QUANTUPSCALE 32
#define QUANTRND 2
#define QUANTSCALE3 3
#define SSEQUANTRND 0.375f

// DCT Coeffs and scaling
#define DCT_C1 4433  // cos(pi*6/16)*sqrt(2)*8192
#define DCT_C2 6270  // 8192/(cos(pi*2/16)*sqrt(2))
#define DCT_C3 15137  // 8192/(cos(pi*6/16)*sqrt(2))
#define DCT_C4 9633  // cos(pi*3/16)*sqrt(2)*8192
#define DCT_C5 2446  // sqrt(2) * (-cos(1*pi/16)+cos(3*pi/16)+cos(5*pi/16)-cos(7*pi/16))
#define DCT_C6 16819  // sqrt(2) * (cos(1*pi/16)+cos(3*pi/16)-cos(5*pi/16)+cos(7*pi/16))
#define DCT_C7 25172  // sqrt(2) * (cos(1*pi/16)+cos(3*pi/16)+cos(5*pi/16)-cos(7*pi/16))
#define DCT_C8 12299  // sqrt(2) * (cos(1*pi/16)+cos(3*pi/16)-cos(5*pi/16)-cos(7*pi/16))
#define DCT_C9 -7373  // sqrt(2) * (cos(7*pi/16)-cos(3*pi/16)
#define DCT_C10 -20995  // sqrt(2) * (-cos(1*pi/16)-cos(3*pi/16)
#define DCT_C11 -16069  // sqrt(2) * (-cos(3*pi/16)-cos(5*pi/16)
#define DCT_C12 -3196  // sqrt(2) * (cos(5*pi/16)-cos(3*pi/16)

#define DCT_SCALE1 4
#define DCT_SCALE2 9
#define DCT_SCALE3 7
#define DCT_ROUND3 32
#define DCT_SCALE4 20
#define DCT_ROUND4 524288

// define some processor types
#define NO_ACCEL_AVAIL 0
#define SSE_AVAIL 1
#define SPU_AVAIL 2

#define VIDEOPESHDROFFSET 6

#define VLCTBLPLIMIT 64
#define VLCTBLMLIMIT -65
#define VLCRUNSIZEBITS 6
#define VLCVALSIZEBITS 12
#define VLCESCSIZEBITS 6

#define VLCADDRESCLIMIT 33
#define MINSLICEROW 0x100
#define MAXSLICEROW 0x1B0

// Types of functions that the thread can perform
#define COMPRESSVIDEO 0
#define DECOMPRESSVIDEO 1

#define IFRAMETYPE 0
#define IREFFRAMETYPE 1
#define PFRAMETYPE 2
#define PREFFRAMETYPE 3
#define BFRAMETYPE 4

#define SKIPTHRESHOLD   10 // Threshold used for skipping
#define NOTCODEDTHRESHOLD   10
#define PINTRATHRESHOLD 64

// macroblock types
#define MB_UNKNOWN 0
#define MB_INTRA 1
#define MB_SKIP 2
#define MB_NOMOTION_CODED 3
#define MB_MOTION_NOTCODED 4
#define MB_MOTION_CODED 5
#define MB_NOMOTION_NOCODED 6  // special case

// Predictor resets
#define RESETDC 1
#define RESETMV 2
#define RESETALL 0xFFFF


//////////////////////////////////////////////////////////////////////////////
// Defines the Video compression class
//////////////////////////////////////////////////////////////////////////////
class CAVVideoCompression : public CrySimpleThread<>
{
public:

	// Constructor and Destructor
	CAVVideoCompression();
	~CAVVideoCompression();


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
	void InitVideoCompression(unsigned int startRow, unsigned int sectionHeight,uint8 *pSerialBuff, unsigned int serialBuffSize,uint8 *pRGBbuffer,uint8 *pYUVbuffer,
							uint8 *pCurRefBuffer, uint8 *pNxtRefBuffer,unsigned int encodedWidth, unsigned int encodedHeight,int mQuant,int videoPesID,
							unsigned int containerStyle,int videoCodecType,int inputFormat,int frameType);


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
	//		intraDcPred		:	DC value quantization
	//		qScaleType		:	linear or non-linear scale array
	//		intraVLCformat	:	Use table B-14 or B-15
	//		coeffLimit		:	255 for MPEG 1 or 2048 for MPEG2
	//		videoCodecType	:	MPEG1 or MPEG2
	//
	//	Output : none
	//
	//
	void InitVideoDeCompression(uint8 *pOutputFrame, uint8 *pSerialBuff, unsigned int serialBuffSize,unsigned int sectionHeight,unsigned int encodedWidth,unsigned int encodedHeight,
								int mQuant,int videoCodecType);



	////////////////////////////////////////////////////////////////////////////////////////
	// Returns the number of bytes encoded by the thread
	//
	//  Input parameters : None
	//
	//	Output : Returns the number of bytes that have been encoded for the video section
	//
	//
	unsigned int GetEncodedBytes() { return m_encodedBytes;}



	////////////////////////////////////////////////////////////////////////////////////////
	// Returns the address of the buffer holding the encoded data for the thread
	//
	//  Input parameters : none
	//
	//	Output : Returns the address of the buffer holding the encoded data for the thread
	//
	//
	uint8 *GetBufferAddr() { return m_SerialStore.GetBufferAddress();}



	////////////////////////////////////////////////////////////////////////////////////////
	// Thread that encodes a section of the input frame
	//
	//  Input parameters : none
	//
	//	Output : none
	//
	void Run(void);

	////////////////////////////////////////////////////////////////////////////////////////
	// Cancel does nothing in this thread
	//
	//  Input parameters : none
	//
	//	Output : none
	//
	void Cancel(void);


protected:	



private:


	////////////////////////////////////////////////////////////////////////////////////////
	// Convert a portion of a frame that is in RGBA format to planer Y,Cr,Cb format
	// using SSE2 if it is available
	// Each input pixel is represented by 32 bits (4 bytes of RGBA)
	// Each output pixel is represented by a short Y, short Cr and short Cb sample
	// The Y, Cr,Cb values are in separate planer arrays
	//
	//  Input parameters : none
	//
	//	Output : None
	//
	//
	void ConvertFrameRGB2YCrCb(void);


	///////////////////////////////////////////////////////////////////////////////////////////////
	// Compress a portion of a frame consisting of a number of consecutive slices (macroblock rows)
	//
	//  Input parameters : none
	//
	//	Output : None
	//
	//
	void CompressFrame(void);


	////////////////////////////////////////////////////////////////////////////////////////
	// VLC encode a macroblock into a serial buffer
	//
	//  Input parameters :
	//		pblk :  Pointer to the macroblock which consists of 64 shorts
	//		cc :  0 for the 4 luma blocks of a macroblock, 1 and 2 for the 2 chroma blocks of a macroblock
	//		lastnonzero  :  gives the index to the last nonzero value in a macroblock
	//
	//	Output : None
	//
	//
	void PutIntraBlk(short *pblk,unsigned int cc,unsigned int lastNonZero);

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
	void PutInterBlk(short *pblk,unsigned int lastNonZero);

	////////////////////////////////////////////////////////////////////////////////////////
	// VLC encode a DC value of a macroblock 
	//  Input parameters :
	//		cc :  0 for the 4 luma blocks of a macroblock, 1 and 2 for the 2 chroma blocks of a macroblock
	//		DCval  :  DC value to be encoded
	//
	//	Output : None
	//
	//
	void PutDC(unsigned int cc,int DCval);


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
	void PutACb14(unsigned int run,int ACval);

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
	void PutACb15(unsigned int run,int ACval);


	////////////////////////////////////////////////////////////////////////////////////////
	// VLC encode motion vectors into a serial buffer
	//
	//  Input parameters :
	//		MVX :  motion vector in the X direction
	//		MVY :  motion vector in the Y direction
	//
	//	Output : None
	//
	void EncodeMotion(int MVX,int MVY);


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
	unsigned int ConvertScanOrder(short *pDin,short *pDout,int MBType);


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
	void ForwardDctQuantize(short *pDatain, short *pQuantizedOutput,int MBType);



	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Perform an in-place forward DCT on a 64 sample block of data
	//  Based on the following paper  :  C. Loeffler, A. Ligtenberg and G. Moschytz, 
	//  "Practical Fast 1-D DCT Algorithms with 11 Multiplications", Proc. Int'l. Conf. on Acoustics,
	//  Speech, and Signal Processing 1989 (ICASSP '89), pp. 988-991.
	//
	//
	//  Input parameters :
	//		pData : pointer to the input data, output data will be placed here as well
	//
	//	Output : none
	//
	void ForwardDct(short *pData);



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
	void ForwardDctSSE2(short *pData);


	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Quantize an 8x8 subblock of video samples
	// per ISO 13818-2 section 7.4
	//
	//
	//  Input parameters :
	//		pDin	:  pointer to the input macroblock (64 luma0 + 64 luma1 + 64 luma2 + 64 luma3 + 64 Cr + 64 Cb)
	//		pDout  :  pointer to hold the processed data (64 luma0 + 64 luma1 + 64 luma2 + 64 luma3 + 64 Cr + 64 Cb)
	//
	//	Output : none
	//
	void QuantizeIntra(short *pDin,short *pDout);


	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Quantize an 8x8 subblock of inter video samples
	//
	//  Input parameters :
	//		pDin	:  pointer to the input macroblock (64 luma0 + 64 luma1 + 64 luma2 + 64 luma3 + 64 Cr + 64 Cb)
	//		pDout	:  pointer to hold the processed data (64 luma0 + 64 luma1 + 64 luma2 + 64 luma3 + 64 Cr + 64 Cb)
	//
	//	Output : none
	//
	void QuantizeInter(short *pDin,short *pDout);


	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Quantize an 8x8 subblock of video samples using SSE2 instructions
	// per ISO 13818-2 section 7.4
	//
	//
	//  Input parameters :
	//		pDin	:  pointer to the input macroblock (64 luma0 + 64 luma1 + 64 luma2 + 64 luma3 + 64 Cr + 64 Cb)
	//		pDout  :  pointer to hold the processed data (64 luma0 + 64 luma1 + 64 luma2 + 64 luma3 + 64 Cr + 64 Cb)
	//
	//	Output : none
	//
	void QuantizeIntraSSE2(short *pDin,short *pDout);



	////////////////////////////////////////////////////////////////////////////////////////
	// Generate a pes and slice header
	//  Input parameters :
	//			sliceRow :  number of the slice row, 0=first row
	//
	//	Output : None
	//
	//
	void GenSliceHdr(unsigned int sliceRow);

	////////////////////////////////////////////////////////////////////////////////////////
	// Generate a pes elementary stream header
	//
	//  Input parameters : None
	//
	//	Output : Starting buffer location
	//
	//
	unsigned int GenPesElemHdr(void);

	////////////////////////////////////////////////////////////////////////////////////////
	// Finds the number of bits needed to represent a positive value
	//  Input parameters :
	//		val :  val to check, should always be > 0
	//
	//	Output : returns the number of bits beeded to represent the value (i.e., the location of
	//			 the highest non-zero bit
	//
	//
	unsigned int FindMinBits(unsigned int val);


	////////////////////////////////////////////////////////////////////////////////////////
	// Calculate a quantization table based on the input quantization parameters
	// Based on ISO 13818-2 sec 7.4
	//
	//  Input parameters : none
	//
	//	Output : None
	//
	//
	void SetQuantTable(void);


	////////////////////////////////////////////////////////////////////////////////////////
	// Decompress a series of MPEG-1 video frames
	//
	//  Input parameters : none
	//
	//	Output : returns true if successful, otherwise returns false
	//
	//
	bool DecompressFrame(void);


	////////////////////////////////////////////////////////////////////////////////////////
	// Align the serial reader with a slice start code
	//
	//  Input parameters : none
	//
	//	Output : returns true if successful, otherwise returns false
	//
	//
	bool FindStartCode(void);

	////////////////////////////////////////////////////////////////////////////////////////
	// Decode an MPEG-1 macroblock
	//
	//  Input parameters : none
	//
	//	Output : returns true if successful, otherwise returns false
	//
	//
	bool DecodeMB(int *pMBdata);


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
	bool DecodeSubblock(int *pdata,int type);


	////////////////////////////////////////////////////////////////////////////////////////
	// Decode a luma DC coefficient from the serial read buffer
	//
	//  Input parameters : none
	//
	//	Output : returns the value of the decoded coefficient
	//
	//  See 13818-2 Section B.5 for details about the table
	// that is being decoded in this routine
	//
	int GetLumaDC(void);


	////////////////////////////////////////////////////////////////////////////////////////
	// Decode a Chroma DC coefficient from the serial read buffer
	//
	//  Input parameters : none
	//
	//	Output : returns the value of the decoded coefficient
	//
	//  See 13818-2 Section B-13 for details about the table
	// that is being decoded in this routine
	//
	int GetChromaDC(void);


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
	bool DecodeCoeff(int *run, int *val,int *sign);

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
	void PlaceMB(unsigned int x, unsigned int y,uint8 *pMBData,unsigned char *pFrameBuffer,int dataType);


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
	void getMB(unsigned int x, unsigned int y,short *pMBbuff,uint8 *pFrameBuffer);

	////////////////////////////////////////////////////////////////////////////////////////
	// Extract a macroblock from a planer frame
	//
	//  Input parameters :
	//		x : x pixel location of the macroblock
	//		y : y pixel lcoation of the macroblock
	//		pMBbuff : Pointer to a buffer that will hold the extracted macroblock (384 shorts = 64 Luma0 + 64 Luma1 +64 Luma2 +64 Luma3 + 64 CR + 64 Cb values)
	//		pFrameBuffer : Pointer to a buffer that holds the planer frame
	//
	//	Output : None
	//
	//
	void GetBlock(unsigned int x, unsigned int y,short *pMBbuff,uint8 *pFrameBuffer);


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
	void InverseDCT(int *pData);

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
	void InverseDctSSE2(int *pData);


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
	void InverseQuantDct(short *pdin,int *pdout,int MBType);

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
	void EncodeDataVLC(short *pBuff,int MBType,int codedPattern);

	////////////////////////////////////////////////////////////////////////////////////////
	// Encode the macroblock type
	//
	//  Input parameters : 
	//		MBType  --  Type of the macroblock
	//
	//	Output : none
	//
	//
	void EncodeMBType(int MBType);

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
	int CheckForSkip(int x,int y,short *pMBbuff);

	////////////////////////////////////////////////////////////////////////////////////////
	// Encode the macroblock address
	//
	//  Input parameters : 
	//
	//		skipCount  -  number of macroblocks that were skipped + 1
	//
	//	Output : none
	//
	void EncodeMBAddr(int skipCount);

	////////////////////////////////////////////////////////////////////////////////////////
	// Reset the DC and motion vector predictors
	//
	//  Input parameters : 
	//		resetType  --  RESETDC,RESETMV or RESETALL
	//
	//	Output : none
	//
	void ResetPredictors(int resetType);

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
	void MBsearch(int x,int y,short *pMBbuff,short *pMBref,int *pBestX,int *pBestY);

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
	//
	int FindMBType(int x,int y,short *pMBbuff,int *pBestX,int *pBestY);

	////////////////////////////////////////////////////////////////////////////////////////
	// VLC encode a differential motion vector into a serial buffer
	//
	//  Input parameters :
	//		MV :  differential motion vector
	//		fCode :  Motion vector range as encoded in the picture header
	//
	//	Output : None
	//
	void PutMV(int MV, int fCode);

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
	void InverseQuant(short *pdin,int *pdout,int MBType);


	////////////////////////////////////////////////////////////////////////////////////////
	// Calculate the coded block pattern for a macroblock
	//
	//  Input parameters : 
	//		pbuff  --  pointer to a macroblock
	//
	//	Output : Returns the coded block pattern for the macroblock
	//
	//
	int FindCBP(short *pBuff);

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
	float CalcSSE(short *pMB1,short *pMB2);




	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Variables

	unsigned int m_functionType;		// Either COMPRESSVIDEO or DECOMPRESSVIDEO
	unsigned int m_startRow;			// Starting pixel row for this thread
	unsigned int m_sectionHeight;		// number of rows to process
	unsigned int m_MBWidth;             // width of frame in macroblocks
	CAVCSerialBuffer m_SerialStore;		// serial line buffer for this thread
	int m_dcDctPred[3];				// DC predictor
	int m_pmv[2];						// Motion vector predictor
	int m_fcodeHoriz;				// horizontal f_code to use
	int m_fcodeVert;				// vertical f_code to use



	uint8 *m_pInputFrame;				// address of the input frame (RGB)
	uint8 *m_pOutputFrame;				// address of the output frame (YUV)
	int m_inputFormat;					// AVCRGB or AVCYUV
	unsigned int m_encodedHeight;		// encoded height of the frame
	unsigned int m_encodedWidth;		// encoded width of the frame
	int m_simdTypeAvail;				// indicates what type if SIMD is availalbe
	int m_mQuant;						// quantization for the frame
	int m_prevmQuant;					// previous quantization value
	int m_dcPredShift;					// intra DC shift value
	int m_intraDcPred;					// intra DC quant value
	unsigned int m_zeroRowCount;		// number of zero rows in the quant table
	int m_videoPesID;					// video PES ID to use if outputing PES headers

	unsigned int m_encodedBytes;		// Number of bytes that were encoded
	unsigned int m_containerStyle;		// container type , e.g., AVI, mpeg
	unsigned int m_qScaleType;			// linear or non linear
	unsigned int m_intraVLCformat;		// use table B-14 or B-15 for intra
	int m_coeffLimit;					// limits on coeff values
	int m_videoCodecType;				// codec style MPEG -1 or MPEG-2
	int m_frameType;					// frame type e.g., I/P/B
	uint8 *m_pCurRefBuffer;				// address of the current reference buffer
	uint8 *m_pNxtRefBuffer;				// address of the next reference buffer

	short m_quantTable[64][256];		// Pre-calculated coeff quantization table
	float m_quantScaleA[32];			// Pre-calculated quant value for SSE (even values)
	float m_quantScaleB[32];			// Pre-calculated quant value for SSE (odd values)


};

#endif //__AVCompressionVideo_h__




