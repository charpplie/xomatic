////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:	IObservable.h
//  Version:	v1.00
//  Created:	12/07/2010 by Nicusor Nedelcu
//  Description:	Handy macro when working with observers in interfaces
//					
// -------------------------------------------------------------------------  
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __IObservable_h__
#define __IObservable_h__
#pragma once

// Observable macro to be used in pure interfaces
#define DEFINE_OBSERVABLE_PURE_METHODS( observerClassName ) \
	virtual bool RegisterObserver( observerClassName* pObserver ) = 0; \
	virtual bool UnregisterObserver( observerClassName* pObserver ) = 0; \
	virtual void UnregisterAllObservers() = 0;

#endif //__IObservable_h__