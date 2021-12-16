#include "../StdTypes.hpp"
#ifdef _MSC_VER
#include <process.h>
#include <direct.h>
#endif
#ifdef UNIX
#include <pthread.h>
#endif
#include "../Error.hpp"
#include "../STLHelper.hpp"
#include "CrySimpleServer.hpp"
#include "CrySimpleSock.hpp"
#include "CrySimpleJob.hpp"
#include "CrySimpleCache.hpp"
#include "CrySimpleErrorLog.hpp"
#include "ShaderList.hpp"
#include <assert.h>
#include <io.h>
#include <algorithm>
#include <memory>


#ifdef WIN32
	#define EXTENSION ".exe"
#else
	#define EXTENSION ""
#endif

const static std::string SHADER_STRIPPER					=	"sce-cgcstrip"EXTENSION;
const static std::string SHADER_COMPILER					=	"sce-cgc"EXTENSION;
const static std::string SHADER_DISASSEMBLER			=	"sce-cgcdisasm"EXTENSION;
const static std::string SHADER_PROFILER					=	"NVShaderPerf"EXTENSION;

const static std::string SHADER_PATH_SOURCE				=	"Source";
const static std::string SHADER_PATH_BINARY				=	"Binary";
const static std::string SHADER_PATH_HALFSTRIPPED	=	"HalfStripped";
const static std::string SHADER_PATH_DISASSEMBLED	=	"DisAsm";
const static std::string SHADER_PATH_STRIPPPED		=	"Stripped";
const static std::string SHADER_PATH_CACHE				=	"Cache";

extern volatile long g_GlobalCompileTasks;

SEnviropment&	SEnviropment::Instance()
{
	static SEnviropment g_Enviropment;
	return g_Enviropment;
}

class CThreadData
{
	uint32_t		m_Counter;
	CCrySimpleSock*	m_pSock;
public:
							CThreadData(uint32_t		Counter,CCrySimpleSock*	pSock):
							m_Counter(Counter),
							m_pSock(pSock){}

							~CThreadData(){delete m_pSock;}

	CCrySimpleSock*	Socket(){return m_pSock;}
	uint32_t		ID()const{return m_Counter;}

};

struct SFileInfo
{
	std::string file;
	__time64_t  time_create;
};

//////////////////////////////////////////////////////////////////////////
inline bool CompareFileInfo( const SFileInfo &f1,const SFileInfo &f2 )
{
	return f1.time_create < f2.time_create;
}

//////////////////////////////////////////////////////////////////////////
void ScanDirectory( const char *path,std::vector<SFileInfo> &files )
{
	__finddata64_t c_file;
	intptr_t hFile;
	char full_path[MAX_PATH];
	char file_spec[MAX_PATH];
	char temp_path[MAX_PATH];

	strcpy_s(full_path,path);
	// Trim ending slash
	if (full_path[strlen(path)-1] == '/' || full_path[strlen(path)-1] == '\\')
		full_path[strlen(path)-1] = 0;

	sprintf_s(file_spec,"%s/*.*",full_path);

	if( (hFile = _findfirst64( file_spec, &c_file )) != -1L ) {
		do {
			if (c_file.attrib & _A_SUBDIR)
			{
				if (c_file.name[0] != '.')
				{
					sprintf_s(temp_path, "%s/%s", full_path, c_file.name);
					ScanDirectory(temp_path,files);
				}
			}
			else
			{
				sprintf_s(temp_path, "%s/%s", full_path, c_file.name);
				SFileInfo f;
				f.file = temp_path;
				f.time_create = c_file.time_create;
				files.push_back( f );
			}
		} while( _findnext64( hFile, &c_file ) == 0 );

		_findclose( hFile );
	}
}

//////////////////////////////////////////////////////////////////////////
uint32_t __stdcall Compile(void* pData)
{
	CThreadData* pThreadData	=	reinterpret_cast<CThreadData*>(pData);
	std::vector<uint8_t> Vec;
	try
	{
		if(pThreadData->Socket()->Recv(Vec))
		{
			std::auto_ptr<CCrySimpleJob> job( new CCrySimpleJob(pThreadData->Socket()->PeerIP(), Vec) );
			pThreadData->Socket()->Send(Vec);
		}
	}
	catch(const CServerError err)
	{
		std::string Value = err.text + "\n" + err.compilerOutput;
		CCrySimpleErrorLog::Instance().Add(Vec,Value);
		CRYSIMPLE_LOG( "<Error> "+err.text );
		
		std::string returnStr = std::string( "[ERROR] " ) + Value;
		// Send error back
		Vec.resize(returnStr.size()+1);
		for (size_t i = 0; i < returnStr.size();i++)
		{
			Vec[i] = returnStr[i];
		}
		Vec[Value.size()] = 0;

		// Compress output data
		tdDataVector rDataRaw;
		rDataRaw.swap(Vec);
		if (!CSTLHelper::Compress( rDataRaw,Vec ))
			Vec.resize(0);

		pThreadData->Socket()->Send(Vec);
	}
	delete pThreadData;
	return 0;
}

//////////////////////////////////////////////////////////////////////////
uint32_t __stdcall TickThread(void* pData)
{
	DWORD t0,t1;
	while (true)
	{
		t1 = GetTickCount();
		if (t1 < t0 || (t1-t0) > 100)
		{
			t0 = t1;
			char str[512];
			sprintf_s(str,"Crytek Remote Shader Compiler (%d compile tasks)",g_GlobalCompileTasks );
			SetConsoleTitle(str);
		}

		const uint32_t T1	=	GetTickCount();
		CCrySimpleErrorLog::Instance().Tick();
		CShaderList::Instance().Tick();
		CCrySimpleCache::Instance().ThreadFunc_SavePendingCacheEntries();
		const uint32_t T2	=	GetTickCount();
		if(T2-T1<100)
			Sleep(100-T2+T1);
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
uint32_t __stdcall LoadCache(void* pUserData)
{

	if(CCrySimpleCache::Instance().LoadCacheFile( SEnviropment::Instance().m_Cache+"Cache.dat" ))
	{
#ifdef WIN32
		printf( "Creating cache backup...\n" );
		DeleteFile( (SEnviropment::Instance().m_Cache+"Cache.bak2").c_str() );
		printf( "Move %s to %s\n",(SEnviropment::Instance().m_Cache+"Cache.bak").c_str(),(SEnviropment::Instance().m_Cache+"Cache.bak2").c_str() );
		MoveFile( (SEnviropment::Instance().m_Cache+"Cache.bak").c_str(),(SEnviropment::Instance().m_Cache+"Cache.bak2").c_str() );
		printf( "Copy %s to %s\n",(SEnviropment::Instance().m_Cache+"Cache.dat").c_str(),(SEnviropment::Instance().m_Cache+"Cache.bak").c_str() );
		CopyFile( (SEnviropment::Instance().m_Cache+"Cache.dat").c_str(),(SEnviropment::Instance().m_Cache+"Cache.bak").c_str(),FALSE );
		printf( "Cache backup done.\n" );
#endif
	}
	else
	{
		// Restoring backup cache!
		printf( "Cache file corrupted!!!\n" );
		printf( "Restoring backup cache...\n" );
		DeleteFile( (SEnviropment::Instance().m_Cache+"Cache.dat").c_str() );
		printf( "Copy %s to %s\n",(SEnviropment::Instance().m_Cache+"Cache.bak").c_str(),(SEnviropment::Instance().m_Cache+"Cache.dat").c_str() );
		CopyFile( (SEnviropment::Instance().m_Cache+"Cache.bak").c_str(),(SEnviropment::Instance().m_Cache+"Cache.dat").c_str(),FALSE );
		if (!CCrySimpleCache::Instance().LoadCacheFile( SEnviropment::Instance().m_Cache+"Cache.dat" ))
		{
			// Backup file corrupted too!
			printf( "Backup file corrupted too!!!\n" );
			printf( "Deleting cache completely\n" );
			DeleteFile( (SEnviropment::Instance().m_Cache+"Cache.dat").c_str() );
		}
	}

	CCrySimpleCache::Instance().Finalize();
	printf( "Ready\n" );


	/*
	printf( "Scanning Directory: %s\n",SEnviropment::Instance().m_Cache.c_str() );

	CCrySimpleCache::Instance().LoadCacheFile( SEnviropment::Instance().m_Cache+"Cache.dat" );

	DWORD t0 = GetTickCount();

	std::vector<SFileInfo> FileList;
	FileList.reserve( 1000000 );
	//feeding cache with existing files
	//ScanDirectory( SEnviropment::Instance().m_Cache.c_str(),FileList );

	// Sort by creation time, to minimize seek time of file reads.
	std::sort( FileList.begin(),FileList.end(),CompareFileInfo );

	if (FileList.size() > 0)
	{
		char fname[_MAX_FNAME];
		std::vector<uint8_t> Data;
		Data.resize(100000);

		printf("Loading Cache:   0%%");
		uint32_t num=0;
		uint32_t Loaded=0;
		for(uint32_t a=0,Size=(uint32_t)FileList.size();a<Size;a++)
		{
			//if(a*100/Size!=Loaded)
			if (a%10 == 0)
			{
				DWORD t = GetTickCount();

				Loaded	=	static_cast<uint32_t>(a*100/Size);
				printf("\rLoading Cache: %3d%%   %6d  time=%d sec ",Loaded,static_cast<uint32_t>(a),(t-t0)/1000);
			}
			const std::string &Entry	=	FileList[a].file;
			if(Entry.size()<32)
				continue;
			const std::string &Name = &Entry.c_str()[Entry.size()-32];

			_splitpath( Entry.c_str(),NULL,NULL,fname,NULL );

			assert( strcmp( Name.c_str(),CSTLHelper::Hash2String(CSTLHelper::String2Hash(fname)).c_str()) == 0 );
			const tdHash	Hash	=	CSTLHelper::String2Hash(fname);

			if (CSTLHelper::FromFile(Entry,Data))
			{
				CCrySimpleCache::Instance().AddToHash(Hash,Data);
			}
			num=a;
		}
		DWORD t = GetTickCount();
		printf("\rLoading Cache: 100%% done %d shaders loaded in %d second  \n",num+1,(t-t0)/1000 );
	}
	*/
	return 0;
}


//////////////////////////////////////////////////////////////////////////
CCrySimpleServer::CCrySimpleServer(const char* pShaderModel,const char* pDst,const char* pSrc,const char* pEntryFunction)
{
	Init();
/*	SCompileData	CData;
	CData.m_Version				=	COMPILER_VERSION;
	CData.m_VertexShader	=	*pShaderModel=='v';
	strcpy(CData.m_Entry,pEntryFunction);

	std::vector<uint8_t> In;
	CSTLHelper::FromFile(pSrc,In);
	CData.m_ShaderSize	=	static_cast<uint32_t>(In.size());

	CSTLHelper::EndianSwizzleU32(CData.m_Version);
	CSTLHelper::EndianSwizzleU32(CData.m_ShaderSize);

	std::vector<uint8_t> Vec;
	Vec.resize(sizeof(SCompileData)+In.size());
	memcpy(&Vec[0],&CData,sizeof(SCompileData));
	memcpy(&Vec[sizeof(SCompileData)],&In[0],In.size());

	delete new CCrySimpleReflect(Vec,0);
	CSTLHelper::ToFile(pDst,Vec);
	*/
}

CCrySimpleServer::CCrySimpleServer()
{
	CrySimple_SECURE_START

		uint32_t Port = SEnviropment::Instance().m_port;

		CCrySimpleSock::Instance().InitRoot(Port);
		Init();
		CCrySimpleSock::Instance().Listen();

		{
			uint32_t Ret;
			CloseHandle(reinterpret_cast<HANDLE>(_beginthreadex(0,0,TickThread,0,0,&Ret)));
		}

		uint32_t JobCounter=0;
		while(1)
		{
			CThreadData* pData	=	new CThreadData(JobCounter++,CCrySimpleSock::Instance().Accept());
#if defined _MSC_VER
			uint32_t Ret;
			CloseHandle(reinterpret_cast<HANDLE>(_beginthreadex(0,0,Compile,(void*)pData,0,&Ret)));
#elif defined UNIX
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
			pthread_t thread;
			if (pthread_create(&thread, &attr,
						reinterpret_cast<void *(*)(void *)>(Compile),
						reinterpret_cast<void *>(pData)) != 0)
				CrySimple_ERROR("thread creation failed");
#else
#error "not implemented"
#endif
		}
	CrySimple_SECURE_END
}

void CCrySimpleServer::Init()
{
	//creating cache file structure
	_mkdir("Error");
	_mkdir("Cache");

	char CurrentDir[1024];
	GetCurrentDirectory(sizeof(CurrentDir),CurrentDir);
	SEnviropment::Instance().m_Root			=	CurrentDir;
	SEnviropment::Instance().m_Root			+=	"/";

	SEnviropment::Instance().m_Compiler	=	SEnviropment::Instance().m_Root+"Compiler/";
	SEnviropment::Instance().m_Cache		=	SEnviropment::Instance().m_Root+"Cache/";
	if (SEnviropment::Instance().m_Temp.empty())
		SEnviropment::Instance().m_Temp			=	SEnviropment::Instance().m_Root+"Temp/";
	if (SEnviropment::Instance().m_Error.empty())
		SEnviropment::Instance().m_Error			=	SEnviropment::Instance().m_Root+"Error/";
	if (SEnviropment::Instance().m_MailInterval == 0)
	{
		SEnviropment::Instance().m_MailInterval = 10;
	}

	_mkdir( SEnviropment::Instance().m_Error.c_str() );
	_mkdir( SEnviropment::Instance().m_Temp.c_str() );
	_mkdir( SEnviropment::Instance().m_Cache.c_str() );


	CShaderList::Instance().m_PC.Load( (SEnviropment::Instance().m_Cache+"ShaderList_PC.txt").c_str() );
	CShaderList::Instance().m_X360.Load( (SEnviropment::Instance().m_Cache+"ShaderList_X360.txt").c_str() );
	CShaderList::Instance().m_PS3.Load( (SEnviropment::Instance().m_Cache+"ShaderList_PS3.txt").c_str() );


	if(SEnviropment::Instance().m_Caching)
	{
		uint32_t Ret;
		CloseHandle(reinterpret_cast<HANDLE>(_beginthreadex(0,0,LoadCache,NULL,0,&Ret)));
	}
	else
		printf("\nNO CACHING, disabled by config\n");
	


}
