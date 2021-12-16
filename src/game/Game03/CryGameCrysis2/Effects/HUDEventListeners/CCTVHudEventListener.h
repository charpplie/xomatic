#ifndef _CCTV_HUD_EVENT_LISTENER_
#define _CCTV_HUD_EVENT_LISTENER_

#pragma once

// Includes
#include "HUD/HUDEventDispatcher.h"

// Defines
const int MAX_CCTV_HUD_TEXT = 32;

//==================================================================================================
// Name: CCCTVSpriteData
// Desc: Sprite data for each sprite item
// Author: James Chilvers
//==================================================================================================
struct CCCTVSpriteData
{
	CCCTVSpriteData()
	{
		pos.set(0.0f,0.0f);
		size.set(0.0f,0.0f);
		color.Set(1.0f,1.0f,1.0f,1.0f);
		texture = NULL;
		enabled = true;
		flashes = false;
		fullScreen = false;
	}

	ColorF		color;
	Vec2			pos;
	Vec2			size;
	ITexture* texture;
	bool			enabled;
	bool			flashes;
	bool			fullScreen;
};//------------------------------------------------------------------------------------------------

//==================================================================================================
// Name: CCCTVTextData
// Desc: Text data for each text item
// Author: James Chilvers
//==================================================================================================
struct CCCTVTextData
{
	CCCTVTextData()
	{
		pos.set(0.0f,0.0f);
		size.set(0.0f,0.0f);
		color.Set(0.0f,0.0f,0.0f,0.0f);
		isCameraName = false;
		text[0] = 0;
	}

	char			text[MAX_CCTV_HUD_TEXT];
	ColorF		color;
	Vec2			pos;
	Vec2			size;
	bool			isCameraName;
};//------------------------------------------------------------------------------------------------

//==================================================================================================
// Name: CCCTVHudEventListener
// Desc: Renders CCTV Hud items
// Author: James Chilvers
//==================================================================================================
class CCCTVHudEventListener : public IHUDEventListener
{
public:
	CCCTVHudEventListener();
	~CCCTVHudEventListener();

	void Initialise(const IItemParamsNode* xmlNode);

	void Register();
	void UnRegister();

	void OnHUDEvent(const SHUDEvent& event);

	void SetCameraEntityId(EntityId cameraEntityId);

private:

	void Update(float frameTime);
	void Draw();

	PodArray<CCCTVSpriteData>		m_spriteData;
	PodArray<CCCTVTextData>			m_textData;
	char												m_cameraName[MAX_CCTV_HUD_TEXT];
	float												m_maxFlashTime;
	float												m_flashTimer;
	EntityId										m_cameraEntityId;
	EntityId										m_newCameraEntityId;
	IFFont*											m_font;
};//------------------------------------------------------------------------------------------------

#endif // _CCTV_HUD_EVENT_LISTENER_
