#include "StdAfx.h"
#include "WarningsManager.h"

#include "HUD/HUDAsset.h"
#include "Flash/Flash.h"
#include "HUD/HUD.h"
#include "Graphics/2DRenderUtils.h"
#include "GameActions.h"
#include "HUD/ScreenLayoutManager.h"
#include "ItemSystem.h"


static const float	POPUP_DISTANCE = 20.0f;
static const int		MAX_NUM_POPUPS = 32;

static TKeyValuePair<EHUDWarningIcon,const char*>
gWarningIcons[] = {
	{eHUDWI_none,"empty"},
	{eHUDWI_attention,"attention"},
	{eHUDWI_question,"question"},
	{eHUDWI_loading,"loading"}
};

//////////////////////////////////////////////////////////////////////////


CWarningsManager::CWarningsManager()
{
	SAFE_HARDWARE_MOUSE_FUNC(AddListener(this));
	m_warnings.reserve(MAX_NUM_POPUPS);
	m_stack.reserve(MAX_NUM_POPUPS);
	m_remove.reserve(MAX_NUM_POPUPS);
	LoadWarnings();
}



CWarningsManager::~CWarningsManager()
{
	SAFE_HARDWARE_MOUSE_FUNC(RemoveListener(this));
}



bool CWarningsManager::IsActive() const
{
	return m_stack.size() > 0;
}



void CWarningsManager::OnHardwareMouseEvent(int iX,int iY, EHARDWAREMOUSEEVENT eHardwareMouseEvent, int wheelDelta)
{
	if(!IsActive())
		return;

	IFlashPlayer* pCurrent = m_stack.back().m_pPlayer;

	SFlashCursorEvent::ECursorState eCursorState = SFlashCursorEvent::eCursorMoved;
	if(HARDWAREMOUSEEVENT_LBUTTONDOWN == eHardwareMouseEvent)
		eCursorState = SFlashCursorEvent::eCursorPressed;
	else if(HARDWAREMOUSEEVENT_LBUTTONUP == eHardwareMouseEvent)
		eCursorState = SFlashCursorEvent::eCursorReleased;

	int x(iX), y(iY);
	pCurrent->ScreenToClient(x,y);
	pCurrent->SendCursorEvent(SFlashCursorEvent(eCursorState,x,y));
}


void CWarningsManager::HandleFSCommand(const char* pCommand, const char* pArgs, void* pUserData)
{
	if(!IsActive())
		return;

	int intId = (int32)atoi(pCommand);
	THUDWarningId id = (THUDWarningId)intId;

	CryLogAlways("FSCommand: '%d' '%s'", id, pArgs);

	const SHUDWarningInstance* pWarningInstance = GetWarningInstance(id);
	if(!pWarningInstance)
		return;

	bool remove = true;

	if(pWarningInstance->m_pListener)
		remove = pWarningInstance->m_pListener->OnWarningReturn(id, pArgs);

	if (remove)
		RemoveWarning(id);
}



void CWarningsManager::OnInputEvent(const SInputEvent &rInputEvent)
{
	if (rInputEvent.state==eIS_Pressed)
	{
		SHUDWarningInstance instance = m_stack.back();
		if(	rInputEvent.keyId == eKI_XI_A ||
				rInputEvent.keyId == eKI_XI_Start ||
				rInputEvent.keyId == eKI_PS3_Cross ||
				rInputEvent.keyId == eKI_PS3_Start ||
				rInputEvent.keyId == eKI_Enter ||
				rInputEvent.keyId == eKI_NP_Enter
				)
			instance.m_pPlayer->Invoke1("inputEvent", 0);
		else if(	rInputEvent.keyId == eKI_XI_B ||
							rInputEvent.keyId == eKI_PS3_Circle ||
							rInputEvent.keyId == eKI_Escape
							)
			instance.m_pPlayer->Invoke1("inputEvent", 1);
	}
}



void CWarningsManager::Update(float frameTime)
{
	if(!IsActive())
		return;

	TRemoveSchedule::const_iterator removeIt = m_remove.begin();
	TRemoveSchedule::const_iterator removeEnd = m_remove.end();

	for(; removeIt!=removeEnd; ++removeIt)
	{
		DeleteWarning((THUDWarningId)*removeIt);
	}

	m_remove.clear();

	TWarningsStack::const_iterator it = m_stack.begin();
	TWarningsStack::const_iterator end = m_stack.end();
	for(; it!=end; ++it)
	{
		it->m_pPlayer->Advance(frameTime);
	}

	Draw();
}



void CWarningsManager::Draw()
{
	if(!IsActive())
		return;

	CRY_TODO(4,11,2009, "Lowtec: Darken the background of Popup Warnings in a more nice way.");
	g_pGame->GetHUD()->Get2DRenderUtils()->PreRender();
	ScreenLayoutStates previous = g_pGame->GetHUD()->GetLayoutManager()->GetState();
	g_pGame->GetHUD()->GetLayoutManager()->SetState(eSLO_FullScreen);
	g_pGame->GetHUD()->Get2DRenderUtils()->DrawQuad(0.0f, 0.0f, 800.0f, 600.0f, ColorF(0.0f, 0.0f, 0.0f, 0.8f));
	g_pGame->GetHUD()->GetLayoutManager()->SetState(previous);
	g_pGame->GetHUD()->Get2DRenderUtils()->PostRender();

	int renderWidth = gEnv->pRenderer->GetWidth();
	int renderHeight = gEnv->pRenderer->GetHeight();

	int size = m_stack.size();

	float startPointX = ((float)renderWidth - (float)size * POPUP_DISTANCE) * 0.5f;
	float startPointY = ((float)renderHeight - (float)size * POPUP_DISTANCE) * 0.5f;

	int count = 0;

	TWarningsStack::const_iterator it = m_stack.begin();
	TWarningsStack::const_iterator end = m_stack.end();
	for(; it!=end; ++it)
	{
		float halfWidth = it->m_pPlayer->GetWidth() * 0.5f;
		float halfHeight = it->m_pPlayer->GetHeight() * 0.5f;

		float step = (float)count * POPUP_DISTANCE;

		int x = int_round(startPointX + step - halfWidth);
		int y = int_round(startPointY + step - halfHeight);

		it->m_pPlayer->SetViewport(x, y, int_round(halfWidth*2.0f), int_round(halfHeight*2.0f));
		it->m_pPlayer->Render();

		++count;
	}
}



void CWarningsManager::AddWarning(const THUDWarningId id, IHUDWarningsListener* pListener, const char *paramMessage/* = NULL*/)
{
	CRY_ASSERT(m_stack.size()<MAX_NUM_POPUPS);

	const SHUDWarningInstance* pInstance = GetWarningInstance(id);
	if(pInstance)
	{
		CryLog("CWarningsManager::AddWarning failed to add, instance already exists");
		return;
	}

	const SHUDWarning* pWarning = GetWarning(id);
	if(!pWarning)
		return;
	
	SHUDWarningInstance instance;

	instance.m_id = id;
	instance.m_pListener = pListener;
	instance.m_pPlayer = CFlash::Get()->CreateSafeFlashPlayerInstance();
	instance.m_pPlayer->Load("Libs/UI/Menu/PopUp.gfx");
	instance.m_pPlayer->SetBackgroundAlpha(0.0f);
	instance.m_pPlayer->SetFSCommandHandler(this);
	instance.m_priority = pWarning->m_priority;

	CHUD* pHUD = g_pGame->GetHUD();

	CryFixedWStringT<32> title = pHUD->LocalizeStringW(pWarning->m_title.c_str());
	CryFixedWStringT<32> msg = pHUD->LocalizeStringW(pWarning->m_message.c_str(), paramMessage);
	CryFixedWStringT<32> btn1 = pHUD->LocalizeStringW(pWarning->m_btn1.c_str());
	CryFixedWStringT<32> btn2 = pWarning->m_btn2.empty() ? L"" : pHUD->LocalizeStringW(pWarning->m_btn2.c_str());

	const char* warningIcon = VALUE_BY_KEY(pWarning->m_icon, gWarningIcons);

	SFlashVarValue args[8] = {
		(int32)pWarning->m_id
		, title.c_str()
		, msg.c_str()
		, warningIcon
		, btn1.c_str()
		, pWarning->m_btn1return.c_str()
		, btn2.c_str()
		, pWarning->m_btn2return.c_str()
	};
	instance.m_pPlayer->Invoke("Set", args, 8);

	if(m_stack.size()<=0)
	{
		g_pGameActions->FilterNoMouse()->Enable(true);
	}
	m_stack.push_back(instance);
	std::sort(m_stack.begin(), m_stack.end());
}



void CWarningsManager::RemoveWarning(const THUDWarningId id)
{
	stl::push_back_unique(m_remove, id);
}



void CWarningsManager::DeleteWarning(const THUDWarningId id)
{
	if(!IsActive())
		return;

	TWarningsStack::iterator it = m_stack.begin();
	TWarningsStack::iterator end = m_stack.end();

	for(; it!=end; ++it)
	{
		SHUDWarningInstance& instance = (*it);
		if(instance.m_id == id)
		{
			if(instance.m_pListener)
				instance.m_pListener->OnWarningRemoved(instance.m_id);
			IFlashPlayer* pPlayer = instance.m_pPlayer;
			m_stack.erase(it);
			SAFE_RELEASE(pPlayer);
			break;
		}
	}

	if(m_stack.size()<=0)
	{
		if (g_pGameActions)
		{
			g_pGameActions->FilterNoMouse()->Enable(false);
		}
	}
}



void CWarningsManager::ClearCurrentWarnings()
{
	while(m_stack.size())
		DeleteWarning(m_stack.back().m_id);
	m_stack.clear();
}



const SHUDWarning* CWarningsManager::GetWarning(const THUDWarningId id) const
{
	TWarnings::const_iterator it = m_warnings.begin();
	TWarnings::const_iterator end = m_warnings.end();
	for(; it!=end; ++it)
	{
		if(it->m_id == id)
		{
			const SHUDWarning& warning = *it;
			return &warning;
		}
	}
	return NULL;
}



const SHUDWarningInstance* CWarningsManager::GetWarningInstance(const THUDWarningId id) const
{
	TWarningsStack::const_iterator it = m_stack.begin();
	TWarningsStack::const_iterator end = m_stack.end();
	for(; it!=end; ++it)
	{
		if(it->m_id == id)
			return &(*it);
	}
	return NULL;
}



void CWarningsManager::LoadWarnings()
{
	XmlNodeRef xml = GetISystem()->LoadXmlFile("Libs/UI/Warnings.xml");
	if(!xml)
	{
		CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "HUD: Failed to load HUD Objects xml file 'Libs/UI/Warnings.xml'");
		return;
	}

	m_warnings.clear();

	IItemParamsNode *paramNode = g_pGame->GetIGameFramework()->GetIItemSystem()->CreateParams();
	paramNode->ConvertFromXML(xml);
	const int childXmlNodeCount = paramNode->GetChildCount();
	for(int i = 0; i < childXmlNodeCount; ++i)
	{
		const IItemParamsNode* child = paramNode->GetChild(i);
		if( 0 == stricmp("Warning", child->GetName() ) )
		{

			const char* stringId = child->GetAttribute( "id" );
			if(!stringId)
				continue;

			SHUDWarning warning;
			warning.m_id = CryStringUtils::CalculateHash(stringId);
			warning.m_title = child->GetAttribute( "title" );
			warning.m_message = child->GetAttribute( "message" );
			warning.m_btn1 = child->GetAttribute( "button1" );
			warning.m_btn1return = child->GetAttribute( "button1return" );
			warning.m_btn2 = child->GetAttribute( "button2" );
			warning.m_btn2return = child->GetAttribute( "button2return" );
			child->GetAttribute("priority", warning.m_priority);

			if (const char *icon = child->GetAttribute( "icon" ))
			{
				warning.m_icon = KEY_BY_VALUE(icon, gWarningIcons);
			}

			m_warnings.push_back(warning);
		}
	}
}

void CWarningsManager::GetMemoryUsage( ICrySizer *pSizer ) const
{
	pSizer->AddObject(m_warnings);
	pSizer->AddObject(m_stack);
	pSizer->AddObject(m_remove);
}

//////////////////////////////////////////////////////////////////////////
