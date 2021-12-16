#ifndef __LevelLoader_h__
#define __LevelLoader_h__

#include <ISystem.h>
#include <ITimer.h>

#define CRY_SAFE_RETURN(cond, rv)			{if(cond) {CryLogAlways("safely returned from %s in %s @%i",  __FUNCTION__, __FILE__, __LINE__); return rv;}}
#define	CRY_SAFE_RETURN_(cond, rv)			{if(cond) return rv;}
#define	CRY_NO_RETURN_VALUE

#define CRY_SAFE_CREATE(ptr, ctor)			{if(!ptr) ptr = ctor;}

#define CRY_ASSERT_FUNCTION_CALLED(times)	{	\
	static int callCounter = 0;					\
	++callCounter;								\
	CRY_ASSERT(callCounter <= times);			\
	CryLogAlways("%s called %i times", __FUNCTION__, callCounter);	}\


struct ILevelInfo
{
	typedef std::vector<string> TStringVec;

	virtual const char*			GetName() const = 0;
	virtual const char*			GetPath() const = 0;
	virtual const char*			GetPaks() const = 0;
	virtual int					GetHeightmapSize() const = 0;
	virtual const TStringVec&	GetMusicLibs() const = 0;
};

class CLevelInfo : public ILevelInfo
{
	friend class CLevelLoader;
public:
	CLevelInfo();;
	virtual ~CLevelInfo();

	//ILevelInfo implementation
	virtual const char*			GetName() const				{ return m_levelName.c_str(); };
	virtual const char*			GetPath() const				{ return m_levelPath.c_str(); };
	virtual const char*			GetPaks() const				{ return m_levelPaks.c_str(); };
	virtual int					GetHeightmapSize() const	{ return m_heightmapSize; };
	virtual const TStringVec&	GetMusicLibs() const		{ return m_musicLibs; };
	//ILevelInfo implementation/
	

private:
	void						ReadMetaData();
	bool						ReadInfo();

	string			m_levelName;
	string			m_levelPath;
	string			m_levelPaks;
	string			m_levelDisplayName;
	TStringVec		m_musicLibs;
	TStringVec		m_gamerules;
	int				m_heightmapSize;

	string			m_xmlFile;
	int				m_cgfCount;
};


class CLevelLoader :
	public ISystem::ILoadingProgressListener
{
public:
	typedef std::vector<CLevelInfo>	TLevelInfoVector;
public:
	CLevelLoader(ISystem* pSystem, const char* levelsFolder);
	virtual ~CLevelLoader();

	virtual void Rescan(const char* levelsFolder);
	virtual void ScanFolder(const char* subfolder);
	virtual bool LoadLevel( const char* levelName );

	virtual CLevelInfo* GetLevelInfo(const char* levelName);

	// Special events to be called from game.
	virtual void OnLevelNotFound(const char* levelName);
	virtual void OnLoadingStart(ILevelInfo* pLevel);
	virtual void OnLoadingComplete(ILevelInfo* pLevel);
	virtual void OnLoadingError(ILevelInfo* pLevel, const char* error);

	// ILoadingProgessListener
	virtual void OnLoadingProgress(int steps);
	// ~ILoadingProgessListener

private:
	bool LoadLevelInfo( CLevelInfo &levelInfo );

	ISystem*			m_pSystem;
	TLevelInfoVector	m_levelInfos;
	string				m_levelsFolder;
	ILevelInfo*			m_pLoadingLevelInfo;

	string				m_lastLevelName;
	float				m_fLastLevelLoadTime;
	float				m_fFilteredProgress;
	float				m_fLastTime;

	bool				m_bLevelLoaded;
	bool				m_bRecordingFileOpens;

	int					m_nLoadedLevelsCount;

	CTimeValue			m_tLevelLoadingStart;
};

#endif //__LevelLoader_h__