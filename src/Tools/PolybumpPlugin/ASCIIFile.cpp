#include "stdafx.h"														// for precompiled headers (has to be in first place)
#include <stdlib.h>
#include <malloc.h>														// malloc,free
#include <stdio.h>														// FILE
#include <fcntl.h>														// filesize
#include <io.h>																// filesize
#include <string.h>														// strlen

#include "ASCIIFile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// constructor
CASCIIFile::CASCIIFile()
{
	m_Size=0;
	m_Data=0;
}

// destructor
CASCIIFile::~CASCIIFile()
{
	free(m_Data);
}


// copy constructor
CASCIIFile::CASCIIFile( CASCIIFile &a )
{
	char *mem=(char *)malloc(a.m_Size);

	if(mem)
	{
		m_Data=mem;
		m_Size=a.m_Size;
	}
	else throw "low memory";
}


// assignment operator
const CASCIIFile CASCIIFile::operator=( const CASCIIFile &a )
{
	char *mem=(char *)malloc(a.m_Size);

	if(mem)
	{
		ReleaseData();
		m_Data=mem;
		m_Size=a.m_Size;
	}
	else throw "low memory";

	return(*this);
}





void CASCIIFile::ReleaseData( void )
{
	free(m_Data);m_Data=0;
	m_Size=0;
}

bool CASCIIFile::IO_GetAvailability( const char *pathname )
{
	int handle;

	handle=open(pathname,O_RDONLY);

	if(handle==-1)return(false);

	close(handle);

	return(true);
}



DWORD CASCIIFile::IO_GetFileSize( const char *Name )
{
    int handle,size;

	handle=open(Name,O_RDONLY);

	if(handle==-1)return(0);

    size=filelength(handle);
    close(handle);

	return(size);
}



bool CASCIIFile::IO_SaveASCIIFile( const char *pathname )	 
{
	if(m_Data==0)return(false);
	
	FILE *datei;

    if((datei=fopen(pathname,"wb"))==NULL){ return(false); }
    else
    {
        fwrite(m_Data,m_Size-1,1,datei);
        fclose(datei);
		return(true);
    }
	return(false);
}


bool CASCIIFile::IO_LoadASCIIFile( const char *pathname )		// allociert 1+Filesize, liest Daten und 0 terminiert, free nicht vergessen !!
{
	if(*pathname==0)return(false);

	DWORD size=IO_GetFileSize(pathname);
	char *ret=0;

	ret=(char *)malloc(size+1);

	if(ret==0)return(false);	// no Memory
	
	FILE *datei;

    if((datei=fopen(pathname,"rb"))==NULL){ free(ret);return(false); }
    else
    {
        fread(ret,size,1,datei);
        ret[size]=0;				// 0 terminieren

		m_Size=size+1;
		m_Data=ret;

        fclose(datei);
		return(true);
    }
	return(false);
}

DWORD CASCIIFile::GetDataSize( void )
{
	if(m_Data)return(m_Size-1);
		else return(0);
}

char *CASCIIFile::GetDataPtr( void )						// Speicherposition
{
	return(m_Data);
}

void CASCIIFile::CoverThisData( char *ptr, DWORD size )	// altes freigeben, neues übernehmen, keine Kopie machen
{
	ReleaseData();
	m_Data=ptr;m_Size=size+1;
}

