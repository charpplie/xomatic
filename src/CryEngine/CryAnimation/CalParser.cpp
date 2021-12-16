/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
Description: 
-------------------------------------------------------------------------
History:
- 03:06:2009   : Created by Dmythro Tsakhilov

*************************************************************************/

#include "stdafx.h"
#include "CalParser.h"

#include "FacialAnimation/FacialModel.h"
#include "FacialAnimation/FaceEffectorLibrary.h"

// gcc only allows function attributes at the definition not the declaration
void Warning(const char* calFileName, EValidatorSeverity severity, const char *format,... ) PRINTF_PARAMS(3, 4);

void Warning(const char* calFileName, EValidatorSeverity severity, const char *format,... )
{
	if (!g_pISystem || !format)
		return;

	va_list	ArgList;
	va_start(ArgList, format);
	g_pISystem->WarningV(VALIDATOR_MODULE_ANIMATION, severity, VALIDATOR_FLAG_FILE, calFileName, format,ArgList);
	va_end(ArgList);
}

CCalParser::CCalParser()
{
	m_arrAnimFiles.reserve(0);
	m_facialAnimations.reserve(0);	
}

CCalParser::~CCalParser(void)
{
	for (uint32 i = 0, size = m_arrAnimFiles.size(); i < size; ++i)
	{
		delete m_arrAnimFiles[i];
	}

	for (uint32 i = 0, size = m_arrWildcardAnimFiles.size(); i < size; ++i)
	{
		delete m_arrWildcardAnimFiles[i];
	}
	m_arrWildcardAnimFiles.clear();

	m_arrAnimFiles.clear();
	m_parsedCalFiles.clear();
	m_modelTracksDatabases.clear();

	CAnimationSet::FacialAnimationSet::container_type().swap(m_facialAnimations);
}

bool CCalParser::Parse(const char* calFileName, string strAnimDirName, bool& parseSubFolders)
{	
	parseSubFolders = false;

	const int BITE = 512;
	m_parsedCalFiles.push_back(calFileName);

	FILE* calFile = g_pIPak->FOpen(calFileName, "r", ICryPak::FOPEN_HINT_QUIET);
	if (calFile == 0)
		return false;

	g_pIPak->FSeek(calFile, 0, SEEK_SET);

	// the flags applicable to the currently being loaded animation
	unsigned animFlags = 0;	

	for (int i = 0; calFile && !g_pIPak->FEof(calFile); ++i)
	{
		CryFixedStringT<BITE> line;
		char buffer[BITE] = "";

		int len = 0;
		g_pIPak->FGets(buffer, BITE, calFile);
		len = strlen(buffer);
		line.assign(buffer);			
		
		while ((len == BITE - 1) && (buffer[BITE - 2] != '\n') && (buffer[BITE - 2] != '\r'))
		{			
			g_pIPak->FGets(buffer, BITE, calFile);
			len = strlen(buffer);
			line.append(buffer);			
		}	
		
    CryFixedStringT<BITE> key;
		CryFixedStringT<BITE> value;		

		if(line.empty() || line.at(0) == '/' || line.at(0) == '\r' || line.at(0) == '\n')
		{
			continue;
		}		

		int pos = 0;
		key = line.Tokenize(" \t\n\r=", pos);
		if (key.empty())
		{
			continue;
		}

		value = line.Tokenize(" \t\n\r=", pos);

		if (value.empty() || value.at(0) == '?')			
		{
			continue;
		}
				
		if (0 == stricmp(key, "#filepath"))
		{
			strAnimDirName = PathUtil::ToUnixPath(value.c_str());
			strAnimDirName.TrimRight('/'); // delete the trailing slashes			
			continue;
		}

		if(0 == stricmp(key, "#ParseSubFolders"))
		{
			parseSubFolders = stricmp (value, "true") == 0? true:false; // if it's false, stricmp return 0
		}

		// remove first '\' and replace '\' with '/'
		value.replace('\\', '/');
		value.TrimLeft('/');		
		

		// process the possible directives
		if (key.at(0) == '$')
		{
			if (!stricmp(key, "$AnimationDir") || !stricmp(key, "$AnimDir")	|| !stricmp(key, "$AnimationDirectory") ||!stricmp(key, "$AnimDirectory"))
			{
				Warning(calFileName, VALIDATOR_ERROR, "Deprecated directive \"%s\"", key.c_str());
			}
			else if (!stricmp(key, "$AnimEventDatabase"))
			{
				if (m_animEventDatabase.empty())
				  m_animEventDatabase = value.c_str();
//				else
//					Warning(calFileName, VALIDATOR_WARNING, "Failed to set animation event database \"%s\". Animation event database is already set to \"%s\"", value.c_str(), m_animEventDatabase.c_str());
			}
			else if (!stricmp(key, "$TracksDatabase"))
			{
				if (!AddIfNewModelTracksDatabase(value.c_str()))
					Warning(calFileName, VALIDATOR_WARNING, "Duplicate model tracks database declared \"%s\"", value.c_str());
			}
			else if (!stricmp(key, "$Include"))
			{
				if (std::find(m_parsedCalFiles.begin(), m_parsedCalFiles.end(), value.c_str()) != m_parsedCalFiles.end())
          Warning(calFileName, VALIDATOR_WARNING, "Duplicate .cal file included \"%s\"", value.c_str());
				else
				{
					bool dummy; // will always use the top-level options for "parseSubFolders"
				  if (!Parse(value, strAnimDirName, dummy))
					  Warning(calFileName, VALIDATOR_ERROR, "Failed to include .cal file \"%s\"", value.c_str());
				}
			}
			else if (!stricmp(key, "$FaceLib"))
			{
				if (m_faceLibFile.empty())
				{
					m_faceLibFile = value.c_str();
					m_faceLibDir = strAnimDirName;
				}
				else
					Warning(calFileName, VALIDATOR_WARNING, "Failed to set face lib \"%s\". Face lib is already set to \"%s\"", value.c_str(), m_faceLibFile.c_str());
			}
			else
				Warning(calFileName, VALIDATOR_ERROR, "Unknown directive in '%s'", key.c_str());
		}
		else
		{			
			// Check whether the filename is a facial animation, by checking the extension.
			const char* szExtension = PathUtil::GetExt(value);
			string szFileName = strAnimDirName + "/" + value.c_str();			

			// is there any wildcard in the file name?
			if ( strchr(value,'*') != NULL || strchr(value,'?') != NULL )
			{
				m_arrWildcardAnimFiles.push_back( new SAnimFile(szFileName, key, animFlags) );
			}
			else
			{
				const char* failedToCreateAlias = "Failed to create animation alias \"%s\" for file \"%s\". Such alias already exists and uses file \"%s\"";
				if (szExtension && stricmp("fsq", szExtension) == 0)
				{
					int duplicateIndex = AddIfNewFacialAnimationAlias(key, szFileName.c_str());
					if (duplicateIndex >= 0)
						Warning(calFileName, VALIDATOR_WARNING, failedToCreateAlias, key.c_str(), szFileName.c_str(), m_facialAnimations[duplicateIndex].path.c_str());
				}
				else
				{
					int duplicateIndex = AddIfNewAnimationAlias(key, szFileName.c_str(), animFlags);
					if (duplicateIndex >= 0)
						Warning(calFileName, VALIDATOR_WARNING, failedToCreateAlias, key.c_str(), szFileName.c_str(), m_arrAnimFiles[duplicateIndex]->m_fileName.c_str());
				}
			}

			//check if CAF or LMG has an Aim-Pose
			uint32 numAnims = m_arrAnimFiles.size();
			if (szExtension && numAnims)
			{
				if ( (stricmp("lmg",szExtension)==0) || (stricmp("caf",szExtension)==0) )
				{
					PREFAST_SUPPRESS_WARNING(6031) strtok (buffer, " \t\n\r=");
					const char *fname = strtok(NULL, " \t\n\r=(");
					while ( (fname = strtok(NULL, " \t\n\r,()")) != NULL && fname[0] != 0 )
					{
						if ( fname[0] == '/' )
							break;
						m_arrAnimFiles.back()->arrAimPoses.push_back( fname );
					}
				}
			}


		}

	} //end of loop

	g_pIPak->FClose (calFile);	
  return true;
}


int CCalParser::AddIfNewFacialAnimationAlias( const char* animName, const char* szFileName )
{	
	for (uint32 i = 0; i < m_facialAnimations.size(); ++i)
		if (0 == stricmp(m_facialAnimations[i], animName))
			return i;

	m_facialAnimations.push_back(CAnimationSet::FacialAnimationEntry(animName, szFileName));
	return -1;
}

int CCalParser::AddIfNewAnimationAlias( const char* animName, const char* szFileName, const unsigned animFlags )
{	
	for (uint32 i = 0; i < m_arrAnimFiles.size(); ++i)
		if (0 == stricmp(m_arrAnimFiles[i]->m_animName.c_str(), animName))
			return i;

  m_arrAnimFiles.push_back(new SAnimFile(szFileName, animName, animFlags));
  return -1;
}

bool CCalParser::AddIfNewModelTracksDatabase(const char* dataBase )
{
	uint32 numDBA=m_modelTracksDatabases.size();
	if (std::find(m_modelTracksDatabases.begin(), m_modelTracksDatabases.end(), dataBase) == m_modelTracksDatabases.end())
	{
	  m_modelTracksDatabases.push_back(dataBase);
		return true;
	}
	else
		return false;	
}

