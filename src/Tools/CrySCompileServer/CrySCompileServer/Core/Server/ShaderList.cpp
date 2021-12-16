#include "ShaderList.hpp"

#include <assert.h>
#ifdef _MSC_VER
#include <process.h>
#include <direct.h>
#endif
#ifdef UNIX
#include <pthread.h>
#endif

static bool g_bSaveThread = false;

CShaderList& CShaderList::Instance()
{
	static CShaderList g_Cache;
	return g_Cache;
}

//////////////////////////////////////////////////////////////////////////
CShaderList::CShaderList()
{
	m_lastTime = 0;
}

//////////////////////////////////////////////////////////////////////////
void CShaderList::Tick()
{
	DWORD t = GetTickCount();
	if (t < m_lastTime || (t - m_lastTime) > 1000*60)
	{
		m_lastTime = t;

		CShaderList::Instance().m_PC.MergeNewLinesAndSave();
		CShaderList::Instance().m_X360.MergeNewLinesAndSave();
		CShaderList::Instance().m_PS3.MergeNewLinesAndSave();
	}
}

//////////////////////////////////////////////////////////////////////////
CShaderListFile::CShaderListFile()
{
	m_bModified = false;

	// some test cases
	assert(CheckSyntax("<1>watervolume@WaterVolumeOutofPS()()(0)(0)(0)(ps_2_0)")==true);
	assert(CheckSyntax("<1>Blurcloak@BlurCloakPS(%BUMP_MAP)(%_RT_FOG|%_RT_HDR_MODE|%_RT_BUMP)(0)(0)(1)(ps_2_0)")==true);
	assert(CheckSyntax("<1>Burninglayer@BurnPS()(%_RT_ADDBLEND|%_RT_)HDR_MODE|%_RT_BUMP|%_RT_3DC)(0)(0)(0)(ps_2_0)")==false);
	assert(CheckSyntax("<1>Illum@IlluminationVS(%DIFFUSE|%SPECULAR|%BUMP_MAP|%VERTCOLORS|%STAT_BRANCHING)(%_RT_RAE_GEOMTERM)(101)(0)(0)(vs_2_0)")==true);
}

//////////////////////////////////////////////////////////////////////////
bool CShaderListFile::Reload()
{
	return Load( m_filename.c_str() );
}

//////////////////////////////////////////////////////////////////////////
bool CShaderListFile::Load( const char *filename )
{
	printf( "Loading ShaderList file: %s\n",filename );
	m_filename = filename;
	FILE *f = fopen( filename,"rt" );
	if (!f)
		return false;

	int nNumLines = 0;
	m_entries.clear();
	char str[65535];
	while (fgets(str,sizeof(str),f) != NULL)
	{
		if(*str && InsertLineInternal(str))
			++nNumLines;
	}
	fclose(f);
	if (nNumLines == m_entries.size())
		m_bModified = false;
	else
		m_bModified = true;

	printf( "Loaded %d combination for %s\n",nNumLines,filename );

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CShaderListFile::Save()
{
	CCrySimpleMutexAutoLock Lock(m_Mutex);

	if (m_filename.empty())
		return false;

	FILE *f = fopen( m_filename.c_str(),"wt" );
	for (Entries::iterator it = m_entries.begin(); it != m_entries.end(); ++it)
	{
		const char *str = it->c_str();
		fprintf( f,"%s\n", str );
	}
	fclose(f);
	m_bModified = false;
	return true;
}

//////////////////////////////////////////////////////////////////////////
inline bool IsHexNumberCharacter( const char c )
{
	return (c>='0' && c<='9') || (c>='a' && c<='f') || (c>='A' && c<='F');
}

//////////////////////////////////////////////////////////////////////////
inline bool IsDecNumberCharacter( const char c )
{
	return (c>='0' && c<='9') || (c>='a' && c<='f') || (c>='A' && c<='F');
}

//////////////////////////////////////////////////////////////////////////
inline bool IsNameCharacter( const char c )
{
	return (c>='a' && c<='z') || (c>='0' && c<='9') || (c>='A' && c<='Z') || c=='@' || c=='/' || c=='%' || c=='_';
}

int shGetHex(const char *buf)
{
	if (!buf)
		return 0;
	int i = 0;

	sscanf(buf, "%x", &i);

	return i;
}

//////////////////////////////////////////////////////////////////////////
bool CShaderListFile::CheckSyntax( const char *szLine,const char **sOutStr )
{
	assert(szLine);

	char g_szLine[2048];

	char *t = g_szLine;

	if (sOutStr)
		*sOutStr = 0;

	// e.g. Blurcloak@BlurCloakPS(%BUMP_MAP|%SPECULAR)(%_RT_FOG|%_RT_HDR_MODE|%_RT_BUMP)(0)(0)(0)(ps_2_0)

	const char *p=szLine;

	if (strlen(szLine) < 4)
		return false;

	if (szLine[0] != '<' || !IsDecNumberCharacter(szLine[1]) || szLine[2] != '>')
		return false;

	*t++ = *p++; // Copy <
	*t++ = *p++; // Copy version
	*t++ = *p++; // Copy >

	// e.g. "Blurcloak@BlurCloakPS"
	while(IsNameCharacter(*p)) *t++ = *p++;

	// e.g. "(%BUMP_MAP|%SPECULAR)(%_RT_FOG|%_RT_HDR_MODE|%_RT_BUMP)"
	for(int i=0;i<2;++i)
	{
		if(*p!='(')	return false;					*t++ = *p++;
		while(true)
		{
			while(IsNameCharacter(*p))			*t++ = *p++;
			if(*p!='|')
				break;
			*t++ = *p++;
		}
		if(*p!=')')	return false;					*t++ = *p++;
	}

	// e.g. "(0)(0)(0)"
	for(int i=0;i<3;++i)
	{
		if(*p!='(')	return false;				*t++ = *p++;
		if (i == 0)
		{
			int n = shGetHex(p);
			while(IsHexNumberCharacter(*p))	p++;
			if (n)
				*t++ = '1';
			else
				*t++ = '0';
		}
		else
		{
			while(IsHexNumberCharacter(*p))	*t++ = *p++;
		}
		if(*p!=')')	return false;				*t++ = *p++;
	}

	// e.g. "(ps_2_0)"
	if(*p!='(')	return false;					*t++ = *p++;
	while(IsNameCharacter(*p))				*t++ = *p++;
	if(*p!=')')	return false;					*t++ = *p++;

	// Copy rest of the line.
	while (*p)
	{
		if (*p != '\n' && *p != '\r')
			*t++ = *p++;
		else
			p++;
	}
	*t++ = '\0'; // end of line.

	if (sOutStr)
		*sOutStr = szLine;

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CShaderListFile::InsertLine( const char *szLine )
{
	if (*szLine != 0)
	{
		CCrySimpleMutexAutoLock Lock(m_Mutex);
		m_newLines.push_back( szLine );
		m_bModified = true;
	}
}

//////////////////////////////////////////////////////////////////////////
bool CShaderListFile::InsertLineInternal( const char *szLine )
{
	const char *szCorrectedLine = 0;
	if(CheckSyntax(szLine,&szCorrectedLine))
	{
		// Trim \n\r
		char *s = const_cast<char*>(szCorrectedLine);
		for (size_t p = strlen(s)-1; p > 0; p--)
		{
			if (s[p] == '\n' || s[p] == '\r')
				s[p] = '\0';
			else
				break;
		}

		if (szCorrectedLine)
		{
			std::pair<Entries::iterator,bool> result = 	m_entries.insert(szCorrectedLine);
			if (result.second)
				m_bModified = true;
		}
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
void CShaderListFile::MergeNewLines()
{
	std::vector<std::string> newLines;

	{
		CCrySimpleMutexAutoLock Lock(m_Mutex);
		newLines.swap( m_newLines );
	}

	if (newLines.empty())
		return;

	m_bModified = false;
	for (std::vector<std::string>::iterator it = newLines.begin(); it != newLines.end(); ++it)
	{
		InsertLineInternal((*it).c_str());
	}
}

//////////////////////////////////////////////////////////////////////////
void CShaderListFile::MergeNewLinesAndSave()
{
	if (m_bModified)
		MergeNewLines();
	if (m_bModified)
		Save();
}
