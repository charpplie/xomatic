#ifndef __ConsoleHotUpdate_h__
#define __ConsoleHotUpdate_h__
#pragma once

#include "CryThread.h"

class CConsoleHotUpdate
{
private:
	class CThread :	public CryThread<CThread>
	{
		// Structures
		private:
			struct HotSyncFileTypes
			{
				const char * extension;
				const char * compileparams;
				bool ignore;
				bool compile;
			};	

			static HotSyncFileTypes fileTypes[];

		public:
			CThread();
			~CThread();

		public:
			void Begin();
			void End();
			void Resume();

			void FileProcessingPause();
			void FileProcessingResume();

			void AddFile(const char *file);

		private:

			void ProcessFile(const char *file);
			bool IgnoreFile(const char * ext);
			bool NeedsCompiling(const char * ext);
			bool GetCompileOptions(const char * ext, CString & options);
			bool ResourceCompiler(CHotUpdateSystem::eConsolePlatforms ePlatform, const char *file, std::vector<string> &output, const char * ext);
			bool AssetCopyAndNotify(CHotUpdateSystem::eConsolePlatforms ePlatform, const char *fileOriginal, std::vector<string> &files);

			// CryThread
		public:
			virtual void Run();

		private:
			bool m_bRun;
			CryConditionVariable m_processCondition;
			CryMutex m_processMutex;
			CryMutex m_pauseMutex;

			CryMT::set<string> m_files;
		} m_thread;

public:
	static CConsoleHotUpdate &Instance()
	{
		static CConsoleHotUpdate instance;
		return instance;
	}

private:
	CConsoleHotUpdate();
	~CConsoleHotUpdate();

public:
	void FileProcessingPause();
	void FileProcessingResume();

	void NotifyFilesChanged(std::set<CString> &files);
};

#endif //__ConsoleHotUpdate_h__
