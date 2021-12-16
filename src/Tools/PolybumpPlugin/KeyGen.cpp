#include "stdafx.h"
#include <stdio.h>



#include "KeyGen.h"										// CCrytekKeyGen::

//
// allowed characters: 0-9 = 10 characters = 3.32 bits
// allowed characters: A-Z (26) 0-9 (10) SPACE 1(1) MINUS (1) = 38 characters = 5.2 bits = 6 characters per 32bit
// '' == '-'

// other companies: 
//
// 3ds max:						700-50557194 -1247944486  = 69 bit
// visual:						55603-652-0000007-18940		= 66 bit
// photoshop:					PWW600R7105467						= 72 bit
// corel draw:				D10NR-3284253T76					= 78 bit
// word:							50106-335-2828951-02854		= 66 bit


// 37 characters
char CCrytekKeyGen::ConvNumberToCharacter( int incChar )
{
	char cRet='-';

	if(incChar<gLetterCount)cRet='A'+incChar;
	 else
	if(incChar<gLetterCount+gNumberCount)cRet='0'+incChar-gLetterCount;

	return(cRet);
}




// 37 characters
int CCrytekKeyGen::ConvCharacterToNumber( unsigned char incChar )
{
	int iRet=-1;

	if(incChar>='a' && incChar<='z')iRet=(incChar-'a');
	if(incChar>='A' && incChar<='Z')iRet=(incChar-'A');

	if(incChar>='0' && incChar<='9')iRet=gLetterCount + (incChar-'0');

	if(incChar=='-')iRet=gLetterCount+gNumberCount;

	return(iRet);
}



bool CCrytekKeyGen::CalculateCheckChar( char *inpBuffer, char &outcCheck )
{
	uint32 ret=0x35673412;

	if(strlen(inpBuffer)<=8)return(false);

	// first character are the check
	for(uint32 i=0;i<strlen(inpBuffer);)
	{
		if(ConvCharacterToNumber(*inpBuffer)!=-1)
		{
			ret=(ret<<2)^ret^i^(i<<6)^(uint32)(*inpBuffer);
			i++;
		}

		inpBuffer++;
	}

	unsigned char no=(unsigned char)(ret % gCharacterCount);

	outcCheck = ConvNumberToCharacter(no);

	return(true);
}

int GetFirstValidCharNo( const char *inpText )
{
	for(int i=0;inpText[i]!=0;i++)
		if(CCrytekKeyGen::ConvCharacterToNumber(inpText[i])!=-1)return(i);

	return(-1);		// no char found
}

// 
bool CCrytekKeyGen::SimpleCheck( char *inszSerial )
{
#ifndef USE_COPYPROTECTION
		return true;
#else
	int iFirstValid=GetFirstValidCharNo(inszSerial);
	char cCheck;

	if(iFirstValid==-1)return(false);

	if(!CalculateCheckChar(&(inszSerial[iFirstValid+1]),cCheck))
		return(false);

	return(cCheck==inszSerial[iFirstValid]);
#endif
}




bool CCrytekKeyGen::ConvStringToNumber( char *inszText, unsigned char *outpBuffer, int iniBufferMaxSize, int &outiBufferSize )
{
	char *pPtr=inszText;

	outiBufferSize=0;

	while(*pPtr)
	{
		int iNum=ConvCharacterToNumber((unsigned char)*pPtr);

		if(iNum!=-1)
		{
			if(outiBufferSize>=iniBufferMaxSize)return(false);			// string too long

			outpBuffer[outiBufferSize++]=iNum;
		}

		pPtr++;
	}

	return(true);
}


bool CCrytekKeyGen::ConvNumberToString( char *outszText, unsigned char *inpBuffer, int iniBufferSize, int iniMaxTextLength )
{
	int iCur=0;

	for(int i=0;i<iniBufferSize;i++)
	{
		char cChar=ConvNumberToCharacter(inpBuffer[i]);

		if(iCur>=iniMaxTextLength)return(false);			// string too long
		outszText[iCur++]=cChar;
	}

	outszText[iCur]=0;																// null termination
	return(true);
}








void CCrytekKeyGen::GenerateHashNumber( const unsigned char *inpBuffer, int &iniBufferSize, unsigned char outCrc[2] )
{
	uint32 ret=0x35673412;

	outCrc[0]=0;outCrc[1]=0;

	if(iniBufferSize<=2)return;
	inpBuffer+=2;

	// first two characters are the crc
	for(int i=2;i<iniBufferSize;i++)
	{
		ret=(ret<<2)^ret^i^(i<<6)^(uint32)(*inpBuffer);

		inpBuffer++;
	}

	outCrc[0] = (unsigned char)(ret % gCharacterCount);
	outCrc[1] = (unsigned char)((ret / gCharacterCount) % gCharacterCount);
}


void CCrytekKeyGen::GenerateHashCRCString( const unsigned char *inpBuffer, unsigned char outCrc[4] )
{
	uint32 ret=0x35673412;

	outCrc[0]=0;outCrc[1]=0;outCrc[2]=0;outCrc[3]=0;

	int iBufferSize=(int)strlen((char *)inpBuffer);

	// first two characters are the crc
	for(int i=0;i<iBufferSize;i++)
	{
		ret=(ret<<2)^ret^i^(i<<6)^(uint32)(*inpBuffer);

		inpBuffer++;
	}

	outCrc[0] = ConvNumberToCharacter((unsigned char)(ret % gCharacterCount));
	outCrc[1] = ConvNumberToCharacter((unsigned char)((ret / gCharacterCount) % gCharacterCount));
	outCrc[2] = ConvNumberToCharacter((unsigned char)((ret / (gCharacterCount*gCharacterCount)) % gCharacterCount));
	outCrc[3] = ConvNumberToCharacter((unsigned char)((ret / (gCharacterCount*gCharacterCount*gCharacterCount)) % gCharacterCount));

	// to prevent occurances of '-'
	if(outCrc[0]=='-')outCrc[0]='1';
	if(outCrc[1]=='-')outCrc[0]='2';
	if(outCrc[2]=='-')outCrc[0]='3';
	if(outCrc[3]=='-')outCrc[0]='4';
}


// use this for futher checks in the application
bool CCrytekKeyGen::FurtherCheck( char *inszSerial, char *inszAppName, uint32 &outdwMonth, 
									 uint32 &outdwDay, uint32 &outdwYear, unsigned char outHardwareID[4] )
{
#ifndef USE_COPYPROTECTION
		return true;
#else
	const int iMaxBuffer=256;
	unsigned char aBuffer[256];
	char szBuffer[1024],*ptr=szBuffer;
	int iLength;

	if(!SimpleCheck(inszSerial))
		return false;

	int iFirstChar=GetFirstValidCharNo(inszSerial);

	if(ConvStringToNumber(inszSerial+iFirstChar+1,aBuffer,iMaxBuffer,iLength))
	{
		DeCrypt(aBuffer,iLength);

		unsigned char cTestCRC[2];

		if(ConvNumberToString(szBuffer,aBuffer,iLength,1024))
		{
			GenerateHashNumber(aBuffer,iLength,cTestCRC);

			if(cTestCRC[0]==aBuffer[0] && cTestCRC[1]==aBuffer[1])								// CRC is ok
			{
				ptr+=2;
				if(ptr[0]==0)return false;
				if(ptr[1]==0)return false;
				if(ptr[2]==0)return false;

				UncompressDate(ptr,outdwMonth,outdwDay,outdwYear);									// 3 characters date
				ptr+=3;

				if(strncmp(inszAppName,ptr,strlen(inszAppName))!=0)									// app name ok?
					return(false);

				ptr+=strlen(inszAppName)+1;																					// over app name and '-'

				if(ptr[0]==0)return false;
				if(ptr[1]==0)return false;
				if(ptr[2]==0)return false;
				if(ptr[3]==0)return false;

				outHardwareID[0]=ptr[0];
				outHardwareID[1]=ptr[1];
				outHardwareID[2]=ptr[2];
				outHardwareID[3]=ptr[3];

				ptr+=5;																															// over hardware id and '-'

				return true;
			}
		}
	}

	return false;
#endif
}





bool CCrytekKeyGen::CompressDate( char *inszDate, char outszDate[4] )
{	
	int m,d,y;

	if(sscanf(inszDate,"%d/%d/%d",&m,&d,&y)!=3)
		return(false);

	outszDate[0]=ConvNumberToCharacter(m);			// max 37
	outszDate[1]=ConvNumberToCharacter(d);			// max 37
	outszDate[2]=ConvNumberToCharacter(y);			// max 37
	outszDate[3]=0;
	return(true);
}


void CCrytekKeyGen::UncompressDate( char *inszDate, uint32 &outdwMonth, uint32 &outdwDay, uint32 &outdwYear )
{	
	outdwMonth=ConvCharacterToNumber(inszDate[0]);
	outdwDay=ConvCharacterToNumber(inszDate[1]);
	outdwYear=ConvCharacterToNumber(inszDate[2]);
}




void CCrytekKeyGen::Crypt( unsigned char *inoutpBuffer, int &iniBufferSize )
{
	unsigned char cCRC=*inoutpBuffer;

	srand(16572+iniBufferSize+(int)cCRC);													// crytek magic number

	// first two characters are the crc
	for(int i=2;i<iniBufferSize;i++)
	{
			int iAdd=(rand()+(int)cCRC)%gCharacterCount;

			inoutpBuffer[i] = ((int)inoutpBuffer[i] + iAdd) % gCharacterCount;
	}
}



void CCrytekKeyGen::DeCrypt( unsigned char *inoutpBuffer, int &iniBufferSize )
{
	unsigned char cCRC=*inoutpBuffer;

	srand(16572+iniBufferSize+(int)cCRC);													// crytek magic number

	// first two characters are the crc
	for(int i=2;i<iniBufferSize;i++)
	{
			int iAdd=(rand()+(int)cCRC)%gCharacterCount;

			inoutpBuffer[i] = ((int)inoutpBuffer[i] + (gCharacterCount-iAdd)) % gCharacterCount;
	}
}