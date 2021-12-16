////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek
// -------------------------------------------------------------------------
//  File name:   XenonThreadSampler.h
//  Version:     v1.00
//  Created:     14/02/2008 by Timur
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __XenonThreadSampler_h__
#define __XenonThreadSampler_h__

//========================================================
// class TThreadSampler
//========================================================
class CXenonThreadSampler
{
public:

	static const int MAX_SAMPLES = 20000;
	static const int MAX_HW_THREADS = 6;

	// Every context switch sample.
	struct ThreadSwitchSample
	{
		uint32 hwThread;
		uint32 fromThreadId;
		uint32 toThreadId;
		int64 time;
	};

	// Pixels span used to display data.
	struct Span
	{
		uint16 start;
		uint16 end;
	};
	struct SnapshotInfo
	{
		// Pointer to the array of context switches, one int per processor, size of the array is given in nProcessorCount.
		int pContextSwitches[MAX_HW_THREADS];
		int nProcessorCount;
	};

	volatile ThreadSwitchSample* m_pSamplingBufferPtr;
	volatile int m_lastContextSwitch;
	volatile int m_nContextSwitches[MAX_HW_THREADS];

	std::vector<uint32> threads;

	int64 m_ticksPerSecond;
	int64 m_referenceTime;

public:
	CXenonThreadSampler();
	~CXenonThreadSampler();

	bool MonitorThreadSwitch( bool bEnable );

	void EnumerateThreads( int nProcessId );
	bool MakeSnapshot( SnapshotInfo &snapshotInfo );

	//Create span list for drawing graph
	//processId,threadId - build list for specified thread.
	//threadId = OTHER_THREADS - build list for threads not belonging to specified process
	//width - width of timeline, in pixels
	//scale - scale of timeline, in 0.5 seconds units, f.e 2 for 1-second timeline. 4 for 2 seconds timeline etc.
	//ProcessorId - index of the hardware processor thread (or core)
	//totalTime - receives total time in miliiseconds, used by this thread during one second (does not depend on 'scale' paraleter).
	void CreateSpanListForThread( uint32 processId,uint32 threadId,std::vector<Span>& spans,uint32 width,uint32 scale,
		uint32* totalTime,int *ProcessorId,uint32 *color );

private:
	int64 m_lastSnapshotTime;
	uint32 m_nLastContextSwitches[MAX_HW_THREADS];
	//int m_nFirstReferenceIndex[MAX_HW_THREADS];
	int m_nFirstReferenceIndex;
	void *m_monitor_session;
	
	ThreadSwitchSample m_samples[MAX_HW_THREADS][MAX_SAMPLES];
	int nSamplesPerThread[MAX_HW_THREADS];

	ThreadSwitchSample m_rawSamplesBuffer1[MAX_SAMPLES];
	ThreadSwitchSample m_rawSamplesBuffer2[MAX_SAMPLES];
};

#endif //__XenonThreadSampler_h__