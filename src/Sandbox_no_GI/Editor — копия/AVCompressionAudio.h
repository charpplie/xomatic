////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2010.
// -------------------------------------------------------------------------
//  File name:   AV_CompressionAudio.h
//  Version:     v1.00
//  Created:     09/04/2010 by John Stewart.
//  Compilers:   
//  Description: Header file for Audio Encoding Routines.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __AVCompressionAudio_h__
#define __AVCompressionAudio_h__
#pragma once

#include "AVCompressionUtil.h"

// Audio packet header sync value
#define AUDIOSYNCWORD	0xFFF

// Audio encoding definitions used during bit allocation
#define AUDIO_SB_EMPTY		0
#define AUDIO_SB_ALLOCATED  1
#define AUDIO_SB_FINISHED   2

// Possible sampling frequencies in Hz
#define SAMPLINGFREQ48000 48000
#define SAMPLINGFREQ44100 44100
#define SAMPLINGFREQ32000 32000

// Audio data rate after compression (192K per channel, 2 channel stereo)
#define COMPRESSEDAUDIORATE 384000

// bit widths for encoded variables
#define SCALECODEBITWIDTH 2
#define SCALEFACTORBITWIDTH 6

// Width of mpeg audio windowing filter ISO 11172-3 3-C.1.5.2
#define MPEGAUDIOWINDOWWIDTH 512
#define FILTSPACING 64
#define FILTFOLD 16
#define SLOTSIZE 8


// Each mpeg audio frame is composed of 32 subbands
// each subband is divided into 3 groups of 12 samples each
// The total samples in a subband is therefore 3*12*32 = 1152
// See ISO 11172-3 3-C.1.5.2
#define MPEGAUDIOFRAMEWIDTH 1152
#define MPEGAUDIOSBNUM 32
#define MPEGAUDIOSBGROUPS 3
#define MPEGAUDIOGROUPSIZE 12
#define MPEGAUDIOSUBGROUPSIZE 4


// Number of subbands from ISO 11172-3 table 3-B.2a for 48000 sampling rate, 192K output rate
#define SBLIMIT1 27
// Number of subbands from ISO 11172-3 table 3-B.2b for 44100 and 32000 sampling rate, 192K output rate
#define SBLIMIT2 30
// Fixed packet overhead for 48000 sampling rate, 192K output rate
#define AUDIOPACKOVERHEAD1 208
// Fixed packet overhead for 44100 and 32000 sampling rate, 192K output rate
#define AUDIOPACKOVERHEAD2 220

// WIdth of the bit allocation table
#define BITALLOCTABLEWIDTH 15

// Possible scale factor transmission patterns
#define SENDALL 0
#define SEND122 1
#define SEND133 2
#define SEND113 3
#define SEND111 4
#define SENDMAX 5
#define SEND333 6
#define SEND222 7

// Define some codes for the number of scale codes that are send
#define SEND3	0
#define SEND2A	1
#define SEND1	2
#define SEND2B	3

// MPEG layer II data window coeff scaling
//#define MPEGAUDIOWINDSCALING 14  // value that was used to scale the windowing coeffs - 7 (e.g., scaled by 2^21)


// IDCT Coeffs based on Byeong Gi Lee. "A New Algorithm to Compute the Discrete Cosine Transform".
#define AUDIOINVSQRT2 0.70710678118654f
#define C0_12 0.5411961001461970f
#define C0_4 1.306562964876376f
#define C1_4 2.562915447741505f
#define C1_12 0.6013448869350452f
#define C1_20 0.8999762231364155f
#define C1_28 0.5097955791041591f
#define C2_2 5.101148618689155f
#define C2_6 0.6468217833599900f
#define C2_10 1.060677685990347f
#define C2_14 0.5224986149396888f
#define C2_18 1.722447098238334f
#define C2_22 0.5669440348163576f
#define C2_26 0.7881546234512502f
#define C2_30 0.502419286188155f
#define C3_0 0.500602998235196f
#define C3_2 0.7445362710022985f
#define C3_4 0.553103896034444f
#define C3_6 1.484164616314166f
#define C3_8 0.5154473099226245f
#define C3_10 0.9725682378619607f
#define C3_12 0.6225041230356648f
#define C3_14 3.407608418468718f
#define C3_16 0.5054709598975436f
#define C3_18 0.8393496454155268f
#define C3_20 0.5829349682061338f
#define C3_22 2.057781009953410f
#define C3_24 0.5310425910897841f
#define C3_26 1.169439933432884f
#define C3_28 0.67480834145500f
#define C3_30 10.1900081235480f


#define AUDIO_BUFF_SIZE 4096   // size of the polyphase audio filter sample state buffer (must be > 512 samples but size is not critical)

#define AUDIOPESHDROFFSET 6


// Defines the Audio compression class
class CAVAudioCompression
{
public:

	// Constructor and Destructor
	CAVAudioCompression();
	~CAVAudioCompression();


	////////////////////////////////////////////////////////////////////////////////////////
	// Initialize the audio encoder
	//
	//  Input parameters :
	//		samplingFreq	:  sampling rate of the input audio (should be 48000, 44100, or 32000)
	//		pesHdrFlag		:  true means to surround the audio in a pes packet.  false means do not add a pes header
	//		streamID		:  stream id to use in the pes header
	//		containerFormat	:  indicates mpeg 1/2 or AVI
	//
	//	Output : true if initialized, false otherwise
	//
	//    Note :	MPEG 1 - layer 2 only allows for 48000,44100, and 32000 sampling frequencies
	//				all others are invalid and will result in an error being returned
	//
	bool InitAudioEncoder(unsigned int samplingFreq, int streamID, int containerFormat);


	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Process raw input audio samples to VLC encoded serialized output
	// according to the ISO 11172-3 specification (aka MPEG-1 layer 2 audio encoding)
	//
	//
	//  Input parameters : 
	//		pAudioCapture	: pointer to an audio capture object
	//		pBuffer			: pointer to a buffer for use in holding the encoded audio
	//		bufSize			: size of the buffer in bytes
	//		packetTime		: Packet time to use in the pes header (in 27 MHz clk counts) if there is one
	//
	//	Output : returns the number of encoded bytes that were put into the buffer
	//
	unsigned int EncodeAudio(CAVAudioCapture *pAudioCapture,uint8 *pBuffer,unsigned int buffSize,float packetTime);


	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Return the number of audio blocks that have been encoded since initialization
	//
	//  Input parameters : none
	//
	//	Output : returns the number of encoded audio blocks since initialization
	//
	unsigned int GetEncodedBlocks(void) {return(m_encodedAudioBlocks);}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Returns the size of the encoded audio data in bytes
	//
	//  Input parameters : none
	//
	//	Output : Returns the size of the encoded audio data in bytes
	//
	unsigned int GetEncodedSize(void) {return(m_audioSerialBuffer.GetBytes());}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Returns the starting address of the encoded audio data
	//
	//  Input parameters : none
	//
	//	Output : Returns the starting address of the encoded audio data
	//
	uint8 *GetBufferAddress(void) {return(m_audioSerialBuffer.GetBufferAddress());}


protected:	



private:


	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Process a group of 1152 samples from raw input samples to VLC encoded serialized output
	// according to the ISO 11172-3 specification (aka MPEG-1 layer 2 audio encoding)
	//
	//  Input parameters : none
	//
	//	Output : none
	//
	void EncodeAudioFrame(void);



	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Encode the audio frame into a serial line buffer
	// See ISO 11172-3 section 2.4.1 for the meaning of the various bit fields
	//
	//  Input parameters : none
	//
	//	Output : none
	//
	//
	void  SerializeAudioFrame(void);



	////////////////////////////////////////////////////////////////////////////////////////
	// Converts the time domain 1152 samples to the freq domain (1152 samples consisting of 32 sub bands)
	// per ISO 11172 sec. 3-C.1.3
	// Output data is arranged as follows
	//
	//	   A0     A1     A2     A3     ...                                A35
	//  |  32  |  32  |  32  |  32  |  32  |  32  |  32  |  32  | ...  |  32  |
	//  Block A0 contains sample 0 from bank 0, sample 0 from bank 1, sample 0 from bank 2, ..., sample 0 from bank 31
	//  Block A1 contains sample 1 from bank 0, sample 1 from bank 1, sample 1 from bank 2, ..., sample 1 from bank 31
	//   .
	//   .
	//   .
	//  Block A35 contains sample 35 from bank 0, sample 35 from bank 1, sample 35 from bank 2, ..., sample 35 from bank 31
	//
	//  Input parameters : none
	//
	//	Output : none
	//
	//
	void ConvertAudioFreqBands(void);



	////////////////////////////////////////////////////////////////////////////////////////
	// Compute the scale codes for a block of data (1152 samples)
	// For each of the 32 sub-bands, divide the 36 samples into 3 sets of 12 samples each
	// Find the maximum value for each of the 3 sub sections
	// Find how many bits are needed to represent the largest values
	// Use this to calculate the 3 scale factors for each band
	// Use the scale factors to find the transmission pattern
	// See ISO 11172-3 3-C.1.5.2
	//
	//  Input parameters : none
	//
	//	Output : none
	//
	//
	void ComputeScaleCodes(void);


	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Compute the number of bits that will be in an encoded audio frame
	// This does not include pes header information, only the audio frame size
	//
	//  Input parameters : none
	//
	//	Output : number of bits that will be in an encoded frame
	//
	//
	unsigned int GetEncodedFrameSize(void);


	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Compute the number of bits that will be used for each sub-band and place in the bit_alloc array
	// Use a fixed signal to mask ratio table instead of a psycho acoustic model
	// See ISO 11172-3 3-C.1.5.2 paragraph 7
	//
	//  Input parameters : none
	//
	//	Output : none
	//
	//
	void AllocateSubbandBits(void);



	////////////////////////////////////////////////////////////////////////////////////////////////////
	// This routine checks to see if an extra byte is needed for the next packet
	// This only occurs for the 44100 sampling case because the 48000 and 32000 cases
	// always divide the 1152*bitrate evenly
	//
	//  Input parameters : none
	//
	//	Output : returns 1 if extra byte is needed, 0 otherwise
	//
	//
	unsigned int CheckAudioPacketSize(void);



	////////////////////////////////////////////////////////////////////////////////////////
	// 32 point 1D inverse DCT 
	// Uses floating point arithmetic and then rounds back to integer before output
	// From Byeong Gi Lee. "A New Algorithm to Compute the Discrete Cosine Transform".
	//  IEEE 7\-ansactions on Acoustic, Speech, and Signal Processing, 32(6):1243-1247,
	//  December 1984.
	//
	//  Input parameters :
	//		pDataIn		: pointer to array of 32 input samples to be transformed
	//		pDataOut	: pointer to array of 32 output samples
	//
	//	Output : none
	//
	//
	//   Notes :  Some loops are unrolled for speed
	//			  
	void InverseDCT32(float *pDataIn, int *pDataOut);


	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Send an audio pes header with timing
	// See ISO 13818-1 sec. 2.4.3.6 for field definitions
	//
	//  Input parameters : none
	//
	//	Output : none
	//
	//
	void SendAudioPesHdr(void);


	////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Variables

	// m_Audio_Context that holds the current state of the audio compression engine
	CAVCSerialBuffer m_audioSerialBuffer;			// Serial buffer for use in holding encoded audio
	int m_filtBuff[2][AUDIO_BUFF_SIZE];				// windowed filter state for 2 channels
	unsigned int m_filterShifterOffset;				// holds current location in the filter buffer
	unsigned int m_subBandLimit;					// Number of bands to use for this sampling rate
	unsigned int m_audioSampFreq;					// Sampling rate of the input audio
	unsigned int m_audioEncodedBitrate;				// Output encoded bitrate
	float m_audioRateFracByte;						// Used for 44100 sampling to adjust packet size
	unsigned int m_audioFrameFillBits;				// number of bits needed to fill the frame to capacity
	int m_sample_band_ch[2][MPEGAUDIOSBGROUPS][MPEGAUDIOGROUPSIZE][MPEGAUDIOSBNUM]; // Holds 2 channels of audio in the freq domain
	int m_scale_code[2][MPEGAUDIOSBNUM];			// Holds the scaling codes for 2 channels for each of the subbands
	int m_scale_factors[2][MPEGAUDIOSBNUM][MPEGAUDIOSBGROUPS];		// Holds the scaling factors for each of the groups in the subbands
	unsigned int m_bit_alloc[2][MPEGAUDIOSBNUM];	// holds the bit allocations for the subbands
	int m_streamID;									// stream id to use in the pes header if there is one
	int m_rawAudioBuff[2*MPEGAUDIOFRAMEWIDTH];		// holds 2 channels of pre encoded audio
	float m_packetTime;								// Holds the current time
	int m_containerFormat;							// Indicates mpeg 1/2 or AVI
	unsigned int m_encodedAudioBlocks;                     // Number of blocks of audio that have been encoded

};


#endif //__AVCompressionAudio_h__


