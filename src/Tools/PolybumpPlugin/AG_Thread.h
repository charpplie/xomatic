/*! \file AG_Thread.h
	\brief definition of CAG_Thread class
	\author AMER GERZIC

	To use derive a new class from CAG_Thread class and
	override the virtual ThreadMain() function like following:			\n
																		\n	
	while(!ShouldDie())	\n
	{																	\n
		... let the worker work											\n
	}																	\n
																		\n
	History:
	- ***** Version 1.0.0.0 ***** 
	- Date: 08.17.2001	Author: Amer Gerzic
	- Class created and tested on windows NT
	- ***** Version 1.0.0.1 ***** 
	- Date: 08.19.2001	Author: Amer Gerzic
	- GetThreadStatus() function added. Reports is the thread running.
	- ***** Version 1.0.0.2 ***** 
	- Date: 07.08.2002	Author: Amer Gerzic
	- Fixed bug in the StartThread function with GetThreadStatus check for thread activity
	- The thread status checking was changed from WaitForSingleObject to GetExitCodeThread
	- Also ThreadMain function made private
	- ***** Version 1.0.0.3 ***** 
	- Date: 04.11.2003	Author: Amer Gerzic
	- Added function "shouldDie()" for easier thread abortion
*/

// lets assume we are running MS Compile Version 5 and up
#pragma once

//! Wrapper for win32 threading
class CAG_Thread  
{
	//! main thread function
	/*!
		This function is used only to call the virtual function (which must be overridden).
		In this way all non-static members can be used from within the thread.
	*/
	static DWORD WINAPI ThreadMain(LPVOID lParam)
	{
		CAG_Thread *pThis = static_cast<CAG_Thread*>(lParam);
		if(pThis != NULL)
			return pThis->ThreadMain();
		else return -1;
	}

protected:
	//! thread id
	DWORD m_dwThreadID;

	//! main window handle
	HWND m_hMainWnd;

	//! thread handle
	HANDLE m_hThread;

	//! handle of the stop event
	HANDLE m_hStopEvent;

	//! Checks if the thread should abort the action
	BOOL ShouldDie() { return(::WaitForSingleObject(m_hStopEvent, 0) == WAIT_OBJECT_0); };

	//! virtual main thread function
	/*!
		This function must be overridden when new class is derived from this one.
	*/
	virtual UINT ThreadMain() = 0;

public:
	//! Standard constructor
	CAG_Thread();
	//! Standard destructor
	virtual ~CAG_Thread();

	//! thread startup function
	/*!
		Use this function to start the thread execution.
	*/
	BOOL StartThread(HWND hWnd = NULL);

	//! thread termination function
	/*!
		Use this function to stop the thread execution. Note that the internal event 
		is used to stop the thread. If the event status is not checked during the 
		thread execution, the applicaiton might freeze.
	*/
	BOOL StopThread();

	BOOL SendStop();

	//! returns thread running status
	/*!
		Use this function to get the thread status. It returns true if the thread
		is running and false if not.
	*/
	BOOL GetThreadStatus();
};
