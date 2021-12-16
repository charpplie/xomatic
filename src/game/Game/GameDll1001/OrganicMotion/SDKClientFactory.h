/*	
 *	=========================================================================================
 *	Copyright(c) 2007 Organic Motion, Inc. All Rights Reserved.
 *	The coded instructions, statements, computer programs, and/or related
 *	material (collectively the "Code") in these files contain unpublished
 *	information proprietary to Organic Motion, Inc., which is protected by
 *	United States of America federal copyright law and by international treaties.
 *	Use, duplication, and or distribution only with written permission from Organic Motion, Inc.
 *	THE CODE IS PROVIDED "AS IS" AND WITHOUT WARRANTY.
 *	=========================================================================================
 */

#pragma once

#include "sdkclientinterfaces.h"


//===========================================================================
//	CSDKClientFactory
//		A class used to handle creation and destruction of interface-based 
//		objects.
//===========================================================================
class CSDKClientFactory
{
public:

	static ISDKClient *	GetNewSDKClient();
	static void			DestroySDKClient(ISDKClient * pSDKClient);

	static IStreamDataFrame *	GetNewStreamDataFrame();
	static void					DestroyStreamDataFrame(IStreamDataFrame * pFrame);

};
