/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
-------------------------------------------------------------------------

Description: Automatic shotgun firemode. It works like the shotgun one, spawning
several pellets on a single shot, but doesn't require 'pump' action
and it has a 'single' magazine reload

-------------------------------------------------------------------------
History:
- 14:09:09   Benito Gangoso Rodriguez

*************************************************************************/
#include "StdAfx.h"
#include "AutomaticShotgun.h"


CAutomaticShotgun::CAutomaticShotgun()
{

}


CAutomaticShotgun::~CAutomaticShotgun()
{

}


void CAutomaticShotgun::Activate( bool activate )
{
	CSingle::Activate(activate);
}

void CAutomaticShotgun::Reload( int zoomed )
{
	CSingle::Reload(zoomed);
}

void CAutomaticShotgun::StartReload(int zoomed)
{
	CSingle::StartReload(zoomed);
}

void CAutomaticShotgun::EndReload( int zoomed )
{
	CSingle::EndReload(zoomed);
}

void CAutomaticShotgun::CancelReload()
{
	CSingle::CancelReload();
}

bool CAutomaticShotgun::CanCancelReload()
{
	return true;
}

const char* CAutomaticShotgun::GetType() const
{
	return "AutomaticShotgun";
}