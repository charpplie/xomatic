/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------
$Id$
$DateTime$

-------------------------------------------------------------------------
History:
- 17:02:2009  Extracted from Launcher/Main.cpp by Alex McCarthy

*************************************************************************/

#ifndef __PARSEENGINECONFIG_H__
#define __PARSEENGINECONFIG_H__

// This is derived from the code found in SystemCFG.cpp
namespace ParseEngineConfig
{
	static const string ENGINE_CFG_FILE = "system.cfg";

	static const string CONFIG_KEY_FOR_GAMEDLL = "sys_dll_game";
	static const string DEFAULT_GAMEDLL_FILENAME = "CryGame.dll";


	/////////////////////////////////////////////////////////////////////////////////
	// replace '\\\\' with '\\' and '\\\"' with '\"'
	void ReplaceSlashes(string &source)
	{
		string result;
		result.reserve(source.length()+1);
		for (string::const_iterator iter = source.begin(); iter != source.end(); ++iter)
		{
			if (*iter == '\\')
			{
				++iter;
				if (iter == source.end())
					;
				else if (*iter == '\\')
					result	+= '\\';
				else if (*iter == '\"')
					result += '\"';
			}
			else
				result += *iter;
		}

		source.swap( result );
	}

	/////////////////////////////////////////////////////////////////////////////////
	string GetGameDllFilename(const char* const szConfigFilename = ENGINE_CFG_FILE)
	{
		string gameDllFilename = DEFAULT_GAMEDLL_FILENAME;
		FILE *file = fopen( szConfigFilename,"rb" );
		if (!file)
			return gameDllFilename;

		fseek(file,0,SEEK_END);
		int nLen = ftell(file);
		fseek(file,0,SEEK_SET);

		char *sAllText = new char [nLen + 16];

		fread( sAllText,1,nLen,file );

		sAllText[nLen] = '\0';
		sAllText[nLen+1] = '\0';

		char *strLast = sAllText+nLen;
		char *str = sAllText;
		while (str < strLast)
		{
			char *s = str;
			while (str < strLast && *str != '\n' && *str != '\r')
				str++;
			*str = '\0';
			str++;
			while (str < strLast && (*str == '\n' || *str == '\r'))
				str++;


			string strLine = s;

			// skip comments
			if (0<strLine.find( "--" ))
			{
				// extract key
				string::size_type posEq( strLine.find( "=", 0 ) );
				if (string::npos!=posEq)
				{
					string strKey( strLine, 0, posEq );
					strKey.Trim();

					//				if (!strKey.empty())
					{
						// extract value
						string::size_type posValueStart( strLine.find( "\"", posEq + 1 ) + 1 );
						// string::size_type posValueEnd( strLine.find( "\"", posValueStart ) );
						string::size_type posValueEnd( strLine.rfind( '\"' ) );

						string strValue;

						if( string::npos != posValueStart && string::npos != posValueEnd )
							strValue=string( strLine, posValueStart, posValueEnd - posValueStart );
						else
						{
							string strTmp( strLine, posEq + 1, strLine.size()-(posEq + 1) );
							strTmp.Trim();
							strValue = strTmp;
						}

						ReplaceSlashes(strValue);

						//						m_pSystem->GetILog()->Log("Setting %s to %s",strKey.c_str(),strValue.c_str());

						if (strKey.compareNoCase(CONFIG_KEY_FOR_GAMEDLL) == 0)
						{
							gameDllFilename = strValue;
							break;
						}
					}
				}
			} //--
		}
		delete []sAllText;
		fclose(file);

		return gameDllFilename;
	}
};

#endif //__PARSEENGINECONFIG_H__
