// ***************************************************************
//  PhonemeRecognizer   version:  1.0   ·  date: 03/06/2006
//  -------------------------------------------------------------
//  Created by Timur.
//  -------------------------------------------------------------
//  Copyright (C) 2006 - All Rights Reserved
// ***************************************************************
// 
// ***************************************************************


//////////////////////////////////////////////////////////////////////////
struct ILipSyncPhonemeRecognizer
{
public:
	struct SPhoneme
	{
		enum { MAX_PHONEME_LENGTH = 8 };

		int startTime;
		int endTime;
		int	nPhonemeCode;
		char sPhoneme[MAX_PHONEME_LENGTH];
		float intensity;
	};

	struct SWord
	{
		int startTime;
		int endTime;
		char *sWord;
	};

	struct SSentance
	{
		char *sSentence;

		int nWordCount;
		SWord* pWords; // Array of words.

		int nPhonemeCount;
		SPhoneme* pPhonemes; // Array of phonemes.
	};


	virtual void Release() = 0;
	virtual bool RecognizePhonemes( const char *wavfile,const char *text,SSentance** pOutSetence ) = 0;
	virtual const char* GetLastError() = 0;
};

