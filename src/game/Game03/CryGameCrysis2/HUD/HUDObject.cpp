#include "StdAfx.h"
#include "HUDObject.h"

#include "HUD/HUD.h"
#include "HUD/HUDAsset.h"
#include "HUD/HUDObjectManager.h"

#include "Flash/Flash.h"

#include "HUD_Impl.h" // TODO : Remove
#include "Graphics/2DRenderUtils.h"
#include "ScreenLayoutUtils.h"
#include "StringUtils.h"
#include "HUD/HUD_UnifiedAsset.h"


//////////////////////////////////////////////////////////////////////////


CHUDObject::CHUDObject()
{
}



CHUDObject::~CHUDObject()
{
	g_pGame->GetHUD()->RemoveHUDEventListener(this);

	while(m_assets.size()!=0)
	{
		TAssets::iterator it = m_assets.begin();
		IHUDAsset* pAsset = it->second;
		SAFE_DELETE(pAsset);
		m_assets.erase(it);
	}

}



void CHUDObject::Initialize(const IItemParamsNode* object)
{
	InitializeAssets(object->GetChild("Assets"));
	InitializeEvents(object->GetChild("Events"));
}



void CHUDObject::InitializeAssets(const IItemParamsNode* assets)
{
	if(!assets)
		return;

	for(int i = 0; i < assets->GetChildCount(); ++i)
	{

		const IItemParamsNode * child = assets->GetChild(i);

		CryFixedStringT<16> name = "";
		CryFixedStringT<64> path = "";

		const int numAttributes = child->GetAttributeCount();

		for(int attr = 0; attr < numAttributes; ++attr)		
		{
			const char* key = child->GetAttributeName(attr);
			const char* value = child->GetAttribute(attr);
			if(key && value)
			{
				if(!stricmp(key, "name"))
					name = value;
				else if(!stricmp(key, "path"))
					path = value;
			}
		}

		if(name.empty() || path.empty())
			continue;

		IHUDAsset* pAsset = GetAssetClassInstance(child->GetName());

		if(!pAsset)
			continue;

		pAsset->SetPath(path.c_str());
		pAsset->SetName(name.c_str());

		ScreenLayoutManager* pScreenLayout = g_pGame->GetHUD()->GetLayoutManager();
		pAsset->Init(child,pScreenLayout);
		pAsset->m_info.Recalculate();

		m_assets.insert(std::make_pair(CryStringUtils::CalculateHash(name.c_str()), pAsset));
	}
}



void CHUDObject::InitializeEvents(const IItemParamsNode* events)
{
	if(!events)
		return;

	for(int i = 0; i < events->GetChildCount(); ++i)
	{
		const IItemParamsNode * child = events->GetChild(i);
		g_pGame->GetHUD()->AddHUDEventListener(this, child->GetName());
	}
}



IHUDAsset* CHUDObject::GetAsset( void )
{
	return CHUD_UnifiedAsset::s_pHUDAssetUnified3D;
}



IHUDAsset* CHUDObject::GetAsset2D( void )
{
	return CHUD_UnifiedAsset::s_pHUDAssetUnified2D;
}



IHUDAsset* CHUDObject::GetAsset(const char* asset)
{
	TAssets::const_iterator it = m_assets.find(CryStringUtils::CalculateHash(asset));
	if (it != m_assets.end())
	{
		IHUDAsset* pPtr = it->second;
		return pPtr;
	}

	CryHUDWarning( VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, string().Format("HUD: Invalid HUD asset '%s' requested!", asset) );
	return CHUD::s_pHUDAssetNULL;
}



IHUDAsset* CHUDObject::GetFirstAsset( void )
{
	TAssets::const_iterator it = m_assets.begin();
	if (it != m_assets.end())
	{
		IHUDAsset* pPtr = it->second;
		return pPtr;
	}

	CryHUDWarning( VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, string().Format("HUD: No HUD Asset found!") );
	return CHUD::s_pHUDAssetNULL;
}



IHUDAsset* CHUDObject::GetAssetClassInstance(const char* classType)
{
	if(!stricmp(classType, "Gfx"))
		return static_cast<IHUDAsset*>(new CFlashAsset());
	else
		return NULL;

}

//////////////////////////////////////////////////////////////////////////
