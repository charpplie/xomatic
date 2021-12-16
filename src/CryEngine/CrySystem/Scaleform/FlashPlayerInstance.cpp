#include "StdAfx.h"

#include "FlashPlayerInstance.h"
#include "../System.h"
#include <IConsole.h>

// flash player implementation via Scaleform's GFx
#ifndef EXCLUDE_SCALEFORM_SDK

#include "GImageInfoXRender.h"
#include "GTextureXRender.h"
#include "GAllocatorCryMem.h"
#include "SharedStates.h"

//////////////////////////////////////////////////////////////////////////
// IFlashPlayer instance profiler helpers

#if defined(ENABLE_FLASH_INFO)
struct SFlashPeak
{
	SFlashPeak(float timeRecorded, const char* pDisplayInfo)
	: m_timeRecorded(timeRecorded)
	, m_displayInfo(pDisplayInfo)
	{
	}

	float m_timeRecorded;
	string m_displayInfo;
};


class CFlashPeakHistory
{
public:
	typedef std::vector<SFlashPeak> FlashPeakHistory;

public:
	CFlashPeakHistory(int maxPeakEntries, float timePeakExpires)
	: m_writeProtected(false)
	, mc_maxPeakEntries(maxPeakEntries)
	, mc_timePeakExpires(timePeakExpires)
	, m_history()
	{
		m_history.reserve(mc_maxPeakEntries);
	}

	void Add(const SFlashPeak& peak)
	{
		if (!m_writeProtected)
		{
			if ((int)m_history.size() >= mc_maxPeakEntries)
				m_history.pop_back();

			m_history.insert(m_history.begin(), peak);
		}
	}

	void ToggleWriteProtection()
	{
		m_writeProtected = !m_writeProtected;
	}

	void UpdateHistory(float curTime)
	{
		FlashPeakHistory::iterator it(m_history.begin());
		for (; it != m_history.end();)
		{
			const SFlashPeak& peak(*it);
			if (curTime - peak.m_timeRecorded > mc_timePeakExpires && !m_writeProtected)
				it = m_history.erase(it);
			else
				++it;
		}
	}

	float GetTimePeakExpires() const
	{
		return mc_timePeakExpires;
	}

	const FlashPeakHistory& GetHistory() const
	{
		return m_history;
	}

private:
	bool m_writeProtected;
	const int mc_maxPeakEntries;
	const float mc_timePeakExpires;
	FlashPeakHistory m_history;
};

static CFlashPeakHistory s_flashPeakHistory(10, 5.0f);


enum EFlashFunctionID
{
	eFncAdvance,
	eFncDisplay,
	eFncSetVar,
	eFncGetVar,
	eFncIsAvailable,
	eFncInvoke,

	eNumFncIDs
};


inline static const char* GetFlashFunctionName(EFlashFunctionID functionID)
{
	switch(functionID)
	{
	case eFncAdvance:
		return "Advance";
	case eFncDisplay:
		return "Display";
	case eFncSetVar:
		return "SetVar";
	case eFncGetVar:
		return "GetVar";
	case eFncIsAvailable:
		return "IsAvailable";
	case eFncInvoke:
		return "Invoke";
	default:
		return "Unknown";
	}
}


template <typename T, int HistogramSize>
struct SFlashProfilerHistogram
{
	SFlashProfilerHistogram()
	: m_curIdx(0)
	{
		for (int i(0); i<HistogramSize; ++i)
			m_hist[i] = 0;
	}

	void Advance()
	{
		m_curIdx = (m_curIdx + 1) % HistogramSize;
		m_hist[m_curIdx] = 0;
	}

	void Add(T amount)
	{
		m_hist[m_curIdx] += amount;
	}

	T GetAvg() const
	{
		T sum(0);
		for (int i(0); i<HistogramSize; ++i)
			sum += m_hist[i];

		return sum / (T) HistogramSize;
	}

	T GetMin() const
	{
		T min(m_hist[0]);
		for (int i(1); i<HistogramSize; ++i)
			min = m_hist[i] < min ? m_hist[i] : min;

		return min;
	}

	T GetMax() const
	{
		T max(m_hist[0]);
		for (int i(1); i<HistogramSize; ++i)
			max = m_hist[i] > max ? m_hist[i] : max;

		return max;
	}

	T GetCur() const
	{
		return m_hist[m_curIdx];
	}

	T GetAt(int idx) const
	{
		return m_hist[idx];
	}

	int GetCurIdx() const
	{
		return m_curIdx;
	}

private:
	int m_curIdx;
	T m_hist[HistogramSize];
};


struct SFlashProfilerData
{
	enum
	{
		HistogramSize = 64
	};

	typedef SFlashProfilerHistogram<int, HistogramSize> TotalCallHistogram;
	typedef SFlashProfilerHistogram<float, HistogramSize> TotalTimeHistogram;

	SFlashProfilerData()
	: m_viewExpanded(false)
	, m_preventFunctionExectution(false)
	, m_totalCalls()
	, m_totalTime()
	, m_totalTimeAllFuncs()
	{
		for (int i(0); i<eNumFncIDs+1; ++i)
		{
			m_funcColorValue[i] = 0;
			m_funcColorValueVariance[i] = 0;
		}
	}

	void Advance()
	{
		if (!ms_histWriteProtected)
		{
			for (int i(0); i<eNumFncIDs; ++i)
			{
				m_totalCalls[i].Advance();
				m_totalTime[i].Advance();
			}
			m_totalTimeAllFuncs.Advance();
		}
	}

	static void AdvanceAllInsts()
	{
		if (!ms_histWriteProtected)
			ms_totalTimeAllInsts.Advance();
	}

	void AddTotalCalls(EFlashFunctionID functionID, int amount)
	{
		if (!ms_histWriteProtected)
			m_totalCalls[functionID].Add(amount);
	}

	void AddTotalTime(EFlashFunctionID functionID, float amount)
	{
		if (!ms_histWriteProtected)
		{
			m_totalTime[functionID].Add(amount);
			m_totalTimeAllFuncs.Add(amount);
			ms_totalTimeAllInsts.Add(amount);
		}
	}

	const TotalCallHistogram& GetTotalCallsHisto(EFlashFunctionID functionID) const
	{
		return m_totalCalls[functionID];
	}

	const TotalTimeHistogram& GetTotalTimeHisto(EFlashFunctionID functionID) const
	{
		return m_totalTime[functionID];
	}

	const TotalTimeHistogram& GetTotalTimeHistoAllFuncs() const
	{
		return m_totalTimeAllFuncs;
	}

	static const TotalTimeHistogram& GetTotalTimeHistoAllInsts()
	{
		return ms_totalTimeAllInsts;
	}

	void SetViewExpanded(bool expanded)
	{
		m_viewExpanded = expanded;
	}

	bool IsViewExpanded() const
	{
		return m_viewExpanded;
	}

	void TogglePreventFunctionExecution()
	{
		m_preventFunctionExectution = !m_preventFunctionExectution;
	}

	bool PreventFunctionExectution() const
	{
		return m_preventFunctionExectution;
	}

	void SetColorValue(EFlashFunctionID functionID, float value)
	{
		if (!ms_histWriteProtected)
			m_funcColorValue[functionID] = value;
	}

	float GetColorValue(EFlashFunctionID functionID) const
	{
		return m_funcColorValue[functionID];
	}

	void SetColorValueVariance(EFlashFunctionID functionID, float value)
	{
		if (!ms_histWriteProtected)
			m_funcColorValueVariance[functionID] = value;
	}

	float GetColorValueVariance(EFlashFunctionID functionID) const
	{
		return m_funcColorValueVariance[functionID];
	}

	static void ToggleHistogramWriteProtection()
	{
		ms_histWriteProtected = !ms_histWriteProtected;
	}

	static bool HistogramWriteProtected()
	{
		return ms_histWriteProtected;
	}

private:
	static bool ms_histWriteProtected;
	static TotalTimeHistogram ms_totalTimeAllInsts;

private:
	bool m_viewExpanded;
	bool m_preventFunctionExectution;
	float m_funcColorValue[eNumFncIDs+1];
	float m_funcColorValueVariance[eNumFncIDs+1];
	TotalCallHistogram m_totalCalls[eNumFncIDs];
	TotalTimeHistogram m_totalTime[eNumFncIDs];
	TotalTimeHistogram m_totalTimeAllFuncs;
};

bool SFlashProfilerData::ms_histWriteProtected(false);
SFlashProfilerData::TotalTimeHistogram SFlashProfilerData::ms_totalTimeAllInsts;


struct SPODVariant
{
	struct SFlashVarValueList
	{
		const SFlashVarValue* pArgs;
		unsigned int numArgs;
	};

	union Data
	{
		bool b;

		int i;
		unsigned int ui;

		double d;
		float f;

		const char* pStr;
		const wchar_t* pWstr;
		const void*	pVoid;

		EFlashVariableArrayType fvat;

		const SFlashVarValue* pfvv;
		SFlashVarValueList fvvl;
	};

	enum Type
	{
		eBool,

		eInt,
		eUInt,

		eDouble,
		eFloat,

		eConstStrPtr,
		eConstWstrPtr,
		eConstVoidPtr,

		eFlashVariableArrayType,

		eFlashVarValuePtr,
		eFlashVarValueList
	};

	SPODVariant(bool val)
		: type(eBool)
	{
		data.b = val;
	}
	SPODVariant(int val)
	: type(eInt)
	{
		data.i = val;
	}
	SPODVariant(unsigned int val)
		: type(eUInt)
	{
		data.ui = val;
	}
	SPODVariant(double val)
	: type(eDouble)
	{
		data.d = val;
	}
	SPODVariant(float val)
		: type(eFloat)
	{
		data.f = val;
	}
	SPODVariant(const char* val)
	: type(eConstStrPtr)
	{
		data.pStr = val;
	}
	SPODVariant(const wchar_t* val)
	: type(eConstWstrPtr)
	{
		data.pWstr = val;
	}
	SPODVariant(const void* val)
	: type(eConstVoidPtr)
	{
		data.pVoid = val;
	}
	SPODVariant(const EFlashVariableArrayType& val)
	: type(eFlashVariableArrayType)
	{
		data.fvat = val;
	}
	SPODVariant(const SFlashVarValue* val)
		: type(eFlashVarValuePtr)
	{
		data.pfvv = val;
	}
	SPODVariant(const SFlashVarValueList& val)
		: type(eFlashVarValueList)
	{
		data.fvvl = val;
	}

	Type type;
	Data data;
};


class CFlashFunctionProfilerLog
{
public:
	static CFlashFunctionProfilerLog& GetAccess()
	{
		static CFlashFunctionProfilerLog s_instance;
		return s_instance;
	}

	void Enable(bool enabled, int curFrame)
	{
		if (!enabled && m_file)
		{
			fclose(m_file);
			m_file = 0;
		}
		else if (enabled && !m_file)
		{
			m_file = fopen("flash.log", "wb");
		}
		m_enabled = enabled;

		if (m_curFrame != curFrame && m_file)
		{
			char frameMsg[512]; frameMsg[511] = '\0';
			sprintf_s(frameMsg, sizeof(frameMsg) - 1, ">>>>>>>> Frame: %.10d <<<<<<<<", curFrame);
			fwrite("\n", 1, 1, m_file);
			fwrite(frameMsg, 1, strlen(frameMsg), m_file);
			fwrite("\n\n", 1, 2, m_file);
		}
		m_curFrame = curFrame;
	}

	bool IsEnabled() const
	{
		return m_enabled;
	}

	void Log(const char* pMsg)
	{
		if (m_file && pMsg)
		{
			fwrite(pMsg, 1, strlen(pMsg), m_file);
			fwrite("\n", 1, 1, m_file);
		}
	}

private:
	CFlashFunctionProfilerLog()
		: m_enabled(false)
		, m_curFrame(0)
		, m_file(0)
	{
	}

	~CFlashFunctionProfilerLog()
	{
	}

	bool m_enabled;
	unsigned int m_curFrame;
	FILE* m_file;
};


class CFlashFunctionProfiler
{
public:
	CFlashFunctionProfiler(float peakTolerance, EFlashFunctionID functionID, int numFuncArgs, const SPODVariant* pFuncArgs, 
		const char* pCustomFuncName, SFlashProfilerData* pProfilerData, const char* pFlashFilePath)
	: m_funcTime()
	, m_peakTolerance(peakTolerance)
	, m_functionID(functionID)
	, m_numFuncArgs(numFuncArgs)
	, m_pFuncArgs(pFuncArgs)
	, m_pCustomFuncName(pCustomFuncName)
	, m_pProfilerData(pProfilerData)
	, m_pFlashFilePath(pFlashFilePath)
	{
	}

	void ProfileEnter()
	{
		m_funcTime = gEnv->pTimer->GetAsyncTime();
	}

	void ProfilerLeave()
	{
		m_funcTime = gEnv->pTimer->GetAsyncTime() - m_funcTime;

		float prevTotalTimeMs(m_pProfilerData->GetTotalTimeHisto(m_functionID).GetCur());
		float curTotalTimeMs(m_funcTime.GetMilliSeconds());

		m_pProfilerData->AddTotalCalls(m_functionID, 1);
		m_pProfilerData->AddTotalTime(m_functionID, curTotalTimeMs);

		bool isPeak(curTotalTimeMs - prevTotalTimeMs > m_peakTolerance && (ms_peakFuncExcludeMask & (1 << m_functionID)) == 0);
		bool reqLog(CFlashFunctionProfilerLog::GetAccess().IsEnabled());
		if (isPeak || reqLog)
		{
			char msg[2048];
			FormatMsg(curTotalTimeMs, msg, sizeof(msg));

			if (isPeak)
				s_flashPeakHistory.Add(SFlashPeak(gEnv->pTimer->GetAsyncCurTime(), msg));

			if (reqLog)
				CFlashFunctionProfilerLog::GetAccess().Log(msg);
		}
	}

private:
	void FormatFlashVarValue(const SFlashVarValue* pArg, char* pBuf, size_t sizeBuf) const
	{
		switch (pArg->GetType())
		{
		case SFlashVarValue::eBool:
			sprintf_s(pBuf, sizeBuf, "%s", pArg->GetBool() ? "true" : "false");
			break;
		case SFlashVarValue::eInt:
			sprintf_s(pBuf, sizeBuf, "%d", pArg->GetInt());
			break;
		case SFlashVarValue::eUInt:
			sprintf_s(pBuf, sizeBuf, "%u", pArg->GetUInt());
			break;
		case SFlashVarValue::eDouble:
			sprintf_s(pBuf, sizeBuf, "%lf", pArg->GetDouble());
			break;
		case SFlashVarValue::eFloat:
			sprintf_s(pBuf, sizeBuf, "%f", pArg->GetFloat());
			break;
		case SFlashVarValue::eConstStrPtr:
			sprintf_s(pBuf, sizeBuf, "\"%s\"", pArg->GetConstStrPtr());
			break;
		case SFlashVarValue::eConstWstrPtr:
			sprintf_s(pBuf, sizeBuf, "\"%ls\"", pArg->GetConstWstrPtr());
			break;
		case SFlashVarValue::eNull:
			sprintf_s(pBuf, sizeBuf, "null");
			break;
		case SFlashVarValue::eObject:
			sprintf_s(pBuf, sizeBuf, "obj");
			break;
		default:
			assert(0);
		case SFlashVarValue::eUndefined:
			sprintf_s(pBuf, sizeBuf, "???");
			break;
		}
	}
	void FormatMsg(float funcTimeMs, char* msgBuf, int sizeMsgBuf) const
	{
		char funcArgList[1024]; funcArgList[0] = '\0';
		{
			size_t pos(0);
			for (int i(0); i<m_numFuncArgs; ++i)
			{
				char curArg[512]; curArg[511] = '\0';
				switch(m_pFuncArgs[i].type)
				{
				case SPODVariant::eBool:
					{
						sprintf_s(curArg, sizeof(curArg) - 1, "%s", m_pFuncArgs[i].data.b ? "true" : "false");
						break;
					}
				case SPODVariant::eInt:
					{
						sprintf_s(curArg, sizeof(curArg) - 1, "%d", m_pFuncArgs[i].data.i);
						break;
					}
				case SPODVariant::eUInt:
					{
						sprintf_s(curArg, sizeof(curArg) - 1, "%u", m_pFuncArgs[i].data.ui);
						break;
					}
				case SPODVariant::eDouble:
					{
						sprintf_s(curArg, sizeof(curArg) - 1, "%lf", m_pFuncArgs[i].data.d);
						break;
					}
				case SPODVariant::eFloat:
					{
						sprintf_s(curArg, sizeof(curArg) - 1, "%f", m_pFuncArgs[i].data.f);
						break;
					}
				case SPODVariant::eConstStrPtr:
					{
						sprintf_s(curArg, sizeof(curArg) - 1, "\"%s\"", m_pFuncArgs[i].data.pStr);
						break;
					}
				case SPODVariant::eConstWstrPtr:
					{
						sprintf_s(curArg, sizeof(curArg) - 1, "\"%ls\"", m_pFuncArgs[i].data.pWstr);
						break;
					}
				case SPODVariant::eConstVoidPtr:
					{
						sprintf_s(curArg, sizeof(curArg) - 1, "0x%p", m_pFuncArgs[i].data.pVoid);
						break;
					}
				case SPODVariant::eFlashVariableArrayType:
					{
						const char* pFlashVarTypeName("FVAT_???");
						switch(m_pFuncArgs[i].data.fvat)
						{
						case FVAT_Int:
							pFlashVarTypeName = "FVAT_Int";
							break;
						case FVAT_Double:
							pFlashVarTypeName = "FVAT_Double";
							break;
						case FVAT_Float:
							pFlashVarTypeName = "FVAT_Float";
							break;
						case FVAT_ConstStrPtr:
							pFlashVarTypeName = "FVAT_ConstStrPtr";
							break;
						case FVAT_ConstWstrPtr:
							pFlashVarTypeName = "FVAT_ConstWstrPtr";
							break;
						default:
							assert(0);
							break;
						}
						sprintf_s(curArg, sizeof(curArg) - 1, "%s", pFlashVarTypeName);
						break;
					}
				case SPODVariant::eFlashVarValuePtr:
					{
						FormatFlashVarValue(m_pFuncArgs[i].data.pfvv, curArg, sizeof(curArg));
						break;
					}
				case SPODVariant::eFlashVarValueList:
					{
						curArg[0] = '\0';
						size_t curArgPos(0);

						const SFlashVarValue* pArgs(m_pFuncArgs[i].data.fvvl.pArgs);
						unsigned int numArgs(m_pFuncArgs[i].data.fvvl.numArgs);
						for (unsigned int n(0); n<numArgs; ++n)
						{
							char tmpArg[512]; tmpArg[511] = '\0';
							FormatFlashVarValue(&pArgs[n], tmpArg, sizeof(tmpArg) - 1);

							strncpy(&curArg[curArgPos], tmpArg, sizeof(curArg) - curArgPos - 1);
							curArgPos += strlen(&curArg[curArgPos]);

							if ((n < numArgs - 1) && curArgPos < sizeof(curArg) - 2)
							{
								curArg[curArgPos]   = ',';
								curArg[curArgPos+1] = ' ';
								curArgPos += 2;
							}
						}
						break;
					}
				default:
					{
						curArg[0] = '\0';
						break;
					}
				}

				strncpy(&funcArgList[pos], curArg, sizeof(funcArgList) - pos - 1);
				pos += strlen(&funcArgList[pos]);
				
				if ((i < m_numFuncArgs - 1) && pos < sizeof(funcArgList) - 2)
				{
					funcArgList[pos]   = ',';
					funcArgList[pos+1] = ' ';
					pos += 2;
				}
			}
		}

		msgBuf[sizeMsgBuf - 1] = '\0';
		sprintf_s(msgBuf, sizeMsgBuf - 1, "%.2f %s(%s) - %s", funcTimeMs, m_pCustomFuncName, funcArgList, m_pFlashFilePath);
	}

public:
	static void ResetPeakFuncExcludeMask()
	{
		ms_peakFuncExcludeMask = 0;
	}
	static void AddToPeakFuncExcludeMask(EFlashFunctionID functionID)
	{
		ms_peakFuncExcludeMask |= 1 << functionID;
	}

private:
	static unsigned int ms_peakFuncExcludeMask;

private:
	CTimeValue m_funcTime;
	float m_peakTolerance;
	EFlashFunctionID m_functionID;
	int m_numFuncArgs;
	const SPODVariant* m_pFuncArgs;
	const char* m_pCustomFuncName;
	SFlashProfilerData* m_pProfilerData;
	const char* m_pFlashFilePath;
};

unsigned int CFlashFunctionProfiler::ms_peakFuncExcludeMask(0);


class CFlashFunctionProfilerProxy
{
public:
	CFlashFunctionProfilerProxy(CFlashFunctionProfiler* pProfiler)
	: m_pProfiler(pProfiler)
	{
		if (m_pProfiler)
			m_pProfiler->ProfileEnter();
	}

	~CFlashFunctionProfilerProxy()
	{
		if (m_pProfiler)
			m_pProfiler->ProfilerLeave();
	}

private:
	CFlashFunctionProfiler* m_pProfiler;
};


#define FLASH_PROFILE_FUNC_PRECOND(defRet) \
	if (!IsFlashEnabled() || ms_sys_flash_info && m_pProfilerData && m_pProfilerData->PreventFunctionExectution()) \
		return defRet; 

#define FLASH_PROFILE_FUNC_BUILDPROFILER_BEGIN(numArgs) \
	CFlashFunctionProfiler* pProfiler(0); \
	char memFlashProfiler[sizeof(CFlashFunctionProfiler)]; \
	char memFuncArgs[(numArgs > 0 ? numArgs : 1) * sizeof(SPODVariant)]; \
	if (ms_sys_flash_info) \
	{ \
		int numArgsInit(0); \
		SPODVariant* pArg((SPODVariant*)memFuncArgs); \

#define FLASH_PROFILE_FUNC_BUILDPROFILER_ADD_ARG(arg) \
		new (pArg) SPODVariant(arg); \
		++pArg; \
		++numArgsInit; \

#define FLASH_PROFILE_FUNC_BUILDPROFILER_ADD_VALIST(vargList, numArgs) \
		if (numArgs > 0) \
		{ \
			SPODVariant::SFlashVarValueList val; \
			val.pArgs = vargList; \
			val.numArgs = numArgs; \
			new (pArg) SPODVariant(val); \
			++pArg; \
			++numArgsInit; \
		} \

#define FLASH_PROFILE_FUNC_BUILDPROFILER_END(funcID, funcCustomName) \
		if (!m_pProfilerData) \
			/*(SFlashProfilerData*)*/ m_pProfilerData = new SFlashProfilerData; \
		pProfiler = new (memFlashProfiler) CFlashFunctionProfiler(ms_sys_flash_info_peak_tolerance, funcID, numArgsInit, \
			numArgsInit > 0 ? (SPODVariant*)memFuncArgs : 0, funcCustomName, m_pProfilerData, m_filePath.c_str()); \
	} \
	CFlashFunctionProfilerProxy proxy(pProfiler);


#define VOID_RETURN ((void) 0)

#define FLASH_PROFILE_FUNC(funcID, defRet, funcCustomName) \
	FLASH_PROFILE_FUNC_PRECOND(defRet) \
	FLASH_PROFILE_FUNC_BUILDPROFILER_BEGIN(0) \
	FLASH_PROFILE_FUNC_BUILDPROFILER_END(funcID, funcCustomName)

#define FLASH_PROFILE_FUNC_1ARG(funcID, defRet, funcCustomName, arg0) \
	FLASH_PROFILE_FUNC_PRECOND(defRet) \
	FLASH_PROFILE_FUNC_BUILDPROFILER_BEGIN(1) \
		FLASH_PROFILE_FUNC_BUILDPROFILER_ADD_ARG(arg0) \
	FLASH_PROFILE_FUNC_BUILDPROFILER_END(funcID, funcCustomName)

#define FLASH_PROFILE_FUNC_1ARG_VALIST(funcID, defRet, funcCustomName, arg0, vargList, numArgs) \
	FLASH_PROFILE_FUNC_PRECOND(defRet) \
	FLASH_PROFILE_FUNC_BUILDPROFILER_BEGIN(2) \
		FLASH_PROFILE_FUNC_BUILDPROFILER_ADD_ARG(arg0) \
		FLASH_PROFILE_FUNC_BUILDPROFILER_ADD_VALIST(vargList, numArgs) \
	FLASH_PROFILE_FUNC_BUILDPROFILER_END(funcID, funcCustomName)

#define FLASH_PROFILE_FUNC_2ARG(funcID, defRet, funcCustomName, arg0, arg1) \
	FLASH_PROFILE_FUNC_PRECOND(defRet) \
	FLASH_PROFILE_FUNC_BUILDPROFILER_BEGIN(2) \
		FLASH_PROFILE_FUNC_BUILDPROFILER_ADD_ARG(arg0) \
		FLASH_PROFILE_FUNC_BUILDPROFILER_ADD_ARG(arg1) \
	FLASH_PROFILE_FUNC_BUILDPROFILER_END(funcID, funcCustomName)

#define FLASH_PROFILE_FUNC_3ARG(funcID, defRet, funcCustomName, arg0, arg1, arg2) \
	FLASH_PROFILE_FUNC_PRECOND(defRet) \
	FLASH_PROFILE_FUNC_BUILDPROFILER_BEGIN(3) \
		FLASH_PROFILE_FUNC_BUILDPROFILER_ADD_ARG(arg0) \
		FLASH_PROFILE_FUNC_BUILDPROFILER_ADD_ARG(arg1) \
		FLASH_PROFILE_FUNC_BUILDPROFILER_ADD_ARG(arg2) \
	FLASH_PROFILE_FUNC_BUILDPROFILER_END(funcID, funcCustomName)

#define FLASH_PROFILE_FUNC_4ARG(funcID, defRet, funcCustomName, arg0, arg1, arg2, arg3) \
	FLASH_PROFILE_FUNC_PRECOND(defRet) \
	FLASH_PROFILE_FUNC_BUILDPROFILER_BEGIN(4) \
		FLASH_PROFILE_FUNC_BUILDPROFILER_ADD_ARG(arg0) \
		FLASH_PROFILE_FUNC_BUILDPROFILER_ADD_ARG(arg1) \
		FLASH_PROFILE_FUNC_BUILDPROFILER_ADD_ARG(arg2) \
		FLASH_PROFILE_FUNC_BUILDPROFILER_ADD_ARG(arg3) \
	FLASH_PROFILE_FUNC_BUILDPROFILER_END(funcID, funcCustomName)

#define FLASH_PROFILE_FUNC_5ARG(funcID, defRet, funcCustomName, arg0, arg1, arg2, arg3, arg4) \
	FLASH_PROFILE_FUNC_PRECOND(defRet) \
	FLASH_PROFILE_FUNC_BUILDPROFILER_BEGIN(5) \
		FLASH_PROFILE_FUNC_BUILDPROFILER_ADD_ARG(arg0) \
		FLASH_PROFILE_FUNC_BUILDPROFILER_ADD_ARG(arg1) \
		FLASH_PROFILE_FUNC_BUILDPROFILER_ADD_ARG(arg2) \
		FLASH_PROFILE_FUNC_BUILDPROFILER_ADD_ARG(arg3) \
		FLASH_PROFILE_FUNC_BUILDPROFILER_ADD_ARG(arg4) \
	FLASH_PROFILE_FUNC_BUILDPROFILER_END(funcID, funcCustomName)

#else // #if defined(ENABLE_FLASH_INFO)

#define FLASH_PROFILE_FUNC_PRECOND(defRet)
#define FLASH_PROFILE_FUNC_BUILDPROFILER_BEGIN(numArgs)
#define FLASH_PROFILE_FUNC_BUILDPROFILER_ADD_ARG(arg)
#define FLASH_PROFILE_FUNC_BUILDPROFILER_ADD_VALIST(vargList, numArgs)
#define FLASH_PROFILE_FUNC_BUILDPROFILER_END(funcID, funcCustomName)

#define VOID_RETURN ((void) 0)

#define FLASH_PROFILE_FUNC(funcID, defRet, funcCustomName)
#define FLASH_PROFILE_FUNC_1ARG(funcID, defRet, funcCustomName, arg0)
#define FLASH_PROFILE_FUNC_1ARG_VALIST(funcID, defRet, funcCustomName, arg0, vargList, numArgs)
#define FLASH_PROFILE_FUNC_2ARG(funcID, defRet, funcCustomName, arg0, arg1)
#define FLASH_PROFILE_FUNC_3ARG(funcID, defRet, funcCustomName, arg0, arg1, arg2)
#define FLASH_PROFILE_FUNC_4ARG(funcID, defRet, funcCustomName, arg0, arg1, arg2, arg3)
#define FLASH_PROFILE_FUNC_5ARG(funcID, defRet, funcCustomName, arg0, arg1, arg2, arg3, arg4)

#endif // #if defined(ENABLE_FLASH_INFO)


//////////////////////////////////////////////////////////////////////////
// flash log context

class CFlashLogContext
{
public:
	CFlashLogContext(CryGFxLog* pLog, const char* pFlashContext)
	: m_pLog(pLog)
	, m_pPrevLogContext(0)
	{
		m_pPrevLogContext = pLog->GetContext();
		pLog->SetContext(pFlashContext);
	}

	~CFlashLogContext()
	{
		m_pLog->SetContext(m_pPrevLogContext);
	}

private:
	CryGFxLog* m_pLog;
	const char* m_pPrevLogContext;
};

#define SET_LOG_CONTEXT(filePath) CFlashLogContext logContext(&CryGFxLog::GetAccess(), filePath.c_str());


#define SYNC_THREADS CryAutoCriticalSection theLock(*m_lock.get());

class CReleaseOnExit
{
public:
	CReleaseOnExit(CFlashPlayer* pFlashPlayer, bool releaseOnExit)
	: m_pFlashPlayer(pFlashPlayer)
	, m_releaseOnExit(releaseOnExit)
	{
	}

	~CReleaseOnExit() 
	{
		if (m_releaseOnExit)
			m_pFlashPlayer->Release();
	}

private:
	CFlashPlayer* m_pFlashPlayer;
	bool m_releaseOnExit;
};

#define RELEASE_ON_EXIT(releaseOnExit) CReleaseOnExit relOnExit(this, releaseOnExit);


//////////////////////////////////////////////////////////////////////////
// GFxValue <-> SFlashVarValue translation

static inline GFxValue ConvertValue(const SFlashVarValue& src)
{
	//FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	switch (src.GetType())
	{
	case SFlashVarValue::eBool:
		return GFxValue(src.GetBool());

	case SFlashVarValue::eInt:
		return GFxValue((double)src.GetInt());

	case SFlashVarValue::eUInt:
		return GFxValue((double)src.GetUInt());

	case SFlashVarValue::eDouble:
		return GFxValue(src.GetDouble());

	case SFlashVarValue::eFloat:
		return GFxValue((double)src.GetFloat());

	case SFlashVarValue::eConstStrPtr:
		return GFxValue(src.GetConstStrPtr());

	case SFlashVarValue::eConstWstrPtr:
		return GFxValue(src.GetConstWstrPtr());

	case SFlashVarValue::eNull:
		return GFxValue(GFxValue::VT_Null);

	case SFlashVarValue::eObject:
		assert(0);
	case SFlashVarValue::eUndefined:
	default:
		return GFxValue();
	}
}

static inline SFlashVarValue ConvertValue(const GFxValue& src)
{
	//FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	switch (src.GetType())
	{
	case GFxValue::VT_Boolean:
		return SFlashVarValue(src.GetBool());

	case GFxValue::VT_Number:
		return SFlashVarValue(src.GetNumber());

	case GFxValue::VT_String:
		return SFlashVarValue(src.GetString());

	case GFxValue::VT_StringW:
		return SFlashVarValue(src.GetStringW());

	case GFxValue::VT_Null:
		return SFlashVarValue::CreateNull();

	case GFxValue::VT_Object:
	case GFxValue::VT_Array:
	case GFxValue::VT_DisplayObject:
		{
			struct SFlashVarValueObj : public SFlashVarValue
			{
				SFlashVarValueObj() : SFlashVarValue(eObject) {}
			};
			return SFlashVarValueObj();
		}

	case GFxValue::VT_Undefined:
	default:
		return SFlashVarValue::CreateUndefined();
	}
}

static inline bool GetArrayType(EFlashVariableArrayType type, GFxMovie::SetArrayType& translatedType)
{
	//FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	switch (type)
	{
	case FVAT_Int:
		translatedType = GFxMovie::SA_Int;
		return true;
	case FVAT_Double:
		translatedType = GFxMovie::SA_Double;
		return true;
	case FVAT_Float:
		translatedType = GFxMovie::SA_Float;
		return true;
	case FVAT_ConstStrPtr:
		translatedType = GFxMovie::SA_String;
		return true;
	case FVAT_ConstWstrPtr:
		translatedType = GFxMovie::SA_StringW;
		return true;
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////
// implementation of IFlashVariableObject

class CFlashVariableObject : public IFlashVariableObject
{
public:
	// IFlashVariableObject interface
	virtual void Release();
	virtual IFlashVariableObject* Clone() const;

	virtual bool IsObject() const;
	virtual bool IsArray() const;
	virtual bool IsDisplayObject() const;

	virtual SFlashVarValue ToVarValue() const;

	virtual bool HasMember(const char* pMemberName) const;
	virtual bool SetMember(const char* pMemberName, const SFlashVarValue& value);
	virtual bool SetMember(const char* pMemberName, const IFlashVariableObject* pVarObj);
	virtual bool GetMember(const char* pMemberName, SFlashVarValue& value) const;
	virtual bool GetMember(const char* pMemberName, IFlashVariableObject*& pVarObj) const;
	virtual void VisitMembers(ObjectVisitor* pVisitor) const;
	virtual bool DeleteMember(const char* pMemberName);
	virtual bool Invoke(const char* pMethodName, const SFlashVarValue* pArgs, unsigned int numArgs, SFlashVarValue* pResult = 0);

	virtual unsigned int GetArraySize() const;
	virtual bool SetArraySize(unsigned int size);
	virtual bool SetElement(unsigned int idx, const SFlashVarValue& value);
	virtual bool SetElement(unsigned int idx, const IFlashVariableObject* pVarObj);
	virtual bool GetElement(unsigned int idx, SFlashVarValue& value) const;
	virtual bool GetElement(unsigned int idx, IFlashVariableObject*& pVarObj) const;
	virtual bool PushBack(const SFlashVarValue& value);
	virtual bool PushBack(const IFlashVariableObject* pVarObj);
	virtual bool PopBack();
	virtual bool RemoveElements(unsigned int idx, int count = -1);

	virtual bool SetDisplayInfo(const SFlashDisplayInfo& info);
	virtual bool GetDisplayInfo(SFlashDisplayInfo& info) const;
	virtual bool SetDisplayMatrix(const Matrix33& mat);
	virtual bool GetDisplayMatrix(Matrix33& mat) const;
	virtual bool Set3DMatrix(const Matrix44& mat);
	virtual bool Get3DMatrix(Matrix44& mat) const;
	virtual bool SetColorTransform(const SFlashCxform& cx);
	virtual bool GetColorTransform(SFlashCxform& cx) const;

	virtual bool SetText(const char* pText);
	virtual bool SetText(const wchar_t* pText);
	virtual bool SetTextHTML(const char* pHtml);
	virtual bool SetTextHTML(const wchar_t* pHtml);
	virtual bool GetText(SFlashVarValue& text) const;
	virtual bool GetTextHTML(SFlashVarValue& html) const;

	virtual bool CreateEmptyMovieClip(IFlashVariableObject*& pVarObjMC, const char* pInstanceName, int depth = -1);
	virtual bool AttachMovie(IFlashVariableObject*& pVarObjMC, const char* pSymbolName, const char* pInstanceName, int depth = -1, const IFlashVariableObject* pInitObj = 0);
	virtual bool GotoAndPlay(const char* pFrame);
	virtual bool GotoAndStop(const char* pFrame);
	virtual bool GotoAndPlay(unsigned int frame);
	virtual bool GotoAndStop(unsigned int frame);
	virtual bool SetVisible(bool visible);

public:
	CFlashVariableObject(const GFxValue& value, const CCryName& refFilePath, const cryshared_ptr<CryCriticalSection>& lock);
	CFlashVariableObject(const CFlashVariableObject& src);
	virtual ~CFlashVariableObject();

	const GFxValue& GetGFxValue() const;

private:
	GFxValue m_value;
	CCryName m_refFilePath;
	GFxValue m_retValRefHolder;
	cryshared_ptr<CryCriticalSection> m_lock;
};


static const GFxValue& GetGFxValue(const IFlashVariableObject* p)
{
	assert(p);
	return static_cast<const CFlashVariableObject*>(p)->GetGFxValue();
}


const GFxValue& CFlashVariableObject::GetGFxValue() const
{
	return m_value;
}


CFlashVariableObject::CFlashVariableObject(const GFxValue& value, const CCryName& refFilePath, const cryshared_ptr<CryCriticalSection>& lock)
: m_value(value)
, m_refFilePath(refFilePath)
, m_retValRefHolder()
, m_lock(lock)
{
}


CFlashVariableObject::CFlashVariableObject(const CFlashVariableObject& src)
: m_value(src.m_value)
, m_refFilePath(src.m_refFilePath)
, m_retValRefHolder()
, m_lock(src.m_lock)
{
}


CFlashVariableObject::~CFlashVariableObject()
{
}


void CFlashVariableObject::Release()
{
	delete this;
}


IFlashVariableObject* CFlashVariableObject::Clone() const
{
	return new CFlashVariableObject(*this);
}


bool CFlashVariableObject::IsObject() const
{
	return m_value.IsObject();
}


bool CFlashVariableObject::IsArray() const
{
	return m_value.IsArray();
}


bool CFlashVariableObject::IsDisplayObject() const
{
	return m_value.IsDisplayObject();
}


SFlashVarValue CFlashVariableObject::ToVarValue() const
{
	return ConvertValue(m_value);
}


bool CFlashVariableObject::HasMember(const char* pMemberName) const
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsObject())
		res = m_value.HasMember(pMemberName);
	return res;
}


bool CFlashVariableObject::SetMember(const char* pMemberName, const SFlashVarValue& value)
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsObject())
		res = m_value.SetMember(pMemberName, ConvertValue(value));
	return res;
}


bool CFlashVariableObject::SetMember(const char* pMemberName, const IFlashVariableObject* pVarObj)
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsObject())
		res = m_value.SetMember(pMemberName, pVarObj ? ::GetGFxValue(pVarObj) : GFxValue());
	return res;
}


bool CFlashVariableObject::GetMember(const char* pMemberName, SFlashVarValue& value) const
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsObject())
	{
		GFxValue retVal;
		res = m_value.GetMember(pMemberName, &retVal);
		value = ConvertValue(retVal);
	}
	return res;
}


bool CFlashVariableObject::GetMember(const char* pMemberName, IFlashVariableObject*& pVarObj) const
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	pVarObj = 0;
	if (m_value.IsObject())
	{
		GFxValue retVal;
		if (m_value.GetMember(pMemberName, &retVal))
		{
			//assert(retVal.IsObject());
			pVarObj = new CFlashVariableObject(retVal, m_refFilePath, m_lock);
		}
	}
	return pVarObj != 0;
}


bool CFlashVariableObject::Invoke(const char* pMethodName, const SFlashVarValue* pArgs, unsigned int numArgs, SFlashVarValue* pResult)
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	m_retValRefHolder = GFxValue();
	bool res(false);
	if (m_value.IsObject() && (pArgs || !numArgs))
	{
		assert(!pArgs || numArgs);
		GFxValue* pTranslatedArgs(0);
		if (pArgs && numArgs)
		{
			pTranslatedArgs = (GFxValue*) alloca(numArgs * sizeof(GFxValue));
			if (pTranslatedArgs)
			{
				for (unsigned int i(0); i<numArgs; ++i)
					new (&pTranslatedArgs[i]) GFxValue(ConvertValue(pArgs[i]));
			}
		}

		if (pTranslatedArgs || !numArgs)
		{
			GFxValue retVal;
			res = m_value.Invoke(pMethodName, &retVal, pTranslatedArgs, numArgs);
			if (pResult)
			{
				if (retVal.IsString() || retVal.IsStringW())
					m_retValRefHolder = retVal;
				*pResult = ConvertValue(retVal);
			}
			for (unsigned int i(0); i<numArgs; ++i)
				pTranslatedArgs[i].~GFxValue();
		}
	}
	return res;
}


void CFlashVariableObject::VisitMembers(ObjectVisitor* pVisitor) const
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	if (m_value.IsObject() && pVisitor)
	{
		struct VisitorAdaptor : public GFxValue::ObjectVisitor
		{
			IFlashVariableObject::ObjectVisitor* m_pClientVisitor;

			VisitorAdaptor(IFlashVariableObject::ObjectVisitor* pClientVisitor)
			: m_pClientVisitor(pClientVisitor)
			{
			}

			virtual void Visit(const char* name, const GFxValue& val)
			{
				assert(m_pClientVisitor);
				m_pClientVisitor->Visit(name);
			}
		};

		VisitorAdaptor vb(pVisitor);
		m_value.VisitMembers(&vb);
	}
}


bool CFlashVariableObject::DeleteMember(const char* pMemberName)
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsObject())
		res = m_value.DeleteMember(pMemberName);
	return res;
}


unsigned int CFlashVariableObject::GetArraySize() const
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	unsigned int res(0);
	if (m_value.IsArray())
		res = m_value.GetArraySize();
	return res;
}


bool CFlashVariableObject::SetArraySize(unsigned int size)
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsArray())
		res = m_value.SetArraySize(size);
	return res;
}


bool CFlashVariableObject::SetElement(unsigned int idx, const SFlashVarValue& value)
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsArray())
		res = m_value.SetElement(idx, ConvertValue(value));
	return res;
}


bool CFlashVariableObject::SetElement(unsigned int idx, const IFlashVariableObject* pVarObj)
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsArray())
		res = m_value.SetElement(idx, pVarObj ? ::GetGFxValue(pVarObj) : GFxValue());
	return res;
}


bool CFlashVariableObject::GetElement(unsigned int idx, SFlashVarValue& value) const
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsArray())
	{
		GFxValue retVal;
		res = m_value.GetElement(idx, &retVal);
		value = ConvertValue(retVal);
	}
	return res;
}


bool CFlashVariableObject::GetElement(unsigned int idx, IFlashVariableObject*& pVarObj) const
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	pVarObj = 0;
	if (m_value.IsArray())
	{
		GFxValue retVal;
		if (m_value.GetElement(idx, &retVal))
		{
			//assert(retVal.IsObject());
			pVarObj = new CFlashVariableObject(retVal, m_refFilePath, m_lock);
		}
	}
	return pVarObj != 0;
}


bool CFlashVariableObject::PushBack(const SFlashVarValue& value)
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsArray())
		res = m_value.PushBack(ConvertValue(value));
	return res;
}


bool CFlashVariableObject::PushBack(const IFlashVariableObject* pVarObj)
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsArray())
		res = m_value.PushBack(pVarObj ? ::GetGFxValue(pVarObj) : GFxValue());
	return res;
}


bool CFlashVariableObject::PopBack()
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsArray())
		res = m_value.PopBack();
	return res;
}


bool CFlashVariableObject::RemoveElements(unsigned int idx, int count)
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsArray())
		res = m_value.RemoveElements(idx, count);
	return res;
}


bool CFlashVariableObject::SetDisplayInfo(const SFlashDisplayInfo& info)
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsDisplayObject())
	{
		unsigned short varsSet =
			(info.IsFlagSet(SFlashDisplayInfo::FDIF_X) ? GFxValue::DisplayInfo::V_x : 0) |
			(info.IsFlagSet(SFlashDisplayInfo::FDIF_Y) ? GFxValue::DisplayInfo::V_y : 0) |
			(info.IsFlagSet(SFlashDisplayInfo::FDIF_Z) ? GFxValue::DisplayInfo::V_z : 0) |

			(info.IsFlagSet(SFlashDisplayInfo::FDIF_XScale) ? GFxValue::DisplayInfo::V_xscale : 0) |
			(info.IsFlagSet(SFlashDisplayInfo::FDIF_YScale) ? GFxValue::DisplayInfo::V_yscale : 0) |
			(info.IsFlagSet(SFlashDisplayInfo::FDIF_ZScale) ? GFxValue::DisplayInfo::V_zscale : 0) |

			(info.IsFlagSet(SFlashDisplayInfo::FDIF_Rotation) ? GFxValue::DisplayInfo::V_rotation : 0) |
			(info.IsFlagSet(SFlashDisplayInfo::FDIF_XRotation) ? GFxValue::DisplayInfo::V_xrotation : 0) |
			(info.IsFlagSet(SFlashDisplayInfo::FDIF_YRotation) ? GFxValue::DisplayInfo::V_yrotation : 0) |

			(info.IsFlagSet(SFlashDisplayInfo::FDIF_Alpha) ? GFxValue::DisplayInfo::V_alpha : 0) |
			(info.IsFlagSet(SFlashDisplayInfo::FDIF_Visible) ? GFxValue::DisplayInfo::V_visible : 0);

		GFxValue::DisplayInfo di;
		di.Initialize(varsSet,
			info.IsFlagSet(SFlashDisplayInfo::FDIF_X) ? info.GetX() : 0,
			info.IsFlagSet(SFlashDisplayInfo::FDIF_Y) ? info.GetY() : 0,
			info.IsFlagSet(SFlashDisplayInfo::FDIF_Rotation) ? info.GetRotation() : 0,
			info.IsFlagSet(SFlashDisplayInfo::FDIF_XScale) ? info.GetXScale() : 0,
			info.IsFlagSet(SFlashDisplayInfo::FDIF_YScale) ? info.GetYScale() : 0,
			info.IsFlagSet(SFlashDisplayInfo::FDIF_Alpha) ? info.GetAlpha() : 0,
			info.IsFlagSet(SFlashDisplayInfo::FDIF_Visible) ? info.GetVisible() : false,
			info.IsFlagSet(SFlashDisplayInfo::FDIF_Z) ? info.GetZ() : 0,
			info.IsFlagSet(SFlashDisplayInfo::FDIF_XRotation) ? info.GetXRotation() : 0,
			info.IsFlagSet(SFlashDisplayInfo::FDIF_YRotation) ? info.GetYRotation() : 0,
			info.IsFlagSet(SFlashDisplayInfo::FDIF_ZScale) ? info.GetZScale() : 0);

		res = m_value.SetDisplayInfo(di);
	}
	return res;
}


bool CFlashVariableObject::GetDisplayInfo(SFlashDisplayInfo& info) const
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsDisplayObject())
	{
		GFxValue::DisplayInfo di;
		res = m_value.GetDisplayInfo(&di);
		if (res)
		{
			unsigned short varsSet =
				(di.IsFlagSet(GFxValue::DisplayInfo::V_x) ? SFlashDisplayInfo::FDIF_X : 0) |
				(di.IsFlagSet(GFxValue::DisplayInfo::V_y) ? SFlashDisplayInfo::FDIF_Y : 0) |
				(di.IsFlagSet(GFxValue::DisplayInfo::V_z) ? SFlashDisplayInfo::FDIF_Z : 0) |

				(di.IsFlagSet(GFxValue::DisplayInfo::V_xscale) ? SFlashDisplayInfo::FDIF_XScale : 0) |
				(di.IsFlagSet(GFxValue::DisplayInfo::V_yscale) ? SFlashDisplayInfo::FDIF_YScale : 0) |
				(di.IsFlagSet(GFxValue::DisplayInfo::V_zscale) ? SFlashDisplayInfo::FDIF_ZScale : 0) |

				(di.IsFlagSet(GFxValue::DisplayInfo::V_rotation) ? SFlashDisplayInfo::FDIF_Rotation : 0) |
				(di.IsFlagSet(GFxValue::DisplayInfo::V_xrotation) ? SFlashDisplayInfo::FDIF_XRotation : 0) |
				(di.IsFlagSet(GFxValue::DisplayInfo::V_yrotation) ? SFlashDisplayInfo::FDIF_YRotation : 0) |

				(di.IsFlagSet(GFxValue::DisplayInfo::V_alpha) ? SFlashDisplayInfo::FDIF_Alpha : 0) |
				(di.IsFlagSet(GFxValue::DisplayInfo::V_visible) ? SFlashDisplayInfo::FDIF_Visible : 0);

			info = SFlashDisplayInfo(
				di.IsFlagSet(GFxValue::DisplayInfo::V_x) ? (float) di.GetX() : 0,
				di.IsFlagSet(GFxValue::DisplayInfo::V_y) ? (float) di.GetY() : 0,
				di.IsFlagSet(GFxValue::DisplayInfo::V_z) ? (float) di.GetZ() : 0,

				di.IsFlagSet(GFxValue::DisplayInfo::V_xscale) ? (float) di.GetXScale() : 0,
				di.IsFlagSet(GFxValue::DisplayInfo::V_yscale) ? (float) di.GetYScale() : 0,
				di.IsFlagSet(GFxValue::DisplayInfo::V_zscale) ? (float) di.GetZScale() : 0,

				di.IsFlagSet(GFxValue::DisplayInfo::V_rotation) ? (float) di.GetRotation() : 0,
				di.IsFlagSet(GFxValue::DisplayInfo::V_xrotation) ? (float) di.GetXRotation() : 0,
				di.IsFlagSet(GFxValue::DisplayInfo::V_yrotation) ? (float) di.GetYRotation() : 0,

				di.IsFlagSet(GFxValue::DisplayInfo::V_alpha) ? (float) di.GetAlpha() : 0,
				di.IsFlagSet(GFxValue::DisplayInfo::V_visible) ? di.GetVisible() : false,
				varsSet);
		}
	}
	return res;
}


bool CFlashVariableObject::SetVisible(bool visible)
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsDisplayObject())
	{
		GFxValue::DisplayInfo di;
		di.SetVisible(visible);
		res = m_value.SetDisplayInfo(di);
	}
	return res;
}


bool CFlashVariableObject::SetDisplayMatrix(const Matrix33& mat)
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsDisplayObject())
	{
		GMatrix2D m(mat.m00, mat.m01, mat.m10, mat.m11, mat.m02, mat.m12);
		res = m_value.SetDisplayMatrix(m);
	}
	return res;
}


bool CFlashVariableObject::GetDisplayMatrix(Matrix33& mat) const
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsDisplayObject())
	{
		GMatrix2D m;
		res = m_value.GetDisplayMatrix(&m);
		if (res)
		{
			mat.m00 = m.M_[0][0]; mat.m01 = m.M_[0][1]; mat.m02 = m.M_[0][2];
			mat.m10 = m.M_[1][0]; mat.m11 = m.M_[1][1]; mat.m12 = m.M_[1][2];
			mat.m20 = 0;          mat.m21 = 0;          mat.m22 = 1;
		}
	}
	return res;
}


bool CFlashVariableObject::Set3DMatrix(const Matrix44& mat)
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsDisplayObject())
	{
		GMatrix3D m;
		m.M_[0][0] = mat.m00; m.M_[0][1] = mat.m10; m.M_[0][2] = mat.m20; m.M_[0][3] = mat.m30;
		m.M_[1][0] = mat.m01; m.M_[1][1] = mat.m11; m.M_[1][2] = mat.m21; m.M_[1][3] = mat.m31;
		m.M_[2][0] = mat.m02; m.M_[2][1] = mat.m12; m.M_[2][2] = mat.m22; m.M_[2][3] = mat.m32;
		m.M_[3][0] = mat.m03; m.M_[3][1] = mat.m13; m.M_[3][2] = mat.m23; m.M_[3][3] = mat.m33;
		res = m_value.SetMatrix3D(m);
	}
	return res;
}


bool CFlashVariableObject::Get3DMatrix(Matrix44& mat) const
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsDisplayObject())
	{
		GMatrix3D m;
		res = m_value.GetMatrix3D(&m);
		if (res)
		{
			mat.m00 = m.M_[0][0]; mat.m01 = m.M_[1][0]; mat.m02 = m.M_[2][0]; mat.m03 = m.M_[3][0];
			mat.m10 = m.M_[0][1]; mat.m11 = m.M_[1][1]; mat.m12 = m.M_[2][1]; mat.m13 = m.M_[3][1];
			mat.m20 = m.M_[0][2]; mat.m21 = m.M_[1][2]; mat.m22 = m.M_[2][2]; mat.m23 = m.M_[3][2];
			mat.m30 = m.M_[0][3]; mat.m31 = m.M_[1][3]; mat.m32 = m.M_[2][3]; mat.m33 = m.M_[3][3];
		}
	}
	return res;
}


bool CFlashVariableObject::SetColorTransform(const SFlashCxform& cx)
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsDisplayObject())
	{
		GRenderer::Cxform cxform;
		cxform.M_[0][0] = cx.mul.r; cxform.M_[0][1] = cx.add.r;
		cxform.M_[1][0] = cx.mul.g; cxform.M_[1][1] = cx.add.g;
		cxform.M_[2][0] = cx.mul.b; cxform.M_[2][1] = cx.add.b;
		cxform.M_[3][0] = cx.mul.a; cxform.M_[3][1] = cx.add.a;
		res = m_value.SetColorTransform(cxform);
	}
	return res;
}


bool CFlashVariableObject::GetColorTransform(SFlashCxform& cx) const
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsDisplayObject())
	{
		GRenderer::Cxform cxform;
		res = m_value.GetColorTransform(&cxform);
		if (res)
		{
			cx.mul.r = cxform.M_[0][0]; cx.add.r = cxform.M_[0][1];
			cx.mul.g = cxform.M_[1][0]; cx.add.g = cxform.M_[1][1];
			cx.mul.b = cxform.M_[2][0]; cx.add.b = cxform.M_[2][1];
			cx.mul.a = cxform.M_[3][0]; cx.add.a = cxform.M_[3][1];
		}
	}
	return res;
}


bool CFlashVariableObject::SetText(const char* pText)
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsDisplayObject())
		res = m_value.SetText(pText);
	return res;
}


bool CFlashVariableObject::SetText(const wchar_t* pText)
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsDisplayObject())
		res = m_value.SetText(pText);
	return res;
}


bool CFlashVariableObject::SetTextHTML(const char* pHtml)
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsDisplayObject())
		res = m_value.SetTextHTML(pHtml);
	return res;
}


bool CFlashVariableObject::SetTextHTML(const wchar_t* pHtml)
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsDisplayObject())
		res = m_value.SetTextHTML(pHtml);
	return res;
}


bool CFlashVariableObject::GetText(SFlashVarValue& text) const
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsDisplayObject())
	{
		GFxValue retVal;
		res = m_value.GetText(&retVal);
		text = ConvertValue(retVal);
	}
	return res;
}


bool CFlashVariableObject::GetTextHTML(SFlashVarValue& html) const
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsDisplayObject())
	{
		GFxValue retVal;
		res = m_value.GetTextHTML(&retVal);
		html = ConvertValue(retVal);
	}
	return res;
}


bool CFlashVariableObject::CreateEmptyMovieClip(IFlashVariableObject*& pVarObjMC, const char* pInstanceName, int depth)
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	pVarObjMC = 0;
	if (m_value.IsDisplayObject())
	{
		GFxValue retVal;
		if (m_value.CreateEmptyMovieClip(&retVal, pInstanceName, depth))
		{
			//assert(retVal.IsObject());
			pVarObjMC = new CFlashVariableObject(retVal, m_refFilePath, m_lock);
		}
	}
	return pVarObjMC != 0;
}


bool CFlashVariableObject::AttachMovie(IFlashVariableObject*& pVarObjMC, const char* pSymbolName, const char* pInstanceName, int depth, const IFlashVariableObject* pInitObj)
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	pVarObjMC = 0;
	if (m_value.IsDisplayObject())
	{
		GFxValue retVal;
		if (m_value.AttachMovie(&retVal, pSymbolName, pInstanceName, depth, pInitObj ? &::GetGFxValue(pInitObj) : 0))
		{
			//assert(retVal.IsObject());
			pVarObjMC = new CFlashVariableObject(retVal, m_refFilePath, m_lock);
		}
	}
	return pVarObjMC != 0;
}


bool CFlashVariableObject::GotoAndPlay(const char* pFrame)
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsDisplayObject())
		res = m_value.GotoAndPlay(pFrame);
	return res;
}


bool CFlashVariableObject::GotoAndStop(const char* pFrame)
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsDisplayObject())
		res = m_value.GotoAndStop(pFrame);
	return res;
}


bool CFlashVariableObject::GotoAndPlay(unsigned int frame)
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsDisplayObject())
		res = m_value.GotoAndPlay(frame);
	return res;
}


bool CFlashVariableObject::GotoAndStop(unsigned int frame)
{
	SYNC_THREADS;

	SET_LOG_CONTEXT(m_refFilePath);
	bool res(false);
	if (m_value.IsDisplayObject())
		res = m_value.GotoAndStop(frame);
	return res;
}


//////////////////////////////////////////////////////////////////////////
// implementation of IFlashPlayer

#if defined(ENABLE_FLASH_INFO)
ICVar* CFlashPlayer::CV_sys_flash_info_peak_exclude(0);
#endif

int CFlashPlayer::ms_sys_flash(1);
int CFlashPlayer::ms_sys_flash_edgeaa(1);
#if defined(ENABLE_FLASH_INFO)
int CFlashPlayer::ms_sys_flash_info(0);
float CFlashPlayer::ms_sys_flash_info_peak_tolerance(5.0f);
float CFlashPlayer::ms_sys_flash_info_histo_scale(1.0f);
#endif
int CFlashPlayer::ms_sys_flash_log_options(0);
float CFlashPlayer::ms_sys_flash_curve_tess_error(1.0f);
int CFlashPlayer::ms_sys_flash_warning_level(1);

SLinkNode<CFlashPlayer> CFlashPlayer::ms_rootNode;
IFlashLoadMovieHandler* CFlashPlayer::ms_pLoadMovieHandler(0);


CFlashPlayer::CFlashPlayer()
: m_refCount(1)
, m_allowEgdeAA(false)
, m_compDepth(0)
, m_stereoPlaneDepth(1)
, m_renderConfig()
, m_asVerbosity()
, m_pFSCmdHandler(0)
, m_pFSCmdHandlerUserData(0)
, m_pEIHandler(0)
, m_pEIHandlerUserData(0)
, m_pMovieDef(0)
, m_pMovieView(0)
, m_pLoader(0)
, m_pRenderer(0)
, m_filePath()
, m_node()
#if defined(ENABLE_FLASH_INFO)
, m_pProfilerData(0)
#endif
, m_lock(new CryCriticalSection)
, m_retValRefHolder()
{
	assert(m_lock.get());
	Link();
}


CFlashPlayer::~CFlashPlayer()
{
	Unlink();

	// release ref counts of all shared resources
	{
		SET_LOG_CONTEXT(m_filePath);

		m_retValRefHolder = GFxValue();

		m_pMovieView = 0;
		m_pMovieDef = 0;

		m_pLoader = 0;
		m_pRenderer = 0;
	}

#if defined(ENABLE_FLASH_INFO)
	SAFE_DELETE(m_pProfilerData);
#endif
}


void CFlashPlayer::AddRef()
{
	CryInterlockedIncrement(&m_refCount);
}


void CFlashPlayer::Release()
{
	long refCount(CryInterlockedDecrement(&m_refCount));
	assert(refCount >= 0);
	if (refCount == 0)
		delete this;
}


static void OnSysFlashInfoLogOptionsChanged(ICVar*)
{
	GFxLoader2* pLoader(CSharedFlashPlayerResources::GetAccess().GetLoader(true));
	if (pLoader)
		pLoader->UpdateParserVerbosity();
}


void CFlashPlayer::InitCVars()
{
#if defined(_DEBUG)
	{
		static bool s_init = false;
		assert(!s_init);
		s_init = true;
	}
#endif

	REGISTER_CVAR2("sys_flash", &ms_sys_flash, 1, 
		VF_CHEAT, "Enables/disables execution of flash files.");

	REGISTER_CVAR2("sys_flash_edgeaa", &ms_sys_flash_edgeaa, 1, 
		0, "Enables/disables edge anti-aliased rendering of flash files.");

#if defined(ENABLE_FLASH_INFO)
	REGISTER_CVAR2("sys_flash_info", &ms_sys_flash_info, 0, 
		0, "Enables flash profiling (1). Additionally sorts the list of flash files lexicographically (2, automatically resets to 1).");

	REGISTER_CVAR2("sys_flash_info_peak_tolerance", &ms_sys_flash_info_peak_tolerance, 5.0f, 
		0, "Defines tolerance value for peaks (in ms) inside the flash profiler.");

	CV_sys_flash_info_peak_exclude = REGISTER_STRING("sys_flash_info_peak_exclude", "", 
		0, "Comma separated list of flash functions to excluded from peak history.");

	REGISTER_CVAR2("sys_flash_info_histo_scale", &ms_sys_flash_info_histo_scale, 1.0f, 
		0, "Defines scaling of function histogram inside the flash profiler.");
#endif

	ICVar* pSysFlashLogOptions = REGISTER_CVAR2("sys_flash_log_options", &ms_sys_flash_log_options, 0, 
		0, "Enables logging of several flash related aspects (add them to combine logging)...\n"
		"1) Flash loading                                                       : 1\n"
		"2) Flash actions script execution                                      : 2\n"
		"3) Flash related high-level calls inspected by the profiler into a file: 4\n"
		"   Please note that for (3) the following cvars apply:\n"
		"   * sys_flash_info\n"
		"   * sys_flash_info_peak_exclude");
	if (pSysFlashLogOptions)
		pSysFlashLogOptions->SetOnChangeCallback(OnSysFlashInfoLogOptionsChanged);

	REGISTER_CVAR2("sys_flash_curve_tess_error", &ms_sys_flash_curve_tess_error, 1.0f, 
		0, "Controls curve tessellation. Larger values result in coarser, more angular curves.");

	REGISTER_CVAR2("sys_flash_warning_level", &ms_sys_flash_warning_level, 1, 
		0, "Sets verbosity level for CryEngine related warnings...\n"
		"0) Omit warning\n"
		"1) Log warning\n"
		"2) Log warning and display message box");
}


int CFlashPlayer::GetWarningLevel()
{
	return ms_sys_flash_warning_level;
}


bool CFlashPlayer::Load(const char* pFilePath, unsigned int options)
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);

	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Other, 0, "Flash (%s)", pFilePath);

	m_pLoader = *CSharedFlashPlayerResources::GetAccess().GetLoader();
	m_pRenderer = *CSharedFlashPlayerResources::GetAccess().GetRenderer();

	// create cryname of file path
	m_filePath = pFilePath;

	SET_LOG_CONTEXT(m_filePath);

	// get movie info
	GFxMovieInfo movieInfo;
	if (!m_pLoader->GetMovieInfo(pFilePath, &movieInfo))
		return false;

	int warningLevel(GetWarningLevel());
	if (warningLevel && !movieInfo.IsStripped())
	{
		switch (warningLevel)
		{
		case 1:
			CryGFxLog::GetAccess().LogWarning("Trying to load non-stripped flash movie! Use gfxexport to strip movies and generate optimized assets.");
			break;
		case 2:
			CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "!Trying to load non-stripped flash movie! "
				"Use gfxexport to strip movies and generate optimized assets. [%s]", pFilePath);
			break;
		default:
			break;
		}
	}

	GPtr<GFxMovieDef> pMovieDef(0);
	GPtr<GFxMovieView> pMovieView(0);

	// create movie
	unsigned int loadFlags(GFxLoader::LoadAll);
	pMovieDef = *m_pLoader->CreateMovie(pFilePath, loadFlags);
	if (!pMovieDef)
		return false;

	// register translator
	pMovieDef->SetTranslator(&CryGFxTranslator::GetAccess());

	// create movie view
	pMovieView = *pMovieDef->CreateInstance((options & INIT_FIRST_FRAME) ? 1 : 0);
	if (!pMovieView)
		return false;

	// set movie definition and movie view
	m_pMovieDef = pMovieDef;
	m_pMovieView = pMovieView;

	// set action script verbosity
	UpdateASVerbosity();
	m_pMovieView->SetActionControl(&m_asVerbosity);
	
	// register custom renderer and set renderer flags
	unsigned int renderFlags(m_renderConfig.GetRenderFlags());
	renderFlags &= ~GFxRenderConfig::RF_StrokeMask;
	renderFlags |= GFxRenderConfig::RF_StrokeNormal;
	m_allowEgdeAA = (options & RENDER_EDGE_AA) != 0;
	if (m_allowEgdeAA)
		renderFlags |= GFxRenderConfig::RF_EdgeAA;
	m_renderConfig.SetRenderFlags(renderFlags);
	m_renderConfig.SetRenderer(m_pRenderer);
	m_pMovieView->SetRenderConfig(&m_renderConfig);

	// register action script (fs & external interface) and user event command handler, 
	// please note that client has to implement IFSCommandHandler interface 
	// and set it via IFlashPlayer::SetFSCommandHandler() to provide hook
	m_pMovieView->SetUserData(this);
	m_pMovieView->SetFSCommandHandler(&CryGFxFSCommandHandler::GetAccess());
	m_pMovieView->SetExternalInterface(&CryGFxExternalInterface::GetAccess());
	m_pMovieView->SetUserEventHandler(&CryGFxUserEventHandler::GetAccess());

	// enable mouse support
	m_pMovieView->SetMouseCursorCount((options & ENABLE_MOUSE_SUPPORT) ? 1 : 0);

	// done
	return true;
}


void CFlashPlayer::SetBackgroundColor(const ColorB& color)
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	if (m_pMovieView)
	{
		GColor col(color.r, color.g, color.b, color.a);
		m_pMovieView->SetBackgroundColor(col);
	}
}


void CFlashPlayer::SetBackgroundAlpha(float alpha)
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	if (m_pMovieView)
	{
		m_pMovieView->SetBackgroundAlpha(alpha);
	}
}


float CFlashPlayer::GetBackgroundAlpha() const
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	float res = 1.0f;
	if (m_pMovieView)
	{
		res = m_pMovieView->GetBackgroundAlpha();
	}
	return res;
}


void CFlashPlayer::SetViewport(int x0, int y0, int width, int height, float aspectRatio)
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	if (m_pMovieView)
	{
		GViewport viewport(width, height, x0, y0, width, height, 0); // invalidates scissor rect!
		viewport.AspectRatio = aspectRatio > 0 ? aspectRatio : 1;
		m_pMovieView->SetViewport(viewport);
	}
}


void CFlashPlayer::SetViewScaleMode(EScaleModeType scaleMode)
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	if (m_pMovieView)
	{
		GFxMovieView::ScaleModeType sm;
		switch(scaleMode)
		{
		case eSM_NoScale:   sm = GFxMovieView::SM_NoScale;  break;
		case eSM_ShowAll:   sm = GFxMovieView::SM_ShowAll;  break;
		case eSM_ExactFit:  sm = GFxMovieView::SM_ExactFit; break;
		case eSM_NoBorder:  sm = GFxMovieView::SM_NoBorder; break;
		default: assert(0); sm = GFxMovieView::SM_NoScale;  break;
		}
		m_pMovieView->SetViewScaleMode(sm);
	}
}


IFlashPlayer::EScaleModeType CFlashPlayer::GetViewScaleMode() const
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	EScaleModeType scaleMode = eSM_NoScale;
	if (m_pMovieView)
	{
		GFxMovieView::ScaleModeType sm = m_pMovieView->GetViewScaleMode();
		switch(sm)
		{
		case GFxMovieView::SM_NoScale:  scaleMode = eSM_NoScale;  break;
		case GFxMovieView::SM_ShowAll:  scaleMode = eSM_ShowAll;  break;
		case GFxMovieView::SM_ExactFit: scaleMode = eSM_ExactFit; break;
		case GFxMovieView::SM_NoBorder: scaleMode = eSM_NoBorder; break;
		default: assert(0);             scaleMode = eSM_NoScale;  break;
		}
	}
	return scaleMode;
}


void CFlashPlayer::SetViewAlignment(EAlignType viewAlignment)
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	if (m_pMovieView)
	{
		GFxMovieView::AlignType va;
		switch(viewAlignment)
		{
		case eAT_Center:        va = GFxMovieView::Align_Center;       break;
		case eAT_TopCenter:     va = GFxMovieView::Align_TopCenter;    break;
		case eAT_BottomCenter:  va = GFxMovieView::Align_BottomCenter; break;
		case eAT_CenterLeft:    va = GFxMovieView::Align_CenterLeft;   break;
		case eAT_CenterRight:   va = GFxMovieView::Align_CenterRight;  break;
		case eAT_TopLeft:       va = GFxMovieView::Align_TopLeft;      break;
		case eAT_TopRight:      va = GFxMovieView::Align_TopRight;     break;
		case eAT_BottomLeft:    va = GFxMovieView::Align_BottomLeft;   break;
		case eAT_BottomRight:   va = GFxMovieView::Align_BottomRight;  break;
		default: assert(0);     va = GFxMovieView::Align_Center;       break;
		}
		m_pMovieView->SetViewAlignment(va);
	}
}


IFlashPlayer::EAlignType CFlashPlayer::GetViewAlignment() const
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	EAlignType viewAlignment = eAT_Center;
	if (m_pMovieView)
	{
		GFxMovieView::AlignType va = m_pMovieView->GetViewAlignment();
		switch(va)
		{
		case GFxMovieView::Align_Center:       viewAlignment = eAT_Center;       break;
		case GFxMovieView::Align_TopCenter:    viewAlignment = eAT_TopCenter;    break;
		case GFxMovieView::Align_BottomCenter: viewAlignment = eAT_BottomCenter; break;
		case GFxMovieView::Align_CenterLeft:   viewAlignment = eAT_CenterLeft;   break;
		case GFxMovieView::Align_CenterRight:  viewAlignment = eAT_CenterRight;  break;
		case GFxMovieView::Align_TopLeft:      viewAlignment = eAT_TopLeft;      break;
		case GFxMovieView::Align_TopRight:     viewAlignment = eAT_TopRight;     break;
		case GFxMovieView::Align_BottomLeft:   viewAlignment = eAT_BottomLeft;   break;
		case GFxMovieView::Align_BottomRight:  viewAlignment = eAT_BottomRight;  break;
		default: assert(0);                    viewAlignment = eAT_Center;       break;
		}
	}
	return viewAlignment;
}


void CFlashPlayer::GetViewport(int& x0, int& y0, int& width, int& height, float& aspectRatio) const
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	if (m_pMovieView)
	{
		GViewport viewport;
		m_pMovieView->GetViewport(&viewport);
		x0 = viewport.Left;
		y0 = viewport.Top;
		width = viewport.Width;
		height = viewport.Height;
		aspectRatio = viewport.AspectRatio;
	}
	else
	{
		x0 = y0 = width = height = 0;
		aspectRatio = 1;
	}
}


void CFlashPlayer::SetScissorRect(int x0, int y0, int width, int height)
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	if (m_pMovieView)
	{
		GViewport viewport;
		m_pMovieView->GetViewport(&viewport);
		viewport.SetScissorRect(x0, y0, width, height);
		m_pMovieView->SetViewport(viewport);
	}
}


void CFlashPlayer::GetScissorRect(int& x0, int& y0, int& width, int& height) const
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	if (m_pMovieView)
	{
		GViewport viewport;
		m_pMovieView->GetViewport(&viewport);
		if (viewport.Flags & GViewport::View_UseScissorRect)
		{
			x0 = viewport.ScissorLeft;
			y0 = viewport.ScissorTop;
			width = viewport.ScissorWidth;
			height = viewport.ScissorHeight;
			return;
		}
	}
	x0 = y0 = width = height = 0;
}


void CFlashPlayer::Advance(float deltaTime)
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	FLASH_PROFILE_FUNC_1ARG(eFncAdvance, VOID_RETURN, "Advance", deltaTime);
	SET_LOG_CONTEXT(m_filePath);
	if (m_pMovieView)
	{
		// UI should run at full speed even if time scaling is used -- MarcoK
		// TODO: revise! 
		//   Either  caller should adjust the time or each flash instance should have 
		//   the means to decide whether to adjust for bullet time or not -- CarstenW
		float timeScale = gEnv->pTimer->GetTimeScale();
		if (timeScale < 1.0f)
		{
			timeScale = max(0.0001f, timeScale);
			deltaTime = deltaTime / timeScale;
		}

		UpdateASVerbosity();
		m_pMovieView->Advance(deltaTime);
	}
}


void CFlashPlayer::Render(bool stereo)
{
	if (!IsFlashEnabled())
		return;

	AddRef();
	gEnv->pRenderer->RT_FlashRender(this, stereo);
}


void CFlashPlayer::RenderCallback(EFrameType ft, bool releaseOnExit)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Flash RenderCallback");

	RELEASE_ON_EXIT(releaseOnExit);
	{
		SYNC_THREADS;
#if defined(ENABLE_FLASH_INFO)
		if (ms_sys_flash_info)
			gEnv->pRenderer->SF_Flush();
#endif
		{
			FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
			FLASH_PROFILE_FUNC(eFncDisplay, VOID_RETURN, "Display");
			SET_LOG_CONTEXT(m_filePath);
			int* isLost((int*) m_pRenderer->GetXRender()->EF_Query(EFQ_DeviceLost, 0));
			if (m_pMovieView && !*isLost)
			{
				UpdateRenderFlags();
				m_pRenderer->SetCompositingDepth(m_compDepth);
				m_pRenderer->SetStereoMode(ft != EFT_Mono, ft == EFT_StereoLeft);
				m_pRenderer->SetStereoPlaneDepth(m_stereoPlaneDepth);
				m_pMovieView->Display();
			}
#if defined(ENABLE_FLASH_INFO)
			if (ms_sys_flash_info)
				gEnv->pRenderer->SF_Flush();
#endif
		}
	}
}


void CFlashPlayer::SetCompositingDepth(float depth)
{
	//SYNC_THREADS; //don't sync (should hardly produce read/write conflicts and thus rendering glitches, if ever)

	m_compDepth = depth;
}


void CFlashPlayer::SetStereoPlaneDepth(float depth)
{
	//SYNC_THREADS; //don't sync (should hardly produce read/write conflicts and thus rendering glitches, if ever)

	m_stereoPlaneDepth = depth;
}


void CFlashPlayer::Restart()
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	if (m_pMovieView)
	{
		m_pMovieView->Restart();
	}
}


bool CFlashPlayer::IsPaused() const
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	bool res = false;
	if (m_pMovieView)
	{
		res = m_pMovieView->IsPaused();
	}
	return res;
}


void CFlashPlayer::Pause(bool pause)
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	if (m_pMovieView)
	{
		m_pMovieView->SetPause(pause);
	}
}


void CFlashPlayer::GotoFrame(unsigned int frameNumber)
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	if (m_pMovieView)
	{
		m_pMovieView->GotoFrame(frameNumber);
	}
}


bool CFlashPlayer::GotoLabeledFrame(const char* pLabel, int offset)
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	bool res = false;
	if (m_pMovieView && pLabel)
	{
		res = m_pMovieView->GotoLabeledFrame(pLabel, offset);
	}
	return res;
}


unsigned int CFlashPlayer::GetCurrentFrame() const
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	unsigned int res = 0;
	if (m_pMovieView)
	{
		res = m_pMovieView->GetCurrentFrame();
	}
	return res;
}


bool CFlashPlayer::HasLooped() const
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	bool res = false;
	if (m_pMovieView)
	{
		res = m_pMovieView->HasLooped();
	}
	return res;
}


void CFlashPlayer::SetFSCommandHandler(IFSCommandHandler* pHandler, void* pUserData)
{
	//SYNC_THREADS; //don't sync (handler needs to reside in same thread as flash player instance)

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	m_pFSCmdHandler = pHandler;
	m_pFSCmdHandlerUserData = pUserData;
}


void CFlashPlayer::SetExternalInterfaceHandler(IExternalInterfaceHandler* pHandler, void* pUserData)
{
	//SYNC_THREADS; //don't sync (handler needs to reside in same thread as flash player instance)

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	m_pEIHandler = pHandler;
	m_pEIHandlerUserData = pUserData;
}


void CFlashPlayer::SendCursorEvent(const SFlashCursorEvent& cursorEvent)
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	if (m_pMovieView)
	{
		switch (cursorEvent.m_state)
		{
		case SFlashCursorEvent::eCursorMoved:
			{
				GFxMouseEvent event(GFxEvent::MouseMove, (UInt) cursorEvent.m_button, (SInt16) cursorEvent.m_cursorX, (SInt16) cursorEvent.m_cursorY);
				m_pMovieView->HandleEvent(event);
				break;
			}
		case SFlashCursorEvent::eCursorPressed:
			{
				GFxMouseEvent event(GFxEvent::MouseDown, (UInt) cursorEvent.m_button, (SInt16) cursorEvent.m_cursorX, (SInt16) cursorEvent.m_cursorY);
				m_pMovieView->HandleEvent(event);
				break;
			}
		case SFlashCursorEvent::eCursorReleased:
			{
				GFxMouseEvent event(GFxEvent::MouseUp, (UInt) cursorEvent.m_button, (SInt16) cursorEvent.m_cursorX, (SInt16) cursorEvent.m_cursorY);
				m_pMovieView->HandleEvent(event);
				break;
			}
		case SFlashCursorEvent::eWheel:
			{
				GFxMouseEvent event(GFxEvent::MouseWheel, (UInt) cursorEvent.m_button, (SInt16) cursorEvent.m_cursorX, (SInt16) cursorEvent.m_cursorY, (Float) cursorEvent.m_wheelScrollVal);
				m_pMovieView->HandleEvent(event);
				break;
			}
		}
	}
}


void CFlashPlayer::SendKeyEvent(const SFlashKeyEvent& keyEvent)
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	if (m_pMovieView)
	{
		GFxKeyEvent event(keyEvent.m_state == SFlashKeyEvent::eKeyDown ? GFxEvent::KeyDown : GFxEvent::KeyUp, 
			(GFxKey::Code) keyEvent.m_keyCode, keyEvent.m_asciiCode, keyEvent.m_wcharCode);

		event.SpecialKeysState.SetShiftPressed((keyEvent.m_specialKeyState & SFlashKeyEvent::eShiftPressed) != 0);
		event.SpecialKeysState.SetCtrlPressed((keyEvent.m_specialKeyState & SFlashKeyEvent::eCtrlPressed) != 0);
		event.SpecialKeysState.SetAltPressed((keyEvent.m_specialKeyState & SFlashKeyEvent::eAltPressed) != 0);
		event.SpecialKeysState.SetCapsToggled((keyEvent.m_specialKeyState & SFlashKeyEvent::eCapsToggled) != 0);
		event.SpecialKeysState.SetNumToggled((keyEvent.m_specialKeyState & SFlashKeyEvent::eNumToggled) != 0);
		event.SpecialKeysState.SetScrollToggled((keyEvent.m_specialKeyState & SFlashKeyEvent::eScrollToggled) != 0);

		m_pMovieView->HandleEvent(event);
	}
}


void CFlashPlayer::SetVisible(bool visible)
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	if (m_pMovieView)
		m_pMovieView->SetVisible(visible);
}


bool CFlashPlayer::GetVisible() const
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	bool res(false);
	if (m_pMovieView)
		res = m_pMovieView->GetVisible();
	return res;
}


bool CFlashPlayer::SetVariable(const char* pPathToVar, const SFlashVarValue& value)
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	FLASH_PROFILE_FUNC_2ARG(eFncSetVar, false, "SetVariable", pPathToVar, &value);
	SET_LOG_CONTEXT(m_filePath);
	bool res(false);
	if (m_pMovieView)
		res = m_pMovieView->SetVariable(pPathToVar, ConvertValue(value));
	return res;
}


bool CFlashPlayer::SetVariable(const char* pPathToVar, const IFlashVariableObject* pVarObj)
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	FLASH_PROFILE_FUNC_2ARG(eFncSetVar, false, "SetVariable", pPathToVar, (void*) pVarObj);
	SET_LOG_CONTEXT(m_filePath);
	bool res(false);
	if (m_pMovieView)
		res = m_pMovieView->SetVariable(pPathToVar, pVarObj ? GetGFxValue(pVarObj) : GFxValue());
	return res;
}


bool CFlashPlayer::GetVariable(const char* pPathToVar, SFlashVarValue& value) const
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	FLASH_PROFILE_FUNC_2ARG(eFncGetVar, false, "GetVariable", pPathToVar, (void*) &value);
	SET_LOG_CONTEXT(m_filePath);
	bool res(false);
	if (m_pMovieView)
	{
		GFxValue retVal;
		res = m_pMovieView->GetVariable(&retVal, pPathToVar);
		value = ConvertValue(retVal);
	}
	return res;
}


bool CFlashPlayer::GetVariable(const char* pPathToVar, IFlashVariableObject*& pVarObj) const
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	FLASH_PROFILE_FUNC_2ARG(eFncGetVar, false, "GetVariable", pPathToVar, (void*) pVarObj);
	SET_LOG_CONTEXT(m_filePath);
	pVarObj = 0;
	if (m_pMovieView)
	{
		GFxValue retVal;
		if (m_pMovieView->GetVariable(&retVal, pPathToVar))
		{
			//assert(retVal.IsObject());
			pVarObj = new CFlashVariableObject(retVal, m_filePath, m_lock);
		}
	}
	return pVarObj != 0;
}


bool CFlashPlayer::IsAvailable(const char* pPathToVar) const
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	FLASH_PROFILE_FUNC_1ARG(eFncIsAvailable, false, "IsAvailable", pPathToVar);
	SET_LOG_CONTEXT(m_filePath);
	bool res(false);
	if (m_pMovieView)
		res = m_pMovieView->IsAvailable(pPathToVar);
	return res;
}


bool CFlashPlayer::SetVariableArray(EFlashVariableArrayType type, const char* pPathToVar, unsigned int index, const void* pData, unsigned int count)
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	FLASH_PROFILE_FUNC_5ARG(eFncSetVar, false, "SetVariableArray", type, pPathToVar, index, pData, count);
	SET_LOG_CONTEXT(m_filePath);
	GFxMovie::SetArrayType translatedType;
	bool res(false);
	if (m_pMovieView && GetArrayType(type, translatedType))
		res = m_pMovieView->SetVariableArray(translatedType, pPathToVar, index, pData, count);
	return res;
}


unsigned int CFlashPlayer::GetVariableArraySize(const char* pPathToVar) const
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	FLASH_PROFILE_FUNC_1ARG(eFncGetVar, 0, "GetVariableArraySize", pPathToVar);
	SET_LOG_CONTEXT(m_filePath);
	int res(0);
	if (m_pMovieView)
		res = m_pMovieView->GetVariableArraySize(pPathToVar);
	return res;
}


bool CFlashPlayer::GetVariableArray(EFlashVariableArrayType type, const char* pPathToVar, unsigned int index, void* pData, unsigned int count) const
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	FLASH_PROFILE_FUNC_5ARG(eFncGetVar, false, "GetVariableArray", type, pPathToVar, index, pData, count);
	SET_LOG_CONTEXT(m_filePath);
	bool res(false);
	if (m_pMovieView && pData)
	{
		GFxMovie::SetArrayType translatedType;
		if (GetArrayType(type, translatedType))
			res = m_pMovieView->GetVariableArray(translatedType, pPathToVar, index, pData, count);
	}
	return res;
}


bool CFlashPlayer::Invoke(const char* pMethodName, const SFlashVarValue* pArgs, unsigned int numArgs, SFlashVarValue* pResult)
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	FLASH_PROFILE_FUNC_1ARG_VALIST(eFncInvoke, false, "Invoke", pMethodName, pArgs, numArgs);
	SET_LOG_CONTEXT(m_filePath);
	m_retValRefHolder = GFxValue();
	bool res(false);
	if (m_pMovieView && (pArgs || !numArgs))
	{
		assert(!pArgs || numArgs);
		GFxValue* pTranslatedArgs(0);
		if (pArgs && numArgs)
		{
			pTranslatedArgs = (GFxValue*) alloca(numArgs * sizeof(GFxValue));
			if (pTranslatedArgs)
			{
				for (unsigned int i(0); i<numArgs; ++i)
					new (&pTranslatedArgs[i]) GFxValue(ConvertValue(pArgs[i]));
			}
		}

		if (pTranslatedArgs || !numArgs)
		{
			GFxValue retVal;
			res = m_pMovieView->Invoke(pMethodName, &retVal, pTranslatedArgs, numArgs);
			if (pResult)
			{
				if (retVal.IsString() || retVal.IsStringW())
					m_retValRefHolder = retVal;
				*pResult = ConvertValue(retVal);
			}
			for (unsigned int i(0); i<numArgs; ++i)
				pTranslatedArgs[i].~GFxValue();
		}
	}
	return res;
}


bool CFlashPlayer::CreateString(const char* pString, IFlashVariableObject*& pVarObj)
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	pVarObj = 0;
	if (m_pMovieView)
	{
		GFxValue retVal;
		m_pMovieView->CreateString(&retVal, pString);
		if (retVal.IsString())
			pVarObj = new CFlashVariableObject(retVal, m_filePath, m_lock);
	}
	return pVarObj != 0;
}


bool CFlashPlayer::CreateStringW(const wchar_t* pString, IFlashVariableObject*& pVarObj)
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	pVarObj = 0;
	if (m_pMovieView)
	{
		GFxValue retVal;
		m_pMovieView->CreateStringW(&retVal, pString);
		if (retVal.IsStringW())
			pVarObj = new CFlashVariableObject(retVal, m_filePath, m_lock);
	}
	return pVarObj != 0;
}


bool CFlashPlayer::CreateObject(const char* pClassName, const SFlashVarValue* pArgs, unsigned int numArgs, IFlashVariableObject*& pVarObj)
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	pVarObj = 0;
	if (m_pMovieView && (pArgs || !numArgs))
	{
		assert(!pArgs || numArgs);
		GFxValue* pTranslatedArgs(0);
		if (pArgs && numArgs)
		{
			pTranslatedArgs = (GFxValue*) alloca(numArgs * sizeof(GFxValue));
			if (pTranslatedArgs)
			{
				for (unsigned int i(0); i<numArgs; ++i)
					new (&pTranslatedArgs[i]) GFxValue(ConvertValue(pArgs[i]));
			}
		}

		if (pTranslatedArgs || !numArgs)
		{
			GFxValue retVal;
			m_pMovieView->CreateObject(&retVal, pClassName, pTranslatedArgs, numArgs);
			if (retVal.IsObject())
				pVarObj = new CFlashVariableObject(retVal, m_filePath, m_lock);
			for (unsigned int i(0); i<numArgs; ++i)
				pTranslatedArgs[i].~GFxValue();
		}
	}
	return pVarObj != 0;
}


bool CFlashPlayer::CreateArray(IFlashVariableObject*& pVarObj)
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	pVarObj = 0;
	if (m_pMovieView)
	{
		GFxValue retVal;
		m_pMovieView->CreateArray(&retVal);
		if (retVal.IsArray())
			pVarObj = new CFlashVariableObject(retVal, m_filePath, m_lock);
	}
	return pVarObj != 0;
}


struct FunctionHandlerAdaptor : public GFxFunctionHandler
{
	struct ReturnValue : public IActionScriptFunction::IReturnValue
	{
		ReturnValue(GFxMovieView* pMovieView)
		: m_pMovieView(pMovieView)
		, m_value()
		{
		}

		~ReturnValue()
		{
		}

		void Set(const SFlashVarValue& value, bool createManagedValue = true)
		{
			switch (value.GetType())
			{
			case SFlashVarValue::eConstStrPtr:
				{
					if (createManagedValue)
						m_pMovieView->CreateString(&m_value, value.GetConstStrPtr());
					else
						m_value = ConvertValue(value);
					break;
				}
			case SFlashVarValue::eConstWstrPtr:
				{
					if (createManagedValue)
						m_pMovieView->CreateStringW(&m_value, value.GetConstWstrPtr());
					else
						m_value = ConvertValue(value);
					break;
				}
			default:
				m_value = ConvertValue(value);
				break;
			}
		}

		GFxMovieView* m_pMovieView;
		GFxValue m_value;
	};

	FunctionHandlerAdaptor(IActionScriptFunction* pFunc, CFlashPlayer* pPlayer)
	: m_pFunc(pFunc)
	, m_pPlayer(pPlayer)
	{
		assert(m_pFunc);
		assert(m_pPlayer);
	}

	~FunctionHandlerAdaptor()
	{
	}

	void Call(const GFxFunctionHandler::Params& params)
	{
		// translate this and arguments
		const unsigned int numArgs = params.ArgCount;

		CFlashVariableObject* pTranslatedArgsWithThisRef = (CFlashVariableObject*) alloca((numArgs + 1) * sizeof(CFlashVariableObject));
		assert(pTranslatedArgsWithThisRef);

		assert(params.pArgsWithThisRef);
		for (unsigned int i=0; i<numArgs+1; ++i)
			new (&pTranslatedArgsWithThisRef[i]) CFlashVariableObject(params.pArgsWithThisRef[i], m_pPlayer->m_filePath, m_pPlayer->m_lock);

		const IFlashVariableObject** ppTranslatedArgs = 0;
		if (numArgs)
		{
			ppTranslatedArgs = (const IFlashVariableObject**) alloca(numArgs * sizeof(IFlashVariableObject*));
			assert(ppTranslatedArgs);
			for (unsigned int i=0; i<numArgs; ++i)
				ppTranslatedArgs[i] = &pTranslatedArgsWithThisRef[i+1];
		}

		// setup translated calling parameters
		IActionScriptFunction::Params translatedParams;
		translatedParams.pFromPlayer = m_pPlayer;
		translatedParams.pUserData = params.pUserData;
		translatedParams.pThis = &pTranslatedArgsWithThisRef[0];
		translatedParams.pArgs = ppTranslatedArgs;
		translatedParams.numArgs = numArgs;

		// call function
		assert(params.pMovie);
		ReturnValue retVal(params.pMovie);

		m_pPlayer->AddRef();
		m_pFunc->Call(translatedParams, &retVal);
		m_pPlayer->Release();

		// translate return value
		if (params.pRetVal)
			*params.pRetVal = retVal.m_value;

		// destruct translated this and arguments
		for (unsigned int i=0; i<numArgs + 1; ++i)
			pTranslatedArgsWithThisRef[i].~CFlashVariableObject();
	}

	IActionScriptFunction* m_pFunc;
	CFlashPlayer* m_pPlayer;
};


bool CFlashPlayer::CreateFunction(IFlashVariableObject*& pFuncVarObj, IActionScriptFunction* pFunc, void* pUserData)
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	pFuncVarObj = 0;
	if (m_pMovieView && pFunc)
	{
		GPtr<FunctionHandlerAdaptor> fhb = *(new FunctionHandlerAdaptor(pFunc, this));
		GFxValue retVal;
		m_pMovieView->CreateFunction(&retVal, fhb, pUserData);
		if (retVal.IsObject())
			pFuncVarObj = new CFlashVariableObject(retVal, m_filePath, m_lock);
	}
	return pFuncVarObj != 0;
}


unsigned int CFlashPlayer::GetFrameCount() const
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	unsigned int res = 0;
	if (m_pMovieView)
	{
		res = m_pMovieView->GetFrameCount();
	}
	return res;
}


float CFlashPlayer::GetFrameRate() const
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	float res = 0;
	if (m_pMovieView)
	{
		res = m_pMovieView->GetFrameRate();
	}
	return res;
}


int CFlashPlayer::GetWidth() const
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	int res(0);
	if (m_pMovieDef)
		res = (int) m_pMovieDef->GetWidth();
	return res;
}


int CFlashPlayer::GetHeight() const
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	int res(0);
	if (m_pMovieDef)
		res = (int) m_pMovieDef->GetHeight();
	return res;
}


size_t CFlashPlayer::GetMetadata(char* pBuff, unsigned int buffSize) const
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	size_t res(0);
	if (m_pMovieDef)
		res = m_pMovieDef->GetMetadata(pBuff, buffSize);
	return res;
}


const char* CFlashPlayer::GetFilePath() const
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	return m_filePath.c_str();
}


void CFlashPlayer::ScreenToClient(int& x, int& y) const
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	if (m_pMovieView)
	{
		GViewport viewport;
		m_pMovieView->GetViewport(&viewport);
		x -= viewport.Left;
		y -= viewport.Top;
	}
	else
	{
		x = y = 0;
	}
}


void CFlashPlayer::ClientToScreen(int& x, int& y) const
{
	SYNC_THREADS;

	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	SET_LOG_CONTEXT(m_filePath);
	if (m_pMovieView)
	{
		GViewport viewport;
		m_pMovieView->GetViewport(&viewport);
		x += viewport.Left;
		y += viewport.Top;
	}
}


void CFlashPlayer::DelegateFSCommandCallback(const char* pCommand, const char* pArgs)
{
	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	// delegate action script command to client
	if (m_pFSCmdHandler)
	{
		AddRef();
		m_pFSCmdHandler->HandleFSCommand(pCommand, pArgs, m_pFSCmdHandlerUserData);
		Release();
	}
}


void CFlashPlayer::DelegateExternalInterfaceCallback(const char* pMethodName, const GFxValue* pArgs, UInt numArgs)
{
	FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_SYSTEM, g_bProfilerEnabled);
	if (m_pMovieView)
	{
		// delegate action script command to client
		GFxValue retVal(GFxValue::VT_Undefined);
		if (m_pEIHandler)
		{
			AddRef();

			assert(!pArgs || numArgs);
			SFlashVarValue* pTranslatedArgs(0);
			if (pArgs && numArgs)
			{
				pTranslatedArgs = (SFlashVarValue*) alloca(numArgs * sizeof(SFlashVarValue));
				if (pTranslatedArgs)
				{
					for (unsigned int i(0); i<numArgs; ++i)
						new (&pTranslatedArgs[i]) SFlashVarValue(ConvertValue(pArgs[i]));
				}
			}

			if (pTranslatedArgs || !numArgs)
			{
				SFlashVarValue ret(SFlashVarValue::CreateUndefined());
				m_pEIHandler->HandleExternalInterfaceCall(pMethodName, pTranslatedArgs, numArgs, m_pEIHandlerUserData, &ret);
				retVal = ConvertValue(ret);
				for (unsigned int i(0); i<numArgs; ++i)
					pTranslatedArgs[i].~SFlashVarValue();
			}

			Release();
		}
		m_pMovieView->SetExternalInterfaceRetVal(retVal);
	}
}


bool CFlashPlayer::IsFlashEnabled()
{
	return ms_sys_flash != 0;
}


bool CFlashPlayer::IsEdgeAaAllowed() const
{
	return m_allowEgdeAA && (ms_sys_flash_edgeaa != 0);
}


void CFlashPlayer::UpdateRenderFlags()
{
	const unsigned int renderFlags(m_renderConfig.GetRenderFlags());
	
	unsigned int newRenderFlags(renderFlags);
	newRenderFlags &= ~GFxRenderConfig::RF_EdgeAA;
	if (IsEdgeAaAllowed())
		newRenderFlags |= GFxRenderConfig::RF_EdgeAA;

	if (newRenderFlags != renderFlags)
		m_renderConfig.SetRenderFlags(newRenderFlags);

	float curMaxCurveError(m_renderConfig.GetMaxCurvePixelError());
	if (ms_sys_flash_curve_tess_error != curMaxCurveError)
	{
		if (ms_sys_flash_curve_tess_error < 1.0f)
			ms_sys_flash_curve_tess_error = 1.0f;
		m_renderConfig.SetMaxCurvePixelError(ms_sys_flash_curve_tess_error);
	}
}


void CFlashPlayer::UpdateASVerbosity()
{
	m_asVerbosity.SetActionFlags((ms_sys_flash_log_options & CFlashPlayer::LO_ACTIONSCRIPT) ? GFxActionControl::Action_Verbose : 0);
}


void CFlashPlayer::Link()
{
	m_node.m_pHandle = this;
	m_node.Link(&ms_rootNode);
}


void CFlashPlayer::Unlink()
{
	m_node.Unlink();
	m_node.m_pHandle = 0;
}


static void Draw2dLabel(float x,float y, float fontSize, const float* pColor, const char* pText)
{
	SDrawTextInfo ti;
	ti.xscale = ti.yscale = fontSize;
	ti.flags = eDrawText_2D | eDrawText_800x600 | eDrawText_FixedSize | eDrawText_Monospace;
	if (pColor)
	{
		ti.color[0] = pColor[0];
		ti.color[1] = pColor[1];
		ti.color[2] = pColor[2];
		ti.color[3] = pColor[3];
	}
	gEnv->pRenderer->DrawTextQueued(Vec3(x, y, 0.5f), ti, pText);
}


static void DrawText(float x, float y, const float* pColor, const char* pFormat, ...)
{
	char buffer[512];
	const size_t cnt = sizeof(buffer);

	va_list args;
	va_start(args, pFormat);
	int written = vsnprintf(buffer, cnt, pFormat, args);
	if (written < 0 || written == cnt)
		buffer[cnt-1] = '\0';
	va_end(args);

	Draw2dLabel(x, y, 1.4f, pColor, buffer);
}


static void DrawTextNoFormat(float x, float y, float* const pColor, const char* pText)
{
	Draw2dLabel(x, y, 1.4f, pColor, pText);
}


static inline float CalculateFlashVarianceFactor(float value, float variance)
{
	const float VAL_EPSILON(0.000001f);
	const float VAR_MULTIPLIER(2.0f);

	//variance = fabs(variance - value*value);
	float difference((float)sqrt_tpl(variance));

	value = (float) fabs(value);
	if (value < VAL_EPSILON)
		return 0;

	float factor(0);
	if (value > 0.01f)
		factor = (difference / value) * VAR_MULTIPLIER;

	return factor;
}


static void CalculateColor(float factor, float* pOutColor)
{
	float ColdColor[4] = {0.15f, 0.9f, 0.15f, 1};
	float HotColor[4] = {1, 1, 1, 1};

	for (int k=0; k<3; ++k)
		pOutColor[k] = HotColor[k] * factor + ColdColor[k] * (1.0f - factor);
	pOutColor[3] = 1;
}


static void CalculateColor(float value, float variance, float* pOutColor)
{
	CalculateColor(clamp_tpl(CalculateFlashVarianceFactor(value, variance), 0.0f, 1.0f), pOutColor);
}


static float CalculateVariance(float value, float avg)
{
	return (value - avg) * (value - avg);
}


static size_t InKB(size_t memSize)
{
	return (memSize + 1023) >> 10;
}


void CFlashPlayer::RenderFlashInfo()
{
#if defined(ENABLE_FLASH_INFO)
	CFlashFunctionProfilerLog::GetAccess().Enable(ms_sys_flash_info != 0 && (ms_sys_flash_log_options & LO_PEAKS) != 0, gEnv->pRenderer->GetFrameID(false));

	GRendererXRender* pFlashRenderer(CSharedFlashPlayerResources::GetAccess().GetRenderer(true));

	if (ms_sys_flash_info)
	{
		// define color
		const float inactiveColChannelMult(0.7f);
		static const float color[4] = {1, 1, 1, 1};
		static const float colorList[4] = {0.3f, 0.8f, 1, 1};
		static const float colorListInactive[4] = {0.6f, 0.6f, 0.6f, 1};
		static const float colorListSelected[4] = {1, 0, 0, 1};
		static const float colorListSelectedInactive[4] = {0.6f, 0, 0, 1};

		IRenderer* pRend = gEnv->pRenderer;
		Vec2 overscanBorders = *(Vec2*) pRend->EF_Query(EFQ_OverscanBorders);
		float xAdj = pRend->GetWidth()  * overscanBorders.x;
		float yAdj = pRend->GetHeight() * overscanBorders.y;

		// display number of primitives rendered
		GRenderer::Stats stats;
		if (pFlashRenderer)
			pFlashRenderer->GetRenderStats(&stats, true);
		DrawText(xAdj + 10.0f, yAdj + 10.0f, color, "#Tris      : %d", stats.Triangles);
		DrawText(xAdj + 10.0f, yAdj + 22.0f, color, "#Lines     : %d", stats.Lines);
		DrawText(xAdj + 10.0f, yAdj + 34.0f, color, "#Masks     : %d", stats.Masks);
		DrawText(xAdj + 10.0f, yAdj + 46.0f, color, "#DrawCalls : %d", stats.Primitives);

		// display memory statistics
		GMemoryHeap* pGFxHeap = GMemory::GetGlobalHeap();
		if (pGFxHeap)
		{
			GFxLoader2* pLoader = CSharedFlashPlayerResources::GetAccess().GetLoader(true);

			GPtr<GFxMeshCacheManager> pMeshCacheMgr = pLoader ? pLoader->GetMeshCacheManager() : 0;
			GMemoryHeap* pHeap = pMeshCacheMgr ? pMeshCacheMgr->GetHeap() : 0;
			size_t memMeshCache = pHeap ? pHeap->GetTotalUsedSpace() : 0;

			GPtr<GFxFontCacheManager> pFontCacheMgr = pLoader ? pLoader->GetFontCacheManager() : 0;
			pHeap = pFontCacheMgr ? pFontCacheMgr->GetHeap() : 0;
			size_t memFontCache = pHeap ? pHeap->GetTotalUsedSpace() : 0;

			const GSysAllocCryMem::Stats& memAllocStats = CSharedFlashPlayerResources::GetAccess().GetSysAllocStats();
			DrawText(xAdj + 10.0f, yAdj +  67.0f, color, "Sys #Alloc()         : %d", memAllocStats.AllocCount);
			DrawText(xAdj + 10.0f, yAdj +  79.0f, color, "Sys #Free()          : %d", memAllocStats.FreeCount);
			DrawText(xAdj + 10.0f, yAdj +  96.0f, color, "Sys Allocated        : %dk", InKB(memAllocStats.Allocated));
			DrawText(xAdj + 10.0f, yAdj + 108.0f, color, "GlobalHeap Footprint : %dk", InKB(pGFxHeap->GetTotalFootprint()));
			DrawText(xAdj + 10.0f, yAdj + 120.0f, color, "GlobalHeap Used      : %dk", InKB(pGFxHeap->GetTotalUsedSpace()));
			DrawText(xAdj + 10.0f, yAdj + 132.0f, color, "MeshCacheHeap Used   : %dk", InKB(memMeshCache));
			DrawText(xAdj + 10.0f, yAdj + 144.0f, color, "FontCacheHeap Used   : %dk", InKB(memFontCache));
			DrawText(xAdj + 10.0f, yAdj + 156.0f, color, "TexMemory Used       : %dk", InKB(GTextureXRender::GetTextureMemoryUsed()));
		}

		// sort lexicographically if requested
		if (ms_sys_flash_info == 2)
		{
			bool needSorting(true);
			while (needSorting)
			{
				needSorting = false;
				SLinkNode<CFlashPlayer>* pNode(ms_rootNode.m_pNext);
				SLinkNode<CFlashPlayer>* pNext(pNode->m_pNext);
				while (pNext != &ms_rootNode)
				{
					if (stricmp(pNode->m_pHandle->m_filePath.c_str(), pNext->m_pHandle->m_filePath.c_str()) > 0)
					{
						needSorting = true;

						SLinkNode<CFlashPlayer>* pNodePrev(pNode->m_pPrev);
						SLinkNode<CFlashPlayer>* pNextNext(pNext->m_pNext);

						pNodePrev->m_pNext = pNext;
						pNextNext->m_pPrev = pNode;

						pNode->m_pPrev = pNext;
						pNode->m_pNext = pNextNext;

						pNext->m_pPrev = pNodePrev;
						pNext->m_pNext = pNode;
					}
					else
						pNode = pNode->m_pNext;

					pNext = pNode->m_pNext;
				}
			}

			ms_sys_flash_info = 1;
		}

		// toggle write protection to pause profiling
		// (should be done before display & gathering of profile data in next frame)
		if (CryGetAsyncKeyState(VK_SCROLL) & 1)
		{
			SFlashProfilerData::ToggleHistogramWriteProtection();
			s_flashPeakHistory.ToggleWriteProtection();
		}
	
		// ypos = 178.0f; placeholder for number of (active) flash files 
		int numFlashFiles(0), numActiveFlashFiles(0);

		// ypos = 196.0f; placeholder for overall cost of flash processing / rendering
		// display flash instance profiling results
		static SLinkNode<CFlashPlayer>* s_pLastSelectedLinkNode(ms_rootNode.m_pNext);
		bool lastSelectedLinkNodeLost(true);

		const float deltaFrameTime(gEnv->pTimer->GetFrameTime());
		const float blendFactor(expf(-deltaFrameTime / 0.35f));
		float ypos(214.0f);
		SLinkNode<CFlashPlayer>* pCurNode(ms_rootNode.m_pNext);
		while (pCurNode != &ms_rootNode)
		{
			if (s_pLastSelectedLinkNode == pCurNode)
				lastSelectedLinkNodeLost = false;
		
			CFlashPlayer* pFlashPlayer(pCurNode->m_pHandle);
			SFlashProfilerData* pFlashProfilerData(pFlashPlayer ? pFlashPlayer->m_pProfilerData : 0);

			float funcCostAvg[eNumFncIDs], funcCostMin[eNumFncIDs], funcCostMax[eNumFncIDs], funcCostCur[eNumFncIDs];
			float totalCostAvg(0), totalCostMin(0), totalCostMax(0), totalCostCur(0);
			if (pFlashProfilerData)
			{
				for (int i(0); i<eNumFncIDs; ++i)
				{
					funcCostAvg[i] = pFlashProfilerData->GetTotalTimeHisto((EFlashFunctionID)i).GetAvg();
					funcCostMin[i] = pFlashProfilerData->GetTotalTimeHisto((EFlashFunctionID)i).GetMin();
					funcCostMax[i] = pFlashProfilerData->GetTotalTimeHisto((EFlashFunctionID)i).GetMax();
					funcCostCur[i] = pFlashProfilerData->GetTotalTimeHisto((EFlashFunctionID)i).GetCur();
				}

				totalCostAvg = pFlashProfilerData->GetTotalTimeHistoAllFuncs().GetAvg();
				totalCostMin = pFlashProfilerData->GetTotalTimeHistoAllFuncs().GetMin();
				totalCostMax = pFlashProfilerData->GetTotalTimeHistoAllFuncs().GetMax();
				totalCostCur = pFlashProfilerData->GetTotalTimeHistoAllFuncs().GetCur();
			}
			else
			{
				for (int i(0); i<eNumFncIDs; ++i)
				{
					funcCostAvg[i] = 0;
					funcCostMin[i] = 0;
					funcCostMax[i] = 0;
					funcCostCur[i] = 0;
				}
			}

			const char* pVisExpChar("");
			if (pFlashProfilerData)
				pVisExpChar = pFlashProfilerData->IsViewExpanded() ? "-" : "+";
			
			const char* pInstanceInfo("");
			if (pFlashPlayer && (!pFlashPlayer->m_pMovieDef || !pFlashPlayer->m_pMovieView))
				pInstanceInfo = "!!! Instantiated but Load() failed !!!";

			const char* pDisabled("");
			if (pFlashProfilerData && pFlashProfilerData->PreventFunctionExectution())
				pDisabled = "[disabled]";

			bool selected(s_pLastSelectedLinkNode == pCurNode); 
			bool inactive(!pFlashProfilerData || totalCostAvg < 1e-3f);

			const float* col(0);
			if (inactive)
				col = (!selected) ? colorListInactive : colorListSelectedInactive;
			else
				col = (!selected) ? colorList : colorListSelected;
			
			DrawText(xAdj + 10.0f, yAdj + ypos, col, "%.2f", totalCostAvg);
			if (pFlashPlayer)
			{
				size_t memMovieView = 0;
				GFxMovieView* pMovieView = pFlashPlayer->m_pMovieView.GetPtr();
				if (pMovieView)
				{
					GMemoryHeap* pHeap = pMovieView->GetHeap();
					memMovieView += pHeap ? pHeap->GetTotalUsedSpace() : 0;
				}

				size_t memMovieDef = 0;
				GFxMovieDef* pMovieDef = pFlashPlayer->m_pMovieDef.GetPtr();
				if (pMovieDef)
				{
					GFxResourceReport* pRR = pMovieDef->GetResourceReport();
					GMemoryHeap* pHeap = pRR ? pRR->GetResourceHeap() : 0;
					memMovieDef += pHeap ? pHeap->GetTotalUsedSpace() : 0;

					GFxResource* pMovieDataDef = pMovieDef->GetMovieDataResource();
					pRR = pMovieDataDef->GetResourceReport();
					pHeap = pRR ? pRR->GetResourceHeap() : 0;
					memMovieDef += pHeap ? pHeap->GetTotalUsedSpace() : 0;
				}

				DrawText(xAdj + 50.0f, yAdj + ypos, col, "%s %s%s (def: 0x%p / %dk, view: 0x%p / %dk, vis: %d) %s", pDisabled, pVisExpChar, pFlashPlayer->m_filePath.c_str(), 
					pFlashPlayer->m_pMovieDef.GetPtr(), InKB(memMovieDef), pFlashPlayer->m_pMovieView.GetPtr(), InKB(memMovieView), !inactive && pFlashPlayer->GetVisible() ? 1 : 0, pInstanceInfo);
			}
			ypos += 12.0f;

			if (pFlashProfilerData && pFlashProfilerData->IsViewExpanded())
			{
				float colChannelMult(!inactive ? 1 : inactiveColChannelMult);
				float colorDyn[4] = {0, 0, 0, 0};
				{
					float newColorValue(totalCostCur * (1 - blendFactor) + pFlashProfilerData->GetColorValue(eNumFncIDs) * blendFactor);
					float variance(CalculateVariance(totalCostCur, totalCostAvg));
					float newColorValueVariance(variance * (1 - blendFactor) + pFlashProfilerData->GetColorValueVariance(eNumFncIDs) * blendFactor);

					pFlashProfilerData->SetColorValue(eNumFncIDs, newColorValue);
					pFlashProfilerData->SetColorValueVariance(eNumFncIDs, newColorValueVariance);

					CalculateColor(newColorValue, newColorValueVariance, colorDyn);
					colorDyn[0] *= colChannelMult;
					colorDyn[1] *= colChannelMult;
					colorDyn[2] *= colChannelMult;

					ypos += 4.0f;
					DrawText(xAdj +  60.0f, yAdj + ypos, colorDyn, "Total:");
					DrawText(xAdj + 170.0f, yAdj + ypos, colorDyn, "avg %.2f", totalCostAvg);
					DrawText(xAdj + 245.0f, yAdj + ypos, colorDyn, "/ min %.2f", totalCostMin);
					DrawText(xAdj + 332.0f, yAdj + ypos, colorDyn, "/ max %.2f", totalCostMax);
					ypos += 20.0f;
				}

				if (ms_sys_flash_info_histo_scale < 1e-2f)
					ms_sys_flash_info_histo_scale = 1e-2f;
				float histoScale(ms_sys_flash_info_histo_scale * 50.0f);

				for (int i(0); i<eNumFncIDs; ++i)
				{
					float newColorValue(funcCostCur[i] * (1 - blendFactor) + pFlashProfilerData->GetColorValue((EFlashFunctionID)i) * blendFactor);
					float variance(CalculateVariance(funcCostCur[i], funcCostAvg[i]));
					float newColorValueVariance(variance * (1 - blendFactor) + pFlashProfilerData->GetColorValueVariance((EFlashFunctionID)i) * blendFactor);

					pFlashProfilerData->SetColorValue((EFlashFunctionID)i, newColorValue);
					pFlashProfilerData->SetColorValueVariance((EFlashFunctionID)i, newColorValueVariance);

					CalculateColor(newColorValue, newColorValueVariance, colorDyn);
					colorDyn[0] *= colChannelMult;
					colorDyn[1] *= colChannelMult;
					colorDyn[2] *= colChannelMult;

					int numAvgCalls(pFlashProfilerData->GetTotalCallsHisto((EFlashFunctionID)i).GetAvg());
					DrawText(xAdj +  25.0f, yAdj + ypos, colorDyn, "%d/", numAvgCalls);
					DrawText(xAdj +  60.0f, yAdj + ypos, colorDyn, "%s()", GetFlashFunctionName((EFlashFunctionID)i));
					DrawText(xAdj + 170.0f, yAdj + ypos, colorDyn, "avg %.2f", funcCostAvg[i]);
					DrawText(xAdj + 245.0f, yAdj + ypos, colorDyn, "/ min %.2f", funcCostMin[i]);
					DrawText(xAdj + 332.0f, yAdj + ypos, colorDyn, "/ max %.2f", funcCostMax[i]);

					unsigned char histogram[SFlashProfilerData::HistogramSize];
					for (int j(0); j<SFlashProfilerData::HistogramSize; ++j)
						histogram[j] = 255 - (unsigned int) clamp_tpl(pFlashProfilerData->GetTotalTimeHisto((EFlashFunctionID)i).GetAt(j) * histoScale, 0.0f, 255.0f);

					ColorF histogramCol(0,1,0,1);
					gEnv->pRenderer->Graph(histogram, (int) (xAdj + 440), (int) (yAdj + ypos), SFlashProfilerData::HistogramSize, 14, pFlashProfilerData->GetTotalTimeHisto((EFlashFunctionID)i).GetCurIdx(), 2, 0, histogramCol, 1);

					ypos += 16.0f;
				}
			}

			if (pFlashProfilerData)
				pFlashProfilerData->Advance();

			++numFlashFiles;
			if (!inactive)
				++numActiveFlashFiles;

			pCurNode = pCurNode->m_pNext;
		}

		// fill placeholders
		DrawText(xAdj +  10.0f, yAdj + 178.0f, color, "#FlashObj : %d", numFlashFiles);
		DrawText(xAdj + 130.0f, yAdj + 178.0f, colorList, "/ #Active : %d", numActiveFlashFiles);
		DrawText(xAdj + 250.0f, yAdj + 178.0f, colorListInactive, "/ #Inactive : %d", numFlashFiles - numActiveFlashFiles);

		DrawText(xAdj +  10.0f, yAdj + 196.0f, color, "Total:");
		DrawText(xAdj +  70.0f, yAdj + 196.0f, color, "avg %.2f", SFlashProfilerData::GetTotalTimeHistoAllInsts().GetAvg());
		DrawText(xAdj + 155.0f, yAdj + 196.0f, color, "/ min %.2f", SFlashProfilerData::GetTotalTimeHistoAllInsts().GetMin());
		DrawText(xAdj + 252.0f, yAdj + 196.0f, color, "/ max %.2f", SFlashProfilerData::GetTotalTimeHistoAllInsts().GetMax());

		SFlashProfilerData::AdvanceAllInsts();

		const char* pProfilerPaused("");
		if (SFlashProfilerData::HistogramWriteProtected())
			pProfilerPaused = "[paused]";
		DrawText(xAdj + 320.0f, yAdj + 196.0f, color, "%s", pProfilerPaused);

		// display flash peak history
		ypos += 4.0f;
		float displayTime(gEnv->pTimer->GetAsyncCurTime());
		DrawText(xAdj + 10.0f, yAdj + ypos, color, "Latest flash peaks (tolerance = %.2f ms)...", ms_sys_flash_info_peak_tolerance);
		ypos += 16.0f;

		s_flashPeakHistory.UpdateHistory(displayTime);
		const float maxFlashPeakDisplayTime(s_flashPeakHistory.GetTimePeakExpires());

		const CFlashPeakHistory::FlashPeakHistory& peakHistory(s_flashPeakHistory.GetHistory());
		CFlashPeakHistory::FlashPeakHistory::const_iterator it(peakHistory.begin());
		CFlashPeakHistory::FlashPeakHistory::const_iterator itEnd(peakHistory.end());
		for (; it != itEnd;)
		{
			const SFlashPeak& peak(*it);
			float colorDyn[4] = {0, 0, 0, 0};
			CalculateColor(clamp_tpl(1.0f - (displayTime - peak.m_timeRecorded) / maxFlashPeakDisplayTime, 0.0f, 1.0f), colorDyn);
			DrawTextNoFormat(xAdj + 10.0f, yAdj + ypos, colorDyn, peak.m_displayInfo.c_str());
			ypos += 12.0f;
			++it;
		}

		// update peak history function exclude mask
		CFlashFunctionProfiler::ResetPeakFuncExcludeMask();
		const char* pExludeMaskStr(CV_sys_flash_info_peak_exclude ? CV_sys_flash_info_peak_exclude->GetString() : 0);
		if (pExludeMaskStr)
		{
			for (int i(0); i<eNumFncIDs; ++i)
			{
				EFlashFunctionID funcID((EFlashFunctionID) i);
				const char* pFlashFuncName(GetFlashFunctionName(funcID));
				if (strstr(pExludeMaskStr, pFlashFuncName))
					CFlashFunctionProfiler::AddToPeakFuncExcludeMask(funcID);
			}
		}

		// allow user navigation for detailed profiling statistics
		if (lastSelectedLinkNodeLost)
			s_pLastSelectedLinkNode = ms_rootNode.m_pNext;

		if (CryGetAsyncKeyState(VK_UP) & 1)
		{
			if (CryGetAsyncKeyState(VK_CONTROL) & 0x8000)
			{
				SLinkNode<CFlashPlayer>* pNode(&ms_rootNode);
				SLinkNode<CFlashPlayer>* pNext(ms_rootNode.m_pNext);

				SLinkNode<CFlashPlayer>* pNodePrev(pNode->m_pPrev);
				SLinkNode<CFlashPlayer>* pNextNext(pNext->m_pNext);

				pNodePrev->m_pNext = pNext;
				pNextNext->m_pPrev = pNode;

				pNode->m_pPrev = pNext;
				pNode->m_pNext = pNextNext;

				pNext->m_pPrev = pNodePrev;
				pNext->m_pNext = pNode;
			}
			else
			{
				s_pLastSelectedLinkNode = s_pLastSelectedLinkNode->m_pPrev;
				if (s_pLastSelectedLinkNode == &ms_rootNode)
					s_pLastSelectedLinkNode = s_pLastSelectedLinkNode->m_pPrev;
			}
		}

		if (CryGetAsyncKeyState(VK_DOWN) & 1)
		{
			if (CryGetAsyncKeyState(VK_CONTROL) & 0x8000)
			{
				SLinkNode<CFlashPlayer>* pNode(&ms_rootNode);
				SLinkNode<CFlashPlayer>* pPrev(ms_rootNode.m_pPrev);

				SLinkNode<CFlashPlayer>* pNodeNext(pNode->m_pNext);
				SLinkNode<CFlashPlayer>* pPrevPrev(pPrev->m_pPrev);

				pNodeNext->m_pPrev = pPrev;
				pPrevPrev->m_pNext = pNode;

				pNode->m_pNext = pPrev;
				pNode->m_pPrev = pPrevPrev;

				pPrev->m_pNext = pNodeNext;
				pPrev->m_pPrev = pNode;
			}
			else
			{
				s_pLastSelectedLinkNode = s_pLastSelectedLinkNode->m_pNext;
				if (s_pLastSelectedLinkNode == &ms_rootNode)
					s_pLastSelectedLinkNode = s_pLastSelectedLinkNode->m_pNext;
			}
		}

		if (CryGetAsyncKeyState(VK_RIGHT) & 1)
		{
			if (s_pLastSelectedLinkNode != &ms_rootNode)
			{
				CFlashPlayer* pFlashPlayer(s_pLastSelectedLinkNode->m_pHandle);
				SFlashProfilerData* pFlashProfilerData(pFlashPlayer ? pFlashPlayer->m_pProfilerData : 0);
				if (pFlashProfilerData)
					pFlashProfilerData->SetViewExpanded(true);
			}
		}

		if (CryGetAsyncKeyState(VK_LEFT) & 1)
		{
			if (s_pLastSelectedLinkNode != &ms_rootNode)
			{
				CFlashPlayer* pFlashPlayer(s_pLastSelectedLinkNode->m_pHandle);
				SFlashProfilerData* pFlashProfilerData(pFlashPlayer ? pFlashPlayer->m_pProfilerData : 0);
				if (pFlashProfilerData)
					pFlashProfilerData->SetViewExpanded(false);
			}
		}

		if (CryGetAsyncKeyState(VK_SPACE) & 1)
		{
			if (s_pLastSelectedLinkNode != &ms_rootNode)
			{
				CFlashPlayer* pFlashPlayer(s_pLastSelectedLinkNode->m_pHandle);
				SFlashProfilerData* pFlashProfilerData(pFlashPlayer ? pFlashPlayer->m_pProfilerData : 0);
				if (pFlashProfilerData)
					pFlashProfilerData->TogglePreventFunctionExecution();
			}
		}
	}
	else
	{
		if (pFlashRenderer)
			pFlashRenderer->GetRenderStats(0, true);
	}
#endif // #if defined(ENABLE_FLASH_INFO)
}


void CFlashPlayer::SetFlashLoadMovieHandler(IFlashLoadMovieHandler* pHandler)
{
	ms_pLoadMovieHandler = pHandler;
}


IFlashLoadMovieHandler* CFlashPlayer::GetFlashLoadMovieHandler()
{
	return ms_pLoadMovieHandler;
}


unsigned int CFlashPlayer::GetLogOptions()
{
	return ms_sys_flash_log_options;
}


#endif // #ifndef EXCLUDE_SCALEFORM_SDK


// factory for IFlashPlayer interface
IFlashPlayer* CSystem::CreateFlashPlayerInstance() const
{
#ifndef EXCLUDE_SCALEFORM_SDK
	return new CFlashPlayer;
#else
	return 0;
#endif
}


void CSystem::RenderFlashInfo()
{
#ifndef EXCLUDE_SCALEFORM_SDK
	CFlashPlayer::RenderFlashInfo();
#endif
}


void CSystem::GetFlashMemoryUsage(ICrySizer* pSizer)
{
#ifndef EXCLUDE_SCALEFORM_SDK
	CSharedFlashPlayerResources::GetAccess().GetMemoryUsage(pSizer);
#endif
}


void CSystem::SetFlashLoadMovieHandler(IFlashLoadMovieHandler* pHandler) const
{
#ifndef EXCLUDE_SCALEFORM_SDK
	CFlashPlayer::SetFlashLoadMovieHandler(pHandler);
#endif
}
