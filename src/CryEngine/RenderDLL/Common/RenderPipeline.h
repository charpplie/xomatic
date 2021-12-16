
/*=============================================================================
RenderPipeline.h : Shaders pipeline declarations.
Copyright (c) 2001 Crytek Studios. All Rights Reserved.

Revision history:
* Created by Honitch Andrey

=============================================================================*/

#ifndef __RENDERPIPELINE_H__
#define __RENDERPIPELINE_H__

//====================================================================

#define MAX_HWINST_PARAMS 32768

#ifndef PS3
#define MAX_REND_OBJECTS 16384



#endif

#if !defined(XENON) && !defined(PS3)
#define MAX_PERM_OBJECTS 32768


#endif

#define MAX_REND_SHADERS 4096
#define MAX_REND_SHADER_RES 16384
#define MAX_REND_LIGHTS  32
#define MAX_DEFERRED_LIGHTS 256
#define SG_SORT_GROUP 0
#define MAX_SHADOWMAP_LOD 8 
#define MAX_SHADOWMAP_FRUSTUMS (2*(MAX_REND_LIGHTS+MAX_DEFERRED_LIGHTS)*MAX_SHADOWMAP_LOD)
#define MAX_SORT_GROUPS  64
#define MAX_REND_LIGHT_GROUPS (MAX_REND_LIGHTS/4)
#define MAX_INSTANCES_THRESHOLD_HW  8
#define MAX_LIST_ORDER   2

struct SViewport
{
	int nX, nY, nWidth, nHeight;
	float fMinZ, fMaxZ;
	SViewport() : nX(0), nY(0), nWidth(0), nHeight(0) {}

	SViewport(int nNewX, int nNewY, int nNewWidth, int nNewHeight)
	{
		nX = nNewX;
		nY = nNewY;
		nWidth = nNewWidth;
		nHeight = nNewHeight;
	}
	_inline friend bool operator != (const SViewport &m1, const SViewport &m2)
	{
		if (m1.nX!=m2.nX || m1.nY!=m2.nY || m1.nWidth!=m2.nWidth || m1.nHeight!=m2.nHeight || m1.fMinZ!=m2.fMinZ || m1.fMaxZ!=m2.fMaxZ)
			return true;
		return false;
	}
};


struct SRenderListDesc
{
	SViewport m_VP;
	int m_nStartRI[MAX_LIST_ORDER][EFSLIST_NUM];
	int m_nEndRI[MAX_LIST_ORDER][EFSLIST_NUM];
	int m_nBatchFlags[MAX_LIST_ORDER][EFSLIST_NUM];
};

typedef union UnINT64
{
	uint64 SortVal;
	struct
	{
		uint32 Low;
		uint32 High;
	}i;
} UnINT64;

struct SRendLightGroup
{
	DynArray<uint32> RendItemsShadows[4];
	DynArray<uint32> RendItemsLights;

	uint32 m_GroupLightMask;
	SRendLightGroup()
	{
		Reset();
	}

	void Reset()
	{
		for (int i=0; i<4; i++)
		{
			RendItemsShadows[i].resize(0);
		}
		RendItemsLights.resize(0);
		m_GroupLightMask = 0;
	}
};

// FIXME: probably better to sort by shaders (Currently sorted by resources)
_MS_ALIGN(8) struct SRendItem
{  
	uint32 SortVal;
	CRendElementBase *Item;
	union
	{
		uint32 ObjSort;
		float fDist;
	};
	CRenderObject *pObj;
	uint32 DynLMask;
	uint32 nBatchFlags;
	uint16 nOcclQuery;

	union QuickCopy
	{
		uint64	data[4];
		char		bytes[32];
	};

#if (defined(PS3) && !defined(__SPU__)) || defined(XENON)

	SRendItem (const SRendItem& cpy)
	{
		//explicit for 32 bytes in size, 8 byte align
		//allocator in std::stable_sort prevents using 16byte align
		COMPILE_TIME_ASSERT(sizeof(SRendItem)==32);

		//Union used to avoid problems with strict aliasing
		const QuickCopy *src = (const QuickCopy*)&cpy;
		QuickCopy *dest = (QuickCopy*)this;

		uint64 data0 = src->data[0];
		uint64 data1 = src->data[1];
		uint64 data2 = src->data[2];
		uint64 data3 = src->data[3];
		dest->data[0] = data0;
		dest->data[1] = data1;
		dest->data[2] = data2;
		dest->data[3] = data3;
	}

	const SRendItem& operator = (const SRendItem& cpy)
	{
		const QuickCopy *src = (const QuickCopy*)&cpy;
		QuickCopy *dest = (QuickCopy*)this;

		uint64 data0 = src->data[0];
		uint64 data1 = src->data[1];
		uint64 data2 = src->data[2];
		uint64 data3 = src->data[3];
		dest->data[0] = data0;
		dest->data[1] = data1;
		dest->data[2] = data2;
		dest->data[3] = data3;

		return *this;
	}
#endif


	//==================================================
	static void *mfGetPointerCommon(ESrcPointer ePT, int *Stride, EParamType Type, ESrcPointer Dst, int Flags);

	static _inline void mfAdd(int nFillID, CRendElementBase *Item, CRenderObject *__restrict pObj, const SShaderItem& pSH, uint32 nList, int nAW, uint32 nBatchFlags)
	{
		SRendItem* __restrict ri = RendItems(nFillID,nAW,nList).AddIndex(1);
		ri->pObj = pObj;
		ri->nOcclQuery = -1;
		ri->ObjSort = (pObj->m_ObjFlags & 0xffff0000) | pObj->m_nSort;
		ri->DynLMask = pObj->m_DynLMMask[nFillID];
		assert (!(pSH.m_nPreprocessFlags & 0xffff));
		ri->nBatchFlags = nBatchFlags | pSH.m_nPreprocessFlags;
		m_BatchFlags[m_RecurseLevel[nFillID]-1][nAW][nList] |= ri->nBatchFlags;
		int nResID = pSH.m_pShaderResources ? ((SRenderShaderResources *)(pSH.m_pShaderResources))->m_Id : 0;

		assert(nResID<CShader::m_ShaderResources_known.size());

		CShader *pShader = (CShader *)pSH.m_pShader;
		ri->SortVal = (nResID<<18) | (pShader->mfGetID()<<6) | (pSH.m_nTechnique&0x3f);
		//ri->SortVal = (pShader->mfGetID()<<20) | (nResID<<6) | (pSH.m_nTechnique&0x3f);
		ri->Item = Item;
	}
	static _inline void mfGet(uint32 nVal, int& nTechnique, CShader*& Shader, SRenderShaderResources*& Res)
	{
		Shader = (CShader *)CShaderMan::m_pContainer->m_RList[(nVal>>6) & (MAX_REND_SHADERS-1)];
		//Shader = (CShader *)CShaderMan::m_pContainer->m_RList[(nVal>>20) & (MAX_REND_SHADERS-1)];
		nTechnique = (nVal & 0x3f);
		if (nTechnique == 0x3f)
			nTechnique = -1;
		int nRes = (nVal>>18) & (MAX_REND_SHADER_RES-1);
		//int nRes = (nVal>>6) & (MAX_REND_SHADER_RES-1);
		Res = (nRes) ? CShader::m_ShaderResources_known[nRes] : NULL;
	}
	static _inline CShader *mfGetShader(uint32 flag)
	{
		return (CShader *)CShaderMan::m_pContainer->m_RList[(flag>>6) & (MAX_REND_SHADERS-1)];
		//return (CShader *)CShaderMan::m_pContainer->m_RList[(flag>>20) & (MAX_REND_SHADERS-1)];
	}
	static bool IsListEmpty(int nList, int nProcessID, SRenderListDesc *pRLD)
	{
		int nREs = pRLD->m_nEndRI[0][nList] - pRLD->m_nStartRI[0][nList];
		nREs += pRLD->m_nEndRI[1][nList] - pRLD->m_nStartRI[1][nList];

		if (!nREs)
			return true;
		return false;
	}

	static bool IsListEmpty(int nList, int nProcessID, SRenderListDesc *pRLD, int nAW)
	{
		int nREs = pRLD->m_nEndRI[nAW][nList] - pRLD->m_nStartRI[nAW][nList];

		if (!nREs)
			return true;
		return false;
	}

	static uint32 BatchFlags(int nList, int nProcessID, SRenderListDesc *pRLD)
	{
		uint32 nBatchFlags = 0;
		int nREs = pRLD->m_nEndRI[0][nList] - pRLD->m_nStartRI[0][nList];
		if (nREs)
			nBatchFlags |= pRLD->m_nBatchFlags[0][nList];
		nREs = pRLD->m_nEndRI[1][nList] - pRLD->m_nStartRI[1][nList];
		if (nREs)
			nBatchFlags |= pRLD->m_nBatchFlags[1][nList];
		return nBatchFlags;
	}

	static void mfGenerateLightGroupsOpaque(SRendItem *First, int Num);
	static void mfGenerateLightGroupsTransparent(SRendItem *First, int Num);

	// Sort by SortVal member of RI
	static void mfSortPreprocess(SRendItem *First, int Num);
	// Sort by distance
	static void mfSortByDist(SRendItem *First, int Num, bool bDecals,bool InvertedOrder=false);
	// Sort by light
	static void mfSortByLight(SRendItem *First, int Num, bool bSort, const bool bIgnoreRePtr, bool bSortDecals);

	void GetMemoryUsage(ICrySizer *pSizer) const {}

	static TDualVal<int> m_RenderView;
	static TDualVal<int> m_RecurseLevel;
	static int m_AppStartRI[MAX_REND_RECURSION_LEVELS][MAX_LIST_ORDER][EFSLIST_NUM];
#ifdef STRIP_RENDER_THREAD
	static TArray<SRendItem> m_RendItems[MAX_LIST_ORDER][EFSLIST_NUM];
	static TArray<SRendItem>& RendItems(int threadIndex, int listOrder, int listNum)
	{
		return m_RendItems[listOrder][listNum];
	}
#else
	static TArray<SRendItem> m_RendItems[2][MAX_LIST_ORDER][EFSLIST_NUM];
	static TArray<SRendItem>& RendItems(int threadIndex, int listOrder, int listNum)
	{
		return m_RendItems[threadIndex][listOrder][listNum];
	}
#endif

	static uint32 m_ShadowsValidMask[MAX_REND_RECURSION_LEVELS][MAX_REND_LIGHT_GROUPS];
	static SRendLightGroup m_RenderLightGroups[MAX_SORT_GROUPS][MAX_REND_LIGHT_GROUPS+1];

	static TMultiVal<int, MAX_REND_LIGHTS+MAX_DEFERRED_LIGHTS> m_StartFrust;
	static TMultiVal<int, MAX_REND_LIGHTS+MAX_DEFERRED_LIGHTS> m_EndFrust;
	static TMultiVal<int, MAX_SHADOWMAP_FRUSTUMS> m_ShadowsStartRI;
	static TMultiVal<int, MAX_SHADOWMAP_FRUSTUMS> m_ShadowsEndRI;

	static int m_nSortGroups;
	static uint32 m_BatchFlags[MAX_REND_RECURSION_LEVELS][MAX_LIST_ORDER][EFSLIST_NUM];

} _ALIGN (8);

struct SRefSprite
{
	CRenderObject *m_pObj;
	void GetMemoryUsage( ICrySizer *pSizer ) const { /*LATER*/}
};

//==================================================================

struct SShaderPass;

union UPipeVertex
{
	void               *Ptr;
	byte               *PtrB;
	float              *PtrF;

	Vec3                                    *VBPtr_0;
	SVF_P3F_C4B_T2F *VBPtr_4;
	SVF_TP3F_C4B_T2F *VBPtr_5;
	SPipTangents                            *VBPtr_6;
	SVF_T2F              *VBPtr_7;
	SVF_P3F_C4B_I4B_PS4F *VBPtr_8;
};

//==================================================================

#define MAX_DYNVBS 4

//==================================================================
// StencilStates

#define FSS_STENCFUNC_ALWAYS   0x0
#define FSS_STENCFUNC_NEVER    0x1
#define FSS_STENCFUNC_LESS     0x2
#define FSS_STENCFUNC_LEQUAL   0x3
#define FSS_STENCFUNC_GREATER  0x4
#define FSS_STENCFUNC_GEQUAL   0x5
#define FSS_STENCFUNC_EQUAL    0x6
#define FSS_STENCFUNC_NOTEQUAL 0x7
#define FSS_STENCFUNC_MASK     0x7

#define FSS_STENCIL_TWOSIDED   0x8

#define FSS_CCW_SHIFT          16

#define FSS_STENCOP_KEEP    0x0
#define FSS_STENCOP_REPLACE 0x1
#define FSS_STENCOP_INCR    0x2
#define FSS_STENCOP_DECR    0x3
#define FSS_STENCOP_ZERO    0x4
#define FSS_STENCOP_INCR_WRAP 0x5
#define FSS_STENCOP_DECR_WRAP 0x6

#define FSS_STENCFAIL_SHIFT   4
#define FSS_STENCFAIL_MASK    (0x7 << FSS_STENCFAIL_SHIFT)

#define FSS_STENCZFAIL_SHIFT  8
#define FSS_STENCZFAIL_MASK   (0x7 << FSS_STENCZFAIL_SHIFT)

#define FSS_STENCPASS_SHIFT   12
#define FSS_STENCPASS_MASK    (0x7 << FSS_STENCPASS_SHIFT)

#define STENC_FUNC(op) (op)
#define STENC_CCW_FUNC(op) (op << FSS_CCW_SHIFT)
#define STENCOP_FAIL(op) (op << FSS_STENCFAIL_SHIFT)
#define STENCOP_ZFAIL(op) (op << FSS_STENCZFAIL_SHIFT)
#define STENCOP_PASS(op) (op << FSS_STENCPASS_SHIFT)
#define STENCOP_CCW_FAIL(op) (op << (FSS_STENCFAIL_SHIFT+FSS_CCW_SHIFT))
#define STENCOP_CCW_ZFAIL(op) (op << (FSS_STENCZFAIL_SHIFT+FSS_CCW_SHIFT))
#define STENCOP_CCW_PASS(op) (op << (FSS_STENCPASS_SHIFT+FSS_CCW_SHIFT))

//==================================================================

#define GS_HIZENABLE             0x00010000
//==================================================================

struct SOnDemandD3DStreamProperties
{
#if defined (DIRECT3D9)
	D3DVERTEXELEMENT9 *m_pElements;					// terminated by .Stream == 0xff
#elif defined (DIRECT3D10)
	D3D11_INPUT_ELEMENT_DESC *m_pElements;
	uint32 m_nNumElements;
#endif
};

struct SOnDemandD3DVertexDeclaration
{
#if defined (DIRECT3D9)
	TArray<D3DVERTEXELEMENT9> m_Declaration;  
#elif defined (DIRECT3D10)
	TArray<D3D11_INPUT_ELEMENT_DESC> m_Declaration;  
#endif
};

struct SOnDemandD3DVertexDeclarationCache
{
#if defined (DIRECT3D9)
	LPDIRECT3DVERTEXDECLARATION9 m_pDeclaration;
#elif defined (DIRECT3D10)
	ID3D11InputLayout *m_pDeclaration;	
#endif
};

#if defined(DIRECT3D9) || defined(DIRECT3D10)

template <class IndexType> class DynamicIB;

template < class VertexType > class DynamicVB;
template < class VertexType > class StaticVB;
union UDynamicVB
{
	DynamicVB <Vec3>                                    *VBPtr_0;
	DynamicVB <SVF_P3F_C4B_T2F>   *VBPtr_4;
	DynamicVB <SVF_P3F_C4B_I4B_PS4F>   *VBPtr_5;
	DynamicVB <SVF_TP3F_C4B_T2F> *VBPtr_6;
	DynamicVB <SPipTangents>                            *VBPtr_11;
	DynamicVB <SVF_T2F>              *VBPtr_12;
};
struct SVertexDeclaration
{
	int StreamMask;
	int VertFormat;
	int InstAttrMask;
#if defined (DIRECT3D9)
	TArray<D3DVERTEXELEMENT9> m_Declaration;  
	LPDIRECT3DVERTEXDECLARATION9 m_pDeclaration;
#elif defined (DIRECT3D10)
	TArray<D3D11_INPUT_ELEMENT_DESC> m_Declaration;  
	ID3D11InputLayout* m_pDeclaration;
#endif
};

#endif

#if defined(DIRECT3D9)
struct SFSAA
{
	D3DMULTISAMPLE_TYPE Type;
	DWORD Quality;
	LPDIRECT3DSURFACE9 m_pZBuffer;
};
#elif defined(DIRECT3D10)
struct SFSAA
{
	UINT Type;
	DWORD Quality;
	D3DTexture *m_pDepthTex;
	ID3D11DepthStencilView *m_pZBuffer;
};
#endif

struct SProfInfo
{
	int NumPolys;
	int NumDips;
	CShader *pShader;
	SShaderTechnique *pTechnique;
	double Time;
	int m_nItems;
	SProfInfo()
	{
		NumPolys = 0;
		NumDips = 0;
		pShader = NULL;
		pTechnique = NULL;
		m_nItems = 0;
	}
};

struct SPlane
{
	byte m_Type;
	byte m_SignBits;
	Vec3 m_Normal;
	float m_Dist;
	void Init()
	{
		if ( m_Normal[0] == 1.0f )
			m_Type = PLANE_X;
		if ( m_Normal[1] == 1.0f )
			m_Type = PLANE_Y;
		if ( m_Normal[2] == 1.0f )
			m_Type = PLANE_Z;
		else
			m_Type = PLANE_NON_AXIAL;

		// for fast box on planeside test
		int bits = 0;
		int j;
		for (j=0; j<3; j++)
		{
			if (m_Normal[j] < 0)
				bits |= 1<<j;
		}
		m_SignBits = bits;
	}
};

struct SStatItem
{
	int m_Nums;
	double m_fTime;
};

#define STARTPROFILE(Item) { ticks(Item.m_fTime); Item.m_Nums++; }
#define ENDPROFILE(Item) { unticks(Item.m_fTime); }

struct SRTargetStat
{
	string m_Name;
	uint32 m_nSize;
	uint32 m_nWidth;
	uint32 m_nHeight;
	ETEX_Format m_eTF;

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(m_Name);
	}
};

struct SPipeStat
{
	int m_NumRendBatches;
	int m_NumRendInstances;
	int m_NumRendHWInstances;
	int m_RendHWInstancesPolysAll;
	int m_RendHWInstancesPolysOne;
	int m_RendHWInstancesDIPs;
	int m_NumLightSetups;
	int m_NumTextChanges;
	int m_NumRTChanges;
	int m_NumStateChanges;
	int m_NumRendSkinnedObjects;
	int m_NumVShadChanges;
	int m_NumPShadChanges;
	int m_NumGShadChanges;
	int m_NumVShaders;
	int m_NumPShaders;
	int m_NumGShaders;
	int m_NumRTs;
	int m_RTCleared;
	int m_RTClearedSize;
	int m_RTCopied;
	int m_RTCopiedSize;
	int m_RTSize;
	int m_NumSprites;
	int m_NumSpriteDIPS;
	int m_NumSpritePolys;
	int m_NumSpriteUpdates;

	int m_NumQIssued;
	int m_NumQOccluded;
	int m_NumQNotReady;

	int m_NumMergedPolys;

	int m_NumPSInstructions;
	int m_NumVSInstructions;
	CHWShader *m_pMaxPShader;
	CHWShader *m_pMaxVShader;
	void *m_pMaxPSInstance;
	void *m_pMaxVSInstance;

	int m_NumTextures;
	int m_ManagedTexturesSize;
	int m_ManagedTexturesFullSize;
	int m_DynTexturesSize;
	int m_MeshUpdateBytes;
	int m_DynMeshUpdateBytes;
	float m_fOverdraw;
	float m_fSkinningTime;
	float m_fPreprocessTime;
	float m_fSceneTime;
	float m_fTexUploadTime;
	float m_fTexRestoreTime;
	float m_fOcclusionTime;
	float m_fRenderTime;
	int m_NumNotFinishFences;
	int m_NumFences;
	float m_fEnvCMapUpdateTime;
	float m_fEnvTextUpdateTime;

	int m_NumImpostersUpdates;
	int m_NumCloudImpostersUpdates;
	int m_NumImpostersDraw;
	int m_NumCloudImpostersDraw;
	int m_ImpostersSizeUpdate;
	int m_CloudImpostersSizeUpdate;

	int m_nDIPs[EFSLIST_NUM];
	int m_nPolygons[EFSLIST_NUM];
	float m_fTimeDIPs[EFSLIST_NUM];
	float m_fTimeDIPsZ;
	float m_fTimeDIPsAO;
	float m_fTimeDIPsRAIN;
	float m_fTimeDIPsDeferredLayers;
	float m_fTimeDIPsSprites;

	int m_nDIPs_DebugNode[EFSLIST_NUM];
};

//Batch flags
#define FB_GENERAL        1
#define FB_TRANSPARENT    2
#define FB_DETAIL         4
#define FB_ZPREPASS       8
#define FB_Z              0x10
#define FB_GLOW           0x20
#define FB_SCATTER        0x40
#define FB_PREPROCESS     0x80
#define FB_MOTIONBLUR     0x100
#define FB_REFRACTIVE     0x200
#define FB_MULTILAYERS    0x400
#define FB_CAUSTICS       0x800
#define FB_CUSTOM_RENDER  0x1000
#define FB_RAIN           0x2000
#define FB_FUR            0x4000
#define FB_LAYER_EFFECT   0x8000
#define FB_DEBUG          0x10000

// m_RP.m_Flags
#define RBF_2D               0x10

#define RBF_MODIF_TC         0x1000
#define RBF_MODIF_VERT       0x2000
#define RBF_MODIF_COL        0x4000
#define RBF_MODIF_MASK       0xf000

#define RBF_NEAREST          0x10000
#define RBF_CUSTOM_CAMERA    0x20000
#define RBF_3D               0x40000
#define RBF_SHOWLINES        0x80000

// m_RP.m_TI.m_PersFlags
#define RBPF_USESTREAM       (1<<0)
#define RBPF_USESTREAM_MASK  ((1<<VSF_NUM)-1)
#define RBPF_DRAWTOTEXTURE   (1<<16)
#define RBPF_MIRRORCAMERA    (1<<17)
#define RBPF_MIRRORCULL      (1<<18)

#define RBPF_ZPASS             (1<<19)
#define RBPF_SHADOWGEN         (1<<20)

#define RBPF_FP_DIRTY          (1<<21)
#define RBPF_FP_MATRIXDIRTY    (1<<22)

#define RBPF_WASWORLDSPACE     (1<<23)
#define RBPF_MAKESPRITE        (1<<24)
#define RBPF_MULTILIGHTS       (1<<25)
#define RBPF_HDR               (1<<26)
#define RBPF_POSTPROCESS       (1<<27)
#define RBPF_IN_CLEAR          (1<<28)
#define RBPF_ALLOW_AO          (1<<29)
#define RBPF_OBLIQUE_FRUSTUM_CLIPPING  (1<<30)


// m_RP.m_TI.m_PersFlags2
#define RBPF2_NOSHADERFOG      (1<<0)			// 0x001
#define RBPF2_OCEANPARTICLES       (1<<1)			// 0x002
#define RBPF2_NOALPHABLEND     (1<<2)			// 0x004
#define RBPF2_IMPOSTERGEN      (1<<3)			// 0x008
#define RBPF2_RAINPASS         (1<<4)			// 0x020
#define RBPF2_FURPASS    (1<<5)			// 0x020
#define RBPF2_NO_SRGBWRITES   (1<<6)
#define RBPF2_LOCKSTENCIL      (1<<7)			// 0x080
#define RBPF2_VSM              (1<<8)			// 0x100
#define RBPF2_HDR_FP16         (1<<9)			// 0x200
#define RBPF2_IN_PREDICATED_TILING (1<<10)			// 0x400
#define RBPF2_COMMIT_CM        (1<<13)
#define RBPF2_ZPREPASS         (1<<14)
#define RBPF2_SCATTERPASS      (1<<15)
#define RBPF2_CLEAR_SHADOW_MASK (1<<16)

#define RBPF2_DONTDRAWNEAREST (1<<17)
#define RBPF2_NOALPHATEST     (1<<18)
#define RBPF2_DRAWLIGHTS      (1<<19)
#define RBPF2_ALLOW_DEFERREDSHADING  (1<<20)

#define RBPF2_COMMIT_PF       (1<<21)
#define RBPF2_SETCLIPPLANE    (1<<22)
#define RBPF2_DRAWTOCUBE      (1<<23)

#define RBPF2_MOTIONBLURPASS     (1<<24)
#define RBPF2_MATERIALLAYERPASS  (1<<25)
#define RBPF2_DISABLECOLORWRITES (1<<26)
#define RBPF2_CUSTOM_RENDER_PASS (1<<27)
#define RBPF2_ENCODE_HDR         (1<<28)
#define RBPF2_LIGHTSHAFTS        (1<<29)
#define RBPF2_ATOC              (1<<30)
#define RBPF2_LIGHTSTENCILCULL  (1<<31)

// m_RP.m_FlagsPerFlush
#define RBSI_NOCULL          0x2
#define RBSI_DRAWAS2D        0x8
#define RBSI_FOGVOLUME       0x80
#define RBSI_INDEXSTREAM     0x1000
#define RBSI_SHADOWPASS      0x10000
#define RBSI_VERTSMERGED     0x100000
#define RBSI_LMTCMERGED      0x200000
#define RBSI_TANGSMERGED     0x400000
#define RBSI_FURPASS         0x800000
#define RBSI_USE_LM          0x2000000
#define RBSI_ENABLESTENCIL   0x4000000
#define RBSI_INSTANCED       0x10000000

// m_RP.m_ShaderLightMask
#define SLMF_DIRECT       0
#define SLMF_POINT        1
#define SLMF_PROJECTED    2
#define SLMF_TYPE_MASK    (SLMF_POINT | SLMF_PROJECTED)

#define SLMF_LTYPE_SHIFT  8
#define SLMF_LTYPE_BITS   4

struct SLightPass
{
	CDLight *pLights[4];
	uint32 nStencLTMask;
	uint32 nLights;
	uint32 nLTMask;
	bool bRect;
	RectI rc;
};

#define MAX_STREAMS 16

struct SStreamInfo
{
	void *pStream;
	int nOffset;
	int nFreq;
};

struct SFogState
{
	int m_nFogMode;
	int m_nCurFogMode;
	bool m_bEnable;
	float m_FogDensity;
	float m_FogStart;
	float m_FogEnd;
	ColorF m_FogColor;
	ColorF m_CurColor;

	bool operator != (SFogState fs)
	{
		if (m_nFogMode!=fs.m_nFogMode || m_FogDensity!=fs.m_FogDensity || m_FogStart!=fs.m_FogStart || m_FogEnd!=fs.m_FogEnd || m_FogColor!=fs.m_FogColor)
			return true;
		return false;
	}
};

struct SThreadInfo
{
	uint32 m_PersFlags;            // Never reset
	uint32 m_PersFlags2;           // Never reset
	float m_RealTime;
	class CMatrixStack* m_matView;  
	class CMatrixStack* m_matProj;  
	CCamera	m_cam;					    // current camera
	CRenderCamera	m_rcam;				// current camera
	int m_nFrameID;							// with recursive calls, access through GetFrameID(true)
	uint32 m_nFrameUpdateID;      // without recursive calls, access through GetFrameID(false)
	int		m_nObjectUpdateId;		// round ID from 3D engine, useful for texture streaming
	SFogState m_FS;
	CRenderObject *m_pIgnoreObject;

	Plane m_pObliqueClipPlane;
	bool m_bObliqueClipPlane;

	byte m_eCurColorOp[MAX_TMU];
	byte m_eCurAlphaOp[MAX_TMU];
	byte m_eCurColorArg[MAX_TMU];
	byte m_eCurAlphaArg[MAX_TMU];

	Vec4 m_vFrustumInfo;
	~SThreadInfo() {}
	SThreadInfo& operator = (const SThreadInfo& ti)
	{
		m_PersFlags = ti.m_PersFlags;
		m_PersFlags2 = ti.m_PersFlags2;
		m_RealTime = ti.m_RealTime;
		m_matView = ti.m_matView;
		m_matProj = ti.m_matProj;
		m_cam = ti.m_cam;
		m_rcam = ti.m_rcam;
		m_nFrameID = ti.m_nFrameID;
		m_nObjectUpdateId = ti.m_nObjectUpdateId;
		m_FS = ti.m_FS;
		m_pIgnoreObject = ti.m_pIgnoreObject;
		m_pObliqueClipPlane = ti.m_pObliqueClipPlane;
		m_bObliqueClipPlane = ti.m_bObliqueClipPlane;
		m_vFrustumInfo = ti.m_vFrustumInfo;
		for (int i=0; i<MAX_TMU; i++)
		{
			m_eCurColorOp[i] = ti.m_eCurColorOp[i];
			m_eCurAlphaOp[i] = ti.m_eCurAlphaOp[i];
			m_eCurColorArg[i] = ti.m_eCurColorArg[i];
			m_eCurAlphaArg[i] = ti.m_eCurAlphaArg[i];
		}
		return *this;
	}
};

struct SMemPool
{
	int m_nBufSize;
	byte *m_pBuf;
	PodArray<alloc_info_struct> m_alloc_info;
};

#ifdef STRIP_RENDER_THREAD
struct SSingleThreadInfo : public SThreadInfo
{
	SThreadInfo& operator[] (const int) {return *this;}
	const SThreadInfo& operator[] (const int)const {return *this;}
};
#endif

struct SDrawCallCountInfo
{
	SDrawCallCountInfo():pPos(0,0,0), nZpass(0), nShadows(0),nGeneral(0), nMisc(0)
	{
	}

	void Update( CRenderObject *pObj );

	Vec3 pPos;
	uint8 nZpass, nShadows, nGeneral, nMisc;
};

// Render pipeline structure
struct SRenderPipeline
{
	CShader *m_pShader;        
	CShader *m_pReplacementShader;
	CRenderObject *m_pCurObject;
	SInstanceInfo *m_pCurInstanceInfo;
	CRendElementBase *m_pRE;
	CRendElementBase *m_pEventRE;
	int m_RendNumVerts;
	uint32 m_nBatchFilter;             // Batch flags ( FB_ )
	SShaderTechnique *m_pRootTechnique;
	SShaderTechnique *m_pCurTechnique;
	SShaderPass *m_pCurPass;
	int m_nShaderTechnique;

	SRenderShaderResources *m_pShaderResources;
	CRenderObject *m_pPrevObject;
	TArray<SRendItem *> m_RIs;

	short m_TransformFrame;
	ColorF m_CurGlobalColor;

	float m_fMinDistance;					// min distance to texture
	int m_ObjFlags;               // Instances flag for batch (merged)
	int m_Flags;                  // Reset on start pipeline

	Matrix44 m_oldOcclusionCamera;
	Matrix44 m_newOcclusionCamera;
	int m_nZOcclusionProcess;
	int m_nZOcclusionReady;

	int m_nFillThreadID;
	int m_nProcessThreadID;

#ifdef STRIP_RENDER_THREAD
	SSingleThreadInfo m_TI;
#else
	SThreadInfo m_TI[2];
#endif
	SThreadInfo m_OldTI[MAX_RECURSION_LEVELS];
	TDualVal<PodArray<ColorF> > m_fogVolumeContibutions;
	TDualVal<PodArray<SSectorTextureSet> > m_TerrainAONodes;

	int m_FlagsPerFlush;          // Flags which resets for each shader flush
	uint32 m_FlagsStreams_Decl;
	uint32 m_FlagsStreams_Stream;
	uint32 m_FlagsShader_LT;        // Shader light mask
	uint64 m_FlagsShader_RT;      // Shader runtime mask
	uint32 m_FlagsShader_MD;        // Shader texture modificator mask
	uint32 m_FlagsShader_MDV;       // Shader vertex modificator mask
	uint32 m_nShaderQuality;

	void (*m_pRenderFunc)();

	uint32 m_CurGPRAllocStateCommit;
	uint32 m_CurGPRAllocState;
	int m_CurHiZState;

	int m_CurState;
	int m_CurAlphaRef;
	int m_MaterialState;
	int m_MaterialAlphaRef;
	bool m_bIgnoreObjectAlpha;



	int m_FrameObject;
	ECull m_eCull;

	int m_CurStencilState;
	uint32 m_CurStencMask;
	uint32 m_CurStencWriteMask;
	uint32 m_CurStencRef;
	int m_CurStencilSide;

	SStreamInfo m_VertexStreams[MAX_STREAMS];
	int m_ReqStreamFrequence[MAX_STREAMS];
	void *m_pIndexStream;
	int32 m_nIndexStream;

	bool m_bNotFirstPass;
	int m_RenderFrame;
	uint32 m_nNumRendPasses;
	int m_NumShaderInstructions;

	string m_sExcludeShader;
	string m_sShowOnlyShader;

	TArray<SProfInfo> m_Profile;


	CCamera m_PrevCamera;

	SRenderListDesc *m_pRLD;
	uint32 m_nRendFlags;
	bool m_bUseHDR;
	int m_nPassGroupID;             // EFSLIST_ pass type
	int m_nPassGroupDIP;            // EFSLIST_ pass type
	int m_nSortGroupID;
	int m_nShadowChannel;
	uint32 m_nFlagsShaderBegin;
	uint32 m_nCurrTarget;

	ERenderQuality m_eQuality;

	uint32 m_DynLMask;
	uint32 m_PrevLMask;
	int m_nCurLightPasses;
	short m_nCurLightGroup;
	int m_nMaxLightsPerPass;
	SRendLightGroup* m_pCurrentLightGroup;
	short m_nCurLightChan;
	int m_nCurLightPass;
	TArrayHolder<ShadowMapFrustum> m_SMFrustums;
	TArrayHolder<CDLight> m_DLights;
	SLightPass m_LPasses[MAX_REND_LIGHTS];
	float m_fProfileTime;

	ShadowMapFrustum* m_pCurShadowFrustum;
	TArrayHolder<SDeferrredDecal> m_DeferrredDecals;

	UPipeVertex m_PtrTang;
	UPipeVertex m_NextPtrTang;

	UPipeVertex m_Ptr;
	UPipeVertex m_NextPtr;
	int m_Stride;
	int m_OffsT;
	int m_OffsD;
	EVertexFormat m_CurVFormat;

	float m_fLastWaterFOVUpdate;
	Vec3 m_LastWaterViewdirUpdate;
	Vec3 m_LastWaterUpdirUpdate;
	Vec3 m_LastWaterPosUpdate;
	float m_fLastWaterUpdate;
	int m_nLastWaterFrameID;
	int m_nAOMaskUpdateLastFrameId;

#if defined(DIRECT3D9) || defined(DIRECT3D10)
	SFSAA   m_FSAAData;
	bool IsFSAAEnabled() const { return m_FSAAData.Type>0; }

	UDynamicVB m_VBs[MAX_DYNVBS];
	DynamicIB <uint16> *m_IndexBuf;
	UDynamicVB m_MergedStreams[3];
	int m_nStreamOffset[3];
	SOnDemandD3DVertexDeclaration m_D3DVertexDeclaration[eVF_Max]; 
	SOnDemandD3DVertexDeclarationCache m_D3DVertexDeclarationCache[1<<VSF_NUM][eVF_Max][2];	// [StreamMask][VertexFmt][Morph] 
	SOnDemandD3DStreamProperties m_D3DStreamProperties[VSF_NUM];
	DynamicVB <vec4_t> *m_VB_Inst;
	TArray<SVertexDeclaration *> m_CustomVD;
#else
	bool IsFSAAEnabled() const { return false; }
#endif




	uint16 *m_RendIndices;        
	uint16 *m_SysRendIndices;        
	byte   *m_SysArray;     
	int    m_SizeSysArray;     

	TDualVal<TArray<byte> > m_SysVertexPool;
	TDualVal<TArray<uint16> > m_SysIndexPool;

	int m_CurVB;

	int m_RendNumGroup;             
	int m_RendNumIndices;             
	int m_FirstIndex;
	int m_IndexOffset;
	int m_FirstVertex;

#if !defined(XENON) && !defined(PS3)
	Vec4 m_LightInfo[256];
#endif

	bool m_bClipPlaneRefract;
	SPlane m_CurClipPlane;
	SPlane m_CurClipPlaneCull;
	int m_ClipPlaneEnabled;

	int m_FT;

	SEfResTexture *m_ShaderTexResources[MAX_TMU];

	int m_Frame;
	int m_FrameMerge;

	int m_nMaxPasses;
	float m_fCurOpacity;

	TDualVal<SPipeStat> m_PS;
	DynArray<SRTargetStat> m_RTStats;

	int m_MaxVerts;
	int m_MaxTris;

	int m_RECustomTexBind[8];
	int m_ShadowCustomTexBind[8];
	int m_ShadowCustomResViewID[8];
	int m_TextureCustomResViewID[16];

	ColorF m_REColor;
	float m_RECustomData[64];

	//===================================================================
	// Input render data
	CDLight *m_pSunLight;
	TDualVal<TArray <CRenderObject *> > m_RejectedObjects;
	TArray <CRenderObject *> m_Objects;
	TArray <CRenderObject *> m_FreeObjects[4];
	TDualVal<TArray <CRenderObject *> > m_TempObjects;
	TArray <CREMesh *> m_TempREs;
	CRenderObject *m_ObjectsPool;
	uint32 m_nNumObjectsInPool;

	//===================================================================
	// Drawcall count debug view

	typedef std::map< IRenderNode* , SDrawCallCountInfo > RNDrawcallsMap;
	typedef RNDrawcallsMap::iterator RNDrawcallsMapItor;
	RNDrawcallsMap m_pRNDrawCallsInfo;

	//================================================================
	// Sprites
	TArray<SRefSprite> m_Sprites;    // Used for particles (AddSpriteToScene)
	TArray<SRefSprite> m_Polygons;   // Used for decals mostly (AddPolygonToScene)

	//================================================================
	// Temporary mesh
	TArray<class CRETempMesh *> m_TempMeshes[2];
	TArray<class CRETempMesh *> *m_CurTempMeshes;

	//================================================================
	// Hdr
	class CREHDRProcess *m_pREHDR;
	class CREDeferredShading *m_pREDeferredShading;

	int m_HeatSize;
	int m_NightSize;  
	int m_RainMapSize;

	//=================================================================
	// WaveForm tables



	static const uint32 sSinTableCount = 1024;

	float m_tSinTable[sSinTableCount];

	SRenderPipeline()
		: m_pShader(0), m_nShaderTechnique(-1), m_pCurTechnique(NULL)
	{}

	_inline SShaderTechnique* GetStartTechnique() const
	{
		if (m_pShader)
			return m_pShader->mfGetStartTechnique(m_nShaderTechnique);
		return NULL;
	}

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject( m_CurTempMeshes );
		pSizer->AddObject( m_pSunLight );
		pSizer->AddObject( m_sExcludeShader );
		pSizer->AddObject( m_sShowOnlyShader );

		pSizer->AddObject( m_SysArray, m_SizeSysArray );
		//TODO make general alignment handling
		pSizer->AddObject( m_ObjectsPool, ((m_nNumObjectsInPool*2+1024) * sizeof(CRenderObject)));

		pSizer->AddObject( m_Objects );
		pSizer->AddObject( m_TempObjects[0] );		
		pSizer->AddObject( m_TempObjects[1] );	
		pSizer->AddObject( m_DLights[0][0] );
		pSizer->AddObject( m_DLights[0][1] );
		pSizer->AddObject( m_DLights[1][0] );
		pSizer->AddObject( m_DLights[1][1] );
		pSizer->AddObject( m_RIs );		
		pSizer->AddObject( m_SysVertexPool[0] );
		pSizer->AddObject( m_SysVertexPool[1] );
		pSizer->AddObject( m_SysIndexPool[0] );
		pSizer->AddObject( m_SysIndexPool[1] );
		pSizer->AddObject( m_RTStats );
		pSizer->AddObject( m_RejectedObjects[0] );
		pSizer->AddObject( m_RejectedObjects[1] );
		pSizer->AddObject( m_FreeObjects[0] );
		pSizer->AddObject( m_FreeObjects[1] );
		pSizer->AddObject( m_FreeObjects[2] );
		pSizer->AddObject( m_FreeObjects[3] );		
		pSizer->AddObject( m_TempREs );
		pSizer->AddObject( m_TempMeshes[0] );
		pSizer->AddObject( m_TempMeshes[1] );
		pSizer->AddObject( m_Sprites );
		pSizer->AddObject( m_Polygons );
		pSizer->AddObject( m_fogVolumeContibutions[0] );
		pSizer->AddObject( m_fogVolumeContibutions[1] );		
	}
};

extern CryCriticalSection m_sREResLock;

#endif	// __RENDERPIPELINE_H__


