

#include "stdafx.h"
#include "MMSystem.h"
#pragma comment(lib, "Winmm.lib")

//////////////////////////////////////////////////////////////////////////
CUtils::CUtils()
{
	m_szBuffer=new char [2048];
}

//////////////////////////////////////////////////////////////////////////
CUtils::~CUtils()
{
	SAFE_DELETE_ARRAY(m_szBuffer);
}

#define SMOOTH_FRAMES	0

//////////////////////////////////////////////////////////////////////////
void CUtils::InitTimer()
{
	//initialize timer
	// let base retain 16 bits of effectively random data
	m_nTimeBase = timeGetTime() & 0xffff0000;	
	m_nFrameId=0;	
	m_fDeltaTime=0;	
	for (int k=0;k<FPS_FRAMES;k++) 
    m_fPreviousTimes[k]=0;
}

//////////////////////////////////////////////////////////////////////////
void CUtils::UpdateTimer(bool bCalcDeltaFrame)
{
	if (bCalcDeltaFrame)
	{	
		m_fCurrTime=(timeGetTime()-m_nTimeBase)/1000.0;

		m_fDeltaTime=m_fCurrTime-m_fLastTime;

		if (m_fDeltaTime<0.000000000001)
			m_fDeltaTime=0.000000000001;
		
	#ifdef SMOOTH_FRAMES
		m_fPreviousTimes[(m_nFrameId) % FPS_FRAMES]=(ftype)(m_fDeltaTime);

		if (m_nFrameId>FPS_FRAMES) 
		{
			// average multiple frames together to smooth changes out a bit
			ftype fTotal=0;
			for (int i=0;i<FPS_FRAMES;i++) 
						fTotal+=m_fPreviousTimes[i];	
		    
			if (!fTotal) 
				fTotal=1;

			m_fDeltaTime=fTotal/(ftype)(FPS_FRAMES);
		}
	#endif
		
		m_fLastTime=m_fCurrTime;		
	}	

	m_nFrameId++;
}

//////////////////////////////////////////////////////////////////////////
//gets the filename without path
const char *CUtils::GetFilename(const char *szSrc)
{
	//find the filename
	int nLen=(int)strlen(szSrc);
	//go at the end of the filename 
	const char *szPtr=szSrc+nLen-1;
		
	//move back till the first '/' or '\\'
	while (*szPtr && szPtr>szSrc)
	{
		if ((*szPtr=='/') || (*szPtr=='\\'))
		{
			//if character found, increment the 
			//string pointer to point to the correct position 
			//(first character after the '/') and stop the loop
			szPtr++; 
			break;
		}

		szPtr--;
	}		

	return (szPtr);
}

//gets the path
/////////////////////////////////////////////////////////////////////////////
const char *CUtils::GetPath(const char *szFilename)
{	
	//extract the path		
	strcpy(m_szBuffer,szFilename);

	char *szSrc = m_szBuffer+strlen(m_szBuffer)-1;
	while (*szSrc && szSrc>m_szBuffer)
	{
		if (*szSrc=='\\' || *szSrc=='/')
		{ 
      szSrc++;
			*szSrc=0; //terminate the string here
			break;
		}
		szSrc--;
	}  

	return (m_szBuffer);
}

//gets filename's extension
//////////////////////////////////////////////////////////////////////
char *CUtils::GetExtension(const char *filename)
{
	char *src = (char *)filename+strlen(filename)-1;
	while (*src && src>=filename)
	{
		if (*src == '.')
		{ 			
			return (++src);
		}
		src--;
	}

	return (NULL);
}

//removes extension from filename
//////////////////////////////////////////////////////////////////////
void CUtils::RemoveExtension(char *szPath)
{
	char *szSrc=szPath+strlen(szPath)-1;
	while (*szSrc && szSrc>szPath && *szSrc!='/')
	{
		if (*szSrc=='.')
		{ 
			*szSrc=0; // remove extension 
			return;  
		}
		szSrc--;
	}
}

//replace filename extension
//////////////////////////////////////////////////////////////////////
void CUtils::ReplaceExtension(char *szPath, const char *szExt)
{
  RemoveExtension(szPath);
  strcat(szPath,".");
  strcat(szPath,szExt);
}

//////////////////////////////////////////////////////////////////////
char *CUtils::LoadInMemory(const char *szFilename,unsigned int &nSize,int nZeroEnd)
{
	nSize=0;

	//try to open the file
	FILE *fpSource=fopen(szFilename, "rb");

	if (!fpSource)
	{
		//if (m_pLog)
		//	m_pLog->Log("<SYSTEM> Couldn't open %s \n",szFilename);		
		return (NULL);
	}

	//calc filesize

	//current position
	int nPos = ftell(fpSource);

	//move till the end
	fseek(fpSource, 0, SEEK_END);
	int nEnd = ftell(fpSource);
	
	//put it back
	fseek(fpSource, nPos, SEEK_SET);
	nSize=nEnd-nPos;

	//read the rest of the file in memeory	
	char *szBuffer = new char[nSize+nZeroEnd]; 
	fread(szBuffer,1,nSize,fpSource);

	if (nZeroEnd)
	{
		szBuffer[nSize]=0;
	}

	//close the file, is no longer needed
	fclose(fpSource);	

	return (szBuffer);
}

// returns the decimal string representation of the given int
//////////////////////////////////////////////////////////////////////////
std::string CUtils::IntToString (int nNumber)
{
	char szNumber[16];
	//	itoa (nNumber, szNumber, 10);
	sprintf (szNumber, "%d", nNumber);
	return szNumber;
}

// returns hexadecimal string representation of the given dword
//////////////////////////////////////////////////////////////////////////
std::string CUtils::UIntToHexString(DWORD dwNumber)
{
	char szNumber[24];
	sprintf (szNumber, "0x%X", dwNumber);
	return szNumber;
}

//////////////////////////////////////////////////////////////////////////
std::string CUtils::TryFormatWinError(DWORD dwError)
{
#ifdef WIN32
	LPVOID lpMsgBuf;   // pointer to the buffer that will accept the formatted message
	//DWORD dwLastError = OsGetLastError(); // the last user error for which the description should be formatted
	DWORD dwFormattedMsgLen = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL );
	
	if (!dwFormattedMsgLen)
		// error. return both the user error and the error received during formatting the user error
		return std::string();
	else
	{// the lpMsgBuf contains allocated by the system call message that is to be returned.
		// we'll copy it into sResult and free it and return sResult
		std::string sResult = (LPCTSTR) lpMsgBuf;
		LocalFree (lpMsgBuf);
		while (!sResult.empty() && ((unsigned char)sResult[sResult.length()-1]) < 0x20)
			sResult.resize(sResult.length()-1);
		return sResult;
	}
#else
	return "Unknown error";
#endif
}

// returns the string representation (in natural language) of the last error retrieved by GetLastError()
//////////////////////////////////////////////////////////////////////////
std::string CUtils::FormatWinError(DWORD dwError)
{
	std::string sResult = TryFormatWinError(dwError);
	
	if (sResult.empty())
		// error. return both the user error and the error received during formatting the user error
		sResult = "Error" + IntToString (GetLastError()) + " while formatting error message";

	return sResult + "\n(" + (dwError & 0x80000000 ? UIntToHexString(dwError):IntToString(dwError)) + ")";
}

//////////////////////////////////////////////////////////////////////////
const char *CUtils::GetToken(const char *&szIn)
{
	if (!szIn)
		return (NULL);
		
	while(1)
	{			
		// skip all crap 
		while (szIn[0] && (*szIn<=' '))
			szIn++;

		if (!szIn[0])
			return (NULL); // the string didnt contain anything useful

		// skip comments
		if ((szIn[0]=='/') && (szIn[1]=='/')) 
		{
			while (*szIn && *szIn!='\r' && *szIn!='\n')
				szIn++; // till the first new line or end 			
			continue; // restart to skip crap from next token
		}

		// skip block comments
		if ((szIn[0]=='/') && (szIn[1]=='*')) 
		{
			szIn+=2; //skip these 2
			while ((*szIn) && (!((szIn[0]=='*') && (szIn[1]=='/')))) 			
				szIn++;
			
			if (!szIn[0]) 
				return(NULL); // nothign found

			// found */, skip these 2
			szIn+=2; 
			continue; // restart to skip crap from next token
		}			

		const char *szStart=szIn;	
		// get the token, if szIn[0] is already 0 it is
		// taken into account here
		while (*szIn && *szIn!='\r' && *szIn!='\n' && *szIn!='\t' && *szIn!=' ')
			szIn++; // till the first new line or end 			
		
		if (szIn-szStart==0) 
			return(NULL);

		int nLen=(int)(szIn-szStart);
		memcpy(m_szToken,szStart,nLen);		
		m_szToken[nLen]=0;
		break;
	} 
	
	return (m_szToken);
}
