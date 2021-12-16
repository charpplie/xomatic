#ifndef __CRYSIMPLECACHE__
#define __CRYSIMPLECACHE__

#include <map>
#include <vector>
#include <list>
#include "CrySimpleMutex.hpp"
#include "../STLHelper.hpp"

/*class CCrySimpleCacheEntry
{
	tdCache
public:
protected:
private:
};*/

typedef std::map<tdHash,tdHash>				tdEntries;
typedef std::map<tdHash,tdDataVector> tdData;

class CCrySimpleCache
{
	volatile bool								m_CachingEnabled;
	tdEntries										m_Entries;
	tdData											m_Data;
	CCrySimpleMutex							m_Mutex;
	CCrySimpleMutex							m_FileMutex;
	
	std::string									CreateFileName(const tdHash& rHash)const;

public:
	void												Init();
	bool												Find(const tdHash& rHash,tdDataVector& rData);
	void												Add(const tdHash& rHash,const tdDataVector& rData);

	bool												LoadCacheFile( const std::string &filename );
	void												Finalize();

	void                        ThreadFunc_SavePendingCacheEntries();

	static CCrySimpleCache&			Instance();

	std::list<tdDataVector*>		m_pendingCacheEntries;
};

#endif
