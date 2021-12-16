#include "Cry_Math.h"									// uint32

const int gLetterCount='z'-'a'+1;
const int gNumberCount='9'-'0'+1;
const int gCharacterCount=gLetterCount+gNumberCount+1;


class CCrytekKeyGen
{
public:
	static char ConvNumberToCharacter( int incChar );
	static int ConvCharacterToNumber( unsigned char incChar );
	static bool CalculateCheckChar( char *inpBuffer, char &outcCheck );
	static bool ConvStringToNumber( char *inszText, unsigned char *outpBuffer, int iniBufferMaxSize, int &outiBufferSize );
	static bool ConvNumberToString( char *outszText, unsigned char *inpBuffer, int iniBufferSize, int iniMaxTextLength );

	//! calculates a hash value from a string (not readable but in character range), (first two characters are the CRC)
	static void GenerateHashNumber( const unsigned char *inpBuffer, int &iniBufferSize, unsigned char outCrc[2] );

	//! calculates a readable hash value from a string, e.g. DK2J (every character is used)
	//! it is garanteed that there is no '-' in the string (only valid characters)
	static void GenerateHashCRCString( const unsigned char *inpBuffer, unsigned char outCrc[4] );

	static bool CompressDate( char *inszDate, char outszDate[4] );
	static void UncompressDate( char *inszDate, uint32 &outdwMonth, uint32 &outdwDay, uint32 &outdwYear );
	static void Crypt( unsigned char *inoutpBuffer, int &iniBufferSize );
	static void DeCrypt( unsigned char *inoutpBuffer, int &iniBufferSize );

	//! use this first (every 37th key is valid) - output should be: key seems to be ok
	static bool SimpleCheck( char *inszSerial );

	//! use this for futher checks in the application
	//!
	//! /param inszAppName e.g. "PB"
	static bool FurtherCheck( char *inszSerial, char *inszAppName, uint32 &outdwMonth, uint32 &outdwDay, uint32 &outdwYear, 
		unsigned char outHardwareID[4] );
};



