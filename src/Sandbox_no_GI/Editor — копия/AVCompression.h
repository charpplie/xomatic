////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Header File.
//  Copyright (C), Crytek Studios, 2010.
// -------------------------------------------------------------------------
//  File name:   AV_Compression.h
//  Version:     v1.00
//  Created:     09/04/2010 by John Stewart.
//  Compilers:   
//  Description: Audio/Video Compression  class implementation.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __AVCompression_h__
#define __AVCompression_h__
#pragma once

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// needed headers
#include <comdef.h>
#include <memory.h>
#include <tchar.h>
#include <string.h>

#include <IRenderer.h>

#include "AVCompressionUtil.h"
#include "AVCompressionAudio.h"
#include "AVCompressionVideo.h"

#include "IInput.h"

// Error codes
#define AVCOMP_OK 0
#define AVCOMP_FILEOPEN AVCOMP_FILEOPEN_UTIL
#define AVCOMP_BUFFERWRITE AVCOMP_BUFFERWRITE_UTIL
#define AVCOMP_BADPARM AVCOMP_BADPARM_UTIL
#define AVCOMP_FILEWRITE AVCOMP_FILEWRITE_UTIL
#define AVCOMP_MEM 5
#define AVCOMP_AUDIO_UNSUPPORTED 6
#define AVCOMP_THREADERROR 7
#define AVCOMP_ALREADYINITIALIZED 8
#define AVCOMP_FRAMECAPTUREINIT 9
#define AVCOMP_AUDIOCREATE 10
#define AVCOMP_VIDEOCREATE 11
#define AVCOMP_TRANSCODE 12
#define AVCOMP_DECODE 13


// Defined states for the encoding thread
#define AVTHREAD_ERROR -1
#define AVTHREAD_IDLE 0
#define AVTHREAD_RECORDINGIDLE 1
#define AVTHREAD_RECORDINGBUSY 2
#define AVTHREAD_TRANSCODINGFAST 3
#define AVTHREAD_TRANSCODINGFULL 4
#define AVTHREAD_TRANSCODINGNULL 5
#define AVTHREAD_PAUSED 6
#define AVTHREAD_ABORT 7
#define AVTHREAD_SHUTDOWN 8
#define AVTHREAD_PAUSE 9
#define AVTHREAD_UNPAUSE 10


// MPEG Header start codes
#define PICTURE_START_CODE		0x100
#define SEQ_START_CODE			0x1B3
#define EXT_START_CODE			0x1B5
#define SEQ_END_CODE			0x1B7
#define GOP_START_CODE			0x1B8
#define PACK_START_CODE			0x1BA
#define PACK_HDR_START_CODE		0x1BB
#define MPEG_PROG_END_CODE		0x1B9
#define VIDEO_PES_ID			0x0E1
#define AUDIO_PES_ID			0x0C1
#define SEQ_ID					0x001
#define DISP_ID					0x002
#define CODING_ID				0x008

// Rate control definitions
#define RATECONTROLFILTERCOEF 0.03f
#define MINQUANTLIMIT  1.0f
#define MAXQUANTLIMIT 30.0f
#define MQUANTADJUSTRATE 0.1f

// Maximum number of threads to use for video compression routines
#define MAX_COMP_THREADS  32  // must be less than  MAXIMUM_WAIT_OBJECTS for windows

// Length of the group of pictures for transcoding
#define GOPLENGTH 12  // Length of the GOP

#define MINCOMPDIM 32  // minimum width and height to compress
#define MAXCOMPDIM 4096  // maximum width and height to compress

#define MACROBLOCKDIM 16  // macroblocks are 16x16
#define RGBASIZE 4  // number of bytes in RGBA pixel

#define MAXREPEATLIMIT 32  // maximum number of times a frame will be repeated

#define AUDIOCAPTBUFFSIZE 32*1024  // Size of storage for audio callback function 

#define VIDEOCLOCKRATE 27000000.0f  // Reference clock freq in Hz

// Intra DC resolutions
#define INTRADCPRED11BIT  3
#define INTRADCPRED10BIT  2
#define INTRADCPRED9BIT  1
#define INTRADCPRED8BIT  0

// value limits for coeffs
#define QUANTMAXMPEG2 2047
#define QUANTMAXMPEG1 255


#define DEFAULTAUDIORATE 48000
#define ISAUDIO			0
#define ISVIDEO			1

#define TRANSCODINGFACTOR	4.0f  // factor to use when reducing bitrate during transcoding

// Size of a serial buffer used to hold headers
#define HDRBUFFSIZE 1024

// Decoder definitions
#define EOB 0xFF

// File header definitions
#define FILEFRAMESYNC 0x7F0F
#define FILEHDRSYNC 0x30F1
#define MAXCRYHDRSIZE 280
#define PESHDROFFSET 6

// AVI file constants
#define MOVIHDROFFSET 4
#define AVI_KEYFRAME 16
#define AVI_HASINDEX        0x00000010
#define AVI_ISINTERLEAVED   0x00000100
#define AVI_TRUSTCKTYPE     0x00000800 
#define AVI_WASCAPTUREFILE  0x00010000
#define AVI_COPYRIGHTED     0x00020000
#define AVIJUNKSIZE			120


// This structure holds parameters that are used 
// to initialize the compression encoder. They are typically
// set by the user
struct AVCRecordParameters
{
	CString m_sFile;		
	unsigned int inputWidth;
	unsigned int inputHeight;
	unsigned int displayWidth;
	unsigned int displayHeight;
	int recordSize;
	unsigned int customWidth;
	unsigned int customHeight;
	unsigned int recordQuality;
	float frameRate;
	int recordMode;
	unsigned int recordTime;
	int maxFileSize;
	bool parmsVaild;
};

// Allowable MPEG frame rates
const float frameratetable[8] = {23.976023976f,24.0f,25.0f,29.97002997f,30.0f,50.0f,59.94005994f,60.0f};


// Defines the Audio/Video compression class
class CAVCompression  : ICaptureFrameListener, public CrySimpleThread<>
{
public:

	// Constructor and Destructor
	CAVCompression();
	~CAVCompression();

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Shuts down the the recording thread.
	//
	//  Input parameters : None
	//
	//	Output :
	//		None
	//
	void Shutdown();

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Initialize the compression engine, this is called when a recording is started
	//
	//  Input parameters : None
	//
	//	Output :
	//		Outputs an AVCOMP error code as defined in header file
	//
	HRESULT InitEngine(void);


	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Initialize the compression variables, this is called before recording is started to set the recording parameters
	//
	//  Input parameters :
	//		pParameterSet	: pointer to a parameter set
	//
	//	Output : None
	//
	void SetParameters(AVCRecordParameters *pParameterSet);


	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Checks if the compression parameters have been set
	//
	//  Input parameters : none
	//
	//	Output : returns true if the parameters have been set, false otherwise
	//
	bool AreParametersValid(void) {return(m_InitParams.parmsVaild);}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Release the compression engine, this is called when the recording is stopped
	// The compressionMode indicates what to do with the recorded data
	// Options are to throw it away, copy it to an AVI file directly, or to re-encode it
	//
	//  Input  : 
	//			compressionMode  -- indicates the action to take
	///
	//	Output : none
	//
	void StopRecording(int compressionMode);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// If a transcode is occuring, it aborts it
	//
	//  Input  : none
	///
	//	Output : none
	//
	void AbortTranscode(void);

	////////////////////////////////////////////////////////////////////////////////////////
	// Get the current status of the AV Compression engine
	//
	//  Input parameters : None
	//
	//	Output : Return true if is initialized and ready to record (or recording), returns false otherwise
	//
	bool IsValid(void) {return(m_validAVCContext ? true : false);}

	/////////////////////////////////////////////////////////////////////////////////////////////
	// Change the state of the pause flag to the opposite of its current state
	//
	//  Input  : None
	//
	//	Output : None
	//
	void TogglePause(void);

	/////////////////////////////////////////////////////////////////////////////////////////////
	// Checks the pause state of the recording
	//
	//  Input  : None
	//
	//	Output : returns true if recording is paused, else returns false
	//
	bool IsPaused(void);
	

	////////////////////////////////////////////////////////////////////////////////////////////
	// Get the current state of the encoding thread
	//
	//  Input parameters : none
	//
	//	Output : current state of the encoding thread
	//
	int GetState(void);

	/////////////////////////////////////////////////////////////////////////////////////////////
	// Frame Capture call back routine that checks to see if a frame is needed
	//
	//  Input  :	None
	//
	//	Output :	returns a pointer to a frame buffer if the encoder is ready to accept a frame
	//				otherwise it returns NULL
	//
	unsigned char *OnNeedFrameData(void);

	/////////////////////////////////////////////////////////////////////////////////////////////
	// Frame Capture call back routine that checks to see if a frame is needed, RSX version for the PS3.
	//
	//  Input  :	None
	//
	//	Output :	Not implemented for this system, so returns 0.
	//
	uint32 OnNeedFrameDataRSXOffset(void) {return 0;}

	/////////////////////////////////////////////////////////////////////////////////////////////
	// Frame Capture call back routine that is called when a frame has been copied to the local buffer
	//	passed back in the OnNeedFrameData routine
	//
	//  Input  :	None
	//
	//	Output :	None
	//
	void OnFrameCaptured(void);

	/////////////////////////////////////////////////////////////////////////////////////////////
	//  Returns the current frame width that is being used
	//
	//  Input  :	None
	//
	//	Output :	returns the current frame width
	//
	int OnGetFrameWidth(void)  {return(m_inputWidth);}

	/////////////////////////////////////////////////////////////////////////////////////////////
	//  Returns the current frame height that is being used
	//
	//  Input  :	None
	//
	//	Output :	returns the current frame height
	//
	int OnGetFrameHeight(void) {return(m_inputHeight);}

	/////////////////////////////////////////////////////////////////////////////////////////////
	//  Used for asynchronous backbuffer rendering. As this captures movies return eCFF_CaptureThisFrame every time to capture every frame
	//
	//  Input  :	None
	//
	//	Output :	returns the setup flags for the frame
	//
	//  Note: OnCaptureFrameBegin returns flags too, so the pair should match. Flags are typically only used by other capture devices
	int OnGetCaptureFlags(void) {return (ICaptureFrameListener::eCFF_CaptureThisFrame);}
	int OnCaptureFrameBegin(void) {return (ICaptureFrameListener::eCFF_CaptureThisFrame);};

	////////////////////////////////////////////////////////////////////////////////////////
	// Set the filename to use for the output files 
	//
	//  Input parameters :
	//		pFileName		: pointer to output filename
	//
	//		Outputs : None
	//
	//
	void SetFileName(CString& pFileName);


	////////////////////////////////////////////////////////////////////////////////////////
	// Check if a filename has been set 
	//
	//  Input parameters : None
	//
	//		Outputs : true if the filename has been set, false otherwise
	//
	//
	bool IsFileNameSet(void) {return(!m_InitParams.m_sFile.IsEmpty());}


	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Initializes a set of default parameters
	//
	//  Input parameters : None
	//
	//	Output : None
	//
	void SetDefaultParams(void);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Copies the AVCRecordParameters into the passed structure
	//
	//  Input parameters : 
	//				pParameterSet  -  Pointer to a AVCRecordParameters structure 
	//
	//	Output : none
	//
	void GetParameters(AVCRecordParameters *pParameterSet);

	////////////////////////////////////////////////////////////////////////////////////////
	// This is the AV compression encoding thread that accepts input frames and actually encodes them
	// It should be started by the CAVCompression class initializer.  This is part of the CryThread Interface
	//
	//  Input parameters : none
	//
	//	Output : none
	//
	void Run(void);


	////////////////////////////////////////////////////////////////////////////////////////
	// This returns the number of frames that were encoded
	//
	//  Input parameters : none
	//
	//	Output : Total number of frames that were encoded
	//
	unsigned int GetEncodedFrames(void) {return(m_totalFramesInFile);}

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Calculates the progress in re-encoding a file
	//
	//  Input parameters :  none
	//
	//	Output : returns a percentage complete value
	//
	//
	float TranscodeProgress(void);
	

	////////////////////////////////////////////////////////////////////////////////////////
	// Returns a pointer to the last error message
	//
	//  Input parameters : none
	//
	//	Output : returns a pointer to a string for the last error message
	//
	LPCTSTR GetLastErrorMessage(void) const								{	return (LPCTSTR) m_sError;};


protected:	

	//! last error string
	CString m_sError;

private:

	///////////////////////////////////////////////////////////////////////////////////////////
	// Set the state of the encoding thread
	//
	//  Input parameters :
	//		state	: state to set
	//
	//	Output : None
	//
	void SetState(int state);


	////////////////////////////////////////////////////////////////////////////////////////
	// Finds the best mpeg framerate codes to use for a given framerate 
	//
	//  Input parameters :
	//		framerate : the framerate to check
	//
	//	Output : none
	//
	//
	void FindFrameRateCode(float framerate);


	////////////////////////////////////////////////////////////////////////////////////////
	// This routine determines how many threads to use
	// based on the number of processors available and other
	// information on the size of the frame to encode
	//
	//  Input parameters : None
	//
	//	Output : none
	//
	//
	void DetectThreadNum(void);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Frees up any memory allocations that have been allocated and deletes various encoder objects
	//
	//  Input  : None
	//
	//	Output : None
	//
	void CleanUpAVC(void);


	////////////////////////////////////////////////////////////////////////////////////////
	// Output a PES header to a serial buffer
	//
	//  Input parameters :
	//		pesPktSize		: Number of bytes pes packet excluding the pes header.  Can be 0 if it is filled in later
	//		streamID		: stream id to include in the pes header
	//		SerialStore		: pointer to the serial buffer object to use for inserting the data
	//
	//	Output : returns the offset of the first byte of the header
	//
	//
	unsigned int PutPesElemStreamHdr(unsigned int pesPktSize,int streamID,CAVCSerialBuffer *SerialStore);


	/////////////////////////////////////////////////////////////////////////////////////////////
	// Initialize the image buffers
	// They need to be aligned on 16 byte boundaries due to the possible use of SSE
	// The image buffer size needs to be based on the larger of the encoded size or 
	// the input window size, accounting for any added border padding that is needed
	//
	//  Input  : None
	//
	//	Output : None
	//
	//  Return :  returns AVCOMP error code or AVCOMP_OK if successful
	//
	HRESULT AllocateFrameBuffers(void);


	////////////////////////////////////////////////////////////////////////////////////////
	// Calculate the largest space that is needed to hold a video frame 
	//
	//  Input parameters : none
	//
	//	Output : Returns the needed size in bytes
	//
	//
	unsigned int FindMaxFrameSize(void);

	////////////////////////////////////////////////////////////////////////////////////////
	// This routine does the actual audio and video encoding for a frame
	//
	//  Input parameters : none
	//
	//	Output : None
	//
	//
	void AVencodeFrame(void);

	////////////////////////////////////////////////////////////////////////////////////////
	// Divide a video frame into sections such that the section sizes are as close to each
	// other as possible, and the section sizes are divisible by 16 (e.g., integer number of MBs)
	// They may not be all the same size.  The starting frame positions
	// for each section and the number of lines in each seaction are put into the passed arrays
	//
	//  Input parameters : 
	//				pStartRow	:	array to hold the starting macroblock row locations
	//				pSectionLen	:	Number of video lines in the section
	//
	//	Output : None
	//
	void AVdivideFrame(unsigned int *pStartRow,unsigned int *pSectionLen);


	////////////////////////////////////////////////////////////////////////////////////////
	// This routine encodes a video frame using multiple threads
	//
	//  Input parameters : 
	//			inputFormat	:	RGB or YUV
	//			frameType	:	I/P/B
	//			pCurRefBuffer	:	pointer to the current reference frame
	//			pNxtRefBuffer	:	pointer to the location to assemble the next reference frame
	//
	//	Output : returns the number of bytes of encoded data produced by the frame
	//
	//
	unsigned int AVencodeVideoFrame(int inputFormat,int frameType,uint8 *pCurRefBuffer,uint8 *pNxtRefBuffer);


	////////////////////////////////////////////////////////////////////////////////////////
	// Decodes an MPEG1 video frame at the slice level 
	//
	//  Input parameters : 
	//				frameBuffer	:	Destination buffer for the decoded video frame
	//				sectionAddr	:	array of starting addresses for sections of the video frames
	//				sectionSize	:	size of the encoded section
	//				numRows		:	number of macroblock rows in the section
	//
	//	Output : returns AVCOMP_OK if successful
	//
	//
	unsigned int AVDecodeVideoFrame(uint8 *frameBuffer,uint8 **sectionAddr,unsigned int *sectionSize,unsigned int *numRows);


	////////////////////////////////////////////////////////////////////////////////////////
	// Create a file header for intermediate compressed file
	//
	//  Input parameters : none
	//
	//	Output : none
	//
	void CreateCRYFileHeader(void);

	////////////////////////////////////////////////////////////////////////////////////////
	// Create a frame header for intermediate compressed file
	//
	//  Input parameters : 
	//		encodedVideoBytes	-  number of encoded bytes in the video frame
	//		encodedAudioBytes	-  number of encoded bytes in the audio frame
	//		frameIndex			-	sequence index for the frame
	//
	//	Output : none
	//
	void CreateCRYFrameHeader(unsigned int encodedVideoBytes,unsigned int encodedAudioBytes,unsigned int frameIndex);



	////////////////////////////////////////////////////////////////////////////////////////
	// Convert the input frame into the proper size and format
	//
	//  Input parameters : none
	//
	//	Output : none
	//
	//
	void PreProcessFrame(void);


	////////////////////////////////////////////////////////////////////////////////////////
	// Resize a frame from the input size to the encoded size
	// This is done by decimating/interpolating the input frame and then adding some padding
	//
	//  Input parameters : none
	//
	//	Output : none
	//
	//
	void ResizeFrame(void);


	////////////////////////////////////////////////////////////////////////////////////////
	// Perform a half band filter in the horizontal direction on an RGBA frame
	// The filter is done in place
	//
	//  Input parameters :
	//		pDin	: pointer to frame data
	//		width	: width in pixels of the input frame
	//		height	: height in pixels of the input frame
	//
	//	Output : None
	//
	//
	void HalfbandFilterHoriz(uint8 *pDin,int width,int height);

	////////////////////////////////////////////////////////////////////////////////////////
	// Perform a half band filter in the Vertical direction on an RGBA frame
	// The filter is done in place
	//
	//  Input parameters :
	//		pDin	: pointer to frame data
	//		width	: width in pixels of the input frame
	//		height	: height in pixels of the input frame
	//
	//	Output : None
	//
	//
	void HalfbandFilterVert(uint8 *pDin,int width,int height);



	////////////////////////////////////////////////////////////////////////////////////////
	// Add black bands to a frame in order to match the aspect ratio of the display frame
	//
	//  Input parameters : 
	//		pSsource	:	pointer to the source data
	//		pDest		:	pointer to the destination location
	//
	//	Output : None
	//
	//
	void AddBlackBars(uint8 *pSsource, uint8 *pDest);


	////////////////////////////////////////////////////////////////////////////////////////
	// Create a frame header 
	// 
	//  Input parameters : 
	//		dataType	:	either ISAUDIO or ISVIDEO
	//		dataSize	:	number of bytes in the encoded frame
	//		frameType	:	type of this frame, e.g., I/P/B
	//
	//	Output : Returns the number of bytes created
	//
	unsigned int CreateFrameHeader(unsigned int dataType,unsigned int dataSize,int frameType);


	////////////////////////////////////////////////////////////////////////////////////////
	// Create an AVI file header 
	// 
	//  Input parameters :  none
	//
	//	Output : Returns the number of bytes created
	//
	unsigned int CreateFileHeader(void);


	////////////////////////////////////////////////////////////////////////////////////////
	// Creates a pes pack header in the header serial buffer
	// See ISO/IEC 13818-1  Section 2.4.3.6 for field definitions for PES headers
	//
	//  Input parameters : none
	//
	//	Output : none
	//
	//
	void PutPesPackHdr(void);


	////////////////////////////////////////////////////////////////////////////////////////
	// Creates a pes pack system header in the header serial buffer
	// See ISO/IEC 13818-1  Section 2.4.3.6 for field definitions for PES headers
	//
	//  Input parameters : none
	//
	//	Output : none
	//
	//
	void PutPesPackSysHdr(void);


	////////////////////////////////////////////////////////////////////////////////////////
	// Creates an MPEG video sequence header
	// See ISO/IEC 13818-2  Section 6.2.2 for field definitions
	//
	//  Input parameters : none
	//
	//	Output : none
	//
	void PutSeqHdr(void);


	////////////////////////////////////////////////////////////////////////////////////////
	// Creates an MPEG video sequence extension header
	// See ISO/IEC 13818-2  Section 6.2.2 for field definitions
	//
	//  Input parameters : none
	//
	//	Output : none
	//
	void PutSeqExtHdr(void);

	////////////////////////////////////////////////////////////////////////////////////////
	// Creates an MPEG video sequence display header
	// See ISO/IEC 13818-2  Section 6.2.2 for field definitions
	//
	//  Input parameters : none
	//
	//	Output : none
	//
	void PutSeqDispExtHdr(void);

	////////////////////////////////////////////////////////////////////////////////////////
	// Creates an MPEG video GOP header
	// See ISO/IEC 13818-2  Section 6.2.2 for field definitions
	//
	//  Input parameters : none
	//
	//	Output : none
	//
	//
	void PutGOPHdr(void);


	////////////////////////////////////////////////////////////////////////////////////////
	// Creates an MPEG video picture header
	// See ISO/IEC 13818-2  Section 6.2.2 for field definitions
	//
	//  Input parameters : 
	//			frameType	:	type of frame, e.g., I/P/B
	//
	//	Output : none
	//
	//
	void PutPictHdr(int frameType);


	////////////////////////////////////////////////////////////////////////////////////////
	// Creates an MPEG video picture coding extension header
	// See ISO/IEC 13818-2  Section 6.2.2 for field definitions
	//
	//  Input parameters : 
	//			frameType	:	type of frame, e.g., I/P/B
	//
	//	Output : none
	//
	//
	void PutPictCodExtHdr(int frameType);


	////////////////////////////////////////////////////////////////////////////////////////
	// Initialize the rate control parameters
	//
	//  Input parameters : 
	//		recordQuality : 0-9, 0=worst, 9 = best
	//		passnum --	0 or 1, pass 0 is I frames only, pass 1 contains I and P frames so adjust
	//					bitrate to account for this
	//
	//	Output : None
	//
	void InitRateControl(unsigned int recordQuality,int passnum);


	////////////////////////////////////////////////////////////////////////////////////////
	// Update the rate control parameters after a frame has been encoded
	//
	//  Input parameters :
	//		numBytes		:  number of bytes encoded
	//		endOfGopFlag	:  1 indicates that this frame is the last frame in a GOP
	//
	//	Output : None
	//
	//
	void UpdateRateControl(unsigned int numBytes,int endOfGopFlag);


	////////////////////////////////////////////////////////////////////////////////////////
	// Set the Size of an AVI header
	//
	//  Input parameters :
	//		SerialStore		: pointer to the serial buffer object to use for inserting the data
	//		startLoc		: index of the location where the header size is located
	//
	//	Output : none
	//
	//
	void PutAVIHdrSize(CAVCSerialBuffer *SerialStore,int startLoc);


	////////////////////////////////////////////////////////////////////////////////////////
	// Output an AVI header to a serial buffer
	//
	//  Input parameters :
	//		SerialStore		: pointer to the serial buffer object to use for inserting the data
	//
	//	Output : returns the offset of the first byte of the header
	//
	//
	unsigned int PutAVIHdr(CAVCSerialBuffer *SerialStore);


	////////////////////////////////////////////////////////////////////////////////////////
	// Output a LIST header to the serial stream
	//
	//  Input parameters :
	//		SerialStore		: pointer to the serial buffer object to use for inserting the data
	//		plistname		: pointer to a char array for the list name (e.g., "LIST")
	//		ptag			: pointer to a char array for the tag
	//
	//	Output : returns the index to location of the list size
	//
	//
	unsigned int AVIStartList(CAVCSerialBuffer *SerialStore,char *plistname, char *ptag);


	////////////////////////////////////////////////////////////////////////////////////////
	// Creates an index for the AVI file 
	//
	//  Input parameters : none
	//
	//	Output : none
	//
	//
	void CreateAVIIndx(void);


	//////////////////////////////////////////////////////////////////////////////////////////////
	// Converts the intermediate file into the final file by recompressing the data 
	// It reads the intermediate file, frame by frame, decodes the frame, then re-encodes the frame
	// 
	//
	//  Input parameters :  none
	//
	//	Output : returns AVCOMP error if unsuccessful or AVCOMP_OK if successful
	//
	//
	int TranscodeAVFileFull(void);


	////////////////////////////////////////////////////////////////////////////////////////
	// Converts the intermediate file into the final file by copying the already compressed
	// data into an AVI file
	//
	//  Input parameters : none
	//
	//	Output : returns AVCOMP error if unsuccessful or AVCOMP_OK if successful
	//
	//
	int TranscodeAVFileFast(void);


	////////////////////////////////////////////////////////////////////////////////////////
	// Determine the frame type for the next frame
	//
	//  Input parameters :
	//		endOfGopFlag	:  pointer to a variable that indicates if this is the last frame in a GOP
	//
	//	Output : Returns frame type for the next frame, e.g., I frame, P frame, etc
	//
	//
	int GetFrameType(int *endOfGopFlag);


	// Code execution variables used by the compression engine
	unsigned int m_numVideoThreads;		// Number of parallel video encoding threads to use
	unsigned int m_validAVCContext;		// 1 indicates that the context is valid, 0 means it is in an undefined state
	int m_ThreadState;					// Current encoding thread state

	// Variables used in managing time
	float m_totalElapsedTime;			// total elapsed time since recording started
	unsigned int m_numOutputFrames;		// Number of video frames to output
	double m_systemClockMPEG;			// Variable used to keep internal time for the packet stream
	unsigned int m_frameIndex;			// Sequential frame counter, is incremented for each frame processed
	unsigned int m_numEncodedFrames;	// Number of encoded frames
	float m_lastFrameTime;				// Last frame capture call back time
	unsigned int m_pausedFrames;		// keeps track of frames while paused


	// Variables for controlling video buffer access
	CryMutex m_ThreadStateLock;					// Used for access control to thread state
	CryConditionVariable m_ThreadStateChange;	// Used to signal when a thread state change occurs


	// Video compression specific variables used during compression
	unsigned int m_inputWidth;			// width of the input frame in pixels (May change during the encoding process)
	unsigned int m_inputHeight;			// height of the input frame in pixels (May change during the encoding process)
	unsigned int m_encodedWidth;		// width of the final encoded frame
	unsigned int m_encodedHeight;		// height of the final encoded frame
	unsigned int m_displayWidth;		// width of the frame that should be displayed at the decoder in pixels
	unsigned int m_displayHeight;		// height of the frame that should be displayed at the decoder in pixels
	unsigned int m_intraDcPred;			// Number of bits used to compress the DC component of a macroblock (0=8, 1=9, 2=10
	unsigned int m_frameRateCode;               // frame rate code for header
	unsigned int m_bitRate;				// encoded stream bitrate (based on the input quality setting)
	unsigned int m_mQuant;				// Quantization parameter used to encode a frame (may change frame to frame)
	unsigned int m_qScaleType;			// 0 = linear, 1 = non-linear
	unsigned int m_intraVLCFormat;		// 0 = B-14, 1 = B-15
	unsigned int m_containerFMT;		// determines the output container type (e.g, mpeg, AVI, etc)
	float m_frameRate;					// Requested framerate
	int m_coeffLimit;					// maximum size of a coeff
	int m_videoCodecType;				// mpeg1 or mpeg2
	unsigned int m_AVIfirstFrame;       // Indicates the start of an AVI file
	unsigned int m_AVIStart;			// Starting index of the AVI header
	unsigned int m_MOVIStart;			// Starting index of the MOVI header
	unsigned int m_totalFramesLoc;		// AVI file location for totalframes
	unsigned int m_streamLenVideo;		// AVI file location for video stream length
	unsigned int m_streamLenAudio;		// AVI file location for audio stream length
	unsigned int m_recordQuality;


	// Variables used in rate control and quantization
	int m_frameQuant;					// Current quantization value
	int m_gopCount;						// position within the GOP
	int m_gopLength;					// size of the GOP in frames
	int m_GOPBits;						// Accumulated bits in a GOP
	float m_filteredBitRate;			// Filtered output bitrate
	float m_filteredMquant;				// mquant value based on the filtered bitrate

	// Buffers for intermediate video processing results
	uint8 *m_pimageBuffs;				// pointer to the allocated image buffer memory
	uint8 *m_pimageBuffAligned[4];		// pointer to a frame buffer aligned to 16 byte boundary
	unsigned int  m_allocatedFrameSize;	// Size of the frame buffers in bytes
	uint8 *m_currentRefFrame;			// Current reference frame for P pictures
	uint8 *m_nextRefFrame;				// Next reference frame currently being constructed


	CAVAudioCompression *m_pAudioEncoder;	// Pointer to the audio encoder
	unsigned int m_audioSampRate;			// audio sampling rate

	// Pointer to audio capture object
	CAVAudioCapture *m_pAudioCapture;

	// Pointers to video compression objects
	CAVVideoCompression *m_pvideoCoders[MAX_COMP_THREADS]; 

	// Thread handle for the main AVC encoding thread
	HANDLE m_hAV_MainThread;

	// pointer File manager object
	CAVCFileManager	*m_pAVCFileManager;

	CAVCSerialBuffer m_headerSerialBuff;	// a small serial buffer for outputting headers
	uint8 m_hdrBuff[HDRBUFFSIZE];			// buffer used for header serial object

	AVCRecordParameters m_InitParams;		// Passed parameter values used to init the encoder
	int m_numFramesInFile;					// frame counter used in the transcoding process
	int m_totalFramesInFile;				// Number of frames in an intermediate file


};

#endif // !defined(__AVCompression_h__)
