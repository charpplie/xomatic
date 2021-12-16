/**********************************************************************

Filename    :   GMemoryHeap.h
Content     :   Public memory heap class declaration.
Created     :   October 1, 2008
Authors     :   Michael Antonov, Maxim Shemanarev

Copyright   :   (c) 2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GMemoryHeap_H
#define INC_GMemoryHeap_H

#include "GTypes.h"
#include "GAtomic.h"
#include "GStats.h"
#include "GList.h"
#include "GSysAlloc.h"


// *** Declared Classes
//
//------------------------------------------------------------------------
struct GAllocDebugInfo;
class  GMemoryHeap;


// *** Predeclarations
//
//------------------------------------------------------------------------
class  GHeapAllocEngine;
class  GHeapMemVisitor;
class  GHeapRoot;
class  GHeapDebugStorage;
class  GHeapSegVisitor;
struct  GFxAmpMemItem;


// *** Heap-Specific Statistics identifiers
//------------------------------------------------------------------------
enum GStatHeap
{    
    // GStatHeap_Summary is a root stat group for all of heap-custom statistics returned by GStatHeap.
    GStatHeap_Summary = GStatHeap_Start,
        GStatHeap_TotalFootprint,       // Total memory allocated for heap and children.
        GStatHeap_LocalFootprint,       // Total memory allocated for heap.
        GStatHeap_ChildFootprint,       // Total footprint of all child heaps.
        GStatHeap_ChildHeaps,           // The number of child heaps.
        GStatHeap_LocalUsedSpace,       // Used memory within footprint.
        GStatHeap_SysDirectSpace,       // Memory allocated directly from GSysAlloc.
        GStatHeap_Bookkeeping,          // Amount of memory used for heap bookkeeping.
        GStatHeap_DebugInfo,            // Amount of memory used for heap debug information.
        GStatHeap_Segments,             // Number of allocated segments.
        GStatHeap_Granularity,          // Specified heap granularity.
        GStatHeap_DynamicGranularity,   // Current dynamic granularity.
        GStatHeap_Reserve               // Heap reserve.
        // 3 slots left. Check/modify GStatGroup in "GStat.h" for more.
};


// *** Debugging Support
//
// Debugging support is optionally stored with every allocation.
// This consists of the following pieces of info:
//  - StatId, used to identify and group allocation for statistic reporting.
//  - Line + FileName, used to report the memory leaks when heap
//    is released as a whole.
//------------------------------------------------------------------------
struct GAllocDebugInfo
{
    // User-specified identifier for allocation.
    UInt        StatId;

#if defined(GFC_BUILD_DEBUG)
    // Track location in code where allocation took place
    // so that it can be reported to debug leaks.
    UInt        Line;
    const char* pFileName;

    GAllocDebugInfo()
        : StatId(GStat_Default_Mem), Line(0), pFileName("")
    { }
    GAllocDebugInfo(UInt statId, const char* pfile, UInt line)
        : StatId(statId), Line(line), pFileName(pfile)
    { }
    GAllocDebugInfo(const GAllocDebugInfo& src)
        : StatId(src.StatId), Line(src.Line), pFileName(src.pFileName)
    { }

#else

    GAllocDebugInfo() : StatId(GStat_Default_Mem)
    { }
    GAllocDebugInfo(UInt statId, const char*, UInt) : StatId(statId)
    { }
    GAllocDebugInfo(const GAllocDebugInfo& src) : StatId(src.StatId)
    { }

#endif

    // Note that we don't store the original Size here, since it is 
    // passed as a separate argument and we want to avoid such overhead.
};



// ***** GMemoryHeap
//------------------------------------------------------------------------
//
// Memory heap from which allocations take place. This interface supports
// creation of child heaps for dedicated purposes, such that when child
// heaps are released all of their memory is released as well. The benefit
// of child heaps is that they grab memory in large chunks of Granularity
// from the root, such that all of heap-local allocations take place in
// those chunks. This strategy reduces overall fragmentation, since once
// child heap is released it will make all of its large chunks available
// for reuse.
//
// The child memory heap use in GFx relies on ability to determine target heap 
// based on the address within allocation in it, done by AllocAutoHeap
// function. This function takes an address that must be within some other
// allocation of the desired heap and allocated new memory within the same
// heap as that allocation. Note that the passed address does NOT have to
// point to the head of previous allocation, it can also be in the middle
// of it. This functionality allows convenient initialization of aggregate
// structures and arrays located within other objects, their allocations
// will automatically take place within the same heap as the containing
// object.
//
// Internally, heaps rely on GSysAlloc interface for allocating large blocks 
// of system memory; this interface is specified in GFxSystem/GSystem constructor.
// Developers can subsitute GFx allocator by re-implementing that interface. 

class GMemoryHeap : public GListNode<GMemoryHeap>
{
    friend class GHeapRoot;

public:

    enum MemReportType
    {
        MemReportBrief,
        MemReportSummary,
        MemReportMedium,
        MemReportFull,
        MemReportSimple,
        MemReportSimpleBrief,
        MemReportFileSummary,
        MemReportHeapsOnly,
    };

    enum HeapFlags
    {        
        Heap_ThreadUnsafe       = 0x0001,

        // Significantly speeds up small allocations, up to 8*MinAlign,
        // but may result in higher memory overhead. The smallest heap
        // size will typically increase by about 8*PageSize. For the 
        // PageSize=4K it's 32K. If there are thousands of small 
        // allocations existing simultaneously, the extra memory 
        // overhead is low.
        Heap_FastTinyBlocks     = 0x0002,

        // Fixed Granularity means that memory segments will have as small
        // size as possible. It slightly reduces the total footprint and 
        // eases returning memory to the system, but may slow down the
        // allocations. It also makes the heap less realloc-friendly.
        Heap_FixedGranularity   = 0x0004,

        // This flag is set if this is the root (global) memory heap.
        Heap_Root               = 0x0008,

        // Set if this heap keeps debug data in an alternative heap.
        // If not, then this heap does not track debug information.
        Heap_NoDebugInfo        = 0x0010,

        // This flag can be set by the user for debug tool allocations, such as the
        // player HUD.  GFx tools can choose to omit these heaps from information
        // reporting or report them separately. This flag is for user information only,
        // it has no effect on heap implementation.
        Heap_UserDebug          = 0x1000
    };

    enum RootHeapParameters
    {
        RootHeap_MinAlign       = 16,
        RootHeap_Granularity    = 16*1024,
        RootHeap_Reserve        = 16*1024,
        RootHeap_Threshold      = 256*1024,
        RootHeap_Limit          = 0
    };

    struct HeapDesc
    {
        // Capability flags such as thread safety, debug support, etc.
        UInt            Flags;

        // Minimum alignment that will be enforced for all allocations from heap.
        UPInt           MinAlign;

        // Granularity represents the smallest block size that the heap
        // will request from the "root" heap memory system, which ultimately
        // routes to GSysAlloc. In other words, this is the size multiple by
        // which the heap will grow. For example, with the default value of 16K,
        // the first allocation of 10 bytes on the heap will allocate a 16K block;
        // from that point on all of the further heap allocations will be serviced from
        // that block until it is exhausted. Once exhausted, an additional block of
        // granularity will be allocated. If all of the allocation within a block
        // are free, it will similarly be returned to root heap manager for reuse
        // in other heaps or return to GSysAlloc.
        UPInt           Granularity;

        // Smallest maintained reserve, or pre-allocated reserve. Data within
        // the reserve will not be given back to parent heap or system until
        // the heap is released. Must be multiple of Granularity.
        UPInt           Reserve;

        // If the allocation size exceeds this threshold the allocation
        // is being redirected directly to GSysAlloc.
        UPInt           Threshold;

        // If not 0, specifies the limit on the amount of memory used by heap.
        // Any attempt to allocate more will fail returning null. Must be
        // multiple of Granularity.
        UPInt           Limit;

        // An arbitrary integer to associate the heap with some additional
        // information. In particular, the Id is associated with the color to
        // be displayed in the memory monitor. If the Id is 0 there is no 
        // associations, but the heap will be displayed anyway. The only 
        // difference is persistence. Heaps with IDs will have guaranteed 
        // persistent colors to be easily distinguishable from other ones.
        UPInt           HeapId;

        // Memory arena used for this heap. The arena must be created, see
        // GMemoryHeap::CreateArena. Arena 0 is reserved for the use by default.
        UPInt           Arena;

        HeapDesc(UInt  flags = 0,
                 UPInt minAlign = 16,
                 UPInt granularity = 8*1024,
                 UPInt reserve = 8*1024, 
                 UPInt threshold=~UPInt(0),
                 UPInt limit = 0,
                 UPInt heapId = 0,
                 UPInt arena = 0)
            : Flags(flags), MinAlign(minAlign),
              Granularity(granularity), Reserve(reserve),
              Threshold(threshold), Limit(limit), 
              HeapId(heapId), Arena(arena)
        { }

        void            Clear()
        {
            Flags       = 0;
            Granularity = 0;
            MinAlign    = 16;
            Reserve     = 0;
            Threshold   = ~UPInt(0);
            Limit       = 0;
            HeapId      = 0;
            Arena       = 0;
        }
    };

    // Root heap descriptor. This class exists for the sole purpose of using
    // its default constructor for initializing a root heap object, as root default
    // arguments are different from those of child heaps.
    struct RootHeapDesc : public HeapDesc
    {
        RootHeapDesc()
            : HeapDesc(0, RootHeap_MinAlign,
                       RootHeap_Granularity, RootHeap_Reserve,
                       RootHeap_Threshold, RootHeap_Limit, GHeapId_Global)
        { }
    };


    //--------------------------------------------------------------------
    struct HeapInfo
    {
        HeapDesc        Desc;

        // Parent heap, if this describes a nested heap. Null for root heap.
        GMemoryHeap*    pParent;

        // Heap name. Can be in UTF-8.
        char*           pName;
    };

    //--------------------------------------------------------------------
    struct HeapVisitor
    {
        virtual ~HeapVisitor() { }

        // Called for each child heap within parent; the child heap is
        // guaranteed to stay alive during the call as long as parent is alive too.
        // Implementation of Visit is not allowed to allocate memory
        // from childHeap due to a risk of deadlock (it can allocate from
        // parent or other heaps).
        virtual void Visit(GMemoryHeap* parentHeap, GMemoryHeap *childHeap) = 0;
    };


    //--------------------------------------------------------------------
    struct LimitHandler
    {
        virtual ~LimitHandler() {}
        // The handler that is called when the limit is reached. The handler
        // can try to free memory of at least "overLimit" size in summary 
        // (release cached elements, invoke GC, etc).
        // If freeing is not possible or there's not enough size of elements
        // being released the function can increase the heap limit 
        // (GMemoryHeap::SetLimit). In both cases it should return true.
        // If neither is possible the function returns false and then the 
        // allocation call fails (returns 0).
        virtual bool OnExceedLimit(GMemoryHeap* heap, UPInt overLimit) = 0;

        // The function is called when the segment is freeing. It allows the
        // application algorithm to decrease the limit when necessary.
        virtual void OnFreeSegment(GMemoryHeap* heap, UPInt freeingSize) = 0;
    };

    //--------------------------------------------------------------------
    struct HeapTracer
    {
        virtual ~HeapTracer() {}
        virtual void OnCreateHeap(const GMemoryHeap* heap) = 0;
        virtual void OnDestroyHeap(const GMemoryHeap* heap) = 0;
        virtual void OnAlloc(const GMemoryHeap* heap, UPInt size, UPInt align, UInt sid, const void* ptr) = 0;
        virtual void OnRealloc(const GMemoryHeap* heap, const void* oldPtr, UPInt newSize, const void* newPtr) = 0;
        virtual void OnFree(const GMemoryHeap* heap, const void* ptr) = 0;
    };

    //--------------------------------------------------------------------
    struct RootStats
    {
        UPInt SysMemFootprint;
        UPInt SysMemUsedSpace;
        UPInt PageMapFootprint;
        UPInt PageMapUsedSpace;
        UPInt BookkeepingFootprint;
        UPInt BookkeepingUsedSpace;
        UPInt DebugInfoFootprint;
        UPInt DebugInfoUsedSpace;
        UPInt UserDebugFootprint;
        UPInt UserDebugUsedSpace;
    };

private:
    GMemoryHeap();  // Explicit creation and destruction is prohibited
    ~GMemoryHeap() {}

public:

    // *** Initial bootstrapping and final clean-up.
    //
    //--------------------------------------------------------------------
    static void GSTDCALL Init(GSysAlloc* sysAlloc);
    static void GSTDCALL CleanUp();

    // *** Operations with memory arenas
    //
    //--------------------------------------------------------------------
    static void GSTDCALL CreateArena(UPInt arena, GSysAlloc* sysAlloc);
    static void GSTDCALL DestroyArena(UPInt arena);
    static bool GSTDCALL ArenaIsEmpty(UPInt arena);

    // *** Initialization
    //
    // Creates a nested child heap; The heap should be destroyed 
    // by calling release. If child heap creation failed due to 
    // out-of-memory condition, returns 0. If child heap creation 
    // is not supported a pointer to the same parent heap will be returned.
    //--------------------------------------------------------------------
    GMemoryHeap*    CreateHeap(const char* name, 
                               const HeapDesc& desc);

    GMemoryHeap*    CreateHeap(const char* name, 
                               UInt  flags = 0,
                               UPInt minAlign = 16,
                               UPInt granularity = 16*1024,
                               UPInt reserve = 16*1024, 
                               UPInt threshold=~UPInt(0), 
                               UPInt limit = 0, 
                               UPInt heapId = 0,
                               UPInt arena = 0)
    {
        HeapDesc desc(flags, minAlign, granularity, reserve, threshold, limit, heapId, arena);
        return CreateHeap(name, desc);
    }

    // *** Service functions
    //--------------------------------------------------------------------

    // Fills in heap descriptor with information.
    void            GetHeapInfo(HeapInfo* infoPtr) const;

    const char*     GetName()       const { return Info.pName;      }
    UPInt           GetId()         const { return Info.Desc.HeapId;}
    GMemoryHeap*    GetParentHeap() const { return Info.pParent;    }
    UInt            GetFlags()      const { return Info.Desc.Flags; }
    UPInt           GetGranularity()const { return Info.Desc.Granularity; }

    void            SetLimitHandler(LimitHandler* handler);
    void            SetLimit(UPInt newLimit);
    UPInt           GetLimit()      const { return Info.Desc.Limit; }

    // Determines if the heap is thread safe or not. One benefit of thread safe heap
    // is that its stats can be obtained from another thread during child heap iteration.
    inline  bool    IsThreadSafe()  const 
    { 
        return (Info.Desc.Flags & Heap_ThreadUnsafe) == 0; 
    }

    // Child heap lifetime is reference counted.
    //  These function need to be implemented when child heaps are supported.

    // Increments heap reference count; this function is primarily useful
    // when enumerating heaps.
    void            AddRef();

    // Releases this heap, deallocating it unless there are other references.
    // Other references can come either from child heaps or multiple external
    // references to the heap.
    // Internal allocations are NOT considered references for heap lifetime.
    // Release should only be called for heaps created with CreateHeap.
    void            Release();

    // Marks allocation for automatic cleanup of heap. When this allocation
    // is freed, the entire heap will be released. Allocation must belong
    // to this heap, and should usually be the last item to be freed.
    void            ReleaseOnFree(void *ptr);

    // Creates the root heap. The function can be called only once. 
    // The second call will return 0. Call ReleaseRootHeap before creating 
    // another root heap.
    static GMemoryHeap* GSTDCALL CreateRootHeap();
    static GMemoryHeap* GSTDCALL CreateRootHeap(const HeapDesc& desc);

    // Releases root heap and/or de-initializes it; intended to be the opposite
    // of the heap-specific CreateRootHeap() static function. ReleaseRootHeap is
    // called during GFxSystem::Destroy before the global heap is reset.
    // Its implementation may or may not include ref-counting semantics.
    static void     GSTDCALL ReleaseRootHeap();

    // Assign heap to current thread causing ASSERTs if called for other thread
    void            AssignToCurrentThread();


    // *** Allocation API
    //--------------------------------------------------------------------
    void*   Alloc(UPInt size, const GAllocDebugInfo* info = 0);
    void*   Alloc(UPInt size, UPInt align, const GAllocDebugInfo* info = 0);

    // Reallocates memory; this call does not maintain custom alignment specified
    // during allocation.
    static void*    GSTDCALL Realloc(void* oldPtr, UPInt newSize); 
    static void     GSTDCALL Free(void* ptr);

    // Allocate while automatically identifying heap and allocation id based on 
    // the specified address.
    static void*    GSTDCALL AllocAutoHeap(const void *thisPtr, UPInt size,
                                  const GAllocDebugInfo* info = 0);


    static void*    GSTDCALL AllocAutoHeap(const void *thisPtr, UPInt size, UPInt align,
                                  const GAllocDebugInfo* info = 0);

    // Determine which heap allocation belongs to. 
    // This function will ASSERT internally if the specified
    // address does not come from one of allocated GFx heaps.
    static GMemoryHeap* GSTDCALL GetAllocHeap(const void *thisPtr);

    // Returns the actual allocation size that can be safely used.
    static UPInt    GSTDCALL GetUsableSize(const void* ptr);

    // The heap can hold a memory segment in its cache to reduce system 
    // memory thrashing. This function releases the cached segments from 
    // all heaps. It makes sense to call this function after some big 
    // unloads to return as much memory to the system as possible.
    static void     GSTDCALL ReleaseCachedMem();

    // Alloc and free directly from the system allocator. These functions
    // are used only for debugging and/or visualization when it's necessary
    // to allocate some large amount of memory with absolute minimal 
    // interference with the existing memory layout. Address alignment
    // is only guaranteed to be GSysAlloc::Info::MinAlign. These functions 
    // must be used with care!
    static void* GSTDCALL AllocSysDirect(UPInt size);
    static void  GSTDCALL FreeSysDirect(void* ptr, UPInt size);

    // *** Statistics
    //
    //--------------------------------------------------------------------
    // Obtains Memory statistics for the heap. Returns false if the 
    // statistics is not supported.
    bool            GetStats(GStatBag* bag);

    // Return the number of bytes allocated from the system
    // and the number of actually allocated bytes in the heap.
    // GetTotalUsedSpace() recursively iterates through all child 
    // heaps and sums up the total used space.
    UPInt           GetFootprint() const;
    UPInt           GetTotalFootprint() const;
    UPInt           GetUsedSpace() const;
    UPInt           GetTotalUsedSpace() const;
    static void     GSTDCALL GetRootStats(RootStats* stats);

    // Traverse all child heaps and dump memory leaks. If no debug info 
    // is present just report alive child heaps. Returns true if memory 
    // leaks have been detected.
    bool            DumpMemoryLeaks();

    static void     GSTDCALL UltimateCheck();

    // Enumerates all of the child heaps, by calling HeapVisitor::Visit.
    void            VisitChildHeaps(HeapVisitor* visitor);

    // DBG: To be removed in future.
    // See GHeapMemVisitor::VisitingFlags for "flags" argument.
    void            VisitMem(GHeapMemVisitor* visitor, UInt flags);

    static void     GSTDCALL VisitRootSegments(GHeapSegVisitor* visitor);
           void     VisitHeapSegments(GHeapSegVisitor* visitor) const;

    static void     GSTDCALL CheckIntegrity();

    static void     GSTDCALL SetTracer(HeapTracer* tracer);

    // Forming the String containing general or detailed information about heaps.
    static void     GSTDCALL MemReport(class GStringBuffer& buffer, MemReportType detailed);
    static void     GSTDCALL MemReport(class GFxLog* pLog, MemReportType detailed);
    static void     GSTDCALL MemReport(GFxAmpMemItem* rootItem, MemReportType detailed);

private:
    //--------------------------------------------------------------------
    void  destroyItself();
    void* allocMem(UPInt size, const GAllocDebugInfo* info);
    void* allocMem(UPInt size, UPInt align, const GAllocDebugInfo* info);
    void* allocMem(const void *thisPtr, UPInt size, const GAllocDebugInfo* info);
    void* allocMem(const void *thisPtr, UPInt size, UPInt align, const GAllocDebugInfo* info);
    void* reallocMem(struct GHeapSegment* seg, void* oldPtr, UPInt newSize);
    void  ultimateCheck();
    void  releaseCachedMem();
    bool  dumpMemoryLeaks();
    void  checkIntegrity() const;
    void  getUserDebugStats(RootStats* stats) const;

    typedef GList<GMemoryHeap> ChildListType;

    UPInt                   SelfSize;
    volatile UInt           RefCount;
    UPInt                   OwnerThreadId;

    // Pointer to allocation that will cause this heap
    // to be automatically released when freed.
    void*                   pAutoRelease;

    HeapInfo                Info;
    ChildListType           ChildHeaps;
    mutable GLock           HeapLock;
    bool                    UseLocks;
    bool                    TrackDebugInfo;
    GHeapAllocEngine*       pEngine;
    GHeapDebugStorage*      pDebugStorage;
};



#endif // INC_GMemoryHeap_H
