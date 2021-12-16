// ***************************************************************
//  PhonemeAnalyzer   version:  1.0   ·  date: 03/06/2006
//  -------------------------------------------------------------
//  Created by Timur.
//  -------------------------------------------------------------
//  Copyright (C) 2006 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************

#include <stdlib.h>
#include <stdio.h>

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "Annosoft/liblipsync.h"
#include "Annosoft/libtranscribe.h"
#include "Annosoft/libarticulator.h"
#include "Annosoft/stfloatingpointstate.h"
#include "Annosoft/liblipsync_license.h"
#include "LipSyncPhonemeRecognizer.h"

#define ANNO_MODEL_FILE "HiQ40_20_38generic.hmm"
#define ANNO_ALEX_FILE "en_lexicon.alex"

typedef void* (*CreateInterfaceFunc)(const char *pName, int *pReturnCode);

class CLipSyncPhonemeRecognizer : public ILipSyncPhonemeRecognizer
{
public:
	CLipSyncPhonemeRecognizer()
	{
		m_outSentence.sSentence = 0;
		m_outSentence.nWordCount = 0;
		m_outSentence.pWords = 0;
		m_outSentence.pPhonemes = 0;
		m_outSentence.nPhonemeCount = 0;
		strcpy(m_sError,"");

		// when your program boots 
		// run this code
		// It contains the runtime licensing codes
		// for your dll.

		// the keys
		const char* cszLicenseKey = "685E-574B-4B57-5C4E-5170-5E5B-5E56-4D4D-565E-5B4E-4058-5C4B-5E69-5E3F-3D31-4E3F";
		const char* cszUserName = "HARALD_SEELEY";
		const char* cszCompanyName = "CRYTEK";

		ILibLipsyncRegistrar* pRegistrar = NULL;
		// pull the registrar interface from the lipsync tool 
		// registrar defined in liblipsync_license.h
		LipsyncGetInfoMgr(&pRegistrar);
		if (pRegistrar)
		{
			pRegistrar->SetUserName(cszUserName);
			pRegistrar->SetCompanyName(cszCompanyName);
			pRegistrar->SetLicenseKey(cszLicenseKey);
			pRegistrar->Release();
		}

	}
	~CLipSyncPhonemeRecognizer()
	{
		ClearSentence(&m_outSentence);
	}

	//////////////////////////////////////////////////////////////////////////
	virtual void Release() { delete this; };
	virtual bool RecognizePhonemes( const char *wavfile,const char *text,SSentance** pOutSetence );
	virtual const char* GetLastError() { return m_sError; };
	//////////////////////////////////////////////////////////////////////////

protected:
	virtual void SetLastError( const char* str) { strcpy(m_sError,str); };
	bool RecognizeTextBased( const char *wavfile,const char *text );
	bool RecognizeTextless( const char *wavfile );
	void ProcessLipSyncResult( ISyncResultsCollection* pCollection,const char* szText );
	void ClearSentence( SSentance *pSentence )
	{
		delete []pSentence->sSentence;

		for (int w = 0; w < pSentence->nWordCount; w++)
		{
			delete []pSentence->pWords[w].sWord;
		}
		delete []pSentence->pPhonemes;
		delete []pSentence->pWords;
	}

private:
	SSentance m_outSentence;
	HMODULE m_hPhonemeExtractorDLL;
	char m_sError[1024];
};


extern "C"
{
	__declspec(dllexport) ILipSyncPhonemeRecognizer* CreatePhonemeParser()
	{
		ILipSyncPhonemeRecognizer* p = new CLipSyncPhonemeRecognizer;
		return p;
	}
}

//////////////////////////////////////////////////////////////////////////
char* ReadFileIntoMemory(const char* szFile, long& nSize, bool bAsString)
{
	FILE *f = fopen(szFile, "rb");
	if (!f)
		return (NULL);
	fseek(f, 0, SEEK_END);
	nSize = ftell(f); 
	fseek(f, 0, SEEK_SET);
	long nAlloc = nSize;
	if (bAsString)
		nAlloc++;
	char *ret = new char [nAlloc];
	if (ret)
	{
		nSize = (long)fread(ret, sizeof(char), nSize, f);
		if (bAsString)
			ret[nSize] = 0x00;
	}
	fclose(f);
	return (ret);
}


class CProgressReporter : public CProgress
{
	virtual void SetMessage (char *szMessage)
	{

	}

	virtual bool UpdateAndCheck (long nValue, long nMax)
	{
		return false;
	}
};


//////////////////////////////////////////////////////////////////////////
bool CLipSyncPhonemeRecognizer::RecognizePhonemes( const char *wavfile,const char *strText,SSentance** pOutSetence )
{
	CStFloatingPointState fpAutoState;
	*pOutSetence = &m_outSentence;
	
	m_outSentence.sSentence = new char[strlen(strText)+1];
	strcpy( m_outSentence.sSentence,strText );

	if (strlen(strText) > 0)
		return RecognizeTextBased( wavfile,strText );
	else
		return RecognizeTextless( wavfile );
}

//////////////////////////////////////////////////////////////////////////
bool CLipSyncPhonemeRecognizer::RecognizeTextBased( const char *wavfile,const char *strText )
{
	CLipSyncAccousticHMM*    pHmm = NULL;        // speech model object
	ITextBasedPhnRecognizer *pAnnotator = NULL;  // recognizer object
	IObservationStream      *pObs = NULL;        // audio data stream
	ILipSyncTranscriber     *pTxScribe = NULL;   // transcription object
	ISyncResultsCollection  *pSyncResults = NULL;// syncronization results
	CProgressReporter        progress;

	long      nHmmBytes;        // hmm loading variable                        
	char       *pHmmData = NULL;  // hmm loading variable
	char        szError[320];     // error string
	char       *pDictData = NULL; // dictionary loading variable

	char       *pTextData = NULL;
	serror      err = kNoError;

	{
		// load the hmm from an external model file
		pHmmData = ReadFileIntoMemory( ANNO_MODEL_FILE, nHmmBytes, false);
		if (!pHmmData)
		{
			SetLastError( ::GetLipsyncErrorMessage(kErrMemory) );
			return false;
		}

		// 2) create the HMM with the loaded model data
		err = CreateAccHMM(pHmmData, nHmmBytes, szError, &pHmm);
		if (err != kNoError)
		{
			SetLastError( szError );
			return false;
		}
		// 3) trigrammer doesn't do anything for text based

		// 4) Create the Transcriber
		err = CreateTranscriberFromLexiconFile( ANNO_ALEX_FILE,&pTxScribe);
		if (err != kNoError)
		{
			SetLastError( ::GetLipsyncErrorMessage(err) );
			return false;
		}

		// 4a) Apply the phoneme constraints from the alex file
		// Some languages work better with constraints, these languages will have
		// their own constraints embedded in the alex file.
		long n_size;
		const char* pConstraints = pTxScribe->GetPhoneDurationConstraints(&n_size);
		// unconditionally set it. This method handles NULL just fine
		::LoadPhoneConstraintsIntoHMM(pHmm, pConstraints, n_size, 0);

		/*
		// 5) load auxillary phoneme constraints (if any)
		if (args.strPhoneLimitsFile.size())
		{
		::LoadPhoneConstraintsFileIntoHMM(pHmm, args.strPhoneLimitsFile.c_str(),
		0);
		}
		*/



		// 6) create the observation stream from our audio source (library function)
		// This will also open mp3 files since we added the IInstallableAudioFileReader for mp3
		// at program startup
		err = ::CreateObservationStreamFromAudioFile(
			wavfile, // [in] audio file
			pHmm,                 // [in] hmm 
			&pObs                 // [out] observation stream
			);


		if (err != kNoError)
		{
			SetLastError( ::GetLipsyncErrorMessage(err) );
			return false;
		}


		// 7) Create the Annotator and SyncResults object
		err = ::CreateTextBasedRecognizer(&pAnnotator);
		if (err != kNoError)
		{
			SetLastError( ::GetLipsyncErrorMessage(err) );
			return false;
		}

		// 8) Create the sync results collection 
		//  options: 
		//  ISyncResultsCollection::opt_intensity_as_morph
		//        output intensities that correspond better to the "openness" of the mouth
		//
		//  ISyncResultsCollection::opt_energies
		//        output energy CSyncMarkers as well. Each state being a state of the hmm
		err = ::CreateSyncResultsCollection(ISyncResultsCollection::opt_intensity_as_morph, &pSyncResults);
		if (err != kNoError)
		{
			SetLastError( ::GetLipsyncErrorMessage(err) );
			return false;
		}

		// 9) Perform the recognition
		err = pAnnotator->RecognizePhonemes(
			pObs,            /* [in] observation stream */
			pHmm,            /* [in] hmm */
			strText,         /* [in] source text */
			pTxScribe,       /* [in] transcriber */
			&progress,       /* [in] progress */
			pSyncResults     /* [out] markers */
			);
		if (err != kNoError && err != kErrCancelled)
		{
			SetLastError( ::GetLipsyncErrorMessage(err) );
		}

		// kErrVirterbiFailed error may still generate phoneme data up to the point
		// of failure. go ahead an process the phoneme data.

		// 10) Do something with the results
		ProcessLipSyncResult( pSyncResults,strText );
	}
	// cleanup;

	// cleanup our load buffers
	delete []pHmmData;    // delete the model data
	delete []pDictData;

	// delete the sync objects
	DestroySyncResultsCollection(pSyncResults);
	DestroyAccHMM(pHmm);
	DestroyTextBasedRecognizer(pAnnotator);
	DestroyObservationStream(pObs);
	DestroyTranscriber(pTxScribe);

	return (err == kNoError || err == kErrCancelled);
}

//////////////////////////////////////////////////////////////////////////
bool CLipSyncPhonemeRecognizer::RecognizeTextless( const char *wavfile )
{
	CLipSyncAccousticHMM*    pHmm = NULL;        // speech model object
	ITextlessPhnRecognizer  *pAnnotator = NULL;  // recognizer object
	IObservationStream      *pObs = NULL;        // audio data stream
	ISyncResultsCollection  *pSyncResults = NULL;// syncronization results
	CProgressReporter        progress;

	long      nHmmBytes;        // hmm loading variable                        
	char       *pHmmData = NULL;  // hmm loading variable
	char        szError[320];     // error string
	char       *pDictData = NULL; // dictionary loading variable

	char       *pTextData = NULL;
	serror      err = kNoError;

	{
		// load the hmm from an external model file
		pHmmData = ReadFileIntoMemory( ANNO_MODEL_FILE, nHmmBytes, false);
		if (!pHmmData)
		{
			SetLastError( ::GetLipsyncErrorMessage(kErrMemory) );
			return false;
		}

		// 2) create the HMM with the loaded model data
		err = CreateAccHMM(pHmmData, nHmmBytes, szError, &pHmm);
		if (err != kNoError)
		{
			SetLastError( szError );
			return false;
		}

		// 3) Load the trigrammer into the HMM (optional but recommended)
		err = ::LoadTrigramFileIntoHMM(pHmm, "anno.trig",false);
		if (err != kNoError) 
		{
			SetLastError( ::GetLipsyncErrorMessage(kErrMemory) );
			return false;
		}

		// 6) create the observation stream from our audio source (library function)
		// This will also open mp3 files since we added the IInstallableAudioFileReader for mp3
		// at program startup
		err = ::CreateObservationStreamFromAudioFile(
			wavfile, // [in] audio file
			pHmm,                 // [in] hmm 
			&pObs                 // [out] observation stream
			);


		if (err != kNoError)
		{
			SetLastError( ::GetLipsyncErrorMessage(err) );
			return false;
		}


		// 7) Create the Annotator and SyncResults object
		err = ::CreateTextlessPhnRecognizer(&pAnnotator);
		if (err != kNoError)
		{
			SetLastError( ::GetLipsyncErrorMessage(err) );
			return false;
		}

		// 8) Create the sync results collection 
		//  options: 
		//  ISyncResultsCollection::opt_intensity_as_morph
		//        output intensities that correspond better to the "openness" of the mouth
		//
		//  ISyncResultsCollection::opt_energies
		//        output energy CSyncMarkers as well. Each state being a state of the hmm
		err = ::CreateSyncResultsCollection(ISyncResultsCollection::opt_intensity_as_morph, &pSyncResults);
		if (err != kNoError)
		{
			SetLastError( ::GetLipsyncErrorMessage(err) );
			return false;
		}

		// 9) Perform the recognition
		err = pAnnotator->RecognizePhonemes(
			pObs,            /* [in] observation stream */
			pHmm,            /* [in] hmm */
			&progress,       /* [in] progress */
			pSyncResults     /* [out] markers */
			);
		if (err != kNoError && err != kErrCancelled)
		{
			SetLastError( ::GetLipsyncErrorMessage(err) );
		}

		// kErrVirterbiFailed error may still generate phoneme data up to the point
		// of failure. go ahead an process the phoneme data.

		// 10) Do something with the results
		ProcessLipSyncResult( pSyncResults,"" );
	}
	// cleanup;

	// cleanup our load buffers
	delete []pHmmData;    // delete the model data
	delete []pDictData;

	// delete the sync objects
	DestroySyncResultsCollection(pSyncResults);
	DestroyAccHMM(pHmm);
	DestroyTextlessPhnRecognizer(pAnnotator);
	DestroyObservationStream(pObs);

	return (err == kNoError || err == kErrCancelled);
}

//////////////////////////////////////////////////////////////////////////
inline int CountWordsInCollection( ISyncResultsCollection* pCollection )
{
	CSyncMarker *pMarker = NULL;
	int nWords = 0;

	for (CSyncMarker *pMarker = pCollection->begin(); pMarker != pCollection->end(); pMarker++)
	{
		if (pMarker->type == CSyncMarker::word)
			nWords++;
	}
	return nWords;
}

//////////////////////////////////////////////////////////////////////////
void CLipSyncPhonemeRecognizer::ProcessLipSyncResult( ISyncResultsCollection* pCollection,const char* szText )
{
	IPhnMixtureArticulator *pArt = NULL;

	//////////////////////////////////////////////////////////////////////////
	//if (parms.bGenerateArticulatorData)
	if (false)
	{
		serror err = CreatePhonemeArticulator(&pArt, 0);
		if (err == kNoError)
		{
			// change the frame rate. default 30 fps
			//pArt->SetFrameRate(20.0f);    // 20 frames per second

			// if desired
			//pArt->SetMaxPhonemesPerArticulation(5);

			// turn on smoothing. This rounds things out
			pArt->SetFlags(af_phn_smooth);

			// the maximum allowed change for a phoneme from frame to frame
			// default .25
			//pArt->SetMaxFrameDelta(.20f);

			// generate the articulations
			pArt->GenerateArticulations(pCollection, NULL);
		}
	}

	SSentance *pSentence = &m_outSentence;

	int nWords = CountWordsInCollection(pCollection);
	pSentence->nWordCount = nWords;
	pSentence->pWords = new SWord[nWords];

	int w = 0;
	for (CSyncMarker *pWordMarker = pCollection->begin(); pWordMarker != pCollection->end(); pWordMarker++)
	{
		if (w >= nWords)
			break;
		if (pWordMarker->type == CSyncMarker::word)
		{
			SWord &word = pSentence->pWords[w];
			w++;
			word.sWord = new char[strlen(pWordMarker->otherLabel)+1];
			strcpy( word.sWord,pWordMarker->otherLabel );
			word.startTime = pWordMarker->milliStart;
			word.endTime = pWordMarker->milliEnd;
		}
	}



	int nPhonemes = 0;
	// Go though all phonemes.
	{
		for (CSyncMarker *pPhonemedMarker = pCollection->begin(); pPhonemedMarker != pCollection->end(); pPhonemedMarker++)
		{
			if (pPhonemedMarker->type == CSyncMarker::phoneme)
			{
				// Phoneme is within this word 
				nPhonemes++;
			}
		}
	}
	if (nPhonemes > 0)
	{
		int phn = 0;
		pSentence->nPhonemeCount = nPhonemes;
		pSentence->pPhonemes = new SPhoneme[nPhonemes];
		for (CSyncMarker *pPhonemedMarker = pCollection->begin(); pPhonemedMarker != pCollection->end(); pPhonemedMarker++)
		{
			if (pPhonemedMarker->type == CSyncMarker::phoneme)
			{
				// Phoneme is within this word 
				SPhoneme &phoneme = pSentence->pPhonemes[phn];
				memset( phoneme.sPhoneme,0,sizeof(phoneme.sPhoneme) );
				memcpy( phoneme.sPhoneme,pPhonemedMarker->szPhoneme,3 );
				// Normalize intensity in 0-1 range.
				phoneme.intensity = pPhonemedMarker->intensity / 100.0f;
				if (phoneme.intensity < 0)
					phoneme.intensity = 0;
				if (phoneme.intensity > 1)
					phoneme.intensity = 1;
				phoneme.startTime = pPhonemedMarker->milliStart;
				phoneme.endTime = pPhonemedMarker->milliEnd;
				phn++;
			}
		}
	}
}
