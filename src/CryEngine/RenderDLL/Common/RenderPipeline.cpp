/*=============================================================================
	CommonRender.cpp: Crytek Common render helper functions and structures declarations.
	Copyright (c) 2001 Crytek Studios. All Rights Reserved.

	Revision history:
		* Created by Honich Andrey

=============================================================================*/

#include "StdAfx.h"
#include "Shadow_Renderer.h"
#include "MsStlSort.h"

#ifdef STRIP_RENDER_THREAD
	TArray<SRendItem> SRendItem::m_RendItems[MAX_LIST_ORDER][EFSLIST_NUM];  
#else
	TArray<SRendItem> SRendItem::m_RendItems[2][MAX_LIST_ORDER][EFSLIST_NUM];  
#endif

TDualVal<int> SRendItem::m_RenderView;
TDualVal<int> SRendItem::m_RecurseLevel;
int SRendItem::m_AppStartRI[MAX_REND_RECURSION_LEVELS][MAX_LIST_ORDER][EFSLIST_NUM];

//int SRendItem::m_ShadowGenRecurLevel[2];
TMultiVal<int, MAX_REND_LIGHTS+MAX_DEFERRED_LIGHTS> SRendItem::m_StartFrust;
TMultiVal<int, MAX_REND_LIGHTS+MAX_DEFERRED_LIGHTS> SRendItem::m_EndFrust;
TMultiVal<int, MAX_SHADOWMAP_FRUSTUMS> SRendItem::m_ShadowsStartRI;
TMultiVal<int, MAX_SHADOWMAP_FRUSTUMS> SRendItem::m_ShadowsEndRI;

SRendLightGroup SRendItem::m_RenderLightGroups[MAX_SORT_GROUPS][MAX_REND_LIGHT_GROUPS+1];
uint32 SRendItem::m_ShadowsValidMask[MAX_REND_RECURSION_LEVELS][MAX_REND_LIGHT_GROUPS];
int SRendItem::m_nSortGroups;
uint32 SRendItem::m_BatchFlags[MAX_REND_RECURSION_LEVELS][MAX_LIST_ORDER][EFSLIST_NUM];

struct CompareItemPreprocess
{
	bool operator()(const SRendItem& a, const SRendItem& b) const
	{
		if (a.nBatchFlags != b.nBatchFlags)
			return a.nBatchFlags < b.nBatchFlags;

		return a.SortVal < b.SortVal;
	}
};

struct CompareRendItem
{
	bool operator()(const SRendItem& a, const SRendItem& b) const
	{
		if (a.DynLMask != b.DynLMask)       // Sort by lights
			return a.DynLMask < b.DynLMask;

		/// Nearest objects should be rendered first
		int nNearA = (a.ObjSort & FOB_NEAREST);
		int nNearB = (b.ObjSort & FOB_NEAREST);
		if (nNearA != nNearB)         // Sort by nearest flag
			return nNearA > nNearB;

		if (a.SortVal != b.SortVal)         // Sort by shaders
			return a.SortVal < b.SortVal;

		if (a.Item != b.Item)               // Sort by geometry
			return a.Item < b.Item;

		return a.ObjSort < b.ObjSort;
	}
};

struct CompareItem_Decal
{
	bool operator()(const SRendItem& a, const SRendItem& b) const
	{
		uint32 objSortA_Low(a.ObjSort & 0xFFFF);
		uint32 objSortA_High(a.ObjSort & ~0xFFFF);
		uint32 objSortB_Low(b.ObjSort & 0xFFFF);
		uint32 objSortB_High(b.ObjSort & ~0xFFFF);

		if (objSortA_Low != objSortB_Low)
			return objSortA_Low < objSortB_Low;

		if (a.DynLMask != b.DynLMask)
			return a.DynLMask < b.DynLMask;

		if (a.SortVal != b.SortVal)
			return a.SortVal < b.SortVal;

		return objSortA_High < objSortB_High;
	}
};

struct CompareItem_Terrain
{
	bool operator()(const SRendItem& a, const SRendItem& b) const
	{
		if (a.DynLMask != b.DynLMask)
			return a.DynLMask < b.DynLMask;

		CRendElementBase *pREa = a.Item;
		CRendElementBase *pREb = b.Item;

		if (pREa->m_CustomTexBind[0] != pREb->m_CustomTexBind[0])
			return pREa->m_CustomTexBind[0] < pREb->m_CustomTexBind[0];

		if (pREa->m_CustomTexBind[1] != pREb->m_CustomTexBind[1])
			return pREa->m_CustomTexBind[1] < pREb->m_CustomTexBind[1];

		return a.ObjSort < b.ObjSort;
	}
};

struct CompareItem_NoPtrCompare
{
	bool operator()(const SRendItem& a, const SRendItem& b) const
	{
		if (a.DynLMask != b.DynLMask)
			return a.DynLMask < b.DynLMask;

		if (a.ObjSort != b.ObjSort)
			return a.ObjSort < b.ObjSort;

		float pSurfTypeA = ((float*)a.Item->m_CustomData)[8];
		float pSurfTypeB = ((float*)b.Item->m_CustomData)[8];
		if (pSurfTypeA != pSurfTypeB)
			return (pSurfTypeA < pSurfTypeB);

		pSurfTypeA = ((float*)a.Item->m_CustomData)[9];
		pSurfTypeB = ((float*)b.Item->m_CustomData)[9];
		if (pSurfTypeA != pSurfTypeB)
			return (pSurfTypeA < pSurfTypeB);

		pSurfTypeA = ((float*)a.Item->m_CustomData)[11];
		pSurfTypeB = ((float*)b.Item->m_CustomData)[11];
		return (pSurfTypeA < pSurfTypeB);
	}
};

struct CompareDist
{
	bool operator()(const SRendItem& a, const SRendItem& b) const
	{
		return (a.fDist > b.fDist);
	}
};

struct CompareDistInverted
{
	bool operator()(const SRendItem& a, const SRendItem& b) const
	{
		return (a.fDist < b.fDist);
	}
};

void SRendItem::mfSortPreprocess(SRendItem *First, int Num)
{
	std::sort(First, First+Num, CompareItemPreprocess());
}

void SRendItem::mfGenerateLightGroupsTransparent(SRendItem *First, int Num)
{
  CRenderer *r = gRenDev;
  int nR = SRendItem::m_RecurseLevel[r->m_RP.m_nProcessThreadID]-1;
  assert(r->m_pRT->IsRenderThread());
  int nThreadList = r->m_RP.m_nProcessThreadID;

  bool bSunUsed = false;

  int nNumLightsCastShadow = r->m_RP.m_DLights[r->m_RP.m_nProcessThreadID][nR].Num();
  for (uint32 n=0; n<r->m_RP.m_DLights[r->m_RP.m_nProcessThreadID][nR].Num(); n++)
  {
    CDLight *pLight = &r->m_RP.m_DLights[r->m_RP.m_nProcessThreadID][nR][n];

    if (pLight->m_Flags & DLF_DIRECTIONAL)
      bSunUsed = true;

    if (!(pLight->m_Flags & DLF_CASTSHADOW_MAPS))
    {
      nNumLightsCastShadow = n;
      break;
    }
  }

  // Create light groups
  int i, j;
  bool bPrevProcessed = false;
  uint32 nSort = 0; 
  SRendLightGroup *pGR = &m_RenderLightGroups[nSort][0];
  for (j=0; j<=MAX_REND_LIGHT_GROUPS; j++)
  {
    pGR[j].Reset();
  }
  for (i=0; i<Num; i++)
  {
    SRendItem *ri = &First[i];
    uint32 LightMask = ri->DynLMask;
    bool bProcessed = false;
    if (LightMask)
    {
      CRenderObject *pObj = ri->pObj;
#ifdef _DEBUG
      CTexture *pTex = CTexture::GetByID(pObj->m_nTextureID);
      CShader *pSh = mfGetShader(ri->SortVal);
      int nnn = 0;
#endif
      if (pObj->m_ObjFlags & FOB_INSHADOW)
      {
        int nFirstLightInGroupID = 0;
        int nPass = 0;
        if (!bPrevProcessed && nSort < MAX_SORT_GROUPS-1)
        {
          nSort++;
          pGR = &m_RenderLightGroups[nSort][0];
          for (j=0; j<=MAX_REND_LIGHT_GROUPS; j++)
          {
            pGR[j].Reset();
          }
          bPrevProcessed = true;
        }
        for (j=0; j<MAX_REND_LIGHT_GROUPS; j++, nFirstLightInGroupID+=4)
        {
          uint32 nCurMaskGroup = 0xf<<nFirstLightInGroupID;
          if (ri->DynLMask & nCurMaskGroup)
          {
            pGR[j].m_GroupLightMask |= (ri->DynLMask & nCurMaskGroup);
            pGR[j].RendItemsLights.push_back(nPass ? i|0x80000000 : i);
            nPass++;
            if (pObj->m_pShadowCasters)
            {
              bool bAdded[4];
              *(uint32 *)&bAdded[0] = 0;
              PodArray<ShadowMapFrustum*> *lsources = pObj->m_pShadowCasters;
              for (int n=0; n<lsources->Count(); n++)
              {
                ShadowMapFrustum* pFr = lsources->GetAt(n);
                if (pFr->nDLightId >= nFirstLightInGroupID && pFr->nDLightId <= nFirstLightInGroupID+3)
                {
                  int nId = pFr->nDLightId-nFirstLightInGroupID;
                  if (!bAdded[nId])
                  {
                    bAdded[nId] = true;
                    pGR[j].RendItemsShadows[nId].push_back(i);
                  }
                }
              } //Diesel cut
            }
          }
          if (ri->DynLMask < (uint32)(1<<(nFirstLightInGroupID+4)))
            break;
        }
        ri->ObjSort = nPass << 8;
        bProcessed = true;
      }
    }
    if (!bProcessed)
    {
      ri->ObjSort = 1 << 8;
      pGR[MAX_REND_LIGHT_GROUPS].RendItemsLights.push_back(i);
      pGR[MAX_REND_LIGHT_GROUPS].m_GroupLightMask |= ri->DynLMask;
      bPrevProcessed = false;
    }
    if (i && bProcessed != bPrevProcessed && nSort < MAX_SORT_GROUPS-1)
    {
      nSort++;
      pGR = &m_RenderLightGroups[nSort][0];
      for (j=0; j<=MAX_REND_LIGHT_GROUPS; j++)
      {
        pGR[j].Reset();
      }
      bPrevProcessed = bProcessed;
    }
  }
  m_nSortGroups = nSort+1;
}

void SRendItem::mfGenerateLightGroupsOpaque(SRendItem *First, int Num)
{
  CRenderer *r = gRenDev;
  int nR = SRendItem::m_RecurseLevel[r->m_RP.m_nProcessThreadID]-1;
  assert(r->m_pRT->IsRenderThread());
  int nThreadList = r->m_RP.m_nProcessThreadID;

  bool bSunUsed = false;

  int nNumLightsCastShadow = r->m_RP.m_DLights[r->m_RP.m_nProcessThreadID][nR].Num();
  for (uint32 n=0; n<r->m_RP.m_DLights[r->m_RP.m_nProcessThreadID][nR].Num(); n++)
  {
    CDLight *pLight = &r->m_RP.m_DLights[r->m_RP.m_nProcessThreadID][nR][n];

    if (pLight->m_Flags & DLF_DIRECTIONAL)
      bSunUsed = true;

    if (!(pLight->m_Flags & DLF_CASTSHADOW_MAPS))
    {
      nNumLightsCastShadow = n;
      break;
    }
  }

  int i, j;
  SRendLightGroup *pGR = &m_RenderLightGroups[0][0];
  for (j=0; j<MAX_REND_LIGHT_GROUPS+1; j++)
  {
    pGR[j].Reset();
  }
  for (i=0; i<Num; i++)
  {
    SRendItem *ri = &First[i];
    uint32 LightMask = ri->DynLMask;
    bool bProcessed = false;
    if (LightMask)
    {
      CRenderObject *pObj = ri->pObj;
#ifdef _DEBUG
      CShader *pSh = mfGetShader(ri->SortVal);
      for (j=0; j<r->m_RP.m_DLights[r->m_RP.m_nProcessThreadID][nR].Num(); j++)
      {
        if (pObj->m_DynLMMask[r->m_RP.m_nProcessThreadID] & (1<<j))
        {
          CDLight *pDL = &r->m_RP.m_DLights[r->m_RP.m_nProcessThreadID][nR][j];
          int nnn = 0;
        }
      }
#endif
      if (pObj->m_ObjFlags & FOB_INSHADOW)
      {
        int nFirstLightInGroupID = 0;
        bool bSecPass = /*bSecondPass*/ false;
        for (j=0; j<MAX_REND_LIGHT_GROUPS; j++, nFirstLightInGroupID+=4)
        {
          uint32 nCurMaskGroup = 0xf<<nFirstLightInGroupID;
          uint32 nCurLMask = LightMask & nCurMaskGroup;
          if (nCurLMask)
          {
            pGR[j].m_GroupLightMask |= (LightMask & nCurMaskGroup);
            pGR[j].RendItemsLights.push_back(bSecPass ? i|0x80000000 : i);
            bSecPass = true;

            //Test for sun light group
            bool bSunOnlyGroup = (nCurLMask <= 1 && bSunUsed);

            // Optimisation: don't build shadow pass lists in deferred shadow mode
            //if ( !CRenderer::CV_r_ShadowPassFS || !bSunOnlyGroup  )
            {
              if (pObj->m_pShadowCasters) 
              {
                bool bAdded[4];
                *(uint32 *)&bAdded[0] = 0; //Diesel cut
                PodArray<ShadowMapFrustum*> *lsources = pObj->m_pShadowCasters;
                for (int n=0; n<lsources->Count(); n++)
                {
                //  ShadowMapFrustum* pFr = lsources->GetAt(n);
                //  int nId = pFr->nDLightId-nFirstLightInGroupID;
                //  if (nId>=0 && nId<4 && !bAdded[nId])
                 // {
                  //  bAdded[nId] = true;
                  //  pGR[j].RendItemsShadows[nId].push_back(i);
					continue; //Diesel new
                  //}
                } //Diesel cut
				}
            }
          }
          if (LightMask < (uint32)(1<<(nFirstLightInGroupID+4)))
            break;
        }
        bProcessed = true;
      }
    }
    if (!bProcessed)
    {
      pGR[MAX_REND_LIGHT_GROUPS].RendItemsLights.push_back(/*bSecondPass ? i|0x80000000 :*/ i);
      pGR[MAX_REND_LIGHT_GROUPS].m_GroupLightMask |= LightMask;
    }
  }
  m_nSortGroups = 1;
}

void SRendItem::mfSortByDist(SRendItem *First, int Num, bool bDecals,bool InvertedOrder)
{
  CRenderer *r = gRenDev;
  int i;
  if (!bDecals)
  {
    for (i=0; i<Num; i++)
    {
      SRendItem *pRI = &First[i];
      CRenderObject *pObj = pRI->pObj;
		  if (!pObj)
		  {
			  CryLogAlways("SRendItem::mfSortByDist: pObject is NULL");
			  continue;
		  }
      float fAddDist = pObj->m_fSort;
      pRI->fDist = pObj->m_fDistance + fAddDist;
    }
		if(InvertedOrder)
		  std::sort(First, First+Num, CompareDistInverted());
		//else //Diesel cut
		//  std::sort(First, First+Num, CompareDist()); //Diesel cut
  }
  else
  {
    std::sort(First, First + Num, CompareItem_Decal());
  }
}

template<class Iter,class StrictWeakOrdering>
void RendItemSort(Iter First,Iter Last,StrictWeakOrdering Comp)
{
#if defined(RENDITEM_SORT_STABLE)
	std::stable_sort(First,Last,Comp);
#else
	std::sort(First,Last,Comp);
#endif
}

void SRendItem::mfSortByLight(SRendItem *First, int Num, bool bSort, const bool bIgnoreRePtr, bool bSortDecals)
{
  if (bSort)
	{
    if (bIgnoreRePtr)
			RendItemSort(First, First + Num, CompareItem_NoPtrCompare());
		else
		{
			if (bSortDecals)
				RendItemSort(First, First + Num, CompareItem_Decal());
			else
				RendItemSort(First, First + Num, CompareRendItem());
		}
	}
}

//=================================================================

