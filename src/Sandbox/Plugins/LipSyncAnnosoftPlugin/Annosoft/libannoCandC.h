/* 
	libannoCandC.h 

	Copyright (c) 2006 Annosoft, LLC. Garland, Texas. All rights reserved.     
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
@file libannoCandC.h
@brief declares the interfaces for command and control systems

**/

#ifndef _H_LIBANNOCANDC
#define _H_LIBANNOCANDC


/**
@brief This class implements a Command & Control Speech Recognition System
@ingroup sdk_interfaces

The command and control system is used to analyze speech events to determine 
which command is best associated with the incoming speech.

It makes the drastic assumption that the incoming speech is one of the specified commands, but also 
returns a score which can be used to threshhold the results. 

Commands are added using plain text or a list of anno phonemes. Plain 
text will be transcribed into phonemes and appropriate speech models created.

Adding phonemes (anno 40 phonemes) can be used to match incoming texts to 
prerecorded audio for which the textless phn recognizer has generated phonemes.

The methods to add phonetic mode commands are:

- IAnnoCmdCtrl::AddCommandText
- IAnnoCmdCtrl::AddCommandPhones
- IAnnoCmdCtrl::AddCommandMarkers

A couple of notes about the phonetics mode. It has some work, in particular, it
has difficulty with multi-word constructs.

My belief is that this is caused by a couple of things. I strongly suspect 
that we'll need to build a model adaptation on top, using speaker 
dependent methods.


@see CreateAnnoCmdAndControl
@see \ref candc_guide
@see \ref command_control Sample
**/
class IAnnoCmdCtrl
{
public:
        /// reference counting. Increment
    virtual ulong AddRef() = 0;
        /// reference counting. release
    virtual ulong Release() = 0;

   /**@brief This method adds a command using plain text<P>
   
       The return value is the command index associated with the new command.
       This value can be used to enable/disable or remove the command
       @param szText - [in] the text of the command
       @param pTxScribe - [in] the lexicon/transcriber that parses the text
       @return ulong command index, or 0 if the operation failed
     **/
    virtual ulong AddCommandText(const char* szText, ILipSyncTranscriber* pTxScribe) = 0;

    /**@brief This method adds a command based on phonemes.

        This allows application developers to keep a list of phonemes,
        pretranscribed to create the command. The phonemes can
        come from an author time transcription, or they can come
        from other sources, such as the textless phn recognizer.
        @param szPhonemes - [in] anno 40 phoneme list
        @return ulong command index, or 0 if the operation failed
    **/
    virtual ulong AddCommandPhones(const char* szPhonemes) = 0;


    /**@brief add a command by a list of transcription markers.

        This allows applications to pre-generate the markers at
        author time. This allows applications to avoid the memory
        hungry lexicons at runtime. 
        @param pBegin - [in] stl style marker for the first marker
        @param pEnd - [in] stl style marker for 1 past the last valid marker
        @return ulong command index, or 0 if the operation failed
    **/
    virtual ulong AddCommandMarkers(CTxMarker* pBegin, CTxMarker* pEnd) = 0;

    /**@brief this method is used to remove the specified command by id

        The command is no longer valid after removal.
        @param cmdId - [in] the command index
    **/
    virtual void RemoveCommand(ulong cmdId) = 0;

    /**@brief this method is used to disable/turn off the specified command.

        The command index is still valid, it's just that the specified
        command will not be used for comparison purposes during BestCommand
        @param cmdId - [in] the command index
     **/
    virtual void DisableCommand(ulong cmdId) = 0;

    /**@brief this method is used enable the specified command.

        The command will be enabled during the next BestCommand 
        run. @see DisableCommand
        @param cmdId - [in] the command index
     **/
    virtual void EnableCommand(ulong cmdId) = 0;


    /**
    @brief find the best matching command to the given audio stream

    This method runs all enabled commands against the specified audio
    stream. The best matching command is returned along with it's
    score. 
    @param pStream - [in] the audio stream
    @param pProgress - [in] progress callback (can be NULL)
    @param pScore - [out] the score of the best command
    @retval the command id of the best command.
    */
    virtual ulong BestCommand(IObservationStream* pStream,
                              CProgress* pProgress,
                              double* pScore) = 0;
};        


/**
@brief the acoustic command and control interface
@ingroup sdk_interfaces

The acoustic command & control is based on recorded samples of
specified commands. Given a list of recorded samples, it
can identify the best match for a given sample. 

To train the system, have the user record (one or more times)
the command. 

If corrective training is necessary, the best way is to simply
add new audio and associate the new command number with the 
existing command. (We may change this so watch out!).

Since the actual acoustic data is much smaller than audio data,
the best way to save the results of the command is to use
the interface to get a buffer for it.  see IAnnoCousticCmdCtrl::Save

Acoustic commands are added through the IAnnoCmdCtrl::AddCommandAudio method.

This supports two ways to retrieve the best command. Either by passing
a IObservationStream that has the audio recorded, or by iteratively
calling:
- IAnnoAcousticCmdCtrl::StartCandC
- IAnnoAcousticCmdCtrl::WriteAudioBytes 
- IAnnoAcousticCmdCtrl::BestCommand

The system has a poor implementation of an endpoint detector,
IAnnoAcousticCmdCtrl::WriteAudioBytes will return kOutOfRange when the system reaches an endpoint.
This currently is not very tolerant to noise.

@see CreateAnnoCmdAndControl
@see \ref candc_guide
@see \ref command_control Sample

*/
class IAnnoAcousticCmdCtrl
{
public:
        /// reference counting. Increment
    virtual ulong AddRef() = 0;
        /// reference counting. release
    virtual ulong Release() = 0;

  
    /** @brief this method adds an audio source for use in matching
        
        This matching strategy uses an alternate approach to matching.
        Specifically, it uses Dynamic Time Warping. Where this audio signal
        and the incoming signal are aligned together using something akin
        to a differencing algorithm. 

        The signal will use the observation stream data (MFCC presumably)
        as it's distortion measure, and for a single speaker, we should
        expect reasonable matching.
        
        @param pObs - [in] observation stream
    **/
    virtual ulong AddCommandAudio(IObservationStream* pObs) = 0;


    /**
        @brief This method is used to save the command data to a buffer

        This can be used to stream in and out commands. 
        If pBuffer is NULL, then only the size needed for the data is
        returned. 
        @param cmdId - [in] the command id
        @param pBuffer[in|out] the byte buffer for the saved data. To get
        the size needed for the buffer call this command first with pBuffer
        set to NULL.
        @return the size of the buffer saved.
    **/
    virtual ulong Save(ulong cmdId, byte* pBuffer) = 0;

	/**
		@brief This load loads an audio command from the specfied buffer

		This is the compliment to "Save". It returns a new command id associated
		with that command
	**/
	virtual ulong Load(byte* pBuffer, long nBytes) = 0;


    /**@brief this method is used to remove the specified command by id

        The command is no longer valid after removal.
        @param cmdId - [in] the command index
    **/
    virtual void RemoveCommand(ulong cmdId) = 0;

    /**@brief this method is used to disable/turn off the specified command.

        The command index is still valid, it's just that the specified
        command will not be used for comparison purposes during BestCommand
        @param cmdId - [in] the command index
     **/
    virtual void DisableCommand(ulong cmdId) = 0;

    /**@brief this method is used enable the specified command.

        The command will be enabled during the next BestCommand 
        run. @see DisableCommand
        @param cmdId - [in] the command index
     **/
    virtual void EnableCommand(ulong cmdId) = 0;

    /**
        @brief This method starts the command evaluation process

        Specify the pcm stream format of the audio data that
        is coming in. When new buffers of audio arrive,
        call WriteAudioBytes to write the audio data. Then
        use BestCommand incrementally or at one time to
        determine the best command at the specified time.
    **/
    virtual serror StartCandC(PCMStreamFormat& audioFormat) = 0;

    /**
        @brief write new audio data to the command system.

        Applications will repeatedly call this method with audio
        recorded from the user. At any time, a call to BestCommand
        will yeild the current best match. 
        
        @note WriteAudioBytes will return kErrOutOfRange when it
        detects an end condition. This is currently not working
        very robustly, it tends to be way too effected by noise,
        and may not generate an end point. Right now, it is advised
        that the application determine it's own end-points and
        then stop calling IAnnoAcousticCmdCtrl::WriteAudioBytes on it's own.
        @param pBytes - [in] audio data
        @param nBytes - [in] number of bytes of audio data
        @retval kNoError - all is well
        @retval kOutOfRange - the system has decided to stop because
        it believes there is no more speech in the system
        @retval \ref error_codes. an appropriate error.
    **/
    virtual serror WriteAudioBytes(const byte* pBytes, long nBytes) = 0;

    /**
        @brief retrieve the current best command 

        This can be called repeatedly while in a WriteAudioBytes loop.
        It will return the curreent best command along with the score.
        Set bFinal to true if this is the last one. 
        @bFinal - [in] is this the final call?
        @pScore - [out] the score of the best command
        @retval the command id of the best command, or kAnnoCmdInvalidMarker
        if no command can be matched.
    **/
    virtual ulong BestCommand(long bFinal, double* pScore) = 0;
        

    /**
    @brief find the best matching command to the given audio stream

    This method runs all enabled commands against the specified audio
    stream. The best matching command is returned along with it's
    score. 
    @param pStream - [in] the audio stream
    @param pProgress - [in] progress callback (can be NULL)
    @param pScore - [out] the score of the best command
    @retval the command id of the best command or kAnnoCmdInvalidMarker
    */
    virtual ulong BestCommand(IObservationStream* pStream,
                              CProgress* pProgress,
                              double* pScore) = 0;
};        


        

/**@brief constant defining an invalid command index<P>
This may be returned by the IAnnoCmdCtrl in response to
an operation which failed.
**/
#define kAnnoCmdInvalidMarker   0
/**
@ingroup sdk_functions
@brief this dll method is used to create a new command and control object

The command and control object will be reference counted to 1. 
Use IAnnoCmdCtrl::Release when finished.

This method calls IAnnoCmdCtrl::setAccHMM.
After calling this function, do not destroy the HMM. We
should use refcounting on the hmm instead, but for now,
just pass the cleanup on to the control system.

@param flags - [in] must be zero
@param pHMM - [in] the HMM to use for the command and control system.
@param baseEnergy - [in] cut off energy point. If it's correctly aligned
with the microphone, performance will improve. see C&C for an example of
how to obtain a reasonable value.
@param ppCmdControl - [out] the new object
@see IAnnoCmdCtrl
**/
LIBLIP_API
serror CreateAnnoCmdCtrl(ulong flags, 
                         CAccousticHMM *pHMM,
                         float baseE, 
                         IAnnoCmdCtrl** ppCmdControl);


/**
@ingroup sdk_functions
@brief this dll method is used to create a new acoustic command and control object

The command and control object will be reference counted to 1. 
Use IAnnoAcousticCmdCtrl::Release when finished.

This method calls IAnnoAcousticCmdCtrl::setAccHMM.
After calling this function, do not destroy the HMM. We
should use refcounting on the hmm instead, but for now,
just pass the cleanup on to the control system.

@param flags - [in] must be zero
@param pHMM - [in] the HMM to use for the command and control system.
@param baseEnergy - [in] cut off energy point. If it's correctly aligned
with the microphone, performance will improve. see C&C for an example of
how to obtain a reasonable value.
@param ppCmdControl - [out] the new object
@see IAnnoAcousticCmdCtrl
**/
LIBLIP_API
serror CreateAnnoAcousticCmdCtrl(ulong flags, 
                                CAccousticHMM *pHMM,
                                float baseE, 
                                IAnnoAcousticCmdCtrl** ppCmdControl);


#endif
