////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2010.
// -------------------------------------------------------------------------
//  File name:   AVCompressionUtil.cpp
//  Version:     v1.00
//  Created:     28/04/2010 by John Stewart.
//  Compilers:   
//  Description: Audio/Video Compression  Utilities class implementations.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"

#include "AVCompression.h"
#include "AVCompressionUtil.h"




/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAV_Compression Serial Buffer Class routines
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// Constructor 
CAVCSerialBuffer::CAVCSerialBuffer()
{
	m_serialState.pSerialBuff = NULL;
	m_serialState.serialBuffSize = 0;
	m_serialState.wordFillCnt = 0;
	m_serialState.serialBuffEmptyFlag = 1;
	m_serialState.serialBuffFullFlag = 1;
	m_serialState.serialMode = SERIALREAD;
	m_serialState.SerialReadAddr = NULL;
	m_serialState.serialReadBuffNum = 0;

	unsigned int maskVal = 0;
	for(unsigned int i=0;i<33;++i)
	{
		CAVCSerialBufferReadMask[i] = maskVal;
		maskVal = (maskVal << 1) | 1;
	}
}

// Destructor
CAVCSerialBuffer::~CAVCSerialBuffer()
{


}


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
bool CAVCSerialBuffer::Init(uint8 *buffer, unsigned int bufflen,int mode)
{

    if(buffer == NULL) return(false);

	m_serialState.pSerialBuff = (TserialType *) buffer;		// Init the memory address of the buffer
	m_serialState.serialBuffSize = bufflen;					// Init the size of the buffer
	m_serialState.serialMode = mode;						// Init the mode of the buffer

	if(mode == SERIALWRITE)
	{
		m_serialState.wordFillCnt = SERIALBITSIZE;				// Init the number of bits in the current byte (counts down so 8 = none)
		m_serialState.serialByteCnt = 0;						// Init the total number of bytes currently in the buffer
		m_serialState.currentFillWord = 0;						// Init the current Byte value
		m_serialState.pSerialCurrent = (TserialType *) buffer;	// Init the pointer to the next buffer location 
		m_serialState.serialBuffFullFlag = 0;					// Indicate that the buffer is not full


		m_serialState.serialReadBuffNum = 0;					// Init the total number of bytes currently in the buffer
		m_serialState.bitsInByte = 0;							// Init the number of bits in the current byte
		m_serialState.SerialByte = 0;							// Init the current Byte value
		m_serialState.SerialReadAddr = NULL;					// Init the pointer to the next buffer location 
		m_serialState.serialBuffEmptyFlag = 1;					// Indicate that the buffer is empty

	}
	else
	{
		m_serialState.wordFillCnt = SERIALBITSIZE;				// Init the number of bits in the current byte (counts down so 8 = none)
		m_serialState.serialByteCnt = 0;						// Init the total number of bytes currently in the buffer
		m_serialState.currentFillWord = 0;						// Init the current Byte value
		m_serialState.pSerialCurrent = NULL;					// Init the pointer to the next buffer location 
		m_serialState.serialBuffFullFlag = 1;					// Indicate that the buffer is not full

		m_serialState.serialReadBuffNum = bufflen;				// Init the total number of bytes currently in the buffer
		m_serialState.bitsInByte = 0;							// Init the number of bits in the current byte
		m_serialState.SerialByte = 0;							// Init the current Byte value
		m_serialState.SerialReadAddr = buffer;					// Init the pointer to the next buffer location 
		m_serialState.serialBuffEmptyFlag = 0;					// Indicate that the buffer is empty
	}

	return(true);
}

////////////////////////////////////////////////////////////////////////////////////////
// Resets a serial buffer to its initial state 
//
//  Input parameters : none
//
//	Output : None
//
//
void CAVCSerialBuffer::Reset(void)
{

	if(m_serialState.serialMode == SERIALWRITE)
	{
		m_serialState.wordFillCnt = SERIALBITSIZE;				// Init the number of bits in the current byte (counts down so 8 = none)
		m_serialState.serialByteCnt = 0;						// Init the total number of bytes currently in the buffer
		m_serialState.currentFillWord = 0;						// Init the current Byte value
		m_serialState.pSerialCurrent = m_serialState.pSerialBuff;			// Init the pointer to the next buffer location 
		m_serialState.serialBuffFullFlag = 0;					// Indicate that the buffer is not full


		m_serialState.serialReadBuffNum = 0;					// Init the total number of bytes currently in the buffer
		m_serialState.bitsInByte = 0;							// Init the number of bits in the current byte
		m_serialState.SerialByte = 0;							// Init the current Byte value
		m_serialState.SerialReadAddr = NULL;					// Init the pointer to the next buffer location 
		m_serialState.serialBuffEmptyFlag = 1;					// Indicate that the buffer is empty
	}
	else
	{
		m_serialState.wordFillCnt = SERIALBITSIZE;				// Init the number of bits in the current byte (counts down so 8 = none)
		m_serialState.serialByteCnt = 0;						// Init the total number of bytes currently in the buffer
		m_serialState.currentFillWord = 0;						// Init the current Byte value
		m_serialState.pSerialCurrent = NULL;					// Init the pointer to the next buffer location 
		m_serialState.serialBuffFullFlag = 1;					// Indicate that the buffer is not full

		m_serialState.serialReadBuffNum = m_serialState.serialBuffSize;		// Init the total number of bytes currently in the buffer
		m_serialState.bitsInByte = 0;							// Init the number of bits in the current byte
		m_serialState.SerialByte = 0;							// Init the current Byte value
		m_serialState.SerialReadAddr = (uint8 *) m_serialState.pSerialBuff;			// Init the pointer to the next buffer location 
		m_serialState.serialBuffEmptyFlag = 0;					// Indicate that the buffer is empty
	}

}

////////////////////////////////////////////////////////////////////////////////////////
// Input bits into a serial buffer 
//
//  Input parameters :
//		val : Value to be input (will be input to the buffer msb first, lsb last
//		n : Number of bits to input (1 to 8*sizeof(int)-1)
//
//	Output : None
//
//  NOTES : Due to the high bit rate for AV encoding, this routine is called very often
//			and is therefore very critical for performance.  Bits are not entered into the
//			buffer if it would cause the buffer to exceed the buffer length used during
//			initialization.
//
//
void CAVCSerialBuffer::PutBit(int val, unsigned int n)
{

	assert((n > 0 && n < 8*sizeof(int)));


	// If the buffer is full,then don't put anymore bits into it
	if(m_serialState.serialBuffFullFlag) return;

	unsigned int bitmask = ((unsigned int) 1 << n) - 1;
	unsigned int newval = val & bitmask;
	unsigned int newlen = n;

	// Continue to loop until all bits have been processed
	while(1) 
	{
		// See if we have room for the entire value in the current word
		if(newlen <= m_serialState.wordFillCnt) 
		{
			// Yes we do so put all of the bits into the current word
			m_serialState.currentFillWord = (m_serialState.currentFillWord << newlen) | newval;
			m_serialState.wordFillCnt -= newlen;

			// Save it if we have a full word
			if(m_serialState.wordFillCnt == 0) 
			{
				*m_serialState.pSerialCurrent = SWAPFUNC(m_serialState.currentFillWord);
				++m_serialState.pSerialCurrent;
				m_serialState.wordFillCnt = SERIALBITSIZE;
				m_serialState.serialByteCnt += SERIALBYTESIZE;

				// Check to see if we are full
				if(m_serialState.serialByteCnt >= m_serialState.serialBuffSize) m_serialState.serialBuffFullFlag = 1;

			}
			return;
		} 
		else
		{
			// Not enough room for the entire value so put in as much as possible
			bitmask = bitmask >> m_serialState.wordFillCnt;
			unsigned int writeval = newval >> (newlen - m_serialState.wordFillCnt);
			newval &= bitmask;
			newlen -= m_serialState.wordFillCnt;

			m_serialState.currentFillWord = (m_serialState.currentFillWord << m_serialState.wordFillCnt) | writeval;
			*m_serialState.pSerialCurrent = SWAPFUNC(m_serialState.currentFillWord);
			++m_serialState.pSerialCurrent;
			m_serialState.wordFillCnt = SERIALBITSIZE;
			m_serialState.serialByteCnt += SERIALBYTESIZE;

			// Check for a full buffer
			if(m_serialState.serialByteCnt >= m_serialState.serialBuffSize)
			{
				m_serialState.serialBuffFullFlag = 1;
				return;
			}
		}
	}

}

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
void CAVCSerialBuffer::PutLe32(int val)
{


	int tmpVal = val;
	PutBit(tmpVal & 0xFF,8);
	tmpVal = tmpVal >> 8;
	PutBit(tmpVal & 0xFF,8);
	tmpVal = tmpVal >> 8;
	PutBit(tmpVal & 0xFF,8);
	tmpVal = tmpVal >> 8;
	PutBit(tmpVal & 0xFF,8);

}

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
void CAVCSerialBuffer::PutLe16(int val)
{


	int tmpVal = val;
	PutBit(tmpVal & 0xFF,8);
	tmpVal = tmpVal >> 8;
	PutBit(tmpVal & 0xFF,8);

}



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
void CAVCSerialBuffer::PutText(char *pstr)
{

	assert(pstr);

	char *ptmp = pstr;
	while(*ptmp != NULL)
	{
		PutBit(((int) *ptmp) & 0xFF,8);
		++ptmp;
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// Copies bytes from a buffer into the serial buffer 
//
//  Input parameters :
//		pstr : pointer to the null terminated string
//		numBytes : number of bytes to copy
//
//	Output : None
//
void CAVCSerialBuffer::CopyBytes(unsigned char *pstr,unsigned int numBytes)
{

	assert(pstr);

	for(unsigned int i=0;i<numBytes;++i) PutBit(((int) pstr[i]) & 0xFF,8);

}


////////////////////////////////////////////////////////////////////////////////////////
// Align a serial buffer to a byte boundary (e.g., shifts zeros into the buffer until the
//	number of bits in the buffer is a multiple of 8)
//
//  Input parameters : none
//
//	Output : None
//
//
void CAVCSerialBuffer::AlignToByte(void)
{

	if(m_serialState.serialMode == SERIALWRITE)
	{
		// Write mode
		unsigned int openBits = m_serialState.wordFillCnt & 0x07;
		if (openBits) PutBit(0,openBits);
	}
	else
	{
		// Read mode
		// Remove any left over bits and align the read pointer to the next byte boundary
		if(m_serialState.bitsInByte != 8) m_serialState.bitsInByte = 0;
	}

}

////////////////////////////////////////////////////////////////////////////////////////
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
void CAVCSerialBuffer::AlignToWord(unsigned int val)
{
	
	if(m_serialState.serialMode == SERIALWRITE)
	{
		while(m_serialState.wordFillCnt != SERIALBITSIZE) PutBit(val,1);
	}
	else
	{
		// Read mode
		// Remove any left over bits and align the read pointer to the next byte boundary
		if(m_serialState.bitsInByte != 8) m_serialState.bitsInByte = 0;
	}

}


////////////////////////////////////////////////////////////////////////////////////////
// Get the number of bytes currently in the buffer.  This includes bytes stored in
// memory as well as any bytes still in the temporary shift word.
//
//  Input parameters : none
//
//	Output : Returns the number of bytes in the serial buffer
//
//
unsigned int CAVCSerialBuffer::GetBytes(void)
{

	unsigned int stored_bytes = m_serialState.serialByteCnt;
	unsigned int not_stored_bytes = ((SERIALBITSIZE) - m_serialState.wordFillCnt) >> 3;
	return(stored_bytes + not_stored_bytes);

}

////////////////////////////////////////////////////////////////////////////////////////
// Get the starting buffer address of serial buffer
//
//  Input parameters : none
//
//	Output : Returns a pointer to the starting buffer address
//
//
uint8 *CAVCSerialBuffer::GetBufferAddress(void)
{
	return((uint8 *) m_serialState.pSerialBuff);
}

////////////////////////////////////////////////////////////////////////////////////////
// Reads data from the serial read buffer
//
//  Input parameters : 
//					bitsToRead	:	number of bits to read
//
//	Output : None
//
unsigned int CAVCSerialBuffer::SerialRead(unsigned int bitsToRead)
{

	unsigned int val = 0;
	unsigned int bitReadCount = bitsToRead;
	while(bitReadCount > 0)
	{
		if(bitReadCount <= m_serialState.bitsInByte)
		{
			val = val << bitReadCount;
			val |= (m_serialState.SerialByte >> (m_serialState.bitsInByte - bitReadCount)) & CAVCSerialBufferReadMask[bitReadCount];
			m_serialState.bitsInByte -= bitReadCount;
			bitReadCount = 0;
		}
		else
		{
			val = val << m_serialState.bitsInByte;
			val |= (m_serialState.SerialByte & CAVCSerialBufferReadMask[m_serialState.bitsInByte]);
			bitReadCount -= m_serialState.bitsInByte;


			if(m_serialState.serialReadBuffNum == 0) 
			{
				// Out of data 
				m_serialState.serialBuffEmptyFlag = 1;
				return(0);
			}
			m_serialState.bitsInByte = 8;
			m_serialState.SerialByte = *m_serialState.SerialReadAddr;
			++m_serialState.SerialReadAddr;
			--m_serialState.serialReadBuffNum;
		}
	}
	return(val);
}



////////////////////////////////////////////////////////////////////////////////////////
// Gets a value from the serial buffer without advancing the serial pointer
//
//  Input parameters : 
//					bitsToRead	:	number of bits to read
//
//	Output : value read from the serial buffer
//
//
unsigned int CAVCSerialBuffer::SerialPeek(unsigned int bitsToRead)
{

	struct sSerialState currentState;
	SaveState(&currentState);

	unsigned int val = 0;
	unsigned int bitReadCount = bitsToRead;
	while(bitReadCount > 0)
	{
		if(bitReadCount <= m_serialState.bitsInByte)
		{
			val = val << bitReadCount;
			val |= (m_serialState.SerialByte >> (m_serialState.bitsInByte - bitReadCount)) & CAVCSerialBufferReadMask[bitReadCount];
			m_serialState.bitsInByte -= bitReadCount;
			bitReadCount = 0;
		}
		else
		{
			val = val << m_serialState.bitsInByte;
			val |= (m_serialState.SerialByte & CAVCSerialBufferReadMask[m_serialState.bitsInByte]);
			bitReadCount -= m_serialState.bitsInByte;
			m_serialState.bitsInByte = 8;
			if(m_serialState.serialReadBuffNum != 0)
			{
				m_serialState.SerialByte = *m_serialState.SerialReadAddr;
				++m_serialState.SerialReadAddr;
				--m_serialState.serialReadBuffNum;
			}
		}
	}

	RestoreState(&currentState);

	return(val);
}

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
void CAVCSerialBuffer::SaveState(struct sSerialState *pState)
{
	memcpy(pState,&m_serialState,sizeof(struct sSerialState));
}

////////////////////////////////////////////////////////////////////////////////////////
// Restore the state of the serial buffer from a previously saved state
//
//  Input parameters : 
//					pState	:	pointer to memory to hold the state information
//
//	Output : none
//
//
void CAVCSerialBuffer::RestoreState(struct sSerialState *pState)
{
	memcpy(&m_serialState,pState,sizeof(struct sSerialState));

}

////////////////////////////////////////////////////////////////////////////////////////
// Checks if the read buffer is empty
//
//  Input parameters : none
//
//	Output : returns true if the read buffer is empty, otherwise returns false
//
//
bool CAVCSerialBuffer::ReadBufferEmpty(void)
{
	return(m_serialState.serialBuffEmptyFlag ? true : false);

}

////////////////////////////////////////////////////////////////////////////////////////
// Checks the if the write buffer is full
//
//  Input parameters : none
//
//	Output : returns true if the write buffer is full, otherwise returns false
//
//
bool CAVCSerialBuffer::WriteBufferFull(void)
{
	return(m_serialState.serialBuffFullFlag ? true : false);

}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAVCFileManager File Manager  Class routines
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



// Constructor 
CAVCFileManager::CAVCFileManager()
{
	m_sFile = _T("Untitled.avi");
	m_AVCOutputFile = NULL;
	m_AVCCircFile = NULL;

}

// Destructor
CAVCFileManager::~CAVCFileManager()
{
		CleanFiles();

}

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
HRESULT CAVCFileManager::Open(CString& pFileName, int recordMode, unsigned int recordTime, float frameRate, int maxFileSize)
{

	// Initialize some recording management parameters
	m_recordMode = recordMode;
	m_circTimeToSave = recordTime;
	m_circArrayLength = CLAMP((1 + (int) recordTime*frameRate),0,MAXRECORDFRAMESIZE);
	m_maxFileSize = ((int64) maxFileSize) << 10;  // convert from KByte to Byte
	m_savedByteCount[0] = 0;
	m_savedByteCount[1] = 0;


	// Save the filename
	m_sFile = pFileName;

	if( m_sFile.Find("avi") == -1) m_sFile += _T(".avi");
	m_sFile_tmp = m_sFile +  _T("_tmp");

	// Open the final output file object
	if(!m_AVCOutputFile.Open((LPCSTR) m_sFile,"w+b",0 )) 
	{
		CleanFiles();
		return(AVCOMP_FILEOPEN_UTIL);
	}

	// Open the intermediate output file object
	if(!m_AVCCircFile.Open((LPCSTR) m_sFile_tmp,"w+b",0 )) 
	{
		CleanFiles();
		return(AVCOMP_FILEOPEN_UTIL);
	}


	// If in the last save xxx mode, open the temporary file object
	if(m_recordMode == AVCAPTURELASTXXX) 
	{
		// Initialize the current saved frame location as "none saved yet"
		m_circFrameIndex = -1;

		// Initialize all of the frames in the array as invalid
		for(unsigned int i=0;i<m_circArrayLength;++i) m_pcircArray[i].cbuffFrameStatus = 0;  // Mark as invalid

	}

	return(AVCOMP_OK_UTIL);
}





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
void CAVCFileManager::StartFrame(int frameIndex,int seghdrflag)
{


	// If saving in the last XX secs mode, then update the frame array
	if(m_recordMode == AVCAPTURELASTXXX) 
	{

		// Update the index location into the circular array
		++m_circFrameIndex;

		// If we are at the end of the circular array, reset our position back to the top of the array
		if(m_circFrameIndex == m_circArrayLength) 
		{
			m_circFrameIndex = 0;
			m_AVCCircFile.Seek(20,SEEK_SET);  // set after the file header
		}


		// Update the circular buffer array for this frame
		m_pcircArray[m_circFrameIndex].cbuffFrameNum = frameIndex;								// save the framenumber
		m_pcircArray[m_circFrameIndex].cbuffFrameStart = m_AVCCircFile.GetPosition();			// Save the position on the disk file
		if(seghdrflag) m_pcircArray[m_circFrameIndex].cbuffFrameStatus |= CAVFRAMEISSEQHDR;	// update the sequence header flag info
		else  m_pcircArray[m_circFrameIndex].cbuffFrameStatus &= !CAVFRAMEISSEQHDR;
		m_pcircArray[m_circFrameIndex].cbuffFrameStatus |= CAVFRAMEVALID;						// Set the status of this entry as valid

	}

}


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
void CAVCFileManager::EndFrame(void)
{


	if(m_recordMode == AVCAPTURELASTXXX) 
	{
		// Update the length of the frame we just wrote to disk
		size_t currentFileLoc = m_AVCCircFile.GetPosition();
		m_pcircArray[m_circFrameIndex].cbuffFrameSize = currentFileLoc - m_pcircArray[m_circFrameIndex].cbuffFrameStart;

		// Check to see if we corrupted any of the frames below us on the list
		// Unless we are at the end of the list and then there is nothing below the current frame to corrupt
		if(m_circFrameIndex < m_circArrayLength - 1) 
		{

			// This was not the last frame so check all frames below the current frame for corruption
			for(unsigned int i=m_circFrameIndex+1;i<m_circArrayLength;++i) 
			{
				if(currentFileLoc <= m_pcircArray[i].cbuffFrameStart) break;
				m_pcircArray[i].cbuffFrameStatus = 0;  // Mark frame as invalid
			}

		}
	}


}


////////////////////////////////////////////////////////////////////////////////////////
// Move the frames in the circular buffer file to the final file
// This must be done in the correct sequential order
//
//  Input parameters : 
//		compressionMode  -  -1 means no re-compression done, delete all files
//
//	Output : none
//
//
void CAVCFileManager::Close(int compressionMode)
{
	CleanFiles();

	// If no recompression is done, delete the main file as well
	if(compressionMode == -1) 
	{
#if (defined (WIN32) || defined(WIN64))
		SetFileAttributes((LPCSTR) m_sFile,FILE_ATTRIBUTE_NORMAL);
#endif
		remove((LPCSTR) m_sFile);
	}

}


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
void CAVCFileManager::Write(uint8 *pBuffAddr,unsigned int numBytes,int filesel)
{

	assert(pBuffAddr);

	if(numBytes > 0)
	{

		if(m_maxFileSize == -1 || m_savedByteCount[filesel] + numBytes < m_maxFileSize) 
		{
			if(filesel == 0)  m_AVCOutputFile.Write(pBuffAddr,numBytes);
			else m_AVCCircFile.Write(pBuffAddr,numBytes);
			m_savedByteCount[filesel] += numBytes;
		}
	}

}

////////////////////////////////////////////////////////////////////////////////////////
// Returns the amount of space available on the disk according to the maxfilesize
//
//  Input parameters :
//		filesel							; 0 uses main file, 1 uses alternate
//
//	Output : None
//
//
int64 CAVCFileManager::AvailableSpace(int filesel)
{

	uint64 availSpace;  
	if(m_maxFileSize != -1) availSpace = m_maxFileSize;
	else availSpace = MAXNTFSFILESIZE; 
	availSpace -= m_savedByteCount[filesel];  
	return(availSpace);

}

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
void CAVCFileManager::WriteAtLocation(unsigned char *pbuff,size_t loc,size_t numBytes,int filesel)
{

	size_t currentLoc;
	if(filesel == 0)
	{
		currentLoc = m_AVCOutputFile.GetPosition();
		m_AVCOutputFile.Seek(loc,SEEK_SET);
		m_AVCOutputFile.Write(pbuff,numBytes);
		m_AVCOutputFile.Seek(currentLoc,SEEK_SET);
	}
	else
	{
		currentLoc = m_AVCCircFile.GetPosition();
		m_AVCCircFile.Seek(loc,SEEK_SET);
		m_AVCCircFile.Write(pbuff,numBytes);
		m_AVCCircFile.Seek(currentLoc,SEEK_SET);
	}




}

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
int CAVCFileManager::ReadFromLocation(unsigned char *pbuff,size_t loc,size_t numBytes,int filesel)
{

	int readBytes;
	if(filesel == 0)
	{
		size_t currentLoc = m_AVCOutputFile.GetPosition();
		m_AVCOutputFile.Seek(loc,SEEK_SET);
		readBytes = m_AVCOutputFile.ReadRaw(pbuff,numBytes);
		m_AVCOutputFile.Seek(currentLoc,SEEK_SET);
	}
	else
	{
		size_t currentLoc = m_AVCCircFile.GetPosition();
		m_AVCCircFile.Seek(loc,SEEK_SET);
		readBytes = m_AVCCircFile.ReadRaw(pbuff,numBytes);
		m_AVCCircFile.Seek(currentLoc,SEEK_SET);
	}
	return(readBytes);
}


////////////////////////////////////////////////////////////////////////////////////////
// Returns the offset of the first frame in the file
//
//  Input parameters : 
//		headersize  -  size of the file header
//
//	Output : file offset to the first frame of the file, or -1 if none found
//
size_t CAVCFileManager::GetStartFrameLocation(int headersize)
{
	CAVCSerialBuffer hdrSerialBuff; 
	uint8 hdrbuff[18];

	if(m_recordMode == AVCAPTURELASTXXX) // Save last xx mode
	{

		// Find the highest numbered frame
		int frameNum = -1;
		int frameIdx = -1;
		for(unsigned int i = 0;i<m_circArrayLength;++i) 
		{
			if((m_pcircArray[i].cbuffFrameStatus & CAVFRAMEVALID) != 0) 
			{
				if(frameNum == -1 || m_pcircArray[i].cbuffFrameNum > frameNum)
				{
					frameNum = m_pcircArray[i].cbuffFrameNum;
					frameIdx = i;
				}
			}
		}



		// If we did not find a valid frame return -1
		if(frameIdx == -1) return(-1);


		// Walk backwards through time too find where the video should start
		int totalFrames = 0;
		while(totalFrames < m_circArrayLength)
		{
			// Read the header to get the number of frames to add
			unsigned int frameOffset = m_pcircArray[frameIdx].cbuffFrameStart;
			hdrSerialBuff.Init(hdrbuff,18,SERIALREAD);
			ReadFromLocation(hdrbuff,frameOffset,18,1);
			unsigned int syncWord = hdrSerialBuff.SerialRead(32);
			if(syncWord != FILEFRAMESYNC) return(-1);
			unsigned int headerSize = hdrSerialBuff.SerialRead(16);
			unsigned int sections = hdrSerialBuff.SerialRead(16);
			unsigned int encodedVideoBytes = hdrSerialBuff.SerialRead(32);
			unsigned int encodedAudioBytes = hdrSerialBuff.SerialRead(32);
			unsigned int numOutputFrames = hdrSerialBuff.SerialRead(16);
			totalFrames += numOutputFrames;

			// Move to previous frame
			int newFrameIdx;
			if(frameIdx == 0) newFrameIdx = m_circArrayLength-1;
			else newFrameIdx = frameIdx - 1;

			// Check if this is a valid frame, if not, return the last valid frame
			if((m_pcircArray[newFrameIdx].cbuffFrameStatus & CAVFRAMEVALID) == 0) return(m_pcircArray[frameIdx].cbuffFrameStart);
			frameIdx = newFrameIdx;
		}
		return(m_pcircArray[frameIdx].cbuffFrameStart);
	}
	else
	{   
		return(headersize);
	}

}

////////////////////////////////////////////////////////////////////////////////////////
// Returns the offset of the next frame in the file, given the current frame offset
//
//  Input parameters : 
//		frameOffset  -  location of the current frame header
//
//	Output : file offset of the next frame of the file
//
size_t CAVCFileManager::GetNextFrameLocation(size_t frameOffset)
{
	CAVCSerialBuffer hdrSerialBuff; 
	uint8 hdrbuff[16];


	if(m_recordMode == AVCAPTURELASTXXX) // Save last xx mode
	{

		// Circular File
		// Find the current offset in the offset array
		int frameIdx = -1;
		for(unsigned int i = 0;i<m_circArrayLength;++i) 
		{
			if(m_pcircArray[i].cbuffFrameStart == frameOffset && (m_pcircArray[i].cbuffFrameStatus & CAVFRAMEVALID) != 0) frameIdx = i;
		}

		// If we can't find the frame, return 0
		if(frameIdx == -1) return(0);
		
		int frameNum = m_pcircArray[frameIdx].cbuffFrameNum;

		// Try to find the next frame
		++frameNum;
		frameIdx = -1;
		for(unsigned int i = 0;i<m_circArrayLength;++i) 
		{
			if(m_pcircArray[i].cbuffFrameNum == frameNum && (m_pcircArray[i].cbuffFrameStatus & CAVFRAMEVALID) != 0) frameIdx = i;
		}

		// If we can't find the frame, return 0
		if(frameIdx == -1) return(0);

		// Found it, return the offset
		return(m_pcircArray[frameIdx].cbuffFrameStart);

	}
	else
	{   

		// Linear file
		// Read the info in the current frame header to find the next frame
		hdrSerialBuff.Init(hdrbuff,16,SERIALREAD);
		ReadFromLocation(hdrbuff,frameOffset,16,1);

		unsigned int syncword = hdrSerialBuff.SerialRead(32);
		if(syncword != FILEFRAMESYNC) return(0);
		unsigned int headerSize = hdrSerialBuff.SerialRead(16);
		unsigned int sections = hdrSerialBuff.SerialRead(16);
		unsigned int encodedVideoBytes = hdrSerialBuff.SerialRead(32);
		unsigned int encodedAudioBytes = hdrSerialBuff.SerialRead(32);

		size_t nextFrameOffset = frameOffset + headerSize + encodedVideoBytes + encodedAudioBytes;
		return(nextFrameOffset);
	}

}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Closes any open files and deletes file objects
//
//  Input  : none
//
//	Output : None
//
void CAVCFileManager::CleanFiles(void)
{

	// Clean up the final output file
	if(m_AVCOutputFile.GetHandle() != NULL) m_AVCOutputFile.Close();
	if(m_AVCCircFile.GetHandle() != NULL) m_AVCCircFile.Close();

#if (defined (WIN32) || defined(WIN64))
		SetFileAttributes((LPCSTR) m_sFile_tmp,FILE_ATTRIBUTE_NORMAL);
#endif
		remove((LPCSTR) m_sFile_tmp);
}








/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAVC_Audio_Capture Audio Capture Class routines
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CAVAudioCapture::CAVAudioCapture()
{
	m_AudioRegistration = 0;
	ResetAudioBuffer();
	RegisterAudioCallBack();

}

// Destructor
CAVAudioCapture::~CAVAudioCapture()
{
	if(m_AudioRegistration)  UnRegisterAudioCallBack();


}

//#define MYBIGBUFFSIZE 1024*1024
//int mybigbuffer0[MYBIGBUFFSIZE][2];
//int mybigbuffindex=0;

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
void CAVAudioCapture::OnReadPCMData(float const* const pfBuffer, int unsigned const nBufferSize, int const nChannels, void const* const pUserData)
{

	assert(pfBuffer);

	if (pUserData)
	{

		// Only get the master category
		ICategory const* const pCategory = (ICategory*)pUserData;
		if (pCategory && !_stricmp(pCategory->GetName(), "master"))
		{
			m_AudioBufferLock.Lock();


float avg_eng=0.0;
float peak_eng = 0.0;
int clipcount=0;

			// Convert the audio data to stereo and copy to the local audio circular buffer
			for(unsigned int i=0;i<nBufferSize*nChannels;i+=nChannels) 
			{
				int ch0Audio = 0;
				int ch1Audio = 0;

				switch(nChannels)
				{
				case AVC_MONO :
					ch0Audio = (int) (AVC_AUDIO_MAX_GAIN * pfBuffer[i]);
					ch1Audio = (int) (AVC_AUDIO_MAX_GAIN * pfBuffer[i]);
					break;


				case AVC_STEREO :
					ch0Audio = (int) (AVC_AUDIO_MAX_GAIN * pfBuffer[i]);
					ch1Audio = (int) (AVC_AUDIO_MAX_GAIN * pfBuffer[i+1]);

/*
					if(mybigbuffindex < MYBIGBUFFSIZE) {
						mybigbuffer0[mybigbuffindex][0] = ch0Audio;
						mybigbuffer0[mybigbuffindex][1] = ch1Audio;
						mybigbuffindex++;
					}

					if(mybigbuffindex == MYBIGBUFFSIZE-1) {

						FILE *myfp1 = fopen("j://temp//audiodata.txt","w");
						if(myfp1 != NULL) for(int ii=0;ii<1024*1024;ii++) fprintf(myfp1,"%8d  %8d\n",mybigbuffer0[ii][0],mybigbuffer0[ii][1]);
						fclose(myfp1);

					}
*/

					break;

				case AVC_5_1:		// 5.1 channel order is Front Left,Front Right,Center,Sub,Rear Left,Rear Right
					ch0Audio = (int) (AVC_AUDIO_MAX_GAIN*(AVC_AUDIO_5_1_FGAIN*pfBuffer[i] + AVC_AUDIO_5_1_RGAIN*pfBuffer[i+4] + AVC_AUDIO_5_1_CGAIN*pfBuffer[i+2]));
					ch1Audio = (int) (AVC_AUDIO_MAX_GAIN*(AVC_AUDIO_5_1_FGAIN*pfBuffer[i+1] + AVC_AUDIO_5_1_RGAIN*pfBuffer[i+5] + AVC_AUDIO_5_1_CGAIN*pfBuffer[i+2]));
					break;

				case AVC_7_1:		// 7.1 channel order is Front Left,Front Right,Center,Sub,Rear Left,Rear Right,Middle Left, Middle Right
					ch0Audio = (int) (AVC_AUDIO_MAX_GAIN*(AVC_AUDIO_7_1_FGAIN*pfBuffer[i] + AVC_AUDIO_7_1_RGAIN*pfBuffer[i+4] + AVC_AUDIO_7_1_SGAIN*pfBuffer[i+6] + AVC_AUDIO_7_1_CGAIN*pfBuffer[i+2]));
					ch1Audio = (int) (AVC_AUDIO_MAX_GAIN*(AVC_AUDIO_7_1_FGAIN*pfBuffer[i+1] + AVC_AUDIO_7_1_RGAIN*pfBuffer[i+5] + AVC_AUDIO_7_1_SGAIN*pfBuffer[i+7] + AVC_AUDIO_7_1_CGAIN*pfBuffer[i+2]));
					break;


				default:	 // Just add it all up and put it to both channels
					float comb_sum = 0.0f;
					for(int k=0;k<nChannels;++k) comb_sum += pfBuffer[i+k];
					ch0Audio = (int) (AVC_AUDIO_MAX_GAIN*comb_sum/(float) nChannels);
					ch1Audio = ch0Audio;
					break;
				}


				if(ch0Audio < 0) {
					avg_eng -= ch0Audio;
					if(-ch0Audio > peak_eng) peak_eng = -ch0Audio;
					if(ch0Audio < -32767) {
						clipcount++;
					}
				}
				else {
					avg_eng += ch0Audio;
					if(ch0Audio > peak_eng) peak_eng = ch0Audio;
					if(ch0Audio > 32767) {
						clipcount++;
					}
				}


				ch0Audio = CLAMP(ch0Audio,-AVCAUDIOAMPLIMIT,AVCAUDIOAMPLIMIT);
				ch1Audio = CLAMP(ch1Audio,-AVCAUDIOAMPLIMIT,AVCAUDIOAMPLIMIT);

				m_pAudioBuff[m_audioCaptHeadLoc] = ch0Audio;
				++m_audioCaptHeadLoc;
				if(m_audioCaptHeadLoc == AUDIOCAPTBUFFSIZE) m_audioCaptHeadLoc = 0;
				m_pAudioBuff[m_audioCaptHeadLoc] = ch1Audio;
				++m_audioCaptHeadLoc;
				if(m_audioCaptHeadLoc == AUDIOCAPTBUFFSIZE) m_audioCaptHeadLoc = 0;
				++m_audioSampCnt;


			}
			if(m_audioSampCnt > (AUDIOCAPTBUFFSIZE >> 2))	
			{
				m_audioCaptOverFlow = 1;
			}
			m_AudioBufferLock.Unlock();

		}
	}

}

////////////////////////////////////////////////////////////////////////////////////////
//  Find the number of audio samples available in the capture buffer
//
//  Input parameters : none
//
//	Output : returns the number of audio samples available in the audio capture buffer
//
unsigned int CAVAudioCapture::GetAudioNumSamples(void)
{


	m_AudioBufferLock.Lock();
	unsigned int samps_avail = m_audioSampCnt;
	m_AudioBufferLock.Unlock();

	return(samps_avail);

}




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
unsigned int CAVAudioCapture::GetAudioData(int *pAudioBuff,unsigned int NumToGet)
{

	assert(pAudioBuff);

	m_AudioBufferLock.Lock();
	unsigned int toget = 0;
	if(m_audioSampCnt >= NumToGet)
	{
		toget = NumToGet;
		int *pBuff = pAudioBuff;
		for(unsigned int i=0;i<toget;++i)
		{
			*pBuff = m_pAudioBuff[m_audioCaptTailLoc];
			++pBuff;
			++m_audioCaptTailLoc;
			if(m_audioCaptTailLoc == AUDIOCAPTBUFFSIZE) m_audioCaptTailLoc = 0;

			*pBuff = m_pAudioBuff[m_audioCaptTailLoc];
			++pBuff;
			++m_audioCaptTailLoc;
			if(m_audioCaptTailLoc == AUDIOCAPTBUFFSIZE) m_audioCaptTailLoc = 0;
			--m_audioSampCnt;
		}

	}
	m_AudioBufferLock.Unlock();

	return(toget);

}

////////////////////////////////////////////////////////////////////////////////////////
// Reset the audio capture buffer to its initial state
//
//  Input parameters : None
//
//	Output : None
//
void CAVAudioCapture::ResetAudioBuffer(void)
{
	m_AudioBufferLock.Lock();
	m_audioCaptHeadLoc = 0;
	m_audioCaptTailLoc = 0;
	m_audioSampCnt = 0;
	m_audioCaptOverFlow = 0;
	m_AudioBufferLock.Unlock();
}


////////////////////////////////////////////////////////////////////////////////////////
//  Register the audio callback routine with the sound driver
//
//  Input parameters : None
//
//	Output : None
//
void CAVAudioCapture::RegisterAudioCallBack(void)
{

	ISoundSystem* const	 pSoundSystem = gEnv->pSoundSystem;
	if(pSoundSystem && m_AudioRegistration == 0)
	{
		ICategory* const	 pCategory = pSoundSystem->GetIMoodManager()->GetMixMoodPtr()->GetMasterCategory();
		pCategory->AddReadPCMDataListener(this, (void*)pCategory);
		m_AudioRegistration = 1;
	}

}

////////////////////////////////////////////////////////////////////////////////////////
//  UnRegister the audio callback routine with the sound driver
//
//  Input parameters : None
//
//	Output : None
//
void CAVAudioCapture::UnRegisterAudioCallBack(void)
{

	ISoundSystem* const	 pSoundSystem = gEnv->pSoundSystem;
	if(pSoundSystem && m_AudioRegistration)
	{
		ICategory* const	 pCategory = pSoundSystem->GetIMoodManager()->GetMixMoodPtr()->GetMasterCategory();
		pCategory->RemoveReadPCMDataListener(this);
		m_AudioRegistration = 0;
	}
}
