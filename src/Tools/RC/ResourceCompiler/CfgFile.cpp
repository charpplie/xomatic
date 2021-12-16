////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   cfgfile.cpp
//  Version:     v1.00
//  Created:     4/11/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

//
// Configuration file class.
// Use format similar to windows .ini files.
//
#include "StdAfx.h"
#include "cfgfile.h"
#include "Config.h"
#include "DebugLog.h"

//#define  RCLog DebugLog
 #define  RCLog while (false)


///////////////////////////////////////////////////////////////////////////////
//
// Class CfgFile implementation.
//
///////////////////////////////////////////////////////////////////////////////

CfgFile::CfgFile() 
{
	// Create IsEmpty section.
	Section section;
	section.name = "";
	m_sections.push_back( section );

	m_modified = false;
}


CfgFile::CfgFile( const string &fileName )
{
	// Create IsEmpty section.
	Section section;
	section.name = "";
	m_sections.push_back( section );
	m_modified = false;

	Load( fileName );
}

CfgFile::CfgFile( const char *buf,int bufSize ) {
	// Create IsEmpty section.
	Section section;
	section.name = "";
	m_sections.push_back( section );
	m_modified = false;

	LoadBuf( buf,bufSize );
}

CfgFile::~CfgFile()
{
}

//!
void CfgFile::SetFileName( const string &fileName )
{
	m_fileName = fileName;
}

// Load configuration file.
bool CfgFile::Load( const string &fileName )
{
	m_fileName = fileName;
	m_modified = false;

	FILE *file = fopen( fileName.c_str(),"rb" );
	if (!file)
	{
		char szCWD[0x400];
		getcwd(szCWD, sizeof(szCWD));
		RCLog("Can't open \"%s\"", fileName.c_str());
		RCLog("CWD=%s", szCWD);
		return false;
	}

	fseek( file,0,SEEK_END );
	int size = ftell(file);
	fseek( file,0,SEEK_SET );
		
	// Read whole file to memory.
	char *s = (char*)malloc( size+1 );
	memset( s,0,size+1 );
	fread( s,1,size,file );
	LoadBuf( s,size );
	free(s);

	fclose(file);

	return true;
}




// Save configuration file, with the stored name in m_fileName
bool CfgFile::Save( void )
{
	FILE *file = fopen(m_fileName.c_str(),"wb");

	if(!file)
		return(false);

	// Loop on sections.
	for (std::vector<Section>::iterator si = m_sections.begin(); si != m_sections.end(); si++)
	{
		Section &sec =*si;

		if(sec.name!="")
			fprintf(file,"[%s]\r\n",sec.name);																						// section

		for (std::list<Entry>::iterator it = sec.entries.begin(); it != sec.entries.end(); ++it)
		{
			if((*it).key=="")
				fprintf(file,"%s\r\n",(*it).value.c_str());															// comment
			 else
				fprintf(file,"%s=%s\r\n",(*it).key.c_str(),(*it).value.c_str());		// key=value
		}
	}

	fclose(file);
	return(true);
}


void CfgFile::UpdateOrCreateEntry( const char *inszSection, const char *inszKey, const char *inszValue )
{
	Section *sec = FindSection( inszSection );				assert(sec);

	for(std::list<Entry>::iterator it = sec->entries.begin(); it != sec->entries.end(); ++it)
	{
		if(stricmp(it->key.c_str(),inszKey) == 0)				// Key found
			if(!it->IsComment())									// update key
			{
				if(it->value==inszValue)
					return;

				it->value=inszValue;
				m_modified=true;
				return;
			}
	}

	// Create new key
	Entry entry;
	entry.key = inszKey;
	entry.value = inszValue;

	sec->entries.push_back(entry);
	m_modified=true;
}

void CfgFile::RemoveEntry( const char* inszSection, const char* inszKey )
{
	Section* sec = FindSection(inszSection);
	if (!sec)
		return;

	for (std::list<Entry>::iterator it = sec->entries.begin(); it != sec->entries.end(); ++it)
	{
		if (stricmp(it->key.c_str(), inszKey) == 0)
		{
			if (!it->IsComment())
			{
				sec->entries.erase(it);
				return;
			}
		}
	}
}

void CfgFile::LoadBuf( const char *buf,size_t bufSize )
{

	// Read entries from config string buffer.
	Section *curr_section = &m_sections.front();	// Empty section.

	const char *s = buf;
	size_t size = bufSize;

	string ss = s;

	char str[4096];
	size_t i = 0;

	while (i < size)
	{
//		while (j < size && s[j] != '\n') j++;

		sscanf( &s[i],"%[^\n]s",str );

		int iLen = strlen(str);

		i+=iLen;
		if(s[i]==10)i++;

		// remove return at end of string
		if(iLen>0 && str[iLen-1]==0xd)
			str[iLen-1]=0;
		
		RCLog("Parsing line: \"%s\"", str);

		bool bComment=false;

		// Is comment?
		Entry entry;
		entry.key = "";
		entry.value = str;

		bComment=entry.IsComment();

		if (bComment)
			RCLog("It's a comment");

		// Analyze entry string, split on key and value and store in lists.
		string entrystr = str;

		if(!bComment)
			entrystr.Trim();

		if(bComment || entrystr.empty())
		{
			RCLog("Empty!");
			// Add this comment to current section.
			curr_section->entries.push_back( entry );
			continue;
		}


		{
			int splitter = int(entrystr.find( '=' ));
			RCLog("found splitter at %d", splitter);

			if (splitter > 0) 
			{
				// Key found.
				entry.key = entrystr.Mid( 0,splitter );	// Before spliter is key name.
				RCLog("Key: %s", entry.key.c_str());
				entry.value = entrystr.Mid( splitter+1 );	// Everything after splittes is value string.
				RCLog("Value: %s", entry.value.c_str());
				entry.key.Trim();
				entry.value.Trim();
				RCLog("Key: %s, Value: %s", entry.key.c_str(), entry.value.c_str());

				// Add this entry to current section.
				curr_section->entries.push_back( entry );
			}
			else 
			{
				// If not key then probably section string.
				if (entrystr[0] == '[' && entrystr[entrystr.size()-1] == ']')
				{
					// World in bracets is section name.
					Section section;
					section.name = entrystr.Mid( 1,entrystr.size()-2 ); // Remove bracets.
					RCLog("Section! name: %s", section.name.c_str());
					m_sections.push_back( section );
					// Set current section.
					curr_section = &m_sections.back();
				} 
				else
				{
					// Just IsEmpty key value.
					entry.key = "";
					entry.value = entrystr;

					curr_section->entries.push_back( entry );
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CfgFile::Section* CfgFile::FindSection( const string &section )
{
	return &m_sections[Find(section.c_str())];
}

//////////////////////////////////////////////////////////////////////////
bool CfgFile::SetConfig( const EConfigPriority ePri, const char *section, IConfigSink *config )
{
	Section *sec = FindSection( section );					assert(sec);

	for (std::list<Entry>::iterator it = sec->entries.begin(); it != sec->entries.end(); ++it)
	{
		if(!(*it).IsComment())
		{
			const char *szKey = (*it).key.c_str();
			const char *szValue = (*it).value.c_str();

			config->Set(ePri,szKey,szValue);
//			printf("%s=%s\n",(*it).key.GetString(),(*it).value.GetString());
		}
	}
	return true;
}
