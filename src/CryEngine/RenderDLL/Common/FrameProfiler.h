//////////////////////////////////////////////////////////////////////////
// A simple profiler useful for collecting multiple call times per frame
// and displaying their different average statistics.
// For usage, see the bottom of the file
//////////////////////////////////////////////////////////////////////////


#ifndef _SIMPLE_FRAME_PROFILER_
#define _SIMPLE_FRAME_PROFILER_



// set #if 0 here if you don't want profiling to be compiled in the code
#ifdef ENABLE_FRAME_PROFILER
#define PROFILE_FRAME(id) FRAME_PROFILER_FAST( "Renderer:" #id,iSystem,PROFILE_RENDERER,g_bProfilerEnabled )

#define PROFILE_DIPS_START \
  float fTimeDIP = iTimer->GetAsyncCurTime();           \

#define PROFILE_DIPS_END(id) \
  gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_fTimeDIPs[id] += iTimer->GetAsyncCurTime() - fTimeDIP; \

#define PROFILE_SHADER_START \
  bool bProfile;             \
  float time0 = 0;           \
  int nNumDips = 0;          \
  int nNumPolys = 0;         \
  if (CRenderer::CV_r_profileshaders==1 || (CRenderer::CV_r_profileshaders==2 && gcpRendD3D->m_RP.m_pCurObject && (gcpRendD3D->m_RP.m_pCurObject->m_ObjFlags & FOB_SELECTED))) \
  {                                   \
    bProfile = true;                  \
    gcpRendD3D->FX_Flush();           \
    time0 = iTimer->GetAsyncCurTime(); \
    gcpRendD3D->m_RP.m_fProfileTime = time0; \
    nNumPolys = gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nPolygons[gcpRendD3D->m_RP.m_nPassGroupDIP]; \
    nNumDips = gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nDIPs[gcpRendD3D->m_RP.m_nPassGroupDIP]; \
  }                                   \
  else                                \
    bProfile = false;                 \

#define PROFILE_SHADER_END            \
  float fTime;                        \
  if (bProfile)                       \
  {                                   \
    gcpRendD3D->FX_Flush();           \
    float time1 = iTimer->GetAsyncCurTime(); \
    fTime = time1 - time0;            \
  }                                   \
                                      \
  if (gcpRendD3D->m_RP.m_pShader && gcpRendD3D->m_RP.m_pCurTechnique) \
  {                                   \
    if (CRenderer::CV_r_profileshaders==1 || (CRenderer::CV_r_profileshaders==2 && gcpRendD3D->m_RP.m_pCurObject && (gcpRendD3D->m_RP.m_pCurObject->m_ObjFlags & FOB_SELECTED))) \
    {                                   \
      if (time0 == gcpRendD3D->m_RP.m_fProfileTime) \
      {                                          \
        SProfInfo pi;                     \
        pi.Time = fTime;                  \
        pi.NumPolys = gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nPolygons[gcpRendD3D->m_RP.m_nPassGroupDIP] - nNumPolys;  \
        pi.NumDips = gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_nDIPs[gcpRendD3D->m_RP.m_nPassGroupDIP] - nNumDips;        \
        assert(gcpRendD3D->m_RP.m_pShader);   \
        pi.pShader = gcpRendD3D->m_RP.m_pShader;      \
        pi.pTechnique = gcpRendD3D->m_RP.m_pCurTechnique; \
        pi.m_nItems = 0;                  \
        gcpRendD3D->m_RP.m_Profile.AddElem(pi);  \
      }                                   \
    }                                     \
  }
#else
#define PROFILE_FRAME(id)
#define PROFILE_SHADER_START
#define PROFILE_SHADER_END
#define PROFILE_DIPS_START
#define PROFILE_DIPS_END(id)
#endif

#if defined(ENABLE_FRAME_PROFILER_LABELS)
	#if defined( PS3 )

			#define PROFILE_LABEL(X) gcpRendD3D->GetD3DDevice()->SetMarker(X);
			#define PROFILE_LABEL_PUSH(X) gcpRendD3D->GetD3DDevice()->PushMarker(X);
			#define PROFILE_LABEL_POP(X) gcpRendD3D->GetD3DDevice()->PopMarker(X);
			#define PROFILE_LABEL_SCOPE(X)
			#define	PROFILE_LABEL_PUSH_SKIP_GPU(X)
			#define PROFILE_LABEL_POP_SKIP_GPU(X)

	#elif  defined( XENON )

		#define PROFILE_LABEL(X) PIXSetMarker(0xffffffff, X);
		#define PROFILE_LABEL_PUSH(X) \
																		PIXBeginNamedEvent(0xff00ff00, X); \
																		gcpRendD3D->RT_BeginGPUTimer(X);
		
		#define PROFILE_LABEL_POP(X)	\
																		PIXEndNamedEvent(); \
																		gcpRendD3D->RT_EndGPUTimer(X);
		#define PROFILE_LABEL_SCOPE(X)

		#define PROFILE_LABEL_PUSH_SKIP_GPU(X)	\
																		PIXBeginNamedEvent(0xff00ff00, X); 
		#define PROFILE_LABEL_POP_SKIP_GPU(X)	\
																		PIXEndNamedEvent();

	#else

		#define PROFILE_LABEL(X) { wchar_t buf[128]; size_t outCount=0; mbstowcs_s(&outCount, buf, X, _countof(buf)-1); D3DPERF_SetMarker(0xffffffff, buf); }
		#define PROFILE_LABEL_PUSH(X) { wchar_t buf[128]; size_t outCount=0; mbstowcs_s(&outCount, buf, X, _countof(buf)-1); D3DPERF_BeginEvent(0xff00ff00, buf); }
		#define PROFILE_LABEL_POP(X) D3DPERF_EndEvent();
		#define PROFILE_LABEL_SCOPE(X)
		#define	PROFILE_LABEL_PUSH_SKIP_GPU(X)
		#define PROFILE_LABEL_POP_SKIP_GPU(X)

	#endif
#else
#define PROFILE_LABEL(X)
#define PROFILE_LABEL_PUSH(X)
#define PROFILE_LABEL_POP(X)
#define PROFILE_LABEL_SCOPE(X)
#define	PROFILE_LABEL_PUSH_SKIP_GPU(X)
#define PROFILE_LABEL_POP_SKIP_GPU(X)
#endif

#define PROFILE_LABEL_SHADER(X) PROFILE_LABEL(X)






















#endif