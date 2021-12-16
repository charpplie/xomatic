/* 
	libalexeditor.h 

	Copyright (c) 2005 Annosoft, LLC. Garland, Texas. All rights reserved.     
	This program and header file contains the confidential trade secret 
	information of Annosoft, LLC.  Use, disclosure, or copying without 
	written consent is strictly prohibited. 

	This header file is used to access annosoft's proprietary Lipsync libraries,
	use of this file or the library files is prohibited without prior approval
	from annosoft.com and the acceptance of the appropriate license agreement. 
	Any internal copies of this file must be accompanied by the appropriate license
	agreement.
*/
/**@file libalexeditor.h
  
   @brief This header file declares the interfaces and functions for editing ALEX (Dictionary) files

	ALEX files are external representation of text to phoneme rules, dictionaries, etc.
    Athough, the ability to edit letter to sound rules is not yet provided, the ability
    to edit, and customize the dictionary provides a level of flexibility that should
    be sufficient for most people. 
*/

#ifndef _H_LIBALEXEDITOR
#define _H_LIBALEXEDITOR		1


#include "libtranscribe.h"


// forward declaration
class IPronunciationRecord;

////////////////////////////////////////////////////////////////////////
// Name: Lexicon Editor
/**@ingroup sdk_interfaces
   @brief This class provides the capability of performing modifications
    to an ALEX (Text Based Lipsync Dictionary) File.

    Text-based lipsync requires a pronunciation dictionary and letter-2-sound rules 
    for the language of the text. Annosoft implements different language configurations 
    by using ALEX files to represent information about the language. ALEX files 
    include rules and a dictionary for converting text into phonemes.

    The ability to customize the dictionary is good feature for many domain 
    specific uses where the default pronunciations may not be optimal.

    The DLL functions CreateTranscriberFromLexiconFile and CreateTranscriberFromLexicon 
    are used to load ALEX files for use in text based lipsync. 

    The ILexiconEditor also loads ALEX files. It's role, however, is to 
    provide editing services on the pronunciation dictionary within the ALEX file. 
    The ILexiconEditor class provides methods to open an alex file, 
    add and remove, change pronunciation dictionary entries, and finally to 
    save them back to dict.

    The ILexiconEditor is created and destroyed using the DLL functions 
    CreateLexiconEditor and DestroyLexiconEditor.

    The lexicon editor pronunciations use the anno 40 phoneme set. 

    The use of the lexicon editor class is fully demonstrating in the 
    lexicon_editor.cpp sample code.
*/
class ILexiconEditor
{
protected:
    virtual ~ILexiconEditor() {} // use DestroyLexiconEditor
public:
    ///////////////////////////////////////////////////////////////////////////
    // Name: ILexiconEditor::Save
    /**@brief
       This method is used to save the modifications to the specified file (as an alex file).
    
       It will save the data in ALEX format which can subsequently be used in 
       text based lipsync using CreateTranscriberFromLexiconFile.

       @param szFile - [in] path and file name where the file should be saved.
       @param pProgress - [in] progress meter for updates and cancelling (can be NULL)
       @return 
        serror - kNoError if successful, otherwise an appropriate error code. It returns
        kErrFileLocked if the file cannot be opened for writing. This could either be a volume lock
        or a file lock.
    */
    virtual serror Save(const char* szFile, CProgress* pProgress) = 0;
    
    ///////////////////////////////////////////////////////////////////////////
    // Name: ILexiconEditor::find_pronunciation
    /**@brief
        find a pronunciation record given a word buffer.
 
        This method is used to find a pronunciation for a specific word. It returns an 
        IPronunciationRecord interface if the word is found in the dictionary. 
        This class can be used to access and/or change the phonetical realization 
        for the word.

        Currently, the text based engine does not support multiple alignments 
        (phonetical realizations) for words, instead it just picks the first one 
        it finds. However, in the future, this feature will be implemented. 
        Therefore, it is necessary for ILexiconEditor methods to support 
        multiple pronunciations for a specific word. This adds some complication, 
        but is required to make it useful in the future.

        The find pronunciation method takes a 0 based index parameter. 
        By incrementing this and repeatedly calling find_pronuncation, 
        all pronunciations for a specific word can be found. 
        The method will return null if the index is out of range, thus a null 
        return value is the condition on which to stop iterating.
 
       
       @param pWord - [in] string representing the word to be found in the dict
       @param nWordLen - [in] the size of the string pWord
       @param index - [in] 0 based index representing the ith pronunciation for the word.
            0 gives the first pronuncation, 1 gives the second pronunciation (if any), 
            2 gives the 3rd pronunciation, etc. Incrementing this value and iterating can be 
            used to retrieve all pronuncations for a given word.
        
       @return
        IPronunciationRecord - the pronunciation of the word. NULL if the word or the index is out of range.
        The application should not dispose of the return value. It is managed by the this class.
    */    
    virtual IPronunciationRecord* find_pronunciation(const char* pWord, long nWordLen, long index) = 0;

    ///////////////////////////////////////////////////////////////////////////
    // Name: ILexiconEditor::delete_pronunciation
    /**@brief
       This method is used to delete a pronunciation from the lexicon.
       
       The pronunciation representation should be in the anno 40 phoneme set unless 
       otherwise instructed.

       In the future, the engine will make use of all pronunciations for a word and pick 
       out the best pronunciation when doing alignment. The ALEX file format 
       supports this, as do the ILexiconEditor methods, but currently it is 
       not implemented in the text based lipsync engine.

       Because of this, delete_pronunciation requires both the word and the 
       pronunciation to be able to delete the entry from the pronunciation 
       dictionary. The pronunciation(s) for a word can be found by calling 
       ILexiconEditor::find_pronunciation which supports the ability to find 
       all pronunciations for a specific word.
       
       @param pWord - [in] the word to delete
       @param szPhonemes - [in] the pronunciation to delete.
       @return
        kNoError if successful. if the word is not found, it returns kOutOfRange
        otherwise, an appropriate error code.
       @note the existing pronunciation record matching this element will be deleted,
       thus any caches by the application are invalid and will cause an exception.
    */
    virtual serror delete_pronunciation(const char* pWord, const char* szPhonemes) = 0;
    
    ///////////////////////////////////////////////////////////////////////////
    // Name: ILexiconEditor::add_pronunciation
    /**@brief
       This method is used to add a pronunciation for the specified word. 

       The pronunciation representation should be in the anno 40 phoneme set unless otherwise instructed.

       Note that if a pronunciation for the word already exists, it will be kept in 
       the dictionary. Use ILexiconEditor::delete_pronunciation to remove 
       the old pronunciation. 

        In the future, the engine will make use of all pronunciations for a word and 
        pick out the best pronunciation when doing alignment. 
        The ALEX file format supports this, as do the ILexiconEditor methods, but 
        currently it is not implemented in the text based lipsync engine.

       
       @param szWord - [in] The text (word) to add to the pronunciation lexicon, null terminated
       @param szPhonemes - [in] The phonetical realization for the word in the anno40 phoneme set.
       @return
        kNoError if successful, error code otherwise.
    */
    virtual serror add_pronunciation(const char* szWord, const char* szPhonemes) = 0;

    ///////////////////////////////////////////////////////////////////////////
    // Name: ILexiconEditor::transcribe
    /** @brief this method is used to convert text into phonemes
       
        This method is used to convert text into phonemes. It uses exactly the 
        same method that the text based lipsync engine uses so it is a pretty 
        good way to test things. 

        It is very useful for a lexicon editor application because dealing with 
        the anno 40 phoneme set can be a difficult and mind bending task. It is 
        hard to learn all the phonemes and it is hard to manually convert a words 
        into phonemes.

        ILexiconEditor::transcribe can be used to generate real phonemes from 
        psuedo-phonetical spellings and word representations. 

        For example, the annosoft transcription for RPG is 'rpIYjIY'. Having to 
        come up with these every time a lexicon change is needed would be very 
        cumbersome to the user. Instead, if the user wished to add RPG. The can 
        type in a sounded out version, such as 'are pee gee' and the application 
        can call ILexiconEditor::transcribe to convert it into 'rpIYjIY'. 

        This is much easier on the end user and is highly recommended. 
       
       
       @param szText  - [in] the orthography or spelled out input, null terminated.
       @param szPhonemes - [out] application passes a buffer which will receive the phonetical output
       @param lenPhonemes - [in] number of bytes in the szPhonemesBuffer.
       @return
        0 if successful, otherwise an appropriate error code. kErrBufferOverrun indicates
       that the size of the lenPhonemes is too short.
    */       
    virtual long transcribe(const char* szText, char* szPhonemes, long lenPhonemes) = 0;

    
    ///////////////////////////////////////////////////////////////////////////
    // Name: getTranscriber
    /**@brief this method returns the transcriber object for this editor

       Internally, the lexicon editor maintains a transcriber object that is 
       used to perform the transcribe operation. This transcriber is updated
       when changes are performed.

       The returned transcriber can be used in ITextBasedPhnRecognizer methods.

       The returned transcriber should be disposed of using ::DestroyTranscriber.

       @return
            ILipSyncTranscriber* - the transcriber instatiated with this lexicon editor,
            or NULL if the ILexiconEditor failed to load properly. 
            When finished::DestroyTranscriber to free this object.
    */
    virtual ILipSyncTranscriber* getTranscriber() = 0;

    ///////////////////////////////////////////////////////////////////////////
    // Name: dict size
    /** @brief returns the number of pronunciation records in the lexicon
     */
    virtual long dict_size() = 0;

    ///////////////////////////////////////////////////////////////////////////
    // Name: GetByIndex
    /** @brief This method, along with dict_ size() can be used to iterate all the pronunciation
       records in the lexicon.
     */
    virtual IPronunciationRecord* GetByIndex(long i) = 0;


    /**
    @brief This method compresses the dictionary.
    This is done by running the dictionary through the
    letter to sound rules and removing dictionary items which are correctly generated by
    letter to sound rules. ILexiconEditor::Save can be used to finalize the modification
    @note ILexiconEditor::dict_size can be used to compare the old dictionary size versus
    the new dictionary size to see how well the letter to sound rules correlate with the dictionary
    **/
    virtual void Compress() = 0;
};


///////////////////////////////////////////////////////////////////////////////
// Name: IPronunciationRecord
/** @ingroup sdk_interfaces
    @brief This interface allows applications to view/change the pronunciations of a word. It is
    returned by ILexiconEditor.

    This interface contains pronunciation information for a specific 
    pronunciation. It can be used to display a pronunciation from 
    the dictionary and it can be used to change the pronunciation. 

    It doesn't allow changing of the word, for that see ILexiconEditor::add. 

    The class and return values are memory managed by the lexicon editor. 
    The application should not dispose of IPronunciation records, 
    or the strings that it returns.

    @see ILexiconEditor::find_pronunciation, ILexiconEditor::GetByIndex, ILexiconEditor
    \ref sdk_interfaces
*/
class IPronunciationRecord
{
public:
    ///////////////////////////////////////////////////////////////////////////
    // Name: GetWord
    /** @brief This method returns the word for which the pronunciation is associated

        The application should not dispose of the returned string, and should not cache 
        it if the dictionary is being changed (make a copy of it), instead copy the
        string into a local buffer.

        @return
            a live pointer to the word. Do not change this value
      */
    virtual const char* GetWord() = 0;

    ///////////////////////////////////////////////////////////////////////////
    // Name: GetPronunciation
    /** @brief Returns the pronunciation of the word in the anno 40 phoneme set
           
        @return
            a live pointer to the pronunciation. Although it can be changed, it is recommended
            that applications use SetPronunication to make changes..
    */
    virtual const char* GetPronunciation() = 0;

    ///////////////////////////////////////////////////////////////////////////
    // Name: SetPronunciation
    /**
        @brief This method changes the pronunciation to the specified phonemes.
     
        This method is used to change the pronunciation of a specified entry. 
        This is the only way a pronunciation of an existing dictionary entry 
        can be changed. The change here will update the ILexiconEditor and, 
        when saved, the ALEX file.      
      
        Since raw transcription is difficult, ILexiconEditor::transcribe provides
        a way to convert loose phonetical typing into raw anno 40 phonemes. This is a better UI
        provide the user a way to type in phonetics, let the LexiconEditor recommend a
        anno40 description, and then set the new pronunciation to that return value.
       
       @param szPhonemes - [in] null terminated phoneme list that is the new anno40 realization for this
                    word.
        @see lexicon_editor.cpp sample code
    */
    virtual serror SetPronunciation(const char* szPhonemes) = 0;
};



///////////////////////////////////////////////////////////////////////////////
// Name: ::CreateLexiconEditor
/** @ingroup sdk_functions
    @brief This DLL function is used to open an ALEX file for editing.
  
    It does this by loading the ALEX file and creating an ILexiconEditor 
    interface which is returned. 

    Errors such as an invalid alex file, or file not found will be returned 
    and no ILexiconEditor interface will be returned.

    When finished with the interface, it is disposed 
    using DestroyLexiconEditor.

  
    @param szAlexFile - [in] path and file name of the ALEX file to edit.
    @param ppEditor - [out] interface to the lexicon editor object or NULL on failure
    @return
        serror - kNoError or appropriate error code
    @see ILexiconEditor, DestroyLexiconEditor, CreateTranscriberFromLexiconFile, \ref sdk_functions
*/
LIBLIP_API
serror CreateLexiconEditor(const char* szAlexFile, ILexiconEditor**ppEditor);


///////////////////////////////////////////////////////////////////////////////
// Name: DestroyLexiconEditor
/** @ingroup sdk_functions
    @brief This DLL function is used to destroy an ILexiconEditor object 
   
    @param pEditor - [in] interface to the lexicon editor object to destroy
    @see ILexiconEditor, CreateLexiconEditor, CreateTranscriberFromLexiconFile, \ref sdk_functions
*/ 
LIBLIP_API
void DestroyLexiconEditor(ILexiconEditor * pEditor);

#endif
