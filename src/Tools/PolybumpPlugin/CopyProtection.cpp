#include "stdafx.h"														// precompiled headers
#include <stdio.h>														// sprintf

#include "CopyProtection.h"										// ChangeSerialYesNo(),GetSerialFromRegistry(),HowToGetSerialDialog()
#include "MAC_Address.h"											// NetbiosHelper_GetCRC()


char g_szSerialNo[256]="";

#define POLYBUMPREGISTRY_KEY     TEXT("Software\\Crytek\\PolyBumpMAXPlugin")






//-----------------------------------------------------------------------------
// Name: ReadRegKey()
// Desc: Read a registry key 
//-----------------------------------------------------------------------------
HRESULT ReadRegKey( HKEY hKey, TCHAR* strName, TCHAR* strValue, 
                    DWORD dwLength, TCHAR* strDefault )
{
	DWORD dwType;
	LONG bResult;

	bResult = RegQueryValueEx( hKey, strName, 0, &dwType, 
							 (LPBYTE) strValue, &dwLength );
	if ( bResult != ERROR_SUCCESS )
		strcpy( strValue, strDefault );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: WriteRegKey()
// Desc: Writes a registry key 
//-----------------------------------------------------------------------------
HRESULT WriteRegKey( HKEY hKey, TCHAR* strName, const TCHAR* strValue )
{
	LONG bResult;

	bResult = RegSetValueEx( hKey, strName, 0, REG_SZ, 
							 (LPBYTE) strValue, (int)strlen(strValue) + 1 );
	if ( bResult != ERROR_SUCCESS )
		return E_FAIL;

    return S_OK;
}


// -> g_SerialNo
void GetSerialFromRegistry( void )
{
	HKEY hAppRegKey=NULL;

	RegCreateKeyEx( HKEY_CURRENT_USER, POLYBUMPREGISTRY_KEY, 0, NULL,
                    REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
                    &hAppRegKey, NULL );

	char ret[256];

  if(ReadRegKey(hAppRegKey,"SerialNo",ret,256,"")==S_OK)
		strcpy(g_szSerialNo,ret);

	CloseHandle( hAppRegKey );
}


void ChangeSerialYesNo( char *inszNewSerial )
{
	HKEY hAppRegKey=NULL;

	if(MessageBox(0,"Are you sure you want to change the serial no?","PolyBump",MB_YESNO)==IDYES)
	{
		strcpy(g_szSerialNo,inszNewSerial);

		RegCreateKeyEx( HKEY_CURRENT_USER, POLYBUMPREGISTRY_KEY, 0, NULL,
										REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
										&hAppRegKey, NULL );

		// Write information to the registry
		WriteRegKey( hAppRegKey, "SerialNo", g_szSerialNo );

		CloseHandle( hAppRegKey );
	}

}

void HowToGetSerialDialog( void )
{
	unsigned char uCRC[4];

	if(NetbiosHelper_GetCRC(uCRC))
	{
		char str[2048];

		sprintf(str,"To get your personal serial number from Crytek\nyou have to send "\
			"an email to PolyBump@Crytek.de with your full name,\n"\
			"address and your Hardware ID string:\n\n      %c%c%c%c.\n\n"\
			"(This string is calculated from your first installed network card.\n"\
			"so make sure it's calculated on the computer\n"\
			"you want to use the tool.)",uCRC[0],uCRC[1],uCRC[2],uCRC[3]);

		MessageBox(0,str,"PolyBumpPlugin",MB_OK);
	}
	else
	{
		MessageBox(0,"To get your personal serial number from Crytek we need a Hardware ID string.\n"\
			"\nThis string is normaly calculated from your first network card, but the operation failed.\n"\
			"You must have a network card installed. Also make sure that Netbios is installed and working.","PolyBumpPlugin",MB_OK);
	}
}
