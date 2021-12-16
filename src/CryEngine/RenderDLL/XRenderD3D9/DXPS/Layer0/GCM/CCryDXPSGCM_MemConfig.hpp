#ifndef __CRYDXPSGCM_MEMCONFIG__
#define __CRYDXPSGCM_MEMCONFIG__

//best for speed
#define CRY_MM_SETUP_4K_16MB
//#define CRY_MM_SETUP_4K_256MB
//#define CRY_MM_SETUP_128_16MB
//#define CRY_MM_SETUP_128_256MB
//best for memory

#if defined(CRY_MM_SETUP_4K_16MB)
enum {CRY_MM_BANK_SIZE			=	16*1024*1024};
enum {CRY_MM_PAGE_SHIFT			=	12};
typedef uint16			tdMemItemID;

#elif defined(CRY_MM_SETUP_4K_256MB)
enum {CRY_MM_BANK_SIZE			=	256*1024*1024};
enum {CRY_MM_PAGE_SHIFT			=	12};
typedef uint32			tdMemItemID;

#elif defined(CRY_MM_SETUP_128_16MB)
enum {CRY_MM_BANK_SIZE			=	16*1024*1024};
enum {CRY_MM_PAGE_SHIFT			=	7};
typedef uint32			tdMemItemID;

#elif defined(CRY_MM_SETUP_128_256MB)
enum {CRY_MM_BANK_SIZE			=	256*1024*1024};
enum {CRY_MM_PAGE_SHIFT			=	7};
typedef uint32			tdMemItemID;

#endif

enum {CRY_MM_MEM_SIZE				=	256*1024*1024};
enum {CRY_MM_PAGE_SIZE			=	1<<CRY_MM_PAGE_SHIFT};
enum {CRY_MM_PAGEBANK_COUNT	=	CRY_MM_BANK_SIZE/CRY_MM_PAGE_SIZE};
enum {CRY_MM_BANK_COUNT			=	CRY_MM_MEM_SIZE/CRY_MM_BANK_SIZE};

enum {CRY_MM_ITEM_COUNT			=	16*1024}; //usually about 2k in use, last time checked



//all memory in 4kb pages, saves half the mem
//enum {CRY_MM_ALIGNMENT			=	CRY_MM_PAGESIZE};
//temporarely to get the initial memory management for animations workin by alexey.
//enum {CRY_MM_RESERVED4ANIMATION	=	40*1024*1024};
enum {CRY_MM_RESERVED4ANIMATION	=	4*1024};


#endif

