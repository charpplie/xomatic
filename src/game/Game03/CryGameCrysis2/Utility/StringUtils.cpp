/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
StringUtils.cpp
*************************************************************************/

#include "StdAfx.h"
#include "StringUtils.h"
#include "CryWatch.h"
#include "HUD/HUD.h"

static bool s_stringUtils_assertEnabled = true;

//--------------------------------------------------------------------------------
bool cry_strncpy(char * destination, const char * source, size_t bufferLength)
{
	bool reply = false;

	CRY_ASSERT(destination);
	CRY_ASSERT(source);

	if (bufferLength)
	{
		size_t i;
		for (i = 0; source[i] && (i + 1) < bufferLength; ++ i)
		{
			destination[i] = source[i];
		}
		destination[i] = '\0';
		reply = (source[i] == '\0');
	}

	CRY_ASSERT_MESSAGE(reply || !s_stringUtils_assertEnabled, string().Format("String '%s' is too big to fit into a buffer of length %u", source, (unsigned int) bufferLength));

	return reply;
}

//--------------------------------------------------------------------------------
size_t cry_copyStringUntilFindChar(char * destination, const char * source, size_t bufferLength, char until)
{
	size_t reply = 0;

	CRY_ASSERT(destination);
	CRY_ASSERT(source);

	if (bufferLength)
	{
		size_t i;
		for (i = 0; source[i] && source[i] != until && (i + 1) < bufferLength; ++ i)
		{
			destination[i] = source[i];
		}
		destination[i] = '\0';
		reply = (source[i] == until) ? (i + 1) : 0;
	}

	return reply;
}

//--------------------------------------------------------------------------------

#ifndef cry_displayMemInHexAndAscii
void cry_displayMemInHexAndAscii(const char * startEachLineWith, const void * data, int size, ITextOutputHandler & output, const int bytesPerLine)
{
	if (size > 0 && data)
	{
		const unsigned char * charData = (const unsigned char *) data;
		string hexLine, asciiLine;
		int bytesOnThisLineSoFar = 0;
		int padTo = 0;

		while (size)
		{
			if (++ bytesOnThisLineSoFar > bytesPerLine)
			{
				output.DoOutput(string().Format("%s%s  %s", startEachLineWith, hexLine.c_str(), asciiLine.c_str()));
				padTo = hexLine.length();
				bytesOnThisLineSoFar -= bytesPerLine;
				asciiLine = "";
				hexLine = "";
			}

			hexLine = hexLine + string().Format("%02x", (int) * charData);

			if ((bytesOnThisLineSoFar & 7) == 0 && (bytesOnThisLineSoFar != bytesPerLine))
			{
				hexLine += " ";
			}

			asciiLine = asciiLine + string().Format("%c", (*charData >= 32 && *charData != 127) ? *charData : '.');

			++ charData;
			-- size;
		}

		output.DoOutput(string().Format("%s%s%s  %s", startEachLineWith, hexLine.c_str(), padTo ? string(' ', padTo - hexLine.length()).c_str() : "", asciiLine.c_str()));
	}
	else
	{
		output.DoOutput(string().Format("%sPTR=%p SIZE=%d", startEachLineWith, size));
	}
}

//---------------------------------------------------------------------
void CCryWatchOutputHandler::DoOutput(const char * text)
{
	CryWatch ("%s", text);
}

//---------------------------------------------------------------------
void CCryLogOutputHandler::DoOutput(const char * text)
{
	CryLog ("%s", text);
}

//---------------------------------------------------------------------
void CCryLogAlwaysOutputHandler::DoOutput(const char * text)
{
	CryLogAlways ("%s", text);
}
#endif

//---------------------------------------------------------------------
void WStrTostr(const wstring& str, string& dstr)
{
	dstr.resize(str.length());
	char* dst = dstr.begin();
	const wchar_t* src = str.c_str();
	while (const char c = (char) (*src++))
	{
		*dst++ = c;
	}
}

//---------------------------------------------------------------------
void StrToWstr(const char* str, wstring& dstr)
{
	dstr.resize(strlen(str));
	wchar_t* dst = dstr.begin();
	const char* src = str;
	while (const wchar_t c=(wchar_t)(*src++))
	{
		*dst++ = c;
	}
}

//---------------------------------------------------------------------
const char * GetTimeString(float secs)
{
	int d, h, m, s;

	CGame::ExpandTimeSeconds(int_round(secs), d, h, m, s);

	static CryFixedStringT<64> result;
	result.clear();

	CHUD* pHUD = g_pGame->GetHUD();
	if (pHUD)
	{
		if (d==1)
			result.Format("%d %s",d, pHUD->LocalizeString("@ui_day"));
		else if (d>1)
			result.Format("%d %s",d, pHUD->LocalizeString("@ui_days"));

		if (h==1)
			result.Format("%s%s%d %s",result.c_str(), result.empty()?"":" ", h, pHUD->LocalizeString("@ui_hr"));
		else if (h>1 || d>0)
			result.Format("%s%s%d %s",result.c_str(), result.empty()?"":" ", h, pHUD->LocalizeString("@ui_hrs"));

		if (m==1)
			result.Format("%s%s%d %s",result.c_str(), result.empty()?"":" ", m, pHUD->LocalizeString("@ui_min"));
		else if (m>1 || h>0 || d>0)
			result.Format("%s%s%d %s",result.c_str(), result.empty()?"":" ", m, pHUD->LocalizeString("@ui_mins"));

		if (s==1)
			result.Format("%s%s%d secs",result.c_str(), result.empty()?"":" ", s, pHUD->LocalizeString("@ui_sec"));
		else
			result.Format("%s%s%d secs",result.c_str(), result.empty()?"":" ", s, pHUD->LocalizeString("@ui_secs"));
	}

	return result.c_str();
}