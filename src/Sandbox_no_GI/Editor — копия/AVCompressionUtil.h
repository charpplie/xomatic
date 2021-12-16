////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Header File.
//  Copyright (C), Crytek Studios, 2010.
// -------------------------------------------------------------------------
//  File name:   AVCompressionUtil.h
//  Version:     v1.00
//  Created:     28/04/2010 by John Stewart.
//  Compilers:   
//  Description: Audio/Video Compression  Utilities class headers
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#if !defined __AVCOMPRESSIONUTIL_H__
#define __AVCOMPRESSIONUTIL_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ISoundMoodManager.h"


// Defines a serial bit buffer word size 
// These are used by the compressor to serialize the compressed data
// The SWAPFUNC is needed to correct endianness for little endian processors
// The SWAPFUNC will be OS specific
#define SIZE4 

#ifdef SIZE1
typedef uint8 TserialType;
#define SWAPFUNC  (int) 
#define SERIALBYTESIZE 1
#define SERIALBITSIZE 8
#endif

#ifdef SIZE2
typedef uint16 TserialType;
#define SWAPFUNC _byteswap_ushort
#define SERIALBYTESIZE 2
#define SERIALBITSIZE 16
#endif

#ifdef SIZE4
typedef uint32 TserialType;
#define SWAPFUNC _byteswap_ulong
#define SERIALBYTESIZE 4
#define SERIALBITSIZE 32
#endif

#ifdef SIZE8
typedef uint64 TserialType;
#define SWAPFUNC _byteswap_uint64
#define SERIALBYTESIZE 8
#define SERIALBITSIZE 64
#endif


#define MAXRECORDFRAMESIZE 60*60  // 60 seconds at 60 frames per second
#define DISKCOPYSIZE 64*1024		// Size of buffer used to copy data from one file to another

#define AUDIOCAPTBUFFSIZE 32*1024  // Size of storage for audio callback function 

// define some audio modes
#define AVC_MONO 1
#define AVC_STEREO 2
#define AVC_5_1 6
#define AVC_7_1 8

// define some audio gain settings
// for combining multichannel into stereo
#define AVC_AUDIO_MAX_GAIN 32767.0f
#define AVC_AUDIO_5_1_CGAIN 0.2916f
#define AVC_AUDIO_5_1_RGAIN 0.2916f
#define AVC_AUDIO_5_1_FGAIN 0.416f
#define AVC_AUDIO_7_1_CGAIN 0.226f
#define AVC_AUDIO_7_1_RGAIN 0.226f
#define AVC_AUDIO_7_1_FGAIN 0.322f
#define AVC_AUDIO_7_1_SGAIN 0.226f
#define AVCAUDIOAMPLIMIT 32767

#define AVCOMP_OK_UTIL 0
#define AVCOMP_FILEOPEN_UTIL 100
#define AVCOMP_BUFFERWRITE_UTIL 101
#define AVCOMP_BADPARM_UTIL 102
#define AVCOMP_FILEWRITE_UTIL 103

// Definitions for capture types
#define AVCAPTURECONTINUOUS 0
#define AVCAPTURELASTXXX 1

// Circular buffer definitions
#define CAVFRAMEVALID 0x08
#define CAVFRAMEISSEQHDR 0x04

#define MPEG1_STYLE 1
#define MPEG2_STYLE 2
#define AVI_STYLE 3
#define CRY_STYLE 4

#define SERIALWRITE 0
#define SERIALREAD 1

#define MAXNTFSFILESIZE (((uint64)1 << 44) - ((uint64)1 << 16))


// Structure that defines the state of a serial buffer
struct sSerialState
{
	unsigned int serialMode;          // Current mode of the serial object, 0=write, 1 = read

	TserialType *pSerialBuff;			// serial buffer pointer
	unsigned int serialBuffSize;		// Serial buffer size
	unsigned int wordFillCnt;			// number of unoccupied bits in the current word
	unsigned int serialByteCnt;		// Total number of bytes currently stored in the buffer (does not include bytes in the current fill word)
	TserialType currentFillWord;		// Holds current working word value
	TserialType *pSerialCurrent;		// Pointers to the next available position in the serial buffer
	unsigned int serialBuffFullFlag;	// Normally 0, set to 1 if the buffer is full

	unsigned int serialReadBuffNum;	// Number of bytes remaining in the read buffer
	unsigned int bitsInByte;			// Number of bits in the current read byte
	uint8 SerialByte;					// Value of the current read byte
	uint8 *SerialReadAddr;			// pointer to the current read address
	unsigned int serialBuffEmptyFlag;	// Normally 0, set to 1 if the buffer is full
};


/////////////////////////////////////////////////////////////
// Defines the Audio/Video compression serial buffering class
/////////////////////////////////////////////////////////////
class CAVCSerialBuffer
{
public:

	// Constructor and Destructor
	CAVCSerialBuffer();
	~CAVCSerialBuffer();

	////////////////////////////////////////////////////////////////////////////////////////
	// Initialize a serial buffer
	//
	//  Input parameters :
	//		buffer : Pointer to an allocated block of memory that will be used for the serial buffer
	//		bufflen : Number of bytes to be used for the buffer
	//		mode    :  0 = read, 1 = write
	//
	//	Output : Returns true if initialized, false otherwise
	//
	//
	bool Init(uint8 *buffer, unsigned int bufflen,int mode);


	////////////////////////////////////////////////////////////////////////////////////////
	// Resets a serial buffer to its initial state,  all data in the buffer will be lost
	//  Input parameters : none
	//
	//	Output : None
	//
	//
	void Reset(void);


	////////////////////////////////////////////////////////////////////////////////////////
	// Input bits into a serial buffer 
	//
	//  Input parameters :
	//		val : Value to be input (will be input to the buffer msb first, lsb last
	//		n : Number of bits to input (1 - 31)
	//
	//	Output : None
	//
	//  NOTES : Due to the high bit rate for AV encoding, this routine is called very often
	//			and is therefore very critical for performance.  Bits are not entered into the
	//			buffer if it would cause the buffer to exceed the buffer length used during
	//			initialization.
	//
	//
	void PutBit(int val, unsigned int n);

	////////////////////////////////////////////////////////////////////////////////////////
	// Write a 32 bit value in little endian order (e.g., least sign. byte first) 
	//
	//  Input parameters :
	//		val : Value to be input (will be input to the buffer msb first, lsb last
	//
	//	Output : None
	//
	//
	//
	void PutLe32(int val);

	////////////////////////////////////////////////////////////////////////////////////////
	// Write a 16 bit value in little endian order (e.g., least sign. byte first) 
	//
	//  Input parameters :
	//		val : Value to be input (will be input to the buffer msb first, lsb last
	//
	//	Output : None
	//
	//
	//
	void PutLe16(int val);



	////////////////////////////////////////////////////////////////////////////////////////
	// Write the characters of a null terminated text string to the serial buffer 
	//
	//  Input parameters :
	//		pstr : pointer to the null terminated string
	//
	//	Output : None
	//
	//
	//
	void PutText(char *pstr);

	////////////////////////////////////////////////////////////////////////////////////////
	// Copies bytes from a buffer into the serial buffer 
	//
	//  Input parameters :
	//		pstr : pointer to the null terminated string
	//		numBytes : number of bytes to copy
	//
	//	Output : None
	//
	void CopyBytes(unsigned char *pstr,unsigned int numBytes);


	////////////////////////////////////////////////////////////////////////////////////////
	// Align a serial buffer to a byte boundary (e.g., shifts zeros into the buffer until the
	//	number of bits in the buffer is a multiple of 8)
	//
	//  Input parameters : none
	//
	//	Output : None
	//
	//
	void AlignToByte(void);


	////////////////////////////////////////////////////////////////////////////////////////
	// Align a serial buffer to a word size boundary (e.g., shifts val into the buffer until the
	//	number of bits in the buffer is a multiple of the word size)
	//
	//  Input parameters : 
	//        val  :   bit value that should be used to align (0 or 1)
	//
	//	Output : None
	//
	//
	void AlignToWord(unsigned int val);

	////////////////////////////////////////////////////////////////////////////////////////
	// Get the number of bytes currently in the buffer.  This includes bytes stored in
	// memory as well as any bytes still in the temporary shift word.
	//
	//  Input parameters : none
	//
	//	Output : Returns the number of bytes in the serial buffer
	//
	//
	unsigned int GetBytes(void);

	////////////////////////////////////////////////////////////////////////////////////////
	// Get the starting buffer address of serial buffer
	//
	//  Input parameters : none
	//
	//	Output : Returns a pointer to the starting buffer address
	//
	//
	uint8 *GetBufferAddress(void);

	////////////////////////////////////////////////////////////////////////////////////////
	// Reads data from the serial read buffer
	//
	//  Input parameters : 
	//					bitsToRead	:	number of bits to read
	//
	//	Output : None
	//
	//
	unsigned int SerialRead(unsigned int bitsToRead);

	////////////////////////////////////////////////////////////////////////////////////////
	// Gets a value from the serial buffer without advancing the serial pointer
	//
	//  Input parameters : 
	//					bitsToRead	:	number of bits to read
	//
	//	Output : value read from the serial buffer
	//
	//
	unsigned int SerialPeek(unsigned int bitsToRead);

	////////////////////////////////////////////////////////////////////////////////////////
	// Checks if the read buffer is empty
	//
	//  Input parameters : none
	//
	//	Output : returns true if the read buffer is empty, otherwise returns false
	//
	//
	bool ReadBufferEmpty(void);

	////////////////////////////////////////////////////////////////////////////////////////
	// Checks the if the write buffer is full
	//
	//  Input parameters : none
	//
	//	Output : returns true if the write buffer is full, otherwise returns false
	//
	//
	bool WriteBufferFull(void);



	////////////////////////////////////////////////////////////////////////////////////////
	// Save the current state of the serial buffer so that it can be restored
	// No serial data is actually stored, only pointers to the data
	//
	//  Input parameters : 
	//					pState	:	pointer to memory to hold the state information
	//
	//	Output : none
	//
	//
	void SaveState(struct sSerialState *pState);


	////////////////////////////////////////////////////////////////////////////////////////
	// Restore the state of the serial buffer from a previously saved state
	//
	//  Input parameters : 
	//					pState	:	pointer to memory to hold the state information
	//
	//	Output : none
	//
	//
	void RestoreState(struct sSerialState *pState);


protected:	

private:

	struct sSerialState m_serialState;
	unsigned int CAVCSerialBufferReadMask[33];


};



/////////////////////////////////////////////////////////
// Defines the Audio/Video compression file manager class
/////////////////////////////////////////////////////////

// defines a structure to hold information about frames stored on a disk based circular file
struct SAVCCircBuffInfo
{
	size_t cbuffFrameStart;			// Starting position of a frame
	int cbuffFrameNum;				// framenumber of the frame
	unsigned int cbuffFrameSize;	// Size of the frame in bytes
	uint8 cbuffFrameStatus;			// bit 0,1 = frame type (0 = I, 1 = P, 2 = B)
	// bit 2 = seq header frame
	// bit 3 = frame valid/invalid
	// bit 4-7 Undefined
};


class CAVCFileManager
{

public:

	// Constructor and Destructor
	CAVCFileManager();
	~CAVCFileManager();


	////////////////////////////////////////////////////////////////////////////////////////
	// Initialize the file manager 
	//
	//  Input parameters :
	//		_sFileName		: pointer to output filename
	//		recordMode		: 0 = continuous, 1 = record last recordTime seconds 
	//		recordTime		: number of seconds to save when using recordMode = 1
	//		frameRate		: frame rate to use for recording
	//		maxFileSize		: maximum output file size in KB that is allowed when recording in continuous mode (to prevent the disk filling up)
	//
	//		Outputs an AVCOMP error code as defined in header file
	//
	//
	HRESULT Open(CString& pFileName, int recordMode, unsigned int recordTime, float frameRate, int maxFileSize);


	////////////////////////////////////////////////////////////////////////////////////////
	// Move the frames in the circular buffer file to the final file
	// This must be done in the correct sequential order
	//
	//  Input parameters : 
	//		compressionMode  --  -1 means no files saved
	//
	//	Output : none
	//
	//
	void Close(int compressionMode);


	////////////////////////////////////////////////////////////////////////////////////////
	// Write the contents of a buffer to disk based on the capture mode
	//
	//  Input parameters :
	//		pBuffAddr						: pointer to the buffer to write
	//		numBytes						: number of bytes to write
	//		filesel							; 0 uses main file, 1 uses alternate
	//
	//	Output : None
	//
	//
	void Write(uint8 *pBuffAddr,unsigned int numBytes,int filesel);



	////////////////////////////////////////////////////////////////////////////////////////
	// Save the starting position information of a frame into the array
	// This routine is called before a frame is written the the disk
	//
	//  Input parameters :
	//		inputFrame  :  frameIndex of this frame
	//		seghdrflag	:  1 indicates that this frame is preceded by a sequence header, 0 indicates that it is not
	//
	//	Output : none
	//
	//
	void StartFrame(int frameIndex,int seghdrflag);


	////////////////////////////////////////////////////////////////////////////////////////
	// Finish updating the circular array info for a frame
	// This routine is called after a frame is written to the disk
	// After a frame is written to disk, we can update the frame information
	// including whether we corrupted any old frames when writing the new frame
	// Individual frames are never wrapped around the circular buffer.  The beginning and end
	// of the circular buffer end on frame boundaries
	//
	//  Input parameters : none
	//
	//	Output : none
	//
	//
	void EndFrame(void);

	////////////////////////////////////////////////////////////////////////////////////////
	// Write a byte of data into a file at a particular location
	//
	//  Input parameters :
	//		pbuff			: pointer to a buffer that contains the data
	//		loc				: loc in the file to place the data
	//		numBytes		: number of bytes to write
	//		filesel			; 0 uses main file, 1 uses alternate
	//
	//	Output : None
	//
	//
	void WriteAtLocation(unsigned char *pbuff,size_t loc,size_t numBytes,int filesel);

		
	////////////////////////////////////////////////////////////////////////////////////////
	// Read a byte of data from a file at a particular location
	//
	//  Input parameters :
	//		pbuff			: pointer to a buffer that will accept the data
	//		loc				: loc in the file to read the data
	//		numBytes		: number of bytes to read
	//		filesel			; 0 uses main file, 1 uses alternate
	//
	//	Output : Outputs number of bytes read
	//
	int ReadFromLocation(unsigned char *pbuff,size_t loc,size_t numBytes,int filesel);


	////////////////////////////////////////////////////////////////////////////////////////
	// Returns the offset of the first frame in the file
	//
	//  Input parameters : 
	//		headersize  -  size of the file header
	//
	//	Output : file offset to the first frame of the file, or -1 if none found
	//
	size_t GetStartFrameLocation(int headersize);

	////////////////////////////////////////////////////////////////////////////////////////
	// Returns the offset of the next frame in the file, given the current frame offset
	//
	//  Input parameters : none
	//
	//	Output : file offset of the next frame of the file
	//
	size_t GetNextFrameLocation(size_t frameOffset);


	////////////////////////////////////////////////////////////////////////////////////////
	// Write a byte of data into a file at a particular location
	//
	//  Input parameters : none
	//
	//	Output : returns the total number of bytes written to the file
	//
	//
	unsigned int OutputByteCount(int fileSel) { return(m_savedByteCount[fileSel]);}

	////////////////////////////////////////////////////////////////////////////////////////
	// Returns the amount of space available on the disk according to the maxfilesize
	//
	//  Input parameters :
	//		filesel							; 0 uses main file, 1 uses alternate
	//
	//	Output : None
	//
	//
	int64 AvailableSpace(int filesel);


protected:	

private:


	/////////////////////////////////////////////////////////////////////////////////////////
	// Closes any open files and deletes file objects
	//
	//  Input  : None
	//
	//	Output : None
	//
	void CleanFiles(void);


	CString m_sFile;		// filename of main file
	CString m_sFile_tmp;	// filename of temporary file for save last XXX mode

	class CCryFile m_AVCOutputFile;			// Final outputfile for encoded stream
	class CCryFile m_AVCCircFile;			// Intermediate file used when using save last XX mode
	uint64 m_savedByteCount[2];				// Total number of bytes saved into the output file
	unsigned int m_recordMode;				// 0 = continuous, 1 = save last
	int64 m_maxFileSize;					// maximum size of the file to write in continuous mode (-1 = no limit)


	// Save Last XX mode variables
	int m_circTimeToSave;					// length of time to save in secs for save last mode (e.g., 30 = last 30 seconds)
	int m_circFrameIndex;					// index of the currently saved frame for save last mode
	int m_circArrayLength;					// Number of frames to save in save last mode
	SAVCCircBuffInfo m_pcircArray[MAXRECORDFRAMESIZE];	// Holds information about locations and sizes of saved frames when using "save last xxx sec" feature

    uint8 m_cpybuff[DISKCOPYSIZE];               // Buffer used to copy from temporary file to final file


	// Variables for the Serial Reader
	unsigned int m_serialReadBuffNum;           // number of bytes in the Serial read buffer
	unsigned int m_bitsInByte;					// number of bits left in the current working byte
	uint8 *m_SerialReadAddr;					// Address of the next byte in the buffer
	uint8 m_SerialByte;							// Current Serial Byte

};

///////////////////////////////////////////////////////////////////////
// Defines the Audio capture class
//////////////////////////////////////////////////////////////////////
class CAVAudioCapture : IReadPCMDataListener
{
public:

	// Constructor and Destructor
	CAVAudioCapture();
	~CAVAudioCapture();


	////////////////////////////////////////////////////////////////////////////////////////
	// Call back routine for audio capture
	//
	//  Input parameters : 
	//			pfBuffer	-	Buffer holding the captured data
	//			nBufferSize -	number of samples captured
	//			nChannels	-	Number of channels of audio present in the data
	//			pUserData	-	pointer to category data
	//
	//	Output : None
	//
	//  Notes  :  This routine is registered with the Audio driver and is called whenever there is new audio data available.
	//
	void OnReadPCMData(float const* const pfBuffer, int unsigned const nBufferSize, int const nChannels, void const* const pUserData);



	////////////////////////////////////////////////////////////////////////////////////////
	//  Find the number of audio samples available in the capture buffer
	//
	//  Input parameters : none
	//
	//	Output : returns the number of audio samples available in the audio capture buffer
	//
	unsigned int GetAudioNumSamples(void);


	////////////////////////////////////////////////////////////////////////////////////////
	//  Get data from the audio capture buffer
	//
	//  Input parameters :
	//		pAudioBuff						: Pointer to an int array to hold 2 channels of audio data
	//		NumToGet						: number of samples to get per channel from the capture buffer (e.g., 10 gets 10 ch0 + 10 ch1 samples)
	//
	//	Output : returns the number of audio samples copied to the ch0 and ch1 array (may be different than NumToGet)
	//
	//     Note :  Data that is copied into the ch0 and ch1 arrays is removed from the capture buffer
	//
	unsigned int GetAudioData(int *pAudioBuff,unsigned int NumToGet);



	////////////////////////////////////////////////////////////////////////////////////////
	// Reset the audio capture buffer to its initial state
	//
	//  Input parameters : None
	//
	//	Output : None
	//
	void ResetAudioBuffer(void);


	////////////////////////////////////////////////////////////////////////////////////////
	//  Register the audio callback routine with the sound driver
	//
	//  Input parameters : None
	//
	//	Output : None
	//
	void RegisterAudioCallBack(void);


	////////////////////////////////////////////////////////////////////////////////////////
	//  UnRegister the audio callback routine with the sound driver
	//
	//  Input parameters : None
	//
	//	Output : None
	//
	void UnRegisterAudioCallBack(void);


protected:	

private:

	// Buffers and controls for capturing audio data
	int m_pAudioBuff[AUDIOCAPTBUFFSIZE];  // pointer to the allocated memory for captured audio
	unsigned int m_audioCaptHeadLoc;	// Index to current input location
	unsigned int m_audioCaptTailLoc;	// Index to current output location
	unsigned int m_audioSampCnt;		// Number of samples in the buffer
	unsigned int m_audioCaptOverFlow;	// 1 indicates a buffer overflow has occurred
	unsigned int m_AudioRegistration;	// Audio callback registration flag
	CryMutex m_AudioBufferLock;			// Used for access control to captured audio samples;

};


#endif  //__AVCOMPRESSIONUTIL_H__