#ifndef _NAMECRCHELPER
#define _NAMECRCHELPER

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


//////////////////////////////////////////////////////////////////////////
// Class with CRC sum. Used for optimizations  s
//////////////////////////////////////////////////////////////////////////

//#include "VectorMap.h"
#include "CryName.h"

#define MAX_STATIC_CHARS	4096

#if !defined(USE_STATIC_NAME_TABLE)
	#define USE_STATIC_NAME_TABLE
#endif

#define _USE_LOWERCASE






	#define CRC_GEN gEnv->pSystem->GetCrc32Gen() 


namespace NameCRCHelper {
#define __ascii_tolower(c)      ( (((c) >= 'A') && ((c) <= 'Z')) ? ((c) - 'A' + 'a') : (c) )
ILINE void MakeLowercase(char * dest, const char * name)
{
	char * p = dest;

	for (const char * src = name; *src; ++src)
	{
		*(p++) = __ascii_tolower(*src);
	}
	*p = 0;

}
}; // namespace NameCRCHelper

struct CNameCRCHelper
{
public:
	CNameCRCHelper() : m_CRC32Name(~0) {};

	uint32 GetCRC32() const { return m_CRC32Name;};

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(m_Name);
	}
protected:
	string m_Name;		// the name of the animation (not the name of the file) - unique per-model
	//CCryName m_Name;
	uint32 m_CRC32Name;			//hash value for searching animations

	const char * GetName() const { return m_Name.c_str(); };
//	const string& GetNameString() const { return m_Name; };

	//----------------------------------------------------------------------------------
	// Set name and compute CRC value for it
	//----------------------------------------------------------------------------------

	void SetName(const string& name)
	{
		m_Name = name; 

#ifdef _USE_LOWERCASE

		assert (name.size() < MAX_STATIC_CHARS);
		char pStringArray[MAX_STATIC_CHARS];

		NameCRCHelper::MakeLowercase(pStringArray, name.c_str());

		m_CRC32Name = CRC_GEN->GetCRC32(pStringArray); 

#else
		m_CRC32Name = CRC_GEN->GetCRC32(m_Name.c_str()); 
#endif
	}

	void SetNameChar(const char * name)
	{
		m_Name = name; 
#ifdef _USE_LOWERCASE

		assert (strlen(name) < MAX_STATIC_CHARS);
		char pStringArray[MAX_STATIC_CHARS];

		NameCRCHelper::MakeLowercase(pStringArray, name);

		m_CRC32Name = CRC_GEN->GetCRC32(pStringArray); 

#else
		m_CRC32Name = CRC_GEN->GetCRC32(m_Name.c_str()); 
#endif
	}

//	void SetNameLower(const string& name)
//	{ 
//		m_Name = name; 
//		m_CRC32Name = CRC_GEN->GetCRC32(name.c_str()); 
//	}
};




//////////////////////////////////////////////////////////////////////////
// Custom hash map class. 
//////////////////////////////////////////////////////////////////////////
struct CNameCRCMap
{


	// fast method 
	typedef std::map<uint32, size_t> NameHashMap;
		//typedef VectorMap<uint32, size_t> NameHashMap;


	//----------------------------------------------------------------------------------
	// Returns the index of the animation from crc value
	//----------------------------------------------------------------------------------

	size_t	GetValueCRC(uint32 crc) const
	{
		NameHashMap::const_iterator it = m_HashMap.find(crc);

		if (it == m_HashMap.end())
			return -1;

		return it->second;
	}

	//----------------------------------------------------------------------------------
	// Returns the index of the animation from name. Name converted in lower case in this function
	//----------------------------------------------------------------------------------

	size_t GetValue(const char * name) const
	{
#ifdef _USE_LOWERCASE

		assert (strlen(name) < MAX_STATIC_CHARS);
		char pStringArray[MAX_STATIC_CHARS];

		NameCRCHelper::MakeLowercase(pStringArray, name);

		return GetValueCRC(CRC_GEN->GetCRC32(pStringArray));
#else
		return GetValueCRC(CRC_GEN->GetCRC32(name));
#endif
	}

	//----------------------------------------------------------------------------------
	// Returns the index of the animation from name. Name should be in lower case!!!
	//----------------------------------------------------------------------------------

	size_t GetValueLower(const char * name) const
	{
		return GetValueCRC(CRC_GEN->GetCRC32(name));
	}

	//----------------------------------------------------------------------------------
	// In
	//----------------------------------------------------------------------------------

	bool InsertValue(CNameCRCHelper * header, size_t num)
	{
		bool res = m_HashMap.find(header->GetCRC32()) == m_HashMap.end();
		//if (m_HashMap.find(header->GetCRC32()) != m_HashMap.end())
		//{
		//	AnimWarning("[Animation] %s exist in the model!", header->GetName());
		//}
		m_HashMap[header->GetCRC32()] = num;

		return res;

	}

	bool InsertValue(uint32 crc, size_t num)
	{
		bool res = m_HashMap.find(crc) == m_HashMap.end();
		m_HashMap[crc] = num;

		return res;
	}

	size_t GetAllocMemSize() const
	{
		return m_HashMap.size() * (sizeof(uint32) + sizeof(size_t));
	}

	size_t GetMapSize() const
	{
		return m_HashMap.size();
	}

	void GetMemoryUsage(class ICrySizer* pSizer) const
	{
		pSizer->AddObject(m_HashMap);
	}
protected:
	NameHashMap		m_HashMap;
};

//////////////////////////////////////////////////////////////////////////
// Class with definition GetAnimName and SetAnimName functions. For convenience 
//////////////////////////////////////////////////////////////////////////
/*
struct CAnimNameHelper : public CNameCRCHelper
{
	const char * GetAnimName() const { return GetName(); };
	const string& GetAnimNameString() const { return GetNameString(); };

	void SetAnimName(const string& name) { SetName(name); };

};

//////////////////////////////////////////////////////////////////////////
// Class with definition GetAnimName and SetAnimName functions. For convenience 
//////////////////////////////////////////////////////////////////////////
struct CPathNameHelper : public CNameCRCHelper
{
	const char * GetPathName() const { return GetName(); };
	const string& GetPathNameString() const { return GetNameString(); };

	void SetPathName(const string& name) { SetName(name); };
};

//////////////////////////////////////////////////////////////////////////
// Class with definition GetAnimName and SetAnimName functions. For convenience 
//////////////////////////////////////////////////////////////////////////
struct CJointNameHelper : public CNameCRCHelper
{
	const char * GetJointName() const { return GetName(); };
	const string& GetJointNameString() const { return GetNameString(); };

	void SetJointName(const string& name) { SetName(name); };
};
*/
#undef CRC_GEN

#endif