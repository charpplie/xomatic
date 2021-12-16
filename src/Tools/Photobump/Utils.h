

#ifndef CUTILS_H
#define CUTILS_H

#pragma once

#define FPS_FRAMES 4

//////////////////////////////////////////////////////////////////////////
class CUtils
{
public:
	CUtils();
	~CUtils();

	void	InitTimer();
	void	UpdateTimer(bool bCalcDeltaFrame);
	ftype	GetDeltaFrameTime() { return(m_fDeltaTime); }
	ftype GetTimeSysMs()			{ return(m_fCurrTime); }
	int		GetFrameNumber()		{ return(m_nFrameId); }

	char	*LoadInMemory(const char *szFilename,unsigned int &nSize,int nZeroEnd=0);
	const char *GetToken(const char *&szIn);

	//filename utils
	//gets the filename without path
	const char *GetFilename(const char *szSrc);
	//gets the path	
	const char *GetPath(const char *szFilename);
	//gets filename's extension	
	char *GetExtension(const char *filename);
	//removes extension from filename	
	void RemoveExtension(char *szPath);
	//replace filename extension	
	void ReplaceExtension(char *szPath, const char *szExt);

	std::string FormatWinError(DWORD dwError);

private:

	std::string IntToString(int nNumber);
	std::string UIntToHexString(DWORD dwNumber);
	std::string TryFormatWinError(DWORD dwError);	

	//timer
	unsigned int		m_nTimeBase,m_nCurTime,m_nFrameId;

	ftype	m_fDeltaTime,m_fCurrTime,m_fLastTime;	
	ftype	m_fPreviousTimes[FPS_FRAMES];	

	char	*m_szBuffer;
	char	m_szToken[1024];
};

#endif