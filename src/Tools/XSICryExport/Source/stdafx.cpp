//***************************************************************************************
//
// File supervisor: Softimage Rendering & Pipeline team
//
// (c) Copyright 2001-2005 Avid Technology, Inc. . All rights reserved.
//
//***************************************************************************************

/****************************************************************************************
THIS CODE IS PUBLISHED AS A SAMPLE ONLY AND IS PROVIDED "AS IS".
IN NO EVENT SHALL SOFTIMAGE, AVID TECHNOLOGY, INC. AND/OR THEIR RESPECTIVE
SUPPLIERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
CONNECTION WITH THE USE OR PERFORMANCE OF THIS CODE .

COPYRIGHT NOTICE. Copyright © 1999-2005 Avid Technology Inc. . All rights reserved. 

SOFTIMAGE is a registered trademark of Avid Technology Inc. or its subsidiaries 
or divisions. Windows NT is a registered trademark of Microsoft Corp. All other
trademarks contained herein are the property of their respective owners. 
****************************************************************************************/

/*! \file stdafx.cpp   
    source file that includes just the standard includes
*/

#include "stdafx.h"

void XSILog( const char* pFormat, ... )
{
	XSI::Application app;
	char str[2048];
	va_list args;
	va_start( args, pFormat );
	vsprintf( str, pFormat, args );
	XSI::CString strXsi;
	strXsi.PutAsciiString( str );
	app.LogMessage( strXsi );
}