#include "StdAfx.h"
#include "HUD_UnifiedAsset.h"

#include "HUD/HUD.h"
#include "IViewSystem.h"

#define SAFE_REMOVE_ENTITY(p)			{ if(p) { gEnv->pEntitySystem->RemoveEntity(p);	(p)=0; } }

IHUDAsset* CHUD_UnifiedAsset::s_pHUDAssetUnified3D = new CHUDAssetNull();
IHUDAsset* CHUD_UnifiedAsset::s_pHUDAssetUnified2D = new CHUDAssetNull();

IEntityClass* CHUD_UnifiedAsset::s_HUDObjectClass = NULL;

//Screen width and height in which the 3D asset is full screen without offsets
static float DEFAULT_WIDTH = 1190.0f;
static float DEFAULT_HEIGHT = 720.0f;
static float DEFAULT_DEPTH_PS3 = 0.98f;
static float DEFAULT_DEPTH_XBOX = 0.93f;
static float DEFAULT_DEPTH_PC = 0.85f;
static float X_OFFSET_MODIFIER = 0.58f;

static float PHYSICAL_IMPULSE_DAMPING = 0.9f;


//////////////////////////////////////////////////////////////////////////


CHUD_UnifiedAsset::CHUD_UnifiedAsset()
: m_objectsValid(false)
, m_pMaterial(NULL)
, m_offset3D(0.0f)
, m_show(true)
, m_additionalMovement(ZERO)
, m_impulse(ZERO)
, m_previousRotation(IDENTITY)
, m_previousPosition(ZERO)
{
	s_HUDObjectClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("HUD_Object");

	InitObject(eHUDElement_objectives,								eHUDPosition_left_front,		"objects/hud/hud_tl.cgf");
	InitObject(eHUDElement_radar,											eHUDPosition_left_front,		"objects/hud/hud_bl.cgf");
	InitObject(eHUDElement_transmissions,							eHUDPosition_right_front,		"objects/hud/hud_tr.cgf");
	InitObject(eHUDElement_tacticalinfo,							eHUDPosition_right_front,		"objects/hud/hud_mr.cgf");
	InitObject(eHUDElement_ammo_suit,									eHUDPosition_right_front,		"objects/hud/hud_br.cgf");
	InitObject(eHUDElement_incoming,									eHUDPosition_center_front,	"objects/hud/hud_mt.cgf");
	InitObject(eHUDElement_leaning,										eHUDPosition_center_front,	"objects/hud/hud_center.cgf");
	InitObject(eHUDElement_scanning,									eHUDPosition_center_front,	"objects/hud/hud_center2.cgf");
	InitObject(eHUDElement_interactions,							eHUDPosition_center_front,	"objects/hud/hud_mb.cgf");
	InitObject(eHUDElement_visor_left,								eHUDPosition_left_front,		"objects/hud/hud_center2_left.cgf");
	InitObject(eHUDElement_visor_right,								eHUDPosition_right_front,		"objects/hud/hud_center2_right.cgf");
}



CHUD_UnifiedAsset::~CHUD_UnifiedAsset()
{
	s_pHUDAssetUnified3D = new CHUDAssetNull();
	s_pHUDAssetUnified2D = new CHUDAssetNull();
	Destroy3DObjects();
	m_pMaterial = 0;
	m_elements.clear();
}



void CHUD_UnifiedAsset::Init()
{
	SAFE_DELETE(s_pHUDAssetUnified2D);
	s_pHUDAssetUnified2D = GetAsset("unified");

	if(!m_pMaterial)
	{
		_smart_ptr<IMaterial> pMaterial = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial("objects/hud/hud", false);

		if (!pMaterial)
			return;

		m_pMaterial = pMaterial;

		IRenderer* pRenderer = gEnv->pRenderer;
		const int subMtlCount = pMaterial->GetSubMtlCount();
		for (int i = 0; i != subMtlCount; ++i)
		{
			IMaterial* pSubMat = pMaterial->GetSubMtl(i);
			if (!pSubMat)
				continue;

			IFlashPlayer* pFlashPlayer = 0;
			const SShaderItem& shaderItem(pSubMat->GetShaderItem());
			if (shaderItem.m_pShaderResources)
			{
				SEfResTexture* pTex(shaderItem.m_pShaderResources->GetTexture(0));
				if (pTex && pTex->m_Sampler.m_pDynTexSource)
				{
					IDynTextureSource::EDynTextureSource type(IDynTextureSource::DTS_I_FLASHPLAYER);

					pTex->m_Sampler.m_pDynTexSource->GetDynTextureSource((void*&)pFlashPlayer, type);
					if (pFlashPlayer && type == IDynTextureSource::DTS_I_FLASHPLAYER)
					{
						SAFE_DELETE(s_pHUDAssetUnified3D);
						CFlashAsset* pAsset = new CFlashAsset(pFlashPlayer);
						s_pHUDAssetUnified3D = static_cast<IHUDAsset*>(pAsset);
						break;
					}
				}
			}
		}
	}
}



void CHUD_UnifiedAsset::OnHUDEvent(const SHUDEvent& event)
{
	switch (event.eventType)
	{
	case eHUDEvent_OnResolutionChange:
		{
			const float width =	event.GetData(0).GetFloat();
			const float height = event.GetData(1).GetFloat();

			const float relWidth = ( DEFAULT_HEIGHT / height ) * width;
			m_offset3D = ( (relWidth / DEFAULT_WIDTH) - 1.0f ) * X_OFFSET_MODIFIER;

			SHUDEvent realignRadarTextureEvent(eHUDEvent_AlignToRadarAsset);
			realignRadarTextureEvent.AddData(s_pHUDAssetUnified3D);
			CHUD::CallEvent(realignRadarTextureEvent);
		}
		break;
	case eHUDEvent_OnHUDReload:
		{
			Destroy3DObjects();
		}
		break;
	case eHUDEvent_OnPhysicalImpulse:
		{
			Vec3 impulse(event.GetData(0).GetFloat(), event.GetData(1).GetFloat(), event.GetData(2).GetFloat());
			OnImpulse(impulse);
		}
		break;
	case eHUDEvent_OnInitPlayer:
		{
			if(!m_objectsValid)
			{
				Create3DObjects();
				Show(true);
			}
		}
		break;
	case eHUDEvent_HUDElementVisibility:
		{
			const int size = event.GetDataSize();
			if(size>1)
			{
				const bool show = event.GetData(0).GetBool();
				for(int i=1; i<size; ++i)
				{
					ShowElement(event.GetData(i).GetInt(), show);
				}
			}
		}
		break;
	default:
		{
			CRY_ASSERT(event.eventType==eHUDEvent_OnViewUpdate);
			UpdateView(static_cast<SViewParams*>(event.GetData(0).GetPtr()));
		}
		break;
	}
}



void CHUD_UnifiedAsset::Update(float frameTime)
{
	s_pHUDAssetUnified2D->Update(frameTime);

	Vec3 inpulse = m_impulse;
	Vec3 position = m_additionalMovement;

	inpulse *= PHYSICAL_IMPULSE_DAMPING;

	position += inpulse;

	position *= PHYSICAL_IMPULSE_DAMPING;

	m_impulse = inpulse;
	m_additionalMovement = position;
}



void CHUD_UnifiedAsset::Draw()
{
	s_pHUDAssetUnified2D->Draw();
}



void CHUD_UnifiedAsset::UpdateView(SViewParams* pViewParams)
{
	Vec3 pos = pViewParams->position;
	Quat rot = pViewParams->rotation;

	//TODO: calculate some kind of offset depending on the last frame values to simulate hud movement

	if(m_previousRotation.IsValid())
	{
		Quat diff = m_previousRotation / rot;

		diff.w *= 4.0f;
		diff.Normalize();

		rot *= !diff;
	}

	m_previousPosition = pos;
	m_previousRotation = rot;

	const Vec3 forward(rot.GetColumn1());
	const Vec3 up(rot.GetColumn2());
	const Vec3 right(-(up % forward));

	Vec3 add = m_additionalMovement;

	//rotate the additional hud movement vector according to our current view dir
	Matrix33 rotMat;
	rotMat.SetIdentity();
	rotMat.SetRotationV0V1(Vec3(0.0f, 1.0f, 0.0f), forward);
	add = rotMat.TransformVector(add);

	pos += add;

	const Vec3 offsetX = right * m_offset3D;

	ScreenLayoutManager* pLayoutMgr = g_pGame->GetHUD()->GetLayoutManager();
	EHUDSafeAreaID safeZoneId = pLayoutMgr->GetCurSafeAreaId();
	switch(safeZoneId)
	{
	case eHSAID_360_tcr:
		pos += (forward*DEFAULT_DEPTH_XBOX);
		break;
	case eHSAID_PS3_trc:
		pos += (forward*DEFAULT_DEPTH_PS3);
		break;
	default:
		pos += (forward*DEFAULT_DEPTH_PC);
	}

	const Vec3 center_front = pos;
	const Vec3 left_front = pos - offsetX;
	const Vec3 right_front = pos + offsetX;

	pos += (forward*0.001f);
	const Vec3 center_back = pos;
	const Vec3 left_back = pos - offsetX;
	const Vec3 right_back = pos + offsetX;

	const int size = m_elements.size();
	for(int i=0; i<size; ++i)
	{
		SHUDElement& element = m_elements[i];
		if(element.m_entity == 0)
			continue;

		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(element.m_entity);
		if(!pEntity)
			continue;

		switch(element.m_position)
		{
		case eHUDPosition_left_front:
			pEntity->SetPos(left_front);
			break;
		case eHUDPosition_right_front:
			pEntity->SetPos(right_front);
			break;
		case eHUDPosition_left_back:
			pEntity->SetPos(left_back);
			break;
		case eHUDPosition_right_back:
			pEntity->SetPos(right_back);
			break;
		case eHUDPosition_center_back:
			pEntity->SetPos(center_back);
			break;
		default:
			pEntity->SetPos(center_front);
			break;
		}
		pEntity->SetRotation(rot);
	}
}



void CHUD_UnifiedAsset::InitObject(const THUDElement element, const EHUDPosition position, const char* model)
{
	SHUDElement hudElement;
	hudElement.m_element = element;
	hudElement.m_position = position;
	hudElement.m_model = model;
	hudElement.m_entity = 0;
	hudElement.m_visible = 1;
	m_elements.push_back(hudElement);
}



void CHUD_UnifiedAsset::LoadObject(SHUDElement& element)
{
	SEntitySpawnParams spawnParams;
	CryFixedStringT<16> name;
	name.Format("HUD%d",(int)element.m_element);
	spawnParams.sName = name.c_str();
	spawnParams.nFlags = ENTITY_FLAG_CLIENT_ONLY | ENTITY_FLAG_NO_PROXIMITY;
	spawnParams.pClass = s_HUDObjectClass;
	if (spawnParams.pClass)
	{
		IEntity* pHUDEntity = gEnv->pEntitySystem->SpawnEntity(spawnParams, true);
		if(pHUDEntity)
		{

			IScriptTable *pScriptTable = pHUDEntity->GetScriptTable();
			assert(pScriptTable);

			Script::CallMethod(pScriptTable, "SetObjectModel", element.m_model);

			uint32 flags = pHUDEntity->GetSlotFlags(0);
			flags |= ENTITY_SLOT_RENDER_NEAREST | ENTITY_SLOT_RENDER_AFTER_POSTPROCESSING;
			pHUDEntity->SetSlotFlags(0, flags);

			IEntityRenderProxy *pRenderProxy = (IEntityRenderProxy*)pHUDEntity->GetProxy(ENTITY_PROXY_RENDER); 
			IRenderNode * pRenderNode = pRenderProxy?pRenderProxy->GetRenderNode():NULL;
			if(pRenderNode)
			{
				pRenderNode->SetRndFlags(ERF_RENDER_ALWAYS, true);
			}

			element.m_entity = pHUDEntity->GetId();
		}
	}
}



void CHUD_UnifiedAsset::Create3DObjects()
{
	const int size = m_elements.size();
	for(int i=0; i<size; ++i)
	{
		LoadObject(m_elements[i]);
	}
	m_objectsValid = true;
}



void CHUD_UnifiedAsset::Destroy3DObjects()
{
	const int size = m_elements.size();
	for(int i=0; i<size; ++i)
	{
		SAFE_REMOVE_ENTITY(m_elements[i].m_entity);
	}
	m_objectsValid = false;
}



void CHUD_UnifiedAsset::Show(const bool show)
{
	const bool shouldShow = show && g_pGame->GetHUD()->ShouldShowHUD();

	const int size = m_elements.size();
	for(int i=0; i<size; ++i)
	{
		SHUDElement& element = m_elements[i];
		if(element.m_entity == 0)
			continue;

		if(IEntity* pEntity = gEnv->pEntitySystem->GetEntity(element.m_entity))
		{
			pEntity->Hide(shouldShow ? !element.m_visible : true);
		}
	}
	m_show = shouldShow;
}



void CHUD_UnifiedAsset::ShowElement(const int index, const bool show)
{
	if(index<0 || index >=m_elements.size())
		return;

	SHUDElement& element = m_elements[index];
	int8 visible = element.m_visible;
	if(show)
	{
		visible++;
	}
	else
	{
		visible--;
	}

	if(element.m_entity != 0)
	{
		if(IEntity* pEntity = gEnv->pEntitySystem->GetEntity(element.m_entity))
		{
			pEntity->Hide(m_show ? visible<=0 : true);
		}
	}
	element.m_visible = visible;
}



void CHUD_UnifiedAsset::OnImpulse(const Vec3& impulse)
{
	m_impulse += impulse;
}


//////////////////////////////////////////////////////////////////////////
