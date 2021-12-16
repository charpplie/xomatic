#include "StdAfx.h"
#include "IFlashPlayer.h"
#include "Flash.h"

//////////////////////////////////////////////////////////////////////////

CFlash* CFlash::m_pInst = NULL;
IFlashPlayer* CFlash::s_pFlashPlayerNull = NULL;



CFlash::CFlash()
{
//	s_pFlashPlayerNull = new CFlashPlayerNull(); //Diesel cut
}



CFlash::~CFlash()
{
	SAFE_RELEASE(s_pFlashPlayerNull);
}



IFlashPlayer* CFlash::CreateSafeFlashPlayerInstance()
{
	IFlashPlayer* player = gEnv->pSystem->CreateFlashPlayerInstance();
	if(player)
		return player; 
	return s_pFlashPlayerNull;
}
