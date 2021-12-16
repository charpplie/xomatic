/* 
	liblipsync.h 

	Copyright (c) 2003 Annosoft, LLC. Garland, Texas. All rights reserved.     
	This program and header file contains the confidential trade secret 
	information of Annosoft, LLC.  Use, disclosure, or copying without 
	written consent is strictly prohibited. 

	This header file is used to access annosoft's proprietary Lipsync libraries,
	use of this file or the library files is prohibited without prior approval
	from annosoft.com and the acceptance of the appropriate license agreement. 
	Any internal copies of this file must be accompanied by the appropriate license
	agreement.
*/

/**
@file liblipsync.h	
  
@brief
		This is the main header file for using the Lipsync SDK. It declares classes,
		structures, and functions used to perform lipsync operations.
	
		This file declares the surface level interfaces and types for liblipsync. 
		The surface level interfaces are implemented as a bridge pattern between
		the internals of liblipsync and a fairly clean external interface

		This file contains a variety of classes, functions, and structures for
		performing lipsync and setting options which can change the lipsync results.
    
*/


#ifndef _H_LIBLIPSYNC
#define _H_LIBLIPSYNC

/// the version of the liblipsync.h header
#define lipsync_sdk_version "3.3.05.00"

////////////////////////////////////////////////////////////////////////
// types
////////////////////////////////////////////////////////////////////////
/** @defgroup simple_types Simple Types
	@brief These are the type declarations for integer, byte, error, and other
	simple types used in the SDK.

	Annosoft uses it's own declarations for integer types to avoid platform
	and configuration issues.
	@{
*/

/** alternate to void */
typedef void          *pvoid;	
/** alternate to signed long */
typedef long           slong;	
/** alternate to unsigned long */
typedef unsigned long  ulong;	
/** alternate to unsigned short */
typedef unsigned short ushort;	
/** alternate to signed short */
typedef short          sshort;	
/** alternate to unsigned char */
typedef unsigned char  byte;	
/** alternate to signed char */
typedef signed char    schar;	
/** alternate boolean */
typedef sshort         Bool;    
/** error code type
	@see error_codes \ref error_codes */
typedef slong          serror;  
/** signed fixed 15-16   */                                
typedef slong          sfixed;  
/** unsigned fixed 16-16  */
typedef ulong          ufixed;
/** @} */ // end of simple types group
  


#define LIBLIP_API extern "C"


////////////////////////////////////////////////////////////////////////
// macros
////////////////////////////////////////////////////////////////////////
/**@defgroup anno_macros SDK Macros
   @brief Annosoft defines a few macros for various purposes

   Primarily, the macros are for math related to fixed point typews
   used in sample rates.
   @{
 */

/// Converts a 16|16 fixed point type into a double type
#define FixToDouble(f)	((double)(f)/65536.0)

/// Converts a double or float type into a 16|16 fixed point type
#define DoubleToFix(f) ((ufixed)((f)*65536.0))

/// Converts a fixed point value into a long
#define FixToLong(f)	((f)>>16)

/// converts an integer or long type into a fixed point value
#define Fix(f)			((f)<<16)

/** @} */ // end of macros

/** @defgroup error_codes SDK Error Codes
	@brief A list of error codes returned by SDK methods

	The SDK functions and methods often return error codes of
	type serror.
	
	@{
*/
/** @brief: success. The operation succeeded.    */
const serror kNoError      = 0;     
/** @brief: no memory error                      */
const serror kErrMemory    = 1;     
/** @brief: argument to function is invalid      */
const serror kInvalidArg   = 2;     
/** @brief: a specified argument is out of range */
const serror kOutOfRange   = 3;     
/** @brief: unknown, possibly internal error     */
const serror kErrUnknown   = 4;     
/** @brief: operation was cancelled by the user  */
const serror kErrCancelled = 5;     
/** @brief: not implemented                      */
const serror kNotImplemented = 6;   
/**	@brief: internal error (bug)				*/
const serror kErrInternal   = 7;    
/** @brief indicates that the virterbi beam failed 
	the returned phoneme list is incomplete 

	This error occurs in text based lipsync when the
	text could not be statistically aligned with the audio.

	The program still generates markers that may be valid to some 
	extent, but the user should take a look at the text transcript
	and fix errors.
*/
const serror kErrViterbiFail = 8;	
/** @brief: Can't open the file because it is locked */
const serror kErrFileLocked =  9;	
/** @brief: Can't open the file because it was not found */
const serror kErrFileNotFound = 10; 
/** @brief: Can't open the file with the specified permission. File may be in use */
const serror kErrFilePermission =  11;   
/** @brief: Can't open the file because it is an invalid format */ 
const serror kErrFileInvalidFormat = 12; 
/** @brief: compressed audio is not handled by the engine */
const serror kErrFileCompressedFile = 13;
/** @brief: a call is out of order, something needs initialization */	
const serror kErrNotInitialized =	14; 
/** @brief: an audio device error */
const serror kErrNoAudioDevice	=   15;	
/** @brief: user is not licensed to use the specified library */
const serror kErrNotLicensed    =   16;	
/** @brief: no disk space, write failed */
const serror kErrNoDiskSpace    =   17;	
/** @brief: no TTS Engine was found */
const serror kErrNoEngine		= 18;	
/** @brief: can't find the named tts voice */
const serror kErrVoiceNotFound	= 19;	
/** @brief: error occurred while processing the text */
const serror kErrProcessingText = 20;	
/** @brief: the model or lexicon file is newer than this program */
const serror kErrFileNewerVersion = 21;	
/** @brief: a crash was detected (debugging support) */
const serror kErrFatal          = 22;   
/** @brief: last error id */
const serror kErrLipsyncLast	=   23;	

/** @} */ // end of error codes

///////////////////////////////////////////////////////////////////////////////
// Name: GetLipsyncErrorMessage
/**@ingroup sdk_functions
   @brief given an serror error id, retrieve an english string describing the error.
   @param err - [in] serror, the error code to get an english version of.
*/
LIBLIP_API const char* GetLipsyncErrorMessage(serror err);

///////////////////////////////////////////////////////////////////////////////
// CProgress
/** @ingroup app_interfaces
	@brief The CProgress interface is used to provide feedback during long operations.	

    This is an optional interface that should be subclassed and implemented 
    by the client application.
	
	CProgress interfaces are passed into the recognition routines and other long
	operations. 

	The SDK will periodically call the interface (if specified) 
	to update it's progress display for the user.
	
	@see CMyAppProgress
*/
class CProgress
{
public:
	
	// CProgress::SetMessage 
	/** @brief
	    The lipsync library will call this to change the message text of 
	    the progress dialog. 
	   
	  	@param szMessage - [in] the text of the message to change
	*/
    virtual void SetMessage (char *szMessage) = 0;
	
	
	// CProgress::UpdateAndCheck. 
	/** @brief Called by the framework to notify of progress and to give the
	   application an opportunity to cancel the operation.
	  
	   Description
	   The lipsync library will call this periodically and specify a 
	   current value and a maximum value so that the application may update
	   a progress meter.

	   Client applications should update their progress user interface based	
	   on this information.

	   the % complete can be calculated by (nValue * 100)/nMax
	   this method returns (true) if the user has cancelled the operation
	   a return of true will cause the service to end and a return value of kErrCancelled
	   to be issued.
	  
	   @param nValue - [in] the current value of progress. nValue/nMax
	   @param nMax - [in] maximum value possible in this process.
	  
	   @return
	  	 true: if the user wants to cancel the operation
	     false: if processing should continue
	*/
    virtual bool UpdateAndCheck (long nValue, long nMax) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// Creating Models

// CAccousticHMM class
/**
	@class CAccousticHMM
    @ingroup sdk_interfaces
	@brief A statistical model class needed for lipsync.
	
	The CAccousticHMM class is a fundemental class for using the annosoft Lipsync. 
	Speech models, HMMs, are defined externally from the lipsync code. 
	They are defined in files with extention .hmm and are included in 
	the /models/ directory of the SDK install.

	The application will need to create an HMM interface using 
	the CreateAccHMM function. 
	This will return an interface to the CAccousticHMM. 

	This is a opaque interface. Applications do not need to make any 
	setup or calls against this object. Applications must create and 
	destroy these objects using CreateAccHMM and DestroyAccHMM, and pass it, 
	as appropriate, into the methods of the various recognition interfaces 
	or functions.
	@see CreateAccHMM, DestroyAccHMM, \ref hmm_file_format
*/
class CAccousticHMM;	// blind declaration

#define CLipSyncAccousticHMM  CAccousticHMM

///////////////////////////////////////////////////////////////////////////////
// Name: CreateAccHMM
/** 
   @ingroup sdk_functions
   @brief The CreateAccHMM function is used to load a speech model
   (*.hmm) from a buffer.
   
	The CreateAccHMM function is used to load a speech model
   (*.hmm) from a buffer.
   
   Annosoft externalizes these speech models into a proprietary
   format. These files are located in the models directory of
   the SDK and have the extension .hmm. The SDK does not read
   the files directly but rather allows the calling application
   to pass in a buffer of the file's data. This could just as
   easily be a resource in the application (The samples do
   both).
   
   This function is used to create a CAccousticHMM* from
   such a buffer. If it fails to create the object, it sets the
   error in szError, and returns kErrInvalidFormat.
   
   @param pBuffer :       [in] pointer to a hmm data buffer
   @param nBufferBytes :  [in]the number of bytes contained in pBuffer
   @param szError :       [in,out] a buffer of at least 256 characters. This
                   will contain a text description of the error if an
                   error occurs 
   @param ppHMM :         [out] an HMM that can be passed to the recognition
                   routine.
   @return
   serror : \ref error_codes for the HMM failure or kNoError for
   success.
   @note
   It is safe to dispose of pBuffer anytime after this routine
   is called. the CAccousticHMM maintains no reference to it.
   when finished with the CAccousticHMM call DestroyAccHMM
   @see
   - \ref hmm_file_format
   - DestroyAccHMM, 
   - ITextlessPhnRecognizer::RecognizePhonemes,ITextBasedPhnRecognizer::RecognizePhonemes,
   - CreateRealtimePhnRecognizer, 
   - LoadTrigramIntoHMM, 
   - \ref sdk_functions,
   - snippet_textbased_lipsync, snippet_textlesslipsync
*/
	
LIBLIP_API serror CreateAccHMM(char* pBuffer, long nBufferBytes, char* szError, CAccousticHMM**ppHMM); 

///////////////////////////////////////////////////////////////////////////////
// Name: DestroyAccHMM
/** @ingroup sdk_functions
	@brief This method is used to destroy an HMM created by the method CreateAccHMM
	it is safe to call this method with a NULL pHMM.

   @param pHmm - [in] object to destroy. can be NULL.
   @see 
   - CreateAccHMM -
   - CAccousticHMM, 
   - snippet_textbased_lipsync, 
   - snippet_textlesslipsync, \ref hmm_file_format
*/
LIBLIP_API void DestroyAccHMM(CAccousticHMM *pHmm);

////////////////////////////////////////////////////////////////////////////////
// Name: LoadTrigramFileIntoHMM
/**@ingroup sdk_functions
   @brief This function loads a trigrammer (.trig) file into an HMM.
   
   This function loads a trigrammer language model from a file.
   into the specified HMM. This is useful for Textless
   recognition only. The language model provides an
   extra level of refinement for the phonemes by removing
   unlikely triplets from the calculation
   
   @param pHmm - [in] Instantiated Hmm in which to load the trigrammer
   @param szTrigramFile - [in] Trigrammer disk file
   @param bTrustNumbers - [in] set to false. If the numbers are sorted a certain way
   set to true, right now, just set to false.
   @return
     serror - kNoError or appropriate \ref error_codes
   @see
     snippet_textlesslipsync
*/
LIBLIP_API serror LoadTrigramFileIntoHMM(CAccousticHMM* pHmm, const char* szTrigramFile, bool bTrustNumbers);

////////////////////////////////////////////////////////////////////////////////
// Name: LoadTrigramIntoHMM
/**@ingroup sdk_functions
   @brief This function loads trigrammer (.trig) data into an HMM.
   
   This function loads a trigrammer language model
   into the specified HMM. This is useful for Textless
   recognition only. The language model provides an
   extra level of refinement for the phonemes by removing
   unlikely triplets from the calculation
   
   @param pHmm - [in] Instantiated Hmm in which to load the trigrammer
   @param szData - [in] Trigrammer data buffer. This can either be loaded from file or resource
   @param dataSize - [in] - size in bytes of szData (the Trigrammer data buffer)
   @param bTrustNumbers - [in] always set to false
   @return
     serror - kNoError or appropriate \ref error_codes
*/
LIBLIP_API serror LoadTrigramIntoHMM(CAccousticHMM* pHmm, const char* szData, long dataSize,  bool bTrustNumbers);

////////////////////////////////////////////////////////////////////////////////
// Name: LoadPhoneConstraintsFileIntoHMM
/**@ingroup sdk_functions 
   @brief This is an additional constraint system much like the trigrammer
    which controls phone transitions. 
   
   The constraints system adds additional controlling information
   to the HMM phoneme recognizer. 
   The constraints file specifies phoneme controls on phoneme durations
   A simple example is to consider putting maximums on m,n,p, etc
   These phonemes in spoken speech are almost never articulated
   for durations passing 1/8th of a second for instance. 
   By adding such a constraint, the recognizer is forced into paths
   which don't allow these durations. Since it is embedded in the
   statistical mix. The dynamic program will choose "the next best thing"
   which is likely better than a long 'm' or 'n'. Constraints
   can also improve performance, but this is secondary to quality.
   Set "flags" to zero, reserved for future use.

   This is used in textless and text based lipsync.

   @param pHMM - [in] the hmm to assign the constraints to
   @param szFile - [in] file formatted like \see duration_constraints_format
   @param flags - [in] reserved for future use. must be zero
   @return kNoError or appropriate \ref error_codes
*/

LIBLIP_API serror LoadPhoneConstraintsFileIntoHMM(
	CAccousticHMM* pHmm, // [in] the hmm
	const char* szFile, 
	ulong flags			// [in] flags - set to zero
	);

////////////////////////////////////////////////////////////////////////////////
// Name: LoadPhoneConstraintsIntoHMM
/**@ingroup sdk_functions
   @brief this function is used to load phoneme duration constraints into the specified
   hmm.
   
   The constraints system adds additional controlling information
   to the HMM phoneme recognizer. 
   The constraints file specifies phoneme controls on phoneme durations
   A simple example is to consider putting maximums on m,n,p, etc
   These phonemes in spoken speech are almost never articulated
   for durations passing 1/8th of a second for instance. 
   By adding such a constraint, the recognizer is forced into paths
   which don't allow these durations. Since it is embedded in the
   statistical mix. The dynamic program will choose "the next best thing"
   which is likely better than a long 'm' or 'n'. Constraints
   can also improve performance, but this is secondary to quality.
   Set "flags" to zero, reserved for future use.

    This is used in textless and text based lipsync.

   To clear phoneme constraints from an HMM, pass in NULL in szData.

   @param pHMM - [in] the hmm to assign the constraints to
   @param szData - [in] buffer of data formatted like \see duration_constraints_format
    It is safe to pass in a null pointer.
   @param dataSize - [in] size fo the data.
   @param flags - [in] reserved for future use. must be zero
   @return kNoError or appropriate \ref error_codes
*/
LIBLIP_API serror LoadPhoneConstraintsIntoHMM(CAccousticHMM* pHmm, const char* szData, 
	long dataSize, ulong flags);


///////////////////////////////////////////////////////////////////////////////
// Audio Stream Interfaces.
// In order to perform recognition, an audio stream must be created.
// Applications can use audio streams

/** @ingroup sdk_structures
	@brief Platform independent audio format structure
	
	This structure is used by the SDK as a bridge between various 
	audio format structures that may be platform independent. 
	
	It is used to communicate audio format information so that
	the PCM audio data can be correctly parsed by frequency domain
	audio classes.
	@see IAnnoPCMStream::GetFormat
*/
typedef struct PCMStreamFormat
{
	/** sampling rate of the stream in 16-16 fixed point */
    ufixed  sampleRate;             
	/** bit size of sample. acceptable values are 8 or 16 */
    ushort  sampleSize;             
	/** number of channels in the audio */
    ushort  numChannels;            
} PCMStreamFormat;


////////////////////////////////////////////////////////////////////////////////
// IObservationStream
/**	@ingroup sdk_interfaces
	@brief The IObservationStream class connects your audio with the Lipsync process. 

   This IObservationStream class is a fundemental class for using the annosoft lipsync.
   There are two ways to create observation streams. Either from a RIFF wav file using 
   CreateObservationStreamFromAudioFile, or from your own audio source using 
   CreateObservationStreamFromAnnoStream. In the later method, applications will subclass 
   IAnnoPCMStream to create their own PCM audio data source.
  
   Applications can also use ::AddAudioFileFormatReader to make various formats available
   and then use ::CreateObservationStreamFromAudioFile to create observation streams
   in their format without having special cases.
   
   Applications can IObservationStream as a black box and probably will not need to access it's
   methods directly.
  
   @note Although methods are defined for IObservationStream in liblipsync.h, 
	applications probably don't ever need to call these methods. 
   @see CreateObservationStreamFromAudioFile, CreateObservationStreamFromAnnoStream, AddAudioFileFormatReader
    snippet_textlesslipsync, snippet_textbased_lipsync
*/
class IObservationStream
{
protected:
	// use DestroyObservationStream
	virtual ~IObservationStream() {}		
public:
	/*** @brief get the number of frames from the observation stream. [internal use]
	  
	   This is the number of observations which is the Total Audio Samples/(FFTSize - OverLap samples)
	   
	   For all the statistical models available, this number is the number of 10 millisecond
	   frames in the audio, rounded up to the nearest 10 millisecond interval.
	   @returns number of frames in the observation stream
	*/
	virtual slong GetNumFrames() = 0;

	/** @brief Read an observation. [internal use]

		This method reads a slice of observation data. usually mel frequency cepstral coefficients
		but not necessarily
		@param pBuffer - [out] fills the buffer with observation data
		@param energy - [out] the energy value of the observation
		@returns kNoError if successful, or appropriate \ref error_codes
	*/
	virtual serror ReadObservation(float *pBuffer, float& energy) = 0;

	/** @brief return the frequency transformation information. [internal use]
		
		this is used internally and probably never needed by external application
	  */
	virtual void   GetWindowSizeAndOverLap (slong *pnFFT, slong *pOverlap) = 0;

	/** @brief the sample rate of the observation stream 
		
		22100.00 for instance. 
	*/
	virtual double GetSampleRate() = 0;
	
	/** @brief [internal use] return the minimum and maximum energies of the observation stream. 
	// 
	   This is non-causal. that is, it will only return the {min,max} of the 
	   observations it has processed.
	*/
	virtual void   GetMiniMaxEnergies(float& /*out*/minE, float& /*out*/maxE) = 0;

	
	// flags - generic processing flags
	// Applications can set these flags
	// or they can be pulled from the ISyncResultsCollection flags
	// ISyncResultsCollection wins, but, if the features are iterated
	// without recognition, then this flag can be set.
	enum obs_flags
	{
		flag_calcPitch = 0x01,	// not yet supported
		flag_dbEnergy = 0x02	// return energies on the decibalic scale
	};

	// Set the flags for IObservationStream
	virtual void SetFlags(ulong flags) = 0;
	
};

////////////////////////////////////////////////////////////////////////////////
// Name: CreateObservationStreamFromAudioFile 
/**@ingroup sdk_functions

   @brief: This function is used to create an IObservationStream 
	given a path to your audio file. 

For both textless and text based lipsync, it is necessary to create an observation stream. 
The observation stream serves as the data source for the recognition engine. The 
IObservationStream is how the application connects their audio file/source with 
the lipsync recognizer.

In order to create an observation stream, it is necessary to select and create 
the appropriate HMM. see CreateAccHmm for details on loading an HMM. 
This is a precondition to calling CreateObservationStreamFromAudioFile.


@param	szFile - [in] file path
@param	pHMM   - [in] the hmm
@param	ppStream - [out] pointer to an observation stream object.

@return
	kNoError: successful
	kErrInvalidFormat: the audio file is not a valid audio format
	kErrFileCompressedFile: the audio file is a compressed RIFF WAVE
	kErrFileNotFound: the specified file doesn't exist or can't be opened for reading

@see DestroyObservationStream, CreateObservationStreamFromAnnoStream,ITextlessPhnRecognizer::RecognizePhonemes
ITextBasedPhnRecognizer::RecognizePhonemes, snippet_textlesslipsync, snippet_textbased_lipsync, snippet_IInstallableAudioFileReader
*/
LIBLIP_API serror
CreateObservationStreamFromAudioFile (const char *szFile, CAccousticHMM* pHMM, 
									  IObservationStream** ppStream);

////////////////////////////////////////////////////////////////////////////////
// Name: DestroyObservationStream
/**@ingroup sdk_functions
   @brief destroys an IObservationStream
   
   The DestroyObservationStream function is used to destroy an IObservationStream created 
   by CreateObservationStreamFromAudioFile or CreateObservationStreamFromAnnoStream.
  
   Call this method when recognition has been finished or aborted. 
   
   @param	pObs - [in] The obs stream
   @see 
  	DestroyObservationStream, CreateObservationStreamFromAnnoStream,ITextlessPhnRecognizer::RecognizePhonemes
  	ITextBasedPhnRecognizer::RecognizePhonemes, snippet_textlesslipsync, snippet_textbased_lipsync
*/
LIBLIP_API
void DestroyObservationStream(IObservationStream* pObs);


///////////////////////////////////////////////////////////////////////////////
// Name: class IAnnoPCMStream
/** @ingroup app_interfaces
   @brief An abstract class allowing applications to implement their own audio readers
   
   The IAnnoPCMStream is an abstract base class which can be subclassed by applications 
   to implement their own audio sources for RecognizePhonemes, either textless or text based.
   
   This allows applications to add new audio formats without changes to the 
   audio library, or to process audio data in from a buffer instead of a file. 
   To use, applications should implement/subclass this interface and then pass in your object 
   to CreateObservationStreamFromAnnoStream.
   @see snippet_annostream, CMp3LipsyncAudioSource
*/
class IAnnoPCMStream
{
public:
	/**
	   @brief called by SDK to get the format of the audio data
	   
	   This method is called by the lipsync library to retrieve the PCMStreamFormat 
	   for the audio data that will be read
	  
	   Subclasses will implement this.
	  
	   
	  	@param pFormat - [out] the format data.
	    @return
	    serror - kNoError or an appropriate \ref error_codes
	*/
	virtual serror GetFormat(PCMStreamFormat* pFormat) = 0;

	/** 
	   @brief: This method returns the number of bytes of data in the PCMStream
	  
	   This should be the number of bytes of PCM Audio Data
	   available via the "Read" method.
	   @return ulong number of bytes in the audio
	*/
	virtual ulong GetNumBytes() = 0;

	/**
	   @brief reads the next buffer of data. This is implemented by subclasses
	  
	   Applications should keep an internal cursor
	   that identifies where in the file to read from
	   and read from there.
	  
	   
	  	@param pData - [in|out] [in] a buffer at least nBytes in size. 
	  			[out] pData should be filled with as much audio data as could be read.
	    @param nBytes - [in] the maximum number of bytes allowed to read. 
	  				  Effort should be made to return the exact number of bytes 
	  				  unless the end of file is reached. in this case, method should return 
	  				  as many as could be read before the end of data.
	   @return
	  	The actual number of bytes read, or 0 if no bytes could be read.
	*/
	virtual long ReadBytes(void *pData, long nBytes) = 0;
	
	/**
	   @brief This method is called to instruct the subclass that the Lipsync process is about to begin. 
	   
	   The application should reset the cursor or re-initialize the audio source.

	   Some applications cannot fully support this method, but if possible, it should be implemented
	   because it offer the flexibility of doing multipass operations on the lipsync tool
	   
	   Applications should make an attempt to support this. Currently its not
	   necessary (just called once), but it is possible in the future that annosoft
	   may want to make multiple passes on the audio source

	   @return serror - kNoError or appropriate error code.
	*/
	virtual serror Start() = 0;

};

/////////////////////////////////////////////////////////////////////////////
// Name: class IAnnoPCMStream2
/** @ingroup app_interfaces
    @brief Abstract class allowing applications to support their own audio readers
  	through AddAudioFileFormatReader
   
   An abstract base class which can be subclassed by applications to implement 
   their own audio sources for RecognizePhonemes, either textless or text based. 
   This interface is a subclass of IAnnoPCMStream. It adds methods to support reference counting 
   so that streams can be passed around and memory can be freed when operations are finished.
  
   This is used to support the pluggable audio reader implementation in the Lipsync tool via 
   IInstallableAudioFileReader and the SDK C function AddAudioFileFormatReader.
  
   This allows applications to install custom audio file type readers at 
   the beginning of the program and then use CreateObservationStreamFromAudioFile 
   to open these custom formats in the same way that WAV files are opened, 
   thus avoiding the need for special cases   (calling either 
   CreateObservationStreamFromAudioFile or CreateObservationStreamFromAnnoStream). 
   Both methods are supported, but for larger applications with multiple file formats, 
   this using this method is probably more straightforward.
  
   @see IInstallableAudioFileReader, AddAudioFileFormatReader, snippet_IInstallableAudioFileReader, CMp3LipsyncAudioSource
*/
class IAnnoPCMStream2 : public IAnnoPCMStream
{
public:
	// Name: AddRef
	/** @brief
        Increment the reference count for the object
       
        Applications implement this method by implementing a reference counting
        system where their subclass cleans itself up when Release() is called and
        the reference count is equal to zero
	   @return
	  	 reference count after the add ref is called
	*/
	virtual long AddRef() = 0;
    //   Name: Release
    /** @brief   
        Decrement the reference count for the object
       
        Applications implement this method by implementing a reference counting
        system where their subclass cleans itself up when Release() is called and
        the reference count is equal to zero.
		@return
	  	 reference count after the release is done
	*/
	virtual long Release() = 0;
};


/////////////////////////////////////////////////////////////////////////////
// Name: CreateObservationStreamFromAnnoStream
/**@ingroup sdk_functions 
   @brief This DLL function is used to create an IObservationStream for use in Lipsync,
   given an IAnnoPCMStream subclass.
   
   This function is used to create an IObservationStream for use
   in Lipsync.
   
   Unlike the CreateObservationStreamFromAudioFile function,
   with this function, the application supplies an interface
   from which to read the audio data.
   
   This allows applications to use their own audio formats, or
   use an in memory rendering of the audio, instead of the
   purely disk based RIFF WAV methods.
   
   To achieve this, applications will subclass IAnnoPCMStream
   and implement the interface. This subclass can then be
   instantiated and passed in to this function. The returned
   IObservationStream, when passed into the Recognizer, will
   read audio data from your IAnnoPCMStream subclass.
   
   In this way, annosoft can support custom audio formats, or
   non-disk based WAV lipsync.
   @note
   The IAnnoPCMStream is only referenced. It is not disposed of
   by LipsyncLib. It can either be a stack based object or an
   allocated object, but either way the calling application is
   responsible for maintaining the object over the life of the
   recognition process and disposing it (if applicable) when
   finished.
   
   @param pAnnoStream :  [in] a pointer to your instantiated IAnnoPCMStream
                  object.
   @param pHMM :         [in] pointer to the HMM. This is needed to correctly
                  identify the features that are needed.
   @param ppStream :     [out] pointer to an observation stream object.
   @return
   kNoError, kInvalidFormat, kErrFileCompressedFile,
   kErrFileNotFound                                                    
   @see snippet_annostream, IAnnoPCMStream
*/
LIBLIP_API serror
CreateObservationStreamFromAnnoStream (IAnnoPCMStream *pAnnoStream, CAccousticHMM* pHMM, 
									  IObservationStream** ppStream);



/////////////////////////////////////////////////////////////////////////////
// Name:CSyncMarker
/**@ingroup sdk_structures
   @brief CSyncMarker is the top level recognition result returned from
   the analyzer. All lipsync results from the annosoft libraries
   are currently returned in CSyncMarker lists.
   
   This class describes a lipsync event, a start, and stop time
   and various parameters. They are accessed through
   ISyncResultsCollection.
   @see _PrintPhonemeMarkers
*/
	class CSyncMarker
	{
	public:
        
        /// enum defining the types of markers available.
		enum 
		{
			/// marker type is a phoneme
			phoneme = 1,	
				/// marker type is a word (for text based only)
			word = 2,		
			/// marker type is a sentence marker (for text based only)
			sentence = 3,
			/// marker type is XML marker (for text based only)
			XML = 4,	
			/// marker type is an energy contour tag
			energy = 5,	
			/// for application use, end of hard coded identifiers
			marker_end = 7 
		} type;	

		/// normalized intensity of the signal (phoneme and energy only)	
		float		intensity;	
		/// start time for the marker
		long		milliStart;	  
		/// end time for the marker
		long		milliEnd;	  
		/** @brief The phoneme label 
        
            When the marker type is a phoneme, 
            The szPhoneme field contains the phoneme label which this marker represents. 
            This will be one of \ref phon_set. */
		char		szPhoneme[3]; 
		/** @brief marker label string
		
		- for instance:
		   - for type == phoneme, NULL
		   - for type == sentence, string identifying the punctionation
		   - for type == word, string identifying the word
		   - for type == userData, string identifying the user data text.
		*/
		char*		otherLabel;  
								
		// for word, sentence and XML with annotator, byte index into the text string
		slong		textPos;	
		// for word, sentence, and XML with annotator, length of data in the text string
		slong		textLength;	

		/** @brief raw intensity value
			if opt_intensity_as_morph is on, this is the original unmorphed
			signal strength. Applications may have a use for this
		 */
		float		rawIntensity;
		// not yet implemeneted
		float		pitch;		
	};

/////////////////////////////////////////////////////////////////////////////
// Name: ISyncResultsCollection
/**@ingroup sdk_interfaces
	@brief The results collection returned from lipsync operations.

   This class is a collection of CSyncMarker objects. The
   collection is is filled by the Lipsync operations and then
   accessed by the application through member methods
 
   The ISyncResultsCollection class is a fundemental class for
   using annosoft lipsync. Resultant lipsync information, such
   as the timing and label of phonemes and words, is accessed
   using the ISyncResultsCollection. 
   
   The ISyncResultsCollection is a collection of CSyncMarker
   objects. These objects specify timing and information about
   phonemes, energy, words that occur in the audio file. An
   ISyncResultsCollection object is passed into
   ITextlessPhnRecognizer::RecognizePhonemes(), ITextBasedPhnRecognizer::RecognizePhonemes(),
   and IRealtimePhnRecognizer::GetMarkersBeforeTime(). These
   methods will extract phoneme information from the audio file
   and store the results into the ISyncResultsCollection. <emit \<br/\>>

   The ISyncResultsCollection members are loosely modelled after
   STL container classes.
   @see
   CSyncMarker,CreateSyncResultsCollection,
   DestroySyncResultsCollection
   ITextlessPhnRecognizer::RecognizePhonemes,ITextBasedPhnRecognizer::RecognizePhonemes,
   IRealtimePhnRecognizer::GetMarkersBeforeTime, _PrintPhonemeMarkers, _PrintArticulationMarkers, snippet_textlesslipsync
*/

class ISyncResultsCollection
{
protected:
	// Prevent accidental "delete". Use DestroySyncResults
	virtual ~ISyncResultsCollection() {}
public:
	/** @brief control codes for various options available to for collection results. 
		opt_intensity_as_morph is recommended.
		
		These flags can be set by passing the flag into 
		CreateSyncResultsCollection, or they can be set through the 
		member methods of this interface.

		To have any effect, they must be set before the lipsync operation.
	*/	
	enum options
	{
		/// instructs the engine to output CSyncMarker::energy tags. 
		opt_energies = 0x01,			
		/** @brief This important flag changes the CSyncMarker::intensity value
			to be a better approximation of the mouth position for morphing with 
			a neutral mouth.

			This flag is recommended but is not the default, for backward compatibility.
			However, because it does a better job, it is highly recommended.
		*/
		opt_intensity_as_morph	= 0x02,	
		// Not yet implemented
		opt_pitch				= 0x04	
										
	};

	/**@brief changes the options
		
		This method sets the results options for the class.
		@param optFlags - [in] ISyncResultsCollection::options bitfield. can be:
			- ISyncResultsCollection::opt_intensity_as_morph 
			- ISyncResultsCollection::opt_energies,
		etc.
	*/
	virtual void		setOptions(ulong optFlags) = 0;
	/// returns the current set of options
	virtual ulong		getOptions() = 0;

	/** @brief Get the number of sync markers in the sync results collection
		
		After lipsync, this class contains a list of CSyncMarker objects. This
		method returns the number of CSyncMarker objects in this collection.
		@return the number of CSyncMarker objects in this collection. */
	virtual ulong		 size() = 0;
	///////////////////////////////////////////////////////////////////////////
	// iterator support
	///////////////////////////////////////////////////////////////////////////
	// begin - get the first sync marker, NULL if there are no sync markers
    /** @brief this method returns a pointer to the first CSyncMarker object
		in this collection. like any STL begin() method.
		@returns - a pointer to the first CSyncMarker or NULL if there are no
		CSyncMarker objects in this object.

		@see ISyncResultsCollection::end, _PrintPhonemeMarkers
	*/
    virtual CSyncMarker* begin() = 0;
	/** @brief this method returns the end marker like STL end().
		
		 This method is used get the end of the sync marker buffer. 
		 like STL this returns a pointer to one AFTER the last marker, thus end() is 
		 never a valid	marker.
		 
		 It may return NULL if there are no objects. In any case, if the list is empty
		 the return value from ISyncResultsCollection::begin will always be the same as
		 ISyncResultsCollection::end

		@returns - an invalid CSyncMarker, 1 past the last CSyncMarker object in the
		collection. Used for iteration.

		@see ISyncResultsCollection::end, _PrintPhonemeMarkers
	*/
  	virtual CSyncMarker* end() = 0;
	
	/** @brief empty the contents of this collection.

		This method can be used to clear the syncronization results, allowing
		the same container to be used multiple times */
	virtual void clear() = 0;

    /** @brief add a marker to the collection
	
		This method can be used to add a marker back into the collection.
		This is used by the app_sync_results (annoSDKUtil) to create phoneme
		markers from an internal representation.
		@param marker - [in] CSyncMarker to add. The data will be fully copied.
	*/
	virtual void add_marker(CSyncMarker* marker) = 0;

};

///////////////////////////////////////////////////////////////////
// Name: CreateSyncResultsCollection
/**@ingroup sdk_functions
   
   @brief Create a new ISyncResultsCollection object for use in audio recognition
   
   When finished, call DestroySyncMarkerCollection.

   For the options parameters, it is highly recommended that applications use
   ISyncResultsCollection::opt_intensity_as_morph. This means that
   intensity values will be normalized so that they can be used directly
   as a morph weight.
   
   @param options - [in] ISyncResultsCollection::options.
   @param ppCollection - [out] The resultant (empty) collection.
   @return serror - kNoError if successful otherwise an appropriate /ref error_codes.
  
   @see ISyncResultsCollection, DestroySyncResultsCollection,
   snippet_textlesslipsync, snippet_textbasedlipsync
*/

LIBLIP_API serror CreateSyncResultsCollection(ulong options, ISyncResultsCollection** ppCollection);

///////////////////////////////////////////////////////////////////////////////
// Name: DestroySyncResultsCollection
/**@ingroup sdk_functions
  
   @brief Destroys an ISyncResultsCollection object.
  
   This method destroys the ISyncResultsCollection and disposes of the
   memory.
   
   @param pCollection - [in] the collection to destroy
  
   @see CreateSyncResultsCollection, snippet_textlesslipsync, snippet_textbasedlipsync
*/
LIBLIP_API void DestroySyncResultsCollection(ISyncResultsCollection* pCollection);


////////////////////////////////////////////////////////////////////////////
// Name: ITextlessPhnRecognizer
/**@ingroup sdk_interfaces
   @brief interface for lipsync recognizer which doesn't require a text file of the audio, 
	but is not realtime. 

    It is created and destroyed using CreateTextlessPhnRecognizer and DestroyTextlessPhnRecognizer.
  
   @see snippet_textlesslipsync
*/
class ITextlessPhnRecognizer
{

protected:
	// protected destructor
	virtual ~ITextlessPhnRecognizer() {};		// use DestroyTextlessPhnRecognizer
public:

	
	////////////////////////////////////////////////////////////////////////////
	// Name: RecognizePhonemes
	/**@brief Call this to perform the lipsync/recognition process.
	  
	   This method performs the operation syncronously. It will periodically call 
	   the CProgress interface passed in to update the application progress (if not NULL)
	  
	   When finish the ISyncResultCollection interface will be populated with
	   phoneme markers representing the statistical best alignment given the audio stream
	   and speech model.
	   
	   @param pStream - [in] created with "CreateObservationStream"
	   @param pHMM - [in] the model file.
	   @param pProgress	- [in]application defined progress interface or NULL
	   @param pMarkerList - [out] - sync markers returned by the recognition process
	  
	   @return
	   kNoError		 - operation succeeded
	   kErrCancelled - User cancelled the operation
	   kErrMemory	 - the operation was not completed because of a memory allocation failure
       @see snippet_textlesslipsync
	*/
	virtual serror RecognizePhonemes(IObservationStream* pStream,
									 CAccousticHMM* pHMM,
									 CProgress* pProgress,
									 ISyncResultsCollection* pMarkerList) = 0;

};

///////////////////////////////////////////////////////////////////////////////
// Name: CreateTextlessPhnRecognizer
/**@ingroup sdk_functions
   @brief The CreateTextlessPhnRecognizer is used to instantiate a 
   lipsync object that can then be used to perform textless lipsync. 
   
   @param ppPhnRecog - [out] instantiated phoneme recognizer
   @return
   serror - kNoError or appropropriate error code of /ref error_codes
   @see ITextlessPhnRecognizer, DestroyTextlessPhnRecognizer, snippet_textlesslipsync
*/
LIBLIP_API serror CreateTextlessPhnRecognizer(ITextlessPhnRecognizer** ppPhnRecog);

///////////////////////////////////////////////////////////////////////////////
// Name: DestroyTextlessPhnRecognizer
/**@ingroup sdk_functions
   @brief The DestroyTextlessPhnRecognizer is used to destroy the recognizer object 
   after operations are completed.
   
   @param pPhnRecog - [in] phoneme recognizer to destroy
   @see ITextlessPhnRecognizer, CreateTextlessPhnRecognizer, snippet_textlesslipsync
*/
LIBLIP_API void DestroyTextlessPhnRecognizer(ITextlessPhnRecognizer* pPhnRecog);



///////////////////////////////////////////////////////////////////////////////
// Text based analysis
// Text based analysis requires more set up. In particular. A transcription class
// a dictionary class, and a G2P rule class


// this are defined in liptranscribe.h.

class ILipSyncTranscriber;

///////////////////////////////////////////////////////////////////////////////
// Name: CreateDefaultTranscriber
/**@ingroup sdk_functions
   @brief The CreateDefaultTranscriber function is used to create the default, 
   english language, ILipSyncTranscriber object required for text based lipsync.
  
  A transcriber, in Annosoft technologies, is something that converts a string 
  of text into a phonetical representation. The default transcriber uses the 
  Carnegie Mellon Pronunciation Dictionary as well as a letter-2-sound rule 
  system. To control disk size and to allow customers to change or update the 
  dictionary, it is not linked into the library. Applications should load the 
  dictionary into a memory buffer and pass that buffer to "CreateDefaultTranscriber".   
  This call will return a ILipSyncTranscriber object that can be passed to the 
  textbased recognizer, along with the text and observation stream.  
  
  For an example dictionary, see dicts/dict.txt. You are free to ship this 
  with your product, or modify it as you see fit. 

   @param pDict - [in] pointer to the dictionary data
   @param nBufferBytes - [in] size in bytes of the dictionary data
   @param ppTranscriber - [out] transcriber object
   @see DestroyTranscriber, ILipSyncTranscriber, CreateTranscriberFromLexiconFile
   ITextBasedPhnRecognizer::RecognizePhonemes,snippet_createtranscriber
*/
LIBLIP_API serror CreateDefaultTranscriber(char *pDict, long nBufferBytes, 
								 ILipSyncTranscriber**ppTranscriber );

///////////////////////////////////////////////////////////////////////////////
//	Name: DestroyTranscriber
/**
@ingroup sdk_functions
	@brief
	This DLL function is used to destroy an ILipsyncTranscriber object


	Destroys an ILipsyncTranscriber created with CreateDefaultTranscriber,
	CreateTranscriberFromLexicon, CreateTranscriberFromLexiconFile

	@see CreateTranscriberFromLexiconFile, CreateTranscriberFromLexicon,
		CreateDefaultTranscriber, snippet_textbased_lipsync, snippet_createtranscriber
*/

LIBLIP_API void DestroyTranscriber(ILipSyncTranscriber* pTranscriber);

///////////////////////////////////////////////////////////////////////////////
// Name: ITextBasedPhnRecognizer
/**@ingroup sdk_interfaces
	@brief This interface is used to do text|wav file alignment. 
	
	 In addition to producing phonemes it also produces CSyncMarker::word 
	 for each word and CSyncMarker::sentence for sentences, and 
	 also application specific XML embedded in text transcription. 

	@see CreateTextBasedRecognizer, DestroyTextBasedRecognizer, snippet_textbased_lipsync
*/
class ITextBasedPhnRecognizer
{
protected:
    // use DestroyTextBasedPhnRecognizer
	virtual ~ITextBasedPhnRecognizer() {};
public:

	///////////////////////////////////////////////////////////////////////////////
	// Name: RecognizePhonemes
	/** @brief This method is used to align words/phonemes/sentences/xml given 
		by the text, a transcription interface, and the audio observation stream.
	
	 Since this operation can take a long time, it will periodically call the 
	 CProgress interface passed in to update the application progress (if not NULL).

	When finished, it will populate the ISyncResultsCollection with phonemes,
	words, punct, and event markers (if any).

    If strText is set to null, the behavior is the same as ITextBasePhnRecognizer::AlignTranscription
    in that it is assumed that the text has already been transcribed and the CTxMarker objects
    are already available (ILipSyncTranscriber::Transcribe will do this).
  
	@param pStream - [in] IObservationStream (audio source)
	@param pHMM	- [in] the accoustic model (CreateAccHmm)
	@param strText - [in]text transcription of the Audio File
						if NULL, the transcriber must already 
						have a phonetical transcription
	@param pTranscriber - [in] interface created with "CreateTranscriber()"
	@param pProgress - [in] application defined progress interface or NULL
	@param pMarkers - [out] sync markers returned by the recognition process
	
	@return 
		- kNoError		 - operation succeeded
		- kErrCancelled - User cancelled the operation
		- kErrMemory	 - the operation was not completed because of a memory allocation failure
		- kErrViterbiFail - the recognition failed to align the string of phonemes with
		the text. out_PhnMarkers contains partial results and end at the location of the
		beam failure. Calling SetBeamWidth with a higher value *may* result in better
		recognition performance.

	@see ITextBasedPhnRecognizer,CreateTranscriberFromLexiconFile,
			CreateTranscriberFromLexicon, CreateAccHMM, IObservationStream, 
			ILipsyncTranscriber, ITextBasedPhnRecognizer::AlignTranscription, 
			snippet_textbased_lipsync

*/
	virtual serror RecognizePhonemes(IObservationStream* pStream,
									 CAccousticHMM* pHMM,
									 const char* strText, 
									 ILipSyncTranscriber* pTranscriber,
									 CProgress* pProgress,
									 ISyncResultsCollection* pMarkers) = 0;

	////////////////////////////////////////////////////////////////////////////////
	// Name: AlignTranscription
	/**@brief
	   A second method to align audio and text with more flexibility
	  
	   This is alternative to ITextBasedPhnRecognizer::RecognizePhonemes. Instead of supplying a 
	   character string, the application supplies a transcriber object, 
	   (see libtranscribe.h for details) which has already performed the 
	   letter to sound conversion.

	   The application may use the default transcriber or build their own.
	   The method will use the align the letter to sound results of
	   the transcriber to the specified audio stream.
	  
	   The SyncMarkers returned by this object contain phoneme, word, punctuation
	   markers, just as RecognizePhonemes does. However, it does not fill
	   in the otherLabel with the text. These values are set to NULL.
	   The application can access this information from the text fed into the
	   transcriber, along with CSyncMarker::textPos and CSyncMarker::textLength

	   This is more versatile than it's RecognizePhonemes counterpart because
	   it allows applications to create, for instance, a Unicode transcription service
	   since the results don't make any assumptions about the size of the
	   characters (this is handled by the transcriber and the application)
	   
	   @param pStream - [in] the observation stream
	   @param pHMM		- [in] the speech model
	   @param pTranscriber - [in] transcriber object (already converted the letter2phoneme
	   @param pProgress	- [in] progress interface
	   @param pMarkers		- [out] the generated markers

	   @return 
		- kNoError		 - operation succeeded
		- kErrCancelled - User cancelled the operation
		- kErrMemory	 - the operation was not completed because of a memory allocation failure
		- kErrViterbiFail - the recognition failed to align the string of phonemes with
		the text. out_PhnMarkers contains partial results and end at the location of the
		beam failure. Calling SetBeamWidth with a higher value *may* result in better
		recognition performance.	
	  
	   @see ITextBasedPhnRecognizer,CreateTranscriberFromLexiconFile
	  		 CreateTranscriberFromLexicon, CreateAccHMM, IObservationStream, 
	  		 ILipsyncTranscriber, ITextBasedPhnRecognizer::AlignTranscription
	*/
	virtual serror AlignTranscription(IObservationStream* pStream,
									 CAccousticHMM* pHMM,
									 ILipSyncTranscriber* pTranscriber,
									 CProgress* pProgress,
									 ISyncResultsCollection* pMarkers) = 0;

	

	///////////////////////////////////////////////////////////////////////////
	// Name: SetBeamWidth
	/**@brief This method changes the search width for text based lipsync.
	
	   This method is used to change the beam width used during recognition
	   A higher number makes the recognizer more robust against failure, but 
	   runs slower
	   The number is in 100ths of a second, that is
	   a width of 100 would mean 1 second
	   a width 1000 would mean 10 seconds
	   a width of 500 would mean 5 seconds
	   
	   @param width - [in] width in 100th of a second of the beam
	*/
	virtual void SetBeamWidth(long width) = 0;
	////////////////////////////////////////////////////////////////////////////////
	// Name: GetBeamWidth
	/**@brief This method retrieves the current beam width.
	   
	   The beam width defines the search space in 100th of a millisecond
	   The higher the beamwidth the broader the search, and the slower
	   the operation. The lower the beamwidth, the faster the operations but
	   the higher probability .
	   @return
		long - the current beam width
	*/
	virtual long GetBeamWidth() = 0;

    /** @brief for compatibility purposes, this method uses the old alignment strategy

        Before 3.5, the system did not support multiple pronunciations. Because of the
        size and scope of the change, the old version is still available. This will
        eventually be removed.
     */        
    virtual serror RecognizePhonemesOld(IObservationStream* pStream,
									 CAccousticHMM* pHMM,
									 const char* strText, 
									 ILipSyncTranscriber* pTranscriber,
									 CProgress* pProgress,
									 ISyncResultsCollection* pMarkers) = 0;
};


////////////////////////////////////////////////////////////////////////////////
// Name: CreateTextBasedRecognizer
/**@ingroup sdk_functions
   @brief
     This dll function is used to create a new ITextBasedPhnRecognizer object
   @param
	ppRecog - [out] the instantiated object
   @return
	kNoError - or appropriate \ref error_codes

   @see DestroyTextBasedRecognizer, ITextBasedPhnRecognizer, snippet_textbased_lipsync
*/
LIBLIP_API
serror CreateTextBasedRecognizer(ITextBasedPhnRecognizer** ppRecog);

////////////////////////////////////////////////////////////////////////////////
// Name: DestroyTextBasedRecognizer
/**@ingroup sdk_functions
   @brief
     This dll function is used to destroy an ITextBasedPhnRecognizer object
   
   @param pRecog - [in] the object to destroy

   @see CreateTextBasedRecognizer, ITextBasedPhnRecognizer, snippet_textbased_lipsync
*/
LIBLIP_API
void DestroyTextBasedRecognizer(ITextBasedPhnRecognizer* pRecog);


////////////////////////////////////////////////////////////////////////////////
// Name: IRealtimePhnRecognizer
/**@ingroup sdk_interfaces
	@brief This interface is used for the realtime phoneme recognition.

	The Realtime SDK works through the IRealtimePhnRecognizer interface, created 
	by CreateRealtimePhnRecognizer.

	The Realtime recognizer model is best thought of as a queue. Audio data is 
	written to the recognition queue using IRealtimePhnRecognizer::WriteBytes. 
	This information is processed in a separate thread. To access the results, 
	applications will call IRealtimePhnRecognizer::GetMarkersBeforeTime(). 
	This call will populate a ISyncResultsCollection with lipsync markers.

	The recognizer works very similar to the wave audio devices under windows. 
	Audio data is queued up using WriteBytes, and return values are extracted 
	using GetMarkersBeforeTime. The application is free to queue up as much data 
	as it desires. BufferTime specifies the initial buffer size for the queue. 
	GetMarkersBeforeTime will not return any phonemes until the initial queue 
	size is satisfied. Therefore, it is advisable to make sure that bufferTime 
	is slightly less than the size of the audio playback buffers of the 
	application. That is, bufferTime seconds of audio should be available 
	immediately when IRealtimePhnRecognizer::Start is called, or shortly 
	thereafter. 

	Applications should use the DestroyRealtimePhnRecognizer when 
	finished with the IRealtimePhnRecognizer. This will work even if the 
	recognizer is currently running.

	There are a couple of gotchas though. The accuracy of the system is 
	completely defined by how full the queue is with audio data. The size of 
	the queue (in seconds) is set when CreateRealtimePhnRecognizer is called. 
	The bufferTime parameter should be something more than 1 second if you 
	can help it. The more queue time the better the results, but the more work 
	that goes into the boot up sequence.

	Directly recorded audio that cannot have a delay. It has to be instantaneous 
	cannot have a large queue. In this case, applications just have to live 
	with the inaccuracies. 

	The realtime demonstration shows the reference implementation for this, 
	bufferSize and all.

	See Also: CreateRealtimePhnRecognizer, DestroyRealtimePhnRecognizer, realtime_console
*/
class IRealtimePhnRecognizer
{
protected:
	// protected destructor. Use DestroyRealtimePhnRecognizer
	virtual ~IRealtimePhnRecognizer() {};
public:
	/*
	   @brief
		The Start method begins the recognition process. 
	   
		The recognizer runs in a separate thread this method launches 
		the thread and begins processing. This is a thread safe method.
	   @return
	    serror - kNoError or appropriate \ref error_codes
	*/
	virtual serror Start() = 0;

	/**@ brief The Stop method halts the recognition thread (and effectively) stops the recognition process.
		
		 This is a thread safe method. */
	virtual void Stop() = 0;
	
	/////////////////////////////////////////////////////////////////////////
	// Name: WriteBytes
	/** @brief
	    This method is used by the application to submit audio data to the recognition process.
	   
	   This is a thread safe method.

	   Applications use this to write audio data to the RealtimePhnRecognizer, usually this occurs 
	   at the same time that the audio data is queued to the audio device. It is the same kind of 
	   process-add the audio data to the recognizer queue for processing. 
	   
	   @param pBytes - [in] Audio data in the format specified in CreateRealtimePhnRecognizer
	   @param nBytes - [in] size in bytes of pBytes
	   @see realtime_console
	*/
	virtual serror WriteBytes(unsigned char *pBytes, slong nBytes) = 0;

	/////////////////////////////////////////////////////////////////////////
	// Name: GetMarkersBeforeTime
	/**@brief The primary method to get sync results from the realtime recognizer
	
		Given an audio time (t) in milliseconds, this method returns all the 
		recognized phonemes that occurred before the specified time and after 
		the last call to this method.  If the audio time (t) is -1, then all 
		markers available are returned.

		The units for t are specified in milliseconds and should be in 
		terms of the audio playback. If the current play time is xt, 
		xt should be used as the bases for pulling out new markers.

		This method is thread safe.
	  
		@param pMarkers - [in,out] new CSyncMarker records will be appended it this and
					then retrieved from it;
		@param t - [in] time in milliseconds. 
	  @return
		- true - if successfully retrieved markers
		- false - otherwise
	  @see ISyncResultsCollection, IRealtimePhnRecognizer
	*/
	virtual bool GetMarkersBeforeTime(ISyncResultsCollection* pMarkers, long t) = 0;

    /////////////////////////////////////////////////////////////////////////
    // Name: SetMarkerWindow
    /**@brief This method is used to have the RealtimePhnRecognizer save markers between calls
        to GetMarkersBeforeTime.

            It would be nice to use the articulator along with realtime phoneme recognition.
            
			The problem is that GetMarkersBeforeTime just returns the markers
            in the current window. So by setting the marker window, instructs
            the Realtime phn recognizer to save old markers and then add them
            in to the results so that the Articulator can be used to more effect
        @param msTime - [in] the lookahead and lookback window (in milliseconds)
    */
    virtual bool SetMarkerWindow(long msTime) = 0;
};

/////////////////////////////////////////////////////////////////////////
// Name: CreateRealtimePhnRecognizer 
/**@ingroup sdk_functions
   @brief
    This dll function is used to instantiate a realtime lipsync object 
	 IRealtimePhnRecognizer given information about the audio stream 
	 and an instantiated CAccousticHMM.

     The realtime recognizer works by writing audio "bytes" to it. In order for 
	 the Recognizer to understand the audio data, it must have certain information 
	 about the byte data that client will be supplying (sample rate (22050 for example), 
	 a sampleSize (in bits, 8 for bits per sample, 16 for 16 bits per sample). 
	 nChannels. the number of channels in the signal (1 for mono, 2 for stereo). It is necessary that this information be supplied and be accurate.
	 
	 Additionally, a bufferTime is passed in. This value specifies the number 
	 of seconds of initial audio that should be queued up before beginning the 
	 recognition process.  This is very similar to queueing up audio for playback. 
	 this allows the user to control how big the initial queue should be. 
	 Like audio, this acts as a buffer against slow downs and also improves 
	 recognition performance. I use 3.25 for playing audio, something less 
	 than the initial size of the waveout queue, and .4 seconds for direct 
	 from mic. This effects quality, the longer the lookahead, the better 
	 the recognition results. But for some applications, such as mic input 
	 where there is can be no delay, you must use a small value in order 
	 to avoid having delays.
   
	@param sampleRate - [in] Fix point sample rate of the audio source.
	@param sampleSize - [in] 8 or 16. bits per sample of the audio signal
	@param nChannels - [in] Number of channels in the audio signal
    @param bufferTime - [in] number of seconds of initial audio (latency) allowed
	@param pHmm	  - [in] The CAccousticHMM to use for calculating phonemes
	@param ppPhnRecog - [out] the recognizer interface
	@see
		IRealtimePhnRecognizer,DestroyRealtimePhnRecognizer
*/	
LIBLIP_API
serror CreateRealtimePhnRecognizer(long sampleRate, long sampleSize, long nChannels, double bufferTime,
		CAccousticHMM* pHmm, IRealtimePhnRecognizer** ppPhnRecog);

/////////////////////////////////////////////////////////////////////////
// Name: DestroyRealtimePhnRecognizer
/**@ingroup sdk_functions
   @brief: This DLL function is used to destroy an IRealtimePhnRecognizer object.
   
   @param pPhnRecog - [in] the object to destroy
   @see CreateRealtimePhnRecognizer, IRealtimePhnRecognizer
*/
LIBLIP_API
void DestroyRealtimePhnRecognizer(IRealtimePhnRecognizer* pPhnRecog);



#endif
