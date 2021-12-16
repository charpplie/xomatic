////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   loadingprofiler.cpp
//  Version:     v1.00
//  Created:     01/12/2007 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "System.h"
#include "LoadingProfiler.h"
#include "DiskProfiler.h"

#define LOADING_TIME_CONTAINER_MAX_TEXT_SIZE 1024


struct SLoadingTimeContainer : public _i_reference_target_t
{
	SLoadingTimeContainer() 	{}

	SLoadingTimeContainer(SLoadingTimeContainer * pParent, const char * pPureFuncName) 
	{ 
		m_dSelfMemUsage = m_dTotalMemUsage = m_dSelfTime = m_dTotalTime = 0; m_nCounter = 1; m_pFuncName = pPureFuncName; m_pParent = pParent;
	}

	static int Cmp_SLoadingTimeContainer_Time(const void* v1, const void* v2)
	{
		SLoadingTimeContainer *pChunk1 = (SLoadingTimeContainer*)v1;
		SLoadingTimeContainer *pChunk2 = (SLoadingTimeContainer*)v2;

		if(pChunk1->m_dSelfTime>pChunk2->m_dSelfTime)
			return -1;
		else if(pChunk1->m_dSelfTime<pChunk2->m_dSelfTime)
			return 1;

		return 0;
	}

	static int Cmp_SLoadingTimeContainer_MemUsage(const void* v1, const void* v2)
	{
		SLoadingTimeContainer *pChunk1 = (SLoadingTimeContainer*)v1;
		SLoadingTimeContainer *pChunk2 = (SLoadingTimeContainer*)v2;

		if(pChunk1->m_dSelfMemUsage>pChunk2->m_dSelfMemUsage)
			return -1;
		else if(pChunk1->m_dSelfMemUsage<pChunk2->m_dSelfMemUsage)
			return 1;

		return 0;
	}

	static double GetUsedMemory(ISystem * pSysytem)
	{
		static IMemoryManager::SProcessMemInfo processMemInfo;
		pSysytem->GetIMemoryManager()->GetProcessMemInfo(processMemInfo);
		return double(processMemInfo.PagefileUsage)/double(1024*1024);
	}


	void Clear() {
		for (size_t i= 0, end = m_pChilds.size(); i < end; ++i) {
			delete m_pChilds[i];
		}
	}

	~SLoadingTimeContainer() {
		Clear();
	}


	double m_dSelfTime, m_dTotalTime;
	double m_dSelfMemUsage, m_dTotalMemUsage;
	uint32 m_nCounter;

	const char * m_pFuncName;
	SLoadingTimeContainer * m_pParent;
	std::vector<SLoadingTimeContainer *> m_pChilds;

	DiskOperationInfo m_selfInfo;
	DiskOperationInfo m_totalInfo;
	bool m_bUsed;

};

bool operator== (const SLoadingTimeContainer& a, const SLoadingTimeContainer& b) {
	return b.m_pFuncName == a.m_pFuncName;
}

bool operator== (const SLoadingTimeContainer& a, const char * b) {
	return b == a.m_pFuncName;
}


SLoadingTimeContainer * CLoadingProfilerSystem::m_pCurrentLoadingTimeContainer = 0;
SLoadingTimeContainer * CLoadingProfilerSystem::m_pRoot = 0;
ICVar * CLoadingProfilerSystem::m_pEnableProfile = 0;



int CLoadingProfilerSystem::nLoadingProfileMode = 1;

int CLoadingProfilerSystem::nLoadingProfilerNotTrackedAllocations = -1;
CryCriticalSection CLoadingProfilerSystem::csLock;

void CLoadingProfilerSystem::OutputLoadingTimeStats(ILog * pLog, int nMode)
{
	nLoadingProfileMode = nMode;

	//if(nLoadingProfileMode>1)
	//{ // loading time stats
	//  pLog->Log("---------------------------------------------------------------------");
	//  pLog->Log("----------- Level loading time (sec) by call stack ------------------");
	//  pLog->Log(" ||Self |  Total |  Calls | CallStack||");
	//  pLog->Log("---------------------------------------------------------------------");

	//  LoadingTimeContainersMap::iterator it = mapLoadingTimeContainers.begin();
	//  for(; it != mapLoadingTimeContainers.end(); ++it)
	//  {
	//    const char * szName = it->first;
	//    const SLoadingTimeContainer * pTimeContainer = it->second;

	//    // call stack level
	//    char szNameNoStack[LOADING_TIME_CONTAINER_MAX_TEXT_SIZE]="";
	//    for(char * pTmp = (char *)szName; pTmp && pTmp<(szName+strlen(szName));)
	//    {
	//      pTmp = strstr(pTmp+2,">>");
	//      if(pTmp)
	//        strcat(szNameNoStack,". ");
	//    }

	//    strcat(szNameNoStack,pTimeContainer->m_pFuncName);

	//    pLog->Log("|%6.1f | %6.1f | %6d | %s|", 
	//      pTimeContainer->m_dSelfTime, pTimeContainer->m_dTotalTime, (int)pTimeContainer->m_nCounter, szNameNoStack);
	//  }

	//  pLog->Log("---------------------------------------------------------------------");
	//}

	//if(nLoadingProfileMode>1)
	//{ // loading mem stats
	//  pLog->Log("------- Level loading memory allocations (MB) by call stack ---------");
	//  pLog->Log(" ||Self |  Total |  Calls | CallStack||");
	//  pLog->Log("---------------------------------------------------------------------");

	//  LoadingTimeContainersMap::iterator it = mapLoadingTimeContainers.begin();
	//  for(; it != mapLoadingTimeContainers.end(); ++it)
	//  {
	//    const char * szName = it->first;
	//    const SLoadingTimeContainer * pTimeContainer = it->second;

	//    // call stack level
	//    char szNameNoStack[LOADING_TIME_CONTAINER_MAX_TEXT_SIZE]="";
	//    for(char * pTmp = (char *)szName; pTmp && pTmp<(szName+strlen(szName));)
	//    {
	//      pTmp = strstr(pTmp+2,">>");
	//      if(pTmp)
	//        strcat(szNameNoStack,". ");
	//    }

	//    strcat(szNameNoStack,pTimeContainer->m_pFuncName);

	//    pLog->Log("|%6.1f | %6.1f | %6d | %s|", 
	//      pTimeContainer->m_dSelfMemUsage, pTimeContainer->m_dTotalMemUsage, (int)pTimeContainer->m_nCounter, szNameNoStack);
	//  }
	//  pLog->Log("---------------------------------------------------------------------");

	//it = mapLoadingTimeContainers.begin();
	//pLog->Log("----------- Level file information by function by call stack -------------------------------------");
	//pLog->Log("||           Self          |           Total         |Bandwith|  Calls | Function||");
	//pLog->Log("|| Seeks |FileOpen|FileRead| Seeks |FileOpen|FileRead|  Kb/s  |        |         ||");

	//for(; it != mapLoadingTimeContainers.end(); ++it)
	//{
	//	const char * szName = it->first;
	//	const SLoadingTimeContainer * pTimeContainer = it->second;

	//	// call stack level
	//	char szNameNoStack[LOADING_TIME_CONTAINER_MAX_TEXT_SIZE]="";
	//	for(char * pTmp = (char *)szName; pTmp && pTmp<(szName+strlen(szName));)
	//	{
	//		pTmp = strstr(pTmp+2,">>");
	//		if(pTmp)
	//			strcat(szNameNoStack,". ");
	//	}

	//	strcat(szNameNoStack,pTimeContainer->m_pFuncName);

	//	double bandwidth = pTimeContainer->m_dSelfTime > 0 ? (pTimeContainer->m_selfInfo.m_dOperationSize / pTimeContainer->m_dSelfTime / 1024.0 ) : 0.;

	//	pLog->Log("|%6d | %6d | %6d |%6d | %6d | %6d | %6.1f | %6d | %s|", 
	//		pTimeContainer->m_selfInfo.m_nSeeksCount, pTimeContainer->m_selfInfo.m_nFileOpenCount, pTimeContainer->m_selfInfo.m_nFileReadCount,
	//		pTimeContainer->m_totalInfo.m_nSeeksCount, pTimeContainer->m_totalInfo.m_nFileOpenCount, pTimeContainer->m_totalInfo.m_nFileReadCount,
	//		bandwidth, (int)pTimeContainer->m_nCounter, szNameNoStack);
	//}

	//pLog->Log("---------------------------------------------------------------------");

	//}

	PodArray<SLoadingTimeContainer> arrNoStack;
	CreateNoStackList(arrNoStack);


	if(nLoadingProfileMode>0)
	{ // loading mem stats per func
		pLog->Log("------ Level loading memory allocations (MB) by function ------------");
		pLog->Log(" ||Self |  Total |  Calls | Function (%d MB lost)||", nLoadingProfilerNotTrackedAllocations);
		pLog->Log("---------------------------------------------------------------------");

		qsort(arrNoStack.GetElements(), arrNoStack.Count(), sizeof(arrNoStack[0]), SLoadingTimeContainer::Cmp_SLoadingTimeContainer_MemUsage);

		for(int i=0; i < arrNoStack.Count(); i++)
		{
			const SLoadingTimeContainer * pTimeContainer = &arrNoStack[i];
			pLog->Log("|%6.1f | %6.1f | %6d | %s|", 
				pTimeContainer->m_dSelfMemUsage, pTimeContainer->m_dTotalMemUsage, (int)pTimeContainer->m_nCounter, pTimeContainer->m_pFuncName);
		}

		pLog->Log("---------------------------------------------------------------------");
	}

	if(nLoadingProfileMode>0)
	{ // loading time stats per func
		pLog->Log("----------- Level loading time (sec) by function --------------------");
		pLog->Log(" ||Self |  Total |  Calls | Function||");
		pLog->Log("---------------------------------------------------------------------");

		qsort(arrNoStack.GetElements(), arrNoStack.Count(), sizeof(arrNoStack[0]), SLoadingTimeContainer::Cmp_SLoadingTimeContainer_Time);

		for(int i=0; i < arrNoStack.Count(); i++)
		{
			const SLoadingTimeContainer * pTimeContainer = &arrNoStack[i];
			pLog->Log("|%6.1f | %6.1f | %6d | %s|", 
				pTimeContainer->m_dSelfTime, pTimeContainer->m_dTotalTime, (int)pTimeContainer->m_nCounter, pTimeContainer->m_pFuncName);
		}

		if(nLoadingProfileMode==1)
			pLog->Log("----- ( Use sys_ProfileLevelLoading 2 for more detailed stats ) -----");
		else
			pLog->Log("---------------------------------------------------------------------");
	}

	if(nLoadingProfileMode>0)
	{ // file info
		pLog->Log("----------------------------- Level file information by function --------------------------------");
		pLog->Log("||           Self          |           Total         |Bandwith|  Calls | Function||");
		pLog->Log("|| Seeks |FileOpen|FileRead| Seeks |FileOpen|FileRead|  Kb/s  |        |         ||");

		qsort(arrNoStack.GetElements(), arrNoStack.Count(), sizeof(arrNoStack[0]), SLoadingTimeContainer::Cmp_SLoadingTimeContainer_Time);

		for(int i=0; i < arrNoStack.Count(); i++)
		{

			const SLoadingTimeContainer * pTimeContainer = &arrNoStack[i];
			double bandwidth = pTimeContainer->m_dSelfTime > 0 ? (pTimeContainer->m_selfInfo.m_dOperationSize / pTimeContainer->m_dSelfTime / 1024.0 ) : 0.;
			pLog->Log("|%6d | %6d | %6d |%6d | %6d | %6d | %6.1f | %6d | %s|", 
				pTimeContainer->m_selfInfo.m_nSeeksCount, pTimeContainer->m_selfInfo.m_nFileOpenCount, pTimeContainer->m_selfInfo.m_nFileReadCount,
				pTimeContainer->m_totalInfo.m_nSeeksCount, pTimeContainer->m_totalInfo.m_nFileOpenCount, pTimeContainer->m_totalInfo.m_nFileReadCount,
				bandwidth, (int)pTimeContainer->m_nCounter, pTimeContainer->m_pFuncName);
		}

		if(nLoadingProfileMode==1)
			pLog->Log("----- ( Use sys_ProfileLevelLoading 2 for more detailed stats ) -----");
		else
			pLog->Log("---------------------------------------------------------------------");
	}

}


struct CSystemEventListner_LoadingProfiler : public ISystemEventListener
{
private:
	CLoadingTimeProfiler * m_pPrecacheProfiler;
	ESystemEvent lastEvent;
public:
	virtual void OnSystemEvent( ESystemEvent event,UINT_PTR wparam,UINT_PTR lparam )
	{
		switch (event)
		{

		case ESYSTEM_EVENT_LEVEL_LOAD_START_BEGIN:
			{
				CLoadingProfilerSystem::Clean();
  			m_pPrecacheProfiler = new CLoadingTimeProfiler(gEnv->pSystem, "LevelLoading");
				break;
			}

		case ESYSTEM_EVENT_LEVEL_LOAD_END:
			{
			  delete m_pPrecacheProfiler;
				m_pPrecacheProfiler = new CLoadingTimeProfiler(gEnv->pSystem, "Precache");
				break;
			}


//Diesel new 3203
///*
			case ESYSTEM_EVENT_LEVEL_PRECACHE_END:
			{
				if (lastEvent == ESYSTEM_EVENT_LEVEL_PRECACHE_START) {
					delete m_pPrecacheProfiler;
					m_pPrecacheProfiler = 0;
					string levelName = "no_level";
					ICVar *sv_map = gEnv->pConsole->GetCVar("sv_map");
					if (sv_map)
						levelName = sv_map->GetString();

					levelName += "_loadingProfiler.xml";
					CLoadingProfilerSystem::SaveTimeContainersToFile(levelName.c_str());
				}
				break;
			}
//*/
//Diesel new 3203




/* //Diesel cut
		case ESYSTEM_EVENT_LEVEL_PRECACHE_END:
			{
				if (lastEvent == ESYSTEM_EVENT_LEVEL_LOAD_END) {
					delete m_pPrecacheProfiler;
					m_pPrecacheProfiler = 0;
					string levelName = "no_level";
					ICVar *sv_map = gEnv->pConsole->GetCVar("sv_map");
					if (sv_map)
						levelName = sv_map->GetString();

					levelName += "_loadingProfiler.xml";
					CLoadingProfilerSystem::SaveTimeContainersToFile(levelName.c_str());
				}
				break;
			}
*/ //Diesel cut

		}

		lastEvent = event;
	}
};

static CSystemEventListner_LoadingProfiler g_system_event_listener_loadingProfiler;

void CLoadingProfilerSystem::Init()
{
	gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_loadingProfiler);
}

SLoadingTimeContainer * CLoadingProfilerSystem::StartLoadingSectionProfiling(CLoadingTimeProfiler * pProfiler, const char * szFuncName)
{
	if(!nLoadingProfileMode)
		return NULL;

	static DWORD dwMainThreadId = GetCurrentThreadId();
	if( GetCurrentThreadId() != dwMainThreadId )
		return NULL;

	if (!m_pEnableProfile) {
		if (gEnv->pConsole) {
			m_pEnableProfile = gEnv->pConsole->GetCVar("sys_ProfileLevelLoading");
		if (!m_pEnableProfile) 
			return 0;
		}
		else
			return 0;
	}

	if (m_pEnableProfile->GetIVal() <= 0)
		return 0;

	//if (m_pCurrentLoadingTimeContainer == m_pRoot && strstr(szFuncName,"Open"))
	//{
	//	pProfiler->m_constructorInfo.m_nFileOpenCount +=1; 
	//}

	CryAutoCriticalSection lock(csLock);

	if (true/*pProfiler && pProfiler->m_pSystem*/) {

		ITimer * pTimer = pProfiler->m_pSystem->GetITimer();
		pProfiler->m_fConstructorTime = pTimer->GetAsyncTime().GetSeconds();
		pProfiler->m_fConstructorMemUsage = SLoadingTimeContainer::GetUsedMemory(pProfiler->m_pSystem);

		DiskOperationInfo info;
		if (gEnv->pSystem->GetIDiskProfiler())
			info = gEnv->pSystem->GetIDiskProfiler()->GetStatistics();

		pProfiler->m_constructorInfo = info;

		if(nLoadingProfilerNotTrackedAllocations<0)
			nLoadingProfilerNotTrackedAllocations = (int)pProfiler->m_fConstructorMemUsage;
	}

	SLoadingTimeContainer * pParent = m_pCurrentLoadingTimeContainer;
	if (!pParent) {
		pParent = m_pCurrentLoadingTimeContainer = m_pRoot = new SLoadingTimeContainer(0, "Root"); 
	}

	for (size_t i = 0, end = m_pCurrentLoadingTimeContainer->m_pChilds.size(); i < end; ++i) {
		if (m_pCurrentLoadingTimeContainer->m_pChilds[i]->m_pFuncName == szFuncName)
		{
			assert(m_pCurrentLoadingTimeContainer->m_pChilds[i]->m_pParent == m_pCurrentLoadingTimeContainer);
			assert(!m_pCurrentLoadingTimeContainer->m_pChilds[i]->m_bUsed);
			m_pCurrentLoadingTimeContainer->m_pChilds[i]->m_bUsed = true;
			m_pCurrentLoadingTimeContainer->m_pChilds[i]->m_nCounter++;
			m_pCurrentLoadingTimeContainer = m_pCurrentLoadingTimeContainer->m_pChilds[i];
			return m_pCurrentLoadingTimeContainer;
		}
	}

	m_pCurrentLoadingTimeContainer = new SLoadingTimeContainer(pParent, szFuncName);
	m_pCurrentLoadingTimeContainer->m_bUsed = true;
	{
		// Need to iterate from the end than	
		pParent->m_pChilds.push_back(m_pCurrentLoadingTimeContainer);
	}

	return m_pCurrentLoadingTimeContainer;
}

void CLoadingProfilerSystem::EndLoadingSectionProfiling(CLoadingTimeProfiler * pProfiler)
{
	if(!nLoadingProfileMode)
		return;

	static DWORD dwMainThreadId = GetCurrentThreadId();
	if( GetCurrentThreadId() != dwMainThreadId )
		return;

	if (!pProfiler->m_pTimeContainer)
		return;

	CryAutoCriticalSection lock(csLock);

	if (true/*pProfiler && pProfiler->m_pSystem*/) {

		ITimer * pTimer = pProfiler->m_pSystem->GetITimer();
		double fSelfTime = pTimer->GetAsyncTime().GetSeconds() - pProfiler->m_fConstructorTime;
		double fMemUsage = SLoadingTimeContainer::GetUsedMemory(pProfiler->m_pSystem);
		double fSelfMemUsage = fMemUsage - pProfiler->m_fConstructorMemUsage;


		if (fSelfTime < 0.0)
			assert(0);
		pProfiler->m_pTimeContainer->m_dSelfTime += fSelfTime;
		pProfiler->m_pTimeContainer->m_dTotalTime += fSelfTime;
		pProfiler->m_pTimeContainer->m_dSelfMemUsage += fSelfMemUsage;
		pProfiler->m_pTimeContainer->m_dTotalMemUsage += fSelfMemUsage;

		DiskOperationInfo info;
		if (gEnv->pSystem->GetIDiskProfiler())
			info = gEnv->pSystem->GetIDiskProfiler()->GetStatistics();

		info -= pProfiler->m_constructorInfo;
		pProfiler->m_pTimeContainer->m_totalInfo += info;
		pProfiler->m_pTimeContainer->m_selfInfo += info;
		pProfiler->m_pTimeContainer->m_bUsed = false;

		SLoadingTimeContainer * pParent = pProfiler->m_pTimeContainer->m_pParent;
		pParent->m_selfInfo -= info;
		pParent->m_dSelfTime -= fSelfTime;
		pParent->m_dSelfMemUsage -= fSelfMemUsage;
		m_pCurrentLoadingTimeContainer = pProfiler->m_pTimeContainer->m_pParent;
	}
}

const char* CLoadingProfilerSystem::GetLoadingProfilerCallstack()
{
	CryAutoCriticalSection lock(csLock);
	if (m_pCurrentLoadingTimeContainer)
		return m_pCurrentLoadingTimeContainer->m_pFuncName;
	return "";
}

void CLoadingProfilerSystem::FillProfilersList(std::vector<SLoadingProfilerInfo>& profilers)
{
	if (m_pRoot) {
		//if (withStack) 
		//{
		//	uint32 count = oldLoadingTimeContainers.size();
		//	profilers.resize(count);

		//	int i = 0;
		//	LoadingTimeContainersMap::iterator it = oldLoadingTimeContainers.begin();
		//	for(; it != oldLoadingTimeContainers.end(); ++it)
		//	{
		//		const char * szName = it->first;
		//		const SLoadingTimeContainer * pTimeContainer = it->second;

		//		// call stack level
		//		char szNameStack[LOADING_TIME_CONTAINER_MAX_TEXT_SIZE]="";
		//		for(char * pTmp = (char *)szName; pTmp && pTmp<(szName+strlen(szName));)
		//		{
		//			pTmp = strstr(pTmp+2,">>");
		//			if(pTmp)
		//				strcat(szNameStack,". ");
		//		}
		//		strcat(szNameStack,pTimeContainer->m_pFuncName);

		//		profilers[i].name = szNameStack;
		//		profilers[i].selfTime = pTimeContainer->m_dSelfTime;
		//		profilers[i].callsTotal = pTimeContainer->m_nCounter;
		//		profilers[i].totalTime = pTimeContainer->m_dTotalTime;
		//		profilers[i].memorySize = pTimeContainer->m_dTotalMemUsage;
		//		profilers[i].selfInfo = pTimeContainer->m_selfInfo;
		//		profilers[i].totalInfo = pTimeContainer->m_totalInfo;
		//		++i;
		//	}
		//}
		//else 
		//	{

		UpdateSelfStatistics(m_pRoot);

		PodArray<SLoadingTimeContainer> arrNoStack;
		CreateNoStackList(arrNoStack);
		//qsort(arrNoStack.GetElements(), arrNoStack.Count(), sizeof(arrNoStack[0]), SLoadingTimeContainer::Cmp_SLoadingTimeContainer_Time);

		uint32 count = arrNoStack.Size();
		profilers.resize(count);

		for (uint32 i = 0; i < count; ++i) {
			profilers[i].name = arrNoStack[i].m_pFuncName;
			profilers[i].selfTime = arrNoStack[i].m_dSelfTime;
			profilers[i].callsTotal = arrNoStack[i].m_nCounter;
			profilers[i].totalTime = arrNoStack[i].m_dTotalTime;
			profilers[i].memorySize = arrNoStack[i].m_dTotalMemUsage;
			profilers[i].selfInfo = arrNoStack[i].m_selfInfo;
			profilers[i].totalInfo = arrNoStack[i].m_totalInfo;
		}
	}
}


void CLoadingProfilerSystem::AddTimeContainerFunction(PodArray<SLoadingTimeContainer>& arrNoStack, SLoadingTimeContainer * node)
{
	SLoadingTimeContainer * it = std::find(arrNoStack.begin(), arrNoStack.end(), node->m_pFuncName);

	if (it ==  arrNoStack.end()) {
		arrNoStack.push_back(*node);
	} 
	else {
		it->m_dSelfMemUsage += node->m_dSelfMemUsage;
		it->m_dSelfTime += node->m_dSelfTime;
		it->m_dTotalMemUsage += node->m_dTotalMemUsage;
		it->m_dTotalTime += node->m_dTotalTime;
		it->m_nCounter += node->m_nCounter;
		it->m_selfInfo += node->m_selfInfo;
		it->m_totalInfo += node->m_totalInfo;
	}

	for (size_t i = 0, end = node->m_pChilds.size(); i < end; ++i) {
		AddTimeContainerFunction(arrNoStack, node->m_pChilds[i]);
	}

}

void CLoadingProfilerSystem::CreateNoStackList(PodArray<SLoadingTimeContainer>& arrNoStack) 
{
	AddTimeContainerFunction(arrNoStack, m_pRoot);
}








#define g_szTestResults "TestResults"



void CLoadingProfilerSystem::SaveTimeContainersToFile(const char * name) 
{

	if (m_pRoot) {

		FILE *f;






		char path[ICryPak::g_nMaxPath];
		path[sizeof(path) - 1] = 0;
		gEnv->pCryPak->AdjustFileName(string(string(g_szTestResults) + "\\" + name).c_str(), path, ICryPak::FLAGS_PATH_REAL | ICryPak::FLAGS_FOR_WRITING);

		gEnv->pCryPak->MakeDir( g_szTestResults );
		f = fopen(path ,"wb" );

		if (f) {
			UpdateSelfStatistics(m_pRoot);
			WriteTimeContainerToFile(m_pRoot, f);
			fclose(f);
		}

		delete m_pRoot;

		m_pRoot = m_pCurrentLoadingTimeContainer = 0;
	}

}

void CLoadingProfilerSystem::WriteTimeContainerToFile(SLoadingTimeContainer * p, FILE* f)
{


	CryFixedStringT<128> str(p->m_pFuncName);
	str.replace(':','_');
	fprintf(f, "<%s selfTime='%f' selfMemory='%f' totalTime='%f' totalMemory='%f' count='%i' totalSeeks='%i' totalReads='%i' totalOpens='%i' totalDiskSize='%f' selfSeeks='%i' selfReads='%i' selfOpens='%i' selfDiskSize='%f'>\n", 
		str.c_str(), p->m_dSelfTime, p->m_dSelfMemUsage, p->m_dTotalTime, p->m_dTotalMemUsage, p->m_nCounter,
		p->m_totalInfo.m_nSeeksCount, p->m_totalInfo.m_nFileReadCount, p->m_totalInfo.m_nFileOpenCount, p->m_totalInfo.m_dOperationSize, 
		p->m_selfInfo.m_nSeeksCount, p->m_selfInfo.m_nFileReadCount, p->m_selfInfo.m_nFileOpenCount, p->m_selfInfo.m_dOperationSize);

	for (size_t i = 0, end = p->m_pChilds.size(); i < end; ++i)
	{
		WriteTimeContainerToFile(p->m_pChilds[i], f);
	}

	fprintf(f, "</%s>\n", str.c_str());
}

void CLoadingProfilerSystem::UpdateSelfStatistics(SLoadingTimeContainer * p)
{
	p->m_dSelfMemUsage = 0;
	p->m_dSelfTime = 0;
	p->m_nCounter = 1;
	p->m_selfInfo.m_dOperationSize = 0;
	p->m_selfInfo.m_nFileOpenCount = 0;
	p->m_selfInfo.m_nFileReadCount = 0;
	p->m_selfInfo.m_nSeeksCount = 0;

	for (size_t i= 0, end = p->m_pChilds.size(); i < end; ++i) {
		p->m_dTotalMemUsage += p->m_pChilds[i]->m_dTotalMemUsage;
		p->m_dTotalTime += p->m_pChilds[i]->m_dTotalTime;
		p->m_totalInfo += p->m_pChilds[i]->m_totalInfo;
	}
}

void CLoadingProfilerSystem::Clean()
{
	if (m_pRoot)
		delete m_pRoot;
	m_pCurrentLoadingTimeContainer = m_pRoot = 0;
}