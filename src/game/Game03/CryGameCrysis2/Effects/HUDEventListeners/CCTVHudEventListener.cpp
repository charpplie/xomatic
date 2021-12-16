//==================================================================================================
// Name: CCCTVHudEventListener
// Desc: Renders CCTV Hud items
// Author: James Chilvers
//==================================================================================================

// Includes
#include "StdAfx.h"
#include "CCTVHudEventListener.h"
#include "HUD/HUD.h"
#include "Graphics/2DRenderUtils.h"
#include <IItemSystem.h>

//--------------------------------------------------------------------------------------------------
// Name: CGameEffectsSystem
// Desc: Constructor
//--------------------------------------------------------------------------------------------------
CCCTVHudEventListener::CCCTVHudEventListener()
{
	m_maxFlashTime = 0.0f;
	m_flashTimer = 0.0f;
	m_cameraEntityId = 0;
	m_newCameraEntityId = 0;
	m_font = NULL;
	strcpy_s(m_cameraName,MAX_CCTV_HUD_TEXT,"DEFAULT CAMERA");
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: ~CGameEffectsSystem
// Desc: Destructor
//--------------------------------------------------------------------------------------------------
CCCTVHudEventListener::~CCCTVHudEventListener()
{
	for(uint i=0; i<m_spriteData.Size(); i++)
	{
		SAFE_RELEASE(m_spriteData[i].texture);
	}
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Initialise
// Desc: Initializes HUD event listener
//--------------------------------------------------------------------------------------------------
void CCCTVHudEventListener::Initialise(const IItemParamsNode* xmlNode)
{
	CRY_ASSERT_MESSAGE(xmlNode,"Missing xml data for cctv hud listener");

	if(xmlNode && gEnv->pRenderer)
	{
		xmlNode->GetAttribute("flashTiming",m_maxFlashTime);

		const IItemParamsNode* spriteItemsXmlNode = xmlNode->GetChild("SpriteItems");
		if(spriteItemsXmlNode)
		{
			const IItemParamsNode* spriteXmlNode = NULL;
			uint spriteCount = spriteItemsXmlNode->GetChildCount();
			m_spriteData.resize(spriteCount);
			if(spriteCount > 0)
			{
				CCCTVSpriteData* spriteData = NULL;
				for(uint i=0; i<spriteCount; i++)
				{
					spriteXmlNode = spriteItemsXmlNode->GetChild(i);
					spriteData = &m_spriteData[i];
					spriteData->texture = gEnv->pRenderer->EF_LoadTexture(spriteXmlNode->GetAttribute("texture"),FT_DONT_RESIZE|FT_DONT_STREAM|FT_STATE_CLAMP,eTT_2D);
					CRY_ASSERT_MESSAGE(m_spriteData[i].texture,"Failed to find texture for CCTV hud");
					if(spriteData->texture)
					{
						spriteData->texture->SetClamp(true);
					}
					
					spriteXmlNode->GetAttribute("posX",spriteData->pos.x);
					spriteXmlNode->GetAttribute("posY",spriteData->pos.y);
					spriteXmlNode->GetAttribute("width",spriteData->size.x);
					spriteXmlNode->GetAttribute("height",spriteData->size.y);

					spriteXmlNode->GetAttribute("red",spriteData->color.r);
					spriteXmlNode->GetAttribute("green",spriteData->color.g);
					spriteXmlNode->GetAttribute("blue",spriteData->color.b);
					spriteXmlNode->GetAttribute("alpha",spriteData->color.a);

					int fullScreen = 0;
					spriteXmlNode->GetAttribute("fullScreen",fullScreen);
					spriteData->fullScreen = (fullScreen>0) ? true : false;
					if(spriteData->fullScreen)
					{
						spriteData->pos.set(0.0f,0.0f);
						spriteData->size.set(800.0f,600.0f);
					}

					int flashes = 0;
					spriteXmlNode->GetAttribute("flashes",flashes);
					spriteData->flashes = (flashes>0) ? true : false;
				}
			}
		}

		const IItemParamsNode* textItemsXmlNode = xmlNode->GetChild("TextItems");
		if(textItemsXmlNode)
		{
			const IItemParamsNode* textXmlNode = NULL;
			uint textCount = textItemsXmlNode->GetChildCount();
			m_textData.resize(textCount);
			if(textCount > 0)
			{
				CCCTVTextData* textData = NULL;
				for(uint i=0; i<textCount; i++)
				{
					textXmlNode = textItemsXmlNode->GetChild(i);
					textData = &m_textData[i];

					const char* text = textXmlNode->GetAttribute("text");
					if(text)
					{
						strcpy_s(textData->text ,MAX_CCTV_HUD_TEXT,text);
					}

					textXmlNode->GetAttribute("posX",textData->pos.x);
					textXmlNode->GetAttribute("posY",textData->pos.y);
					textXmlNode->GetAttribute("width",textData->size.x);
					textXmlNode->GetAttribute("height",textData->size.y);
					textXmlNode->GetAttribute("red",textData->color.r);
					textXmlNode->GetAttribute("green",textData->color.g);
					textXmlNode->GetAttribute("blue",textData->color.b);
					textXmlNode->GetAttribute("alpha",textData->color.a);

					int isCameraName = 0;
					textXmlNode->GetAttribute("isCameraName",isCameraName);
					textData->isCameraName = (isCameraName>0) ? true : false;
				}
			}
		}
	}

	m_font = gEnv->pCryFont->GetFont("default");
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Register
// Desc: Registers HUD event listener with HUD
//--------------------------------------------------------------------------------------------------
void CCCTVHudEventListener::Register()
{
	CHUD* pHUD = g_pGame->GetHUD();
	if (pHUD)
	{
		pHUD->AddHUDEventListener(this, "OnPostHUDDraw");
	}
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: UnRegister
// Desc: UnRegisters HUD event listener with HUD
//--------------------------------------------------------------------------------------------------
void CCCTVHudEventListener::UnRegister()
{
	CHUD* pHUD = g_pGame->GetHUD();
	if (pHUD)
	{
		pHUD->RemoveHUDEventListener(this);
	}
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: OnHUDEvent
// Desc: Called on the event this listener is registered with
//--------------------------------------------------------------------------------------------------
void CCCTVHudEventListener::OnHUDEvent(const SHUDEvent& event)
{
	switch(event.eventType)
	{
		case eHUDEvent_OnPostHUDDraw:
		{
			Update(event.eventFloatData);
			Draw();
			break;
		}
	}
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Update
// Desc: Updates HUD item
//--------------------------------------------------------------------------------------------------
void CCCTVHudEventListener::Update(float frameTime)
{
	bool flash = false;
	m_flashTimer += frameTime;
	if(m_flashTimer > m_maxFlashTime)
	{
		m_flashTimer = 0.0f;
		flash = true;
	}

	uint spriteCount = m_spriteData.Size();
	CCCTVSpriteData* spriteData = NULL;
	for(uint i=0; i<spriteCount; i++)
	{
		spriteData = &m_spriteData[i];

		if(spriteData->flashes && flash)
		{
			spriteData->enabled = !spriteData->enabled;
		}
	}

	// Update camera name
	if(m_cameraEntityId != m_newCameraEntityId)
	{
		m_cameraEntityId = m_newCameraEntityId;

		IEntity* cameraEntity = gEnv->pEntitySystem->GetEntity(m_cameraEntityId);

		CHUD* pHUD = g_pGame->GetHUD();
		if(pHUD)
		{
			const char* newCameraName = pHUD->LocalizeString((cameraEntity ? cameraEntity->GetName() : "-"), NULL, NULL);
			strcpy_s(m_cameraName,MAX_CCTV_HUD_TEXT,newCameraName);
		}
	}
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: SetCameraEntityId
// Desc: Sets the camera entity ID
//--------------------------------------------------------------------------------------------------
void CCCTVHudEventListener::SetCameraEntityId(EntityId cameraEntityId)
{
	m_newCameraEntityId = cameraEntityId;
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Draw
// Desc: Draws HUD item
//--------------------------------------------------------------------------------------------------
void CCCTVHudEventListener::Draw()
{
	CHUD* pHUD = g_pGame->GetHUD();
	if (pHUD)
	{
		pHUD->Get2DRenderUtils()->PreRender();

		// Draw sprites
		uint spriteCount = m_spriteData.Size();
		CCCTVSpriteData* spriteData = NULL;
		for(uint i=0; i<spriteCount; i++)
		{
			spriteData = &m_spriteData[i];

			if(spriteData->texture && spriteData->enabled)
			{
				const float rotation = 0.0f;
				const float u0 = 0.0f;
				const float v0 = 1.0f;
				const float u1 = 1.0f;
				const float v1 = 0.0f;
				if(spriteData->fullScreen == false)
				{
					pHUD->Get2DRenderUtils()->DrawImage(	spriteData->texture->GetTextureID(),
																								spriteData->pos.x,
																								spriteData->pos.y,
																								spriteData->size.x,
																								spriteData->size.y,
																								rotation,
																								spriteData->color,
																								u0,
																								v0,
																								u1,
																								v1);
				}
				else
				{
					// The renderer draw image function doesn't take into account the safe area, so it works better for full screen
					gEnv->pRenderer->Draw2dImage(	spriteData->pos.x,
																				spriteData->pos.y,
																				spriteData->size.x,
																				spriteData->size.y,
																				spriteData->texture->GetTextureID(),
																				u0,
																				v0,
																				u1,
																				v1,
																				rotation,
																				spriteData->color.r,
																				spriteData->color.g,
																				spriteData->color.b,
																				spriteData->color.a );
				}
			}
		}

		// Draw text
		uint textCount = m_textData.Size();
		CCCTVTextData* textData = NULL;

		CHUD* pHUD = g_pGame->GetHUD();
		if(pHUD)
		{
			pHUD->Get2DRenderUtils()->SetFont(m_font);

			for(uint i=0; i<textCount; i++)
			{
				textData = &m_textData[i];

				const char* text = (textData->isCameraName) ? m_cameraName : textData->text;
				if(text)
				{
					pHUD->Get2DRenderUtils()->DrawText(	textData->pos.x,
																							textData->pos.y,
																							textData->size.x,
																							textData->size.y,
																							text,
																							textData->color,
																							UIDRAWHORIZONTAL_LEFT,
																							UIDRAWVERTICAL_TOP);
				}
			}
		}
		pHUD->Get2DRenderUtils()->PostRender();
	}
}//-------------------------------------------------------------------------------------------------
