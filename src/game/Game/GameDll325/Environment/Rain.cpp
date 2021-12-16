#include "StdAfx.h"
#include "Rain.h"
#include "IActorSystem.h"

CRain::CRain()
{
}

CRain::~CRain()
{
	m_Textures.clear();
}

//------------------------------------------------------------------------
bool CRain::Init(IGameObject *pGameObject)
{
	SetGameObject(pGameObject);
	PreloadTextures();

	return Reset();
}

//------------------------------------------------------------------------
void CRain::PreloadTextures()
{
	uint32 nDefaultFlags = FT_DONT_RELEASE | FT_DONT_RESIZE | FT_DONT_STREAM;

	XmlNodeRef root = GetISystem()->LoadXmlFile( "Shaders/EngineAssets/raintextures.xml" );
	if (root)
	{
		for (int i = 0; i < root->getChildCount(); i++)
		{
			XmlNodeRef entry = root->getChild(i);
			if (!entry->isTag("entry"))
				continue;

			uint32 nFlags = nDefaultFlags;

			// check attributes to modify the loading flags
			int nNoMips = 0;
			if (entry->getAttr("nomips", nNoMips) && nNoMips)
				nFlags |= FT_NOMIPS;

			ITexture* pTexture = gEnv->pRenderer->EF_LoadTexture(entry->getContent(), nFlags, eTF_Unknown);
			if (pTexture)	{
				m_Textures.push_back(pTexture);
			}
		}
	}
}

//------------------------------------------------------------------------
void CRain::PostInit(IGameObject *pGameObject)
{
	GetGameObject()->EnableUpdateSlot(this, 0);
}

//------------------------------------------------------------------------
bool CRain::ReloadExtension( IGameObject * pGameObject, const SEntitySpawnParams &params )
{
	//ResetGameObject();

	CRY_ASSERT_MESSAGE(false, "CRain::ReloadExtension not implemented");
	
	return false;
}

//------------------------------------------------------------------------
bool CRain::GetEntityPoolSignature( TSerialize signature )
{
	CRY_ASSERT_MESSAGE(false, "CRain::GetEntityPoolSignature not implemented");
	
	return true;
}

//------------------------------------------------------------------------
void CRain::Release()
{
	delete this;
}

//------------------------------------------------------------------------
void CRain::FullSerialize(TSerialize ser)
{
	ser.Value("bEnabled", m_bEnabled);
	ser.Value("fRadius", m_fRadius);
	ser.Value("fAmount", m_fAmount);
	ser.Value("clrColor", m_vColor);
	ser.Value("fReflectionAmount", m_fReflectionAmount);
	ser.Value("fPuddlesAmount", m_fPuddlesAmount);
	ser.Value("bRainDrops", m_bRainDrops);
}

//------------------------------------------------------------------------
void CRain::Update(SEntityUpdateContext &ctx, int updateSlot)
{
/* //Diesel cut
	const IActor * pClient = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if (pClient && Reset())
	{
		Vec3 vR = (GetEntity()->GetWorldPos() - pClient->GetEntity()->GetWorldPos()) / m_fRadius;
		float fAttenAmount = 1.0f - vR.dot(vR);
		fAttenAmount *= m_fAmount;

		// Force set if current values not valid
		float fCurRadius, fCurAmount, fCurFresnel, fCurPuddles;
		Vec3 vCurColor;
		bool bDrops;
		bool bSet = !gEnv->p3DEngine->GetRainParams(vR, fCurRadius, fCurAmount, vCurColor, fCurFresnel, fCurPuddles, bDrops);

		// Set if stronger
		bSet |= fAttenAmount > fCurAmount;

		if (bSet)
			gEnv->p3DEngine->SetRainParams(GetEntity()->GetWorldPos(), m_fRadius, fAttenAmount, m_vColor, 0.5f * m_fReflectionAmount, m_fPuddlesAmount, m_bRainDrops);
	}
*/ //Diesel cut
}

//------------------------------------------------------------------------
void CRain::HandleEvent(const SGameObjectEvent &event)
{
}

//------------------------------------------------------------------------
void CRain::ProcessEvent(SEntityEvent &event)
{
	switch (event.event)
	{
	case ENTITY_EVENT_RESET:
		Reset();
		break;
	}
}

//------------------------------------------------------------------------
void CRain::SetAuthority(bool auth)
{
}

//------------------------------------------------------------------------
bool CRain::Reset()
{
	//Initialize default values before (in case ScriptTable fails)
	m_fRadius = 50.f;
	m_fAmount = 1.f;
	m_vColor.Set(1,1,1);
	m_fReflectionAmount = 1.f;
	m_fPuddlesAmount = 3.f;
	m_bRainDrops = true;

	SmartScriptTable props;
	IScriptTable* pScriptTable = GetEntity()->GetScriptTable();
	if (!pScriptTable || !pScriptTable->GetValue("Properties", props))
		return false;

	props->GetValue("fRadius", m_fRadius);
	props->GetValue("fAmount", m_fAmount);
	props->GetValue("clrColor", m_vColor);
	props->GetValue("fReflectionAmount", m_fReflectionAmount);
	props->GetValue("fPuddlesAmount", m_fPuddlesAmount);
	props->GetValue("bRainDrops", m_bRainDrops);
	props->GetValue("bEnabled", m_bEnabled);
	if (!m_bEnabled)
		m_fAmount = 0;

	return true;
}