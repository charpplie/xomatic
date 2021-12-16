////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2010.
// -------------------------------------------------------------------------
//  File name:   AV_Compression.cpp
//  Version:     v1.00
//  Created:     09/04/2010 by John Stewart.
//  Compilers:   
//  Description: Audio/Video Compression  class implementation.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AVCompression.h"


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAVCompression Class routines
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// Constructor 
CAVCompression::CAVCompression()
{

	// Initialize the memory and file pointers
	m_sError = _T("Ok");
	m_pimageBuffs = NULL;
	m_pAudioCapture = NULL;
	m_pAudioEncoder = NULL;
	m_pAVCFileManager = NULL;
	m_allocatedFrameSize = 0;
	m_validAVCContext = 0;
	m_AVIfirstFrame = 1;
	m_gopLength = 1;  // First encoding is done with only I frames
	m_InitParams.parmsVaild = false;

	memset(m_pvideoCoders,NULL,MAX_COMP_THREADS*sizeof(CAVVideoCompression *));

	m_containerFMT = AVI_STYLE;			// Sets the container type
	m_videoCodecType = MPEG1_STYLE;		// Set the video encoder type


	if(m_videoCodecType == MPEG2_STYLE)
	{
		m_qScaleType = 1;
		m_intraVLCFormat = 1;
		m_intraDcPred = INTRADCPRED10BIT; 
		m_coeffLimit = QUANTMAXMPEG2;
	}

	if(m_videoCodecType == MPEG1_STYLE)
	{
		m_qScaleType = 0;
		m_intraVLCFormat = 0;
		m_intraDcPred = INTRADCPRED8BIT;
		m_coeffLimit = QUANTMAXMPEG1;
	}


	// Create the main AV compression thread
	m_ThreadState = AVTHREAD_IDLE;
	Start();
	m_hAV_MainThread = (HANDLE) GetHandle();
	if(m_hAV_MainThread == NULL) m_ThreadState = AVTHREAD_ERROR;

}

// Destructor
CAVCompression::~CAVCompression()
{
	Shutdown();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialize the compression engine, this is called when a recording is started
//
//  Input parameters : none
//
//	Output :
//		Outputs an AVCOMP error code as defined in header file
//
HRESULT CAVCompression::InitEngine(void)
{

	m_sError=_T("Ok");
	HRESULT hr = AVCOMP_OK;


	// Make sure this routine does not try to initialize an already initialized context
    if(m_validAVCContext) 
	{
		m_sError=_T("Compression Engine failed to initialize. Compression Context Already Initialized");
		return(AVCOMP_ALREADYINITIALIZED);  
	}


	// Setup the parameters dealing with the framesize
	m_inputWidth = m_InitParams.inputWidth;
	m_inputHeight = m_InitParams.inputHeight;
	m_displayWidth = CLAMP(m_InitParams.displayWidth,MINCOMPDIM,MAXCOMPDIM);
	m_displayHeight = CLAMP(m_InitParams.displayHeight,MINCOMPDIM,MAXCOMPDIM);
	m_encodedWidth = 16*((m_displayWidth+15) >> 4);		// Per ISO 13818-2 the encoded width/height must be divisible by 16 and at least as big as the display width/height
	m_encodedHeight = 16*((m_displayHeight+15) >> 4);	// Per ISO 13818-2 the encoded width/height must be divisible by 16 and at least as big as the display width/height


	// Find the closest valid frame rate to the one requested
	// and use it instead of the input framerate
	FindFrameRateCode(m_InitParams.frameRate);

	// Figure out how many threads to use for encoding
	DetectThreadNum();

	// Initialize the rate control parameters and target bitrate
	InitRateControl(m_InitParams.recordQuality,0);

	// Init a small serial buffer to use for headers
	m_headerSerialBuff.Init(m_hdrBuff,HDRBUFFSIZE,SERIALWRITE);

	// Clear the frame counter, elapsed time, and mpeg system clock
	m_frameIndex = 0;
	m_pausedFrames = 0;
	m_numEncodedFrames = 0;
	m_totalElapsedTime = 0.0f;
	m_systemClockMPEG = 0.0f;


    // Create and initialize the file recording manager
	m_pAVCFileManager = new CAVCFileManager;
	if(!m_pAVCFileManager)
	{
		CleanUpAVC();
		m_sError=_T("Compression Engine failed to initialize. Could not create output file object");
		return(AVCOMP_FILEOPEN);  
	}

	hr = m_pAVCFileManager->Open(m_InitParams.m_sFile,m_InitParams.recordMode,m_InitParams.recordTime,m_InitParams.frameRate,m_InitParams.maxFileSize);
	if(hr != AVCOMP_OK)
	{
		CleanUpAVC();
		m_sError=_T("Compression Engine failed to initialize. Could not create output file object");
		return(AVCOMP_FILEOPEN);  
	}

	// Create an audio encoder object
	m_pAudioEncoder = new CAVAudioCompression;
	if(!m_pAudioEncoder) 
	{
		CleanUpAVC();
		m_sError=_T("Compression Engine failed to initialize. Could not create an Audio Encoder");
		return(AVCOMP_AUDIOCREATE);
	}

	// Find the audio sampling rate and initialize the audio encoder
	unsigned int audioSampRate = DEFAULTAUDIORATE;
	ICVar* pvar = gEnv->pConsole->GetCVar("s_FormatSampleRate");
	if (pvar) m_audioSampRate = pvar->GetIVal();
	if(!m_pAudioEncoder->InitAudioEncoder(m_audioSampRate,AUDIO_PES_ID,m_containerFMT)) 
	{
		m_sError=_T("Compression Engine failed to initialize. Unsupported Audio sampling frequency");
		CleanUpAVC();
		return(AVCOMP_AUDIO_UNSUPPORTED);
	}


	// Create an audio capture object
	m_pAudioCapture = new CAVAudioCapture;
	if(!m_pAudioCapture) 
	{
		CleanUpAVC();
		m_sError=_T("Compression Engine failed to initialize. Could not create an Audio Encoder");
		return(AVCOMP_AUDIOCREATE);
	}


	// Create the video encoder objects
	for(unsigned int i=0;i<m_numVideoThreads;++i) 
	{
		m_pvideoCoders[i] = new CAVVideoCompression;
		if(!m_pvideoCoders[i])
		{
			m_sError=_T("Compression Engine failed to initialize. Could not create Video Encoder");
			CleanUpAVC();
			return(AVCOMP_VIDEOCREATE);
		}
	}


	// Allocate buffers to hold the video frames
	if((hr = AllocateFrameBuffers()) != AVCOMP_OK) 
	{
		m_sError=_T("Compression Engine failed to initialize. Memory Allocation Failed");
		CleanUpAVC();
		return(hr);
	}

	// Write the intermediate file header
	CreateCRYFileHeader();

	// Initialize the last frame time
	m_lastFrameTime = gEnv->pTimer->GetCurrTime();
	m_AVIfirstFrame = 1;

	// Register the frame capture routine
	if(!gEnv->pRenderer->RegisterCaptureFrame(this))
	{
		m_sError=_T("Compression Engine failed to initialize. Video frame capture failed to initialize");
		CleanUpAVC();
		return(AVCOMP_THREADERROR);
	}

	// Indicate that the AV context is a valid one and the encoding thread is running
	m_validAVCContext = 1;

	// Change the thread state to RECORDING
	SetState(AVTHREAD_RECORDINGIDLE);

	
	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Set the state of the encoding thread
//
//  Input parameters :
//		state	: state to set
//
//	Output : None
//
void CAVCompression::SetState(int state)
{

	m_ThreadStateLock.Lock();
	m_ThreadState = state;
	m_ThreadStateLock.Unlock();
	m_ThreadStateChange.Notify();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the current state of the encoding thread
//
//  Input parameters : none
//
//	Output : current state of the encoding thread
//
int CAVCompression::GetState(void)
{
	m_ThreadStateLock.Lock();
	int state = m_ThreadState;
	m_ThreadStateLock.Unlock();
	return(state);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialize the compression variables, this is called before recording is started
//
//  Input parameters :
//		pParameterSet	: pointer to a parameter set
//
//	Output : None
//
void CAVCompression::SetParameters(AVCRecordParameters *pParameterSet)
{

		// Setup the parameters dealing with the framesize
		m_InitParams.inputWidth = pParameterSet->inputWidth;
		m_InitParams.inputHeight = pParameterSet->inputHeight;
		m_InitParams.displayWidth = pParameterSet->displayWidth;
		m_InitParams.displayHeight = pParameterSet->displayHeight;
		m_InitParams.customWidth = pParameterSet->customWidth;
		m_InitParams.customHeight = pParameterSet->customHeight;
		m_InitParams.recordSize = pParameterSet->recordSize;
		m_InitParams.recordQuality = pParameterSet->recordQuality;
		m_InitParams.frameRate = pParameterSet->frameRate;
		m_InitParams.recordMode = pParameterSet->recordMode;
		m_InitParams.recordTime = pParameterSet->recordTime;
		m_InitParams.maxFileSize = pParameterSet->maxFileSize;
		m_InitParams.parmsVaild = true;

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copies the AVCRecordParameters into the passed structure
//
//  Input parameters : 
//				pParameterSet  -  Pointer to a AVCRecordParameters structure 
//
//	Output : none
//
void CAVCompression::GetParameters(AVCRecordParameters *pParameterSet)
{

	// Copy the parameters 
	pParameterSet->inputWidth = m_InitParams.inputWidth;
	pParameterSet->inputHeight = m_InitParams.inputHeight;
	pParameterSet->displayWidth = m_InitParams.displayWidth;
	pParameterSet->displayHeight = m_InitParams.displayHeight;
	pParameterSet->customWidth = m_InitParams.customWidth;
	pParameterSet->customHeight = m_InitParams.customHeight;
	pParameterSet->recordSize = m_InitParams.recordSize;
	pParameterSet->recordQuality = m_InitParams.recordQuality;
	pParameterSet->frameRate = m_InitParams.frameRate;
	pParameterSet->recordMode = m_InitParams.recordMode;
	pParameterSet->recordTime = m_InitParams.recordTime;
	pParameterSet->maxFileSize = m_InitParams.maxFileSize;
	pParameterSet->parmsVaild = m_InitParams.parmsVaild;

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initializes a set of default parameters
//
//  Input parameters : None
//
//	Output : None
//
void CAVCompression::SetDefaultParams(void)
{

	// Setup the parameters dealing with the framesize
	if(gEnv)
	{
		if(gEnv->pRenderer) 
		{
			m_InitParams.inputWidth = gEnv->pRenderer->GetWidth();
			m_InitParams.inputHeight = gEnv->pRenderer->GetHeight();
			m_InitParams.displayWidth = m_InitParams.inputWidth;
			m_InitParams.displayHeight = m_InitParams.inputHeight;
			m_InitParams.customWidth = 1280;
			m_InitParams.customHeight = 720;
			m_InitParams.recordSize = -1;
			m_InitParams.recordQuality = 6;
			m_InitParams.frameRate = 30.0f;
			m_InitParams.recordMode = AVCAPTURECONTINUOUS;
			m_InitParams.recordTime = 30;
			m_InitParams.maxFileSize = 2000000;
			m_InitParams.parmsVaild = true;
		}
	}


}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Release the compression engine, this is called when the recording is stopped
// The compressionMode indicates what to do with the recorded data
// Options are to throw it away, copy it to an AVI file directly, or to re-encode it
//
//  Input  : 
//			compressionMode  -- indicates the action to take
///
//	Output : Returns the number of frames encoded to this point (may be off by 1 if a frame is currently being encoded)
//
void CAVCompression::StopRecording(int compressionMode)
{

	// Get the current encoding thread state
	int state = GetState();

	// If currently recording, then stop the recording and do something with it
	if(state == AVTHREAD_RECORDINGIDLE || state == AVTHREAD_RECORDINGBUSY || state == AVTHREAD_PAUSE) SetState(compressionMode);

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// If a transcode is occuring, it aborts it
//
//  Input  : none
///
//	Output : none
//
void CAVCompression::AbortTranscode(void)
{

	// Get the current encoding thread state
	int state = GetState();
	if(state == AVTHREAD_TRANSCODINGFAST || state == AVTHREAD_TRANSCODINGFULL)  SetState(AVTHREAD_ABORT);

}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Frees up any memory allocations that have been allocated and deletes various encoder objects
//
//  Input  : None
//
//	Output : None
//
void CAVCompression::CleanUpAVC(void)
{


	if(m_pimageBuffs != NULL) free(m_pimageBuffs);
	m_pimageBuffs = NULL;
	m_allocatedFrameSize = 0;

	SAFE_DELETE(m_pAudioCapture)
	SAFE_DELETE(m_pAudioEncoder)
	SAFE_DELETE(m_pAVCFileManager)
	for(unsigned int i=0;i<MAX_COMP_THREADS;++i) SAFE_DELETE(m_pvideoCoders[i])

}


/////////////////////////////////////////////////////////////////////////////////////////////
// Initialize the intermediate image buffers
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
HRESULT CAVCompression::AllocateFrameBuffers(void)
{

	// Find the largest frame size that is needed for the image buffers
	// If it is larger than we already have allocated, then we need
	// to free up the old memory and allocate new memory

	int maxFrameSize = FindMaxFrameSize();
	if(maxFrameSize > m_allocatedFrameSize)
	{
		// Need to free and allocate buffers again
		if(m_pimageBuffs != NULL) free(m_pimageBuffs);
		m_pimageBuffs = (uint8 *) malloc(16 + 4*maxFrameSize);
		if(m_pimageBuffs == NULL) return(AVCOMP_MEM);


		// Set the 16 byte aligned image buffer addresses and size for use by the video encoder
		// This is for the possible use of SSE 
		m_pimageBuffAligned[0] = (uint8 *) (((UINT_PTR) m_pimageBuffs + 15) & ( (UINT_PTR) ~15));
		m_pimageBuffAligned[1] = m_pimageBuffAligned[0] + maxFrameSize;
		m_pimageBuffAligned[2] = m_pimageBuffAligned[1] + maxFrameSize;
		m_pimageBuffAligned[3] = m_pimageBuffAligned[2] + maxFrameSize;
		m_allocatedFrameSize = maxFrameSize;

	}

	return(AVCOMP_OK);

}

/////////////////////////////////////////////////////////////////////////////////////////////
// Change the state of the pause flag to the opposite of its current state
//
//  Input  : None
//
//	Output : None
//
void CAVCompression::TogglePause(void)
{
	if(	m_validAVCContext == 1)  
	{
		int state = GetState();
		if(state == AVTHREAD_PAUSE) SetState(AVTHREAD_UNPAUSE);
		else 
		{
			// Only pause if already recording
			if(state == AVTHREAD_RECORDINGIDLE || state == AVTHREAD_RECORDINGBUSY) SetState(AVTHREAD_PAUSE);
		}
	}

}

/////////////////////////////////////////////////////////////////////////////////////////////
// Checks the pause state of the recording
//
//  Input  : None
//
//	Output : returns true if recording is paused, else returns false
//
bool CAVCompression::IsPaused(void)
{
	
	int state = GetState();
	if(state == AVTHREAD_PAUSE) return(true);
	else return(false);

}


/////////////////////////////////////////////////////////////////////////////////////////////
// Frame Capture call back routine that checks to see if a frame is needed
//
//  Input  :	None
//
//	Output :	returns a pointer to a frame buffer if the encoder is ready to accept a frame
//				otherwise it returns NULL
//
unsigned char *CAVCompression::OnNeedFrameData(void)
{

	// Get the current time and calculate the time since the last call
	float currentTime = gEnv->pTimer->GetCurrTime();
	float frameTime = currentTime - m_lastFrameTime;
	m_lastFrameTime = currentTime;
	if(frameTime < 0) return(NULL);  // Take care of timer rollover

	// Update the total elapsed time since recording started
	m_totalElapsedTime += frameTime;

	// Check if we need to output a frame by calculated how many frames should have been 
	// output in the total elapsed time so far and comparing it to the number we have actually output
	unsigned int expectedFrames = (unsigned int) (m_totalElapsedTime * m_frameRate);
	m_numOutputFrames = expectedFrames - (m_frameIndex + m_pausedFrames);

	// Make sure some event that delays the game does not
	// cause a large number of frames to be repeated
	if(m_numOutputFrames > MAXREPEATLIMIT) m_numOutputFrames = MAXREPEATLIMIT;

	// See if we should capture a frame
	unsigned char *buffAddr = NULL;
	int state = GetState();
	if(m_validAVCContext == 1 && m_numOutputFrames > 0)
	{


		switch(state)
		{
			case AVTHREAD_PAUSE :			m_pausedFrames += m_numOutputFrames;
											break;

			case AVTHREAD_RECORDINGIDLE : 	// Check if the window size has changed and adjust memory if needed
											buffAddr = (unsigned char *) m_pimageBuffAligned[0];
											int width = gEnv->pRenderer->GetWidth();
											int height = gEnv->pRenderer->GetHeight();
											if(m_inputWidth != width || m_inputHeight != height)
											{
												// Size has changed so we need to update our encoding parameters and possibly reallocate out frame buffers
												m_inputWidth = width;
												m_inputHeight = height;
												buffAddr = NULL;

												int hr = AllocateFrameBuffers();
												if(hr != AVCOMP_OK) m_validAVCContext = 0;
												else buffAddr = (unsigned char *) m_pimageBuffAligned[0];
											}
											break;
		}
	}

	return(buffAddr);
}

/////////////////////////////////////////////////////////////////////////////////////////////
//  Frame Capture call back routine that is called when a frame has been copied to the local buffer
//	passed back in the OnNeedFrameData routine
//
//  Input  :	None
//
//	Output :	None
//
void CAVCompression::OnFrameCaptured(void)
{

	// Notify the encoder thread that a frame is ready to be encoded
	int state = GetState();
	if(state == AVTHREAD_RECORDINGIDLE) SetState(AVTHREAD_RECORDINGBUSY);

}

////////////////////////////////////////////////////////////////////////////////////////
// Set the filename to use for the output files 
//
//  Input parameters :
//		pFileName		: pointer to output filename
//
//		Outputs : None
//
//
void CAVCompression::SetFileName(CString& pFileName)
{
	// Save the filename
	m_InitParams.m_sFile = pFileName;
	if( m_InitParams.m_sFile.Find("avi") == -1) m_InitParams.m_sFile += _T(".avi");
}


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
void CAVCompression::DetectThreadNum(void)
{
	// Figure out how many threads to use.  If only 1 processor then use only 1 thread.
	// Otherwise use the lesser of the number of Macroblock rows and the number of processors

	unsigned int num_processors = gEnv->pi.numCoresAvailableToProcess;
	if(num_processors > MAX_COMP_THREADS) num_processors = MAX_COMP_THREADS;

	if(num_processors < 2) m_numVideoThreads = 1;
	else 
	{
		unsigned int rows = m_encodedHeight/(float) MACROBLOCKDIM; 
		if(rows < num_processors) m_numVideoThreads = rows;
		else m_numVideoThreads = num_processors;
	}

}


////////////////////////////////////////////////////////////////////////////////////////
// This is the AV compression encoding thread that processes the input according to the 
// current compression state
// It should be started by the CAVCompression class initializer
//
//  Input parameters : none
//
//	Output : none
//
void CAVCompression::Run(void)
{

	while (1)
	{
		// Wait on a state change notification
		m_ThreadStateLock.Lock();
		m_ThreadStateChange.Wait(m_ThreadStateLock);
		int currentState = m_ThreadState;
		m_ThreadStateLock.Unlock();


		// Encode a frame
		if(currentState == AVTHREAD_RECORDINGBUSY)
		{
				PreProcessFrame();
				AVencodeFrame();
				// Some other process could have changed the state while we were encoding
				// so only change the state if this did not happen
				int state = GetState();
				if(state == AVTHREAD_RECORDINGBUSY) SetState(AVTHREAD_RECORDINGIDLE);
		}

		// Unpause the recording
		if(currentState == AVTHREAD_UNPAUSE)
		{
			if(m_pAudioCapture) m_pAudioCapture->ResetAudioBuffer();
			SetState(AVTHREAD_RECORDINGIDLE);
		}


		// Stop recording but do not transcode the saved data
		if(currentState == AVTHREAD_TRANSCODINGNULL || currentState == AVTHREAD_ABORT)
		{
			if(m_validAVCContext == 1)
			{
				// Shut down the video capture
				gEnv->pRenderer->UnRegisterCaptureFrame(this);
				m_pAVCFileManager->Close(-1);
				CleanUpAVC();
				m_validAVCContext = 0;
			}
			SetState(AVTHREAD_IDLE);
		}

		// Abort whatever we are doing and exit the thread
		if(currentState == AVTHREAD_SHUTDOWN)
		{
				// Shut down the video capture
				if(gEnv->pRenderer)
					gEnv->pRenderer->UnRegisterCaptureFrame(this);
				if(m_pAVCFileManager)
					m_pAVCFileManager->Close(-1);
				CleanUpAVC();
				return;
		}


		// Stop recording and transcode using either the fast or full method
		if(currentState == AVTHREAD_TRANSCODINGFAST || currentState == AVTHREAD_TRANSCODINGFULL)
		{
			if(m_validAVCContext == 1)
			{
				// Shut down the video capture
				gEnv->pRenderer->UnRegisterCaptureFrame(this);

				// Change the number of frames to the max that could be saved in the lastXX format
				if(m_InitParams.recordMode == AVCAPTURELASTXXX) m_numEncodedFrames = (int) (m_frameRate * m_InitParams.recordTime);

				// Write the number of frames into the file header
				m_headerSerialBuff.Reset();
				m_headerSerialBuff.PutBit(m_numEncodedFrames >> 16,16);
				m_headerSerialBuff.PutBit(m_numEncodedFrames,16);
				unsigned int hdrLen = m_headerSerialBuff.GetBytes();
				m_headerSerialBuff.AlignToWord(0);
				m_pAVCFileManager->WriteAtLocation(m_headerSerialBuff.GetBufferAddress(),6,hdrLen,1);

				if(currentState == AVTHREAD_TRANSCODINGFAST) TranscodeAVFileFast();
				else TranscodeAVFileFull();

				m_pAVCFileManager->Close(0);
				CleanUpAVC();
				m_validAVCContext = 0;
			}
			SetState(AVTHREAD_IDLE);
		}
	}


}

////////////////////////////////////////////////////////////////////////////////////////
// This routine does the actual audio and video encoding for a frame
//
//  Input parameters : none
//
//	Output : None
//
//
void CAVCompression::AVencodeFrame(void)
{


	// Encode the video frame
	unsigned int encodedVideoBytes = AVencodeVideoFrame(AVCRGB,IFRAMETYPE,NULL,NULL);

	// Encode any audio that is available
	unsigned int encodedAudioBytes = m_pAudioEncoder->EncodeAudio(m_pAudioCapture,m_pimageBuffAligned[1],m_allocatedFrameSize,m_systemClockMPEG);


	// See if there is enough space for the frame
	int64 availSpace = m_pAVCFileManager->AvailableSpace(1);
	int64 neededSpace = encodedVideoBytes + encodedAudioBytes + MAXCRYHDRSIZE;
	if(neededSpace > availSpace)
	{
		// If not enough space for the frame, go into the pause state
		SetState(AVTHREAD_PAUSE);
		return;
	}


	// This is the beginning of a frame so update the file manager with the initial frame info
	m_pAVCFileManager->StartFrame(m_numEncodedFrames,1);

	// Output the frame header
	CreateCRYFrameHeader(encodedVideoBytes,encodedAudioBytes,m_numEncodedFrames);

	// Output the Video
	for(unsigned int i=0;i<m_numVideoThreads;++i) m_pAVCFileManager->Write(m_pvideoCoders[i]->GetBufferAddr(),m_pvideoCoders[i]->GetEncodedBytes(),1); 

	// Output the Audio
	m_pAVCFileManager->Write(m_pAudioEncoder->GetBufferAddress(),m_pAudioEncoder->GetEncodedSize(),1);   

	// End of the frame so update the file manager
	m_pAVCFileManager->EndFrame();

	// update frame counter
	m_frameIndex += m_numOutputFrames;
	++m_numEncodedFrames;

	// Update the system clock value
	m_systemClockMPEG  += (m_numOutputFrames * VIDEOCLOCKRATE/m_frameRate);

	// Update the rate control with the number of bytes used in the frames
	for(unsigned int i=0;i<m_numOutputFrames;i++) 
	{
		UpdateRateControl(encodedVideoBytes+encodedAudioBytes,1);
	}

}


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
void CAVCompression::AVdivideFrame(unsigned int *pStartRow,unsigned int *pSectionLen)
{

	assert(m_numVideoThreads);

	// The frame is divided into sections that are as close in size as possible
	float MB_accum = 0.0f;
	float MB_delta = (float) m_encodedHeight/((float) m_numVideoThreads * MACROBLOCKDIM);
	for(unsigned int i=0;i<m_numVideoThreads;++i) 
	{
		pStartRow[i] = MACROBLOCKDIM * (unsigned int) MB_accum;
		MB_accum += MB_delta;
	}

	for(unsigned int i=0;i<m_numVideoThreads;++i) 
	{
		if(i != m_numVideoThreads-1) pSectionLen[i] = pStartRow[i+1] - pStartRow[i];
		else pSectionLen[i] = m_encodedHeight - pStartRow[i];
	}
}


////////////////////////////////////////////////////////////////////////////////////////
// This routine encodes a video frame using multiple threads
//
//  Input parameters : 
//			inputFormat		:	RGB or YUV
//			frameType		:	I/P/B
//			pCurRefBuffer	:	pointer to the current reference frame
//			pNxtRefBuffer	:	pointer to the location to assemble the next reference frame
//
//	Output : returns the number of bytes of encoded data produced by the frame
//
//
unsigned int CAVCompression::AVencodeVideoFrame(int inputFormat,int frameType,uint8 *pCurRefBuffer,uint8 *pNxtRefBuffer)
{

	unsigned int pStartRow[MAX_COMP_THREADS];
	unsigned int pSectionLen[MAX_COMP_THREADS];

	// The frame is divided into equal sections and a thread is created for each section 
	AVdivideFrame(pStartRow,pSectionLen);

	uint8 *pSerialBuffAddr[MAX_COMP_THREADS];
	for(unsigned int i=0;i<m_numVideoThreads;++i) 
	{
		pSerialBuffAddr[i] = &m_pimageBuffAligned[0][pStartRow[i]*m_encodedWidth*RGBASIZE]; 
		unsigned int serialBuffSize = (unsigned int) pSectionLen[i]*m_encodedWidth*RGBASIZE;


		// Initialize the video encoders
		m_pvideoCoders[i]->InitVideoCompression(pStartRow[i],pSectionLen[i],pSerialBuffAddr[i],serialBuffSize,m_pimageBuffAligned[0],m_pimageBuffAligned[1],pCurRefBuffer,pNxtRefBuffer,
			m_encodedWidth,m_encodedHeight,m_mQuant,VIDEO_PES_ID,m_containerFMT,m_videoCodecType,inputFormat,frameType);

	}

	// Start the encoding threads
	for(unsigned int i=0;i<m_numVideoThreads;++i) 
	{
		m_pvideoCoders[i]->Start();
		HANDLE hThread = (HANDLE) m_pvideoCoders[i]->GetHandle();
		if (hThread == 0) {
			for(unsigned int j=0;j<i;++j) m_pvideoCoders[j]->WaitForThread();
			for(unsigned int j=0;j<m_numVideoThreads;++j) m_pvideoCoders[j]->Stop();
			return(0);
		}
	}

	// Wait on the threads to finish
	for(unsigned int i=0;i<m_numVideoThreads;++i)
	{
		m_pvideoCoders[i]->WaitForThread();
		m_pvideoCoders[i]->Stop();
	}



	// Find out how many bytes are in the encoded video frame
	unsigned int encodedVideoBytes=0;
	for(unsigned int i=0;i<m_numVideoThreads;++i) encodedVideoBytes += m_pvideoCoders[i]->GetEncodedBytes();

	return(encodedVideoBytes);

}




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
void CAVCompression::InitRateControl(unsigned int recordQuality,int passnum)
{

	// Select a bitrate and starting quantization value based on the recordQuality setting
	const float bpp[10] = {0.7f,0.8f,0.9f,1.0f,1.1f,1.2f,1.3f,1.4f,1.5f,1.6f};
	const int mquant_start[10] = {11,10,9,8,7,6,5,4,3,2};  // corresponds roughly with the bpp above

	// Calculate a 4th root non-linear scale factor to account for the fact that as the picture
	// size gets smaller, the information per pixel increases.  This scaling will give a higher
	// bit per pixel for smaller picture sizes for the same quality setting (this is based on trial and error)
	float sizeScale = sqrt(sqrt((1920.0f*1080.0f)/(float) (m_encodedWidth*m_encodedHeight)));

	m_bitRate = sizeScale * m_encodedWidth*m_encodedHeight*bpp[recordQuality]*m_frameRate;
	m_mQuant = mquant_start[recordQuality];  

	// If passnum = 1 then we are doing P frames, so reduce the bitrate by a factor
	if(passnum) m_bitRate = m_bitRate/TRANSCODINGFACTOR;

	// initialize the running average for bitrate and the initial quantization value
	m_filteredMquant = (float) m_mQuant;
	m_filteredBitRate = m_bitRate;
	m_GOPBits = 0;
	m_gopCount = 0;

}

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
void CAVCompression::UpdateRateControl(unsigned int numBytes,int endOfGopFlag)
{

	assert(m_gopLength);
	assert(m_bitRate);
	assert(numBytes);

	// Update the acculmulated bits for the GOP
	m_GOPBits += numBytes*8;

	// Only update the rate control parameters at the end of a GOP
	if(endOfGopFlag)
	{

		float lastGOPBitRate = m_frameRate*(m_GOPBits/(float) m_gopLength);

		// Filter the bit rate to smooth out variations
		m_filteredBitRate = RATECONTROLFILTERCOEF*lastGOPBitRate + (1.0f - RATECONTROLFILTERCOEF)*m_filteredBitRate;


		// Adjust the quantization value based on the filtered bit rate and the last frame
		if(m_filteredBitRate < m_bitRate && lastGOPBitRate < m_bitRate) 
		{
			// Need to increase the bitrate
			m_filteredMquant -=  MQUANTADJUSTRATE * m_gopLength * ((m_bitRate/lastGOPBitRate)  - 1.0f); // lastGOPBitRate cannot be 0 since there are always headers
			if(m_filteredMquant < MINQUANTLIMIT) m_filteredMquant = MINQUANTLIMIT;
		}

		if(m_filteredBitRate > m_bitRate && lastGOPBitRate > m_bitRate)
		{
			// Need to decrease the bitrate
			m_filteredMquant +=  MQUANTADJUSTRATE * m_gopLength * ((lastGOPBitRate/(float) m_bitRate)  - 1.0f);  // m_bitRate cannot be 0 since minimum quant value is used
			if(m_filteredMquant > MAXQUANTLIMIT) m_filteredMquant = MAXQUANTLIMIT;
		}

		// Set the mquant value based on the averaged data
		int lastmquant = m_mQuant;
		int tmpquant = (int) (m_filteredMquant + 0.5f);  // Round to nearest integer
		m_mQuant = tmpquant;

		// Reset the GOP bit accumulator for the next GOP
		m_GOPBits = 0;
	}

}

////////////////////////////////////////////////////////////////////////////////////////
// Determine the frame type for the next frame
//
//  Input parameters :
//		endOfGopFlag	:  pointer to a variable that indicates if this is the last frame in a GOP
//
//	Output : Returns frame type for the next frame, e.g., I frame, P frame, etc
//
//
int CAVCompression::GetFrameType(int *endOfGopFlag)
{

		assert(endOfGopFlag);


		int frameType;

		if(m_gopLength == 1)
		{
			frameType = IFRAMETYPE;  // For I frames only, don't need any reference frames
			*endOfGopFlag = 1;
		}
		else
		{
			if(m_gopCount == 0) 
			{
				frameType = IREFFRAMETYPE;  // this frame will be used as a reference frame
				*endOfGopFlag = 0;
			}
			else
			{
				if(m_gopCount == m_gopLength-1)
				{
					frameType = PFRAMETYPE;  // next frame will be an I frame so don't need this as a reference
					*endOfGopFlag = 1;
				}
				else
				{
					frameType = PREFFRAMETYPE;  // this frame will be used as a reference frame
					*endOfGopFlag = 0;
				}
			}
		}

		++m_gopCount;
		if(m_gopCount == m_gopLength) m_gopCount = 0;

		return(frameType);
}



////////////////////////////////////////////////////////////////////////////////////////
// Create a file header for intermediate compressed file
//
//  Input parameters : none
//
//	Output : none
//
void CAVCompression::CreateCRYFileHeader(void)
{

	// Create and output the headers for the file
	m_headerSerialBuff.Reset();
	m_headerSerialBuff.PutBit(FILEHDRSYNC >> 16,16);
	m_headerSerialBuff.PutBit(FILEHDRSYNC,16);
	m_headerSerialBuff.PutBit(20,16); // Header Size in Bytes including the sync
	m_headerSerialBuff.PutBit(0,16); // number of frames in the file, 0 for now
	m_headerSerialBuff.PutBit(0,16); // number of frames in the file, 0 for now
	m_headerSerialBuff.PutBit(m_encodedWidth,16);
	m_headerSerialBuff.PutBit(m_encodedHeight,16);
	m_headerSerialBuff.PutBit(m_displayWidth,16);
	m_headerSerialBuff.PutBit(m_displayHeight,16);
	m_headerSerialBuff.PutBit(m_frameRateCode,4);
	m_headerSerialBuff.PutBit(m_InitParams.recordMode,4);
	m_headerSerialBuff.PutBit(m_InitParams.recordQuality,8);
	unsigned int hdrLen = m_headerSerialBuff.GetBytes();
	m_headerSerialBuff.AlignToWord(0);
	m_pAVCFileManager->Write(m_headerSerialBuff.GetBufferAddress(),hdrLen,1);   

}

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
void CAVCompression::CreateCRYFrameHeader(unsigned int encodedVideoBytes,unsigned int encodedAudioBytes,unsigned int frameIndex)
{

	unsigned int pStartRow[MAX_COMP_THREADS];
	unsigned int pSectionLen[MAX_COMP_THREADS];

	AVdivideFrame(pStartRow,pSectionLen);

	// Create and output the headers for the file
	m_headerSerialBuff.Reset();
	m_headerSerialBuff.PutBit(FILEFRAMESYNC >> 16,16);
	m_headerSerialBuff.PutBit(FILEFRAMESYNC,16);
	m_headerSerialBuff.PutBit(24+m_numVideoThreads*8,16);  // frame header size
	m_headerSerialBuff.PutBit(m_numVideoThreads,16);  // number of sections
	m_headerSerialBuff.PutBit(encodedVideoBytes >> 16,16);
	m_headerSerialBuff.PutBit(encodedVideoBytes,16);
	m_headerSerialBuff.PutBit(encodedAudioBytes >> 16,16);
	m_headerSerialBuff.PutBit(encodedAudioBytes,16);
	m_headerSerialBuff.PutBit(m_numOutputFrames,16);
	m_headerSerialBuff.PutBit(m_mQuant,16);
	m_headerSerialBuff.PutBit(frameIndex >> 16,16);
	m_headerSerialBuff.PutBit(frameIndex,16);

	for(unsigned int i=0;i<m_numVideoThreads;++i)
	{
		unsigned int sectionSize = m_pvideoCoders[i]->GetEncodedBytes();
		m_headerSerialBuff.PutBit(sectionSize >> 16,16);
		m_headerSerialBuff.PutBit(sectionSize,16);

		unsigned int sectionLen = pSectionLen[i];
		m_headerSerialBuff.PutBit(sectionLen >> 16,16);
		m_headerSerialBuff.PutBit(sectionLen,16);

	}
	unsigned int hdrLen = m_headerSerialBuff.GetBytes();
	m_headerSerialBuff.AlignToWord(0);
	m_pAVCFileManager->Write(m_headerSerialBuff.GetBufferAddress(),hdrLen,1);   
}


////////////////////////////////////////////////////////////////////////////////////////
// Create an AVI file header 
// 
//
//  Input parameters :  none
//
//	Output : Returns the number of bytes created
//
unsigned int CAVCompression::CreateFileHeader(void)
{

	int nonVideoBytes = 0;
	if(m_containerFMT == AVI_STYLE && m_AVIfirstFrame == 1)
	{

		// Create and output the file header
		m_headerSerialBuff.Reset();
		int startLoc = m_headerSerialBuff.GetBytes();
		uint8 *pTmp = m_headerSerialBuff.GetBufferAddress();

		PutAVIHdr(&m_headerSerialBuff); 
		m_AVIfirstFrame = 0;

		// Write it to disk
		unsigned int nonVideoBytes = m_headerSerialBuff.GetBytes() - startLoc;
		m_pAVCFileManager->Write(pTmp,nonVideoBytes,0);

	}

	return(nonVideoBytes);
}




////////////////////////////////////////////////////////////////////////////////////////
// Create a frame header 
// 
//
//  Input parameters : 
//		dataType	:	either ISAUDIO or ISVIDEO
//		dataSize	:	number of bytes in the encoded frame
//		frameType	:	type of this frame, e.g., I/P/B
//
//	Output : Returns the number of bytes created
//
unsigned int CAVCompression::CreateFrameHeader(unsigned int dataType,unsigned int dataSize,int frameType)
{


	// Don't do anything if there is no data
	if(dataSize == 0) return(0);

	// Create and output the headers for the frame
	m_headerSerialBuff.Reset();
	int startLoc = m_headerSerialBuff.GetBytes();
	uint8 *pTmp = m_headerSerialBuff.GetBufferAddress();


	if(m_containerFMT == AVI_STYLE && m_AVIfirstFrame == 1)
	{
		// For the first frame, output the main AVI header
		PutAVIHdr(&m_headerSerialBuff); 
		m_AVIfirstFrame = 0;
	}


	if(dataType == ISVIDEO)
	{
		if(m_containerFMT == AVI_STYLE)
		{
			// Output a header for each frame
			m_headerSerialBuff.PutText("00dc"); 
			int moviStartLoc = m_headerSerialBuff.GetBytes();
			m_headerSerialBuff.PutLe32(0);
			if(frameType == IFRAMETYPE || frameType == IREFFRAMETYPE)
			{
				PutSeqHdr();
				if(m_videoCodecType == MPEG2_STYLE)
				{
					PutSeqExtHdr();
					PutSeqDispExtHdr();
				}
				PutGOPHdr();
			}
			PutPictHdr(frameType);
			if(m_videoCodecType == MPEG2_STYLE) PutPictCodExtHdr(frameType);
			m_headerSerialBuff.AlignToWord(0);
			// Adjust the header size
			unsigned int moviEndLoc = m_headerSerialBuff.GetBytes();
			unsigned int videoSize = dataSize + (moviEndLoc - moviStartLoc - MOVIHDROFFSET);  // The 4 length bytes are not counted
			pTmp[moviStartLoc] = (uint8) (videoSize & 0xFF);
			pTmp[moviStartLoc+1] = (uint8) ((videoSize >> 8) & 0xFF);
			pTmp[moviStartLoc+2] = (uint8) ((videoSize >> 16) & 0xFF);
			pTmp[moviStartLoc+3] = (uint8) ((videoSize >> 24) & 0xFF);
		}

		
		
		if(m_containerFMT == MPEG1_STYLE || m_containerFMT == MPEG2_STYLE)
		{
			// Output a header for each frame
			PutPesPackHdr();
			PutPesPackSysHdr();
			int pesStartLoc = PutPesElemStreamHdr(0,VIDEO_PES_ID,&m_headerSerialBuff);
			if(frameType == IFRAMETYPE || frameType == IREFFRAMETYPE)
			{
				PutSeqHdr();
				if(m_videoCodecType == MPEG2_STYLE)
				{
					PutSeqExtHdr();
					PutSeqDispExtHdr();
				}
				PutGOPHdr();
			}
			PutPictHdr(frameType);
			if(m_videoCodecType == MPEG2_STYLE) PutPictCodExtHdr(frameType);
			m_headerSerialBuff.AlignToWord(0);
			// Adjust the header size
			unsigned int pesEndLoc = m_headerSerialBuff.GetBytes();
			unsigned int videoPesSize = pesEndLoc - pesStartLoc - PESHDROFFSET;  // The pes packet length does not count the first 6 bytes
			pTmp[pesStartLoc+4] = (uint8) ((videoPesSize >> 8) & 0xFF);
			pTmp[pesStartLoc+5] = (uint8) ((videoPesSize) & 0xFF);
		}
	}


	// Write it to disk
	unsigned int nonVideoBytes = m_headerSerialBuff.GetBytes() - startLoc;
	m_pAVCFileManager->Write(pTmp,nonVideoBytes,0);

	return(nonVideoBytes);
}

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
void CAVCompression::PutAVIHdrSize(CAVCSerialBuffer *SerialStore,int startLoc)
{

	assert(SerialStore);

	int strhEndLoc = SerialStore->GetBytes();
	unsigned char *pstrhBuff = SerialStore->GetBufferAddress();
	int strhLen = strhEndLoc - startLoc - 4;  // the 4 byte length is not counted
	for(unsigned int i=0;i<4;i++)
	{
		pstrhBuff[startLoc] = (unsigned char) (strhLen & 0xFF);
		strhLen >>= 8;
		++startLoc;
	}
}


////////////////////////////////////////////////////////////////////////////////////////
// Output a LIST header to the serial stream
//
//  Input parameters :
//		SerialStore		: pointer to the serial buffer object to use for inserting the data
//		plistname		: pointer to a char array for the list name (e.g., "LIST")
//		tag				: pointer to a char array for the tag
//
//	Output : returns the index to location of the list size
//
unsigned int CAVCompression::AVIStartList(CAVCSerialBuffer *SerialStore,char *plistname, char *ptag)
{

	assert(SerialStore);

	SerialStore->PutText(plistname);	// LIST header
	int StartListLoc = SerialStore->GetBytes();
	SerialStore->PutLe32(0);		// Space for the length to be filled in later
	SerialStore->PutText(ptag);		// output the tag
	return(StartListLoc);
}

////////////////////////////////////////////////////////////////////////////////////////
// Output an AVI header to a serial buffer
//
//  Input parameters :
//		SerialStore		: pointer to the serial buffer object to use for inserting the data
//
//	Output : returns the offset of the first byte of the header
//
//
unsigned int CAVCompression::PutAVIHdr(CAVCSerialBuffer *SerialStore)
{

	assert(SerialStore);


	// NOTE :  Indentation in this file is non-standard and 
	// is used to show the hierarchy of the AVI lists



	// Start the top level RIFF list
	m_AVIStart = AVIStartList(SerialStore,"RIFF","AVI ");


		// Start of the AVI header List
		int hdrlStartListLoc = AVIStartList(SerialStore,"LIST","hdrl");

			// Insert the avih header
			SerialStore->PutText("avih");  // avih header
			int sectionStartLoc = SerialStore->GetBytes();
			SerialStore->PutLe32(0);		// header size, filled in later
			SerialStore->PutLe32((int) 1000000.0f/m_frameRate);  // Output the microsecs per frame
			SerialStore->PutLe32(m_bitRate >> 3);  // Output the Byte rate per frame
			SerialStore->PutLe32(0);		// Indicate the padding type
			SerialStore->PutLe32(AVI_TRUSTCKTYPE | AVI_HASINDEX | AVI_ISINTERLEAVED);  // Set the AVI flags
			m_totalFramesLoc = SerialStore->GetBytes(); // Remember where to put the frame count
			SerialStore->PutLe32(0);						// Dummy frame count, filled in later
			SerialStore->PutLe32(0);						// Initial Frame
			SerialStore->PutLe32(2);						// Number of streams in the file, 1 audio + 1 video
			SerialStore->PutLe32(m_encodedWidth*m_encodedHeight);		// Suggested buffer size
			SerialStore->PutLe32(m_encodedWidth);						// Frame Width
			SerialStore->PutLe32(m_encodedHeight);						// Frame Height
			SerialStore->PutLe32(0);									// Reserved
			SerialStore->PutLe32(0);									// Reserved
			SerialStore->PutLe32(0);									// Reserved
			SerialStore->PutLe32(0);									// Reserved
			PutAVIHdrSize(SerialStore,sectionStartLoc);  // set the avih size




			// Start of the video stream list
			int vidlistStartLoc = AVIStartList(SerialStore,"LIST","strl");


				//**************************************STRH***********************************
				// Start of the stream header for video
				SerialStore->PutText("strh");
				sectionStartLoc = SerialStore->GetBytes();
				SerialStore->PutLe32(0);	// Space for the length to be filled in later
				SerialStore->PutText("vids");  // fccType (video)
				// Indicate video handler
				switch(m_videoCodecType)
				{
					case MPEG1_STYLE :	SerialStore->PutText("mpg1");
										break;
					case MPEG2_STYLE :	SerialStore->PutText("mpg2");
										break;

				}
				SerialStore->PutLe32(0);	// flags
				SerialStore->PutLe32(0);	// priority & language
				SerialStore->PutLe32(0);	// initial frame

				if((m_frameRate - (int) m_frameRate) != 0.0f)
				{
					SerialStore->PutLe32(1001);	// scale
					SerialStore->PutLe32((int) (m_frameRate*1001.0f));	// frames per second
				}
				else
				{
					SerialStore->PutLe32(1);	// scale
					SerialStore->PutLe32((int) m_frameRate);	// frames per second
				}

				SerialStore->PutLe32(0);	// start time of the stream
				m_streamLenVideo = SerialStore->GetBytes();	// Need to fill in this location later
				SerialStore->PutLe32(0);					// Size of the stream
				SerialStore->PutLe32(m_encodedWidth*m_encodedHeight);		// Suggested buffer size
				SerialStore->PutLe32(-1);	// Quality
				SerialStore->PutLe32(0);	// Samples per chunk (0 = 1 frame)
				SerialStore->PutLe16(0);	// rect desc start x
				SerialStore->PutLe16(0);	// rect desc start y
				SerialStore->PutLe16(m_encodedWidth); // rect desc end x
				SerialStore->PutLe16(m_encodedHeight); // rect desc end y
				PutAVIHdrSize(SerialStore,sectionStartLoc);  // set the strh size
				//*****************************************************************************



				//**************************************STRF***********************************
				// Start of the stream header list for video
				SerialStore->PutText("strf");
				sectionStartLoc = SerialStore->GetBytes();
				SerialStore->PutLe32(0);	// Space for the length to be filled in later
				SerialStore->PutLe32(40);   // Size of bitinfo struct
				SerialStore->PutLe32(m_encodedWidth);	
				SerialStore->PutLe32(m_encodedHeight);	    // Switch this to -height if upside down
				SerialStore->PutLe16(1);					// number of planes
				SerialStore->PutLe16(24);					// bit depth

				// codec tag as an integer
				if(m_videoCodecType == MPEG1_STYLE) SerialStore->PutText("mpg1");
				else SerialStore->PutText("mpg2");

				SerialStore->PutLe32(m_encodedWidth*m_encodedHeight*3);	    // decoded frame size
				SerialStore->PutLe32(0);
				SerialStore->PutLe32(0);
				SerialStore->PutLe32(0);
				SerialStore->PutLe32(0);
				PutAVIHdrSize(SerialStore,sectionStartLoc);
				//*****************************************************************************



				//**************************************VPRP***********************************
				// Start of the video properties section
				SerialStore->PutText("vprp");
				sectionStartLoc = SerialStore->GetBytes();
				SerialStore->PutLe32(0);	// Space for the length to be filled in later
				SerialStore->PutLe32(0);    // Video format unknown
				SerialStore->PutLe32(0);	// Video standard unknown
				SerialStore->PutLe32(m_frameRate);	// vertical refresh rate
				SerialStore->PutLe32(m_encodedWidth);	// encoded width
				SerialStore->PutLe32(m_encodedHeight);	// encoded height
				SerialStore->PutLe16(m_encodedHeight);	// den of aspect ratio  (may need to reduce)
				SerialStore->PutLe16(m_encodedWidth);	// num of aspect ratio
				SerialStore->PutLe32(m_encodedWidth);	// encoded width
				SerialStore->PutLe32(m_encodedHeight);	// encoded height
				SerialStore->PutLe32(1);				// Indicate it is a progressive frame
				SerialStore->PutLe32(m_encodedHeight);	// encoded height
				SerialStore->PutLe32(m_encodedWidth);	// encoded width
				SerialStore->PutLe32(m_displayHeight);	// display height
				SerialStore->PutLe32(m_displayWidth);	// display width
				SerialStore->PutLe32(0);				// x offset
				SerialStore->PutLe32(0);				// y offset
				SerialStore->PutLe32(0);				// x offset
				SerialStore->PutLe32(0);				// y offset
				PutAVIHdrSize(SerialStore,sectionStartLoc);
				//*****************************************************************************


			// End of the video list
			PutAVIHdrSize(SerialStore,vidlistStartLoc);




			// Start of the audio stream list
			int audlistStartLoc = AVIStartList(SerialStore,"LIST","strl");


				//**************************************STRH***********************************
				// Start of the stream header for audio
				SerialStore->PutText("strh");
				sectionStartLoc = SerialStore->GetBytes();
				SerialStore->PutLe32(0);		// Space for the length to be filled in later
				SerialStore->PutText("auds");	// Indicate audio
				SerialStore->PutLe32(1);		// audio tag
				SerialStore->PutLe32(0);	// flags
				SerialStore->PutLe32(0);	// priority & language
				SerialStore->PutLe32(0);	// initial frame
				switch(m_audioSampRate)
				{

				case SAMPLINGFREQ48000 :	// 48000/1152
											SerialStore->PutLe32(3);	// time scale
											SerialStore->PutLe32(125);	// audio sampling rate
											break;

				case SAMPLINGFREQ44100 :	// 44100/1152
											SerialStore->PutLe32(32);	// time scale
											SerialStore->PutLe32(1125);	// audio sampling rate
											break;

				case SAMPLINGFREQ32000 :	// 32000/1152
											SerialStore->PutLe32(9);	// time scale
											SerialStore->PutLe32(250);	// audio sampling rate
											break;
				}
				SerialStore->PutLe32(0);	// start time of the stream
				m_streamLenAudio = SerialStore->GetBytes();	// Need to fill in this location later
				SerialStore->PutLe32(0);					// Size of the stream
				SerialStore->PutLe32(m_audioSampRate);		// Suggested buffer size
				SerialStore->PutLe32(-1);	// Quality
				SerialStore->PutLe32(0);	// Samples per chunk (0 = 1 frame)
				SerialStore->PutLe32(0);	// rect desc (not sure if this is used in audio
				SerialStore->PutLe32(0);
				PutAVIHdrSize(SerialStore,sectionStartLoc);  // set the strh size
				//*****************************************************************************



				//**************************************STRF***********************************
				// Start of the stream header list for audio
				SerialStore->PutText("strf");
				sectionStartLoc = SerialStore->GetBytes();
				SerialStore->PutLe32(0);	// Space for the length to be filled in later
				SerialStore->PutLe16(80);	// output is MPEG audio
				SerialStore->PutLe16(2);	// number of channels
				SerialStore->PutLe32(m_audioSampRate);	// sampling rate
				SerialStore->PutLe32(COMPRESSEDAUDIORATE/8);	// Avg bytes per second
				SerialStore->PutLe16(MPEGAUDIOFRAMEWIDTH);				// block size for MPEG layer 2
				SerialStore->PutLe16(0);	// bits per sample, set to 0 since this is compressed data
				SerialStore->PutLe16(22);	// Size of extra data for mpeg layer 2
				SerialStore->PutLe16(2);	// Indicate layer 2 audio
				SerialStore->PutLe32(COMPRESSEDAUDIORATE);	// Bit rate
				SerialStore->PutLe16(1);	// Indicate stereo
				SerialStore->PutLe16(0);	// HeadMode extension (only used for joint stereo)
				SerialStore->PutLe16(0);	// De-emphasis mode (0=none)
				SerialStore->PutLe16(16);	// flags :  16 = mpeg-1 stream
				SerialStore->PutLe32(0);	// first PTS low
				SerialStore->PutLe32(0);	// first PTS high
				PutAVIHdrSize(SerialStore,sectionStartLoc);
				//*****************************************************************************


			// End of the audio list
			PutAVIHdrSize(SerialStore,audlistStartLoc);


		// End of the main list
		PutAVIHdrSize(SerialStore,hdrlStartListLoc);



		// Put some info about the file
		int infoStartLoc = AVIStartList(SerialStore,"LIST","INFO");

			SerialStore->PutText("ICMT");  // Indicate it is a comment
			char *commentfield = "MPEG layer II audio, MPEG 1 Video";
			unsigned int strLength = strlen(commentfield);
			unsigned int extraBytes = 4 - strLength%4;
			SerialStore->PutLe32(strLength + extraBytes);
			SerialStore->PutText(commentfield);
			for(unsigned int i=0;i<extraBytes;i++) SerialStore->PutBit(0,8);


			// Can put other defined fields here if wanted

		// End of Info List
		PutAVIHdrSize(SerialStore,infoStartLoc);


		// Add some dummy data to allow some space
		// for the INFO list above to be extended by editors
		SerialStore->PutText("JUNK");
		sectionStartLoc = SerialStore->GetBytes();
		SerialStore->PutLe32(0);	// Size, filled in later
		for(unsigned int i=0;i<AVIJUNKSIZE;i++) SerialStore->PutBit(0,8);
		SerialStore->AlignToWord(0);  // Make sure that the header is aligned to a word boundary
		PutAVIHdrSize(SerialStore,sectionStartLoc);


		// Start the top level movi list
		m_MOVIStart = AVIStartList(SerialStore,"LIST","movi");


	return(m_AVIStart - 4);

}

////////////////////////////////////////////////////////////////////////////////////////
// Creates an index for the AVI file 
//
//  Input parameters : none
//
//	Output : none
//
//
void CAVCompression::CreateAVIIndx(void)
{

	CAVCSerialBuffer indexSerialBuff; 
	unsigned char tmpBuff[16];

	// Initialize the serial line buffer that will be used to hold the index
	// Reuse the video frame buffers since they are no longer needed
	unsigned int bufferSize = 2*m_allocatedFrameSize;
	indexSerialBuff.Init(m_pimageBuffAligned[0],bufferSize,SERIALWRITE);

    // Get the file location to write the index size
	unsigned int endMovi = m_pAVCFileManager->OutputByteCount(0);

	// Write the index header
	indexSerialBuff.PutText("idx1");
	indexSerialBuff.PutLe32(0);	// Space for the length to be filled in later

	// Index through the chunks and make the list
	unsigned int moviIndex = 4;
	unsigned int fileOffset = m_MOVIStart + 8;
	while(fileOffset < endMovi)
	{
		// Read the stream type
		m_pAVCFileManager->ReadFromLocation(tmpBuff,fileOffset,8,0);

		// Check to make sure it is valid chunk
		if(tmpBuff[0] != '0') break;
		if(!(tmpBuff[1] == '0' || tmpBuff[1] == '1')) break;
		if(!(tmpBuff[2] == 'w' || tmpBuff[2] == 'd')) break;
		if(!(tmpBuff[3] == 'b' || tmpBuff[3] == 'c')) break;
		indexSerialBuff.CopyBytes(tmpBuff,4);

		// Set the flags
		indexSerialBuff.PutLe32(AVI_KEYFRAME);	

		// Set the offset from movi start
		indexSerialBuff.PutLe32(moviIndex);	

		// Calculate the length of the chunk
		unsigned chunkLength = ((unsigned int) tmpBuff[4]) + (((unsigned int) tmpBuff[5]) << 8) + (((unsigned int) tmpBuff[6]) << 16) + (((unsigned int) tmpBuff[7]) << 24);
		indexSerialBuff.PutLe32(chunkLength);
		chunkLength += 8;  // Must account for the first 8 bytes of the chunk which are not counted in the chunkLength

		// update the file location
		fileOffset += chunkLength;
		moviIndex += chunkLength;

		// Check to see if the next entry will overrun the buffer size
		unsigned int nextLoc = indexSerialBuff.GetBytes() + 64;  // Give some extra room
		if(nextLoc > bufferSize)
		{
			// Flush and reset the serial buffer
			// We do not have to take care of the word size since we always write a multiple of 16 bytes
			m_pAVCFileManager->Write(indexSerialBuff.GetBufferAddress(),indexSerialBuff.GetBytes(),0);
			indexSerialBuff.Init(m_pimageBuffAligned[0],bufferSize,SERIALWRITE);
		}

	}

	// Write the buffer to disk
	if(indexSerialBuff.GetBytes() > 0) m_pAVCFileManager->Write(indexSerialBuff.GetBufferAddress(),indexSerialBuff.GetBytes(),0);


	// Close the idx1 chunk
	unsigned int numBytes = m_pAVCFileManager->OutputByteCount(0) - endMovi - 8;
	m_pAVCFileManager->WriteAtLocation((unsigned char *) &numBytes,endMovi + 4,4,0);

}



////////////////////////////////////////////////////////////////////////////////////////
// Creates a pes pack header in the header serial buffer 
// See ISO/IEC 13818-1  Section 2.4.3.6 for field definitions for PES headers
//
//  Input parameters : none
//
//	Output : none
//
//
void CAVCompression::PutPesPackHdr(void)
{

	// Convert system clock time to the proper format
	int64 PCR_BASE = ((int64) (m_systemClockMPEG/300.0f));  // MPEG Clock

	m_headerSerialBuff.PutBit(0,16);
	m_headerSerialBuff.PutBit(PACK_START_CODE,16);
	if(m_containerFMT == MPEG1_STYLE) m_headerSerialBuff.PutBit(2,4); // Indicate MPEG 1
	else m_headerSerialBuff.PutBit(1,2); // Indicate MPEG 2
	m_headerSerialBuff.PutBit((int)((PCR_BASE >> 30) & 0x07),3);
	m_headerSerialBuff.PutBit(1,1);  // marker bit
	m_headerSerialBuff.PutBit((int)((PCR_BASE >> 15) & 0x7fff),15);
	m_headerSerialBuff.PutBit(1,1);   // marker bit
	m_headerSerialBuff.PutBit((int)(PCR_BASE & 0x7fff),15);
	m_headerSerialBuff.PutBit(1,1);   // marker bit
	if(m_containerFMT == MPEG2_STYLE) m_headerSerialBuff.PutBit(0,9);  // clock ref extension
	m_headerSerialBuff.PutBit(1,1);   // marker bit
	m_headerSerialBuff.PutBit((int) m_bitRate/400.0f,22);  // Mux rate of the pack
	m_headerSerialBuff.PutBit(1,1);   // marker bit
	if(m_containerFMT == MPEG2_STYLE)
	{
		m_headerSerialBuff.PutBit(1,1);   // marker bit
		m_headerSerialBuff.PutBit(0x1f,5); // reserved
		m_headerSerialBuff.PutBit(0,3);    // stuffing length
	}

}


////////////////////////////////////////////////////////////////////////////////////////
// Creates a pes pack system header in the header serial buffer
// See ISO/IEC 13818-1  Section 2.4.3.6 for field definitions for PES headers
//
//  Input parameters : none
//
//	Output : none
//
//
void CAVCompression::PutPesPackSysHdr(void)
{

	m_headerSerialBuff.PutBit(0,16);
	m_headerSerialBuff.PutBit(PACK_HDR_START_CODE,16);
	m_headerSerialBuff.PutBit(12,16);		// Header length after this length field in bytes
	m_headerSerialBuff.PutBit(1,1);			// marker bit
	m_headerSerialBuff.PutBit((int) m_bitRate/400.0f,22);  // Mux rate bound
	m_headerSerialBuff.PutBit(1,1);			// marker bit
	m_headerSerialBuff.PutBit(1,6);			// Number of active audio streams
	m_headerSerialBuff.PutBit(0,1);			// may be a variable rate stream
	m_headerSerialBuff.PutBit(0,1);			// non constrained bitstream
	m_headerSerialBuff.PutBit(1,1);			// audio locked
	m_headerSerialBuff.PutBit(1,1);			// video locked
	m_headerSerialBuff.PutBit(1,1);			// marker bit
	m_headerSerialBuff.PutBit(1,5);			// Number of active video streams
	m_headerSerialBuff.PutBit(0,1);			// Packet rate restriction flag
	m_headerSerialBuff.PutBit(0x7f, 7);		// Reserved bits
	m_headerSerialBuff.PutBit(0xB9, 8);		// Setup up video streams
	m_headerSerialBuff.PutBit(3,2);
	m_headerSerialBuff.PutBit(1,1);
	m_headerSerialBuff.PutBit(0x0300,13);	// Video buffer size
	m_headerSerialBuff.PutBit(0xB8, 8);		// Setup up audio streams
	m_headerSerialBuff.PutBit(3,2);
	m_headerSerialBuff.PutBit(0,1);
	m_headerSerialBuff.PutBit(0x0040,13);	// Audio buffer size 
}

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
unsigned int CAVCompression::PutPesElemStreamHdr(unsigned int pesPktSize,int streamID,CAVCSerialBuffer *SerialStore)
{

	assert(SerialStore);

	unsigned int startloc = SerialStore->GetBytes();
	int64 PCR_BASE = ((int64) (m_systemClockMPEG/300.0f));

	SerialStore->PutBit(1,24);
	SerialStore->PutBit(streamID,8);
	if(m_containerFMT == MPEG1_STYLE) 
	{
		SerialStore->PutBit(pesPktSize+7,16);  // PES packet length
		SerialStore->PutBit(1,2);    // padding bits
		SerialStore->PutBit(1,1);    // P-STD buffer scale
		SerialStore->PutBit(1024,13);    // P-STD buffer size
	}
	else
	{
		SerialStore->PutBit(pesPktSize+8,16);  // PES packet length
		SerialStore->PutBit(2,2);    // padding bits
		SerialStore->PutBit(0,2);    // PES_scrambling_control
		SerialStore->PutBit(0,1);    // PES_priority
		SerialStore->PutBit(0,1);    // data_alignment_indicator
		SerialStore->PutBit(0,1);    // copyright
		SerialStore->PutBit(1,1);    // original_or_copy
		SerialStore->PutBit(2,2);    // PTS_DTS_flags
		SerialStore->PutBit(0,1);    // ESCR_flag
		SerialStore->PutBit(0,1);    // ES_rate_flag
		SerialStore->PutBit(0,1);    // DSM_trick_mode_flag
		SerialStore->PutBit(0,1);    // additional_copy_info_flag
		SerialStore->PutBit(0,1);    // PES_CRC_flag
		SerialStore->PutBit(0,1);    // PES_extension_flag
		SerialStore->PutBit(5,8);    // PES_header_data_length
	}
	SerialStore->PutBit(2,4);    
	SerialStore->PutBit((INT)((PCR_BASE >> 30) & 0x07),3);
	SerialStore->PutBit(1,1);  // marker bit
	SerialStore->PutBit((INT)((PCR_BASE >> 15) & 0x7fff),15);
	SerialStore->PutBit(1,1);   // marker bit
	SerialStore->PutBit((INT)(PCR_BASE & 0x7fff),15);
	SerialStore->PutBit(1,1);   // marker bit

	return(startloc);


}


////////////////////////////////////////////////////////////////////////////////////////
// Creates an MPEG video sequence header
// See ISO/IEC 13818-2  Section 6.2.2 for field definitions
//
//  Input parameters : none
//
//	Output : none
//
//
void CAVCompression::PutSeqHdr(void)
{

	m_headerSerialBuff.PutBit(0,16); 
	m_headerSerialBuff.PutBit(SEQ_START_CODE,16); 
	m_headerSerialBuff.PutBit(m_encodedWidth,12); 
	m_headerSerialBuff.PutBit(m_encodedHeight,12); 
	m_headerSerialBuff.PutBit(1,4); // 1 = square pixels, 2 = 4:3, 3 = 16:9,4 = 2.21:1
	m_headerSerialBuff.PutBit(m_frameRateCode,4); // bitrate code
	m_headerSerialBuff.PutBit((int) ceil(m_bitRate/400.0f),18); // bitrate
	m_headerSerialBuff.PutBit(1,1);  //marker bit
	m_headerSerialBuff.PutBit(112,10); // vbv buffer size
	m_headerSerialBuff.PutBit(0,1); // constrained params flag
	m_headerSerialBuff.PutBit(0,1);  // Use default quant table
	m_headerSerialBuff.PutBit(0,1);  // Use default non intra quant table

}


////////////////////////////////////////////////////////////////////////////////////////
// Creates an MPEG video sequence extension header
// See ISO/IEC 13818-2  Section 6.2.2 for field definitions
//
//  Input parameters : none
//
//	Output : none
//
//
void CAVCompression::PutSeqExtHdr(void)
{
	m_headerSerialBuff.PutBit(0,16); 
	m_headerSerialBuff.PutBit(EXT_START_CODE,16); 
	m_headerSerialBuff.PutBit(SEQ_ID,4);
	m_headerSerialBuff.PutBit(0x44,8); // Main profile, High level
	m_headerSerialBuff.PutBit(1,1); // progressive seq
	m_headerSerialBuff.PutBit(1,2);// 1 = 4:2:0, 2 = 4:2:2, 3 = 4:4:4
	m_headerSerialBuff.PutBit(m_encodedWidth >> 12,2); // horiz size ext
	m_headerSerialBuff.PutBit(m_encodedHeight >> 12,2); // vert size ext
	m_headerSerialBuff.PutBit(((int) ceil(m_bitRate/400.0f)) >> 18,12); // bit rate ext
	m_headerSerialBuff.PutBit(1,1); // marker bit
	m_headerSerialBuff.PutBit(0,8); // vbv buffer size ext
	m_headerSerialBuff.PutBit(0,1); // not low delay
	m_headerSerialBuff.PutBit(0,2); // frame rate ext n
	m_headerSerialBuff.PutBit(0,5); // frame rate ext d
}

////////////////////////////////////////////////////////////////////////////////////////
// Creates an MPEG video sequence display header
// See ISO/IEC 13818-2  Section 6.2.2 for field definitions
//
//  Input parameters : none
//
//	Output : none
//
//
void CAVCompression::PutSeqDispExtHdr(void)
{
	m_headerSerialBuff.PutBit(0,16); 
	m_headerSerialBuff.PutBit(EXT_START_CODE,16); 
	m_headerSerialBuff.PutBit(DISP_ID,4);
	m_headerSerialBuff.PutBit(2,3);
	m_headerSerialBuff.PutBit(1,1); 
	m_headerSerialBuff.PutBit(5,8); 
	m_headerSerialBuff.PutBit(5,8);
	m_headerSerialBuff.PutBit(4,8);
	m_headerSerialBuff.PutBit(m_displayWidth,14);	// display width
	m_headerSerialBuff.PutBit(1,1);					//marker bit 
	m_headerSerialBuff.PutBit(m_displayHeight,14);  // display height
	m_headerSerialBuff.PutBit(0,3);					// padding to make it a multiple of 8 bits

}

////////////////////////////////////////////////////////////////////////////////////////
// Creates an MPEG video GOP header
// See ISO/IEC 13818-2  Section 6.2.2 for field definitions
//
//  Input parameters : none
//
//	Output : none
//
//
void CAVCompression::PutGOPHdr(void)
{

	assert(m_frameRate);


	float currentFrameTime = m_frameIndex/m_frameRate;  // Elapsed time in seconds

	unsigned int hrs = ((int) (currentFrameTime/3600.0f));
	currentFrameTime -= hrs*3600.0f;
	unsigned int mins = ((int) (currentFrameTime/60.0));
	currentFrameTime -= mins*60.0f;
	unsigned int secs = (int) currentFrameTime;
	currentFrameTime -= secs;
	unsigned int picture = (int) (currentFrameTime* m_frameRate);


	// Calculate hrs, mins, secs
	m_headerSerialBuff.PutBit(0,16);
	m_headerSerialBuff.PutBit(GOP_START_CODE,16);
	m_headerSerialBuff.PutBit(0,1);				// drop frame flag
	m_headerSerialBuff.PutBit(hrs%24,5);		// hours
	m_headerSerialBuff.PutBit(mins,6);			// minutes
	m_headerSerialBuff.PutBit(1,1);				// marker bit
	m_headerSerialBuff.PutBit(secs,6);			// seconds 
	m_headerSerialBuff.PutBit(picture,6);		// frames
	m_headerSerialBuff.PutBit(1,1);				// closed_gop
	m_headerSerialBuff.PutBit(0,1);				// broken_link
	m_headerSerialBuff.PutBit(0,5);				// filler bits


}


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
void CAVCompression::PutPictHdr(int frameType)
{

	m_headerSerialBuff.PutBit(0,16);
	m_headerSerialBuff.PutBit(PICTURE_START_CODE,16);
	m_headerSerialBuff.PutBit(m_frameIndex,10); // Incrementing frame counter

	switch(frameType)
	{
		case IFRAMETYPE		:
		case IREFFRAMETYPE  :	m_headerSerialBuff.PutBit(1,3);  // I frame
								break;
		case PFRAMETYPE		:
		case PREFFRAMETYPE  :	m_headerSerialBuff.PutBit(2,3);  // P frame
								break;

		case BFRAMETYPE		:	m_headerSerialBuff.PutBit(3,3); // B frame
								break;

	}

	m_headerSerialBuff.PutBit(0xFFFF,16);		// constant 

	switch(frameType)
	{
		case IFRAMETYPE		:
		case IREFFRAMETYPE  :	m_headerSerialBuff.PutBit(0,1);	// no extra info
								m_headerSerialBuff.PutBit(0,2);	// padding to make it align on byte boundary
								break;

		case PFRAMETYPE		:
		case PREFFRAMETYPE  :	m_headerSerialBuff.PutBit(0,1); // 1 = full pel, 0 = half pel accuracy for mpeg 1, should be 0 for mpeg 2
								if(m_videoCodecType == MPEG1_STYLE) m_headerSerialBuff.PutBit(DEFAULTFCODE,3); // forward range for mpeg 1
								else m_headerSerialBuff.PutBit(7,3);
								m_headerSerialBuff.PutBit(0,1);	// no extra info
								m_headerSerialBuff.PutBit(0,6);	// padding to make it align on byte boundary
								break;

		case BFRAMETYPE		:	m_headerSerialBuff.PutBit(0,1); // forward 1 = full pel, 0 = half pel accuracy for mpeg 1, should be 0 for mpeg 2
								if(m_videoCodecType == MPEG1_STYLE) m_headerSerialBuff.PutBit(DEFAULTFCODE,3); // forward range for mpeg 1
								else m_headerSerialBuff.PutBit(7,3);
								m_headerSerialBuff.PutBit(0,1); // backward 1 = full pel, 0 = half pel accuracy for mpeg 1, should be 0 for mpeg 2
								if(m_videoCodecType == MPEG1_STYLE) m_headerSerialBuff.PutBit(DEFAULTFCODE,3); //backward range for mpeg 1
								else m_headerSerialBuff.PutBit(7,3);
								m_headerSerialBuff.PutBit(0,1);	// no extra info
								m_headerSerialBuff.PutBit(0,2);	// padding to make it align on byte boundary
								break;

	}

}

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
void CAVCompression::PutPictCodExtHdr(int frameType)
{
	m_headerSerialBuff.PutBit(0,16);
	m_headerSerialBuff.PutBit(EXT_START_CODE,16);
	m_headerSerialBuff.PutBit(CODING_ID,4);
	if(frameType == IFRAMETYPE || frameType == IREFFRAMETYPE)
	{
		m_headerSerialBuff.PutBit(0x0F,4); // forward_horizontal_f_code not used since I frame only
		m_headerSerialBuff.PutBit(0x0F,4); // forward_vertical_f_code not used since I frame only
		m_headerSerialBuff.PutBit(0x0F,4); // backward_horizontal_f_code not used since I frame only
		m_headerSerialBuff.PutBit(0x0F,4); // backward_vertical_f_code not used since I frame only
	}
	if(frameType == PFRAMETYPE || frameType == PREFFRAMETYPE)
	{
		m_headerSerialBuff.PutBit(0x09,4); // set max resolution for forward
		m_headerSerialBuff.PutBit(0x09,4); // set max resolution for forward
		m_headerSerialBuff.PutBit(0x0F,4); // backward_horizontal_f_code not used since I frame only
		m_headerSerialBuff.PutBit(0x0F,4); // backward_vertical_f_code not used since I frame only
	}
	if(frameType == BFRAMETYPE)
	{
		m_headerSerialBuff.PutBit(0x09,4); // set max resolution for forward
		m_headerSerialBuff.PutBit(0x09,4); // set max resolution for forward
		m_headerSerialBuff.PutBit(0x09,4); // set max resolution for backward
		m_headerSerialBuff.PutBit(0x09,4); // set max resolution for backward
	}
	m_headerSerialBuff.PutBit(m_intraDcPred,2); // 0 = 8 bit, 1 = 9 bits, 2 = 10 bits, 3 = 11 bits (intra_dc_precision)
	m_headerSerialBuff.PutBit(3,2); // frame pictures only
	m_headerSerialBuff.PutBit(0,1); // output progressive frame
	m_headerSerialBuff.PutBit(1,1); // Only use frame dct (no field dct)
	m_headerSerialBuff.PutBit(0,1); // no concealment motion vectors
	m_headerSerialBuff.PutBit(m_qScaleType,1); // q scale type
	m_headerSerialBuff.PutBit(m_intraVLCFormat,1); // intravlc
	m_headerSerialBuff.PutBit(0,1); // scan format = normal
	m_headerSerialBuff.PutBit(0,1);  // prog frame only
	m_headerSerialBuff.PutBit(1,1); // prog frame only
	m_headerSerialBuff.PutBit(1,1); // prog frame only
	m_headerSerialBuff.PutBit(0,1); // no composite info
	m_headerSerialBuff.PutBit(0,6); // padding to make integer num of bytes

}




////////////////////////////////////////////////////////////////////////////////////////
// Convert the input frame into the proper size and format
//
//  Input parameters : none
//
//	Output : none
//
//
void CAVCompression::PreProcessFrame(void)
{


	// Add black bars to the input frame if needed to match the aspect ratio of the requested display frame
	AddBlackBars(m_pimageBuffAligned[0],m_pimageBuffAligned[1]);

	// Reduce the horizontal size of the input frame as much as possible
	// This is done by repeatedly filtering the frame in the horizontal direction
	while(m_inputWidth > m_displayWidth*1.75f)
	{  
		HalfbandFilterHoriz(m_pimageBuffAligned[1],m_inputWidth,m_inputHeight);
		m_inputWidth  = m_inputWidth >> 1;
	}

	// Reduce the vertical size of the input frame as much as possible
	// This is done by repeatedly filtering the frame in the vertical direction
	while(m_inputHeight > m_displayHeight*1.75f) 
	{  
		HalfbandFilterVert(m_pimageBuffAligned[1],m_inputWidth,m_inputHeight);
		m_inputHeight  = m_inputHeight >> 1;
	}


	// Resize the frame to match the requested display frame size
	ResizeFrame();

}





////////////////////////////////////////////////////////////////////////////////////////
// Resize a frame from the input size to the encoded size
// This is done by decimating/interpolating the input frame and then adding some padding
//
//  Input parameters : none
//
//	Output : none
//
void CAVCompression::ResizeFrame(void)
{


	assert(m_displayWidth);
	assert(m_displayHeight);


	// Calculate any padding that is needed to make the frame size equal to the encoded frame size
	unsigned int rightpadding =  m_encodedWidth - m_displayWidth;
	unsigned int vertpadding = m_encodedHeight - m_displayHeight;
	uint8 *addrout = m_pimageBuffAligned[0];
	uint8 *addrin = m_pimageBuffAligned[1];
	uint8 mpegDataMask = 0xFF;
	if(m_videoCodecType == MPEG1_STYLE) mpegDataMask = 0x00;



	// Check to see if we need to do any interpolation/decimation
	if(m_inputWidth != m_displayWidth  || m_inputHeight != m_displayHeight) 
	{
		// Need some interpolation/decimation

		// Calculate the size ratios to use in resizing the frame
		float horiz_delta = m_inputWidth/(float) m_displayWidth;
		float vert_delta = m_inputHeight/(float) m_displayHeight;

		// Change the size of the input frame to the display size
		float cury = 0.0f;
		for(unsigned int i=0;i<m_displayHeight;++i) 
		{
			int locy = (int) cury;
			int offsety = locy*m_inputWidth;
			float curx = 0.0f;
			for(unsigned int j=0;j<m_displayWidth;++j) 
			{
				int locx = (int) curx;
				addrin = m_pimageBuffAligned[1] + (offsety + locx) * RGBASIZE;

#ifdef AVIBILINEAR
				// Do a bilinear interpolation
				float x1 = curx - locx;
				float x0 = 1.0f - x1;
				float y1 = cury - locy;
				float y0 = 1.0f - y1;

				// Do calc for Blue
				float val0 = (float) addrin[0];
				float val1 = (float) addrin[4];
				float val2 = (float) addrin[m_inputWidth*RGBASIZE];
				float val3 = (float) addrin[m_inputWidth*RGBASIZE+RGBASIZE];
				float total = val0*x0*y0 + val1*x1*y0 + val2*x0*y1 + val2*x1*y1;
				addrout[0] = (unsigned char) (total + 0.5f);

				// Do calc for Green
				val0 = (float) addrin[1];
				val1 = (float) addrin[5];
				val2 = (float) addrin[m_inputWidth*RGBASIZE+1];
				val3 = (float) addrin[m_inputWidth*RGBASIZE+5];
				total = val0*x0*y0 + val1*x1*y0 + val2*x0*y1 + val2*x1*y1;
				addrout[1] = (unsigned char) (total + 0.5f);


				// Do calc for Red
				val0 = (float) addrin[2];
				val1 = (float) addrin[6];
				val2 = (float) addrin[m_inputWidth*RGBASIZE+2];
				val3 = (float) addrin[m_inputWidth*RGBASIZE+6];
				total = val0*x0*y0 + val1*x1*y0 + val2*x0*y1 + val2*x1*y1;
				addrout[2] = (unsigned char) (total + 0.5f);
#else
				// Simple subsampling
				addrout[0] = addrin[0];
				addrout[1] = addrin[1];
				addrout[2] = addrin[2];
				addrout[3] = addrin[3];

#endif

				addrout += RGBASIZE;
				curx += horiz_delta;
			}

			// Add any padding to the right side that is needed
			for(unsigned int k=0;k<rightpadding;++k) 
			{
				addrout[0] = addrin[0] & mpegDataMask;
				addrout[1] = addrin[1] & mpegDataMask;
				addrout[2] = addrin[2] & mpegDataMask;
				addrout[3] = addrin[3] & mpegDataMask;
				addrout += RGBASIZE;
			}

			cury += vert_delta;
		}

	}
	else 
	{
		// Resizing is not needed, only padding may be needed
		for(unsigned int i=0;i<m_displayHeight;++i) 
		{
			CopyMemory(addrout,addrin,RGBASIZE*m_displayWidth);
			addrout += RGBASIZE*m_displayWidth;
			addrin += RGBASIZE*m_displayWidth;

			for(unsigned int j=0;j<rightpadding;++j)
			{
				addrout[0] = addrin[-4] & mpegDataMask;
				addrout[1] = addrin[-3] & mpegDataMask;
				addrout[2] = addrin[-2] & mpegDataMask;
				addrout[3] = addrin[-1] & mpegDataMask;
				addrout += RGBASIZE;
			}
		}

	}

	// Pad the bottom if required
	if(vertpadding > 0) 
	{

		addrin = addrout - RGBASIZE*m_encodedWidth;  // Use the last line as the padding
		for(unsigned int k=0;k<vertpadding;++k)
		{
			if(m_videoCodecType == MPEG1_STYLE) ZeroMemory(addrout,RGBASIZE*m_encodedWidth);
			else CopyMemory(addrout,addrin,RGBASIZE*m_encodedWidth);
			addrout += RGBASIZE*m_encodedWidth;

		}
	}

	// Adjust the size to the new frame size
	m_inputWidth = m_encodedWidth;
	m_inputHeight = m_encodedHeight;


}


////////////////////////////////////////////////////////////////////////////////////////
// Perform a half band filter in the horizontal direction on an RGBA frame
// The filter is done in place
//  Input parameters :
//		pDin	: pointer to frame data
//		width	: width in pixels of the input frame
//		height	: height in pixels of the input frame
//
//	Output : None
//
//
void CAVCompression::HalfbandFilterHoriz(uint8 *pDin,int width,int height)
{

	assert(pDin);


	// Very simple in place filter, could be done much faster using SSE
	// 3 tap filter (0.5,1.0,0.5)
	uint8 *pdataout = pDin;
	for(unsigned int j=0;j<height;++j) 
	{
		uint8 *pdatain = pDin + j*width*RGBASIZE;
		unsigned int index0 = 0;
		unsigned int index1 = RGBASIZE;
		unsigned int index2 = 2*RGBASIZE;
		for(int i=0;i<width;i+=2) 
		{
			unsigned int sum0 = ((unsigned int) pdatain[index0] + 2*(unsigned int) pdatain[index1] + (unsigned int) pdatain[index2] + 2) >> 2;
			++index0;
			++index1;
			++index2;
			unsigned int sum1 = ((unsigned int) pdatain[index0] + 2*(unsigned int) pdatain[index1] + (unsigned int) pdatain[index2] + 2) >> 2;
			++index0;
			++index1;
			++index2;
			unsigned int sum2 = ((unsigned int) pdatain[index0] + 2*(unsigned int) pdatain[index1] + (unsigned int) pdatain[index2] + 2) >> 2;
			index0 += RGBASIZE + 2;  // Skip over the next pixel
			index1 += RGBASIZE + 2;
			index2 += RGBASIZE + 2;

			pdataout[0] = (uint8) sum0;
			pdataout[1] = (uint8) sum1;
			pdataout[2] = (uint8) sum2;
			pdataout += RGBASIZE;

		}
	}


}


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
void CAVCompression::HalfbandFilterVert(uint8 *pDin,int width,int height)
{

	assert(pDin);

	// Very simple in place filter
	// 3 tap filter (0.5,1.0,0.5)
	uint8 *pdataout = pDin;
	for(unsigned int j=0;j<height;j+=2) 
	{
		uint8 *pdatain = pDin + j*width*RGBASIZE;
		unsigned int index0 = 0;
		unsigned int index1 = width*RGBASIZE;
		unsigned int index2 = width*RGBASIZE*2;
		for(unsigned int i=0;i<width;++i) 
		{
			unsigned int sum0 = ((unsigned int) pdatain[index0] + 2*(unsigned int) pdatain[index1] + (unsigned int) pdatain[index2] + 2) >> 2;
			++index0;
			++index1;
			++index2;
			unsigned int sum1 = ((unsigned int) pdatain[index0] + 2*(unsigned int) pdatain[index1] + (unsigned int) pdatain[index2] + 2) >> 2;
			++index0;
			++index1;
			++index2;
			unsigned int sum2 = ((unsigned int) pdatain[index0] + 2*(unsigned int) pdatain[index1] + (unsigned int) pdatain[index2] + 2) >> 2;
			index0 += 2;
			index1 += 2;
			index2 += 2;

			pdataout[0] = (uint8) sum0;
			pdataout[1] = (uint8) sum1;
			pdataout[2] = (uint8) sum2;
			pdataout += RGBASIZE;

		}
	}


}



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
void CAVCompression::AddBlackBars(uint8 *pSsource, uint8 *pDest)
{

	assert(m_inputHeight);
	assert(m_displayHeight);
	assert(m_displayWidth);
	assert(pSsource);
	assert(pDest);


	uint8 *sourceaddr = pSsource;
	uint8 *destaddr = pDest;

	// Black bands may need to be added to the input frame so that its aspect ratio matches the display frame size
	// See if the top/bottom or left/right need black banding
	if((m_inputWidth/(float) m_inputHeight) - (m_displayWidth/(float)m_displayHeight) > 0) 
	{
		// Need to add black bars to the top and bottom
		unsigned int paddingV = (((unsigned int) (m_displayHeight * m_inputWidth/(float) m_displayWidth)) - m_inputHeight);

		unsigned int BBVertTopSize = paddingV >> 1;
		unsigned int BBVertBottomSize = paddingV - BBVertTopSize;

		// Add a black bar at the top
		unsigned int blocksize = RGBASIZE*m_inputWidth*BBVertTopSize;
		ZeroMemory(destaddr,blocksize);
		destaddr += blocksize;

		// Copy the input to the center
		blocksize = RGBASIZE*m_inputWidth*m_inputHeight;
		CopyMemory(destaddr,sourceaddr,blocksize);
		destaddr += blocksize;

		// Add a black bar at the bottom
		blocksize = RGBASIZE*m_inputWidth*BBVertBottomSize;
		ZeroMemory(destaddr,blocksize);

		// Adjust the input size to account for the black bars
		m_inputHeight += BBVertTopSize + BBVertBottomSize;


	} 
	else 
	{
		// Need to add black bars to the sides
		unsigned int paddingH = (((unsigned int) (m_displayWidth * m_inputHeight/(float) m_displayHeight)) - m_inputWidth);

		unsigned int BBHorizLeftSize = paddingH >> 1;
		unsigned int BBHorizRightSize = paddingH - BBHorizLeftSize;

		int stride = m_inputWidth*RGBASIZE;
		for(int i=0;i<m_inputHeight;i++) 
		{
			// Pad on the left
			ZeroMemory(destaddr,BBHorizLeftSize*RGBASIZE);
			destaddr += BBHorizLeftSize*RGBASIZE;

			// Copy center
			CopyMemory(destaddr,sourceaddr,stride);
			destaddr += stride;
			sourceaddr += stride;

			// Pad on the right
			ZeroMemory(destaddr,BBHorizRightSize*RGBASIZE);
			destaddr += BBHorizRightSize*RGBASIZE;
		} 

		// Adjust the input size to account for the black bars
		m_inputWidth += BBHorizLeftSize + BBHorizRightSize;

	}


}



////////////////////////////////////////////////////////////////////////////////////////
// Calculate the largest space that is needed to hold a video frame 
//
//  Input parameters : none
//
//	Output : Returns the needed size in bytes
//
//
unsigned int CAVCompression::FindMaxFrameSize(void)
{

	assert(m_inputHeight);
	assert(m_displayHeight);
	assert(m_displayWidth);

	// Calculate the size of any black bars that will be needed for aspect ratio adjustment for an input frame
	unsigned int vertpadding = 0;
	unsigned int horizpadding = 0;
	if((m_inputWidth/(float) m_inputHeight) - (m_displayWidth/(float)m_displayHeight) > 0) 
	{
		vertpadding = (((int) (m_displayHeight * m_inputWidth/(float) m_displayWidth)) - m_inputHeight);
	} 
	else 
	{
		horizpadding = (((int) (m_displayWidth * m_inputHeight/(float) m_displayHeight)) - m_inputWidth);
	}


	// Calculate the size of a padded input frame
	unsigned int inputwidth = m_inputWidth + horizpadding;
	unsigned int inputheight = m_inputHeight + vertpadding;

	unsigned int paddedinputframesize = RGBASIZE * inputwidth * inputheight;
	unsigned int encodedframesize = RGBASIZE * m_encodedHeight * m_encodedWidth;

	// Pick the larger of the padded input size, or the encoded size
	if(paddedinputframesize > encodedframesize) return(paddedinputframesize);
	else return(encodedframesize);

}


////////////////////////////////////////////////////////////////////////////////////////
// Finds the best mpeg framerate codes to use for a given framerate 
//
//  Input parameters :
//		framerate : the framerate to check
//
//	Output : none
//
//
void CAVCompression::FindFrameRateCode(float framerate)
{

	// Init to one of the possibilities
	m_frameRateCode = 1;
	float bestframerateerror = fabs(frameratetable[m_frameRateCode-1] - framerate);

	// Check all of the possibilities for the best match
	for(unsigned int i=0;i<8;++i) 
	{
		float framerateerror = fabs(frameratetable[i] - framerate);
		if(framerateerror < bestframerateerror) 
		{
			bestframerateerror = framerateerror;
			m_frameRateCode = i+1;
			m_frameRate = frameratetable[i];
		}
	}
}

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
int CAVCompression::TranscodeAVFileFull(void)
{

	unsigned int sectionSize[MAX_COMP_THREADS];
	unsigned int numRows[MAX_COMP_THREADS];
	uint8 *sectionAddr[MAX_COMP_THREADS];

	CAVCSerialBuffer hdrSerialBuff; 
	uint8 hdrbuff[24+MAX_COMP_THREADS*8];
	size_t currentFileLoc = 0;
	unsigned int fileHeaderSize = 20;

	// Set up the reference frames
	m_currentRefFrame = m_pimageBuffAligned[2];
	m_nextRefFrame = m_pimageBuffAligned[3];

	m_gopLength = GOPLENGTH;  // For re-encoding switch to I/P frames

	// Read the file header
	hdrSerialBuff.Init(hdrbuff,fileHeaderSize,SERIALREAD);
	int bytesRead = m_pAVCFileManager->ReadFromLocation(hdrbuff,currentFileLoc,fileHeaderSize,1);

	// Check for the sync word
	unsigned int filesync = hdrSerialBuff.SerialRead(32);
	if(filesync != FILEHDRSYNC)  return(AVCOMP_TRANSCODE);

	// Read the file parameters
	unsigned int headerSize = hdrSerialBuff.SerialRead(16);
	m_numFramesInFile = hdrSerialBuff.SerialRead(32);
	m_totalFramesInFile = m_numFramesInFile;
	m_encodedWidth = hdrSerialBuff.SerialRead(16);
	m_encodedHeight = hdrSerialBuff.SerialRead(16);
	m_displayWidth = hdrSerialBuff.SerialRead(16);
	m_displayHeight = hdrSerialBuff.SerialRead(16);

	m_frameRateCode = hdrSerialBuff.SerialRead(4);
	if(m_frameRateCode < 1 || m_frameRateCode > 8)  return(AVCOMP_TRANSCODE); // bad file

	m_frameRate = frameratetable[m_frameRateCode-1];
	m_InitParams.recordMode = hdrSerialBuff.SerialRead(4);
	m_InitParams.recordQuality = hdrSerialBuff.SerialRead(8);

	// Initialize the rate control
	InitRateControl(m_InitParams.recordQuality,1);

	// Output an AVI header if needed
	CreateFileHeader();

	// Read the frames, decode them, and then re-encode them
	size_t frameloc = m_pAVCFileManager->GetStartFrameLocation(headerSize);
	m_frameIndex=0;
	while(m_numFramesInFile > 0)
	{

		// Check if we need to abort before processing the next frame
		int state = GetState();
		if(state != AVTHREAD_TRANSCODINGFULL) break;

		// Read a frame header
		currentFileLoc = frameloc;
		hdrSerialBuff.Init(hdrbuff,24+MAX_COMP_THREADS*8,SERIALREAD);
		m_pAVCFileManager->ReadFromLocation(hdrbuff,currentFileLoc,24+MAX_COMP_THREADS*8,1);

		unsigned int syncword = hdrSerialBuff.SerialRead(32);
		if(syncword != FILEFRAMESYNC)  return(AVCOMP_TRANSCODE);

		// Read the frame parameters
		unsigned int headerSize = hdrSerialBuff.SerialRead(16);
		unsigned int videoSections = hdrSerialBuff.SerialRead(16);
		unsigned int encodedVideoBytes = hdrSerialBuff.SerialRead(32);
		unsigned int encodedAudioBytes = hdrSerialBuff.SerialRead(32);
		unsigned int numOutputFrames = hdrSerialBuff.SerialRead(16);
		m_frameQuant = hdrSerialBuff.SerialRead(16);  // quantization value for the frame
		unsigned int frameIndex = hdrSerialBuff.SerialRead(32);

		// Read the section info for this frame
		unsigned int frameOffset = 0;
		for(unsigned int i=0;i<videoSections;++i)
		{
			sectionSize[i] = hdrSerialBuff.SerialRead(32);  
			numRows[i] = hdrSerialBuff.SerialRead(32);
			sectionAddr[i] = m_pimageBuffAligned[0] + frameOffset;
			frameOffset += sectionSize[i];
		}
		currentFileLoc += headerSize;


		// Read the actual video encoded data into a buffer
		int readBytes = m_pAVCFileManager->ReadFromLocation(m_pimageBuffAligned[0],currentFileLoc,encodedVideoBytes,1);
		if(readBytes != encodedVideoBytes) return(AVCOMP_TRANSCODE);
		currentFileLoc += encodedVideoBytes;


		// Output any headers that are needed for the audio and output the audio
		int frameBytes = 0;
		if(encodedAudioBytes > 0)
		{
			m_pAVCFileManager->ReadFromLocation(m_pimageBuffAligned[1],currentFileLoc,encodedAudioBytes,1);
			m_pAVCFileManager->Write(m_pimageBuffAligned[1],encodedAudioBytes,0);   
			frameBytes += encodedAudioBytes;
		}


		// Decode the video frame
		unsigned int status = AVDecodeVideoFrame(m_pimageBuffAligned[1],sectionAddr,sectionSize,numRows);
		if(status  != AVCOMP_OK) return(status);


		//  Output the frame 1 or more times
		for(unsigned int j=0;j<numOutputFrames;++j) 
		{
			int endOfGopFlag;

			// Get the frame type for this frame
			int frameType = GetFrameType(&endOfGopFlag);

			// Re-encode the video frame
			unsigned int reEncodedVideoBytes = AVencodeVideoFrame(AVCYUV,frameType,m_currentRefFrame,m_nextRefFrame);

			// Output any headers that are needed for the video and output the video
			frameBytes += CreateFrameHeader(ISVIDEO,reEncodedVideoBytes,frameType);
			for(unsigned int i=0;i<m_numVideoThreads;++i) m_pAVCFileManager->Write(m_pvideoCoders[i]->GetBufferAddr(),m_pvideoCoders[i]->GetEncodedBytes(),0); 
			frameBytes += reEncodedVideoBytes;

			// update frame counter
			m_frameIndex++;
			if(m_InitParams.recordMode == AVCAPTURELASTXXX) --m_numFramesInFile;

			// Update the system clock value
			m_systemClockMPEG  += VIDEOCLOCKRATE/m_frameRate;

			// Update the rate control with the number of bytes used in the frame
			UpdateRateControl(frameBytes,endOfGopFlag);
			frameBytes = 0;

			// flip the current and new reference frames
			uint8 *pTmp = m_currentRefFrame;
			m_currentRefFrame = m_nextRefFrame;
			m_nextRefFrame = pTmp;

		}

		// Get the file location for the next frame in the sequence
		frameloc = m_pAVCFileManager->GetNextFrameLocation(frameloc);
		if(frameloc == 0) break;

		if(m_InitParams.recordMode != AVCAPTURELASTXXX) --m_numFramesInFile;

	}


	// Output any final headers
	if(m_containerFMT == MPEG1_STYLE || m_containerFMT == MPEG2_STYLE)
	{
		m_headerSerialBuff.Reset();
		PutPesElemStreamHdr(4,VIDEO_PES_ID,&m_headerSerialBuff);
		m_headerSerialBuff.PutBit(0,16);
		m_headerSerialBuff.PutBit(SEQ_END_CODE,16);
		m_headerSerialBuff.PutBit(0,16);  // This is outside of the pes packet
		m_headerSerialBuff.PutBit(MPEG_PROG_END_CODE,16);  // This is outside of the pes packet
		m_pAVCFileManager->Write(m_headerSerialBuff.GetBufferAddress(),m_headerSerialBuff.GetBytes(),0);  // Write out the buffer
	}

	if(m_containerFMT == AVI_STYLE)
	{

		// Close the MOVI list
		unsigned int numBytes = m_pAVCFileManager->OutputByteCount(0) - m_MOVIStart - 4;
		m_pAVCFileManager->WriteAtLocation((unsigned char *) &numBytes,m_MOVIStart,4,0);


		// Create the index for the AVI file
		CreateAVIIndx();

		// Close the AVI list
		numBytes = m_pAVCFileManager->OutputByteCount(0) - m_AVIStart - 4;
		m_pAVCFileManager->WriteAtLocation((unsigned char *) &numBytes,m_AVIStart,4,0);

		// Update the number of video frames
		m_pAVCFileManager->WriteAtLocation((unsigned char *) &m_frameIndex,m_totalFramesLoc,4,0);

		// Update the video stream length
		m_pAVCFileManager->WriteAtLocation((unsigned char *) &m_frameIndex,m_streamLenVideo,4,0);


		// Update the audio stream length
		unsigned int numAudioFrames = m_pAudioEncoder->GetEncodedBlocks();
		m_pAVCFileManager->WriteAtLocation((unsigned char *) &numAudioFrames,m_streamLenAudio,4,0);

	}

	return(AVCOMP_OK);
}


////////////////////////////////////////////////////////////////////////////////////////
// Converts the intermediate file into the final file by copying the already compressed
// data into an AVI file
//
//  Input parameters : none
//
//	Output : returns AVCOMP error if unsuccessful or AVCOMP_OK if successful
//
//
int CAVCompression::TranscodeAVFileFast(void)
{

	CAVCSerialBuffer hdrSerialBuff; 
	uint8 hdrbuff[24+MAX_COMP_THREADS*8];
	size_t currentFileLoc = 0;
	unsigned int fileHeaderSize = 20;


	// Read the file header
	hdrSerialBuff.Init(hdrbuff,fileHeaderSize,SERIALREAD);
	int bytesRead = m_pAVCFileManager->ReadFromLocation(hdrbuff,currentFileLoc,fileHeaderSize,1);

	// Check for the sync word
	unsigned int filesync = hdrSerialBuff.SerialRead(32);
	if(filesync != FILEHDRSYNC)  return(AVCOMP_TRANSCODE);

	// Read the file parameters
	unsigned int headerSize = hdrSerialBuff.SerialRead(16);
	m_numFramesInFile = hdrSerialBuff.SerialRead(32);
	m_totalFramesInFile = m_numFramesInFile;
	m_encodedWidth = hdrSerialBuff.SerialRead(16);
	m_encodedHeight = hdrSerialBuff.SerialRead(16);
	m_displayWidth = hdrSerialBuff.SerialRead(16);
	m_displayHeight = hdrSerialBuff.SerialRead(16);

	m_frameRateCode = hdrSerialBuff.SerialRead(4);
	if(m_frameRateCode < 1 || m_frameRateCode > 8)  return(AVCOMP_TRANSCODE); // bad file

	m_frameRate = frameratetable[m_frameRateCode-1];
	m_InitParams.recordMode = hdrSerialBuff.SerialRead(4);
	m_InitParams.recordQuality = hdrSerialBuff.SerialRead(8);

	// Get the location of the first frame in the sequence
	size_t frameloc = m_pAVCFileManager->GetStartFrameLocation(headerSize);
	if(frameloc == -1) return(AVCOMP_TRANSCODE); // bad file

	// Read the files from the intermediate file in sequence and
	// copy to the final file
	m_frameIndex=0;
	while(m_numFramesInFile > 0)
	{

		// Check if we need to abort or stop before processing the next frame
		int state = GetState();
		if(state != AVTHREAD_TRANSCODINGFAST) break;

		// Read a frame header
		currentFileLoc = frameloc;
		hdrSerialBuff.Init(hdrbuff,24+MAX_COMP_THREADS*8,SERIALREAD);
		m_pAVCFileManager->ReadFromLocation(hdrbuff,currentFileLoc,24+MAX_COMP_THREADS*8,1);

		unsigned int syncword = hdrSerialBuff.SerialRead(32);
		if(syncword != FILEFRAMESYNC)  return(AVCOMP_TRANSCODE);

		// Read the frame parameters
		unsigned int headerSize = hdrSerialBuff.SerialRead(16);
		unsigned int videoSections = hdrSerialBuff.SerialRead(16);
		unsigned int encodedVideoBytes = hdrSerialBuff.SerialRead(32);
		unsigned int encodedAudioBytes = hdrSerialBuff.SerialRead(32);
		unsigned int numOutputFrames = hdrSerialBuff.SerialRead(16);
		m_frameQuant = hdrSerialBuff.SerialRead(16);  // quantization value for the frame
		unsigned int frameIndex = hdrSerialBuff.SerialRead(32);
		currentFileLoc += headerSize;

		// Read the actual encoded data into a buffer
		int readBytes = m_pAVCFileManager->ReadFromLocation(m_pimageBuffAligned[0],currentFileLoc,encodedVideoBytes,1);
		if(readBytes != encodedVideoBytes) return(AVCOMP_TRANSCODE);
		currentFileLoc += encodedVideoBytes;

		// Read the encoded audio data into a buffer
		m_pAVCFileManager->ReadFromLocation(m_pimageBuffAligned[1],currentFileLoc,encodedAudioBytes,1);
		currentFileLoc += encodedAudioBytes;


		// Output one or more frames for each input frame
		unsigned int frameBytes=0;
		for(unsigned int j=0;j<numOutputFrames;++j) 
		{

			// Output any headers that are needed for the video and output the video
			frameBytes += CreateFrameHeader(ISVIDEO,encodedVideoBytes,IFRAMETYPE);
			m_pAVCFileManager->Write(m_pimageBuffAligned[0],encodedVideoBytes,0); 
			frameBytes += encodedVideoBytes;

			// Output any headers that are needed for the audio and output the audio
			if(encodedAudioBytes > 0) m_pAVCFileManager->Write(m_pimageBuffAligned[1],encodedAudioBytes,0);   
			frameBytes += encodedAudioBytes;
			encodedAudioBytes = 0;

			// update frame counter
			m_frameIndex++;
			if(m_InitParams.recordMode == AVCAPTURELASTXXX) --m_numFramesInFile;


			// Update the system clock value
			m_systemClockMPEG  += VIDEOCLOCKRATE/m_frameRate;

		}

		// Get the location for the next frame in the saved sequence
		frameloc = m_pAVCFileManager->GetNextFrameLocation(frameloc);
		if(frameloc == 0) break;
		if(m_InitParams.recordMode != AVCAPTURELASTXXX) --m_numFramesInFile;

	}


	// Output any final headers
	if(m_containerFMT == MPEG1_STYLE || m_containerFMT == MPEG2_STYLE)
	{
		m_headerSerialBuff.Reset();
		PutPesElemStreamHdr(4,VIDEO_PES_ID,&m_headerSerialBuff);
		m_headerSerialBuff.PutBit(0,16);
		m_headerSerialBuff.PutBit(SEQ_END_CODE,16);
		m_headerSerialBuff.PutBit(0,16);  // This is outside of the pes packet
		m_headerSerialBuff.PutBit(MPEG_PROG_END_CODE,16);  // This is outside of the pes packet
		m_pAVCFileManager->Write(m_headerSerialBuff.GetBufferAddress(),m_headerSerialBuff.GetBytes(),0);  // Write out the buffer
	}

	if(m_containerFMT == AVI_STYLE)
	{

		// Close the MOVI list
		unsigned int numBytes = m_pAVCFileManager->OutputByteCount(0) - m_MOVIStart - 4;
		m_pAVCFileManager->WriteAtLocation((unsigned char *) &numBytes,m_MOVIStart,4,0);

		// Create the index for the AVI file
		CreateAVIIndx();

		// Close the AVI list
		numBytes = m_pAVCFileManager->OutputByteCount(0) - m_AVIStart - 4;
		m_pAVCFileManager->WriteAtLocation((unsigned char *) &numBytes,m_AVIStart,4,0);

		// Update the number of video frames
		m_pAVCFileManager->WriteAtLocation((unsigned char *) &m_frameIndex,m_totalFramesLoc,4,0);

		// Update the video stream length
		m_pAVCFileManager->WriteAtLocation((unsigned char *) &m_frameIndex,m_streamLenVideo,4,0);

		// Update the audio stream length
		unsigned int numAudioFrames = m_pAudioEncoder->GetEncodedBlocks();
		m_pAVCFileManager->WriteAtLocation((unsigned char *) &numAudioFrames,m_streamLenAudio,4,0);

	}

	return(AVCOMP_OK);
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Calculates the progress in re-encoding a file
//
//  Input parameters :  none
//
//	Output : returns a percentage complete value
//
//
float CAVCompression::TranscodeProgress(void)
{

	float percentComplete = 0.0f;
	int state = GetState();
	if(state == AVTHREAD_TRANSCODINGFAST || state == AVTHREAD_TRANSCODINGFULL)
	{
		if(m_totalFramesInFile > 0) {
			percentComplete = 100.0f * (m_totalFramesInFile - m_numFramesInFile)/(float) m_totalFramesInFile;
			percentComplete = CLAMP(percentComplete,0.0f,100.0f);
		}
	}

	return(percentComplete);

}

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
unsigned int CAVCompression::AVDecodeVideoFrame(uint8 *frameBuffer,uint8 **sectionAddr,unsigned int *sectionSize,unsigned int *numRows)
{

	assert(frameBuffer);
	assert(sectionAddr);
	assert(sectionSize);


	// The frame sections are assigned to threads for decoding 
	HANDLE hThread[MAX_COMP_THREADS];
	for(unsigned int i=0;i<m_numVideoThreads;++i) 
	{
		// Initialize the video decoders
		m_pvideoCoders[i]->InitVideoDeCompression(frameBuffer,sectionAddr[i],sectionSize[i],numRows[i],m_encodedWidth,m_encodedHeight,
			m_mQuant,m_videoCodecType);
	}

	// Start the decoders
	for(unsigned int i=0;i<m_numVideoThreads;++i) 
	{
		m_pvideoCoders[i]->Start();
		hThread[i] = (HANDLE) m_pvideoCoders[i]->GetHandle();
		if (hThread[i] == 0) {
			WaitForMultipleObjects(i,hThread,TRUE, INFINITE);
			return(AVCOMP_DECODE);
		}
	}

	// Wait for the decoders to finish
	WaitForMultipleObjects(m_numVideoThreads,hThread,TRUE, INFINITE);

	return(AVCOMP_OK);

}

void CAVCompression::Shutdown()
{
	SetState(AVTHREAD_SHUTDOWN);
	WaitForThread();
	Stop();
}
