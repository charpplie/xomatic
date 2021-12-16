////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2005.
// -------------------------------------------------------------------------
//  File name:   LipSync.h
//  Version:     v1.00
//  Created:     17/10/2005 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __LipSync_h__
#define __LipSync_h__
#pragma once

#include <IFacialAnimation.h>
#include "VectorSet.h"

class CFacialAnimationContext;

//////////////////////////////////////////////////////////////////////////
struct SPhoneme
{
	wchar_t codeIPA;     // IPA (International Phonetic Alphabet) code.
	char    ASCII[4];    // ASCII name for this phoneme (SAMPA for English).
	string  description; // Phoneme description.
};

//////////////////////////////////////////////////////////////////////////
class CPhonemesLibrary : public IPhonemeLibrary
{
public:
	CPhonemesLibrary();

	//////////////////////////////////////////////////////////////////////////
	// IPhonemeLibrary
	//////////////////////////////////////////////////////////////////////////
	VIRTUAL int GetPhonemeCount() const;
	VIRTUAL bool GetPhonemeInfo( int nIndex,SPhonemeInfo &phoneme );
	VIRTUAL int FindPhonemeByName( const char *sPhonemeName );
	//////////////////////////////////////////////////////////////////////////

	SPhoneme& GetPhoneme( int nIndex );
	void LoadPhonemes( const char *filename );

private:
	std::vector<SPhoneme> m_phonemes;
};

//////////////////////////////////////////////////////////////////////////
class CFacialSentence : public IFacialSentence, public _reference_target_t
{
public:
	CFacialSentence();

	//////////////////////////////////////////////////////////////////////////
	// IFacialSentence
	//////////////////////////////////////////////////////////////////////////
	IPhonemeLibrary* GetPhonemeLib();
	VIRTUAL void SetText( const char *text ) { m_text = text; };
	VIRTUAL const char* GetText() { return m_text; };
	VIRTUAL void ClearAllPhonemes() { m_phonemes.clear(); m_words.clear(); ++m_nValidateID;};
	VIRTUAL int  GetPhonemeCount() { return (int)m_phonemes.size(); };
	VIRTUAL bool GetPhoneme( int index,Phoneme &ph );
	VIRTUAL int  AddPhoneme( const Phoneme &ph );

	VIRTUAL void ClearAllWords() { m_words.clear(); ++m_nValidateID;};
	VIRTUAL int GetWordCount() { return (int)m_words.size() ; };
	VIRTUAL bool GetWord( int index,Word &wrd );
	VIRTUAL void AddWord( const Word &wrd );

	VIRTUAL int Evaluate(float fTime, float fInputPhonemeStrength, int maxSamples, ChannelSample* samples);
	//////////////////////////////////////////////////////////////////////////

	int GetPhonemeFromTime( int timeMs,int nFirst=0 );

	bool GetPhonemeInfo( int phonemeId,SPhonemeInfo &phonemeInfo ) const;
	void Serialize( XmlNodeRef &node,bool bLoading );

	Phoneme& GetPhoneme( int index ) { return m_phonemes[index]; };

	void Animate( const QuatTS& rAnimLocationNext, CFacialAnimationContext *pAnimContext,float fTime,float fPhonemeStrength, const VectorSet<string, stl::less_stricmp<const char*> >& overriddenPhonemes);

	int GetValidateID() {return m_nValidateID;}

private:
	struct WordRec
	{
		string text;
		int startTime;
		int endTime;
		std::vector<Phoneme> phonemes;
	};
	string m_text;
	std::vector<Phoneme> m_phonemes;
	std::vector<WordRec> m_words;

	// If this value has changed, then the sentence has changed.
	int m_nValidateID;
};

#endif // __LipSync_h__
