#ifndef __SHADERLIST__
#define __SHADERLIST__

#include <map>
#include <set>
#include <vector>

#include "../StdTypes.hpp"
#include "../Error.hpp"
#include "../STLHelper.hpp"
#include "CrySimpleMutex.hpp"

class CShaderListFile
{
	//////////////////////////////////////////////////////////////////////////
	bool m_bModified;
	std::string m_filename;
	typedef std::set<std::string> Entries;
	Entries m_entries;
	std::vector<std::string> m_newLines;

	CCrySimpleMutex m_Mutex;

public:
	CShaderListFile();

	bool Load( const char *filename );
	bool Save();
	bool Reload();
	bool IsModified() const { return m_bModified; }

	void InsertLine( const char *szLine );
	void MergeNewLinesAndSave();

private:
	void MergeNewLines();
	// Returns:
	//   true - line was instered, false otherwise
	bool InsertLineInternal( const char *szLine );

	// Returns
	//   true=syntax is ok, false=syntax is wrong
	static bool CheckSyntax( const char *szLine,const char **sOutStr=NULL );
};

class CShaderList
{
	CCrySimpleMutex							m_Mutex;
	unsigned int                m_lastTime;

public:

	// ShaderList file per platform.
	CShaderListFile m_PC;
	CShaderListFile m_X360;
	CShaderListFile m_PS3;

	static CShaderList&			Instance();

	CShaderList();
	void Tick();
};

#endif
