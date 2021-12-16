/* 
	liblipsync_register.h 

	Copyright (c) 2003 Annosoft, LLC. Garland, Texas. All rights reserved.     
	This program and header file contains the confidential trade secret 
	information of Annosoft, LLC.  Use, disclosure, or copying without 
	written consent is strictly prohibited. 

	This header file is used to access annosoft's proprietary Lipsync libraries,
	use of this file or the library files is prohibited without prior approval
	from annosoft.

	This surfaces the registration interface used by the annosoft
	libraries.     

	Before using the full version of the annosoft libraries
	This registration interface must be called, and filled
	in with parameters supplied by Annosoft at the time of purchase.

*/

#ifndef _H_LIBLIPSYNC_REGISTER
#define _H_LIBLIPSYNC_REGISTER		1

#include "liblipsync.h"

class ILibLipsyncRegistrar
{
protected:
	virtual ~ILibLipsyncRegistrar() {};	// use "Release()"
public:
	///////////////////////////////////////////////////////////////////////////
	// Set the user name under which this product is registered
	virtual void SetUserName(const char *szUserName) = 0;
	///////////////////////////////////////////////////////////////////////////
	// Set the company name under which this product is registered
	virtual void SetCompanyName(const char* szCompanyName) = 0;
	
	///////////////////////////////////////////////////////////////////////////
	// SetLicenseKey
	// This will activate the product. 
	virtual void SetLicenseKey(const char* szLicenseCode) = 0;

	//////////////////////////////////////////////////////////////////////////
	// Release()
	// call release after the license key has been set.
	// The license will still remain active even after release is called
	virtual void Release() = 0;


	//////////////////////////////////////////////////////////////////////////
	// v1.9

	///////////////////////////////////////////////////////////////////////////
	// AddLanguageKey 
	// This will activate a language. Use this to add one or more language
	// codes. 
	virtual void AddLanguageKey(const char* szLangCode) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// Retrieve the registration object
// Before using the lipsync methods, applications must first call this
// once at the beginning of the program. You can, but don't need to,
// call this method multiple times
// 
// LipsyncGetInfoMgr(&pRegistrar)
// pRegistrar->SetUserName([provided by annosoft])
// pRegistrar->SetCompanyName([provided by annosoft])
// pRegistrar->SetLicenseKey([provided by annosoft]);
// pRegistrar->Release().
LIBLIP_API serror LipsyncGetInfoMgr(ILibLipsyncRegistrar** ppRegistrar);




#endif
