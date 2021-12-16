/* 
	stfloatingpointstate.h 

	Copyright (C) 2003 Annosoft, LLC. Garland, Texas. All rights reserved.     
	This program and header file contains the confidential trade secret 
	information of Annosoft, LLC.  Use, disclosure, or copying without 
	written consent is strictly prohibited. 
*/
/**
@file stfloatingpointstate.h
@brief
	This file declares a useful class for doing 23 bit float to 53 bit float
	conversion.


	Annosoft requires 53 bit precision for it's operations
	many graphics programs run better on 23 bits.
	There is contention.

	We developed this simple stack based state mechanism
	to save and restore the floating point state to address this.

    There were some bugs initially relating to this, but we feel we've tracked
    them down. However, there may be some. This file is exposed so that
    there is a quick workaround.

*/


#ifndef _H_STFLOATINGPOINTSTATE
#define _H_STFLOATINGPOINTSTATE




#include <float.h>

/**@ingroup anno_sdk_classes
@brief A stack utility class that allows applications (and the SDK)
    to push and pop the floating point state. 

	Annosoft requires 53 bit precision for it's operations
	many graphics programs run better on 23 bits.
	There is contention.<emit \<br/\>>

	We developed this simple stack based state mechanism
	to save and restore the floating point state to address this.
@code
myFunction()
{ 
    CStFloatingPointState st; 

    // call the lipsync function
    ...

    // st's destructor gets called automatically when it goes out of scope    
}
*/
class CStFloatingPointState
{
public:
    /** @brief constructor which stores the current fp state and then sets the
        floating point state to _PC_53
    
        This is a stack based program. Just create it and then let its destructor
        get called when it goes out of scope
    */
	CStFloatingPointState()
	{
#ifdef WIN32	
		m_oldState = _controlfp(0, 0);
		_controlfp( _PC_53, MCW_PC ) ;
        _controlfp(_MCW_EM, _EM_UNDERFLOW);

#endif
	}
    /// destructor. Restores the floating point state to the saved state.
	~CStFloatingPointState()
	{
#ifdef WIN32	
		_controlfp( m_oldState, 0xfffff );
        _clearfp();
#endif	
	}
	unsigned int m_oldState;	/// the saved fp state. (stored by the constructor).
};


#endif