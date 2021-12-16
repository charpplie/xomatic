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

#include "StdAfx.h"

#include "LocalizedStringManager.h"
#include <IInput.h>
#include <ISystem.h>
#include "System.h" // to access InitLocalization()
#include <CryPath.h>
#include <IConsole.h>
#include <StringUtils.h>
#include <locale.h>
#include <time.h>

#define MAX_CELL_COUNT 32

// CVAR names
const char c_sys_localization_debug[] = "sys_localization_debug";

enum ELocalizedXmlColumns
{
	ELOCALIZED_COLUMN_SKIP  = 0,
	ELOCALIZED_COLUMN_KEY, 
	ELOCALIZED_COLUMN_AUDIOFILE,
	ELOCALIZED_COLUMN_ORIGINAL_CHARACTER_NAME,
	ELOCALIZED_COLUMN_TRANSLATED_CHARACTER_NAME,
	ELOCALIZED_COLUMN_ORIGINAL_TEXT,
	ELOCALIZED_COLUMN_TRANSLATED_TEXT,
	ELOCALIZED_COLUMN_ORIGINAL_ACTOR_LINE,
	ELOCALIZED_COLUMN_TRANSLATED_ACTOR_LINE,
	ELOCALIZED_COLUMN_USE_SUBTITLE,
	ELOCALIZED_COLUMN_VOLUME,
	ELOCALIZED_COLUMN_SOUNDEVENT,
	ELOCALIZED_COLUMN_RADIO_RATIO,
	ELOCALIZED_COLUMN_EVENTPARAMETER,
	ELOCALIZED_COLUMN_SOUNDMOOD,
	// legacy names
	ELOCALIZED_COLUMN_LEGACY_PERSON,
	ELOCALIZED_COLUMN_LEGACY_CHARACTERNAME,
	ELOCALIZED_COLUMN_LEGACY_TRANSLATED_CHARACTERNAME,
	ELOCALIZED_COLUMN_LEGACY_ENGLISH_DIALOGUE,
	ELOCALIZED_COLUMN_LEGACY_TRANSLATION,
	ELOCALIZED_COLUMN_LEGACY_YOUR_TRANSLATION,
	ELOCALIZED_COLUMN_LEGACY_ENGLISH_SUBTITLE,
	ELOCALIZED_COLUMN_LEGACY_TRANSLATED_SUBTITLE,
	ELOCALIZED_COLUMN_LAST,
};

// The order must match to the order of the ELocalizedXmlColumns
static const char* sLocalizedColumnNames[] = 
{
	// everyhing read by the file will be convert to lower cases
	"skip",
	"key",
	"audio_filename",
	"original character name",
	"translated character name",
	"original text",
	"translated text",
	"original actor line",
	"translated actor line",
	"use subtitle",
	"volume",
	"prototype event",
	"radio ratio",
	"eventparameter",
	"soundmood",
	// legacy names
	"person",
	"character name",
	"translated character name",
	"english dialogue",
	"translation",
	"your translation",
	"english subtitle",
	"translated subtitle",
	};





//////////////////////////////////////////////////////////////////////////
static void ReloadDialogData( IConsoleCmdArgs* pArgs )
{
	//gEnv->pSystem->GetLocalizationManager()->FreeData();
	//CSystem *pSystem = (CSystem*) gEnv->pSystem;
	//pSystem->InitLocalization();
	//pSystem->OpenBasicPaks();
	gEnv->pSystem->GetLocalizationManager()->ReloadData();
}

//////////////////////////////////////////////////////////////////////////
static void TestFormatMessage (IConsoleCmdArgs* pArgs)
{
	string fmt1 ("abc %1 def % gh%2i %");
	string fmt2 ("abc %[action:abc] %2 def % gh%1i %1");
	wstring wfmt (L"abc %2 def % gh%1i %");
	string out1, out2;
	wstring wout;
	gEnv->pSystem->GetLocalizationManager()->FormatStringMessage(out1, fmt1, "first", "second", "third");
	CryLogAlways("%s", out1.c_str());
	gEnv->pSystem->GetLocalizationManager()->FormatStringMessage(out2, fmt2, "second");
	CryLogAlways("%s", out2.c_str());
	gEnv->pSystem->GetLocalizationManager()->FormatStringMessage(wout, wfmt, L"second", L"first");
}



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
CLocalizedStringsManager::CLocalizedStringsManager( ISystem *pSystem )
: m_cvarLocalizationDebug(0)
{
	m_pSystem = pSystem;
	m_languages.reserve(4);
	m_tempWString.resize(1024); 
	m_tempWString[0] = L'\0';
	m_pLanguage = 0;

	REGISTER_COMMAND( "ReloadDialogData",ReloadDialogData,VF_NULL,
		"Reloads all localization dependent XML sheets for the currently set language.");

	REGISTER_COMMAND( "_TestFormatMessage", TestFormatMessage,VF_NULL,"" );

	REGISTER_CVAR2(c_sys_localization_debug, &m_cvarLocalizationDebug, m_cvarLocalizationDebug, VF_CHEAT,
		"Toggles debugging of the Localization Manager.\n"
		"Usage: sys_localization_debug [0/1]\n"
		"Default is 0 (off).");
}

//////////////////////////////////////////////////////////////////////
CLocalizedStringsManager::~CLocalizedStringsManager()
{
	FreeData();
}

//////////////////////////////////////////////////////////////////////
void CLocalizedStringsManager::FreeLocalizationData()
{
	for (uint32 i = 0; i < m_languages.size(); i++)
	{
		std::for_each(m_languages[i]->m_vLocalizedStrings.begin(),m_languages[i]->m_vLocalizedStrings.end(),stl::container_object_deleter());
		m_languages[i]->m_keysMap.clear();
		m_languages[i]->m_vLocalizedStrings.clear();
	}
	m_loadedTables.clear();
}

//////////////////////////////////////////////////////////////////////
void CLocalizedStringsManager::FreeData()
{
	FreeLocalizationData();

	for (uint32 i = 0; i < m_languages.size(); i++)
	{
		delete m_languages[i];
	}
	m_languages.resize(0);
	m_loadedTables.clear();

	m_pLanguage = 0;
}

//////////////////////////////////////////////////////////////////////
const char* CLocalizedStringsManager::GetLanguage()
{
	if (m_pLanguage == 0)
		return "";
	return m_pLanguage->sLanguage.c_str();
}

//////////////////////////////////////////////////////////////////////
bool CLocalizedStringsManager::SetLanguage( const char *sLanguage )
{
	if (m_cvarLocalizationDebug)
		CryLog("<Localization> Set language to %s", sLanguage);

	// Check if already language loaded.
	for (uint32 i = 0; i < m_languages.size(); i++)
	{
		if (stricmp(sLanguage,m_languages[i]->sLanguage)==0)
		{
			InternalSetCurrentLanguage(m_languages[i]);
			return true;
		}
	}

	SLanguage *pLanguage = new SLanguage;
	m_languages.push_back( pLanguage );

	if (m_cvarLocalizationDebug)
		CryLog("<Localization> Insert new language to %s", sLanguage);

	pLanguage->sLanguage = sLanguage;
	InternalSetCurrentLanguage(pLanguage);

	//------------------------------------------------------------------------------------------------- 
	// input localization
	//------------------------------------------------------------------------------------------------- 
	// keyboard
	for (int i = 0; i <= 0x80; i++)
	{
		AddControl(i);
	}

	// mouse
	for (int i = 1; i <= 0x0f; i++)
	{
		AddControl(i*0x10000);
	}

	return (true);
}

//////////////////////////////////////////////////////////////////////////
void CLocalizedStringsManager::AddControl(int nKey)
{
	//marcok: currently not implemented ... will get fixed soon
	/*IInput *pInput = m_pSystem->GetIInput();

	if (!pInput)
	{
		return;
	}

	wchar_t szwKeyName[256] = {0};
	char		szKey[256] = {0};

	if (!IS_MOUSE_KEY(nKey))
	{
		if (pInput->GetOSKeyName(nKey, szwKeyName, 255))
		{
			sprintf(szKey, "control%d", nKey);

			SLocalizedStringEntry loc;
			loc.sEnglish.Format( "%S",szwKeyName );
			loc.sLocalized = szwKeyName;
			loc.sKey = szKey;
			AddLocalizedString( m_pLanguage,loc );
		}
	}*/
}


//////////////////////////////////////////////////////////////////////////
void CLocalizedStringsManager::ParseFirstLine( XmlNodeRef &nodeRow,char *nCellIndexToType, std::map<int, string> &SoundMoodIndex, std::map<int, string> &EventParameterIndex )
{
	int nCellIndex = 0;
	int nNewIndex = 0;
	for (int cell = 0; cell < nodeRow->getChildCount(); ++cell)
	{
		if (cell >= MAX_CELL_COUNT)
			continue;

		XmlNodeRef nodeCell = nodeRow->getChild(cell);
		if (!nodeCell->isTag("Cell"))
			continue;

		if (nodeCell->getAttr("ss:Index",nNewIndex))
		{
			// Check if some cells are skipped.
			nCellIndex = nNewIndex-1;
		}

		XmlNodeRef nodeCellData = nodeCell->findChild("Data");
		if (!nodeCellData)
		{
			++nCellIndex;
			continue;
		}

		string sCellContent = nodeCellData->getContent();
		sCellContent.MakeLower();

		for (int i = 0; i < sizeof(sLocalizedColumnNames)/sizeof(sLocalizedColumnNames[0]); ++i)
		{
			const char *pFind = strstr(sCellContent.c_str(), sLocalizedColumnNames[i]);
			if ( pFind != 0)
			{
				nCellIndexToType[nCellIndex] = i;

				// find SoundMood
				if (i == ELOCALIZED_COLUMN_SOUNDMOOD)
				{
					const char *pSoundMoodName = pFind + strlen(sLocalizedColumnNames[i]) + 1;
					int nSoundMoodNameLength = sCellContent.length() - strlen(sLocalizedColumnNames[i]) - 1;
					if (nSoundMoodNameLength > 0)
					{
						SoundMoodIndex[nCellIndex] = pSoundMoodName;
					}
				}

				// find EventParameter
				if (i == ELOCALIZED_COLUMN_EVENTPARAMETER)
				{
					const char *pParameterName = pFind + strlen(sLocalizedColumnNames[i]) + 1;
					int nParameterNameLength = sCellContent.length() - strlen(sLocalizedColumnNames[i]) - 1;
					if (nParameterNameLength > 0)
					{
						EventParameterIndex[nCellIndex] = pParameterName;
					}
				}

				break;
			}
			// workaround until all columns are renamed to "Translation"
			//if (stricmp(sCellContent, "Your Translation") == 0)
			//{
			//	nCellIndexToType[nCellIndex] = ELOCALIZED_COLUMN_TRANSLATED_ACTOR_LINE;
			//	break;
			//}
		}
		++nCellIndex;
	}
}

// copy & make-lower-case & 0-terminate string
void strlwrncpy(char* dst, const char * src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;
	if (n != 0)
	{
		char c;
		while ((c=*s++) && --n) 
		{
			if ('A' <= c && c <= 'Z')
				*d++ = c + ('a' - 'A');
			else
				*d++ = c;
		}
		*d='\0';
	}
}

//////////////////////////////////////////////////////////////////////
// Loads a string-table from a Excel XML Spreadsheet file.
bool CLocalizedStringsManager::LoadExcelXmlSpreadsheet( const char *sFileName,bool bReload )
{ 
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Other, 0, "Localization XML (%s)", sFileName);

	if (!m_pLanguage)
		return false;

	//check if this table has already been loaded
	if (!bReload)
	{
		if (m_loadedTables.find(CONST_TEMP_STRING(sFileName)) != m_loadedTables.end())
			return (true);
	}

	XmlNodeRef root = m_pSystem->LoadXmlFile( sFileName );

	if (!root)
	{
		CryLog( "Loading Localization File %s failed!",sFileName );
		return false;
	}

	// bug search, re-export to a file to compare it
	//string sReExport = sFileName;
	//sReExport += ".re";
	//root->saveToFile(sReExport.c_str());

	CryLog( "Loading Localization File %s",sFileName );

	XmlNodeRef nodeWorksheet = root->findChild( "Worksheet" );
	if (!nodeWorksheet)
		return false;

	XmlNodeRef nodeTable = nodeWorksheet->findChild( "Table" );
	if (!nodeTable)
		return false;

	pairFileName sNewFile;
	sNewFile.first = sFileName;
	sNewFile.second.bDataStripping = false; // this is off for now
	m_loadedTables.insert(sNewFile);

	// Cell Index
	char nCellIndexToType[MAX_CELL_COUNT];
	memset( nCellIndexToType,0,sizeof(nCellIndexToType) );

	// SoundMood Index
	std::map<int, string> SoundMoodIndex;

	// EventParameter Index
	std::map<int, string> EventParameterIndex;

	bool bFirstRow = true;

	// lower case event name
	char szLowerCaseEvent[128];
	// lower case key
	char szLowerCaseKey[1024];

	int nMemSize = 0;

	int rows = nodeTable->getChildCount();
	m_pLanguage->m_vLocalizedStrings.reserve(rows);

	int nRowIndex = 0;
	for (int row = 0; row < nodeTable->getChildCount(); row++)
	{
		XmlNodeRef nodeRow = nodeTable->getChild(row);
		if (!nodeRow->isTag("Row"))
			continue;

		int nNewRowIndex = 0;
		if (nodeRow->getAttr("ss:Index",nNewRowIndex))
		{
			// Check if some rows are skipped.
			nRowIndex = nNewRowIndex-1;
		}
		++nRowIndex;

		if (bFirstRow)
		{
			bFirstRow = false;
			ParseFirstLine( nodeRow,nCellIndexToType, SoundMoodIndex, EventParameterIndex );
			// Skip first row, it only have description.
			continue;
		}

		bool bValidKey = false;
		bool bValidTranslatedText = false;
		bool bValidTranslatedCharacterName = false;
		bool bUseSubtitle = true;
		const char* sKeyString = "";
		const char* sOriginalCharacterName = "";
		const char* sTranslatedCharacterName = "";
		const char* sOriginalText = "";
		const char* sTranslatedText = "";
		const char* sOriginalActorLine = "";
		const char* sTranslatedActorLine = "";
		const char* sSoundEvent = "";
		float fVolume = 1.0f;
		float fRadioRatio = 1.0f;
		float fEventParameterValue = 0.0f;
		float fSoundMoodValue = 0.0f;

		int nCellIndex = 0;
		int nNewIndex = 0;
		int nItems = 0;
		std::map<int, float> SoundMoodValues;
		std::map<int, float> EventParameterValues;

		for (int cell = 0; cell < nodeRow->getChildCount(); cell++)
		{
			XmlNodeRef nodeCell = nodeRow->getChild(cell);
			if (!nodeCell->isTag("Cell"))
				continue;

			if (nodeCell->getAttr("ss:Index",nNewIndex))
			{
				// Check if some cells are skipped.
				nCellIndex = nNewIndex-1;
			}
			if (nCellIndex >= MAX_CELL_COUNT)
				break;

			XmlNodeRef nodeCellData = nodeCell->findChild("Data");
			if (!nodeCellData)
			{
				// output warning
#if defined(USER_alexl)
				// does it contain an unsupported ss:Data tag?
				nodeCellData = nodeCell->findChild("ss:Data");
				if (nodeCellData)
				{
					if (*sKeyString)
					{
						const char* content = nodeCellData->getContent();
						if (content && *content)
						{
							CryWarning( VALIDATOR_MODULE_SYSTEM,VALIDATOR_WARNING,"[LocError] File '%s' : Key '%s' contains unsupported formatting data in Row=%d Col=%d ('%s')", 
								sFileName, sKeyString, nRowIndex, nCellIndex+1, content);
						}
					}
				}
#endif
				// in any case, we don't accept that entry!
				++nCellIndex;
				continue;
			}

			char nCellType = nCellIndexToType[nCellIndex];

			const char *sCellContent = nodeCellData->getContent();

			switch (nCellType)
			{
			case ELOCALIZED_COLUMN_SKIP:
				break;
			case ELOCALIZED_COLUMN_KEY:
				sKeyString = sCellContent;
				bValidKey = sKeyString[0] != 0;
				++nItems;
				break;
			case ELOCALIZED_COLUMN_AUDIOFILE:
				sKeyString = sCellContent;
				bValidKey = sKeyString[0] != 0;
				++nItems;
				break;
			case ELOCALIZED_COLUMN_ORIGINAL_CHARACTER_NAME:
				sOriginalCharacterName = sCellContent;
				++nItems;
				break;
			case ELOCALIZED_COLUMN_TRANSLATED_CHARACTER_NAME:
				sTranslatedCharacterName = sCellContent;
				bValidTranslatedCharacterName = sTranslatedCharacterName[0] != 0;
				++nItems;
				break;
			case ELOCALIZED_COLUMN_ORIGINAL_TEXT:
				sOriginalText = sCellContent;
				break;
			case ELOCALIZED_COLUMN_TRANSLATED_TEXT:
				sTranslatedText = sCellContent;
				bValidTranslatedText = sTranslatedText[0] != 0;
				break;
			case ELOCALIZED_COLUMN_ORIGINAL_ACTOR_LINE:
				sOriginalActorLine = sCellContent;
				++nItems;
				break;
			case ELOCALIZED_COLUMN_TRANSLATED_ACTOR_LINE:
				sTranslatedActorLine = sCellContent;
				++nItems;
				break;
			case ELOCALIZED_COLUMN_USE_SUBTITLE:
				bUseSubtitle = CryStringUtils::toYesNoType(sCellContent) == CryStringUtils::nYNT_No ? false : true; // favor yes (yes and invalid -> yes)
				break;
			case ELOCALIZED_COLUMN_VOLUME:
				fVolume = (float)atof(sCellContent);
				++nItems;
				break;
			case ELOCALIZED_COLUMN_SOUNDEVENT:
				sSoundEvent = sCellContent;
				++nItems;
				break;
			case ELOCALIZED_COLUMN_RADIO_RATIO:
				fRadioRatio = (float)atof(sCellContent);
				++nItems;
				break;
			case ELOCALIZED_COLUMN_EVENTPARAMETER:
				fEventParameterValue = (float)atof(sCellContent);
				EventParameterValues[nCellIndex] = fEventParameterValue;
				++nItems;
				break;
			case ELOCALIZED_COLUMN_SOUNDMOOD:
				fSoundMoodValue = (float)atof(sCellContent);
				SoundMoodValues[nCellIndex] = fSoundMoodValue;
				++nItems;
				break;
			// legacy names
			case ELOCALIZED_COLUMN_LEGACY_PERSON:
				// old file often only have content in this column
				if (sCellContent)
				{
					sOriginalCharacterName = sCellContent;
					sTranslatedCharacterName = sCellContent;
					bValidTranslatedCharacterName = sTranslatedCharacterName[0] != 0;
				}
				++nItems;
				break;
			case ELOCALIZED_COLUMN_LEGACY_CHARACTERNAME:
				if (sCellContent)
				{
					sOriginalCharacterName = sCellContent;
					sTranslatedCharacterName = sCellContent;
					bValidTranslatedCharacterName = sTranslatedCharacterName[0] != 0;
				}
				++nItems;
				break;
			case ELOCALIZED_COLUMN_LEGACY_TRANSLATED_CHARACTERNAME:
				if (sCellContent)
				{
					sTranslatedCharacterName = sCellContent;
					bValidTranslatedCharacterName = sTranslatedCharacterName[0] != 0;
				}
				++nItems;
				break;
			case ELOCALIZED_COLUMN_LEGACY_ENGLISH_DIALOGUE:
				// old file often only have content in this column
				if (sCellContent)
				{
					sOriginalActorLine = sCellContent;
					sOriginalText = sCellContent;
				}
				++nItems;
				break;
			case ELOCALIZED_COLUMN_LEGACY_TRANSLATION:
				if (sCellContent)
				{
					sTranslatedActorLine = sCellContent;
					sTranslatedText = sCellContent;
					bValidTranslatedText = sTranslatedText[0] != 0;
				}
				++nItems;
				break;
			case ELOCALIZED_COLUMN_LEGACY_YOUR_TRANSLATION:
				if (sCellContent)
				{
					sTranslatedActorLine = sCellContent;
					sTranslatedText = sCellContent;
					bValidTranslatedText = sTranslatedText[0] != 0;
				}
				++nItems;
				break;
			case ELOCALIZED_COLUMN_LEGACY_ENGLISH_SUBTITLE:
				if (sCellContent)
				{
					sOriginalText = sCellContent;
				}
				++nItems;
				break;
			case ELOCALIZED_COLUMN_LEGACY_TRANSLATED_SUBTITLE:
				if (sCellContent)
				{
					sTranslatedText = sCellContent;
					sTranslatedActorLine = sCellContent;
					bValidTranslatedText = sTranslatedText[0] != 0;
				}
				++nItems;
				break;
			}

			++nCellIndex;

		}

		if (!bValidKey)
			continue;

		if (!bValidTranslatedText)
		{
			if (m_cvarLocalizationDebug)
				CryWarning( VALIDATOR_MODULE_SYSTEM,VALIDATOR_WARNING,"[LocError] Key '%s' in file <%s> has no translated text", sKeyString, sFileName);

			// use translated actor line entry if availible before falling back to original entry
			if (sTranslatedActorLine[0] != 0)
				sTranslatedText = sTranslatedActorLine;
			else
				sTranslatedText = sOriginalText;
		}

		if (sSoundEvent[0] != 0 && !bValidTranslatedCharacterName)
		{
			if (m_cvarLocalizationDebug)
				CryWarning( VALIDATOR_MODULE_SYSTEM,VALIDATOR_WARNING,"[LocError] Key '%s' in file <%s> has no translated character name", sKeyString, sFileName);

			sTranslatedCharacterName = sOriginalCharacterName;
		}

		if (nItems==1) // skip lines which contain just one item in the key
			continue;

		// reject to store text if line was marked with no subtitles in game mode
		if (!bUseSubtitle && !gEnv->bEditor)
		{
			sOriginalText = "";
			sTranslatedText = "";
		}

		// Skip @ character in the key string.
		if (sKeyString[0] == '@')
			sKeyString++;

		strlwrncpy(szLowerCaseEvent, sSoundEvent, sizeof(szLowerCaseEvent));
		strlwrncpy(szLowerCaseKey, sKeyString, sizeof(szLowerCaseKey));

		if (m_pLanguage->m_keysMap.find(CONST_TEMP_STRING(szLowerCaseKey)) != m_pLanguage->m_keysMap.end())
		{
			CryWarning( VALIDATOR_MODULE_SYSTEM,VALIDATOR_WARNING,"[LocError] Localized String '%s' Already Loaded for Language %s", sKeyString,m_pLanguage->sLanguage.c_str());
			continue;
		}

		if (*sOriginalActorLine)
		{
			ReplaceEndOfLine( const_cast<char*>(sOriginalActorLine) );
		}

		SLocalizedStringEntry *pEntry = new SLocalizedStringEntry;
		pEntry->sKey = szLowerCaseKey;
		//pEntry->sOriginalActorLine = sOriginalActorLine;
		pEntry->bUseSubtitle = bUseSubtitle;
		pEntry->nRow = nRowIndex;

		if (*sOriginalActorLine && gEnv->bEditor)
		{
			ReplaceEndOfLine( const_cast<char*>(sOriginalActorLine) );
			pEntry->sOriginalActorLine = sOriginalActorLine;
		}
		if (*sTranslatedActorLine && gEnv->bEditor)
		{
			ReplaceEndOfLine( const_cast<char*>(sTranslatedActorLine) );
			AppendToUnicodeString( pEntry->swTranslatedActorLine,sTranslatedActorLine );
		}
		if (bUseSubtitle && *sOriginalText && gEnv->bEditor)
		{
			ReplaceEndOfLine( const_cast<char*>(sOriginalText) );
			pEntry->sOriginalText = sOriginalText;
		}

		if (bUseSubtitle && *sTranslatedText)
		{
			ReplaceEndOfLine( const_cast<char*>(sTranslatedText) );
			AppendToUnicodeString( pEntry->swTranslatedText, sTranslatedText );
		}

		// the following is used to cleverly assign strings
		// we store all known string into the m_prototypeEvents set and assign known entries from there
		// the CryString makes sure, that only the ref-count is increment on assignment
		if (*szLowerCaseEvent)
		{
			PrototypeSoundEvents::iterator it = m_prototypeEvents.find( CONST_TEMP_STRING(szLowerCaseEvent) );
			if (it != m_prototypeEvents.end())
				pEntry->sPrototypeSoundEvent = *it;
			else
			{
				pEntry->sPrototypeSoundEvent = szLowerCaseEvent;
				m_prototypeEvents.insert(pEntry->sPrototypeSoundEvent);
			}
		}

		// only use the translated character name
		if (gEnv->bEditor)
			pEntry->sOriginalCharacterName = sOriginalCharacterName;

		const char* sWho = *sTranslatedCharacterName ? sTranslatedCharacterName : sOriginalCharacterName;
		if (*sWho)
		{
			ReplaceEndOfLine( const_cast<char*>(sWho) );
			wstring tmp;
			AppendToUnicodeString(tmp, sWho);
			CharacterNameSet::iterator it = m_characterNameSet.find(tmp);
			if (it != m_characterNameSet.end())
				pEntry->swTranslatedCharacterName = *it;
			else
			{
				pEntry->swTranslatedCharacterName = tmp;
				m_characterNameSet.insert(pEntry->swTranslatedCharacterName);
			}
		}

		pEntry->fVolume = fVolume;

		// SoundMood Entries
		pEntry->nNumSoundMoods = SoundMoodValues.size();
		if (pEntry->nNumSoundMoods > 0)
		{
			pEntry->pSoundMoods = new SLocalizedAdvancesSoundEntry[pEntry->nNumSoundMoods];
			std::map<int, float>::const_iterator itEnd = SoundMoodValues.end();
			int nSoundMoodCount = 0;
			for(std::map<int, float>::const_iterator it = SoundMoodValues.begin(); it != itEnd; ++it)
			{
				pEntry->pSoundMoods[nSoundMoodCount].fValue = (*it).second;
				pEntry->pSoundMoods[nSoundMoodCount].sName = SoundMoodIndex[(*it).first];
				++nSoundMoodCount;
			}
		}
		else
			pEntry->pSoundMoods = NULL;

		// EventParameter Entries
		pEntry->nNumEventParameters = EventParameterValues.size();
		if (pEntry->nNumEventParameters > 0)
		{
			pEntry->pEventParameters = new SLocalizedAdvancesSoundEntry[pEntry->nNumEventParameters];
			std::map<int, float>::const_iterator itEnd = EventParameterValues.end();
			int nEventParameterCount = 0;
			for(std::map<int, float>::const_iterator it = EventParameterValues.begin(); it != itEnd; ++it)
			{
				pEntry->pEventParameters[nEventParameterCount].fValue = (*it).second;
				pEntry->pEventParameters[nEventParameterCount].sName = EventParameterIndex[(*it).first];
				++nEventParameterCount;
			}
		}
		else
			pEntry->pEventParameters = NULL;

		pEntry->fRadioRatio = fRadioRatio;
		nMemSize += sizeof(*pEntry) + pEntry->sKey.length() 
			+ pEntry->sOriginalActorLine.length() + pEntry->swTranslatedActorLine.length()*sizeof(wchar_t) 
			+ pEntry->sOriginalText.length() + pEntry->swTranslatedText.length()*sizeof(wchar_t) 
			+ pEntry->sOriginalCharacterName.length() + pEntry->swTranslatedCharacterName.length()*sizeof(wchar_t);
		

		// Compression Preparation
		//unsigned int nSourceSize = pEntry->swTranslatedText.length()*sizeof(wchar_t);
		//if (nSourceSize)
		//	int zResult = Compress(pDest, nDestLen, pEntry->swTranslatedText.c_str(), nSourceSize);

		AddLocalizedString( m_pLanguage,pEntry );
	}
	return (true);
}

//////////////////////////////////////////////////////////////////////////
void CLocalizedStringsManager::ReloadData()
{
	tmapFilenames temp = m_loadedTables;

	FreeLocalizationData();
	for (tmapFilenames::iterator it = temp.begin(); it != temp.end(); it++)
	{
		LoadExcelXmlSpreadsheet( (*it).first, true );
	}
}

//////////////////////////////////////////////////////////////////////////
void CLocalizedStringsManager::ReplaceEndOfLine( wchar_t *str )
{
	wchar_t *s;
	while ((s = wcsstr(str,L"\\n")) != 0)
	{
		s[0] = L' ';
		s[1] = L'\n';
	}
}

//////////////////////////////////////////////////////////////////////////
void CLocalizedStringsManager::ReplaceEndOfLine( char *str )
{
	char *s;
	while ((s = strstr(str,"\\n")) != 0)
	{
		s[0] = ' ';
		s[1] = '\n';
	}
}

//////////////////////////////////////////////////////////////////////////
void CLocalizedStringsManager::AddLocalizedString( SLanguage *pLanguage,SLocalizedStringEntry *pEntry )
{
	pLanguage->m_vLocalizedStrings.push_back(pEntry);
	int nId = (int)pLanguage->m_vLocalizedStrings.size()-1;
	pLanguage->m_keysMap[pEntry->sKey] = nId;

	if (m_cvarLocalizationDebug)
		CryLog("<Localization> Add new string <%s> with ID %d to <%s>", pEntry->sKey.c_str(), nId, pLanguage->sLanguage.c_str());
}

//////////////////////////////////////////////////////////////////////////
bool CLocalizedStringsManager::LocalizeString(const char* sString, wstring& outLocalizedString, bool bEnglish)
{ 	
	string tmpString(sString);
	return LocalizeString(tmpString, outLocalizedString, bEnglish);
}

//////////////////////////////////////////////////////////////////////////
bool CLocalizedStringsManager::LocalizeString(const string& sString, wstring& outLocalizedString, bool bEnglish)
{ 	
	assert (m_pLanguage);
	if (m_pLanguage == 0)
	{
		CryWarning( VALIDATOR_MODULE_SYSTEM,VALIDATOR_WARNING,"LocalizeString: No language set.");
		AppendToUnicodeString(outLocalizedString, sString);
		return false;
	}

	outLocalizedString.assign (L"");

	// scan the string
	bool done = false;

	int len = sString.length();
	int curpos = 0;
	while (!done)
	{
		int pos = sString.find_first_of("@", curpos);
		if (pos == string::npos)
		{
			// did not find anymore, so we are done
			done = true;
			pos = len;
		}
		// found an occurrence

		// we have skipped a few characters, so copy them over
		if (pos > curpos)
		{
			// skip
			AppendToUnicodeString( outLocalizedString,sString.substr(curpos, pos - curpos) );
			curpos = pos;
		}

		if (curpos == pos)
		{
			// find the end of the label
			int endpos = sString.find_first_of(" ", curpos);
			if (endpos == string::npos)
			{
				// have reached the end
				done = true;
				endpos = len;
			}

			if (endpos > curpos)
			{
				// localize token		
				if (bEnglish)
				{
					string sLocalizedToken;
					GetEnglishString(sString.substr(curpos, endpos - curpos).c_str(), sLocalizedToken);
					AppendToUnicodeString( outLocalizedString,sLocalizedToken );
				}
				else
				{
					wstring sLocalizedToken;
					LocalizeLabel(sString.substr(curpos, endpos - curpos), sLocalizedToken);
					// append
					outLocalizedString+=sLocalizedToken;
				}			

				curpos = endpos;
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CLocalizedStringsManager::LocalizeLabel(const char* sLabel, wstring& outLocalString, bool bEnglish)
{
	assert(sLabel);
	if (!m_pLanguage || !sLabel)
		return false;

	// Label sign.
	if (sLabel[0] == '@')
	{
		int nId = -1;
		// if it continues with cc_ it's a control code
		if (sLabel[1] && (sLabel[1] == 'c' || sLabel[1] == 'C') && 
			  sLabel[2] && (sLabel[2] == 'c' || sLabel[2] == 'C') && 
				sLabel[3] && sLabel[3] == '_')
		{
			nId = stl::find_in_map( m_pLanguage->m_keysMap,CONST_TEMP_STRING(sLabel+1),-1 ); // skip @ character.
			if (nId < 0)
			{
				// controlcode
				// lookup KeyName
				IInput* pInput = gEnv->pInput;
				SInputEvent ev;
				ev.deviceId = eDI_Keyboard;
				ev.keyName = sLabel+4; // skip @cc_
				const char* keyName = pInput->GetKeyName(ev, true);

				if (keyName && keyName[0] == 0)
				{
					// try OS key
					const wchar_t* wKeyName = pInput->GetOSKeyName(ev);
					if (wKeyName && *wKeyName)
					{
						outLocalString.assign(wKeyName);
						return true;
					}
					// if we got some empty, try non-keyboard as well
					ev.deviceId = eDI_Unknown;
					keyName = pInput->GetKeyName(ev, true);
				}
				
				if (keyName && *keyName)
				{
					if (keyName[1])
					{
						// in case we get a string back, which is not localized
						// fake missing localization!
						outLocalString.assign(L"");
						AppendToUnicodeString( outLocalString, string(sLabel) );
						return true;
					}
					wchar_t tmpString[2];
					tmpString[0] = 0;
#if defined(LINUX) || defined(PS3)
					mbstowcs( tmpString, keyName, 1 );
#else
					MultiByteToWideChar( CP_ACP, 0, keyName, -1, tmpString, 1 );
#endif
					tmpString[0] = ToUpperCase(tmpString[0]); // use localized version!
					tmpString[1] = 0;
					outLocalString.assign(tmpString);
					return true;
				}
			}
			// we found an localized entry (e.g. @cc_space), use normal translation -> Space/Leertaste
		}
		else
			nId = stl::find_in_map( m_pLanguage->m_keysMap,CONST_TEMP_STRING(sLabel+1),-1 ); // skip @ character.
		if (nId >= 0)
		{
			if (bEnglish || m_pLanguage->m_vLocalizedStrings[nId]->swTranslatedText.empty())
			{
				assert(!"No Localization Text availible!");
				outLocalString = L"";
				AppendToUnicodeString( outLocalString,m_pLanguage->m_vLocalizedStrings[nId]->sOriginalText );
				return true;
			}
			else
			{
				outLocalString = m_pLanguage->m_vLocalizedStrings[nId]->swTranslatedText;
			}
			return true;
		}
		else 
		{
			CryWarning( VALIDATOR_MODULE_SYSTEM,VALIDATOR_WARNING,"Localized string for Label <%s> not found", sLabel );
		}
	}
	else
	{
		CryWarning( VALIDATOR_MODULE_SYSTEM,VALIDATOR_WARNING,"Not a valid localized string Label <%s>, must start with @ symbol", sLabel );
	}

	outLocalString = L"";
	AppendToUnicodeString( outLocalString,sLabel );

	return false;
}


//////////////////////////////////////////////////////////////////////////
bool CLocalizedStringsManager::GetEnglishString( const char *sKey, string &sLocalizedString )
{
	assert(sKey);
	if (!m_pLanguage || !sKey)
		return false;

	// Label sign.
	if (sKey[0] == '@')
	{
		int nId = stl::find_in_map( m_pLanguage->m_keysMap,CONST_TEMP_STRING(sKey+1),-1 ); // skip @ character.
		if (nId >= 0)
		{
			sLocalizedString = m_pLanguage->m_vLocalizedStrings[nId]->sOriginalText;
			return true;
		}
		else
		{
			nId = stl::find_in_map( m_pLanguage->m_keysMap,CONST_TEMP_STRING(sKey),-1 ); 
			if (nId >= 0)
			{
				sLocalizedString = m_pLanguage->m_vLocalizedStrings[nId]->sOriginalText;
				return true;
			}
			else
			{
				// CryWarning( VALIDATOR_MODULE_SYSTEM,VALIDATOR_WARNING,"Localized string for Label <%s> not found", sKey );
				sLocalizedString = sKey;
				return false;
			}
		}
	}
	else
	{
		// CryWarning( VALIDATOR_MODULE_SYSTEM,VALIDATOR_WARNING,"Not a valid localized string Label <%s>, must start with @ symbol", sKey );
	}

	sLocalizedString = sKey;
	return false;
}


//////////////////////////////////////////////////////////////////////////
void CLocalizedStringsManager::AppendToUnicodeString( wstring &sDest,const string& sSource )
{
	size_t len = sSource.length();
	m_tempWString.reserve(len+1);   // post-cond: m_tempWString.capacity() >= maxSize
	wchar_t* tmpString = m_tempWString.begin();
	tmpString[0] = L'\0';

#if defined(LINUX) || defined(PS3)
	mbstowcs( tmpString, sSource.c_str(), len );
	tmpString[len] = L'\0';
#else
	MultiByteToWideChar( CP_UTF8,0,sSource.c_str(),-1,tmpString,len );
	tmpString[len] = L'\0';
#endif

	sDest += tmpString;

/*
#if 1
	size_t maxSize = sSource.length() + 1;
	m_tempWString.reserve(maxSize);   // post-cond: m_tempWString.capacity() >= maxSize
	wchar_t* tmpString = m_tempWString.begin();
	wchar_t* d = tmpString;
	const char* s = sSource.c_str();
	while (--maxSize > 0) // NOTE: preincrement, because maxSize = length+1
		mbtowc(d++, s++, 1);
	*d = L'\0';
#else
	size_t len = sSource.length();
	m_tempWString.reserve(len+1);   // post-cond: m_tempWString.capacity() >= maxSize
	wchar_t* tmpString = m_tempWString.begin();
	tmpString[0] = L'\0';

#if defined(LINUX) || defined(PS3)
	mbstowcs( tmpString, sSource.c_str(), len );
	tmpString[len] = L'\0';
#else
	MultiByteToWideChar( CP_UTF8,0,sSource.c_str(),-1,tmpString,len );
#endif

#endif
	// append to dest string
	sDest += tmpString;
	*/
}

//////////////////////////////////////////////////////////////////////////
bool CLocalizedStringsManager::GetLocalizedInfoByKey(const char* sKey, SLocalizedInfoGame& outGameInfo)
{
	if (!m_pLanguage)
		return false;
	const int nIndex = stl::find_in_map(m_pLanguage->m_keysMap, CONST_TEMP_STRING(sKey), -1);
	if (nIndex >= 0)
		return GetLocalizedInfoByIndex(nIndex, outGameInfo);
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////
bool CLocalizedStringsManager::GetLocalizedInfoByKey(const char *sKey, SLocalizedSoundInfoGame* pOutSoundInfo)
{
	assert(sKey);
	if (!m_pLanguage || !sKey || !pOutSoundInfo)
		return false;

	bool bResult = false;

	const int nIndex = stl::find_in_map(m_pLanguage->m_keysMap, CONST_TEMP_STRING(sKey), -1);
	if (nIndex >= 0)
	{
		bResult = true;
		const SLocalizedStringEntry* pEntry = m_pLanguage->m_vLocalizedStrings[nIndex];

		pOutSoundInfo->swTranslatedCharacterName = pEntry->swTranslatedCharacterName;
		pOutSoundInfo->swTranslatedText = pEntry->swTranslatedText;

		//pOutSoundInfo->sOriginalActorLine = pEntry->sOriginalActorLine.c_str();
		//pOutSoundInfo->sTranslatedActorLine = pEntry->swTranslatedActorLine.c_str();
		//pOutSoundInfo->sOriginalText = pEntry->sOriginalText;
		// original Character

		pOutSoundInfo->sSoundEvent = pEntry->sPrototypeSoundEvent.c_str();
		pOutSoundInfo->fVolume = pEntry->fVolume;
		pOutSoundInfo->fRadioRatio = pEntry->fRadioRatio;
		pOutSoundInfo->bUseSubtitle = pEntry->bUseSubtitle;

		//SoundMoods
		if (pOutSoundInfo->nNumSoundMoods >= pEntry->nNumSoundMoods)
		{
			// enough space to copy data
			int i = 0;
			for (; i<pEntry->nNumSoundMoods; ++i)
			{
				pOutSoundInfo->pSoundMoods[i].sName = pEntry->pSoundMoods[i].sName;
				pOutSoundInfo->pSoundMoods[i].fValue = pEntry->pSoundMoods[i].fValue;
			}
			// if mode is available fill it with default
			for (; i<pOutSoundInfo->nNumSoundMoods; ++i)
			{
				pOutSoundInfo->pSoundMoods[i].sName = "";
				pOutSoundInfo->pSoundMoods[i].fValue = 0.0f;
			}
			pOutSoundInfo->nNumSoundMoods = pEntry->nNumSoundMoods;
		}
		else
		{
			// not enough memory, say what is needed
			pOutSoundInfo->nNumSoundMoods = pEntry->nNumSoundMoods;
			bResult = (pOutSoundInfo->pSoundMoods == NULL); // only report error if memory was provided but is too small
		}

		//EventParameters
		if (pOutSoundInfo->nNumEventParameters >= pEntry->nNumEventParameters)
		{
			// enough space to copy data
			int i = 0;
			for (; i<pEntry->nNumEventParameters; ++i)
			{
				pOutSoundInfo->pEventParameters[i].sName = pEntry->pEventParameters[i].sName;
				pOutSoundInfo->pEventParameters[i].fValue = pEntry->pEventParameters[i].fValue;
			}
			// if mode is available fill it with default
			for (; i<pOutSoundInfo->nNumEventParameters; ++i)
			{
				pOutSoundInfo->pEventParameters[i].sName = "";
				pOutSoundInfo->pEventParameters[i].fValue = 0.0f;
			}
			pOutSoundInfo->nNumEventParameters = pEntry->nNumEventParameters;
		}
		else
		{
			// not enough memory, say what is needed
			pOutSoundInfo->nNumEventParameters = pEntry->nNumEventParameters;
			bResult = (pOutSoundInfo->pSoundMoods == NULL); // only report error if memory was provided but is too small
		}
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
int CLocalizedStringsManager::GetLocalizedStringCount()
{
	if (!m_pLanguage)
		return 0;
	return m_pLanguage->m_vLocalizedStrings.size();
}

//////////////////////////////////////////////////////////////////////////
bool CLocalizedStringsManager::GetLocalizedInfoByIndex(int nIndex, SLocalizedInfoGame& outGameInfo)
{
	if (!m_pLanguage)
		return false;
	const std::vector<SLocalizedStringEntry*>& entryVec = m_pLanguage->m_vLocalizedStrings;
	if (nIndex < 0 || nIndex >= (int)entryVec.size())
		return false;
	const SLocalizedStringEntry* pEntry = entryVec[nIndex];
	outGameInfo.sKey = pEntry->sKey;
	outGameInfo.swTranslatedCharacterName = pEntry->swTranslatedCharacterName;
	outGameInfo.swTranslatedText = pEntry->swTranslatedText;

	//outInfo.sOriginalActorLine = pEntry->sOriginalActorLine;
	//outInfo.swTranslatedActorLine = pEntry->swTranslatedActorLine;
	//outInfo.sOriginalText = pEntry->sOriginalText;
	//outInfo.sOriginalCharacterName = pEntry->sOriginalCharacterName;
	
	outGameInfo.nRow = pEntry->nRow;
	outGameInfo.bUseSubtitle = pEntry->bUseSubtitle;
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CLocalizedStringsManager::GetLocalizedInfoByIndex(int nIndex, SLocalizedInfoEditor& outEditorInfo)
{
	if (!m_pLanguage)
		return false;
	const std::vector<SLocalizedStringEntry*>& entryVec = m_pLanguage->m_vLocalizedStrings;
	if (nIndex < 0 || nIndex >= (int)entryVec.size())
		return false;
	const SLocalizedStringEntry* pEntry = entryVec[nIndex];
	outEditorInfo.sKey = pEntry->sKey;
	outEditorInfo.swTranslatedCharacterName = pEntry->swTranslatedCharacterName;
	outEditorInfo.swTranslatedText = pEntry->swTranslatedText;

	outEditorInfo.sOriginalActorLine = pEntry->sOriginalActorLine;
	outEditorInfo.swTranslatedActorLine = pEntry->swTranslatedActorLine;

	//outEditorInfo.sOriginalText = pEntry->sOriginalText;
	outEditorInfo.sOriginalCharacterName = pEntry->sOriginalCharacterName;

	outEditorInfo.nRow = pEntry->nRow;
	outEditorInfo.bUseSubtitle = pEntry->bUseSubtitle;
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CLocalizedStringsManager::GetSubtitle( const char* sKeyOrLabel, wstring& outSubtitle, bool bForceSubtitle)
{
	assert(sKeyOrLabel);
	if (!m_pLanguage || !sKeyOrLabel || !*sKeyOrLabel)
		return false;
	if (*sKeyOrLabel == '@')
		++sKeyOrLabel;
	const int nIndex = stl::find_in_map(m_pLanguage->m_keysMap, CONST_TEMP_STRING(sKeyOrLabel), -1);
	if (nIndex >= 0)
	{
		const SLocalizedStringEntry* pEntry = m_pLanguage->m_vLocalizedStrings[nIndex];
		if (pEntry->bUseSubtitle == false && !bForceSubtitle)
			return false;

		// TODO verify that no fallback is needed
		
		outSubtitle = pEntry->swTranslatedText;

		if (pEntry->swTranslatedText.empty() == false)
			outSubtitle = pEntry->swTranslatedText;
		else if (pEntry->swTranslatedActorLine.empty() == false)
			outSubtitle = pEntry->swTranslatedActorLine;
		else if (pEntry->sOriginalText.empty() == false)
		{
			outSubtitle.resize(0);
			AppendToUnicodeString(outSubtitle, pEntry->sOriginalText);
		}
		else  if (pEntry->sOriginalActorLine.empty() == false)
		{
			outSubtitle.resize(0);
			AppendToUnicodeString(outSubtitle, pEntry->sOriginalActorLine);
		}
		else  if (pEntry->swTranslatedText.empty() == false)
		{
			outSubtitle.resize(0);
			outSubtitle = pEntry->swTranslatedText;
		}
		return true;
	}
	return false;
}

template<typename StringClass, typename CharType>
void _InternalFormatStringMessage(StringClass& outString, const StringClass& sString, const CharType** sParams, int nParams)
{
	static const CharType token = (CharType) '%';
	static const CharType tokens1[2] = { token, (CharType) '\0' };
	static const CharType tokens2[3] = { token, token, (CharType) '\0' };

	int maxArgUsed = 0;
	int lastPos = 0;
	int curPos = 0;
	const int sourceLen = sString.length();
	while (true)
	{
		int foundPos = sString.find(token, curPos);
		if (foundPos != string::npos)
		{
			if (foundPos+1 < sourceLen)
			{
				const int nArg = sString[foundPos+1] -'1';
				if (nArg >= 0 && nArg <= 9)
				{
					if (nArg < nParams)
					{
						outString.append(sString, lastPos, foundPos-lastPos);
						outString.append(sParams[nArg]);
						curPos = foundPos+2;
						lastPos = curPos;
						maxArgUsed = std::max(maxArgUsed, nArg);
					}
					else
					{
						StringClass tmp (sString);
						tmp.replace(tokens1, tokens2);
						if(sizeof(*tmp.c_str()) == sizeof(char))
							CryWarning( VALIDATOR_MODULE_SYSTEM,VALIDATOR_WARNING,"Parameter for argument %d is missing. [%s]", nArg+1, (const char*)tmp.c_str() );
						else
							CryWarning( VALIDATOR_MODULE_SYSTEM,VALIDATOR_WARNING,"Parameter for argument %d is missing. [%S]", nArg+1, (const wchar_t*)tmp.c_str() );
						curPos = foundPos+1;
					}
				}
				else
					curPos = foundPos+1;
			}
			else
				curPos = foundPos+1;
		}
		else
		{
			outString.append(sString, lastPos, sourceLen-lastPos );
			break;
		}
	}
	if (maxArgUsed < nParams-1)
	{
		StringClass tmp (sString);
		tmp.replace(tokens1, tokens2);
		if(sizeof(*tmp.c_str()) == sizeof(char))
			CryWarning( VALIDATOR_MODULE_SYSTEM,VALIDATOR_WARNING,"Not all parameters used for [%s] (Req:%d/Given:%d)", (const char*)tmp.c_str(), maxArgUsed+1, nParams );
		else
			CryWarning( VALIDATOR_MODULE_SYSTEM,VALIDATOR_WARNING,"Not all parameters used for [%S] (Req:%d/Given:%d)", (const wchar_t*)tmp.c_str(), maxArgUsed+1, nParams );
	}
}

template<typename StringClass, typename CharType>
void _InternalFormatStringMessage(StringClass& outString, const StringClass& sString, const CharType* param1, const CharType* param2=0, const CharType* param3=0, const CharType* param4=0)
{
	static const int MAX_PARAMS = 4;
	const CharType* params[MAX_PARAMS] = { param1, param2, param3, param4 };
	int nParams = 0;
	while (nParams < MAX_PARAMS && params[nParams])
		++nParams;
	_InternalFormatStringMessage(outString, sString, params, nParams);
}

//////////////////////////////////////////////////////////////////////////
void CLocalizedStringsManager::FormatStringMessage(string& outString, const string& sString, const char** sParams, int nParams)
{
	_InternalFormatStringMessage(outString, sString, sParams, nParams);
}

//////////////////////////////////////////////////////////////////////////
void CLocalizedStringsManager::FormatStringMessage(string& outString, const string& sString, const char* param1, const char* param2, const char* param3, const char* param4)
{
	_InternalFormatStringMessage(outString, sString, param1, param2, param3, param4);
}

//////////////////////////////////////////////////////////////////////////
void CLocalizedStringsManager::FormatStringMessage(wstring& outString, const wstring& sString, const wchar_t** sParams, int nParams)
{
	_InternalFormatStringMessage(outString, sString, sParams, nParams);
}

//////////////////////////////////////////////////////////////////////////
void CLocalizedStringsManager::FormatStringMessage(wstring& outString, const wstring& sString, const wchar_t* param1, const wchar_t* param2, const wchar_t* param3, const wchar_t* param4)
{
	_InternalFormatStringMessage(outString, sString, param1, param2, param3, param4);
}

//////////////////////////////////////////////////////////////////////////
int CLocalizedStringsManager::GetMemoryUsage( ICrySizer *pSizer )
{
	int nSize = 0;
	for (uint32 i = 0; i < m_languages.size(); i++)
	{
		for (std::vector<SLocalizedStringEntry*>::iterator it = m_languages[i]->m_vLocalizedStrings.begin(),
			end = m_languages[i]->m_vLocalizedStrings.end(); it != end; ++it)
		{
			const SLocalizedStringEntry *pEntry = *it;
			nSize += sizeof(*pEntry);

			// who and prototype event don't count here, because they are shared
			nSize += pEntry->sKey.capacity();
			//nSize += pEntry->sOriginalCharacterName.capacity();
			//nSize += pEntry->swTranslatedCharacterName.capacity()*sizeof(wchar_t);
			nSize += pEntry->sOriginalText.capacity();
			nSize += pEntry->swTranslatedText.capacity()*sizeof(wchar_t);
			nSize += pEntry->sOriginalActorLine.capacity();
			nSize += pEntry->swTranslatedActorLine.capacity()*sizeof(wchar_t);
			
		}
		nSize += m_languages[i]->m_keysMap.size() * sizeof(StringsKeyMap::value_type);
	}
	// prototype events
	for (PrototypeSoundEvents::const_iterator it = m_prototypeEvents.begin(); it != m_prototypeEvents.end(); ++it)
		nSize += it->capacity();

	// character names
	for (CharacterNameSet::const_iterator it = m_characterNameSet.begin(); it != m_characterNameSet.end(); ++it)
		nSize += it->capacity();

	nSize += m_tempWString.capacity()*sizeof(DynArray<wchar_t>::value_type);
	nSize += sizeof(*this);
	pSizer->AddObject( this,nSize );
	return nSize;
}

#if defined (WIN32) || defined(WIN64)
namespace
{
	struct LanguageID 
	{
		const char*   language;
		unsigned long lcID;
	};

	LanguageID languageIDArray[] =
	{
		{ "Chinese",   0x404 }, // 1028
		{ "Czech",     0x405 }, // 1029
		{ "English",   0x409 }, // 1033
		{ "French",    0x40C }, // 1036
		{ "German",    0x407 }, // 1031
		{ "Hungarian", 0x40E }, // 1038
		{ "Italian",   0x410 }, // 1040
		{ "Japanese",  0x411 }, // 1041
		{ "Korean",    0x412 }, // 1042
		{ "Polish",    0x415 }, // 1045
		{ "Russian",   0x419 }, // 1049
		{ "Spanish",   0x40A }, // 1034
		{ "Thai",      0x41E }, // 1054
		{ "Turkish",   0x41F }, // 1055
	};

	const size_t numLanguagesIDs = sizeof(languageIDArray) / sizeof(languageIDArray[0]);

	LanguageID GetLanguageID(const char* language)
	{
		// default is English (US)
		const LanguageID defaultLanguage = { "English", 0x409 };
		for (int i = 0; i<numLanguagesIDs; ++i)
		{
			if (stricmp(language, languageIDArray[i].language) == 0)
				return languageIDArray[i];
		}
		return defaultLanguage;
	}

	LanguageID g_currentLanguageID = { 0, 0 };
};
#endif

void CLocalizedStringsManager::InternalSetCurrentLanguage(CLocalizedStringsManager::SLanguage *pLanguage)
{
	m_pLanguage = pLanguage;
#if defined (WIN32) || defined(WIN64)
	if (m_pLanguage != 0)
		g_currentLanguageID = GetLanguageID(m_pLanguage->sLanguage);
	else
	{
		g_currentLanguageID.language = 0;
		g_currentLanguageID.lcID = 0;
	}
#endif
	// TODO: on Linux based systems we should now set the locale
	// Enabling this on windows something seems to get corrupted...
	// on Windows we always use Windows Platform Functions, which use the lcid










}

void CLocalizedStringsManager::LocalizeDuration(int seconds, wstring &outDurationString)
{
	int s = seconds;
	int d,h,m;
	d  = s / 86400;
	s -= d * 86400;
	h = s / 3600;
	s -= h * 3600;
	m = s / 60;
	s = s - m * 60;
	string str;
	if (d > 1)
		str.Format("%d @ui_days %02d:%02d:%02d", d, h, m, s);
	else if (d > 0)
		str.Format("%d @ui_day %02d:%02d:%02d", d, h, m, s);
	else if (h > 0)
		str.Format("%02d:%02d:%02d", h, m, s);
	else
		str.Format("%02d:%02d", m, s);
	LocalizeString(str, outDurationString);
}

#if defined (WIN32) || defined(WIN64)
namespace 
{
	void UnixTimeToFileTime(time_t unixtime, FILETIME* filetime)
	{ 
		LONGLONG longlong = Int32x32To64(unixtime, 10000000) + 116444736000000000; 
		filetime->dwLowDateTime = (DWORD) longlong;
		filetime->dwHighDateTime = (DWORD)(longlong >> 32);
	}

	void UnixTimeToSystemTime(time_t unixtime, SYSTEMTIME* systemtime)
	{ 
		FILETIME filetime;
		UnixTimeToFileTime(unixtime, &filetime);
		FileTimeToSystemTime(&filetime, systemtime); 
	} 

	time_t UnixTimeFromFileTime(const FILETIME* filetime)
	{
		LONGLONG longlong = filetime->dwHighDateTime;
		longlong <<= 32; longlong |= filetime->dwLowDateTime;
		longlong -= 116444736000000000; 
		return longlong / 10000000;
	}

	time_t UnixTimeFromSystemTime(const SYSTEMTIME* systemtime)
	{ 
		// convert systemtime to filetime
		FILETIME filetime; 
		SystemTimeToFileTime(systemtime, &filetime);
		// convert filetime to unixtime
		time_t unixtime = UnixTimeFromFileTime(&filetime);
		return unixtime;
	}
};

wchar_t CLocalizedStringsManager::ToUpperCase(wchar_t c)
{
	wchar_t widechar;
	LCID lcID = g_currentLanguageID.lcID ? g_currentLanguageID.lcID : LOCALE_USER_DEFAULT;
	/* convert wide char to uppercase */
	if ( 0 == ::LCMapStringW(lcID, LCMAP_UPPERCASE,	(LPCWSTR)&c, 1,	(LPWSTR)&widechar, 1))
	{
		return c;
	}
	return widechar;
}

wchar_t CLocalizedStringsManager::ToLowerCase(wchar_t c)
{
	wchar_t widechar;
	LCID lcID = g_currentLanguageID.lcID ? g_currentLanguageID.lcID : LOCALE_USER_DEFAULT;
	/* convert wide char to lowercase */
	if ( 0 == ::LCMapStringW(lcID, LCMAP_LOWERCASE,	(LPCWSTR)&c, 1,	(LPWSTR)&widechar, 1))
	{
		return c;
	}
	return widechar;
}

void CLocalizedStringsManager::LocalizeTime(time_t t, bool bMakeLocalTime, bool bShowSeconds, wstring& outTimeString)
{
	if (bMakeLocalTime)
	{
		struct tm thetime;
		thetime = *localtime(&t);
		t = gEnv->pTimer->DateToSecondsUTC(thetime);
	}
	outTimeString.resize(0);
	LCID lcID = g_currentLanguageID.lcID ? g_currentLanguageID.lcID : LOCALE_USER_DEFAULT;
	DWORD flags = bShowSeconds == false ? TIME_NOSECONDS : 0;
	SYSTEMTIME systemTime;
	UnixTimeToSystemTime(t, &systemTime);
	int len = ::GetTimeFormatW(lcID, flags, &systemTime, 0, 0, 0);
	if (len > 0)
	{
		// len includes terminating null!
		CryFixedWStringT<256> tmpString;
		tmpString.resize(len);
		::GetTimeFormatW(lcID, flags, &systemTime, 0, (wchar_t*) tmpString.c_str(), len);
		outTimeString.assign(tmpString.c_str(), len-1);
	}
}

void CLocalizedStringsManager::LocalizeDate(time_t t, bool bMakeLocalTime, bool bShort, bool bIncludeWeekday, wstring& outDateString)
{
	if (bMakeLocalTime)
	{
		struct tm thetime;
		thetime = *localtime(&t);
		t = gEnv->pTimer->DateToSecondsUTC(thetime);
	}
	outDateString.resize(0);
	LCID lcID = g_currentLanguageID.lcID ? g_currentLanguageID.lcID : LOCALE_USER_DEFAULT;
	SYSTEMTIME systemTime;
	UnixTimeToSystemTime(t, &systemTime);

	// len includes terminating null!
	CryFixedWStringT<256> tmpString;

	if (bIncludeWeekday)
	{
		// Get name of day
		int len = ::GetDateFormatW(lcID, 0, &systemTime, L"ddd", 0, 0);
		if (len > 0)
		{
			// len includes terminating null!
			tmpString.resize(len);
			::GetDateFormatW(lcID, 0, &systemTime, L"ddd", (wchar_t*) tmpString.c_str(), len);
			outDateString.append(tmpString.c_str(), len-1);
			outDateString.append(L" ");
		}
	}
	DWORD flags = bShort ? DATE_SHORTDATE : DATE_LONGDATE;
	int len = ::GetDateFormatW(lcID, flags, &systemTime, 0, 0, 0);
	if (len > 0)
	{
		// len includes terminating null!
		tmpString.resize(len);
		::GetDateFormatW(lcID, flags, &systemTime, 0, (wchar_t*) tmpString.c_str(), len);
		outDateString.append(tmpString.c_str(), len-1);
	}
}

//////////////////////////////////////////////////////////////////////////
// zlib defines WIN32 so must be last in file.
//////////////////////////////////////////////////////////////////////////
#include "zlib/zlib.h"
//////////////////////////////////////////////////////////////////////////
int CLocalizedStringsManager::CompressWChar( void *dest,unsigned int &destLen,const void *source, unsigned int sourceLen,int level )
{
	unsigned long dlen = destLen;
	int res = ::compress2( (Bytef*)dest, &dlen, (const Bytef*)source, sourceLen, Z_BEST_COMPRESSION );
	destLen = dlen;
	return res;
}

//////////////////////////////////////////////////////////////////////////
int CLocalizedStringsManager::UncompressWChar( void *dest,unsigned int &destLen,const void *source, unsigned int sourceLen )
{
	unsigned long dlen = destLen;
	int res = ::uncompress( (Bytef*)dest,&dlen,(const Bytef*)source,sourceLen );
	destLen = dlen;
	return res;
}

#else // #if defined (WIN32) || defined(WIN64)

// on non windows system we rely on setlocale and CRT functions
wchar_t CLocalizedStringsManager::ToUpperCase(wchar_t c)
{
	return towupper(c);
}

wchar_t CLocalizedStringsManager::ToLowerCase(wchar_t c)
{
	return towlower(c);
}

void CLocalizedStringsManager::LocalizeTime(time_t t, bool bMakeLocalTime, bool bShowSeconds, wstring& outTimeString)
{
	struct tm theTime;
	if (bMakeLocalTime)
		theTime = *localtime(&t);
	else
		theTime = *gmtime(&t);

	wchar_t buf[256];
	const size_t bufSize = sizeof(buf) / sizeof(buf[0]);
	wcsftime(buf, bufSize, bShowSeconds ? L"%#X" :L"%X", &theTime);
	buf[bufSize-1] = 0;
	outTimeString.assign(buf);
}

void CLocalizedStringsManager::LocalizeDate(time_t t, bool bMakeLocalTime, bool bShort, bool bIncludeWeekday, wstring& outDateString)
{
	struct tm theTime;
	if (bMakeLocalTime)
		theTime = *localtime(&t);
	else
		theTime = *gmtime(&t);

	wchar_t buf[256];
	const size_t bufSize = sizeof(buf) / sizeof(buf[0]);
	const wchar_t* format = bShort ? (bIncludeWeekday ? L"%a %x" : L"%x") : L"%#x"; // long format always contains Weekday name
	wcsftime(buf, bufSize, format, &theTime);
	buf[bufSize-1] = 0;
	outDateString.assign(buf);
}



#endif

#include UNIQUE_VIRTUAL_WRAPPER(ILocalizationManager)