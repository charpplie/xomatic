#ifndef LOADINGPROFILERSYSTEM_H
#define LOADINGPROFILERSYSTEM_H

#define MAX_LOADING_TIME_PROFILER_STACK_DEPTH 16
struct SLoadingTimeContainer;


struct SLoadingProfilerInfo
{
	string name;
	double selfTime;
	double totalTime;
	uint32 callsTotal;
	double memorySize; 

	DiskOperationInfo selfInfo;
	DiskOperationInfo totalInfo;
};


class CLoadingProfilerSystem
{
public:
	static void Init();
	static void CreateNoStackList(PodArray<SLoadingTimeContainer>&);
	static void OutputLoadingTimeStats(ILog * pLog, int nMode);
	static SLoadingTimeContainer * StartLoadingSectionProfiling(CLoadingTimeProfiler * pProfiler, const char * szFuncName);
	static void EndLoadingSectionProfiling(CLoadingTimeProfiler * pProfiler);
	static const char* GetLoadingProfilerCallstack();
	static void FillProfilersList(std::vector<SLoadingProfilerInfo>& profilers);
	static void FlushTimeContainers();
	static void SaveTimeContainersToFile(const char *);
	static void WriteTimeContainerToFile(SLoadingTimeContainer * p, FILE* f);
	static void UpdateSelfStatistics(SLoadingTimeContainer * p);
	static void Clean();
protected:
	static void AddTimeContainerFunction(PodArray<SLoadingTimeContainer>&, SLoadingTimeContainer *) ;
protected:
	static int nLoadingProfileMode;
	static int nLoadingProfilerNotTrackedAllocations;
	static CryCriticalSection csLock;
	static int m_iMaxArraySize;
	static SLoadingTimeContainer * m_pCurrentLoadingTimeContainer;
	static SLoadingTimeContainer * m_pRoot;
  static ICVar * m_pEnableProfile;
};

#endif // LOADINGPROFILERSYSTEM_H