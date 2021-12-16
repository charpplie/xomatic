////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2005.
// -------------------------------------------------------------------------
//  File name:   LocalizedStringManager.h
//  Version:     v1.00
//  Created:     22/9/2005 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __LocalizedStringManager_h__
#define __LocalizedStringManager_h__
#pragma once

#include <ILocalizationManager.h>
#include <StlUtils.h>

//////////////////////////////////////////////////////////////////////////
/*
	Manage Localization Data
*/
class CLocalizedStringsManager : public ILocalizationManager
{
public:
	CLocalizedStringsManager( ISystem* pSystem );
	virtual ~CLocalizedStringsManager();
	
	// ILocalizationManager
	VIRTUAL const char* GetLanguage();
	VIRTUAL bool SetLanguage( const char* sLanguage );
	VIRTUAL bool LoadExcelXmlSpreadsheet( const char* sFileName, bool bReload=false );
	VIRTUAL void ReloadData();
	VIRTUAL	void FreeData();

	VIRTUAL bool LocalizeString( const string& sString, wstring& outLocalizedString, bool bEnglish=false ) ; // currently faster
	VIRTUAL bool LocalizeString( const char* sString, wstring& outLocalizedString, bool bEnglish=false ) ;
	VIRTUAL bool LocalizeLabel( const char* sLabel, wstring& outLocalizedString, bool bEnglish=false );
	VIRTUAL bool GetLocalizedInfoByKey( const char* sKey, SLocalizedInfoGame& outGameInfo );
	VIRTUAL bool GetLocalizedInfoByKey( const char* sKey, SLocalizedSoundInfoGame* pOutSoundInfoGame );
	VIRTUAL int  GetLocalizedStringCount();
	VIRTUAL bool GetLocalizedInfoByIndex( int nIndex, SLocalizedInfoGame& outGameInfo );
	VIRTUAL bool GetLocalizedInfoByIndex( int nIndex, SLocalizedInfoEditor& outEditorInfo );
	
	VIRTUAL bool GetEnglishString( const char *sKey, string &sLocalizedString );
	VIRTUAL bool GetSubtitle( const char* sKeyOrLabel, wstring& outSubtitle, bool bForceSubtitle = false);

	VIRTUAL void FormatStringMessage( string& outString, const string& sString, const char** sParams, int nParams );
	VIRTUAL void FormatStringMessage( string& outString, const string& sString, const char* param1, const char* param2=0, const char* param3=0, const char* param4=0 );
	VIRTUAL void FormatStringMessage( wstring& outString, const wstring& sString, const wchar_t** sParams, int nParams );
	VIRTUAL void FormatStringMessage( wstring& outString, const wstring& sString, const wchar_t* param1, const wchar_t* param2=0, const wchar_t* param3=0, const wchar_t* param4=0 );

	VIRTUAL wchar_t ToUpperCase(wchar_t c);
	VIRTUAL wchar_t ToLowerCase(wchar_t c);
	VIRTUAL void LocalizeTime(time_t t, bool bMakeLocalTime, bool bShowSeconds, wstring& outTimeString);
	VIRTUAL void LocalizeDate(time_t t, bool bMakeLocalTime, bool bShort, bool bIncludeWeekday, wstring& outDateString);
	VIRTUAL void LocalizeDuration(int seconds, wstring& outDurationString);
	// ~ILocalizationManager

	int GetMemoryUsage( ICrySizer *pSizer );
	
	void FreeLocalizationData();

private:

	// Preparation for future compression, not used yet
	//////////////////////////////////////////////////////////////////////////
	// Compress block of data with zlib.
	int CompressWChar( void *dest,unsigned int &destLen,const void *source, unsigned int sourceLen,int level=6 );
	// Uncompress block of data with zlib.
	int UncompressWChar( void *dest,unsigned int &destLen,const void *source, unsigned int sourceLen );


	typedef std::map<string, int, stl::less_stricmp<string> >	StringsKeyMap;

	struct SLocalizedStringEntry
	{
		string	sKey;					// key of this entry (without @)

		wstring swTranslatedCharacterName;         // character name speaking via XML asset
		wstring swTranslatedText;		// Subtitle of this line

		string	sOriginalActorLine;			// english text
		wstring	swTranslatedActorLine;		// localized text
		string  sOriginalText; // subtitle. if empty, uses English text
		string  sOriginalCharacterName;  // english character name speaking via XML asset
		
		unsigned int nRow;		// Number of row in XML file
		// audio specific part
		string	sPrototypeSoundEvent;			// associated sound event prototype (radio, ...)
		float		fVolume;		
		float		fRadioRatio;
		// SoundMoods
		int			nNumSoundMoods;
		SLocalizedAdvancesSoundEntry* pSoundMoods;
		// EventParameters
		int			nNumEventParameters;
		SLocalizedAdvancesSoundEntry* pEventParameters;
		// ~audio specific part

		// subtitle
		bool    bUseSubtitle; // should a subtitle displayed for this key?

		// bool    bDependentTranslation; // if the english/localized text contains other localization labels
	};

	struct SLanguage
	{
		string sLanguage;
		StringsKeyMap m_keysMap;
		std::vector<SLocalizedStringEntry*> m_vLocalizedStrings;
	};

	struct SFileInfo
	{
		bool bDataStripping;
	};

	// uses class-local buffer m_tempWString 
	void AppendToUnicodeString( wstring &sDest,const string& sSource );
	void AddLocalizedString( SLanguage *pLanguage,SLocalizedStringEntry *pEntry );
	void AddControl(int nKey);
	void ReplaceEndOfLine( wchar_t *str );
	void ReplaceEndOfLine( char *str );
	//////////////////////////////////////////////////////////////////////////
	void ParseFirstLine( XmlNodeRef &rowNode, char *nCellIndexToType, std::map<int, string> &SoundMoodIndex, std::map<int, string> &EventParameterIndex );
	void InternalSetCurrentLanguage(SLanguage* pLanguage);
	ISystem* m_pSystem;
	// Pointer to the current language.
	SLanguage* m_pLanguage;

	// all loaded Localization Files
	typedef std::pair<string, SFileInfo> pairFileName;
	typedef std::map<string, SFileInfo> tmapFilenames;
	tmapFilenames m_loadedTables;
	
	// Array of loaded languages.
	std::vector<SLanguage*> m_languages;

	typedef std::set<string> PrototypeSoundEvents;
	PrototypeSoundEvents m_prototypeEvents;  // this set is purely used for clever string/string assigning to save memory

	struct less_wcscmp : public std::binary_function<const wstring&,const wstring&,bool> 
	{
		bool operator()( const wstring& left,const wstring& right ) const
		{
			return wcscmp(left.c_str(), right.c_str()) < 0;
		}
	};

	typedef std::set<wstring, less_wcscmp> CharacterNameSet;
	CharacterNameSet m_characterNameSet; // this set is purely used for clever string/string assigning to save memory

	DynArray<wchar_t> m_tempWString;

	// CVARs
	int m_cvarLocalizationDebug;
};


#endif // __LocalizedStringManager_h__
