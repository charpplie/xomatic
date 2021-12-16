#ifdef PS3

#define OverAllocateInputBufferSize 128
#define OverAllocateOutputBufferSize 128
#define InflateInputBufferMaxSize	(52*1024)
#define InflateOutputBufferMaxSize	(80*1024)

#define LS_RANGE (256*1024)
#ifndef TASK_INFLATE
	#define TASK_INFLATE 1
#endif

struct SPollEvent
{
#ifdef __SPU__
	uint32 val[4];
	//set the value into the slot matching last 4 bits of EA, return LS to dma from
	const uint32 SetValue(const uint32 cEA, const uint32 cVal)
	{
		const uint32 cSlot = (cEA & 15) / 4;
		val[cSlot] = cVal;
		return (uint32)&val[cSlot];
	}
#else
	volatile uint32 val;
	inline void Reset(){val = 1;}
	inline bool IsSet(){return val != 1;}
	inline uint32 Value(){return val;}
#endif
} __attribute__ ((aligned(16)));//unfortunately ignored for thread local storage

#endif