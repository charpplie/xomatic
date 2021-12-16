/* 
	libtranscribe.h 

	Copyright (c) 2002-2005 Annosoft, LLC. Garland, Texas. All rights reserved.     
	This program and header file contains the confidential trade secret 
	information of Annosoft, LLC.  Use, disclosure, or copying without 
	written consent is strictly prohibited. 

	This header file is used to access annosoft's proprietary Lipsync libraries,
	use of this file or the library files is prohibited without prior approval
	from annosoft.com and the acceptance of the appropriate license agreement. 
	Any internal copies of this file must be accompanied by the appropriate license
	agreement.
*/
/** @file libtranscribe.h
   @brief This header file declares interfaces and functions for handling the
	conversion of text into phonemes.

	libtranscribe declares functions and interfaces for using annosoft's
	predefined text to phoneme conversion methods. It also provides
	an architecture where applications can implement their own
	text to phoneme converter, if needed.
	 
*/

#ifndef _H_LIBTRANSCRIBE
#define _H_LIBTRANSCRIBE		1



#include "liblipsync.h"


// forward declarations
class tx_pronunciation_alt;

///////////////////////////////////////////////////////////////////////////////
// Name: CPronunciationRec
/**@ingroup sdk_structures
   @brief CPronunciationRec is a data class containing information about an entry in an ISpeakDict
   
   This data class is returned by the ISpeakDict to requests for
   word/dictionary information. Applications usually do not need to access the speak
   dictionary directly.
  
   @note Applications should not attempt to delete the character pointers. They
   are managed by the speak dict. They are invalidated when the dictionary is 
   freed or when the dictionary is modified. 
   @see ISpeakDict
*/
class CPronunciationRec
{
public:
	/// the word (orthograph) for the entry (not null terminated)
	char  *strWord;     
	/// the buffer length of the word
	ushort  nWordLen;   
	/// the phoneme transcription for the word (not null terminated)
	char  *strPhonemes; 
	/// the length of the phoneme transcription for the word
	ushort  nPhonemeLen;
};

///////////////////////////////////////////////////////////////////////////////
// Name: ISpeakDict
/**@ingroup sdk_interfaces
   @brief Pronunciation dictionary class. 
   
   This interface is exposed to allow for dictionary expansion
   without the need to edit the dictionary file or resource. or to produce
   a custom dictionary hooked in with the default letter-2-sound rules.

   However, developers are encourage to explore ILexiconEditor and the \ref lexicon_editor
   sample program. This new architecture is a better way to implement custom terms/dictionaries.
  
   The class includes interfaces for extracting multiple phonetical representations
   from a single grapheme (word)
*/
class ISpeakDict
{
protected:
	virtual ~ISpeakDict() {}	// use "Release()"
public:
	//////////////////////////////////////////////////////////////////////////
	// Name: ISpeakDict::Release
	/**@brief: Destroy the ISpeakDict
	   All speak dicts must implement the "Release" method. This allows both
	   the engine and the application to use a safe method to destroy either
	   engine or application supplied dictionaries.
	   
	   Internally, my "Release" method looks like this: \code
	   Release()
	   {
	  	  delete this;
	   }
	   \endcode 
	*/
	virtual void Release() = 0;

	//////////////////////////////////////////////////////////////////////////
    // Name: ISpeakDict::lookup	
    /**@brief This method finds the given word in the dictionary
       
       This method, given a word, looks the word up in the dictionary and returns
       the best phonetical representation for the word.
       
       @param pWord - [in] character buffer (word) to find
       @param nWordLen - [in] length of the word (pWord)
       @param szPhonemes - [in, out] the phonemes for the given word (NULL terminated)
                    this buffer should be the maximum sized phoneme transcription for
                    a word in the dictionary. A good number is 255.
                    
	   @return
        true if found, false if not.
	*/
	virtual bool Lookup(char* pWord, long nWordLen, char *szPhonemes) = 0;

	//////////////////////////////////////////////////////////////////////////
    // Name: ISpeakDict::find_pronunciation
    /** @brief: This method finds a pronunciation record for the given word.
       
	   This method returns a pronunciation record for the specified
       word, if present. A given word can have more than one pronunciation.
       To access multiple pronunciations, the index parameter can be incremented to
       find the next, the method will return false if there is no ith pronunciation.
      
        
       
       @param pWord - [in] string representing the word to be found in the dict
       @param nWordLen - [in] the size of the string pWord
       @param index - [in] 0 based index representing the ith pronunciation for the word.
       @param pEntry - [out] the pronunciation for the specified word, if found. 
       @return
        true if the entry was found, false if not
       @code
	   For Example
        ISpeakDict *mySpeakDict = Create...();
        CPronunciationRec theRecord;
        const char* szTest = "read and write"
        long i = 0;
        bool bFound;
        do
        {
            bFound = mySpeakDict->find_pronunciation(szText, 4, i, &theRecord);
            if (bFound)
            {
                std::string strPhonemes(theRecord.strPhonemes, theRecord.nPhonemeLen);
                std::cout << strPhonemes << std::endl;
                i++;
             }
        } while (bFound);
		@endcode
	*/
    virtual bool find_pronunciation(char* pWord, long nWordLen, long index, CPronunciationRec* pEntry) = 0;
  
};


///////////////////////////////////////////////////////////////////////////////
// Name: CTxMarker
/**@ingroup sdk_structures
   @brief a transcriber generates these markers which precisely identifies the start 
   and end indices of of a marker object in the input string.

   A Transcriber must generate CTxMarkers in order to perform text based lipsync.

   It defines the position of the marker within the source text, as well as other information.

   Word markers, and other markers, such as pause markers, will include pronunciations
   for the marker. These are anno 40 pronunciations for the specified term, and are reflected in
   the CTxMarker::pron_paths field of this structure. The format for this structure is complicated
   by the fact that we want to support multiple pronunciation of words, and even
   multiple pronunciations of individual segments of a specific term. "www.the.com" for example.

   To support ITextBasedPhnRecognizer::RecognizePhonemesOld, CTxMarker::dstStartIdx and
   CTxMarker::dstEndIdx must be set by the transcriber. If ITextBasePhnRecognizer::RecognizePhonemes
   is used, dstStartIdx and dstEndIdx need not be set by the transcriber.
   
   The CTxMarker structure is a mapping marker. It maps "tokens", 
   either words, punctuation marks, or user data from a 
   source transcription, it's place in the original text file, 
   to the phoneme list produced by the transcriber for the text. 
   This allows us, after aligning the phonemes with the text, 
   to back up and figure out exactly where the words, punctionation, 
   and user data align, in terms of time, to the audio. 
	@see ILipSyncTranscriber::GetOutputMarkers, ILipSyncTranscriber
*/
class CTxMarker
{
public:
	/** @brief Each marker is assigned a type. A list of possible
		types is found in this enum */
	enum markerTypes
	{
		/// marker to nothing!
		txNone,			
		/// marker to a word
		txWord,			
		/// marker to XML
		txXML,			
		/// marker identifying a user defined cut point in the text file
		txCutMarker,	
		/// marker to punctuation
		txPunct
	};
    /**@brief marker flags indicating how to process the marker. </P>
        
        */
    enum markerFlags
    {
        txfNone = 0x00, ///< process normally
        txfOptional = 0x01, ///< treat the inner phonetics contained in the marker as totally optional
        txfForced = 0x02   ///< force silence regions. Usually "x" is always optional. Now, x is required
    };
    
	/// start index in the transcription
	long srcStartIdx; 
	/// end index in the transcription
	long srcEndIdx;	  
	/// start index of the token in the phoneme (result) array. 
	long dstStartIdx; 
	/// end index of the token in the phoneme (result) array
	long dstEndIdx;      
	/// type of marker. one of CTxMarker::markerTypes
	long type;		  
	/// for type markerTypes::txCutMarker - milli time of this cut point.
	long cutMilli;	  
    /// flags txf*** bitfield
    long flags;
        /// the number of pronunciation paths for this marker
    long        num_pron_paths;
    /// the pronunciation paths for this marker. [0...num_pron_paths-1]
    tx_pronunciation_alt *pron_paths;
};

////////////////////////////////////////////////////////////////////////////
// tx_pronunciation_alt
/**@ingroup sdk_structures
   @brief This structure defines an alternate pronunciation for a CTxMarker

   The CTxMarker keeps a list of 0 or more tx_pronunciation_alt structures.
   Each tx_pronunciation_alt structure defines 1 or more pronunciation choices.
   This structure allows the engine to support multiple pronunciations for a word.

   CTxMarker can include more than one alt record because an constructive term,
   such as a web site, a large number, e-mail address, etc will be have separate
   pronunciations for different parts of the word. For example: mzartler@the-annosoft.com
   "the" may have two pronunciations "DHIY" and "THAH". The segments in the CTxMarker
   will look like:
   @verbatim
   mzartler -> the   -> annosoft -> com
               tha
   num_paths = 4
   @endverbatim
*/
class tx_pronunciation_alt
{
public:
    /// the number of pronunciations pron_list[0..num_pron-1]
    long num_pron; 
    /// an array of strings [0...num_pron-1]
    char **pron_list;
};

////////////////////////////////////////////////////////////////////////////
// Transcriber Information data structure
/**@ingroup sdk_structures
   @brief This structure, returned from ILipSyncTranscriber, contains displayable
   information about the transcriber.

   This information can be useful to the application in displaying
   available languages, etc.
   This data structure can be queried from an instantiated ILipsyncTranscriber
   or can be accessed directly from a lexicon buffer.
   @see ILipSyncTranscriber::getTranscriberInfo, GetTranscriberInfo dll function
*/
typedef struct tTranscriberInfo
{
	/// the display name of the transcriber
	char  name[64];	
	/// the language code of the transcriber
	ulong langCode;	
	/// properties and features of the transcriber. None supports right now.
	ulong flags;	

    /// the locale of the transcriber. if not empty, the locale string used for transcription
    char  locale[64];

    /// flag options. 
	enum
	{
        /// identifies the lexicon as requiring unicode input. CURRENTLY UNUSED.
		flag_unicode = 0x01,
        /// indicates the transcriber will flip the pronunciation of "twenty one" to "one and twenty" - support for germanic languages
        flag_one_and_twenty = 0x02,
        /// flag indicating this transcriber has a locale associated with it.
        flag_locale     = 0x04,
        /// transcriber has language specific durational constraints. @see ILipSyncTranscriber::GetPhoneDurationConstraints
        flag_duration_constraints = 0x08
	};
} tTranscriberInfo;

////////////////////////////////////////////////////////////////////////////
// Name: ILipSyncTranscriber

/* base class for transcription services required to perform
   transcription services By creating an interface out of this.
   It allows clients to create their own transcription services
   without requiring a code change.                             */

/**@ingroup sdk_interfaces
  @brief
    An abstract interface that implements a word to phoneme conversion for
	text based lipsync.
  
    An abstract interface that implements a transcription service. The 
	transcriber is responsible for converting plain text into a phonetical 
	representation of that text. It is also responsible for creating markers 
	which define where words, punctuation, and user data are located in the 
	orthographic transcription as well as the generated phonetical alignment 
	(see CTxMarker for details).

	Annosoft provides a few ways to create transcribers without implementing 
	your own. CreateDefaultTranscriber, CreateTranscriberFromLexicon, and 
	CreateTranscriberFromLexiconFile.

	Annosoft currently supports English, French, and Spanish transcribers. 
	However, if needed, applications can define their own transcription objects 
	and use them within the system. This allows customization to support a) 
	foriegn languages. b) unicode. c) specialty formats in terms of user 
	defined markers in the text file. 

	If you do not plan to implement your own transcriber, you do not need to 
	understand the transcriber interface, methods, or structures.

	For those that need to implement their own, read on!
   @see
    snippet_createtranscriber
*/
class ILipSyncTranscriber
{
public:
	/**
		@brief
		 This performs the transcription of the specified string. 
		
		 This performs the transcription of the specified string. It stores 
		 the transcription results in internal buffers of the object, accessed 
		 through different accessors. It assumes a single-byte character set. 
		 Currently, all annosoft transcribers support this. In the future, when 
		 multi-byte languages are supported, this method may be unsupported by these transcribers. 
		 It returns TRUE, if supported, FALSE, if not supported.<emit \<br/\>>

		 For applications building their own transcribers, they only need to 
		 support this method if they are calling 
		 ITextBasePhnRecognizer::RecognizePhonemes(). This method will call the 
		 specified transcriber's Transcribe routine. If applications call 
		 ITextBasedPhnRecogner::AlignTranscription(), this method will not be called, 
		 and it is safe to omit it.
		 
		
		@param szString - [in] the text to transcribe
		@return
			true - if the transcription interface supports single byte.
			false - if the transcription interface supports unicode
	*/
	virtual bool Transcribe(char *szString) = 0;

	//////////////////////////////////////////////////////////////////
	// Name: TranscribeW
	/** @brief Transcribe the unicode string. This is not supported currently
	   by annosoft transcribers.
	   
	   This performs the transcription of the specified unicode string. It 
	   stores the transcription results in internal buffers of the object, 
	   accessed through different accessors. It assumes a unicode character set. 
	   Currently, NO annosoft transcribers support this. 
	   
	   In the future, when multi-byte languages are supported, this method will 
	   be used to transcribe unicode strings. It returns TRUE, if supported, FALSE, 
	   if not supported.

	   Application implementations can choose to support and implement this method
	   in their own interface
	   
	   @param uszString - [in] unicode string
	   @return
	  		true - if the transcription interface supports unicode.
	  		false - if the transcription interface doesn't support unicode
	*/
	 virtual bool TranscribeW(const ushort *uszString) = 0;

	/**
		@brief
			This method is used to retrieve information about this transcriber.
		
			This method fills in the tTranscriberInfo structure specifying information 
			about the transcriber, such as name, language code, and whether it 
			is a unicode or ascii transcriber.
			@param pInfo - [out] tTranscriberInfo structure describing this transcriber.
			@see tTranscriberInfo
	*/
	virtual void getTranscriberInfo(tTranscriberInfo* pInfo) = 0;

/**
	@brief
	  This method returns a list of transcription markers generated by Transcribe, or TranscribeW
	
	  It returns a live pointer to the output markers created by the transcription 
	  process. This buffer of data contains markers for words, xml, and 
	  punctuation, that are part of the transcription.

	If defining a new transcriber, applications need to implement this if they 
	want to time align words, punctuation or user data.

	It returns a live pointer to the markers, as well as the number of items 
	contained in the marker list.

	In order to use this it is necessary to understand what the CTxMarker does, 
	and it's fields.
	
	@param ppNumItems - [out] the total number of markers.
	@return CTxMarker * - pointer to the marker array.
	@see	CTxMarker, ILipsyncTranscriber
*/
	virtual CTxMarker* GetOutputMarkers(long *ppNumItems) = 0;

/**
	@brief
	  This method is used to retrieve a pointer to the transcription string of phonemes.

	  After the conversion from the transcription into phonemes and CTxMarkers (ILipSyncTranscriber::Transcribe), 
	  this method can be used to retrieve the phonetical representation of the text, 
	  presumably the Anno 40 phoneme set.
	  
	  If defining a new transcriber, applications must implement this to support
      ITextBasedPhnRecognizer::RecognizePhonemesOld

	@note: This is usually a live pointer whose lifecycle is tied to the
	lifecycle of the ILipsyncTranscriber object
	@returns
		transcription string, anno 40 phoneme transcription of the text
*/
	virtual const char*	GetTranscription() = 0;


    /**
    @brief This method is used to retrieve the HMM Durational constraints in the lexicon<P>
    Durational constraints on the hmm can be dependent on the current language. This was
    added because russian "y" and other consonants generally hold too long. This bad behavior
    can be controlled by durational constraints. However, since these may be language based,
    they need to be stored somewhere. Instead of requiring the user to load a constraints
    text file, we use the alex file as a resource for durational constraints that typify the
    language. 

    Note that this is not applied automatically. The application must call this method to
    pick up the constraints data and then call ::LoadPhoneConstraintsIntoHMM() with the
    data returned from here. Note that this may be NULL. In which case, the alex file
    does not have phoneme constraints.
    @param pSize - [out] size of the constraints.
    @return the text of the constraint. It can be NULL!
    **/
    virtual const char* GetPhoneDurationConstraints(long* pSize) = 0;


    // Name: AddRef
	/** @brief
        Increment the reference count for the object
       
        Applications will only need to use this if they are copying around transcribers,
        SDK Functions returning transcribers are already referenced.

        Transcriber subclasses will need to implement this

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

        This is also an alternative to calling ::DestroyTranscriber

		@return
	  	 reference count after the release is done
	*/
    virtual long Release() = 0;

};




///////////////////////////////////////////////////////////////////////////////
// Name: CreateTranscriberFromLexicon
/**@ingroup: sdk_functions
   @brief This DLL function is used to create an ILipSyncTranscriber object from an ALEX formatted buffer.

	A transcriber, in Annosoft technologies, is something that converts a string 
	of text into a phonetical representation. 

	A lexicon file is a proprietary file format that contains transcriber 
	information for a particular language. A lexicon file, by convention, 
	has the extension .alex. 

	CreateTranscriberFromLexicon is used to instantiate a ILipSyncTranscriber 
	given an ALEX buffer, such as an ALEX file loaded wholly into memory, or 
	such as a ALEX file compiled as a resource in your application.

	You only need this file and object for text based lipsync.    

	@param pDataBuffer - [in] buffer of .alex formatted data
	@param nBufferBytes- [in] size of buffer in bytes
	@param ppTranscriber - [out] transcriber object
	@return
	kNoError or appropriate error code
	@see
    snippet_createtranscriber
*/
LIBLIP_API
serror CreateTranscriberFromLexicon(char* pDataBuffer, long nBufferBytes,
									ILipSyncTranscriber** ppTranscriber);

///////////////////////////////////////////////////////////////////////////////
// Name: CreateTranscriberFromLexicon
/**@ingroup: sdk_functions
@brief
	This DLL function is used to create an ILipSyncTranscriber object from an ALEX disk file.

	A transcriber, in Annosoft technologies, is something that converts a string 
	of text into a phonetical representation. 

	A lexicon file is a proprietary file format that contains transcriber 
	information for a particular language. A lexicon file, by convention, 
	has the extension .alex. 

	CreateTranscriberFromLexiconFile is used to instantiate a ILipSyncTranscriber 
	given an ALEX file.
	
	You only need this file and object for text based lipsync.    

	@param szFileName - [in] path to alex file
	@param ppTranscriber - [out] transcriber object
@return
	kNoError or appropriate error code
@see
    snippet_createtranscriber
*/
LIBLIP_API
serror CreateTranscriberFromLexiconFile(const char* szFileName,
									ILipSyncTranscriber** ppTranscriber);

///////////////////////////////////////////////////////////////////////////////
// GetTranscriberInfo
/**@ingroup sdk_functions
	@brief
	This DLL function retrieves transcriber information given the alex header (first 80 bytes)

	Given the first 128 bytes of a lexicon data buffer (such as created from loading
	the first 128 bytes of an annosoft lexicon file), return the transcriber
	information for the specified transcriber.

	This is useful for enumerating the available languages in various lexcion
	files, or memory buffers.
 
	@param pDataBuffer - [in] the first 128 or more bytes of an alex file or buffer
	@param nBufferBytes - [in] the size of pDataBuffer
	@param pInfo - [out] tTranscriberInfo, the information about the alex buffer
 @return
	kNoError or appropriate error code
*/
LIBLIP_API
serror GetTranscriberInfo(char* pDataBuffer, long nBufferBytes,
						  tTranscriberInfo* pInfo);




///////////////////////////////////////////////////////////////////////////////
// CreateSpeakDict
/**@ingroup sdk_functions   
   @brief
	The DLL function creates an ISpeakDict given a buffer of dictionary data. Most applications
	do not need to use this function.
      
	@param pDictBuffer - [in] pronunciation dictionary (buffer)
	@param nBufferBytes - [in] the size in bytes of the pronunciation dictionary
   @return
    ISpeakDict*
*/
LIBLIP_API
ISpeakDict* CreateSpeakDict(char *pDictBuffer, long nBufferBytes);

#endif
