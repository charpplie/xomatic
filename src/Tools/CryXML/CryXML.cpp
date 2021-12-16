// CryXML.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "ICryXML.h"
#include "XMLSerializer.h"

//int sEmptyStringBuffer[] = { -1, 0, 0, 0 };
//string::StrHeader* string::m_emptyStringData = (string::StrHeader*)&sEmptyStringBuffer;
//wstring::StrHeader* wstring::m_emptyStringData = (wstring::StrHeader*)&sEmptyStringBuffer;

class CryXML : public ICryXML
{
public:
	CryXML();
	virtual void AddRef();
	virtual void Release();
	virtual IXMLSerializer* GetXMLSerializer();

private:
	int nRefCount;
	XMLSerializer serializer;
};

static CryXML* s_pCryXML = 0;

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	return TRUE;
}

ICryXML* __stdcall GetICryXML()
{
	if (s_pCryXML == 0)
		s_pCryXML = new CryXML;
	return s_pCryXML;
}

CryXML::CryXML()
:	nRefCount(0)
{
}

void CryXML::AddRef()
{
	++this->nRefCount;
}

void CryXML::Release()
{
	--this->nRefCount;
	if (this->nRefCount == 0)
		delete this;
}

IXMLSerializer* CryXML::GetXMLSerializer()
{
	return &this->serializer;
}


// STLPort requires folowing functions defined:

// when using STL Port _STLP_DEBUG and _STLP_DEBUG_TERMINATE - avoid actually 
// crashing (default terminator seems to kill the thread, which isn't nice).
#ifdef _STLP_DEBUG_TERMINATE
void __stl_debug_terminate(void)
{
	assert( 0 && "STL Debug Error" );
}
#endif
#ifdef _STLP_DEBUG_MESSAGE
void __stl_debug_message(const char * format_str, ...)
{	
	va_list __args;
	va_start( __args, format_str );
	vprintf(format_str, __args);
	va_end(__args);
}
#endif //_STLP_DEBUG_MESSAGE
