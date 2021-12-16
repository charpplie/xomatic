////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2006.
// -------------------------------------------------------------------------
//  File name:   MeshCompiler.h
//  Version:     v1.00
//  Created:     6/22/2004 by Alexey Medvedev.
//  Compilers:   Visual Studio.NET 2005
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __AnimationCompiler_h__
#define __AnimationCompiler_h__
#pragma once

#include "IConvertor.h"
#include "cgf/CGFLoader.h"

#include "AnimationInfoLoader.h"
#include "ResourceCompiler.h"

struct ConvertContext;
class CContentCGF;
class CChunkFile;
class ICryXML;
class ILoaderCGFListener;
class CSkeletonInfo;
struct SAnimationDesc;
class CTrackStorage;



struct SCompilerBehavior
{
	bool m_bNeedCalculate; // that means we need recalculate compressed data. We need an uncompressed data
	bool m_bUseDBA; // that means - we use .dba file as reference 
	bool m_bNeedDeleteUncompressedData; // that means - we need delete only uncompressed chuncks
	bool m_bNeedDeleteAllData; // we need delete all data
	bool m_bDBAExists; // we need update only new file
	bool m_bNeedSwapEndian;
	bool m_bCheckLocomotion;
	bool m_bUseMultiThreading;
	int m_iSupportedThreads;

	SCompilerBehavior() 
	{
		m_bNeedCalculate = false;
		m_bUseDBA = true;
		m_bNeedDeleteUncompressedData = false;
		m_bNeedDeleteAllData = false;
		m_bDBAExists = false; 
		m_bNeedSwapEndian = false;
		m_bCheckLocomotion = false;
		m_bUseMultiThreading = false;
		m_iSupportedThreads = 0;
	}
};

class CAnimationCompiler;
//struct SAnimationDesc;

struct ThreadCompiler //: public CrySimpleThread<>
{
	ThreadCompiler(int a_id)
		: m_id(a_id)
	{
	}

	void Run();
	void Cancel();

	string m_referenceModel;
	CSkeletonInfo* m_currentSkeleton;
	string m_FilePathDBA;
	string m_animFile;
	string m_destFile; 
	ILoaderCGFListener* m_listener;
	ConvertContext m_cc;
	SAnimationDesc m_pDesc;
	int m_oldsize;
	CAnimationCompiler* m_pCompiler;
	int m_id;
	int m_resSize;
};


//typedef CrySimpleThread<ThreadCompiler> RCThreadCompiler;

class CAnimationCompiler :
	public IConvertor,
	public ICompiler
{
public:
	CAnimationCompiler(ICryXML * pXML);
	virtual ~CAnimationCompiler();

	class Error
	{
	public:
		Error (int nCode);
		Error (const char* szFormat, ...);
		const char* c_str()const {return m_strReason.c_str();}
	protected:
		string m_strReason;
	};

	// interface ICompiler + IConvertor ------------------------------------------------

	//! Release memory of interface.
	virtual void Release();

	// interface ICompiler -------------------------------------------------------------

	//! Process file
	//! \return success
	virtual bool Process(ConvertContext &cc);

	XmlNodeRef LoadXml( const char *filename );
	uint32 LoadIKSetup( const char* AimIK_FileName, CSkinningInfo* pSkinInfo );

	//! Construct the name of the file that will be produced from the source file
	//! and call ConvertContext::SetOutputFile().
	// @param cc is the context of conversion; it contains valid sourceFile.
	virtual void ConstructAndSetOutputFile(ConvertContext &cc);

	//! Returns name of the file which will be used by RC in
	//! its "up-t-to-date check": RC will compare timestamp of this
	//! file and timestamp of the source file (RC will not perform 
	//! compilation if timestamps are equal). If compilation
	//! finished successfully, then RC will change timestamp
	//! of this file to the timestamp of the source.
	//! If the filename is "", then RC skips the "up-to-date check".
	// @param cc is the context of conversion; it contains valid sourceFile.
	// @param filenameBuffer is the buffer for the filename
	// @param bufferSize size of the buffer in bytes
	virtual void GetFilenameForUpToDateCheck(ConvertContext &cc, char* filenameBuffer, size_t bufferSize) const;

	// interface IConvertor ------------------------------------------------------------

	//! Initialize the convertor.
	// @param cc is the context of conversion; it contains valid directory and may contain fallback instruction.
	virtual void Init(IConfig* config, const char* exePath) {}

	//! Finalize the convertor.
	// @param cc is the context of conversion; it contains valid directory and may contain fallback instruction.
	virtual void DeInit() {}

	//! Return an object that will do the actual processing. For some implementations, this may be
	//! the original object. Should not be called more than once if SupportsMultithreading() returns false.
	//! Otherwise multiple compilers can be called and run from separate threads.
	virtual ICompiler* CreateCompiler();

	//! Check whether the convertor supports multithreading. See CreateCompiler() for more details.
	virtual bool SupportsMultithreading() const;

	//! Return platforms supported by this convertor.
	virtual int GetNumPlatforms() const { return 3; };
	//! Get supported platform.
	//! @param index Index of platform must be in range 0 < index < GetNumPlatforms().
	virtual EPlatform GetPlatform( int index ) const { 

		if (index == 0)
			return ePlatform_PC; 
		if (index == 1)
			return ePlatform_X360;
		if (index == 2)
			return ePlatform_PS3;
		return ePlatform_UNKNOWN;
	};

	//! Get number of supported extensions.
	virtual int GetNumExt() const { return 1; };
	//! Get supported extension.
	//! @param index Index of extension must be in range 0 < index < GetNumExt().
	virtual const char* GetExt( int index ) const { return "cba";};

	// this should retrieve the timestamp of the convertor executable:
	// when it was created by the linker, normally. This date/time is used to
	// compare with the compiled file date/time and even if the compiled file
	// is not older than the source file, it will be recompiled if it's older than the
	// convertor
	virtual DWORD GetTimestamp() const { return GetTimestampForLoadedLibrary(g_hInst);};

	// Create a convert context object. It is necessary to have the convertor
	// create this object rather than creating it in the main EXE to work
	// correctly across DLL boundaries.
	virtual IConvertContext* CreateConvertContext() const {return new ConvertContext;}

	// ---------------------------------------------------------------------------------

	void  FillFileList(std::vector<string>& list, const char * path, const char * ext, bool bRecursive);

	CChunkFile m_ChunkFileCAF;
	CChunkFile m_ChunkFileAIM;
	uint32 SaveAnimationImageFileCAF(const char* name, FILETIME timeStamp);
	uint32 SaveAnimationImageFileAIM(const char* name, FILETIME timeStamp);

	void ProcessSingleAnimation(
		const char* refernceModel,
		CSkeletonInfo* currentSkeleton, 
		const string& strFilePathDBA, const string& animFile, const string& destFile, 
		ILoaderCGFListener* listener,
		ConvertContext& cc, SAnimationDesc& pDesc, int oldsize
		);

	const SCompilerBehavior & GetBehavior() const
	{
		return m_Behavior;
	}

	// tack stack initialisation
	void PrepareThreads();
	// return timesteps

	ThreadCompiler* AcquireThread();
	void ReleaseThread(ThreadCompiler* thread);
	void WaitAllThreads();

public:
	ICryXML*	m_pXMLParser;

	bool m_bStatistics;
	uint32 m_TotalShared;
	uint32 m_TotalMemoryShared;
	uint32 m_TotalTracks;
	uint32 m_TotalMemory;

	int m_iCount;
	float m_fAverage;
	float m_fTotal;

	//int m_iThreads;
	std::auto_ptr<CTrackStorage> m_pTrackStorage;
	SCompilerBehavior m_Behavior;
	int m_refCount;

private:
	std::vector<ThreadCompiler*> m_vThreads;
	CryCriticalSectionRC				 m_threadsLock;
	bool m_bProcessed;
};

extern CAnimationCompiler * g_pAnimationCompiler;

#define SWAP 	if (g_pAnimationCompiler->GetBehavior().m_bNeedSwapEndian) 

/*
template <class T>
void swappod(T& t)
{
	if (g_pAnimationCompiler->GetBehavior().m_bNeedSwapEndian)
	{

		char * p = (char *)&t;
		for (uint32 i = 0; i < sizeof(t); i +=2)
		{
			std::swap(*(p + i), *(p + i + 1));	
		}
	}
}
*/


#endif