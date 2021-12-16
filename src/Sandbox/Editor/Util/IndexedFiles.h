//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2010.
// -------------------------------------------------------------------------
//  File Name        : IndexedFiles.h
//  Author           : Jaewon Jung
//  Time of creation : 1/29/2010   13:46
//  Compilers        : VS2008
//  Description      : Tagged files database for 'SmartFileOpen' dialog
//  Notice           : Refer "SmartFileOpenDialog.h"
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////

#include "FileUtil.h"

class CIndexedFiles
{
	friend class CFileIndexingThread;
public:
	static CIndexedFiles& GetDB()
	{ 
		// A global database for tagged files
		static CIndexedFiles s_indexedFiles;

		return s_indexedFiles; 
	}

	static bool HasFileIndexingDone()
	{ return s_bIndexingDone > 0; }
	
	static void StartFileIndexing()
	{
		assert(s_bIndexingDone == 0);
		GetFileIndexingThread().Start(-1, "FileIndexing");
	}

public:
	void Initialize(const CString& path, CFileUtil::ScanDirectoryUpdateCallBack updateCB=NULL);

	// Adds a new file to the database.
	void AddFile(const CFileUtil::FileDesc& path);
	// Removes a no-longer-existing file from the database.
	void RemoveFile(const CString& path);
	// Refreshes this database for the subdirectory.
	void Refresh(const CString& path, bool recursive=true);

	void GetFilesWithTags(CFileUtil::FileArray& files, const std::vector<CString>& tags) const;

	//! This method returns all the tags which start with a given prefix.
	//! It is useful for the tag auto-completion.
	void GetTagsOfPrefix(std::vector<CString>& tags, const CString& prefix) const;

	uint32 GetTotalCount() const
	{ return (uint32)m_files.size(); }

private:
	CFileUtil::FileArray m_files;
	std::map<CString, int> m_pathToIndex;
	typedef std::map<CString, std::set<int> > TagTable;
	TagTable m_tags;
	CString m_rootPath;

	void GetTags(std::vector<CString>& tags, const CString& path) const;
	void PrepareTagTable();

	// A done flag for the background file indexing
	static volatile TIntAtomic s_bIndexingDone;
	// A thread for the background file indexing
	class CFileIndexingThread : public CryThread<CFileIndexingThread>
	{
	public:
		virtual void Run()
		{
			CIndexedFiles::GetDB().Initialize(PathUtil::GetGameFolder().c_str(), CallBack);
			CryInterlockedAdd(CIndexedFiles::s_bIndexingDone.Addr(), 1);
		}

		CFileIndexingThread() : m_abort(false) {}
		virtual ~CFileIndexingThread()
		{
			m_abort = true;
			Stop();
		}
	private:
		bool m_abort;
		static bool CallBack(const CString& msg)
		{
			if(CIndexedFiles::GetFileIndexingThread().m_abort)
				return false;
			return true;
		}
	};

	static CFileIndexingThread& GetFileIndexingThread()
	{
		static CFileIndexingThread s_fileIndexingThread;

		return s_fileIndexingThread;
	}
};