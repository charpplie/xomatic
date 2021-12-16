#include "stdafx.h"																		// precompiled headers

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
//#include <iostream>
#include <Nb30.h>
#include "KeyGen.h"																		// GenerateHasNumber2()
#include "Mac_Address.h"															// NetBiosProc


#include <string>																			// STL string


// #pragma comment(lib, "Netapi32.lib")

typedef UCHAR (CALLBACK *NetBiosProc)( PNCB ); 






bool NetbiosHelper_GetAdapterInfo( int adapter_num, std::string &mac_addr, NetBiosProc inNetBiosProc )
{
  // Reset the LAN adapter so that we can begin querying it
  NCB Ncb;
  memset(&Ncb, 0, sizeof(Ncb));
  Ncb.ncb_command = NCBRESET;
  Ncb.ncb_lana_num = adapter_num;

  if(inNetBiosProc(&Ncb) != NRC_GOODRET)
	{
		char str[2];
    mac_addr = "bad (NCBRESET): ";
		str[0]=Ncb.ncb_retcode;str[1]=0;
    mac_addr += std::string(str);
    return false;
  }

  // Prepare to get the adapter status block
  memset(&Ncb,0,sizeof(Ncb));
  Ncb.ncb_command = NCBASTAT;
  Ncb.ncb_lana_num = adapter_num;
  strcpy((char *) Ncb.ncb_callname, "*");
  struct ASTAT
  {
    ADAPTER_STATUS adapt;
    NAME_BUFFER NameBuff[30];
  } Adapter;
  memset(&Adapter,0,sizeof(Adapter));
  Ncb.ncb_buffer = (unsigned char *)&Adapter;
  Ncb.ncb_length = sizeof(Adapter);

  // Get the adapter's info and, if this works, return it in standard,
  // colon-delimited form.
  if (inNetBiosProc(&Ncb) == 0)
  {
    char acMAC[18];
    sprintf(acMAC, "%02X:%02X:%02X:%02X:%02X:%02X",
            int (Adapter.adapt.adapter_address[0]),
            int (Adapter.adapt.adapter_address[1]),
            int (Adapter.adapt.adapter_address[2]),
            int (Adapter.adapt.adapter_address[3]),
            int (Adapter.adapt.adapter_address[4]),
            int (Adapter.adapt.adapter_address[5]));
    mac_addr = acMAC;
    return true;
  }
  else
  {
		char str[2];
    mac_addr = "bad (NCBASTAT): ";
		str[0]=Ncb.ncb_retcode;str[1]=0;
    mac_addr += std::string(str);
    return false;
  }
}




bool NetbiosHelper_GetAdapters( LANA_ENUM &outAdapterList, NetBiosProc &outNetBiosProc )
{
	HMODULE hMod=LoadLibrary("Netapi32.dll");

	if(!hMod)return(false);

	NetBiosProc Netbios=(NetBiosProc)GetProcAddress(hMod,"Netbios");

	if(!Netbios)return(false);

  NCB Ncb;
  memset(&Ncb, 0, sizeof(NCB));
  Ncb.ncb_command = NCBENUM;
  Ncb.ncb_buffer = (unsigned char *)&outAdapterList;
  Ncb.ncb_length = sizeof(outAdapterList);

	Netbios(&Ncb);

	outNetBiosProc=Netbios;

	return(true);
}




//! check CRC against all installed network cards
//! /patam inMacAdd MAC Adresse in the format %02X%02X%02X%02X%02X%02X
//! /return true=CRC is ok, false=CRC is not valid (correct network card ist not installed of netbios is not present)
bool NetbiosHelper_CheckCRC( unsigned char inCrc[4] )
{
#ifndef USE_COPYPROTECTION
	return true;
#else // USE_COPYPROTECTION
	LANA_ENUM AdapterList;
	NetBiosProc NetBiosProc;

	if(!NetbiosHelper_GetAdapters(AdapterList,NetBiosProc))return(false);

  // Get all of the local ethernet addresses
	std::string mac_addr;
	for(int i = 0; i < AdapterList.length; ++i)
  {
    if(NetbiosHelper_GetAdapterInfo(AdapterList.lana[i],mac_addr,NetBiosProc))
    {
//      cout << "Adapter " << int (AdapterList.lana[i]) << "'s MAC is " << mac_addr << endl;
			unsigned char CRC[4];

			CCrytekKeyGen::GenerateHashCRCString((const unsigned char *)(mac_addr.c_str()),CRC);

			if(inCrc[0]==CRC[0])
			if(inCrc[1]==CRC[1])
			if(inCrc[2]==CRC[2])
			if(inCrc[3]==CRC[3])
			{
				return(true);
			}
    }
		/*
    else
    {
      cerr << "Failed to get MAC address! Do you" << endl;
      cerr << "have the NetBIOS protocol installed?" << endl;
      break;
    }
		*/
  }

	return false;
#endif // USE_COPYPROTECTION
}

//! get CRC of first network card
//! /patam inMacAdd MAC Adresse in the format %02X%02X%02X%02X%02X%02X
//! /return true=operation was successfull, false=operation failed, no network card installed of netbios is not present
bool NetbiosHelper_GetCRC( unsigned char outCRC[4] )
{
	LANA_ENUM AdapterList;
	NetBiosProc NetBiosProc;

	if(!NetbiosHelper_GetAdapters(AdapterList,NetBiosProc))return(false);

  // Get all of the local ethernet addresses
	std::string mac_addr;
	for(int i = 0; i < AdapterList.length; ++i)
  {
    if(NetbiosHelper_GetAdapterInfo(AdapterList.lana[i],mac_addr,NetBiosProc))
    {
//      cout << "Adapter " << int (AdapterList.lana[i]) << "'s MAC is " << mac_addr << endl;
			CCrytekKeyGen::GenerateHashCRCString((const unsigned char *)(mac_addr.c_str()),outCRC);

			return(true);
    }
		/*
    else
    {
      cerr << "Failed to get MAC address! Do you" << endl;
      cerr << "have the NetBIOS protocol installed?" << endl;
      break;
    }
		*/
  }

	return(false);
}

