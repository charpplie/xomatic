#include "StdAfx.h"
#include "FlowBaseNode.h"
#include "IStreamEngine.h"
#include "ILevelSystem.h"

class CFlowNode_PortalSwitch : public CFlowBaseNode
{
public:
	CFlowNode_PortalSwitch( SActivationInfo * pActInfo )
	{
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}

	void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig<SFlowSystemVoid>("Activate"),
			InputPortConfig<SFlowSystemVoid>("Deactivate"),
			{0}
		};
		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = in_config;
		config.SetCategory(EFLN_ADVANCED);
	}

	void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_SetEntityId:
			{
				if (!pActInfo->pEntity)
					return;
				// don't disable here, because it will trigger on QL as well
				//gEnv->p3DEngine->ActivatePortal( pActInfo->pEntity->GetWorldPos(), false, pActInfo->pEntity->GetName() );
			}
			break;

		case eFE_Initialize:
			{
				if (!pActInfo->pEntity)
					return;
				gEnv->p3DEngine->ActivatePortal( pActInfo->pEntity->GetWorldPos(), false, pActInfo->pEntity->GetName() );
			}
			break;
		case eFE_Activate:
			{
				if (!pActInfo->pEntity)
				{
					GameWarning("[flow] Trying to activate a portal at a non-existent entity");
					return;
				}
				bool doAnything = false;
				bool activate = false;
				if (IsPortActive(pActInfo, 0))
				{
					doAnything = true;
					activate = true;
				}
				else if (IsPortActive(pActInfo, 1))
				{
					doAnything = true;
					activate = false;
				}
				if (doAnything)
					gEnv->p3DEngine->ActivatePortal( pActInfo->pEntity->GetWorldPos(), activate, pActInfo->pEntity->GetName() );
			}
			break;
		}
	}
};



//////////////////////////////////////////////////////////////////////////////////////
// Node for ocean rendering management ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////


class CFlowNode_OceanSwitch : public CFlowBaseNode
{
public:
	CFlowNode_OceanSwitch( SActivationInfo * pActInfo )
	{
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}

	void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig<bool>("Enable", true, _HELP("Input for enabling/disabling ocean rendering")),
			{0}
		};
		config.pInputPorts = in_config;
		config.sDescription = _HELP("Enabling ocean rendering on demand");
		config.SetCategory(EFLN_ADVANCED);
	}

	void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Activate:
			if (IsPortActive(pActInfo, 0))
			{
				bool bEnabled = GetPortBool( pActInfo, 0);
				gEnv->p3DEngine->EnableOceanRendering(bEnabled);
			}
			break;
		}
	}
};


//////////////////////////////////////////////////////////////////////////////////////
//  Sky box asynchronous loading /////////////////////////////////////////////////////
//! [GDC09]: Async change SkyBox material	////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
namespace SkyBoxLoading
{
	class CSkyBoxLoadingCallback;
};

class CFlowNode_SkyboxSwitch : public CFlowBaseNode
{
	friend class SkyBoxLoading::CSkyBoxLoadingCallback;
protected:
	IReadStreamPtr	m_Job;
public:
	CFlowNode_SkyboxSwitch( SActivationInfo * pActInfo );
	virtual void GetMemoryStatistics(ICrySizer * s);
	void GetConfiguration( SFlowNodeConfig& config );
	void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo );
};

namespace SkyBoxLoading
{
	class CSkyBoxLoadingCallback : public IStreamCallback
	{
		virtual void StreamAsyncOnComplete (IReadStream* pStream, unsigned nError);
		virtual void StreamOnComplete (IReadStream* pStream, unsigned nError);
	};

	struct SSkyboxLoadingInfo
	{
		uint32 									nSelfSize;
		string									sSkyTextureName;
		ITexture*								pTextures[3];
		class CFlowNode_SkyboxSwitch* pParentNode;
		CSkyBoxLoadingCallback	pCallBack;

		SSkyboxLoadingInfo(class CFlowNode_SkyboxSwitch* _pParent)
		{
			pTextures[0] = NULL;
			pTextures[1] = NULL;
			pTextures[2] = NULL;
			pParentNode = _pParent;
			nSelfSize = sizeof(SSkyboxLoadingInfo);
		}
	};


	void CSkyBoxLoadingCallback::StreamAsyncOnComplete (IReadStream* pStream, unsigned nError)
	{
		CRY_ASSERT(nError == 0);
		SSkyboxLoadingInfo* pSkyboxLoadingInfo = (SSkyboxLoadingInfo*)pStream->GetParams().dwUserData;
		if(!pSkyboxLoadingInfo || pSkyboxLoadingInfo->nSelfSize != sizeof(SSkyboxLoadingInfo))
		{
			CRY_ASSERT(0);
			return;
		}

		static const char* pNameSuffixes[] = { "box_12.dds", "box_34.dds", "box_5.dds" };
		string szNameBump;
		for(uint32 i=0;i<3;++i)
		{
			szNameBump = PathUtil::Make(pSkyboxLoadingInfo->sSkyTextureName, pNameSuffixes[i]);
			// check if the texture exists
			if(gEnv->pCryPak->IsFileExist(szNameBump))
				pSkyboxLoadingInfo->pTextures[i] = gEnv->pRenderer->EF_LoadTexture(szNameBump,FT_DONT_STREAM,eTT_2D);
			else
			{
				gEnv->pLog->LogError("Sky box texture not found: %s", szNameBump.c_str());
				for(uint32 j=i;j<3;++j)
				{
					SAFE_RELEASE(pSkyboxLoadingInfo->pTextures[i]);
				}
				return;
			}
			// in case of error release the rest textures
			if(!pSkyboxLoadingInfo->pTextures[i])
			{
				gEnv->pLog->LogError("Error loading sky box texture: %s", szNameBump.c_str());
				CRY_ASSERT(0);
				for(uint32 j=i;j<3;++j)
				{
					SAFE_RELEASE(pSkyboxLoadingInfo->pTextures[i]);
				}
				return;
			}
		}
	}

	void CSkyBoxLoadingCallback::StreamOnComplete (IReadStream* pStream, unsigned nError)
	{
		CRY_ASSERT(nError == 0);
		SSkyboxLoadingInfo* pSkyboxLoadingInfo = (SSkyboxLoadingInfo*)pStream->GetParams().dwUserData;
		if(!pSkyboxLoadingInfo || pSkyboxLoadingInfo->nSelfSize != sizeof(SSkyboxLoadingInfo) || !pSkyboxLoadingInfo->pTextures[0])
		{
			CRY_ASSERT(0);
			gEnv->pLog->LogError("Error switching sky box");
			return;
		}

		IMaterial* pSkyMat = gEnv->p3DEngine->GetSkyMaterial();

		bool bSucceeded = false;
		if(pSkyMat && !(pSkyMat->GetFlags() & EF_SKY))
		{
			SShaderItem& rShaderItem = pSkyMat->GetShaderItem();
			if(rShaderItem.m_pShaderResources)
			{
				SSkyInfo* pSkyInfo = rShaderItem.m_pShaderResources->GetSkyInfo();

				if(pSkyInfo)
				{
					for(int i=0;i<3;++i)
					{
						SAFE_RELEASE(pSkyInfo->m_SkyBox[i]);
						pSkyInfo->m_SkyBox[i] = pSkyboxLoadingInfo->pTextures[i];
					}
					bSucceeded = true;
				}
			}
		}

		// remove job
		pSkyboxLoadingInfo->pParentNode->m_Job = NULL;

		if(!bSucceeded)
		{
			gEnv->pLog->LogError("Error switching sky box: HDR sky box is not supported");
			for(int i=0;i<3;++i)
				SAFE_RELEASE(pSkyboxLoadingInfo->pTextures[i]);
		}
		else
		{
			gEnv->pLog->Log("Sky box switched");
		}
	}
}

CFlowNode_SkyboxSwitch::CFlowNode_SkyboxSwitch( SActivationInfo * pActInfo )
{
	m_Job = NULL;
}

void CFlowNode_SkyboxSwitch::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
}

void CFlowNode_SkyboxSwitch::GetConfiguration( SFlowNodeConfig& config )
{
	static const SInputPortConfig in_config[] = {
		InputPortConfig<string>("Texture", "", _HELP("Skybox texture name")),
		InputPortConfig<bool>("Start", false, _HELP("Trigger to start the loading")),
		InputPortConfig<float>("Angle", 1.f, _HELP("Sky box rotation")),
		InputPortConfig<float>("Stretching", 1.f, _HELP("Sky box stretching")),
		{0}
	};

	config.pInputPorts = in_config;
	config.sDescription = _HELP("Node for asynchronous sky box switching");
	config.SetCategory(EFLN_ADVANCED);
}

void CFlowNode_SkyboxSwitch::ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
{
	switch (event)
	{
	case eFE_Activate:
		// start loading signal
		if (IsPortActive(pActInfo, 1) && !m_Job)
		{
			// set up sky box size and angle
			gEnv->p3DEngine->SetGlobalParameter(E3DPARAM_SKY_SKYBOX_ANGLE, GetPortFloat(pActInfo, 2));
			gEnv->p3DEngine->SetGlobalParameter(E3DPARAM_SKY_SKYBOX_STRETCHING, GetPortFloat(pActInfo, 3));

			string sTextureName = GetPortString(pActInfo, 0);
			// start asynchronous job
			{
				// fill streaming params
				SkyBoxLoading::SSkyboxLoadingInfo* pSkyboxLoadingInfo = new SkyBoxLoading::SSkyboxLoadingInfo(this);
				pSkyboxLoadingInfo->sSkyTextureName = sTextureName;
				StreamReadParams pStreamParams;
				pStreamParams.nFlags = 0;
#if !defined(XENON) && !defined(PS3)
				// don't support async texture creation on DX9.
				if(gEnv->pRenderer->GetRenderType() < eRT_DX11)
					pStreamParams.nFlags |= SRP_FLAGS_FORCE_SYNC_CALLBACKS;
#endif
				pStreamParams.dwUserData = (DWORD_PTR)pSkyboxLoadingInfo;
				pStreamParams.nPriority = 0;
				pStreamParams.pBuffer = NULL;
				pStreamParams.nOffset = 0;
				pStreamParams.nSize = 0;
				m_Job = gEnv->pSystem->GetStreamEngine()->StartRead(eStreamTaskTypeTexture, NULL, &pSkyboxLoadingInfo->pCallBack, &pStreamParams);
			}
		}
		break;
	}
}


//////////////////////////////////////////////////////////////////////////////////////
// Node for layer switching rendering management ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////


class CFlowNode_LayerSwitch : public CFlowBaseNode
{
public:
	CFlowNode_LayerSwitch( SActivationInfo * pActInfo )
	{
	}

	virtual void GetMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}

	void GetConfiguration( SFlowNodeConfig& config )
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig<string>("Layer", _HELP("Layer name")),
			InputPortConfig_Void("Hide", _HELP("Input for enabling objects in layer rendering")),
			InputPortConfig_Void("Unhide", _HELP("Input for disabling objects in layer rendering")),
			{0}
		};

		static const SOutputPortConfig out_config[] = {
			OutputPortConfig_Void("Hidden",_HELP("Triggered if Boolean was False")),
			OutputPortConfig_Void("Unhidden", _HELP("Triggered if Boolean was True")),
			{0}
		};

		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.sDescription = _HELP("Activate/Deactivate objects in layer");
		config.SetCategory(EFLN_ADVANCED);
	}

	void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Activate:
			{
				bool bEnabled = true;
				if (IsPortActive(pActInfo, 1))
				{
					bEnabled = false;
				}
				else if (IsPortActive(pActInfo, 2))
				{
					bEnabled = true;
				}
				else
					break;

				string layer = GetPortString(pActInfo, 0);
				if(gEnv->pEntitySystem)
				{
					gEnv->pEntitySystem->EnableLayer(layer, bEnabled);
				}
				ActivateOutput(pActInfo, bEnabled ? 1 : 0, true);
			}
			break;
		}
	}
};




//////////////////////////////////////////////////////////////////////////////////////

REGISTER_FLOW_NODE_SINGLETON("Engine:PortalSwitch", CFlowNode_PortalSwitch);
REGISTER_FLOW_NODE_SINGLETON("Engine:OceanSwitch", CFlowNode_OceanSwitch);
REGISTER_FLOW_NODE("Engine:SkyboxSwitch", CFlowNode_SkyboxSwitch);
REGISTER_FLOW_NODE("Engine:LayerSwitch", CFlowNode_LayerSwitch);
