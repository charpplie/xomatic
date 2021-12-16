/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
StringUtils.h

TODO: Move contents of this file into CryEngine/CryCommon/StringUtils.h
and contents of StringUtils.cpp into CryEngine/CryCommon/StringUtils.cpp
*************************************************************************/

#ifndef __STRING_UTILS_H__
#define __STRING_UTILS_H__

//--------------------------------------------------------------------------------
// cry_strncpy: Parameter order is the same as strncpy; return value
// states whether entirety of source was copied into destination.
bool cry_strncpy(char * destination, const char * source, size_t bufferLength);

//--------------------------------------------------------------------------------
// cry_copyStringUntilFindChar: Parameter order is the same as strncpy;
// additional 'until' parameter defines which additional character should
// stop the copying. Return value is number of bytes (including NULL)
// written into 'destination', or 0 if 'until' character not found in
// first 'bufferLength' bytes of 'source'.
size_t cry_copyStringUntilFindChar(char * destination, const char * source, size_t bufferLength, char until);

#if !defined(_DEBUG)
#define cry_displayMemInHexAndAscii(...)			(void)(0)
#else

class ITextOutputHandler
{
public:
	virtual void DoOutput(const char * text) = 0;
};

class CCryWatchOutputHandler : public ITextOutputHandler
{
	virtual void DoOutput(const char * text);
};

class CCryLogOutputHandler : public ITextOutputHandler
{
	virtual void DoOutput(const char * text);
};

class CCryLogAlwaysOutputHandler : public ITextOutputHandler
{
	virtual void DoOutput(const char * text);
};

//--------------------------------------------------------------------------------
// cry_displayMemInHexAndAscii outputs (using an ITextOutputHandler subclass) the
// contents of the first 'size' bytes starting at memory location 'data'.
void cry_displayMemInHexAndAscii(const char * startEachLineWith, const void * data, int size, ITextOutputHandler & output, const int bytesPerLine = 32);
#endif

//--------------------------------------------------------------------------------
// Convert a wstring into a standard string
void WStrTostr(const wstring& str, string& dstr);

//--------------------------------------------------------------------------------
// Convert a standard string into a wstring, useful.
void StrToWstr(const char* str, wstring& dstr);

//--------------------------------------------------------------------------------
// Generates a string in the format X days X hrs X mins X secs.
const char * GetTimeString(float secs);

#endif // __STRING_UTILS_H__