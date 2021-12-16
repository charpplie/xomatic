////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   CleanString.cpp
//  Version:     v1.00
//  Created:     07/10/2009 by Steve Barnett.
//  Description: This the declaration for CleanString
// -------------------------------------------------------------------------
//  History:
//
//	08/10/2009: Clean up a string to make it safe for use in an XML doc
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

void CleanString( char* const pString )
{
	if ( !pString ) return;

	for ( char* pCur = pString; *pCur != '\0'; ++pCur )
	{
		if ( *pCur < 0x20 && *pCur != '\t' ) *pCur = '?';
	}
}

void CleanString( char* const pDest, const char* const pString )
{
	if ( !pString || !pDest ) return;

	char* pDst = pDest;
	for ( const char* pCur = pString; *pCur != '\0'; ++pCur, ++pDst )
	{
		if ( *pCur < 0x20 && *pCur != '\t' )
		{
			*pDst = '?';
		}
		else
		{
			*pDst = *pCur;
		}
	}
	*pDst = '\0';
}
