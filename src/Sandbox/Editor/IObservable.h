#ifndef		__IObservable_H__
#define		__IObservable_H__
#pragma once

//! Observable macro to be used in pure interfaces
#define DEFINE_OBSERVABLE_PURE_METHODS( observerClassName ) \
	virtual bool RegisterObserver( observerClassName* pObserver ) = 0; \
	virtual bool UnregisterObserver( observerClassName* pObserver ) = 0; \
	virtual void UnregisterAllObservers() = 0;

#endif