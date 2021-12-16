////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   IAnimationNameSelectionChangedListener.h
//  Version:     v1.00
//  Created:     02/09/2009 by Pau Novau
//  Description: Interface for listening to changes in the selected animation
//               in the Animation List control.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __I_Animation_Name_Selection_Changed_Listener__h__
#define __I_Animation_Name_Selection_Changed_Listener__h__
#pragma once

struct IAnimationNameSelectionChangedListener
{
	virtual void AnimationNameSelectionChanged( const CString& currentSelectedAnimationName ) = 0;
};

#endif