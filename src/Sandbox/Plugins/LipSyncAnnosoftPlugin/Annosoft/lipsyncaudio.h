/* 
Name:
	lipsyncaudio.h 

	Copyright (c) 2004 Annosoft, LLC. Garland, Texas. All rights reserved.     
	This program and header file contains the confidential trade secret 
	information of Annosoft, LLC.  Use, disclosure, or copying without 
	written consent is strictly prohibited. 
*/
/**
@file lipsyncaudio.h
@brief This file declares interfaces and functions for dealing with audio at a lower level.

Initially, this file provided support for the Lipsync Tool operations, such as preloading
audio, drawing audio, etc.

Now, it also declares the IInstallableAudioFileReader and functions to allow applications
to insert their own audio format readers into the SDK.
*/


#ifndef _H_LIPSYNCAUDIO
#define _H_LIPSYNCAUDIO


#include "liblipsync.h"


///////////////////////////////////////////////////////////////////////////////
// Name: IInstallableAudioFileReader
/**@ingroup app_interfaces
   @brief  
   The IInstallableAudioFileReader interface is subclassed by the application to add 
    custom audio file readers to the lipsync SDK.
  
   
   This architecture allows applications to use ::CreateObservationStreamFromAudioFile or 
   ::CreatePCMDataSource to read files of native file types (WAV) and file types that you 
   define in the same way.
   An example implementation is MP3AudioFileReaderPlugin defined in mp3_audio_source.h
  
   To use, applications will subclass this interface and and also subclass 
   an IAnnoPCMStream2 interface for their reader. 
  
   The IInstallableAudioFileReader subclass will be added to the available native formats
   using the DLL function ::AddAudioFileFormatReader. 
  
   The nice thing about this architecture is that it allows the actual lipsync code to
   work independently of special cases for audio types. The Lipsync Tool uses this.
  
   @see 
   - ::AddAudioFileFormatReader
   - IAnnoPCMStream2
   - ::CreateObservationStreamFromAudioFile,
   - CMp3LipsyncAudioSource and MP3AudioFileReaderPlugin
*/
class IInstallableAudioFileReader 
{
public:
	///////////////////////////////////////////////////////////////////////////
    // Name: IsFileType
    /**@brief
	   This method is called by the SDK to determine if the specified source file is
	   a format readable by this file reader.
	  
	   Subclasses will implement this method.
	   
	   @param  szSourceFile - [in] path to source file to validate against our type.
	   @return
	   true - if szSourceFile is of a type readable by this file reader.
	   false - if szSource file is not of a type readable by this reader.
    */
	virtual bool  IsFileType(const char* szSourceFile) = 0;

	///////////////////////////////////////////////////////////////////////////
    // Name: MakeStream
    /**@brief
	   This method is called by the SDK to construct an IAnnoPCMStream2* from the given 
       audio file.
      
	   Subclasses will implement this to construct their subclass of IAnnoPCMStream2
	   given the source file.
	   
	   
	   @param szSourceFile - [in] Path to source file
	   @param ppStream - [out] Instantiated IAnnoPCMStream2 subclass used for reading PCM Data
	  
	   @return
	   serror - possible errors include kNoError, kErrInvalidFormat, kErrMemory
	  
	   @note the resulting stream should be reference counted by 1. That is
	   we should be able to call pStream->Release() without making a call to AddRef first
    */
	virtual serror MakeStream(const char* szSourceFile, IAnnoPCMStream2** ppStream) = 0;

	///////////////////////////////////////////////////////////////////////////
    // Name: AddRef
    /**@brief
	   This method increments the reference count for this object
	  
	   @return
	  	The new reference count for the object
    */
	virtual long   AddRef() = 0;

	///////////////////////////////////////////////////////////////////////////
    // Name: Release
    /**@brief
	   This method decrements the reference count of this object, possibly destroying if it goes to 0
	  
	   @return
	  	The new reference count for the object
    */
	virtual long   Release() = 0;
};


///////////////////////////////////////////////////////////////////////////
// Name: AddAudioFileFormatReader
/**@ingroup sdk_functions
   @brief
   This function is used to add a custom audio file format (readers) to the Lipsync SDK. 
    
   This allows applications to use the same audio creation methods 
   regardless of format (so long as its supported)

   To support an external format. Create an IAnnoPCMStream2 that is capable of reading
   the format, create an IInstallableAudioFileReader, add the Reader to the LipsyncSDK
   using this method
   Parameters
  	pFormat - [in] the file reader
   @see IInstallableAudioFileReader, IAnnoPCMStream2, IAnnoPCMStream
*/
LIBLIP_API void AddAudioFileFormatReader(IInstallableAudioFileReader* pFormat);

/**
@defgroup lipsync_audio Audio Interfaces and Functions used by the Lipsync Tool
@brief This collection of interfaces and functions is used by the Lipsync Tool

The Lipsync Tool needed more audio information, such as the pcm data
spectral data, etc. This would significantly complicate the lipsync interfaces
so this information has been rolled into a separate group.

For people using the lipsync tool source code, it might be necessary
to learn some of this. But probably not. 

For SDK customers, this stuff is probably not needed
*/

/**
@brief IPCMDataSource class is used to read pcm data from a file.
@ingroup lipsync_audio

It supports a myriad of operations, the bulk of these are used in
the Lipsync Tool. SDK customers who do not use the lipsync tool
probably don't need to learn this class object.
**/
class IPCMDataSource
{
protected:
	/// protected destructor (needed for memory cleanup in VC6)
	virtual ~IPCMDataSource() {}
public:
    /**@brief This method returns the format of the pcm data
	   @param pFormat - [out] the format data.
	   @return serror - kNoError or an appropriate error code
    **/
	virtual serror GetFormat (PCMStreamFormat *pFormat)= 0;

	/**@brief This method returns the sample rate in fixed point notation
	    @return ufixed - 16|16 fixed point value.
	    @see FixToDouble, Fix **/
    virtual ufixed GetSampleRate ()= 0;
	
	/**@brief This method returns the size of each sample in bits (8-16)
	   @retval 8 - 8 bits per sample
	   @retval 16 - 16 bits per sample
     **/
    virtual ushort GetSampleSize ()= 0;
	
	/** 
    @brief 
    This method returns the size in bytes of each frame of audio data. 
    @verbatim 
    = Number of Channels * IPCMDataSource::GetSampleSize()/8.
    @endverbatim
    **/
	virtual ulong  GetFrameSize()= 0;

	/**
    @brief 
    This method returns the number of audio frames in the audio data
	@verbatim
    = GetNumBytes()/GetFrameSize().
    @endverbatim
    **/
    virtual slong  GetNumFrames ()= 0;

	/**
    @brief 
    This method returns the total number of bytes of audio data
    **/
	virtual slong  GetNumBytes ()= 0;

    /**
    @brief This method calculates the millisecond at which the given sample
	occurs.
	@param nSample - [in] sample/frame to calculate the time for.
	@return double - millisecond time of the sample
    **/
    virtual double SampleToMilli (slong nSample)= 0;   

	/** 
    @brief 
    This method, given a byte position in the audio data,
	@return the frame/sample represented by the byte.
    **/
	virtual slong ByteToFrame(const slong byte)= 0;

	/**@brief
      This method, given a frame number, returns the position 
      in bytes in the audio data of the specified frame.
      @param frame - [in] frame number
      @return - byte offset to the specified frame
      **/
	virtual slong FrameToByte(const slong frame)= 0;

	/** @brief
        This method determines the sample position of 
        the specified millisecond time.
        @param nMilli - [in] millisecond time
        @return sample/frame for the specified time 
    **/
    virtual long MilliToSample (long nMilli)= 0;

	/** 
    @brief
    This method reads audio data, given a buffer, a start frame, a frame count,
	and a desired channel.

	@param pData - [in,out] A buffer of data at that can hold *pnFrames of audio data. On return, it will be filled with audio data.
	@param nStartFrame - [in] The frame of audio data to start the read from. 
	@param pnFrames	- [in, out] On input, the number of frames to attempt to read. On output, the actual number of frames read.
	@param nChannels   - [in] The channel number to read. if 0, all channels are read. if 1, the first channel only, if 2 the second channel only. etc.
    @return serror - kNoError or an appropriate error code
    **/
    virtual serror Read (void *pData, slong nStartFrame, slong *pnFrames, slong nChannel)= 0;

	/**
	@brief
    This method reads audio data, given a buffer, a start frame, a frame count,
	and a desired channel. 
    
    It converts all sample sizes into 16 bit before returning.
	@param pData - [in,out] A buffer of data at that can hold *pnFrames of audio data. On return, it will be filled with audio data.
	@param nStartFrame - [in] The frame of audio data to start the read from. 
	@param pnFrames	- [in, out] On input, the number of frames to attempt to read. On output, the actual number of frames read.
	@param nChannels   - [in] The channel number to read. if 0, all channels are read. if 1, the first channel only, if 2 the second channel only. etc.
    @return serror - kNoError or an appropriate error code
    **/
    virtual serror ReadSamples16 (sshort *pData, slong nStartFrame, slong *pnFrames, slong nChannel)= 0;
	
	/**@brief
    This method reads audio data, given a buffer, a start frame, a frame count,
	and a desired channel. 
    
    It converts all sample sizes to 16bit floating point values before returning.
	
    @param pData - [in,out] A buffer of data at that can hold *pnFrames of audio data. On return, it will be filled with audio data.
	@param nStartFrame - [in] The frame of audio data to start the read from. 
	@param pnFrames	- [in, out] On input, the number of frames to attempt to read. On output, the actual number of frames read.
	@param nChannels   - [in] The channel number to read. if 0, all channels are read. if 1, the first channel only, if 2 the second channel only. etc.
    @return serror - kNoError or an appropriate error code
    **/
    virtual serror ReadSamples16Float (float *pData, slong nStartFrame, slong *pnFrames, slong nChannel)= 0;

    /** @brief
        overwrite the samples at the specified frame with new data.
        
	    FOR CONSTRUCTION PURPOSES. CAREFUL. 
        This won't change the size of the data source, so it's not very generic. 
        We don't have full read-write capability in this class 
        @param pData - [in] data to write
        @param nStartFrame - [in] start frame of overwrite
        @param nChannels   - [in] The channel index
        @return serror - kNoError or an appropriate error code
    **/
	virtual serror WriteSamples16Float(float *pData, slong nStartFrame, slong nFrames, slong nChannel)= 0;

    /**
    @brief
    This method retrieves a live pointer to the PCM data. 
      
    Applications can directly access the source data but should not dispose this data 
    or use it  beyond the lifecycle of this object. 
    @return PCM data 
    **/
    virtual void* GetData()= 0;

	/**@brief
    This method is used to make a clean copy of the object. 

    It is deeply copied and thus the source and destination audio 
    sources can be/must be disposed of independently
    @return IPCMDataSource* exact copy of this object 
    **/
	virtual IPCMDataSource* CopySelf() = 0;

	/**
    @brief
      Deep cppy the specified IPCMDataSource into the object

      This method is needed by the Lipsync Tool. It's a funky little beast that, given
	  an IPCMDataSource* will create a new IPCMLDataSource from it.
      @param pSrc - [in] source for the copy operation
	  @note Applications should only use this on IPCMDataSource* created by
	    CreatePCMDataSource.
        
    **/
	virtual serror CopyFrom(IPCMDataSource* pSrc) = 0;

	/** 
    @brief
	This method is used to Release or destroy this object. 
    It is a corrallary to DestroyPCMDataSource
    **/
	virtual void  Release() = 0;
};


/**
@brief This dll function creates an IPCMDataSource From a file
@ingroup lipsync_audio

Create the PCMData source given an audio file, the desired samplerate/size,
a progress meter (can be NULL). It returns either an error if failed or cancelled
or an instantiated data source.
@param szFile - [in] input wav file
@param resampleTo - [in] resampling (destination wave format)
@param pProgress - [in] progress meter or NULL
@param IPCMDataSource - [out] constructed data source or NULL on failure.
@return serror code \ref error_codes
**/
LIBLIP_API 
serror CreatePCMDataSource(const char* szFile, PCMStreamFormat& resampleTo,
						   CProgress* pProgress, IPCMDataSource **ppOutDataSource);


/**
@brief This dll function create a data source from an ANNO stream.
@ingroup lipsync_audio

This allows applications to define and use their own audio format (by implementing
    the anno bridge).
    @param inStream - [in] input audio stream
    @param pProgress - [in] progress bar
    @param ppOutDataSource - [out] instantiated IPCMDataSource
    @return kNoError or one of \ref error_codes
**/
LIBLIP_API
serror CreatePCMDataSourceFromAnnoStream(IAnnoPCMStream* inStream, CProgress* pProgress,
										IPCMDataSource **ppOutDataSource);

/**
@brief
This dll function is used to destroy a pcm data source.
@ingroup lipsync_audio
@see 
- CreatePCMDataSource
- CreatePCMDataSourceFromAnnoStream 
- IPCMDataSource::Release
**/
LIBLIP_API
void DestroyPCMDataSource(IPCMDataSource* pDataSource);

/**
@brief This class bridges the data source and the IAnnoPCMStream interface.
@ingroup lipsync_audio

This bridge allows data sources to be used as audio sources for lipsync.
The Lipsync Tool uses this class to hook up data sources and parts
of these files for lipsync and partial lipsync
*/
class IPCMDataSourceStream : public IAnnoPCMStream
{
protected:
	/// protected destructor. use ::DestroyPCMDataSourceStream
	virtual ~IPCMDataSourceStream() {};	
public:
	/**
    @brief
	    This method is used to set the recognition extent in frames. 

    This can be used to control the which audio data is lipsync'd and
	allows for partial lipsync.
    @param startFrame - [in] start frame of partial recognition
    @param numFrames - [in] number of frames to process
    **/
	virtual void SetPartialRecognition(long startFrame, long numFrames) = 0;

	/**
    @brief
    This method is used to set or change the data source of the object.

    This is how the data source is hooked into the IAnnoPCMStream interface
    for use in lipsync.
    @param itsPCMSource - [in] the IPCMDataSource to use in lipsync.
    @note - a pointer is maintained not a copy. Lipsync should only
    be done while the IPCMDataSource is still constructed **/
	virtual void SetDataSource (IPCMDataSource *itsPCMSource) = 0;

	/**
    @brief 
    IAnnoPCMStream override. @see IAnnoPCMStream::GetFormat
    **/
	virtual serror GetFormat(PCMStreamFormat* pFormat) = 0;

	/**
    @brief
    IAnnoPCMStream override. @see IAnnoPCMStream::GetNumBytes
	
    In this implementation the number of bytes can be controlled
	by IPCMDataSourceStream::SetPartialRecognition. If partial recognition
    is turned off, the number of bytes will be the total size of the
    recognition stream
	@see IPCMDataSourceStream::SetPartialRecognition
    **/
	virtual ulong GetNumBytes() = 0;

	/**
    @brief IAnnoPCMStream override. @see IAnnoPCMStream::ReadBytes 
    **/
	virtual long ReadBytes(void *pData, long nBytes) = 0;

	/**
    @brief IAnnoPCMStream implementation. see IAnnoPCMStream::Start
    **/
    virtual serror Start () = 0;    /* reset the reader */

	/// This method is used to destroy the object
	virtual void Release() = 0;

};

/**
@brief
This dll function is used to create an IPCMDataSourceStream given an instantiated IPCMDataSource object.
@ingroup lipsync_audio

@param pSourceObject - [in] the source pcm object
@param ppOutStream	 - [out] the newly created data source stream
@return serror - kNoError or appropriate error code.
@see IPCMDataSource, IPCMDataSourceStream, DestroyPCMDataSourceStream
**/
LIBLIP_API serror CreatePCMDataSourceStream(IPCMDataSource* pSourceObject, // [in] can be NULL
											IPCMDataSourceStream** ppOutStream // [out]
											);
/**
@brief
This dll function is used to destroy an IPCMDataSourceStream.
@ingroup lipsync_audio

It is the same as calling IPCMDataSourceStream::Release.
@param pStream - [in] the object to destroy
@return serror - kNoError or appropriate error code.
**/
LIBLIP_API serror DestroyPCMDataSourceStream(IPCMDataSourceStream* pStream);

/**
@brief Spectral information reader used by lipsync tool
@ingroup lipsync_audio

This class is used by the Lipsync Tool UI to draw spectral representations
of the audio data. It's not needed unless applications which to draw similar
spectragraphs as the Lipsync Tool. It is provided in the API since the Lipsync Tool
source code has been made available for licensing
**/
class ISpectralDataSource 
{
protected:
	/// protected destructor
	virtual ~ISpectralDataSource() {}
public:
    /// get the total number of spectral frames 
    virtual slong  GetNumFrames () = 0;
    /// get the ms per frame (the "width" of a frame) 
    virtual float  GetMsPerFrame () = 0;    
    
	/// Perform an FFT on the specified pcm data 
    virtual float PerformMagitudeFFT (sshort *pBuffer, long nFFT, bool bF0, long& f0) = 0;
	/// Perform an FFT on the specified pcm data, but return floating point values 
    virtual void PerformMagitudeFFT1 (float *pBuffer, long nFFT) = 0;

    /// calculate the spectral mean of the specified buffer
	virtual void CalculateFFTMean(long nFFT, sshort* pBuffer, IPCMDataSource *pSrc, CProgress* pProgress) = 0;

	/**
    @brief
    turn on/off mean subtraction
	@param bMean - [in] true - turn on mean. false - turn off mean subtraction
	@param pMean - [in] the mean buffer.
	@param nFFTMean - [in] size of the mean buffer
	@note: mean buffering is enabled on PerformMagnitudeFFT
	and only if the fft sizes match. Default is off **/
	virtual void SetMeanSubtraction(bool bMean, sshort *pMean, long nFFTMean) = 0;

	/// destroy the object
	virtual void Release() = 0;
};

/**
@brief
This dll function is used to create a spectral data source object.
@ingroup lipsync_audio
@param ppOut - [out] the instantiated ISpectralDataSource
@return serror - kNoError or appropriate error code.
**/
LIBLIP_API serror CreateSpectralDataSource(ISpectralDataSource** ppOut);

/**
@brief
This dll function is used to Destroy a spectral data source object
@ingroup lipsync_audio

@param pSrc - [in] The instantiated ISpectralDataSource.
**/
LIBLIP_API void DestroySpectralDataSource(ISpectralDataSource* pSrc);

/**
@brief 
This dll function will calculate the true decibalic power of a buffer of audio. 
@ingroup lipsync_audio

@param p - [in] buffer of pcm data of length at least [n]
@param ioprev - [in,out] - the previous last buffer of audio (last frame) analyzed by CalcDecibalPower.
				   when completed, this function will fill in ioprev with an appropriate value. 
                    Initiatialize to zero.
@param n - [in] The number of bytes to use in the calculation.
@return float - the decibalic power of the signal. Between 0 and 127 usually	
**/
LIBLIP_API float CalcDecibalPower(float *p, float& ioprev, long n);

#endif
