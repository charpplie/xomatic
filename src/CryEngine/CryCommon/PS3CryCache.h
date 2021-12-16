////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   PS3CryCache.h
//  Version:     v1.00
//  Created:     02/02/2007 by Michael Glueck.
//  Compilers:   Visual Studio.NET
//  Description: Software Cache spiecific definitions
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef _CRY_CACHE_H_
#define _CRY_CACHE_H_
#pragma once








































































































































































































































#if !defined __CRYCG__
	#define __CRYCG_NOINLINE__
	#if defined __cplusplus
		#if !defined SPU_MAIN_PTR
			#define SPU_MAIN_PTR(PTR) (PTR)
		#endif
		#if !defined SPU_MAIN_REF
			#define SPU_MAIN_REF(REF) (REF)
		#endif
		#if !defined SPU_LOCAL_PTR
			#define SPU_LOCAL_PTR(PTR) (PTR)
		#endif
		#if !defined SPU_LOCAL_REF
			#define SPU_LOCAL_REF(REF) (REF)
		#endif
		#if !defined SPU_LINK_PTR
			#define SPU_LINK_PTR(PTR, LINK) (PTR)
		#endif
		#if !defined SPU_LINK_REF
			#define SPU_LINK_REF(REF, LINK) (PTR)
		#endif
	#endif /* __cplusplus */
	#if !defined SPU_DOMAIN_MAIN
		#define SPU_DOMAIN_MAIN
	#endif
	#if !defined SPU_DOMAIN_LOCAL
		#define SPU_DOMAIN_LOCAL
	#endif
	#if !defined SPU_DOMAIN_LINK
		#define SPU_DOMAIN_LINK(ID)
	#endif
  #if !defined SPU_VERBATIM_BLOCK
    #define SPU_VERBATIM_BLOCK(X) ((void)0)
  #endif 
  #if !defined SPU_FRAME_PROFILER
		#define SPU_FRAME_PROFILER(X){}
  #endif
#endif /* __CRYCG__ */








//prefix of code generator referring to page names (later replaced by IDs)
#define PAGE_PREFIX "__spu_page_id_"
#define DECL_PAGE(a) extern vec_uchar16 __spu_page_id_##a;
#define PAGE_ID(a) __spu_page_id_##a

#define PAGE_CROSS_CALL_PREFIX "__spu_page_fnct_id_"
#define DECL_PAGE_CROSS_CALL(a) extern vec_ushort8 __spu_page_fnct_id_##a;
#define CROSS_CALL_ID(a) __spu_page_fnct_id_##a

#define PAGE_EXT_LIB_CALL_PREFIX "__spu_page_ext_lib_id_"
#define DECL_PAGE_EXT_LIB_CALL(a) extern vec_ushort8 __spu_page_ext_lib_id_##a;
#define EXT_LIB_CALL_ID(a) __spu_page_ext_lib_id_##a

#define PAGE_FUNC_PTR_CALL_PREFIX "__spu_page_fnct_ptr_"

#define SPU_JOB_VAR_PREFIX "__spu_global_job_var_"

//separator for ::
#define GLOB_VAR_SEP "__S"
#define GLOB_VAR_PREFIX "__spu_global_var_"












	#define DECL_GLOB_VAR(a)
	#define RESOLVE_GLOB_VAR_ADDR(a)

	#define DECL_SPU_JOB_VAR(a)
	#define SPU_JOB_VAR(a)



#endif //_CRY_CACHE_H_
