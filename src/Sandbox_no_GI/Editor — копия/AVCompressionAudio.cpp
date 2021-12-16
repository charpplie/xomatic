////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2010.
// -------------------------------------------------------------------------
//  File name:   AV_CompressionAudio.cpp
//  Version:     v1.00
//  Created:     09/04/2010 by John Stewart.
//  Compilers:   
//  Description: MPEG Layer II Audio Encoding Routines.
//				  Please see ISO 11172-3 for details
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "AVCompressionAudio.h"
#include "AVCompressionAudioTable.h"


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAVAudioCompression Class routines
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Constructor
CAVAudioCompression::CAVAudioCompression()
{

	// Clear the windowed data buffer so that there is not
	// random audio on startup
	m_filterShifterOffset = AUDIO_BUFF_SIZE - (MPEGAUDIOWINDOWWIDTH-MPEGAUDIOSBNUM);
	memset(m_filtBuff,0,2*AUDIO_BUFF_SIZE*sizeof(int));
}


// Destructor
CAVAudioCompression::~CAVAudioCompression()
{

}


////////////////////////////////////////////////////////////////////////////////////////
// Initialize the audio encoder
//
//  Input parameters :
//		samplingFreq	:  sampling rate of the input audio (should be 48000, 44100, or 32000)
//		pesHdrFlag		:  true means to surround the audio in a pes packet.  false means do not add a pes header
//		streamID		:  stream id to use in the pes header
//		containerFormat		:  1 indicates mpeg 1, 2 indicates mpeg 2
//
//	Output : true if initialized, false otherwise
//
//    Note :	MPEG 1 - layer 2 only allows for 48000,44100, and 32000 sampling frequencies
//				all others are invalid and will result in an error being returned
//
bool CAVAudioCompression::InitAudioEncoder(unsigned int samplingFreq, int streamID, int containerFormat)
{

	// Check for supported sampling frequency
	if(samplingFreq != SAMPLINGFREQ48000 && samplingFreq != SAMPLINGFREQ44100 && samplingFreq != SAMPLINGFREQ32000) return(false);



	// Set the sampling freq and the associated sub band limits based on 
	// ISO 11172-3 tables 3-B.2a,b,c
	m_audioSampFreq = samplingFreq;
	if(m_audioSampFreq == SAMPLINGFREQ48000) m_subBandLimit =  SBLIMIT1;
	else m_subBandLimit =  SBLIMIT2;

	m_audioEncodedBitrate = COMPRESSEDAUDIORATE;	// compressed bitrate
	m_audioRateFracByte = 0.0f;						// Used for 44100 sampling rate to sometimes add an extra slot in the encoded frame
													// to make the average rate correct.  Used to calculate padding_bit field (ISO 11172 sec. 2.4.2.3)

	// Save pes flags
	m_streamID = streamID;
	m_containerFormat = containerFormat;
	m_encodedAudioBlocks = 0;

	return(true);

}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Process raw input audio samples to VLC encoded serialized output
// according to the ISO 11172-3 specification (aka MPEG-1 layer 2 audio encoding)
//
//  Input parameters : 
//		pAudioCapture	: pointer to an audio capture object
//		pBuffer			: pointer to a buffer for use in holding the encoded audio
//		bufSize			: size of the buffer in bytes
//		packetTime		: Packet time to use in the pes header (in 27 MHz clk counts) if there is one
//
//	Output : returns the number of encoded bytes that were put into the buffer
//
unsigned int CAVAudioCompression::EncodeAudio(CAVAudioCapture *pAudioCapture,uint8 *pBuffer,unsigned int buffSize,float packetTime)
{

	unsigned int numAudioSampsAvail = pAudioCapture->GetAudioNumSamples();
	unsigned int numAudioBytesEncoded = 0;
	m_packetTime = packetTime;

	// Initialize the serial buffer 
	m_audioSerialBuffer.Init(pBuffer,buffSize,SERIALWRITE);


	// If there are enough samples, then encode the audio otherwise just skip
	if(numAudioSampsAvail >= MPEGAUDIOFRAMEWIDTH)
	{

		// Send a header if needed
		if(m_containerFormat == MPEG1_STYLE ||   m_containerFormat == MPEG2_STYLE) SendAudioPesHdr();


		// Construct as many audio packets as possible
		while(numAudioSampsAvail >= MPEGAUDIOFRAMEWIDTH) 
		{
			pAudioCapture->GetAudioData(m_rawAudioBuff,MPEGAUDIOFRAMEWIDTH);
			EncodeAudioFrame();
			numAudioSampsAvail -= MPEGAUDIOFRAMEWIDTH;
		}

		numAudioBytesEncoded = m_audioSerialBuffer.GetBytes();

		// Force any partial words to be written to memory
		m_audioSerialBuffer.AlignToWord(0);


		if(m_containerFormat == MPEG1_STYLE ||   m_containerFormat == MPEG2_STYLE)
		{
			// fix the pes packet size
			// The pes packet length does not count the first 6 bytes
			unsigned int audioPesSize = numAudioBytesEncoded - AUDIOPESHDROFFSET;
			uint8 *pTmp = m_audioSerialBuffer.GetBufferAddress();
			pTmp[4] = (uint8) ((audioPesSize >> 8) & 0xFF);
			pTmp[5] = (uint8) ((audioPesSize) & 0xFF);
		}
	}

	return(numAudioBytesEncoded);

}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Process a group of 1152 samples from raw input samples to VLC encoded serialized output
// according to the ISO 11172-3 specification (aka MPEG-1 layer 2 audio encoding)
//
//  Input parameters : none
//
//	Output : none
//
void CAVAudioCompression::EncodeAudioFrame(void)
{


	// Convert the input frame from time to freq domain
	// ISO 11172-3 sec. 3-C.1.5.2 para. 3
	ConvertAudioFreqBands();

	// Compute the scale codes and factors for each of the sub-bands
	// ISO 11172-3 sec. 3-C.1.5.2 para. 4,5,6
	ComputeScaleCodes();

	// Compute bit allocations for each sub-band
	// ISO 11172-3 sec. 3-C.1.5.2 para. 7
	AllocateSubbandBits();

	// Quantize and encode the frame
	// ISO 11172-3 sec. 3-C.1.5.2 para. 8,9,10
	SerializeAudioFrame();

	++m_encodedAudioBlocks;

}


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
void CAVAudioCompression::ConvertAudioFreqBands(void)
{

	unsigned int filterOffset;

	// Input data is processed 32 samples at a time by shifting 32 samples into
	// a filter state register.  512 samples are processed (32 new and 480 old) to yield 32 output samples
	// Then another 32 samples are shifted in and processed.  This continues until all 1152 samples are processed


    // Perform the conversion for each of the two channels
	for(unsigned int channel=0;channel < 2;++channel)
	{

		int inputOffset = channel;
		int *pDataOut = &m_sample_band_ch[channel][0][0][0];

		// Get the current offset from the filter state
		filterOffset = m_filterShifterOffset;


		// The 1152 sample frame is processed in 36 batches of 32 samples each.
		// Each batch produces 32 output samples, one sample for each freq band per
		// ISO 11172 sec. 3-C.1.3
		for(unsigned int j=0;j<MPEGAUDIOSBGROUPS*MPEGAUDIOGROUPSIZE;++j) 
		{

			// See if there is room in the shift register for new samples
			if(filterOffset < MPEGAUDIOSBNUM)
			{
				// Make room
				int copyLength = MPEGAUDIOWINDOWWIDTH - MPEGAUDIOSBNUM;
				memcpy(&m_filtBuff[channel][AUDIO_BUFF_SIZE - copyLength],&m_filtBuff[channel][filterOffset],copyLength*sizeof(int));
				filterOffset = AUDIO_BUFF_SIZE - copyLength;
			}


			// copy the new data into the buffer
			for(unsigned int k=0;k<MPEGAUDIOSBNUM;++k) 
			{
				m_filtBuff[channel][filterOffset-1] = m_rawAudioBuff[inputOffset];
				inputOffset += 2;
				--filterOffset;
			}


			// Perform the windowing function per ISO 11172 sec. 3-C.1.3
			float sum[FILTSPACING];
			int *p0 = m_filtBuff[channel] + filterOffset;
			const float *wc = AudioWindowCoeffs;
			for(unsigned int i=0;i<FILTSPACING;++i) 
			{
				sum[i] =	p0[0*FILTSPACING]*wc[0*FILTSPACING] + p0[1*FILTSPACING]*wc[1*FILTSPACING] + p0[2*FILTSPACING]*wc[2*FILTSPACING] + p0[3*FILTSPACING]*wc[3*FILTSPACING] + 
					p0[4*FILTSPACING]*wc[4*FILTSPACING] + p0[5*FILTSPACING]*wc[5*FILTSPACING] + p0[6*FILTSPACING]*wc[6*FILTSPACING] + p0[7*FILTSPACING]*wc[7*FILTSPACING];
				++p0;
				++wc;
			}

			// Scale the results and fold
			float winOut[MPEGAUDIOSBNUM];
			winOut[0] = sum[FILTFOLD];
			for(unsigned int i=1; i < FILTFOLD+1; ++i) 
			{
				winOut[i] = (sum[i+FILTFOLD]+sum[FILTFOLD-i]);
			}
			for(unsigned int i=FILTFOLD+1; i < MPEGAUDIOSBNUM; ++i) 
			{
				winOut[i] = (sum[i+FILTFOLD]-sum[FILTFOLD+FILTSPACING-i]);
			}


			// Finish converting the windowed stereo data to the frequency domain
			// using an inverse DCT method
			InverseDCT32(winOut,pDataOut);

			// Update the output pointers for the next group of samples
			pDataOut += MPEGAUDIOSBNUM;

		}

	}

	// save the location of shift buffer for the next block of data
	m_filterShifterOffset = filterOffset;


}


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
void CAVAudioCompression::ComputeScaleCodes(void)
{


	// Loop over both channels
	for(unsigned int channel=0;channel < 2;++channel) 
	{

		// Do for each sub band
		for(unsigned int j=0;j<m_subBandLimit;++j) 
		{

			// Each sub band has 3 scale factors, one for each group of 12 samples
			for(unsigned int i=0;i<MPEGAUDIOSBGROUPS;++i) 
			{

				// Find the largest sample in the current group of 12
				int max12 = abs(m_sample_band_ch[channel][i][0][j]);
				for(unsigned int k=1;k<MPEGAUDIOGROUPSIZE;++k) 
				{
					int x = abs(m_sample_band_ch[channel][i][k][j]);
					if (x > max12) max12 = x;
				}

				// Search the scale factor table for the location of the max value that was just found
				// use a binary search for speed
				int sfIndex = 32;
				if(max12 > 1) 
				{
					int binInc = 16;
					while(binInc > 0) 
					{
						if(max12 < AudioScalingTable[sfIndex]) sfIndex += binInc;
						else sfIndex -= binInc;
						binInc >>= 1;
					}

					if(sfIndex > 1) sfIndex -= 2;
					else sfIndex = 0;
					while (max12 <= AudioScalingTable[sfIndex+1]) sfIndex++;

				} 
				else 
				{
					sfIndex = 62;
				}



				// save the scale factor 
				m_scale_factors[channel][j][i] = sfIndex;
			}

			// Based on the scale factors, find the classes and transmission pattern
			// per ISO 11172 section 3-C.1.5.2, paragraph 4 and 5
			int dscf1 = m_scale_factors[channel][j][0] - m_scale_factors[channel][j][1];
			dscf1 = 3 + CLAMP(dscf1,-3,3);
			int Aclass1 = AclassVal[dscf1];


			int dscf2 = m_scale_factors[channel][j][1] - m_scale_factors[channel][j][2];
			dscf2 = 3 + CLAMP(dscf2,-3,3);
			int Aclass2 = AclassVal[dscf2];

			// Look up the scale code based on ISO 11172 3-ANNEX C, Table 3-C.4
			m_scale_code[channel][j] = AcodeVal[Aclass1*5 + Aclass2];
			int scalePattern = ApatternVal[Aclass1*5 + Aclass2];


			// Adjust the scale factors based on the pattern of scale codes that will be sent
			// These are from ISO 11172 TABLE 3-C.4:  LAYER II Scalefactor transmission patterns
			// Either 1,2 or 3 scale factors will be sent
			switch(scalePattern) {
				case SEND122 :	m_scale_factors[channel][j][2] = m_scale_factors[channel][j][1];
								break;

				case SEND133 :  m_scale_factors[channel][j][1] = m_scale_factors[channel][j][2];
								break;

				case SEND113:	m_scale_factors[channel][j][1] = m_scale_factors[channel][j][0];
								break;

				case SEND111:	m_scale_factors[channel][j][1] = m_scale_factors[channel][j][0];
								m_scale_factors[channel][j][2] = m_scale_factors[channel][j][0];
								break;

				case SENDMAX:  // This patterns finds the max code and sends only it
								if(m_scale_factors[channel][j][2] < m_scale_factors[channel][j][1]) m_scale_factors[channel][j][1] = m_scale_factors[channel][j][2];
								if(m_scale_factors[channel][j][1] < m_scale_factors[channel][j][0]) m_scale_factors[channel][j][0] = m_scale_factors[channel][j][1];
								m_scale_factors[channel][j][1] = m_scale_factors[channel][j][0];
								m_scale_factors[channel][j][2] = m_scale_factors[channel][j][0];
								break;

				case SEND333:	m_scale_factors[channel][j][0] = m_scale_factors[channel][j][2];
								m_scale_factors[channel][j][1] = m_scale_factors[channel][j][2];
								break;

				case SEND222:	m_scale_factors[channel][j][0] = m_scale_factors[channel][j][1];
								m_scale_factors[channel][j][2] = m_scale_factors[channel][j][1];
								break;


				default : break;  // Will send all 3 codes so make no adjustments

			}

		}  // subband

	} // channel


}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Compute the number of bits that will be in an encoded audio frame
// This does not include pes header information, only the audio frame size
//
//  Input parameters : none
//
//	Output : number of bits that will be in an encoded frame
//
//
unsigned int CAVAudioCompression::GetEncodedFrameSize(void)
{
	assert(m_audioSampFreq);

	unsigned int extraByte = CheckAudioPacketSize();
	unsigned int audioEncodedFrameSize = SLOTSIZE*extraByte + (SLOTSIZE * (int) (m_audioEncodedBitrate * (float) MPEGAUDIOFRAMEWIDTH/((float) m_audioSampFreq*SLOTSIZE)));  

    return(audioEncodedFrameSize);
}



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
void CAVAudioCompression::AllocateSubbandBits(void)
{



	// Initialize the allocation variables
	int tempSMR[2][MPEGAUDIOSBNUM];
	int minSMR = AudioFixedSMR[0];
	uint8 subbandStatus[2][MPEGAUDIOSBNUM];

	for(unsigned int channel = 0;channel < 2;++channel)
	{
		for(unsigned int i=0;i<MPEGAUDIOSBNUM;++i) 
		{
			subbandStatus[channel][i] = AUDIO_SB_EMPTY;
			m_bit_alloc[channel][i] = 0;
			tempSMR[channel][i] = AudioFixedSMR[i];
			if(minSMR > AudioFixedSMR[i]) minSMR = AudioFixedSMR[i];
		}

	}


	// Get the number of bits for the frame
	// During bit allocation, this number cannot be exceeded
	unsigned int audioEncodedFrameSize = GetEncodedFrameSize();

	// Initialize the running frame size to the header + bit alloc size since these
	// are fixed sizes and do not depend on the allocation. 
	unsigned int currentFrameSize;
	if(m_audioSampFreq == SAMPLINGFREQ48000) currentFrameSize = AUDIOPACKOVERHEAD1;
	else currentFrameSize = AUDIOPACKOVERHEAD2;


	// Keep allocating bits until none are left to allocate or all subbands have been maxed out
	while(1) 
	{

		// find the sub-band with the largest signal to mask ratio 
		int maxSB = -1;
		int maxCH = -1;
		int maxSMR = minSMR-1;
		for(unsigned int channel = 0;channel < 2;++channel) 
		{
			for(unsigned int i=0;i < m_subBandLimit;++i) 
			{
				if (subbandStatus[channel][i] != AUDIO_SB_FINISHED && tempSMR[channel][i] > maxSMR) 
				{
					maxSMR = tempSMR[channel][i];
					maxSB = i;
					maxCH = channel;
				}
			}
		}
		// If all sub bands are finished then exit
		if (maxSB < 0) break;


		// Point to the row of the bit allocation table for this sub band
		// ISO 11172-3 tables 3-B.2
		const unsigned char *pallocRow = &AudioBitAllocTable[BITALLOCTABLEWIDTH*maxSB];


		// Calculate the number of added bits that will be needed if the bit allocation for this sub band is increased
		unsigned int addedBits;
		int columnIndex = m_bit_alloc[maxCH][maxSB];
		if (subbandStatus[maxCH][maxSB] == AUDIO_SB_EMPTY) 
		{
			// nothing has been coded for this band yet
			// initialize this band with the minimum bit count it can have
			addedBits = SCALECODEBITWIDTH + AudioTotalQuantBits[pallocRow[columnIndex]];
			switch(m_scale_code[maxCH][maxSB]) {
						case SEND3:		// sending 3 scalefactors
										addedBits += 3 * SCALEFACTORBITWIDTH;
										break;

						case SEND2A:	// Sending 2 scale factors
						case SEND2B:	addedBits += 2 * SCALEFACTORBITWIDTH;
										break;

						case SEND1:		// Sending 1 scale factor 
										addedBits += 1 * SCALEFACTORBITWIDTH;
										break;
			}

		} 
		else 
		{
			// Find the added bits for increasing the bit allocation by 1 in this subband
			addedBits = AudioTotalQuantBits[pallocRow[columnIndex]] - AudioTotalQuantBits[pallocRow[columnIndex-1]];
		}



		// See if it is possible to increase the resolution for this band
		if (currentFrameSize + addedBits <= audioEncodedFrameSize) 
		{

			// Yes we can so increment the bit allocation for this band and channel and update the number of bits in the frame
			m_bit_alloc[maxCH][maxSB] += 1;
			currentFrameSize += addedBits;
			++columnIndex;

			// Calculate a new signal to mask ratio for this band based on the new sub band resolution
			tempSMR[maxCH][maxSB] = AudioFixedSMR[maxSB] - AudioQuantSNR[pallocRow[columnIndex-1]];


			// Check to see if we have allocated all of the bits possible for this subband 
			if (columnIndex == MaxBitsSubband[maxSB]) subbandStatus[maxCH][maxSB] = AUDIO_SB_FINISHED;
			else subbandStatus[maxCH][maxSB] = AUDIO_SB_ALLOCATED;

		} 
		else 
		{

			// Not enough room to increase this sub-band so do not add any bits to it
			// and mark it as finished so we will not check it again
			subbandStatus[maxCH][maxSB] = AUDIO_SB_FINISHED;

		}

	}


	// Save the number of fill bits needed to fill out the audio frame
	m_audioFrameFillBits = audioEncodedFrameSize - currentFrameSize;


}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Send an audio pes header with timing
// See ISO 13818-1 sec. 2.4.3.6 for field definitions
//
//  Input parameters : none
//
//	Output : none
//
//
void CAVAudioCompression::SendAudioPesHdr(void)
{

		// Add a pes packet header
		// See ISO 13818-1 sec. 2.4.3.6 for field definitions

		// Convert the time to the proper format
		int64 pcrBase = ((int64) (m_packetTime/300.0f));

		m_audioSerialBuffer.PutBit(1,24);
		m_audioSerialBuffer.PutBit(m_streamID,8);
		m_audioSerialBuffer.PutBit(0,16);  // PES packet length
		if(m_containerFormat == MPEG1_STYLE)
		{
			m_audioSerialBuffer.PutBit(1,2);    // padding bits
			m_audioSerialBuffer.PutBit(1,1);    // P-STD buffer scale
			m_audioSerialBuffer.PutBit(1024,13);    // P-STD buffer size

		}
		if(m_containerFormat == MPEG2_STYLE)
		{
			m_audioSerialBuffer.PutBit(2,2);    // padding bits
			m_audioSerialBuffer.PutBit(0,2);    // PES_scrambling_control
			m_audioSerialBuffer.PutBit(0,1);    // PES_priority
			m_audioSerialBuffer.PutBit(1,1);    // data_alignment_indicator
			m_audioSerialBuffer.PutBit(0,1);    // copyright
			m_audioSerialBuffer.PutBit(1,1);    // original_or_copy
			m_audioSerialBuffer.PutBit(2,2);    // PTS_DTS_flags
			m_audioSerialBuffer.PutBit(0,1);    // ESCR_flag
			m_audioSerialBuffer.PutBit(0,1);    // ES_rate_flag
			m_audioSerialBuffer.PutBit(0,1);    // DSM_trick_mode_flag
			m_audioSerialBuffer.PutBit(0,1);    // additional_copy_info_flag
			m_audioSerialBuffer.PutBit(0,1);    // PES_CRC_flag
			m_audioSerialBuffer.PutBit(0,1);    // PES_extension_flag
			m_audioSerialBuffer.PutBit(5,8);    // PES_header_data_length
		}
		m_audioSerialBuffer.PutBit(2,4);    
		m_audioSerialBuffer.PutBit((INT)((pcrBase >> 30) & 0x07),3);
		m_audioSerialBuffer.PutBit(1, 1);  // marker bit
		m_audioSerialBuffer.PutBit((INT)((pcrBase >> 15) & 0x7fff),15);
		m_audioSerialBuffer.PutBit(1, 1);   // marker bit
		m_audioSerialBuffer.PutBit((INT)(pcrBase & 0x7fff),15);
		m_audioSerialBuffer.PutBit(1, 1);   // marker bit


}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Encode the audio frame into a serial line buffer
// See ISO 11172-3 section 2.4.1 for the meaning of the various bit fields
//
//  Input parameters : none
//
//	Output : Returns the number of bytes that were encoded
//
//
void CAVAudioCompression::SerializeAudioFrame(void)
{


	// Check whether an extra byte is being sent
	int extra_byte = CheckAudioPacketSize();

	// Determine the sampling code rate to send
	// per ISO 11172-3 sec. 2.4.2.3
	int samplingratecode;
	switch(m_audioSampFreq)
	{
	case SAMPLINGFREQ44100 :	samplingratecode = 0;
		break;
	case SAMPLINGFREQ48000 :	samplingratecode = 1;
		break;
	default :					samplingratecode = 2;
		break;
	}


	unsigned int startLoc;
	if(m_containerFormat == AVI_STYLE)
	{
		// Insert an AVI header
		m_audioSerialBuffer.PutText("01wb"); 
		startLoc = m_audioSerialBuffer.GetBytes();
		m_audioSerialBuffer.PutLe32(0);
	}


	// Insert an audio header before the actual audio data 
	// See ISO 11172-3 section 2.4.2.3 for the meaning of the various bit fields
	m_audioSerialBuffer.PutBit(AUDIOSYNCWORD,12);
	m_audioSerialBuffer.PutBit(1,1);  // 1 = mpeg1 ID
	m_audioSerialBuffer.PutBit(2,2);  // layer 2 
	m_audioSerialBuffer.PutBit(1,1);  // no error protection 
	m_audioSerialBuffer.PutBit(14,4); // bitrate = 384000
	m_audioSerialBuffer.PutBit(samplingratecode,2);
	m_audioSerialBuffer.PutBit(extra_byte,1);  // use padding 
	m_audioSerialBuffer.PutBit(0,1);  // private_bit 
	m_audioSerialBuffer.PutBit(0,2);  // Stereo
	m_audioSerialBuffer.PutBit(0,2);  // mode_ext 
	m_audioSerialBuffer.PutBit(0,1);  // no copyright 
	m_audioSerialBuffer.PutBit(1,1);  // original 
	m_audioSerialBuffer.PutBit(0,2);  // no emphasis 



		// Send the number of bits allocated for each sub band in both channels
		for(unsigned int i=0;i < m_subBandLimit;++i) 
		{
			for(unsigned int channel=0;channel < 2;++channel) 
			{
				m_audioSerialBuffer.PutBit(m_bit_alloc[channel][i],EncodedBitsSubband[i]);
			}
		}

		// Send the scale codes for each sub band in both channels
		for(unsigned int i=0;i < m_subBandLimit;++i) 
		{
			for(unsigned int channel=0;channel < 2;++channel) 
			{
				if (m_bit_alloc[channel][i])  m_audioSerialBuffer.PutBit(m_scale_code[channel][i],SCALECODEBITWIDTH);
			}
		}


		// Send the scale factors for each sub band in both channels
		for(unsigned int i=0;i < m_subBandLimit;++i) 
		{
			for(unsigned int channel=0;channel < 2; ++channel)
			{

				if (m_bit_alloc[channel][i]) // Only send scale factors for a subband if bits were allocated to it
				{
					if(m_scale_code[channel][i] == SEND3) 
					{
						// Need to send all 3 codes
						m_audioSerialBuffer.PutBit(m_scale_factors[channel][i][0],SCALEFACTORBITWIDTH);
						m_audioSerialBuffer.PutBit(m_scale_factors[channel][i][1],SCALEFACTORBITWIDTH);
						m_audioSerialBuffer.PutBit(m_scale_factors[channel][i][2],SCALEFACTORBITWIDTH);
					}
					if(m_scale_code[channel][i] == SEND2A || m_scale_code[channel][i] == SEND2B) 
					{
						// Need to send only 2 of the codes
						m_audioSerialBuffer.PutBit(m_scale_factors[channel][i][0],SCALEFACTORBITWIDTH);
						m_audioSerialBuffer.PutBit(m_scale_factors[channel][i][2],SCALEFACTORBITWIDTH);
					}
					if(m_scale_code[channel][i] == SEND1) 
					{
						// Need to send only 1 of the codes
						m_audioSerialBuffer.PutBit(m_scale_factors[channel][i][0],SCALEFACTORBITWIDTH);
					}

				}

			}

		}

		// quantize and output the actual samples 
		// See ISO 11172-3 3-C.1.5.2 paragraph 8 for details on quantization
		for(unsigned int k=0;k < MPEGAUDIOSBGROUPS;++k) 
		{
			for(unsigned int l=0;l < MPEGAUDIOSUBGROUPSIZE;++l) 
			{
				int subGrooupIndex = l*MPEGAUDIOSBGROUPS;
				for(unsigned int i=0;i < m_subBandLimit;++i) 
				{

					for(unsigned int channel=0;channel < 2;++channel)
					{

						// If this sub band has bits allocated then output the samples
						// 3 consecutive samples are encoded as a group
						// They may be sent as a single combined sample or 3 separate samples
						int columnIndex = m_bit_alloc[channel][i];

						if (columnIndex) 
						{
							int qindex = AudioBitAllocTable[i*BITALLOCTABLEWIDTH+columnIndex-1];
							int steps = AudioQuantSteps[qindex];
							float stepScale = steps * 0.5f;
							int bits = AudioQuantBits[qindex];
							if(bits < 0)
							{

								// Combine 3 values in a row to send as one value
								float scfa = AudioQuantInverseMult[m_scale_factors[channel][i][k]];

								int tmp = (int)((m_sample_band_ch[channel][k][subGrooupIndex + 2][i] * scfa + 1.0f) * stepScale);
								if (tmp >= steps) tmp = steps - 1;
								int oneval = tmp*steps;

								tmp = (int)((m_sample_band_ch[channel][k][subGrooupIndex + 1][i] * scfa + 1.0f) * stepScale);
								if (tmp >= steps) tmp = steps - 1;
								oneval = (oneval + tmp)*steps;

								tmp = (int)((m_sample_band_ch[channel][k][subGrooupIndex][i] * scfa + 1.0f) * stepScale);
								if (tmp >= steps) tmp = steps - 1;
								oneval += tmp;

								m_audioSerialBuffer.PutBit(oneval,-bits);

							} 
							else 
							{
								// Just send the 3 values separately
								float scfa = AudioQuantInverseMult[m_scale_factors[channel][i][k]];

								int tmp = (int)((m_sample_band_ch[channel][k][subGrooupIndex][i] * scfa + 1.0f) * stepScale);
								if (tmp >= steps) tmp = steps - 1;
								m_audioSerialBuffer.PutBit(tmp,bits);

								tmp = (int)((m_sample_band_ch[channel][k][subGrooupIndex + 1][i] * scfa + 1.0f) * stepScale);
								if (tmp >= steps) tmp = steps - 1;
								m_audioSerialBuffer.PutBit(tmp,bits);

								tmp = (int)((m_sample_band_ch[channel][k][subGrooupIndex + 2][i] * scfa + 1.0f) * stepScale);
								if (tmp >= steps) tmp = steps - 1;
								m_audioSerialBuffer.PutBit(tmp,bits);

							}

						}

					} // Channel

				} // subbands

			} // subgroup

		} // subband groups


		// fill any remaining space with 0 
		for(int i=0;i < m_audioFrameFillBits;i++) m_audioSerialBuffer.PutBit(0,1);


		if(m_containerFormat == AVI_STYLE)
		{
			// Insert the proper header size
			uint8 *pTmp = m_audioSerialBuffer.GetBufferAddress(); 
			unsigned int endLoc = m_audioSerialBuffer.GetBytes();
			unsigned int dataSize = endLoc - startLoc - 4;  // Don't include the 4 byte size
			pTmp[startLoc] = (uint8) (dataSize & 0xFF);
			pTmp[startLoc+1] = (uint8) ((dataSize >> 8) & 0xFF);
			pTmp[startLoc+2] = (uint8) ((dataSize >> 16) & 0xFF);
			pTmp[startLoc+3] = (uint8) ((dataSize >> 24) & 0xFF);
		}
}


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
unsigned int CAVAudioCompression::CheckAudioPacketSize(void)
{

	assert(m_audioSampFreq);

	float tmp = m_audioEncodedBitrate * (float) MPEGAUDIOFRAMEWIDTH/((float) m_audioSampFreq*SLOTSIZE);
	float fractional_part = tmp - (float) ((uint64) tmp);

	m_audioRateFracByte += fractional_part;
	unsigned int extra_byte = 0;
	if(m_audioRateFracByte > 1.0f) 
	{
		extra_byte = 1;
		m_audioRateFracByte -= 1.0f;
	} 

	return(extra_byte);


}


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
void CAVAudioCompression::InverseDCT32(float *pDataIn, int *pDataOut)
{

	assert(pDataIn);
	assert(pDataOut);


	float data_t0[32];

	// Do the DCT butterflys (intentionally unrolled)
	data_t0[31] = (pDataIn[31] + pDataIn[29]);
	data_t0[29] = (pDataIn[29] + pDataIn[27]);
	data_t0[27] = (pDataIn[27] + pDataIn[25]);
	data_t0[25] = (pDataIn[25] + pDataIn[23]);
	data_t0[23] = (pDataIn[23] + pDataIn[21]);
	data_t0[21] = (pDataIn[21] + pDataIn[19]);
	data_t0[19] = (pDataIn[19] + pDataIn[17]);
	data_t0[17] = (pDataIn[17] + pDataIn[15]);
	data_t0[15] = (pDataIn[15] + pDataIn[13]);
	data_t0[13] = (pDataIn[13] + pDataIn[11]);
	data_t0[11] = (pDataIn[11] + pDataIn[9]);
	data_t0[9] =  (pDataIn[9] + pDataIn[7]);
	data_t0[7] =  (pDataIn[7] + pDataIn[5]);
	data_t0[5] =  (pDataIn[5] + pDataIn[3]);
	data_t0[3] =  (pDataIn[3] + pDataIn[1]);

	data_t0[30] = pDataIn[30];
	data_t0[28] = pDataIn[28];
	data_t0[26] = pDataIn[26];
	data_t0[24] = pDataIn[24];
	data_t0[22] = pDataIn[22];
	data_t0[20] = pDataIn[20];
	data_t0[18] = pDataIn[18];
	data_t0[16] = pDataIn[16];
	data_t0[14] = pDataIn[14];
	data_t0[12] = pDataIn[12];
	data_t0[10] = pDataIn[10];
	data_t0[8] = pDataIn[8];
	data_t0[6] = pDataIn[6];
	data_t0[4] = pDataIn[4];
	data_t0[2] = pDataIn[2];
	data_t0[1] = pDataIn[1];
	data_t0[0] = pDataIn[0];


	data_t0[31] += data_t0[27];
	data_t0[30] += data_t0[26];
	data_t0[27] += data_t0[23];
	data_t0[26] += data_t0[22];
	data_t0[23] += data_t0[19];
	data_t0[22] += data_t0[18];
	data_t0[19] += data_t0[15];
	data_t0[18] += data_t0[14];
	data_t0[15] += data_t0[11];
	data_t0[14] += data_t0[10];
	data_t0[11] += data_t0[7];
	data_t0[10] += data_t0[6];
	data_t0[7]  += data_t0[3];
	data_t0[6]  += data_t0[2];

	data_t0[31] += data_t0[23];
	data_t0[30] += data_t0[22];
	data_t0[29] += data_t0[21];
	data_t0[28] += data_t0[20];

	data_t0[23] += data_t0[15];
	data_t0[22] += data_t0[14];
	data_t0[21] += data_t0[13];
	data_t0[20] += data_t0[12];

	data_t0[15] += data_t0[7];
	data_t0[14] += data_t0[6];
	data_t0[13] += data_t0[5];
	data_t0[12] += data_t0[4];

	data_t0[3]  = -data_t0[3];
	data_t0[6]  = -data_t0[6];
	data_t0[11] = -data_t0[11];
	data_t0[12] = -data_t0[12];
	data_t0[13] = -data_t0[13];
	data_t0[15] = -data_t0[15];


	data_t0[19] = -data_t0[19];
	data_t0[22] = -data_t0[22];
	data_t0[27] = -data_t0[27];
	data_t0[28] = -data_t0[28];
	data_t0[29] = -data_t0[29];
	data_t0[31] = -data_t0[31];


	float tmp0,tmp1,tmp2,tmp3,tmp4;
	for(unsigned int i=0;i<8;++i) 
	{
		tmp0 = (data_t0[24+i] + data_t0[8+i]) * -AUDIOINVSQRT2;
		tmp1 = (data_t0[8+i] - tmp0) * C0_12;
		tmp2 = (data_t0[8+i] + tmp0) * C0_4;

		tmp3 = data_t0[0+i] - (data_t0[16+i] * AUDIOINVSQRT2);
		tmp4 = data_t0[0+i] + (data_t0[16+i] * AUDIOINVSQRT2);


		data_t0[ 0+i] = tmp4 + tmp1;
		data_t0[24+i] = tmp4 - tmp1;
		data_t0[ 8+i] = tmp3 - tmp2;
		data_t0[16+i] = tmp3 + tmp2;
	}


	for(unsigned int i=0;i<4;++i) 
	{
		tmp0 = data_t0[0+i] - (data_t0[28+i] * C1_28);
		tmp1 = data_t0[0+i] + (data_t0[28+i] * C1_28);
		data_t0[28+i] = tmp0;
		data_t0[0+i] = tmp1;

		tmp0 = data_t0[24+i] - (data_t0[4+i] * C1_4);
		tmp1 = data_t0[24+i] + (data_t0[4+i] * C1_4);
		data_t0[ 4+i] = tmp0;
		data_t0[24+i] = tmp1;

		tmp0 = data_t0[8+i] - (data_t0[20+i] * C1_20);
		tmp1 = data_t0[8+i] + (data_t0[20+i] * C1_20);
		data_t0[20+i] = tmp0;
		data_t0[ 8+i] = tmp1;

		tmp0 = data_t0[16+i] - (data_t0[12+i] * C1_12);
		tmp1 = data_t0[16+i] + (data_t0[12+i] * C1_12);
		data_t0[12+i] = tmp0;
		data_t0[16+i] = tmp1;
	}


	tmp0 = data_t0[30] * C2_30;
	data_t0[30] = data_t0[0] - tmp0;
	data_t0[   0] = data_t0[0] + tmp0;

	tmp0 = data_t0[ 2] * C2_2;
	data_t0[ 2] = data_t0[28] - tmp0;
	data_t0[28] = data_t0[28] + tmp0;

	tmp0 = data_t0[31] * C2_30;
	data_t0[31] = data_t0[1] - tmp0;
	data_t0[ 1] = data_t0[1] + tmp0;

	tmp0 = data_t0[ 3] * C2_2;
	data_t0[ 3] = data_t0[29] - tmp0;
	data_t0[29] = data_t0[29] + tmp0;


	tmp0 = data_t0[26] * C2_26;
	data_t0[26] = data_t0[4] - tmp0;
	data_t0[   4] = data_t0[4] + tmp0;

	tmp0 = data_t0[ 6] * C2_6;
	data_t0[ 6] = data_t0[24] - tmp0;
	data_t0[24] = data_t0[24] + tmp0;

	tmp0 = data_t0[27] * C2_26;
	data_t0[27] = data_t0[5] - tmp0;
	data_t0[ 5] = data_t0[5] + tmp0;

	tmp0 = data_t0[ 7] * C2_6;
	data_t0[ 7] = data_t0[25] - tmp0;
	data_t0[25] = data_t0[25] + tmp0;


	tmp0 = data_t0[22] * C2_22;
	data_t0[22] = data_t0[8] - tmp0;
	data_t0[   8] = data_t0[8] + tmp0;

	tmp0 = data_t0[ 10] * C2_10;
	data_t0[ 10] = data_t0[20] - tmp0;
	data_t0[20] = data_t0[20] + tmp0;

	tmp0 = data_t0[23] * C2_22;
	data_t0[23] = data_t0[9] - tmp0;
	data_t0[ 9] = data_t0[9] + tmp0;

	tmp0 = data_t0[ 11] * C2_10;
	data_t0[ 11] = data_t0[21] - tmp0;
	data_t0[21] = data_t0[21] + tmp0;

	tmp0 = data_t0[18] * C2_18;
	data_t0[18] = data_t0[12] - tmp0;
	data_t0[   12] = data_t0[12] + tmp0;

	tmp0 = data_t0[ 14] * C2_14;
	data_t0[ 14] = data_t0[16] - tmp0;
	data_t0[16] = data_t0[16] + tmp0;

	tmp0 = data_t0[19] * C2_18;
	data_t0[19] = data_t0[13] - tmp0;
	data_t0[ 13] = data_t0[13] + tmp0;

	tmp0 = data_t0[ 15] * C2_14;
	data_t0[ 15] = data_t0[17] - tmp0;
	data_t0[17] = data_t0[17] + tmp0;



	tmp0 = data_t0[1] * C3_30;
	data_t0[1] = data_t0[30] - tmp0;
	data_t0[30] = data_t0[30] + tmp0;

	tmp0 = data_t0[3] * C3_28;
	data_t0[3] = data_t0[28] - tmp0;
	data_t0[28] = data_t0[28] + tmp0;

	tmp0 = data_t0[5] * C3_26;
	data_t0[5] = data_t0[26] - tmp0;
	data_t0[26] = data_t0[26] + tmp0;

	tmp0 = data_t0[7] * C3_24;
	data_t0[7] = data_t0[24] - tmp0;
	data_t0[24] = data_t0[24] + tmp0;

	tmp0 = data_t0[9] * C3_22;
	data_t0[9] = data_t0[22] - tmp0;
	data_t0[22] = data_t0[22] + tmp0;

	tmp0 = data_t0[11] * C3_20;
	data_t0[11] = data_t0[20] - tmp0;
	data_t0[20] = data_t0[20] + tmp0;

	tmp0 = data_t0[13] * C3_18;
	data_t0[13] = data_t0[18] - tmp0;
	data_t0[18] = data_t0[18] + tmp0;

	tmp0 = data_t0[15] * C3_16;
	data_t0[15] = data_t0[16] - tmp0;
	data_t0[16] = data_t0[16] + tmp0;

	tmp0 = data_t0[17] * C3_14;
	data_t0[17] = data_t0[14] - tmp0;
	data_t0[14] = data_t0[14] + tmp0;

	tmp0 = data_t0[19] * C3_12;
	data_t0[19] = data_t0[12] - tmp0;
	data_t0[12] = data_t0[12] + tmp0;

	tmp0 = data_t0[21] * C3_10;
	data_t0[21] = data_t0[10] - tmp0;
	data_t0[10] = data_t0[10] + tmp0;

	tmp0 = data_t0[23] * C3_8;
	data_t0[23] = data_t0[8] - tmp0;
	data_t0[8] = data_t0[8] + tmp0;

	tmp0 = data_t0[25] * C3_6;
	data_t0[25] = data_t0[6] - tmp0;
	data_t0[6] = data_t0[6] + tmp0;

	tmp0 = data_t0[27] * C3_4;
	data_t0[27] = data_t0[4] - tmp0;
	data_t0[4] = data_t0[4] + tmp0;

	tmp0 = data_t0[29] * C3_2;
	data_t0[29] = data_t0[2] - tmp0;
	data_t0[2] = data_t0[2] + tmp0;

	tmp0 = data_t0[31] * C3_0;
	data_t0[31] = data_t0[0] - tmp0;
	data_t0[0] = data_t0[0] + tmp0;


	// Rearrange the output back to correct order and round
	pDataOut[0] = (int) (data_t0[0] + 0.5f);
	pDataOut[1] = (int) (data_t0[16] + 0.5f);
	pDataOut[2] = (int) (data_t0[8] + 0.5f);
	pDataOut[3] = (int) (data_t0[24] + 0.5f);
	pDataOut[4] = (int) (data_t0[4] + 0.5f);
	pDataOut[5] = (int) (data_t0[20] + 0.5f);
	pDataOut[6] = (int) (data_t0[12] + 0.5f);
	pDataOut[7] = (int) (data_t0[28] + 0.5f);
	pDataOut[8] = (int) (data_t0[2] + 0.5f);
	pDataOut[9] = (int) (data_t0[18] + 0.5f);
	pDataOut[10] = (int) (data_t0[10] + 0.5f);
	pDataOut[11] = (int) (data_t0[26] + 0.5f);
	pDataOut[12] = (int) (data_t0[6] + 0.5f);
	pDataOut[13] = (int) (data_t0[22] + 0.5f);
	pDataOut[14] = (int) (data_t0[14] + 0.5f);
	pDataOut[15] = (int) (data_t0[30] + 0.5f);
	pDataOut[16] = (int) (data_t0[1] + 0.5f);
	pDataOut[17] = (int) (data_t0[17] + 0.5f);
	pDataOut[18] = (int) (data_t0[9] + 0.5f);
	pDataOut[19] = (int) (data_t0[25] + 0.5f);
	pDataOut[20] = (int) (data_t0[5] + 0.5f);
	pDataOut[21] = (int) (data_t0[21] + 0.5f);
	pDataOut[22] = (int) (data_t0[13] + 0.5f);
	pDataOut[23] = (int) (data_t0[29] + 0.5f);
	pDataOut[24] = (int) (data_t0[3] + 0.5f);
	pDataOut[25] = (int) (data_t0[19] + 0.5f);
	pDataOut[26] = (int) (data_t0[11] + 0.5f);
	pDataOut[27] = (int) (data_t0[27] + 0.5f);
	pDataOut[28] = (int) (data_t0[7] + 0.5f);
	pDataOut[29] = (int) (data_t0[23] + 0.5f);
	pDataOut[30] = (int) (data_t0[15] + 0.5f);
	pDataOut[31] = (int) (data_t0[31] + 0.5f);

}


