////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   CleanString.h
//  Version:     v1.00
//  Created:     07/10/2009 by Steve Barnett.
//  Description: This the declaration for CleanString
// -------------------------------------------------------------------------
//  History:
//
//	08/10/2009: Clean up a string to make it safe for use in an XML doc
//
////////////////////////////////////////////////////////////////////////////

#ifndef __CLEANSTRING_H__
#define __CLEANSTRING_H__

void CleanString( char* const pString );
void CleanString( char* const pDest, const char* const pString );

#endif
