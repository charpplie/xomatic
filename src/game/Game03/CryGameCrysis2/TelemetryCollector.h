/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Logs data to a centralised server for later analysis
various game subsystems, forwarding necessary events to stats
recording system.

-------------------------------------------------------------------------
History:
- 18:11:2009  : Created by Mark Tully

*************************************************************************/

#ifndef __TELEMETRYCOLLECTOR_H__
#define __TELEMETRYCOLLECTOR_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ITelemetryCollector.h"
#include "ILevelSystem.h"

struct ITelemetryCollector;
struct ICryTelemetry;
struct STelemetryData;
typedef _smart_ptr<STelemetryData> STelemetryDataPtr;

class CTelemetryCollector : public ITelemetryCollector, public ILevelSystemListener
{
	public:
		typedef uint32			TTelemetrySubmitFlags;
		enum
		{
			k_tf_none					= 0,
			k_tf_appendToRemoteFile		= (1<<0)
		};

	protected:
		ICVar					*m_serverNameCVar;
		ICVar					*m_serverPortCVar;
		ICVar					*m_telemetryTransactionRecordings;
		ICVar					*m_telemetryEnabled;
		ICVar					*m_serverTelemetryPath;
		ICVar					*m_serverTelemetryPathSP;
		ICVar					*m_telemetryServerLogging;
		string					m_curSessionId;
		string					m_websafeClientName;
		CryFixedStringT<512>	m_telemetryRecordingPath;
		CryFixedStringT<512> m_telemetryMemoryLogPath;
		int						m_fileUploadCounter;
		int						m_lastLevelRotationIndex;

		ICryTelemetry*			m_pTelemetry;

		void					EscapeString(
									string			*ioString);

		const char				*GetWebSafePlatformName();
		string					GetWebSafeClientName();
		string					GetWebSafeSessionId();

		static void				SubmitGameLog(
									IConsoleCmdArgs *inArgs);
		static void				OutputSessionId(
									IConsoleCmdArgs *inArgs);

		void					Log(int level, const char *format, ...);

		bool					InitService();

		bool					UploadData(
									STelemetryDataPtr pData,
									const char			*inReferenceFilename);

		int						MakePostHeader(
									const char			*inRemoteFileName,
									int					inDataLength,
									char				*outBuffer,
									int					inMaxBufferSize,
									TTelemetrySubmitFlags	inFlags);

	public:
		static const int		k_maxHttpHeaderSize=1024;

								CTelemetryCollector();
								~CTelemetryCollector();

		bool					ShouldSubmitTelemetry();

		bool					SubmitFile(
									const char		*inLocalFilePath);
		bool					AppendToFile(
									const char		*inLocalFilePath,
									const char		*inDataToAppend);

		void					SetNewSessionId();
		void					OutputTelemetryServerHintFile();
		void					OutputMemoryUsage(const char *message, const char *newLevelName);
		string					GetSessionId();
		void					SetSessionId(
									string			inNewSessionId);

		// ILevelSystemListener
		virtual void OnLevelNotFound(const char *levelName) {}
		virtual void OnLoadingStart(ILevelInfo *pLevel);
		virtual void OnLoadingComplete(ILevel *pLevel);
		virtual void OnLoadingError(ILevelInfo *pLevel, const char *error) {}
		virtual void OnLoadingProgress(ILevelInfo *pLevel, int progressAmount) {}
		//~ILevelSystemListener
};

////////////////////////////////////////////////////////////////////////////////
// Telemetry Buffer util class
////////////////////////////////////////////////////////////////////////////////

#include "RecordingBuffer.h"

enum ETelemetryPacketType
{
	// If you add enums to this list - you will also need to update the code in TelemetryCollector::FormatBufferData()
	eTPT_Performance = eRBPT_Custom,
	eTPT_Bandwidth,
	eTPT_Memory,
};

typedef SRecording_Packet ITelemetryBufferData;

struct SPerformanceTelemetry : ITelemetryBufferData
{
	SPerformanceTelemetry()
	{
		size = sizeof(SPerformanceTelemetry);
		type = eTPT_Performance;
	}
	float	m_timeInSeconds;
	int		m_numTicks;
	float m_gpuTime; //gpu frame time in secs
	int	  m_gpuLimited; // num frames gpu limited
};

struct SBandwidthTelemetry : ITelemetryBufferData
{
	SBandwidthTelemetry()
	{
		size = sizeof(SBandwidthTelemetry);
		type = eTPT_Bandwidth;
	}
	uint64		m_bandwidthSent;
	uint64		m_bandwidthReceived;
	float	m_timeInSeconds;
	int		m_packetsSent;
};

struct SMemoryTelemetry : ITelemetryBufferData
{
	SMemoryTelemetry()
	{
		size = sizeof(SMemoryTelemetry);
		type = eTPT_Memory;
	}
	float	m_timeInSeconds;
	float	m_cpuMemUsedInMB;
};

class CTelemetryBuffer
{
public:
	CTelemetryBuffer(int buffersize, ITelemetryCollector *collector, size_t structSize);

	void AddData(ITelemetryBufferData *dataStruct);
	void SubmitToServer(const char *filename);
	void DumpToFile(const char *filename);
	void Reset();

protected:
	void FormatBufferData(ITelemetryBufferData* packet, CryFixedStringT<1024> &outString);

	CRecordingBuffer *m_pBuffer;
	ITelemetryCollector *m_collector;

	size_t m_structSize;
};


#endif // __TELEMETRYCOLLECTOR_H__
